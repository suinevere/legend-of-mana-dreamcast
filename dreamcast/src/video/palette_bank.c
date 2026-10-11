/*----------------------
 | palette_bank.c
 | Description: Manages PowerVR palette RAM as 64 slots of 16 ARGB8888 entries: 4bpp CLUTs take one slot, 8bpp CLUTs an aligned run of 16, evicted least recently used.
 | Author: suinevere
 | Dependencies: palette_bank.h, vram_mirror.h, gpu_decode.h
 | Globals: s_palette_ops, s_slots, s_tick
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "palette_bank.h"
#include "vram_mirror.h"
#include "gpu_decode.h"

/*----------------------
 | PALETTE_SLOTS
 | Description: Slots in PowerVR palette RAM: 1024 entries split into groups of 16.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PALETTE_SLOTS 64

/*----------------------
 | PALETTE_SLOT_ENTRIES
 | Description: ARGB8888 entries in one slot, the size of a 4bpp CLUT.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PALETTE_SLOT_ENTRIES 16

/*----------------------
 | PALETTE_RUN_SLOTS
 | Description: Slots an 8bpp CLUT occupies, always starting on a multiple of this.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PALETTE_RUN_SLOTS 16

/*----------------------
 | SLOT_FREE, SLOT_HEAD, SLOT_TAIL
 | Description: A slot is free, holds a CLUT's bookkeeping, or is one of the 15 trailing slots of an 8bpp run.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { SLOT_FREE = 0, SLOT_HEAD = 1, SLOT_TAIL = 2 };

/*----------------------
 | PaletteSlot
 | Description: Bookkeeping for one palette slot: the CLUT key, the VRAM generation it was filled from, its last use, and whether a newer copy replaced it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    uint32_t generation;
    uint32_t tick;
    unsigned frame;
    int16_t clut_x;
    int16_t clut_y;
    uint8_t depth;
    uint8_t variant;
    uint8_t state;
    uint8_t head;
    uint8_t retired;
} PaletteSlot;

/*----------------------
 | s_palette_ops
 | Description: Backend that writes palette RAM entries, set by palette_bank_init.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const PaletteOps *s_palette_ops;

/*----------------------
 | s_slots
 | Description: Bookkeeping for every palette slot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static PaletteSlot s_slots[PALETTE_SLOTS];

/*----------------------
 | s_tick
 | Description: Monotonic acquire counter used to order slots by recency; free slots keep 0, the oldest.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint32_t s_tick;

/*----------------------
 | clut_entries
 | Description: Number of CLUT entries for a texture depth.
 | Author: suinevere
 | Dependencies: gpu_decode.h
 | Globals: N/A
 | Params: depth -- DC_DEPTH_4 or DC_DEPTH_8
 | Returns: 16, 256, or 0 for a depth without a CLUT
 ----------------------*/
static int clut_entries(int depth)
{
    if (depth == DC_DEPTH_4)
    {
        return 16;
    }
    if (depth == DC_DEPTH_8)
    {
        return 256;
    }
    return 0;
}

/*----------------------
 | palette_bank_init
 | Description: Forgets every slot and remembers the backend that writes palette RAM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_palette_ops, s_slots, s_tick
 | Params: ops -- palette RAM backend
 | Returns: N/A
 ----------------------*/
void palette_bank_init(const PaletteOps *ops)
{
    int i;

    s_palette_ops = ops;
    s_tick = 0;
    for (i = 0; i < PALETTE_SLOTS; i++)
    {
        s_slots[i].state = SLOT_FREE;
        s_slots[i].tick = 0;
        s_slots[i].head = (uint8_t)i;
        s_slots[i].retired = 0;
    }
}

/*----------------------
 | palette_convert
 | Description: Converts a PS1 15-bit colour to ARGB8888, expanding 5-bit channels and deriving alpha from bit 15 per variant; 0x0000 is always transparent and PAL_COVER turns set texels opaque white.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ps1 -- PS1 colour word, variant -- PAL_OPAQUE, PAL_HALF, PAL_STP_ONLY, PAL_NONSTP_ONLY or PAL_COVER
 | Returns: ARGB8888 colour
 ----------------------*/
uint32_t palette_convert(uint16_t ps1, int variant)
{
    int stp = (ps1 & 0x8000u) != 0;
    uint32_t r = ps1 & 0x1Fu;
    uint32_t g = (ps1 >> 5) & 0x1Fu;
    uint32_t b = (ps1 >> 10) & 0x1Fu;
    uint32_t alpha = 0xFFu;

    if (ps1 == 0)
    {
        return 0;
    }
    if (variant == PAL_COVER)
    {
        return stp ? 0xFFFFFFFFu : 0;
    }
    if (variant == PAL_HALF && stp)
    {
        alpha = 0x80u;
    }
    else if ((variant == PAL_STP_ONLY && !stp) || (variant == PAL_NONSTP_ONLY && stp))
    {
        return 0;
    }
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return (alpha << 24) | (r << 16) | (g << 8) | b;
}

/*----------------------
 | palette_clut_stp
 | Description: Classifies a CLUT by the semi-transparency bit of its non-zero entries.
 | Author: suinevere
 | Dependencies: clut_entries, vram_mirror_at
 | Globals: N/A
 | Params: clut_x, clut_y -- CLUT position in VRAM words, depth -- DC_DEPTH_4 or DC_DEPTH_8
 | Returns: STP_NONE, STP_ALL or STP_MIXED
 ----------------------*/
int palette_clut_stp(int clut_x, int clut_y, int depth)
{
    int count = clut_entries(depth);
    int set = 0;
    int clear = 0;
    int i;

    for (i = 0; i < count; i++)
    {
        uint16_t word = *vram_mirror_at(clut_x + i, clut_y);

        if (word == 0)
        {
            continue;
        }
        if (word & 0x8000u)
        {
            set++;
        }
        else
        {
            clear++;
        }
    }
    if (set == 0)
    {
        return STP_NONE;
    }
    return clear == 0 ? STP_ALL : STP_MIXED;
}

/*----------------------
 | find_slot
 | Description: Finds the live head slot holding a CLUT with this key; retired copies kept for earlier primitives are skipped.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_slots
 | Params: clut_x, clut_y -- CLUT position, depth -- texture depth, variant -- alpha variant
 | Returns: slot index, or -1 if not resident
 ----------------------*/
static int find_slot(int clut_x, int clut_y, int depth, int variant)
{
    int i;

    for (i = 0; i < PALETTE_SLOTS; i++)
    {
        const PaletteSlot *s = &s_slots[i];

        if (s->state == SLOT_HEAD && !s->retired && s->clut_x == clut_x && s->clut_y == clut_y && s->depth == depth && s->variant == variant)
        {
            return i;
        }
    }
    return -1;
}

/*----------------------
 | run_length
 | Description: Slots covered by the CLUT whose head is at slot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_slots
 | Params: slot -- head slot index
 | Returns: 1 for 4bpp, PALETTE_RUN_SLOTS for 8bpp
 ----------------------*/
static int run_length(int slot)
{
    return s_slots[slot].depth == DC_DEPTH_8 ? PALETTE_RUN_SLOTS : 1;
}

/*----------------------
 | pick_victim
 | Description: Picks the aligned slot range whose resident CLUTs were least recently used and none is pinned by frame.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_slots
 | Params: run -- slots needed (1 or PALETTE_RUN_SLOTS), frame -- current frame
 | Returns: first slot of the range, or -1 when every candidate is pinned
 ----------------------*/
static int pick_victim(int run, unsigned frame)
{
    int best = -1;
    uint32_t best_age = 0;
    int base;

    for (base = 0; base < PALETTE_SLOTS; base += run)
    {
        uint32_t age = 0;
        int pinned = 0;
        int i;

        for (i = base; i < base + run && !pinned; i++)
        {
            const PaletteSlot *owner;

            if (s_slots[i].state == SLOT_FREE)
            {
                continue;
            }
            owner = &s_slots[s_slots[i].head];
            if (owner->frame == frame)
            {
                pinned = 1;
            }
            else if (owner->tick > age)
            {
                age = owner->tick;
            }
        }
        if (!pinned && (best < 0 || age < best_age))
        {
            best = base;
            best_age = age;
        }
    }
    return best;
}

/*----------------------
 | release_range
 | Description: Frees every CLUT that touches slots [base, base + run), including whole 8bpp runs reached through a tail.
 | Author: suinevere
 | Dependencies: run_length
 | Globals: s_slots
 | Params: base -- first slot, run -- slot count
 | Returns: N/A
 ----------------------*/
static void release_range(int base, int run)
{
    int i;

    for (i = base; i < base + run; i++)
    {
        int head;
        int len;
        int j;

        if (s_slots[i].state == SLOT_FREE)
        {
            continue;
        }
        head = s_slots[i].head;
        len = run_length(head);
        for (j = head; j < head + len; j++)
        {
            s_slots[j].state = SLOT_FREE;
            s_slots[j].tick = 0;
            s_slots[j].head = (uint8_t)j;
        }
    }
}

/*----------------------
 | fill_slot
 | Description: Converts the CLUT from the VRAM mirror into palette RAM starting at slot * 16.
 | Author: suinevere
 | Dependencies: palette_convert, vram_mirror_at
 | Globals: s_palette_ops
 | Params: slot -- head slot, clut_x, clut_y -- CLUT position, count -- entries, variant -- alpha variant
 | Returns: N/A
 ----------------------*/
static void fill_slot(int slot, int clut_x, int clut_y, int count, int variant)
{
    int i;

    if (s_palette_ops == 0 || s_palette_ops->set_entry == 0)
    {
        return;
    }
    for (i = 0; i < count; i++)
    {
        s_palette_ops->set_entry((unsigned)(slot * PALETTE_SLOT_ENTRIES + i), palette_convert(*vram_mirror_at(clut_x + i, clut_y), variant));
    }
}

/*----------------------
 | palette_bank_acquire
 | Description: Returns the slot for this CLUT and variant, loading it into the least recently used unpinned slots if absent; a changed CLUT refills in place unless frame used it.
 | Author: suinevere
 | Dependencies: clut_entries, find_slot, pick_victim, release_range, fill_slot, vram_mirror_generation
 | Globals: s_slots, s_tick, s_palette_ops
 | Params: clut_x, clut_y -- CLUT position in VRAM words, depth -- DC_DEPTH_4 or DC_DEPTH_8, variant -- alpha variant, frame -- current frame, pins the slot
 | Returns: slot index (palette entry = slot * 16), or -1 when every candidate is pinned or depth has no CLUT
 ----------------------*/
int palette_bank_acquire(int clut_x, int clut_y, int depth, int variant, unsigned frame)
{
    int count = clut_entries(depth);
    int run = depth == DC_DEPTH_8 ? PALETTE_RUN_SLOTS : 1;
    uint32_t generation;
    int stale = -1;
    int slot;
    int i;

    if (count == 0)
    {
        return -1;
    }
    generation = vram_mirror_generation(clut_x, clut_y, count, 1);
    slot = find_slot(clut_x, clut_y, depth, variant);
    if (slot >= 0 && s_slots[slot].generation != generation)
    {
        if (s_slots[slot].frame != frame)
        {
            s_slots[slot].generation = generation;
            fill_slot(slot, clut_x, clut_y, count, variant);
        }
        else
        {
            stale = slot;
            slot = -1;
        }
    }
    if (slot < 0)
    {
        slot = pick_victim(run, frame);
        if (slot < 0)
        {
            return -1;
        }
        if (stale >= 0)
        {
            s_slots[stale].retired = 1;
        }
        release_range(slot, run);
        for (i = slot; i < slot + run; i++)
        {
            s_slots[i].state = SLOT_TAIL;
            s_slots[i].head = (uint8_t)slot;
        }
        s_slots[slot].state = SLOT_HEAD;
        s_slots[slot].retired = 0;
        s_slots[slot].clut_x = (int16_t)clut_x;
        s_slots[slot].clut_y = (int16_t)clut_y;
        s_slots[slot].depth = (uint8_t)depth;
        s_slots[slot].variant = (uint8_t)variant;
        s_slots[slot].generation = generation;
        fill_slot(slot, clut_x, clut_y, count, variant);
    }
    s_slots[slot].frame = frame;
    s_slots[slot].tick = ++s_tick;
    return slot;
}

/*----------------------
 | palette_bank_in_use
 | Description: Counts palette slots currently holding a CLUT, an 8bpp run counting all 16.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_slots
 | Params: N/A
 | Returns: occupied slot count, 0 to 64
 ----------------------*/
unsigned palette_bank_in_use(void)
{
    unsigned used = 0;
    int i;

    for (i = 0; i < PALETTE_SLOTS; i++)
    {
        if (s_slots[i].state != SLOT_FREE)
        {
            used++;
        }
    }
    return used;
}
