/*----------------------
 | cdrom_dc.c
 | Description: Dreamcast implementation of the decomp's cdrom.h API: stdio reads of converted resources under LOM_CD_ROOT, keyed by PS1 resource index, with PS1 callback ordering for queued reads.
 | Author: suinevere
 | Dependencies: main/cdrom.h, cdrom_dc.h, resource_index.h, layout_pack.h, overlay_loader_dc.h, dc_system.h
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

#include "cdrom_dc.h"
#include "resource_index.h"
#include "layout_pack.h"
#include "overlay_loader_dc.h"
#include "dc_system.h"

/*----------------------
 | CDROM_INDEX_BLOB_RESOURCE_MAX
 | Description: Resource count the index blob reserves room for; mirrors resource_index.c's RESOURCE_INDEX_MAX and convert_disc.py's RESOURCE_INDEX_MAX.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CDROM_INDEX_BLOB_RESOURCE_MAX 256

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
 | CD_SECTOR_BYTES
 | Description: User-data bytes per disc sector, the stride unit for ResourcePack offsets.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CD_SECTOR_BYTES 2048

static u8 s_index_blob[4 + CDROM_INDEX_BLOB_RESOURCE_MAX * 8];

u8 g_cd_data_ready_pending;

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
 | cd_open
 | Description: Opens a disc file for reading under LOM_CD_ROOT, in its OVL/RES subfolder or directly at the root.
 | Author: suinevere
 | Dependencies: LOM_CD_ROOT
 | Globals: N/A
 | Params: folder -- CD_FOLDER_*, name -- file name
 | Returns: the open file, or 0
 ----------------------*/
static FILE *cd_open(int folder, const char *name)
{
    char path[80];

    if (folder == CD_FOLDER_ROOT)
    {
        snprintf(path, sizeof(path), "%s/%s", LOM_CD_ROOT, name);
    }
    else
    {
        snprintf(path, sizeof(path), "%s/%s/%s", LOM_CD_ROOT, folder == CD_FOLDER_OVL ? "OVL" : "RES", name);
    }
    return fopen(path, "rb");
}

/*----------------------
 | dc_file_read
 | Description: Reads up to max_bytes of a disc file into dst.
 | Author: suinevere
 | Dependencies: cd_open
 | Globals: N/A
 | Params: folder -- CD_FOLDER_*, name -- file name, dst -- destination, max_bytes -- destination capacity
 | Returns: bytes read, or -1 when the file cannot be opened
 ----------------------*/
static s32 dc_file_read(int folder, const char *name, void *dst, s32 max_bytes)
{
    FILE *f = cd_open(folder, name);
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
    n = dc_file_read(resource_folder(entry->kind), name, dst, (s32)entry->size);
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
 | read_pack_cell
 | Description: Reads one cell of a resource pack into the destination when the pack is read through this call; a wrong call, a missing pack or a failed read halts naming the id.
 | Author: suinevere
 | Dependencies: resource_index_find, resource_folder, resource_file_name, cd_open, cdrom_format_resource_halt, dc_halt
 | Globals: s_halt_message
 | Params: pack -- ResourcePack, cell -- pack cell, read_path -- RESOURCE_PACK_* of the caller, dst -- destination of pack->item_bytes bytes
 | Returns: pack->item_bytes
 ----------------------*/
static s32 read_pack_cell(const ResourcePack *pack, int cell, int read_path, void *dst)
{
    const ResourceEntry *entry = resource_index_find(pack->first);
    char name[16];
    FILE *f;
    size_t n;

    if (pack->read_path != read_path)
    {
        dc_halt(cdrom_format_resource_halt(s_halt_message, sizeof(s_halt_message),
                                            "cd: resource ", pack->first + cell, " read through the wrong call"));
    }
    if (entry == 0)
    {
        dc_halt(cdrom_format_resource_halt(s_halt_message, sizeof(s_halt_message),
                                            "cd: pack ", pack->first, " not exported"));
    }
    resource_file_name(entry->index, entry->kind, name);
    f = cd_open(resource_folder(entry->kind), name);
    if (f == 0 || fseek(f, (long)cell * pack->stride_sectors * CD_SECTOR_BYTES, SEEK_SET) != 0)
    {
        dc_halt(cdrom_format_resource_halt(s_halt_message, sizeof(s_halt_message),
                                            "cd: resource ", pack->first + cell, " read failed"));
    }
    n = fread(dst, 1, (size_t)pack->item_bytes, f);
    fclose(f);
    if (n != (size_t)pack->item_bytes)
    {
        dc_halt(cdrom_format_resource_halt(s_halt_message, sizeof(s_halt_message),
                                            "cd: resource ", pack->first + cell, " read failed"));
    }
    return pack->item_bytes;
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
    s32 len = dc_file_read(CD_FOLDER_ROOT, "RESINDEX.BIN", s_index_blob, (s32)sizeof(s_index_blob));

    if (len < 0 || resource_index_load(s_index_blob, len) < 0)
    {
        dc_halt("RESINDEX.BIN missing; run tools/convert_disc.py");
    }
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
 | Description: Reads a stream pack's cell, loads an overlay into its arena region, or reads a resource into the destination; a missing resource halts naming it, but a skipped one (not on this disc) silently returns, matching the Saturn exactly since game code (e.g. the intro movie) calls this for resources some targets omit.
 | Author: suinevere
 | Dependencies: resource_pack_find, read_pack_cell, resource_index_find, load_overlay, read_resource, cdrom_format_resource_halt, dc_halt
 | Globals: s_halt_message
 | Params: resource_index -- PS1 resource, destination -- load address for non-overlay resources
 | Returns: 0
 ----------------------*/
s32 cdrom_stream(s32 resource_index, u8 *destination)
{
    int cell;
    const ResourcePack *pack = resource_pack_find(resource_index, &cell);
    const ResourceEntry *entry;

    if (pack != 0)
    {
        read_pack_cell(pack, cell, RESOURCE_PACK_STREAM, destination);
        return 0;
    }
    entry = resource_index_find(resource_index);
    if (entry == 0)
    {
        dc_halt(cdrom_format_resource_halt(s_halt_message, sizeof(s_halt_message),
                                            "cdrom_stream: resource ", resource_index, " not exported"));
    }
    if (entry->kind == RESOURCE_KIND_SKIP)
    {
        return 0;
    }
    if (entry->kind == RESOURCE_KIND_OVERLAY)
    {
        load_overlay(resource_index);
    }
    else
    {
        read_resource(entry, destination);
    }
    return 0;
}

/*----------------------
 | cdrom_queue_read
 | Description: Reads a queue-read pack's cell (a WMAP layout), otherwise reads an exported resource immediately. A missing or skipped resource halts naming it, matching cdrom_stream.
 | Author: suinevere
 | Dependencies: resource_pack_find, read_pack_cell, resource_index_find, read_resource, cdrom_format_resource_halt, dc_halt
 | Globals: s_halt_message
 | Params: resource_index -- PS1 resource, dst_buffer -- destination
 | Returns: the resource's byte size, the pack's item size for a pack cell
 ----------------------*/
s32 cdrom_queue_read(s32 resource_index, void *dst_buffer)
{
    int cell;
    const ResourcePack *pack = resource_pack_find(resource_index, &cell);
    const ResourceEntry *entry;

    if (pack != 0)
    {
        return read_pack_cell(pack, cell, RESOURCE_PACK_QUEUE_READ, dst_buffer);
    }
    entry = resource_index_find(resource_index);
    if (entry == 0 || entry->kind == RESOURCE_KIND_SKIP)
    {
        dc_halt(cdrom_format_resource_halt(s_halt_message, sizeof(s_halt_message),
                                            "cd: resource ", resource_index, " not exported"));
    }
    return read_resource(entry, dst_buffer);
}

/*----------------------
 | cdrom_queue_read_with_callback
 | Description: Copies the whole resource into a scratch buffer now, then defers both the buffer-request and completion callback calls to the next cdrom_process_state poll, so the game sees the PS1's order of events.
 | Author: suinevere
 | Dependencies: resource_index_find, read_resource, flush_pending_callback_read, cdrom_format_resource_halt, dc_halt
 | Globals: s_halt_message, s_pending_callback_read
 | Params: resource_index -- PS1 resource, callback -- PS1 buffer callback
 | Returns: 0
 ----------------------*/
s32 cdrom_queue_read_with_callback(s32 resource_index, CdCommandCallback callback)
{
    const ResourceEntry *entry = resource_index_find(resource_index);
    void *data;

    if (entry == 0 || entry->kind == RESOURCE_KIND_SKIP)
    {
        dc_halt(cdrom_format_resource_halt(s_halt_message, sizeof(s_halt_message),
                                            "cd: resource ", resource_index, " not exported"));
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
