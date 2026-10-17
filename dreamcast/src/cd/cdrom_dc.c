/*----------------------
 | cdrom_dc.c
 | Description: Dreamcast implementation of the decomp's cdrom.h API: stdio reads of converted resources under LOM_CD_ROOT, keyed by PS1 resource index, with PS1 callback ordering for queued reads.
 | Author: suinevere
 | Dependencies: main/cdrom.h, main/cdrom_decompress.h, cdrom_dc.h, map_relocate.h, resource_index.h, overlay_loader_dc.h, dc_system.h
 | Globals: s_index_blob, g_cd_data_ready_pending, s_halt_message, s_pending_callback_read
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Pre-empts common.h's splat labels.inc/macro.inc inline asm (unneeded: nothing here calls INCLUDE_ASM/INCLUDE_RODATA) so make -C dreamcast's cwd does not have to resolve it. */
#define INCLUDE_ASM_H
#include "main/cdrom.h"
#include "main/cdrom_decompress.h"

#include "cdrom_dc.h"
#include "map_relocate.h"
#include "resource_index.h"
#include "overlay_loader_dc.h"
#include "dc_system.h"

/*----------------------
 | RESOURCE_INDEX_MAX
 | Description: Resource count the index blob reserves room for; mirrors resource_index.c's RESOURCE_INDEX_MAX and convert_disc.py's DREAMCAST_RESOURCE_INDEX_MAX. dreamcast/makefile overrides it with -D (6144, every live PS1 resource); the host test build leaves it at this default.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef RESOURCE_INDEX_MAX
#define RESOURCE_INDEX_MAX 256
#endif

/*----------------------
 | CDROM_INDEX_BLOB_RESOURCE_MAX
 | Description: Alias of RESOURCE_INDEX_MAX kept for the s_index_blob sizing below.
 | Author: suinevere
 | Dependencies: RESOURCE_INDEX_MAX
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CDROM_INDEX_BLOB_RESOURCE_MAX RESOURCE_INDEX_MAX

/*----------------------
 | CDROM_HALT_MESSAGE_MAX
 | Description: Byte size of the static buffer that holds one resource-naming halt message.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CDROM_HALT_MESSAGE_MAX 96

/*----------------------
 | s_index_blob
 | Description: The resource index blob read from LOM_CD_ROOT at init, keyed by PS1 resource index as resource_index.c and convert_disc.py pack it.
 | Author: suinevere
 | Dependencies: resource_index.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static u8 s_index_blob[4 + CDROM_INDEX_BLOB_RESOURCE_MAX * 8];

/*----------------------
 | g_cd_data_ready_pending
 | Description: Nonzero while a queued read's disc copy is outstanding; cdrom_process_state clears it once the copy and any deferred callback complete.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 g_cd_data_ready_pending;

/*----------------------
 | s_halt_message
 | Description: Static buffer cdrom_format_resource_halt writes a resource-naming halt message into.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static char s_halt_message[CDROM_HALT_MESSAGE_MAX];

/*----------------------
 | PendingCallbackRead
 | Description: A cdrom_queue_read_with_callback read whose disc copy already finished and whose caller callback is deferred to the next cdrom_process_state poll.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    int pending;
    CdCommandCallback callback;
    void *data;
    s32 size;
} PendingCallbackRead;

/*----------------------
 | s_pending_callback_read
 | Description: The single in-flight PendingCallbackRead; cdrom_dc.c queues reads one at a time, so one slot suffices.
 | Author: suinevere
 | Dependencies: PendingCallbackRead
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static PendingCallbackRead s_pending_callback_read;

/*----------------------
 | cdrom_format_resource_halt
 | Description: Writes prefix, the decimal resource index, then suffix into out, truncating to fit; a pure helper a host test can call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: out -- destination buffer, out_size -- buffer size in bytes, prefix -- text before the index, resource_index -- PS1 resource index, suffix -- text after the index
 | Returns: out
 ----------------------*/
char *cdrom_format_resource_halt(char *out, int out_size, const char *prefix, s32 resource_index, const char *suffix)
{
    int pos = 0;
    char digits[12];
    int digit_count = 0;
    u32 value;

    while (*prefix != 0 && pos < out_size - 1)
    {
        out[pos++] = *prefix++;
    }
    if (resource_index < 0)
    {
        if (pos < out_size - 1)
        {
            out[pos++] = '-';
        }
        value = (u32)0 - (u32)resource_index;
    }
    else
    {
        value = (u32)resource_index;
    }
    do
    {
        digits[digit_count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value > 0 && digit_count < (int)sizeof(digits));
    while (digit_count > 0 && pos < out_size - 1)
    {
        out[pos++] = digits[--digit_count];
    }
    while (*suffix != 0 && pos < out_size - 1)
    {
        out[pos++] = *suffix++;
    }
    out[pos] = 0;
    return out;
}

/*----------------------
 | dreamcast_res_subfolder_hex
 | Description: Writes the two-uppercase-hex-digit RES subfolder name for a PS1 resource index (index >> 8) into out; matches convert_disc.py's dreamcast_res_subfolder exactly.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: out -- 3-byte buffer, ps1_index -- PS1 resource index
 | Returns: out
 ----------------------*/
static char *dreamcast_res_subfolder_hex(char out[3], int ps1_index)
{
    static const char digits[] = "0123456789ABCDEF";
    unsigned v = (unsigned)(ps1_index >> 8) & 0xFF;

    out[0] = digits[(v >> 4) & 0xF];
    out[1] = digits[v & 0xF];
    out[2] = 0;
    return out;
}

/*----------------------
 | cd_open
 | Description: Opens a disc file for reading under LOM_CD_ROOT: at the root, in OVL, or in RES/<XX> (dreamcast_res_subfolder_hex of ps1_index), which the converter's resource_path fans every RES file into.
 | Author: suinevere
 | Dependencies: LOM_CD_ROOT, dreamcast_res_subfolder_hex
 | Globals: N/A
 | Params: folder -- CD_FOLDER_*, ps1_index -- PS1 resource index (ignored outside CD_FOLDER_RES), name -- file name
 | Returns: the open file, or 0
 ----------------------*/
static FILE *cd_open(int folder, int ps1_index, const char *name)
{
    char path[128];
    char sub[3];

    if (folder == CD_FOLDER_ROOT)
    {
        snprintf(path, sizeof(path), "%s/%s", LOM_CD_ROOT, name);
    }
    else if (folder == CD_FOLDER_OVL)
    {
        snprintf(path, sizeof(path), "%s/OVL/%s", LOM_CD_ROOT, name);
    }
    else
    {
        dreamcast_res_subfolder_hex(sub, ps1_index);
        snprintf(path, sizeof(path), "%s/RES/%s/%s", LOM_CD_ROOT, sub, name);
    }
    return fopen(path, "rb");
}

/*----------------------
 | dc_file_read
 | Description: Reads up to max_bytes of a disc file into dst.
 | Author: suinevere
 | Dependencies: cd_open
 | Globals: N/A
 | Params: folder -- CD_FOLDER_*, ps1_index -- PS1 resource index, name -- file name, dst -- destination, max_bytes -- destination capacity
 | Returns: bytes read, or -1 when the file cannot be opened
 ----------------------*/
static s32 dc_file_read(int folder, int ps1_index, const char *name, void *dst, s32 max_bytes)
{
    FILE *f = cd_open(folder, ps1_index, name);
    size_t n;

    if (f == 0)
    {
        return -1;
    }
    n = fread(dst, 1, (size_t)max_bytes, f);
    fclose(f);
    return (s32)n;
}

/*----------------------
 | read_resource
 | Description: Reads an exported resource's Dreamcast file into a destination; halts naming the index and both sizes when the file is missing or shorter than the index declares.
 | Author: suinevere
 | Dependencies: resource_folder, resource_file_name, dc_file_read, dc_halt
 | Globals: s_halt_message
 | Params: entry -- index entry, dst -- destination
 | Returns: bytes read
 ----------------------*/
static s32 read_resource(const ResourceEntry *entry, void *dst)
{
    char name[16];
    s32 n;

    resource_file_name(entry->index, entry->kind, name);
    n = dc_file_read(resource_folder(entry->kind), entry->index, name, dst, (s32)entry->size);
    if (n < 0)
    {
        n = 0;
    }
    if ((u32)n < entry->size)
    {
        snprintf(s_halt_message, sizeof(s_halt_message),
                 "cd: resource %d short: want %u got %d", entry->index, (unsigned)entry->size, n);
        dc_halt(s_halt_message);
    }
    return n;
}

/*----------------------
 | stream_raw_resource
 | Description: Reads a raw PS1 resource whole and decompresses it into the destination, as the PS1's cdrom_stream decompresses everything it streams.
 | Author: suinevere
 | Dependencies: read_resource, cdrom_decompress_buffer, dc_halt
 | Globals: N/A
 | Params: entry -- index entry of a KIND_RAW resource, destination -- decompressed output
 | Returns: N/A
 ----------------------*/
static void stream_raw_resource(const ResourceEntry *entry, u8 *destination)
{
    u8 *raw = malloc(entry->size != 0 ? entry->size : 1);

    if (raw == 0)
    {
        dc_halt("cd: stream out of memory");
    }
    read_resource(entry, raw);
    cdrom_decompress_buffer(raw, destination);
    free(raw);
}

/*----------------------
 | flush_pending_callback_read
 | Description: If a queued callback read is pending, hands its already-read bytes to the caller's callback (ask, then complete) and clears the pending slot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_pending_callback_read
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void flush_pending_callback_read(void)
{
    CdCommandCallback callback;
    void *data;
    s32 size;
    u8 *dst;

    if (!s_pending_callback_read.pending)
    {
        return;
    }
    callback = s_pending_callback_read.callback;
    data = s_pending_callback_read.data;
    size = s_pending_callback_read.size;
    s_pending_callback_read.pending = 0;
    s_pending_callback_read.data = 0;
    dst = callback(0, (u32)size);
    if (dst != 0)
    {
        memcpy(dst, data, (size_t)size);
    }
    free(data);
    callback(size, 0);
}

/*----------------------
 | cdrom_init
 | Description: Loads RESINDEX.BIN from the disc root; the Dreamcast disc needs no further drive setup.
 | Author: suinevere
 | Dependencies: dc_file_read, resource_index_load, dc_halt
 | Globals: s_index_blob
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void cdrom_init(void)
{
    s32 len = dc_file_read(CD_FOLDER_ROOT, 0, "RESINDEX.BIN", s_index_blob, (s32)sizeof(s_index_blob));

    if (len < 0 || resource_index_load(s_index_blob, len) < 0)
    {
        dc_halt("RESINDEX.BIN missing; run tools/convert_disc.py");
    }
}

/*----------------------
 | halt_resource_not_on_disc
 | Description: Halts naming a resource index this disc does not carry: either not listed in the index at all, or (outside cdrom_stream's SKIP tolerance) deliberately marked SKIP.
 | Author: suinevere
 | Dependencies: cdrom_format_resource_halt, dc_halt
 | Globals: s_halt_message
 | Params: resource_index -- PS1 resource
 | Returns: N/A (never returns)
 ----------------------*/
static void halt_resource_not_on_disc(s32 resource_index) __attribute__((noreturn));
static void halt_resource_not_on_disc(s32 resource_index)
{
    dc_halt(cdrom_format_resource_halt(s_halt_message, sizeof(s_halt_message),
                                        "resource ", resource_index, " not on disc"));
}

/*----------------------
 | cdrom_load_resource_table
 | Description: The resource index already replaces SKCDPOSE.DAT, so there is nothing to load.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: lba, data_size_bytes -- ignored
 | Returns: N/A
 ----------------------*/
void cdrom_load_resource_table(s32 lba, s32 data_size_bytes)
{
    (void)lba;
    (void)data_size_bytes;
}

/*----------------------
 | cdrom_stream
 | Description: Loads an overlay into its arena region, or decompresses a raw resource or copies a converted stream into the destination, rebasing a map's PS1 pointers.
 |              A missing index halts naming it; a skipped one silently returns instead, matching the Saturn since game code (e.g. the intro movie) expects some targets to omit it.
 | Author: suinevere
 | Dependencies: resource_index_find, load_overlay, stream_raw_resource, read_resource, map_relocate, halt_resource_not_on_disc, dc_log
 | Globals: N/A
 | Params: resource_index -- PS1 resource, destination -- load address for non-overlay resources
 | Returns: 0
 ----------------------*/
s32 cdrom_stream(s32 resource_index, u8 *destination)
{
    const ResourceEntry *entry = resource_index_find(resource_index);

    dc_log("cd stream %d", (int)resource_index);
    if (entry == 0)
    {
        halt_resource_not_on_disc(resource_index);
    }
    if (entry->kind == RESOURCE_KIND_SKIP)
    {
        return 0;
    }
    if (entry->kind == RESOURCE_KIND_OVERLAY)
    {
        load_overlay(resource_index);
    }
    else if (entry->kind == RESOURCE_KIND_RAW)
    {
        stream_raw_resource(entry, destination);
        map_relocate(destination, MAP_RELOCATE_MAX_BYTES);
    }
    else
    {
        read_resource(entry, destination);
        map_relocate(destination, MAP_RELOCATE_MAX_BYTES);
    }
    return 0;
}

/*----------------------
 | cdrom_queue_read
 | Description: Reads an exported resource, WMAP layout cells included, immediately, rebasing a map's PS1 pointers. A missing or skipped resource halts naming it, matching cdrom_stream.
 | Author: suinevere
 | Dependencies: resource_index_find, read_resource, map_relocate, halt_resource_not_on_disc, dc_log
 | Globals: N/A
 | Params: resource_index -- PS1 resource, dst_buffer -- destination
 | Returns: the resource's byte size
 ----------------------*/
s32 cdrom_queue_read(s32 resource_index, void *dst_buffer)
{
    const ResourceEntry *entry = resource_index_find(resource_index);
    s32 size;

    dc_log("cd read %d", (int)resource_index);
    if (entry == 0 || entry->kind == RESOURCE_KIND_SKIP)
    {
        halt_resource_not_on_disc(resource_index);
    }
    size = read_resource(entry, dst_buffer);
    map_relocate((u8 *)dst_buffer, (u32)size);
    return size;
}

/*----------------------
 | cdrom_queue_read_with_callback
 | Description: Copies the whole resource into a scratch buffer now, then defers both the buffer-request and completion callback calls to the next cdrom_process_state poll, so the game sees the PS1's order of events.
 | Author: suinevere
 | Dependencies: resource_index_find, read_resource, flush_pending_callback_read, halt_resource_not_on_disc
 | Globals: s_pending_callback_read
 | Params: resource_index -- PS1 resource, callback -- PS1 buffer callback
 | Returns: 0
 ----------------------*/
s32 cdrom_queue_read_with_callback(s32 resource_index, CdCommandCallback callback)
{
    const ResourceEntry *entry = resource_index_find(resource_index);
    void *data;

    if (entry == 0 || entry->kind == RESOURCE_KIND_SKIP)
    {
        halt_resource_not_on_disc(resource_index);
    }
    flush_pending_callback_read();
    data = malloc(entry->size != 0 ? entry->size : 1);
    if (data == 0)
    {
        dc_halt("cd: queue read out of memory");
    }
    read_resource(entry, data);
    s_pending_callback_read.pending = 1;
    s_pending_callback_read.callback = callback;
    s_pending_callback_read.data = data;
    s_pending_callback_read.size = (s32)entry->size;
    return 0;
}

/*----------------------
 | cdrom_get_resource_size
 | Description: Size of an exported resource's Dreamcast file.
 | Author: suinevere
 | Dependencies: resource_index_find
 | Globals: N/A
 | Params: resource_index -- PS1 resource
 | Returns: byte size, or 0 when not exported
 ----------------------*/
s32 cdrom_get_resource_size(s32 resource_index)
{
    const ResourceEntry *entry = resource_index_find(resource_index);

    return entry ? (s32)entry->size : 0;
}

/*----------------------
 | cdrom_queue_seek
 | Description: Reads are synchronous, so there is no head to position ahead of a read; returns what the PS1 queue does without reading.
 | Author: suinevere
 | Dependencies: cdrom_get_resource_size
 | Globals: N/A
 | Params: resource_index -- PS1 resource
 | Returns: byte size, or 0 when not exported
 ----------------------*/
s32 cdrom_queue_seek(s32 resource_index)
{
    return cdrom_get_resource_size(resource_index);
}

/*----------------------
 | cdrom_can_queue_resource
 | Description: Reads are synchronous, so the queue always has room.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: resource_index -- ignored
 | Returns: 1
 ----------------------*/
s32 cdrom_can_queue_resource(s32 resource_index)
{
    (void)resource_index;
    return 1;
}

/*----------------------
 | cdrom_process_state
 | Description: Services a pending cdrom_queue_read_with_callback read, if any, by invoking the caller's callback now.
 | Author: suinevere
 | Dependencies: flush_pending_callback_read
 | Globals: s_pending_callback_read
 | Params: N/A
 | Returns: 0
 ----------------------*/
u32 cdrom_process_state(void)
{
    flush_pending_callback_read();
    return 0;
}

/*----------------------
 | cdrom_get_error_status
 | Description: Dreamcast reads halt on failure instead of entering PS1 drive recovery, so a caller polling between reads always sees no error.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 0, PS1's CD_ERROR_STATUS_NONE
 ----------------------*/
s32 cdrom_get_error_status(void)
{
    return 0;
}

/*----------------------
 | cdrom_wait_queue_empty
 | Description: Reads complete before cdrom_queue_read(_with_callback) returns, so the queue is always empty.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void cdrom_wait_queue_empty(void)
{
}

/*----------------------
 | cdrom_stop
 | Description: No background CD activity to stop.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void cdrom_stop(void)
{
}

/*----------------------
 | cdrom_enter_recovery_mode
 | Description: Referenced only by CHECKPS's JP drive check; the Dreamcast has no PS1 drive to enter recovery on.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 0
 ----------------------*/
s32 cdrom_enter_recovery_mode(void)
{
    return 0;
}

/*----------------------
 | cdrom_recover
 | Description: Referenced only by CHECKPS's JP drive check; the Dreamcast has no PS1 drive to recover.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 0
 ----------------------*/
s32 cdrom_recover(void)
{
    return 0;
}
