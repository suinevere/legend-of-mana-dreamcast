/*----------------------
 | texture_cache.c
 | Description: Builds twiddled PowerVR textures from PS1 pages in the VRAM mirror and reuses them until the page's VRAM generation moves; never frees or overwrites memory the current frame used.
 | Author: suinevere
 | Dependencies: texture_cache.h, palette_bank.h, vram_mirror.h, gpu_decode.h
 | Globals: s_texture_ops, s_entries, s_flushes, s_expansions, s_skips, s_flush_pending, s_scratch
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "texture_cache.h"
#include "palette_bank.h"
#include "vram_mirror.h"
#include "gpu_decode.h"

/*----------------------
 | TEXTURE_PAGE_SIZE
 | Description: Width and height in texels of every PS1 texture page.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TEXTURE_PAGE_SIZE 256

/*----------------------
 | TEXTURE_PAGE_TEXELS
 | Description: Texels in one page.
 | Author: suinevere
 | Dependencies: TEXTURE_PAGE_SIZE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TEXTURE_PAGE_TEXELS (TEXTURE_PAGE_SIZE * TEXTURE_PAGE_SIZE)

/*----------------------
 | TEXTURE_CACHE_ENTRIES
 | Description: Fixed number of cached textures; a full table flushes like a failed allocation.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TEXTURE_CACHE_ENTRIES 128

/*----------------------
 | ENTRY_FREE, ENTRY_PAGE, ENTRY_EXPANSION
 | Description: An entry is unused, a CLUT-independent page (PAL4, PAL8 or 15bpp), or an ARGB1555 expansion of a paletted page through one CLUT.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { ENTRY_FREE = 0, ENTRY_PAGE = 1, ENTRY_EXPANSION = 2 };

/*----------------------
 | TextureEntry
 | Description: One cached texture: its key, the VRAM generations it was built from, where it lives in texture memory and the last frame that used it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    void *texture;
    uint32_t page_gen;
    uint32_t clut_gen;
    unsigned last_frame;
    int16_t tpage_x;
    int16_t tpage_y;
    int16_t clut_x;
    int16_t clut_y;
    uint8_t depth;
    uint8_t variant;
    uint8_t kind;
    uint8_t built;
    uint8_t used;
} TextureEntry;

/*----------------------
 | s_texture_ops
 | Description: Texture memory backend, set by texture_cache_init.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const TextureOps *s_texture_ops;

/*----------------------
 | s_entries
 | Description: Every cached texture.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static TextureEntry s_entries[TEXTURE_CACHE_ENTRIES];

/*----------------------
 | s_flushes
 | Description: Wholesale texture memory flushes, counted when performed at once or when deferred to the next frame.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_flushes;

/*----------------------
 | s_expansions
 | Description: ARGB1555 expansions built because palette RAM had no free slot for the CLUT.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_expansions;

/*----------------------
 | s_skips
 | Description: Lookups that returned -1, so the caller skipped the primitive.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_skips;

/*----------------------
 | s_flush_pending
 | Description: Set when memory ran out while the current frame held textures; texture_cache_begin_frame performs the flush.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_flush_pending;

/*----------------------
 | s_scratch
 | Description: The single 128 KB buffer every texture is built in before upload.
 | Author: suinevere
 | Dependencies: TEXTURE_PAGE_TEXELS
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint16_t s_scratch[TEXTURE_PAGE_TEXELS];

/*----------------------
 | spread_bits
 | Description: Spreads the low 8 bits of v into the even bit positions of a 16-bit value.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value, only bits 0-7 used
 | Returns: v with a zero bit after each bit
 ----------------------*/
static uint32_t spread_bits(unsigned v)
{
    uint32_t s = v & 0xFFu;

    s = (s | (s << 4)) & 0x0F0Fu;
    s = (s | (s << 2)) & 0x3333u;
    s = (s | (s << 1)) & 0x5555u;
    return s;
}

/*----------------------
 | texture_twiddle_index
 | Description: PowerVR twiddled index of texel (x, y) in a 256x256 texture: y's bits in the even positions, x's in the odd.
 | Author: suinevere
 | Dependencies: spread_bits
 | Globals: N/A
 | Params: x, y -- texel coordinates, 0 to 255
 | Returns: twiddled texel index, 0 to 65535
 ----------------------*/
uint32_t texture_twiddle_index(unsigned x, unsigned y)
{
    return (spread_bits(x) << 1) | spread_bits(y);
}

/*----------------------
 | page_words_wide
 | Description: Width in VRAM words of a 256-texel page at a given depth.
 | Author: suinevere
 | Dependencies: gpu_decode.h
 | Globals: N/A
 | Params: depth -- DC_DEPTH_4, DC_DEPTH_8 or DC_DEPTH_15
 | Returns: 64, 128 or 256
 ----------------------*/
static int page_words_wide(int depth)
{
    if (depth == DC_DEPTH_4)
    {
        return 64;
    }
    return depth == DC_DEPTH_8 ? 128 : 256;
}

/*----------------------
 | page_texel
 | Description: Reads texel (u, v) of a page from the wrapped VRAM mirror: a nibble or byte index for 4bpp and 8bpp, the PS1 colour word for 15bpp.
 | Author: suinevere
 | Dependencies: vram_mirror_at
 | Globals: N/A
 | Params: tpage_x, tpage_y -- page origin in VRAM words, depth -- texture depth, u, v -- texel coordinates
 | Returns: palette index or PS1 colour
 ----------------------*/
static uint16_t page_texel(int tpage_x, int tpage_y, int depth, int u, int v)
{
    uint16_t word;

    if (depth == DC_DEPTH_4)
    {
        word = *vram_mirror_at(tpage_x + (u >> 2), tpage_y + v);
        return (uint16_t)((word >> ((u & 3) * 4)) & 0xFu);
    }
    if (depth == DC_DEPTH_8)
    {
        word = *vram_mirror_at(tpage_x + (u >> 1), tpage_y + v);
        return (uint16_t)((word >> ((u & 1) * 8)) & 0xFFu);
    }
    return *vram_mirror_at(tpage_x + u, tpage_y + v);
}

/*----------------------
 | texture_read_page
 | Description: Reads a whole 256x256 page row-major from the wrapped VRAM mirror into indices (4bpp, 8bpp) or direct colours (15bpp).
 | Author: suinevere
 | Dependencies: page_texel
 | Globals: N/A
 | Params: tpage_x, tpage_y -- page origin, depth -- texture depth, out_indices -- 65536 indices or NULL, out_direct -- 65536 PS1 colours or NULL
 | Returns: N/A
 ----------------------*/
void texture_read_page(int tpage_x, int tpage_y, int depth, uint8_t *out_indices, uint16_t *out_direct)
{
    int u;
    int v;

    for (v = 0; v < TEXTURE_PAGE_SIZE; v++)
    {
        for (u = 0; u < TEXTURE_PAGE_SIZE; u++)
        {
            uint16_t texel = page_texel(tpage_x, tpage_y, depth, u, v);

            if (depth == DC_DEPTH_15)
            {
                if (out_direct != 0)
                {
                    out_direct[v * TEXTURE_PAGE_SIZE + u] = texel;
                }
            }
            else if (out_indices != 0)
            {
                out_indices[v * TEXTURE_PAGE_SIZE + u] = (uint8_t)texel;
            }
        }
    }
}

/*----------------------
 | ps1_to_argb1555
 | Description: Converts a PS1 15-bit colour to PowerVR ARGB1555, swapping red and blue; alpha is set for any non-zero texel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ps1 -- PS1 colour word
 | Returns: ARGB1555 colour
 ----------------------*/
static uint16_t ps1_to_argb1555(uint16_t ps1)
{
    if (ps1 == 0)
    {
        return 0;
    }
    return (uint16_t)(0x8000u | ((ps1 & 0x1Fu) << 10) | (ps1 & 0x03E0u) | ((ps1 >> 10) & 0x1Fu));
}

/*----------------------
 | argb8888_to_1555
 | Description: Narrows an ARGB8888 palette colour to ARGB1555, with alpha set for any non-zero alpha.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: argb -- ARGB8888 colour
 | Returns: ARGB1555 colour
 ----------------------*/
static uint16_t argb8888_to_1555(uint32_t argb)
{
    uint16_t alpha = (argb >> 24) != 0 ? 0x8000u : 0;

    return (uint16_t)(alpha | (((argb >> 19) & 0x1Fu) << 10) | (((argb >> 11) & 0x1Fu) << 5) | ((argb >> 3) & 0x1Fu));
}

/*----------------------
 | entry_bytes
 | Description: Texture memory an entry needs: 32 KB PAL4, 64 KB PAL8, 128 KB ARGB1555.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: kind -- ENTRY_PAGE or ENTRY_EXPANSION, depth -- texture depth
 | Returns: bytes
 ----------------------*/
static unsigned entry_bytes(int kind, int depth)
{
    if (kind == ENTRY_EXPANSION || depth == DC_DEPTH_15)
    {
        return TEXTURE_PAGE_TEXELS * 2u;
    }
    return depth == DC_DEPTH_8 ? TEXTURE_PAGE_TEXELS : TEXTURE_PAGE_TEXELS / 2u;
}

/*----------------------
 | build_texture
 | Description: Builds a twiddled texture for an entry in s_scratch: packed indices for a paletted page, ARGB1555 for 15bpp or an expansion through its CLUT.
 | Author: suinevere
 | Dependencies: page_texel, texture_twiddle_index, ps1_to_argb1555, argb8888_to_1555, palette_convert, vram_mirror_at, entry_bytes
 | Globals: s_scratch
 | Params: e -- entry to build
 | Returns: bytes built
 ----------------------*/
static unsigned build_texture(const TextureEntry *e)
{
    uint8_t *bytes = (uint8_t *)s_scratch;
    int u;
    int v;

    for (v = 0; v < TEXTURE_PAGE_SIZE; v++)
    {
        for (u = 0; u < TEXTURE_PAGE_SIZE; u++)
        {
            uint32_t t = texture_twiddle_index((unsigned)u, (unsigned)v);
            uint16_t texel = page_texel(e->tpage_x, e->tpage_y, e->depth, u, v);

            if (e->kind == ENTRY_EXPANSION)
            {
                s_scratch[t] = argb8888_to_1555(palette_convert(*vram_mirror_at(e->clut_x + texel, e->clut_y), e->variant));
            }
            else if (e->depth == DC_DEPTH_15)
            {
                s_scratch[t] = ps1_to_argb1555(texel);
            }
            else if (e->depth == DC_DEPTH_8)
            {
                bytes[t] = (uint8_t)texel;
            }
            else if (t & 1u)
            {
                bytes[t >> 1] = (uint8_t)((bytes[t >> 1] & 0x0Fu) | (texel << 4));
            }
            else
            {
                bytes[t >> 1] = (uint8_t)((bytes[t >> 1] & 0xF0u) | texel);
            }
        }
    }
    return entry_bytes(e->kind, e->depth);
}

/*----------------------
 | flush_all
 | Description: Frees all texture memory and forgets every entry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_texture_ops, s_entries
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void flush_all(void)
{
    int i;

    if (s_texture_ops->free_all != 0)
    {
        s_texture_ops->free_all();
    }
    for (i = 0; i < TEXTURE_CACHE_ENTRIES; i++)
    {
        s_entries[i].kind = ENTRY_FREE;
    }
}

/*----------------------
 | frame_holds_textures
 | Description: Whether any entry was handed out during frame, so its memory may still be read by the PowerVR.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_entries
 | Params: frame -- current frame
 | Returns: 1 if some entry was used in frame, else 0
 ----------------------*/
static int frame_holds_textures(unsigned frame)
{
    int i;

    for (i = 0; i < TEXTURE_CACHE_ENTRIES; i++)
    {
        if (s_entries[i].kind != ENTRY_FREE && s_entries[i].used && s_entries[i].last_frame == frame)
        {
            return 1;
        }
    }
    return 0;
}

/*----------------------
 | make_room
 | Description: Handles running out of memory or entries: flushes at once if frame holds no textures, otherwise defers the flush to texture_cache_begin_frame.
 | Author: suinevere
 | Dependencies: frame_holds_textures, flush_all
 | Globals: s_texture_ops, s_entries, s_flushes, s_flush_pending
 | Params: frame -- current frame
 | Returns: 1 if everything was flushed now, 0 if the flush is pending
 ----------------------*/
static int make_room(unsigned frame)
{
    if (frame_holds_textures(frame))
    {
        if (!s_flush_pending)
        {
            s_flush_pending = 1;
            s_flushes++;
        }
        return 0;
    }
    flush_all();
    s_flushes++;
    return 1;
}

/*----------------------
 | find_or_create
 | Description: Finds the entry for a key, or allocates memory for a new unbuilt one, flushing and retrying once only if frame holds no textures.
 | Author: suinevere
 | Dependencies: entry_bytes, make_room
 | Globals: s_texture_ops, s_entries, s_flushes, s_flush_pending
 | Params: kind -- ENTRY_PAGE or ENTRY_EXPANSION, tpage_x, tpage_y -- page origin, depth -- texture depth, clut_x, clut_y, variant -- expansion key, 0 for pages, frame -- current frame
 | Returns: entry, or NULL when memory or entries ran out
 ----------------------*/
static TextureEntry *find_or_create(int kind, int tpage_x, int tpage_y, int depth, int clut_x, int clut_y, int variant, unsigned frame)
{
    TextureEntry *free_entry = 0;
    void *texture;
    int i;

    for (i = 0; i < TEXTURE_CACHE_ENTRIES; i++)
    {
        TextureEntry *e = &s_entries[i];

        if (e->kind == ENTRY_FREE)
        {
            if (free_entry == 0)
            {
                free_entry = e;
            }
            continue;
        }
        if (e->kind == kind && e->tpage_x == tpage_x && e->tpage_y == tpage_y && e->depth == depth
            && e->clut_x == clut_x && e->clut_y == clut_y && e->variant == variant)
        {
            return e;
        }
    }
    if (free_entry == 0)
    {
        if (!make_room(frame))
        {
            return 0;
        }
        free_entry = &s_entries[0];
    }
    texture = s_texture_ops->alloc(entry_bytes(kind, depth));
    if (texture == 0)
    {
        if (!make_room(frame))
        {
            return 0;
        }
        free_entry = &s_entries[0];
        texture = s_texture_ops->alloc(entry_bytes(kind, depth));
        if (texture == 0)
        {
            return 0;
        }
    }
    free_entry->texture = texture;
    free_entry->kind = (uint8_t)kind;
    free_entry->tpage_x = (int16_t)tpage_x;
    free_entry->tpage_y = (int16_t)tpage_y;
    free_entry->depth = (uint8_t)depth;
    free_entry->clut_x = (int16_t)clut_x;
    free_entry->clut_y = (int16_t)clut_y;
    free_entry->variant = (uint8_t)variant;
    free_entry->built = 0;
    free_entry->used = 0;
    return free_entry;
}

/*----------------------
 | ensure_built
 | Description: Rebuilds and uploads a new or stale entry; a stale entry frame already used moves to fresh memory, leaving the old bytes for earlier primitives.
 | Author: suinevere
 | Dependencies: build_texture, entry_bytes, make_room
 | Globals: s_texture_ops, s_scratch, s_flushes, s_flush_pending
 | Params: e -- entry, page_gen -- current page generation, clut_gen -- current CLUT generation, 0 for pages, frame -- current frame
 | Returns: 1 if rebuilt, 0 if already current, -1 if fresh memory was needed and not available
 ----------------------*/
static int ensure_built(TextureEntry *e, uint32_t page_gen, uint32_t clut_gen, unsigned frame)
{
    unsigned bytes;

    if (e->built && e->page_gen == page_gen && e->clut_gen == clut_gen)
    {
        return 0;
    }
    if (e->built && e->used && e->last_frame == frame)
    {
        void *texture = s_texture_ops->alloc(entry_bytes(e->kind, e->depth));

        if (texture == 0)
        {
            make_room(frame);
            return -1;
        }
        e->texture = texture;
    }
    bytes = build_texture(e);
    if (s_texture_ops->upload != 0)
    {
        s_texture_ops->upload(e->texture, s_scratch, bytes);
    }
    e->page_gen = page_gen;
    e->clut_gen = clut_gen;
    e->built = 1;
    return 1;
}

/*----------------------
 | texture_cache_init
 | Description: Forgets every entry and zeroes the counters; texture memory is assumed empty.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_texture_ops, s_entries, s_flushes, s_expansions, s_skips, s_flush_pending
 | Params: ops -- texture memory backend
 | Returns: N/A
 ----------------------*/
void texture_cache_init(const TextureOps *ops)
{
    int i;

    s_texture_ops = ops;
    for (i = 0; i < TEXTURE_CACHE_ENTRIES; i++)
    {
        s_entries[i].kind = ENTRY_FREE;
    }
    s_flushes = 0;
    s_expansions = 0;
    s_skips = 0;
    s_flush_pending = 0;
}

/*----------------------
 | texture_cache_begin_frame
 | Description: Performs a flush the previous frame deferred; call after the PowerVR finished that frame and before the new frame's first lookup.
 | Author: suinevere
 | Dependencies: flush_all
 | Globals: s_texture_ops, s_entries, s_flush_pending
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void texture_cache_begin_frame(void)
{
    if (s_flush_pending && s_texture_ops != 0)
    {
        flush_all();
    }
    s_flush_pending = 0;
}

/*----------------------
 | texture_cache_flush_now
 | Description: Unconditionally frees every cached texture; call only between frames, such as an overlay switch, where no entry is still in flight.
 | Author: suinevere
 | Dependencies: flush_all
 | Globals: s_texture_ops, s_entries, s_flush_pending
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void texture_cache_flush_now(void)
{
    if (s_texture_ops != 0)
    {
        flush_all();
    }
    s_flush_pending = 0;
}

/*----------------------
 | lookup_entry
 | Description: Finds or creates an entry, brings it up to date and marks it used by frame.
 | Author: suinevere
 | Dependencies: find_or_create, ensure_built
 | Globals: s_texture_ops, s_entries, s_expansions, s_flushes, s_flush_pending, s_scratch
 | Params: kind, tpage_x, tpage_y, depth, clut_x, clut_y, variant -- entry key, page_gen, clut_gen -- current generations, frame -- current frame
 | Returns: entry, or NULL when memory ran out
 ----------------------*/
static TextureEntry *lookup_entry(int kind, int tpage_x, int tpage_y, int depth, int clut_x, int clut_y, int variant,
                                  uint32_t page_gen, uint32_t clut_gen, unsigned frame)
{
    TextureEntry *e = find_or_create(kind, tpage_x, tpage_y, depth, clut_x, clut_y, variant, frame);
    int built;

    if (e == 0)
    {
        return 0;
    }
    built = ensure_built(e, page_gen, clut_gen, frame);
    if (built < 0)
    {
        return 0;
    }
    if (built > 0 && kind == ENTRY_EXPANSION)
    {
        s_expansions++;
    }
    e->used = 1;
    e->last_frame = frame;
    return e;
}

/*----------------------
 | texture_cache_lookup
 | Description: Returns the texture for a page drawn through a CLUT and variant.
 | A shared paletted page with a palette slot, a 15bpp page, or an ARGB1555 expansion when no slot is free.
 | Author: suinevere
 | Dependencies: lookup_entry, page_words_wide, palette_bank_acquire, vram_mirror_generation
 | Globals: s_texture_ops, s_entries, s_flushes, s_expansions, s_skips, s_flush_pending, s_scratch
 | Params: tpage_x, tpage_y -- page origin in VRAM words, clut_x, clut_y -- CLUT position, depth -- DC_DEPTH_*, variant -- PAL_* alpha variant, frame -- current frame, out -- result
 | Returns: 0 on success, -1 (counted as a skip) on a bad depth or when memory ran out this frame
 ----------------------*/
int texture_cache_lookup(int tpage_x, int tpage_y, int clut_x, int clut_y, int depth, int variant, unsigned frame, DcSurfaceTexture *out)
{
    TextureEntry *e;
    uint32_t page_gen;
    int slot;

    if (s_texture_ops == 0 || out == 0 || (depth != DC_DEPTH_4 && depth != DC_DEPTH_8 && depth != DC_DEPTH_15))
    {
        s_skips++;
        return -1;
    }
    page_gen = vram_mirror_generation(tpage_x, tpage_y, page_words_wide(depth), TEXTURE_PAGE_SIZE);
    slot = depth == DC_DEPTH_15 ? -1 : palette_bank_acquire(clut_x, clut_y, depth, variant, frame);
    if (depth == DC_DEPTH_15 || slot >= 0)
    {
        e = lookup_entry(ENTRY_PAGE, tpage_x, tpage_y, depth, 0, 0, 0, page_gen, 0, frame);
        out->format = depth == DC_DEPTH_4 ? TEX_FMT_PAL4 : depth == DC_DEPTH_8 ? TEX_FMT_PAL8 : TEX_FMT_ARGB1555;
    }
    else
    {
        uint32_t clut_gen = vram_mirror_generation(clut_x, clut_y, depth == DC_DEPTH_4 ? 16 : 256, 1);

        e = lookup_entry(ENTRY_EXPANSION, tpage_x, tpage_y, depth, clut_x, clut_y, variant, page_gen, clut_gen, frame);
        out->format = TEX_FMT_ARGB1555;
    }
    if (e == 0)
    {
        s_skips++;
        return -1;
    }
    out->texture = e->texture;
    out->palette_slot = slot;
    return 0;
}

/*----------------------
 | texture_cache_flushes
 | Description: Number of wholesale texture memory flushes since init.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_flushes
 | Params: N/A
 | Returns: flush count
 ----------------------*/
unsigned texture_cache_flushes(void)
{
    return s_flushes;
}

/*----------------------
 | texture_cache_expansions
 | Description: Number of ARGB1555 expansions built or rebuilt since init because palette RAM was pinned full.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_expansions
 | Params: N/A
 | Returns: expansion build count
 ----------------------*/
unsigned texture_cache_expansions(void)
{
    return s_expansions;
}

/*----------------------
 | texture_cache_skips
 | Description: Number of lookups since init that returned -1, so the caller skipped the primitive.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_skips
 | Params: N/A
 | Returns: skip count
 ----------------------*/
unsigned texture_cache_skips(void)
{
    return s_skips;
}
