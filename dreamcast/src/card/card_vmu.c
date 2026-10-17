/*----------------------
 | card_vmu.c
 | Description: Memory-card BIOS, libcard and libmcx calls answered as a PS1 memory card whose files live in a card_store (VMUs on the Dreamcast); with no store or no card a slot answers as empty.
 | Author: suinevere
 | Dependencies: libapi.h, libmcx.h, card_store.h, vmu_format.h, stdlib.h, string.h
 | Globals: s_store, s_event_descs, s_event_specs, s_event_count, s_posted, s_mcx_pending, s_mcx_result, s_files, s_handles, s_dir, s_dir_count, s_dir_valid, s_unreadable, s_search, s_search_count, s_search_pos, s_search_pattern
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdlib.h>
#include <string.h>
#include <libapi.h>
#include <libmcx.h>
#include "card_store.h"
#include "vmu_format.h"

/*----------------------
 | CARD_EVENT_MAX / CARD_EVENT_HANDLE_BASE
 | Description: Most events OpenEvent records, and the bit pattern of the handles it returns, like the BIOS's 0xF1000000-based descriptors.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CARD_EVENT_MAX 32
#define CARD_EVENT_HANDLE_BASE 0xF1000000L

/*----------------------
 | CARD_CLASS_SW / CARD_CLASS_HW / CARD_CLASSES
 | Description: Indices of the software (SwCARD) and hardware (HwCARD) event classes in s_posted.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CARD_CLASS_SW 0
#define CARD_CLASS_HW 1
#define CARD_CLASSES 2

/*----------------------
 | CARD_SLOTS / CARD_DIR_MAX / CARD_RAM_FILES / CARD_HANDLES / CARD_SECTOR_BYTES / CARD_SECTORS_PER_BLOCK
 | Description: Slot count, most directory entries per slot, RAM file and open-handle table sizes, and the PS1 card's sector geometry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CARD_SLOTS 2
#define CARD_DIR_MAX 16
#define CARD_RAM_FILES 8
#define CARD_HANDLES 4
#define CARD_SECTOR_BYTES 128
#define CARD_SECTORS_PER_BLOCK 64

/*----------------------
 | CARD_MODE_CREATE / CARD_MODE_ASYNC / CARD_MODE_BLOCKS
 | Description: Psy-Q open() mode bits: create, asynchronous, and the block count a create allocates.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CARD_MODE_CREATE 0x0200UL
#define CARD_MODE_ASYNC 0x8000UL
#define CARD_MODE_BLOCKS(mode) ((int)(((mode) >> 16) & 0xFF))

/*----------------------
 | CARD_FILLER_NAME / CARD_DIR_ATTR
 | Description: Name of the directory entry that stands for VMU space other data uses, and the attribute a file's first block carries.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CARD_FILLER_NAME "BADCVMU-OTHER"
#define CARD_DIR_ATTR 0x51

/*----------------------
 | card_ram_file
 | Description: A card file held in RAM while it is open, created but not yet stored, or written but not yet stored.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct card_ram_file
{
    int used;
    int slot;
    char name[CARD_STORE_NAME_BYTES];
    unsigned int size;
    unsigned char *data;
    int dirty;
    int stored;
    int open_count;
} card_ram_file;

/*----------------------
 | card_handle
 | Description: An open file: its RAM file, read/write position and whether its I/O posts completion events.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct card_handle
{
    card_ram_file *file;
    unsigned int pos;
    int async;
} card_handle;

/*----------------------
 | s_store
 | Description: The store card files live in, or NULL for empty slots.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const card_store_ops *s_store;

/*----------------------
 | s_event_descs / s_event_specs / s_event_count
 | Description: Class and spec of each event OpenEvent handed out, indexed by handle minus CARD_EVENT_HANDLE_BASE.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned long s_event_descs[CARD_EVENT_MAX];
static long s_event_specs[CARD_EVENT_MAX];
static int s_event_count;

/*----------------------
 | s_posted
 | Description: The outcome spec each event class has waiting for TestEvent, or 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static long s_posted[CARD_CLASSES];

/*----------------------
 | s_mcx_pending / s_mcx_result
 | Description: Whether an Mcx command awaits McxSync, and the result it will report.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_mcx_pending;
static long s_mcx_result;

/*----------------------
 | s_files / s_handles
 | Description: RAM files and open handles; a handle's fd is its index.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static card_ram_file s_files[CARD_RAM_FILES];
static card_handle s_handles[CARD_HANDLES];

/*----------------------
 | s_dir / s_dir_count / s_dir_valid
 | Description: Each slot's cached store listing and whether it is current.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static card_store_entry s_dir[CARD_SLOTS][CARD_DIR_MAX];
static int s_dir_count[CARD_SLOTS];
static int s_dir_valid[CARD_SLOTS];

/*----------------------
 | s_unreadable
 | Description: Names per slot whose stored file failed to read; they stay out of listings and fail to open without touching the VMU until written, erased or the card leaves.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static char s_unreadable[CARD_SLOTS][CARD_DIR_MAX][CARD_STORE_NAME_BYTES];

/*----------------------
 | s_search / s_search_count / s_search_pos / s_search_pattern
 | Description: The listing a firstfile search walks, where nextfile resumes, and the pattern it matches.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static card_store_entry s_search[CARD_DIR_MAX + CARD_RAM_FILES + 1];
static int s_search_count;
static int s_search_pos;
static char s_search_pattern[CARD_STORE_NAME_BYTES];

/*----------------------
 | card_vmu_set_store
 | Description: Installs the store card files live in and forgets every cached listing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_store, s_dir_valid
 | Params: ops -- store, or NULL for empty slots
 | Returns: N/A
 ----------------------*/
void card_vmu_set_store(const card_store_ops *ops)
{
    s_store = ops;
    s_dir_valid[0] = 0;
    s_dir_valid[1] = 0;
}

/*----------------------
 | slot_present
 | Description: Whether a slot has a store and a card in it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_store
 | Params: slot -- 0 or 1
 | Returns: nonzero when present
 ----------------------*/
static int slot_present(int slot)
{
    return s_store != NULL && slot >= 0 && slot < CARD_SLOTS && s_store->present(slot);
}

/*----------------------
 | chan_slot / path_slot
 | Description: Slot of a libcard channel (0x00 or 0x10) or of a "buX0:" path; -1 for a path naming no card device.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: chan -- channel, path -- card path
 | Returns: 0, 1 or -1
 ----------------------*/
static int chan_slot(long chan)
{
    return (int)((chan >> 4) & 1);
}

static int path_slot(const char *path)
{
    if (path == NULL || path[0] != 'b' || path[1] != 'u' || (path[2] != '0' && path[2] != '1') || path[4] != ':')
    {
        return -1;
    }
    return path[2] - '0';
}

/*----------------------
 | post
 | Description: Leaves an outcome waiting for the next TestEvent of that class.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_posted
 | Params: cls -- CARD_CLASS_SW or CARD_CLASS_HW, spec -- EvSp outcome
 | Returns: N/A
 ----------------------*/
static void post(int cls, long spec)
{
    s_posted[cls] = spec;
}

/*----------------------
 | post_absent
 | Description: Answers a request to an empty slot: both classes time out.
 | Author: suinevere
 | Dependencies: post
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void post_absent(void)
{
    post(CARD_CLASS_SW, EvSpTIMOUT);
    post(CARD_CLASS_HW, EvSpTIMOUT);
}

/*----------------------
 | post_done
 | Description: Answers a card-level request (info, clear, load) to a present card: like the PS1 BIOS, it completes on both classes, since CLOAD spins on the hardware one.
 | Author: suinevere
 | Dependencies: post
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void post_done(void)
{
    post(CARD_CLASS_SW, EvSpIOE);
    post(CARD_CLASS_HW, EvSpIOE);
}

/*----------------------
 | is_temp_name
 | Description: Whether a name is one of the game's temporary save names, which stay in RAM until renamed.
 | Author: suinevere
 | Dependencies: strstr
 | Globals: N/A
 | Params: name -- PS1 file name
 | Returns: nonzero for a temporary name
 ----------------------*/
static int is_temp_name(const char *name)
{
    return strstr(name, "DUMMY") != NULL || strstr(name, "dummy") != NULL;
}

/*----------------------
 | name_matches
 | Description: PS1 BIOS directory pattern match: '?' matches one character and '*' any run.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: pattern -- pattern, name -- file name
 | Returns: nonzero on a match
 ----------------------*/
static int name_matches(const char *pattern, const char *name)
{
    if (*pattern == 0)
    {
        return *name == 0;
    }
    if (*pattern == '*')
    {
        return name_matches(pattern + 1, name) || (*name != 0 && name_matches(pattern, name + 1));
    }
    if (*name == 0 || (*pattern != '?' && *pattern != *name))
    {
        return 0;
    }
    return name_matches(pattern + 1, name + 1);
}

/*----------------------
 | copy_name
 | Description: Copies a PS1 file name, cut to CARD_STORE_NAME_BYTES - 1 characters.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: dst -- destination, src -- source
 | Returns: N/A
 ----------------------*/
static void copy_name(char *dst, const char *src)
{
    int i;

    for (i = 0; i < CARD_STORE_NAME_BYTES - 1 && src[i] != 0; i++)
    {
        dst[i] = src[i];
    }
    dst[i] = 0;
}

/*----------------------
 | is_unreadable / mark_unreadable / clear_unreadable
 | Description: Test, set or clear a name's unreadable mark; clear_unreadable with a NULL name clears the whole slot.
 | Author: suinevere
 | Dependencies: copy_name
 | Globals: s_unreadable
 | Params: slot -- 0 or 1, name -- PS1 file name or NULL
 | Returns: is_unreadable returns nonzero when marked
 ----------------------*/
static int is_unreadable(int slot, const char *name)
{
    int i;

    for (i = 0; i < CARD_DIR_MAX; i++)
    {
        if (s_unreadable[slot][i][0] != 0 && strcmp(s_unreadable[slot][i], name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static void mark_unreadable(int slot, const char *name)
{
    int i;

    for (i = 0; i < CARD_DIR_MAX; i++)
    {
        if (s_unreadable[slot][i][0] == 0)
        {
            copy_name(s_unreadable[slot][i], name);
            return;
        }
    }
}

static void clear_unreadable(int slot, const char *name)
{
    int i;

    for (i = 0; i < CARD_DIR_MAX; i++)
    {
        if (name == NULL || strcmp(s_unreadable[slot][i], name) == 0)
        {
            s_unreadable[slot][i][0] = 0;
        }
    }
}

/*----------------------
 | refresh_dir
 | Description: Reloads a slot's listing from the store when it is out of date, leaving out names marked unreadable.
 | Author: suinevere
 | Dependencies: slot_present, is_unreadable
 | Globals: s_store, s_dir, s_dir_count, s_dir_valid
 | Params: slot -- 0 or 1, force -- reload even when current
 | Returns: N/A
 ----------------------*/
static void refresh_dir(int slot, int force)
{
    int count;
    int kept = 0;
    int i;

    if (s_dir_valid[slot] && !force)
    {
        return;
    }
    count = slot_present(slot) ? s_store->list(slot, s_dir[slot], CARD_DIR_MAX) : -1;
    for (i = 0; i < count; i++)
    {
        if (!is_unreadable(slot, s_dir[slot][i].ps1_name))
        {
            s_dir[slot][kept++] = s_dir[slot][i];
        }
    }
    s_dir_count[slot] = kept;
    s_dir_valid[slot] = count >= 0;
}

/*----------------------
 | find_ram / find_dir
 | Description: The RAM file, or the cached listing entry, for a slot and name.
 | Author: suinevere
 | Dependencies: refresh_dir
 | Globals: s_files, s_dir, s_dir_count
 | Params: slot -- 0 or 1, name -- PS1 file name
 | Returns: pointer, or NULL when absent
 ----------------------*/
static card_ram_file *find_ram(int slot, const char *name)
{
    int i;

    for (i = 0; i < CARD_RAM_FILES; i++)
    {
        if (s_files[i].used && s_files[i].slot == slot && strcmp(s_files[i].name, name) == 0)
        {
            return &s_files[i];
        }
    }
    return NULL;
}

static card_store_entry *find_dir(int slot, const char *name)
{
    int i;

    refresh_dir(slot, 0);
    for (i = 0; i < s_dir_count[slot]; i++)
    {
        if (strcmp(s_dir[slot][i].ps1_name, name) == 0)
        {
            return &s_dir[slot][i];
        }
    }
    return NULL;
}

/*----------------------
 | free_ram
 | Description: Releases a RAM file's bytes and its table slot.
 | Author: suinevere
 | Dependencies: free
 | Globals: N/A
 | Params: file -- RAM file
 | Returns: N/A
 ----------------------*/
static void free_ram(card_ram_file *file)
{
    free(file->data);
    file->data = NULL;
    file->used = 0;
}

/*----------------------
 | drop_slot_ram
 | Description: Releases a slot's RAM files that no handle holds, either all of them or only those already stored and unchanged.
 | Author: suinevere
 | Dependencies: free_ram
 | Globals: s_files
 | Params: slot -- 0 or 1, clean_only -- keep unstored or changed files
 | Returns: N/A
 ----------------------*/
static void drop_slot_ram(int slot, int clean_only)
{
    int i;

    for (i = 0; i < CARD_RAM_FILES; i++)
    {
        card_ram_file *file = &s_files[i];

        if (file->used && file->slot == slot && file->open_count == 0 && (!clean_only || (file->stored && !file->dirty)))
        {
            free_ram(file);
        }
    }
}

/*----------------------
 | new_ram
 | Description: Claims a RAM file of the given size, zero-filled.
 | Author: suinevere
 | Dependencies: calloc, copy_name
 | Globals: s_files
 | Params: slot -- 0 or 1, name -- PS1 file name, size -- bytes
 | Returns: RAM file, or NULL when the table or heap is full
 ----------------------*/
static card_ram_file *new_ram(int slot, const char *name, unsigned int size)
{
    int i;

    for (i = 0; i < CARD_RAM_FILES; i++)
    {
        if (!s_files[i].used)
        {
            s_files[i].data = (unsigned char *)calloc(size ? size : 1, 1);
            if (s_files[i].data == NULL)
            {
                return NULL;
            }
            s_files[i].used = 1;
            s_files[i].slot = slot;
            copy_name(s_files[i].name, name);
            s_files[i].size = size;
            s_files[i].dirty = 0;
            s_files[i].stored = 0;
            s_files[i].open_count = 0;
            return &s_files[i];
        }
    }
    return NULL;
}

/*----------------------
 | load_ram
 | Description: The RAM copy of a file, read from the store when it is not already in RAM; a failed read marks the name unreadable.
 | Author: suinevere
 | Dependencies: find_ram, find_dir, new_ram, free_ram, mark_unreadable
 | Globals: s_store, s_dir_valid
 | Params: slot -- 0 or 1, name -- PS1 file name
 | Returns: RAM file, or NULL when the file is absent or unreadable
 ----------------------*/
static card_ram_file *load_ram(int slot, const char *name)
{
    card_ram_file *file = find_ram(slot, name);
    card_store_entry *entry;

    if (file != NULL)
    {
        return file;
    }
    entry = find_dir(slot, name);
    if (entry == NULL || (file = new_ram(slot, name, entry->size)) == NULL)
    {
        return NULL;
    }
    if (s_store->read(slot, name, file->data, file->size) != (int)file->size)
    {
        free_ram(file);
        mark_unreadable(slot, name);
        s_dir_valid[slot] = 0;
        return NULL;
    }
    file->stored = 1;
    return file;
}

/*----------------------
 | store_ram
 | Description: Writes a RAM file to the store under its current name, clearing any unreadable mark it had.
 | Author: suinevere
 | Dependencies: slot_present, clear_unreadable
 | Globals: s_store, s_dir_valid
 | Params: file -- RAM file
 | Returns: 0 on success, -1 on failure
 ----------------------*/
static int store_ram(card_ram_file *file)
{
    if (!slot_present(file->slot) || s_store->write(file->slot, file->name, file->data, file->size) != 0)
    {
        return -1;
    }
    file->stored = 1;
    file->dirty = 0;
    clear_unreadable(file->slot, file->name);
    s_dir_valid[file->slot] = 0;
    return 0;
}

/*----------------------
 | build_listing
 | Description: Fills s_search with a slot's stored files, RAM-only files and, when the VMU's free space is short of the PS1 card's, one filler entry for the difference.
 | Author: suinevere
 | Dependencies: refresh_dir, find_dir, ps1_blocks_free
 | Globals: s_store, s_files, s_dir, s_dir_count, s_search, s_search_count
 | Params: slot -- 0 or 1
 | Returns: PS1 blocks the slot's files use, not counting the filler
 ----------------------*/
static int build_listing(int slot)
{
    int used_blocks = 0;
    int filler;
    int i;

    refresh_dir(slot, 0);
    s_search_count = 0;
    for (i = 0; i < s_dir_count[slot]; i++)
    {
        card_ram_file *file = find_ram(slot, s_dir[slot][i].ps1_name);

        s_search[s_search_count] = s_dir[slot][i];
        if (file != NULL)
        {
            s_search[s_search_count].size = file->size;
        }
        s_search_count++;
    }
    for (i = 0; i < CARD_RAM_FILES; i++)
    {
        if (s_files[i].used && s_files[i].slot == slot && find_dir(slot, s_files[i].name) == NULL)
        {
            copy_name(s_search[s_search_count].ps1_name, s_files[i].name);
            s_search[s_search_count].size = s_files[i].size;
            s_search_count++;
        }
    }
    for (i = 0; i < s_search_count; i++)
    {
        used_blocks += (int)((s_search[i].size + PS1_BLOCK_BYTES - 1) / PS1_BLOCK_BYTES);
    }
    filler = PS1_CARD_BLOCKS - used_blocks - ps1_blocks_free(s_store->free_blocks(slot), used_blocks);
    if (filler > 0)
    {
        copy_name(s_search[s_search_count].ps1_name, CARD_FILLER_NAME);
        s_search[s_search_count].size = (unsigned int)filler * PS1_BLOCK_BYTES;
        s_search_count++;
    }
    return used_blocks;
}

/*----------------------
 | listing_file
 | Description: The RAM copy of the file at a listing index, loading it when needed; the filler entry has none.
 | Author: suinevere
 | Dependencies: build_listing, load_ram
 | Globals: s_search, s_search_count
 | Params: slot -- 0 or 1, index -- listing index
 | Returns: RAM file, or NULL
 ----------------------*/
static card_ram_file *listing_file(int slot, int index)
{
    build_listing(slot);
    if (index < 0 || index >= s_search_count || strcmp(s_search[index].ps1_name, CARD_FILLER_NAME) == 0)
    {
        return NULL;
    }
    return load_ram(slot, s_search[index].ps1_name);
}

/*----------------------
 | OpenEvent
 | Description: Records the event's class and spec and returns a new handle; a full table returns the last handle again.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_event_descs, s_event_specs, s_event_count
 | Params: desc -- event class, spec -- event kind, mode -- delivery mode, func -- handler
 | Returns: event handle
 ----------------------*/
long OpenEvent(unsigned long desc, long spec, long mode, long (*func)())
{
    (void)mode;
    (void)func;
    if (s_event_count < CARD_EVENT_MAX)
    {
        s_event_descs[s_event_count] = desc;
        s_event_specs[s_event_count++] = spec;
    }
    return CARD_EVENT_HANDLE_BASE | (s_event_count - 1);
}

/*----------------------
 | CloseEvent / EnableEvent
 | Description: Accept any handle; events need no state beyond their class and spec.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: event -- event handle
 | Returns: 1
 ----------------------*/
long CloseEvent(long event)
{
    (void)event;
    return 1;
}

long EnableEvent(long event)
{
    (void)event;
    return 1;
}

/*----------------------
 | TestEvent
 | Description: Reports whether the event's class has its spec waiting, and takes it when so.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_event_descs, s_event_specs, s_event_count, s_posted
 | Params: event -- event handle
 | Returns: 1 when the event fired, else 0
 ----------------------*/
long TestEvent(long event)
{
    long index = event & ~CARD_EVENT_HANDLE_BASE;
    int cls;

    if (index < 0 || index >= s_event_count)
    {
        return 0;
    }
    if (s_event_descs[index] == SwCARD)
    {
        cls = CARD_CLASS_SW;
    }
    else if (s_event_descs[index] == HwCARD)
    {
        cls = CARD_CLASS_HW;
    }
    else
    {
        return 0;
    }
    if (s_posted[cls] != s_event_specs[index])
    {
        return 0;
    }
    s_posted[cls] = 0;
    return 1;
}

/*----------------------
 | EnterCriticalSection / ExitCriticalSection
 | Description: Nothing to mask, since card requests complete before they return.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: EnterCriticalSection returns 1
 ----------------------*/
int EnterCriticalSection(void)
{
    return 1;
}

void ExitCriticalSection(void)
{
}

/*----------------------
 | _card_info
 | Description: Reports a present card as ready, and an empty slot as timed out, dropping that slot's RAM files and unreadable marks.
 | Author: suinevere
 | Dependencies: slot_present, post_done, post_absent, drop_slot_ram, clear_unreadable
 | Globals: N/A
 | Params: chan -- slot channel
 | Returns: 1 (issued)
 ----------------------*/
long _card_info(long chan)
{
    int slot = chan_slot(chan);

    if (!slot_present(slot))
    {
        drop_slot_ram(slot, 0);
        clear_unreadable(slot, NULL);
        post_absent();
        return 1;
    }
    post_done();
    return 1;
}

/*----------------------
 | _card_load
 | Description: Rereads a present card's directory from the store.
 | Author: suinevere
 | Dependencies: slot_present, refresh_dir, drop_slot_ram, post_done, post_absent
 | Globals: N/A
 | Params: chan -- slot channel
 | Returns: 1 (issued)
 ----------------------*/
long _card_load(long chan)
{
    int slot = chan_slot(chan);

    if (!slot_present(slot))
    {
        post_absent();
        return 1;
    }
    drop_slot_ram(slot, 1);
    refresh_dir(slot, 1);
    post_done();
    return 1;
}

/*----------------------
 | _card_clear / _card_format
 | Description: Succeed on a present card without touching it, since a VMU is never cleared or formatted for the PS1.
 | Author: suinevere
 | Dependencies: slot_present, post_done, post_absent
 | Globals: N/A
 | Params: chan -- slot channel
 | Returns: 1
 ----------------------*/
long _card_clear(long chan)
{
    if (!slot_present(chan_slot(chan)))
    {
        post_absent();
        return 1;
    }
    post_done();
    return 1;
}

long _card_format(long chan)
{
    return _card_clear(chan);
}

/*----------------------
 | _card_read
 | Description: Reads one 128-byte sector: sector 0 is the "MC" card header, and sector head + k of a directory entry is frame k of that file.
 | Author: suinevere
 | Dependencies: slot_present, listing_file, post, post_absent
 | Globals: N/A
 | Params: chan -- slot channel, block -- sector number, buf -- 128-byte destination
 | Returns: 1 (issued)
 ----------------------*/
long _card_read(long chan, long block, unsigned char *buf)
{
    int slot = chan_slot(chan);
    card_ram_file *file;
    unsigned int offset;

    if (!slot_present(slot))
    {
        post_absent();
        return 1;
    }
    memset(buf, 0, CARD_SECTOR_BYTES);
    if (block < CARD_SECTORS_PER_BLOCK)
    {
        buf[0] = 'M';
        buf[1] = 'C';
        post(CARD_CLASS_HW, EvSpIOE);
        return 1;
    }
    file = listing_file(slot, (int)(block / CARD_SECTORS_PER_BLOCK) - 1);
    offset = (unsigned int)(block % CARD_SECTORS_PER_BLOCK) * CARD_SECTOR_BYTES;
    if (file == NULL || offset + CARD_SECTOR_BYTES > file->size)
    {
        post(CARD_CLASS_HW, EvSpERROR);
        return 1;
    }
    memcpy(buf, file->data + offset, CARD_SECTOR_BYTES);
    post(CARD_CLASS_HW, EvSpIOE);
    return 1;
}

/*----------------------
 | _card_write
 | Description: Writes one 128-byte sector of a file and stores the file, unless it carries a temporary name.
 | Author: suinevere
 | Dependencies: slot_present, listing_file, store_ram, is_temp_name, post, post_absent
 | Globals: N/A
 | Params: chan -- slot channel, block -- sector number, buf -- 128-byte source
 | Returns: 1 (issued)
 ----------------------*/
long _card_write(long chan, long block, unsigned char *buf)
{
    int slot = chan_slot(chan);
    card_ram_file *file;
    unsigned int offset;

    if (!slot_present(slot))
    {
        post_absent();
        return 1;
    }
    file = block < CARD_SECTORS_PER_BLOCK ? NULL : listing_file(slot, (int)(block / CARD_SECTORS_PER_BLOCK) - 1);
    offset = (unsigned int)(block % CARD_SECTORS_PER_BLOCK) * CARD_SECTOR_BYTES;
    if (file == NULL || offset + CARD_SECTOR_BYTES > file->size)
    {
        post(CARD_CLASS_HW, EvSpERROR);
        return 1;
    }
    memcpy(file->data + offset, buf, CARD_SECTOR_BYTES);
    file->dirty = 1;
    if (!is_temp_name(file->name) && store_ram(file) != 0)
    {
        post(CARD_CLASS_HW, EvSpERROR);
        return 1;
    }
    post(CARD_CLASS_HW, EvSpIOE);
    return 1;
}

/*----------------------
 | _new_card / _card_wait
 | Description: Nothing to acknowledge or wait for, since every request completes before it returns.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: drive -- slot
 | Returns: _card_wait returns 1
 ----------------------*/
void _new_card(void)
{
}

long _card_wait(long drive)
{
    (void)drive;
    return 1;
}

/*----------------------
 | firstfile / nextfile
 | Description: Directory search over a slot's listing with the BIOS's '?' and '*' patterns.
 | Author: suinevere
 | Dependencies: path_slot, slot_present, build_listing, name_matches, copy_name
 | Globals: s_search, s_search_count, s_search_pos, s_search_pattern
 | Params: path -- "buX0:" plus pattern, entry -- result record
 | Returns: entry, or NULL when nothing more matches
 ----------------------*/
struct DIRENTRY *firstfile(char *path, struct DIRENTRY *entry)
{
    int slot = path_slot(path);

    s_search_count = 0;
    s_search_pos = 0;
    if (slot < 0 || !slot_present(slot))
    {
        return NULL;
    }
    build_listing(slot);
    copy_name(s_search_pattern, path + 5);
    return nextfile(entry);
}

struct DIRENTRY *nextfile(struct DIRENTRY *entry)
{
    while (s_search_pos < s_search_count)
    {
        int index = s_search_pos++;

        if (name_matches(s_search_pattern, s_search[index].ps1_name))
        {
            memset(entry->name, 0, sizeof entry->name);
            strncpy(entry->name, s_search[index].ps1_name, sizeof entry->name);
            entry->attr = CARD_DIR_ATTR;
            entry->size = (long)s_search[index].size;
            entry->next = NULL;
            entry->head = (long)(index + 1) * CARD_SECTORS_PER_BLOCK;
            memset(entry->system, 0, sizeof entry->system);
            return entry;
        }
    }
    return NULL;
}

/*----------------------
 | lom_card_open
 | Description: BIOS open on a card path: a create makes an empty RAM file of the requested blocks when the card has room, otherwise the file is loaded for reading or writing.
 | Author: suinevere
 | Dependencies: path_slot, slot_present, find_ram, find_dir, build_listing, ps1_blocks_free, new_ram, load_ram
 | Globals: s_store, s_handles
 | Params: path -- "buX0:" plus file name, mode -- Psy-Q open flags
 | Returns: file handle, or -1
 ----------------------*/
long lom_card_open(char *path, unsigned long mode)
{
    int slot = path_slot(path);
    const char *name = path + 5;
    card_ram_file *file;
    int fd;

    if (slot < 0 || !slot_present(slot))
    {
        return -1;
    }
    for (fd = 0; fd < CARD_HANDLES && s_handles[fd].file != NULL; fd++)
    {
    }
    if (fd == CARD_HANDLES)
    {
        return -1;
    }
    if (mode & CARD_MODE_CREATE)
    {
        int blocks = CARD_MODE_BLOCKS(mode) ? CARD_MODE_BLOCKS(mode) : 1;

        if (find_ram(slot, name) != NULL || find_dir(slot, name) != NULL)
        {
            return -1;
        }
        if (blocks > ps1_blocks_free(s_store->free_blocks(slot), build_listing(slot)))
        {
            return -1;
        }
        file = new_ram(slot, name, (unsigned int)blocks * PS1_BLOCK_BYTES);
    }
    else
    {
        file = load_ram(slot, name);
    }
    if (file == NULL)
    {
        return -1;
    }
    file->open_count++;
    s_handles[fd].file = file;
    s_handles[fd].pos = 0;
    s_handles[fd].async = (mode & CARD_MODE_ASYNC) != 0;
    return fd;
}

/*----------------------
 | handle_at
 | Description: The open handle for an fd.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_handles
 | Params: fd -- file handle
 | Returns: handle, or NULL when fd is not open
 ----------------------*/
static card_handle *handle_at(long fd)
{
    if (fd < 0 || fd >= CARD_HANDLES || s_handles[fd].file == NULL)
    {
        return NULL;
    }
    return &s_handles[fd];
}

/*----------------------
 | lom_card_read / lom_card_write
 | Description: Copy bytes at the handle's position, stopping at the file's end; asynchronous handles post a SwCARD completion.
 | Author: suinevere
 | Dependencies: handle_at, post
 | Globals: N/A
 | Params: fd -- file handle, buf -- data, n -- byte count
 | Returns: bytes copied, or -1
 ----------------------*/
long lom_card_read(long fd, void *buf, long n)
{
    card_handle *handle = handle_at(fd);
    unsigned int count;

    if (handle == NULL || n < 0)
    {
        post(CARD_CLASS_SW, EvSpERROR);
        return -1;
    }
    count = handle->file->size - handle->pos;
    count = (unsigned int)n < count ? (unsigned int)n : count;
    memcpy(buf, handle->file->data + handle->pos, count);
    handle->pos += count;
    if (handle->async)
    {
        post(CARD_CLASS_SW, EvSpIOE);
    }
    return (long)count;
}

long lom_card_write(long fd, void *buf, long n)
{
    card_handle *handle = handle_at(fd);
    unsigned int count;

    if (handle == NULL || n < 0)
    {
        post(CARD_CLASS_SW, EvSpERROR);
        return -1;
    }
    count = handle->file->size - handle->pos;
    count = (unsigned int)n < count ? (unsigned int)n : count;
    memcpy(handle->file->data + handle->pos, buf, count);
    handle->pos += count;
    handle->file->dirty = 1;
    if (handle->async)
    {
        post(CARD_CLASS_SW, EvSpIOE);
    }
    return (long)count;
}

/*----------------------
 | lom_card_close
 | Description: Closes a handle; a changed file with a lasting name is stored, and a stored, unchanged file no handle holds leaves RAM.
 | Author: suinevere
 | Dependencies: handle_at, is_temp_name, store_ram, free_ram
 | Globals: N/A
 | Params: fd -- file handle
 | Returns: 0
 ----------------------*/
long lom_card_close(long fd)
{
    card_handle *handle = handle_at(fd);
    card_ram_file *file;

    if (handle == NULL)
    {
        return 0;
    }
    file = handle->file;
    handle->file = NULL;
    file->open_count--;
    if (file->dirty && !is_temp_name(file->name))
    {
        store_ram(file);
    }
    if (file->open_count == 0 && file->stored && !file->dirty)
    {
        free_ram(file);
    }
    return 0;
}

/*----------------------
 | lom_card_rename
 | Description: Renames a file and stores it under its new name, replacing any file already there, then removes the old name from the store.
 | Author: suinevere
 | Dependencies: path_slot, slot_present, load_ram, copy_name, store_ram, free_ram
 | Globals: s_store, s_dir_valid
 | Params: from -- current path, to -- new path on the same slot
 | Returns: 0 on success, -1 on failure
 ----------------------*/
long lom_card_rename(char *from, char *to)
{
    int slot = path_slot(from);
    char old_name[CARD_STORE_NAME_BYTES];
    card_ram_file *file;
    int was_stored;

    if (slot < 0 || slot != path_slot(to) || !slot_present(slot) || (file = load_ram(slot, from + 5)) == NULL)
    {
        return -1;
    }
    copy_name(old_name, file->name);
    was_stored = file->stored;
    copy_name(file->name, to + 5);
    if (store_ram(file) != 0)
    {
        copy_name(file->name, old_name);
        return -1;
    }
    if (was_stored)
    {
        s_store->remove(slot, old_name);
        s_dir_valid[slot] = 0;
    }
    if (file->open_count == 0)
    {
        free_ram(file);
    }
    return 0;
}

/*----------------------
 | erase
 | Description: Deletes a card file from RAM and from the store, including one marked unreadable.
 | Author: suinevere
 | Dependencies: path_slot, slot_present, find_ram, find_dir, free_ram, is_unreadable, clear_unreadable
 | Globals: s_store, s_dir_valid
 | Params: path -- card file path
 | Returns: 1 when a file was deleted, else 0
 ----------------------*/
long erase(char *path)
{
    int slot = path_slot(path);
    card_ram_file *file;
    int erased = 0;

    if (slot < 0 || !slot_present(slot))
    {
        return 0;
    }
    file = find_ram(slot, path + 5);
    if (file != NULL && file->open_count == 0)
    {
        free_ram(file);
        erased = 1;
    }
    if ((find_dir(slot, path + 5) != NULL || is_unreadable(slot, path + 5)) && s_store->remove(slot, path + 5) == 0)
    {
        erased = 1;
    }
    clear_unreadable(slot, path + 5);
    s_dir_valid[slot] = 0;
    return erased;
}

/*----------------------
 | McxCardType / McxGetTime
 | Description: Accept the command; its McxSync result says a present card is not a PocketStation, else that no card is there.
 | Author: suinevere
 | Dependencies: slot_present
 | Globals: s_mcx_pending, s_mcx_result
 | Params: port -- slot channel, time -- clock bytes (left untouched)
 | Returns: MCX_COMMAND_ISSUED
 ----------------------*/
int McxCardType(int port)
{
    s_mcx_pending = 1;
    s_mcx_result = slot_present(chan_slot(port)) ? McxErrInvalid : McxErrNoCard;
    return MCX_COMMAND_ISSUED;
}

int McxGetTime(int port, unsigned char *time)
{
    (void)time;
    return McxCardType(port);
}

/*----------------------
 | McxSync
 | Description: Finishes the pending command with its result, or reports none pending.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_mcx_pending, s_mcx_result
 | Params: mode -- wait mode (ignored), cmd -- command out, result -- result out
 | Returns: McxSyncFin when a command finished, else McxSyncNone
 ----------------------*/
int McxSync(int mode, long *cmd, long *result)
{
    (void)mode;
    if (!s_mcx_pending)
    {
        return McxSyncNone;
    }
    s_mcx_pending = 0;
    *cmd = 0;
    *result = s_mcx_result;
    return McxSyncFin;
}

/*----------------------
 | func_80033E7C / func_80034648
 | Description: libmcx internals CARDA calls on its PocketStation save path; with no PocketStation both fail.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: chan -- slot channel, block -- first block, count -- block count
 | Returns: 0
 ----------------------*/
int func_80033E7C(long chan)
{
    (void)chan;
    return 0;
}

int func_80034648(long chan, long block, long count)
{
    (void)chan;
    (void)block;
    (void)count;
    return 0;
}

/*----------------------
 | func_8001680C / func_8001681C / func_8001682C / func_8001683C / func_8001685C
 | Description: BIOS file open, read, write, close and rename as main's trampolines expose them to NIKI.
 | Author: suinevere
 | Dependencies: lom_card_open, lom_card_read, lom_card_write, lom_card_close, lom_card_rename
 | Globals: N/A
 | Params: path/from/to -- card file paths, mode -- open flags, fd -- file handle, buf -- data, n -- byte count
 | Returns: as the lom_card_* call
 ----------------------*/
long func_8001680C(char *path, long mode)
{
    return lom_card_open(path, (unsigned long)mode);
}

long func_8001681C(long fd, void *buf, long n)
{
    return lom_card_read(fd, buf, n);
}

long func_8001682C(long fd, void *buf, long n)
{
    return lom_card_write(fd, buf, n);
}

long func_8001683C(long fd)
{
    return lom_card_close(fd);
}

long func_8001685C(char *from, char *to)
{
    return lom_card_rename(from, to);
}

/*----------------------
 | func_80016F9C / func_800170BC
 | Description: BIOS strcat and strcpy, which NIKI uses to build card file paths.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: dst -- destination string, src -- source string
 | Returns: dst
 ----------------------*/
char *func_80016F9C(char *dst, const char *src)
{
    char *end = dst;

    while (*end != 0)
    {
        end++;
    }
    while ((*end++ = *src++) != 0)
    {
    }
    return dst;
}

char *func_800170BC(char *dst, const char *src)
{
    char *out = dst;

    while ((*out++ = *src++) != 0)
    {
    }
    return dst;
}
