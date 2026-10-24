/*----------------------
 | texture_cache.c
 | Description: Builds twiddled PowerVR textures from PS1 pages in the VRAM mirror and reuses them until the page's VRAM generation moves; never frees or overwrites memory the current frame used.
 | Author: suinevere
 | Dependencies: texture_cache.h, palette_bank.h, vram_mirror.h, gpu_decode.h
 | Globals: s_texture_ops, s_entries, s_flushes, s_expansions, s_skips, s_flush_pending, s_retired, s_retired_count, s_scratch, s_lookup_memo, s_memo_epoch
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "texture_cache.h"
#include "palette_bank.h"
#include "vram_mirror.h"
#include "gpu_decode.h"
#include "dc_system.h"

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
    void *spare;
    uint32_t spare_gen;
    uint8_t spare_valid;
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
 | s_builds / s_build_us
 | Description: Texture entries converted from the VRAM mirror, first builds and rebuilds after a page or CLUT write alike, the microseconds spent converting them, and the microseconds spent uploading them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_builds;
static unsigned long long s_build_us;
static unsigned long long s_upload_us;

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
 | RETIRED_SLOTS / s_retired / s_retired_count
 | Description: Texture memory an entry replaced while the current frame still read it, freed by texture_cache_begin_frame once that frame is on screen.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RETIRED_SLOTS 64
static void *s_retired[RETIRED_SLOTS];
static int s_retired_count;

/*----------------------
 | retire_texture
 | Description: Queues replaced texture memory for texture_cache_begin_frame to free; with no free hook or a full queue it stays allocated until the next flush.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_texture_ops, s_retired, s_retired_count
 | Params: texture -- memory the current frame may still read
 | Returns: N/A
 ----------------------*/
static void retire_texture(void *texture)
{
    if (texture != 0 && s_texture_ops->free != 0 && s_retired_count < RETIRED_SLOTS)
    {
        s_retired[s_retired_count++] = texture;
    }
}

/*----------------------
 | LOOKUP_MEMO_ENTRIES
 | Description: Size of the direct-mapped table of recent lookups; a power of two.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOOKUP_MEMO_ENTRIES 64

/*----------------------
 | LookupMemo
 | Description: One remembered lookup result with its key, frame, VRAM write count and flush epoch, valid only while all of them still match.
 | Author: suinevere
 | Dependencies: DcSurfaceTexture
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    DcSurfaceTexture out;
    uint32_t writes;
    unsigned epoch;
    unsigned frame;
    int16_t tpage_x;
    int16_t tpage_y;
    int16_t clut_x;
    int16_t clut_y;
    uint8_t depth;
    uint8_t variant;
    uint8_t valid;
} LookupMemo;

/*----------------------
 | s_lookup_memo
 | Description: Recent lookups, so primitives repeating a key within a frame skip the palette and entry scans.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static LookupMemo s_lookup_memo[LOOKUP_MEMO_ENTRIES];

/*----------------------
 | s_memo_epoch
 | Description: Bumped by every flush and init, retiring all remembered lookups at once.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_memo_epoch;

/*----------------------
 | s_scratch
 | Description: The single 128 KB buffer every texture is built in before upload.
 | Author: suinevere
 | Dependencies: TEXTURE_PAGE_TEXELS
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint16_t s_scratch[TEXTURE_PAGE_TEXELS] __attribute__((aligned(32)));

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
 | s_twiddle_u / s_twiddle_v / s_twiddle_ready
 | Description: The column and row halves of texture_twiddle_index, whose OR is the twiddled index, filled on first use.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint16_t s_twiddle_u[TEXTURE_PAGE_SIZE];
static uint16_t s_twiddle_v[TEXTURE_PAGE_SIZE];
static int s_twiddle_ready;

/*----------------------
 | twiddle_tables
 | Description: Fills s_twiddle_u and s_twiddle_v once.
 | Author: suinevere
 | Dependencies: spread_bits
 | Globals: s_twiddle_u, s_twiddle_v, s_twiddle_ready
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void twiddle_tables(void)
{
    unsigned i;

    if (s_twiddle_ready)
    {
        return;
    }
    for (i = 0; i < TEXTURE_PAGE_SIZE; i++)
    {
        s_twiddle_u[i] = (uint16_t)(spread_bits(i) << 1);
        s_twiddle_v[i] = (uint16_t)spread_bits(i);
    }
    s_twiddle_ready = 1;
}

/*----------------------
 | page_row
 | Description: Row v of a page as consecutive VRAM words: straight from the mirror when the row does not wrap, else gathered into tmp.
 | Author: suinevere
 | Dependencies: vram_mirror_at
 | Globals: N/A
 | Params: tpage_x, tpage_y -- page origin, v -- row, words -- page width in words, tmp -- room for words values
 | Returns: pointer to the row's words
 ----------------------*/
static const uint16_t *page_row(int tpage_x, int tpage_y, int v, int words, uint16_t *tmp)
{
    const uint16_t *first = vram_mirror_at(tpage_x, tpage_y + v);
    int w;

    if (vram_mirror_at(tpage_x + words - 1, tpage_y + v) == first + words - 1)
    {
        return first;
    }
    for (w = 0; w < words; w++)
    {
        tmp[w] = *vram_mirror_at(tpage_x + w, tpage_y + v);
    }
    return tmp;
}

/*----------------------
 | entry_colours
 | Description: Converts an expansion entry's CLUT to ARGB1555 once, so each texel is a table lookup.
 | Author: suinevere
 | Dependencies: argb8888_to_1555, palette_convert, vram_mirror_at
 | Globals: N/A
 | Params: e -- entry, colours -- 256 outputs, 16 used for 4bpp
 | Returns: N/A
 ----------------------*/
static void entry_colours(const TextureEntry *e, uint16_t *colours)
{
    int count = e->depth == DC_DEPTH_4 ? 16 : 256;
    int i;

    if (e->kind != ENTRY_EXPANSION)
    {
        return;
    }
    for (i = 0; i < count; i++)
    {
        colours[i] = argb8888_to_1555(palette_convert(*vram_mirror_at(e->clut_x + i, e->clut_y), e->variant));
    }
}

/*----------------------
 | build_region_pairs
 | Description: build_region for even bounds, two rows at a time: a 2x2 texel block is four consecutive twiddled slots, so it lands in one or two aligned stores instead of four scattered ones.
 | Author: suinevere
 | Dependencies: page_row, page_words_wide, ps1_to_argb1555
 | Globals: s_scratch, s_twiddle_u, s_twiddle_v
 | Params: e -- entry, u0, u1, v0, v1 -- even texel bounds, colours -- entry_colours output for an expansion
 | Returns: N/A
 ----------------------*/
static void build_region_pairs(const TextureEntry *e, int u0, int u1, int v0, int v1, const uint16_t *colours)
{
    uint8_t *bytes = (uint8_t *)s_scratch;
    uint16_t tmp0[TEXTURE_PAGE_SIZE];
    uint16_t tmp1[TEXTURE_PAGE_SIZE];
    int words = page_words_wide(e->depth);
    int expand = e->kind == ENTRY_EXPANSION;
    int u;
    int v;

    for (v = v0; v < v1; v += 2)
    {
        const uint16_t *r0 = page_row(e->tpage_x, e->tpage_y, v, words, tmp0);
        const uint16_t *r1 = page_row(e->tpage_x, e->tpage_y, v + 1, words, tmp1);
        unsigned tv = s_twiddle_v[v];

        for (u = u0; u < u1; u += 2)
        {
            unsigned t = s_twiddle_u[u] | tv;
            uint32_t *wide = (uint32_t *)&s_scratch[t];
            unsigned a;
            unsigned b;
            unsigned c;
            unsigned d;

            if (e->depth == DC_DEPTH_15)
            {
                wide[0] = (uint32_t)ps1_to_argb1555(r0[u]) | ((uint32_t)ps1_to_argb1555(r1[u]) << 16);
                wide[1] = (uint32_t)ps1_to_argb1555(r0[u + 1]) | ((uint32_t)ps1_to_argb1555(r1[u + 1]) << 16);
                continue;
            }
            if (e->depth == DC_DEPTH_8)
            {
                a = r0[u >> 1] & 0xFFu;
                c = r0[u >> 1] >> 8;
                b = r1[u >> 1] & 0xFFu;
                d = r1[u >> 1] >> 8;
            }
            else
            {
                unsigned shift = (unsigned)(u & 3) * 4u;

                a = (r0[u >> 2] >> shift) & 0xFu;
                c = (r0[u >> 2] >> (shift + 4u)) & 0xFu;
                b = (r1[u >> 2] >> shift) & 0xFu;
                d = (r1[u >> 2] >> (shift + 4u)) & 0xFu;
            }
            if (expand)
            {
                wide[0] = (uint32_t)colours[a] | ((uint32_t)colours[b] << 16);
                wide[1] = (uint32_t)colours[c] | ((uint32_t)colours[d] << 16);
            }
            else if (e->depth == DC_DEPTH_8)
            {
                *(uint32_t *)&bytes[t] = a | (b << 8) | (c << 16) | (d << 24);
            }
            else
            {
                *(uint16_t *)&bytes[t >> 1] = (uint16_t)(a | (b << 4) | (c << 8) | (d << 12));
            }
        }
    }
}

/*----------------------
 | build_region
 | Description: Writes texels u0..u1-1 of rows v0..v1-1 into s_scratch at their twiddled places: packed indices for a paletted page, ARGB1555 for 15bpp or an expansion.
 | Author: suinevere
 | Dependencies: twiddle_tables, page_row, page_words_wide, ps1_to_argb1555, build_region_pairs
 | Globals: s_scratch, s_twiddle_u, s_twiddle_v
 | Params: e -- entry, u0, u1, v0, v1 -- texel bounds, colours -- entry_colours output for an expansion
 | Returns: N/A
 ----------------------*/
static void build_region(const TextureEntry *e, int u0, int u1, int v0, int v1, const uint16_t *colours)
{
    uint8_t *bytes = (uint8_t *)s_scratch;
    uint16_t tmp[TEXTURE_PAGE_SIZE];
    int words = page_words_wide(e->depth);
    int expand = e->kind == ENTRY_EXPANSION;
    int u;
    int v;

    twiddle_tables();
    if (((u0 | u1 | v0 | v1) & 1) == 0)
    {
        build_region_pairs(e, u0, u1, v0, v1, colours);
        return;
    }
    for (v = v0; v < v1; v++)
    {
        const uint16_t *row = page_row(e->tpage_x, e->tpage_y, v, words, tmp);
        unsigned tv = s_twiddle_v[v];

        if (e->depth == DC_DEPTH_15)
        {
            for (u = u0; u < u1; u++)
            {
                s_scratch[s_twiddle_u[u] | tv] = ps1_to_argb1555(row[u]);
            }
        }
        else if (e->depth == DC_DEPTH_8)
        {
            for (u = u0; u < u1; u++)
            {
                unsigned index = (row[u >> 1] >> ((u & 1) * 8)) & 0xFFu;
                unsigned t = s_twiddle_u[u] | tv;

                if (expand)
                {
                    s_scratch[t] = colours[index];
                }
                else
                {
                    bytes[t] = (uint8_t)index;
                }
            }
        }
        else
        {
            for (u = u0; u < u1; u++)
            {
                unsigned index = (row[u >> 2] >> ((u & 3) * 4)) & 0xFu;
                unsigned t = s_twiddle_u[u] | tv;

                if (expand)
                {
                    s_scratch[t] = colours[index];
                }
                else if (t & 1u)
                {
                    bytes[t >> 1] = (uint8_t)((bytes[t >> 1] & 0x0Fu) | (index << 4));
                }
                else
                {
                    bytes[t >> 1] = (uint8_t)((bytes[t >> 1] & 0xF0u) | index);
                }
            }
        }
    }
}

/*----------------------
 | build_texture
 | Description: Builds a whole entry's twiddled texture in s_scratch.
 | Author: suinevere
 | Dependencies: entry_colours, build_region, entry_bytes
 | Globals: s_scratch
 | Params: e -- entry to build
 | Returns: bytes built
 ----------------------*/
static unsigned build_texture(const TextureEntry *e)
{
    uint16_t colours[256];

    entry_colours(e, colours);
    build_region(e, 0, TEXTURE_PAGE_SIZE, 0, TEXTURE_PAGE_SIZE, colours);
    return entry_bytes(e->kind, e->depth);
}

/*----------------------
 | DIRTY_BLOCK_WORDS / DIRTY_BLOCK_ROWS / SQUARE_TEXELS
 | Description: The VRAM mirror's generation block (VRAM_BLOCK_WIDTH by VRAM_BLOCK_HEIGHT words), and the side of the texel square whose twiddled texels are one contiguous run.
 | Author: suinevere
 | Dependencies: vram_mirror.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DIRTY_BLOCK_WORDS 64
#define DIRTY_BLOCK_ROWS 16
#define SQUARE_TEXELS 16

/*----------------------
 | copy_texture
 | Description: Copies built bytes into texture memory, through the backend's waiting upload when a submitted scene may still read the target, else through its write.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_texture_ops
 | Params: dst -- texture memory, src -- bytes, bytes -- count, settle -- nonzero when a submitted scene may read dst
 | Returns: N/A
 ----------------------*/
static void copy_texture(void *dst, const void *src, unsigned bytes, int settle)
{
    if (!settle && s_texture_ops->write != 0)
    {
        s_texture_ops->write(dst, src, bytes);
    }
    else if (s_texture_ops->upload != 0)
    {
        s_texture_ops->upload(dst, src, bytes);
    }
}

/*----------------------
 | rebuild_dirty
 | Description: Rebuilds only the texel squares under mirror blocks written after generation since, and copies each square's run into target.
 | Author: suinevere
 | Dependencies: entry_colours, build_region, page_words_wide, vram_mirror_generation, twiddle_tables, copy_texture, dc_time_us
 | Globals: s_scratch, s_twiddle_u, s_twiddle_v, s_build_us, s_upload_us
 | Params: e -- entry whose CLUT is unchanged, target -- texture memory holding the page as of since, since -- its generation, settle -- as copy_texture
 | Returns: N/A
 ----------------------*/
static void rebuild_dirty(const TextureEntry *e, void *target, uint32_t since, int settle)
{
    uint16_t colours[256];
    int words = page_words_wide(e->depth);
    int texels_per_word = TEXTURE_PAGE_SIZE / words;
    int wide = e->kind == ENTRY_EXPANSION || e->depth == DC_DEPTH_15;
    unsigned square_bytes = SQUARE_TEXELS * SQUARE_TEXELS;
    int bx;
    int by;

    square_bytes = wide ? square_bytes * 2u : (e->depth == DC_DEPTH_8 ? square_bytes : square_bytes / 2u);
    twiddle_tables();
    entry_colours(e, colours);
    for (by = 0; by < TEXTURE_PAGE_SIZE / DIRTY_BLOCK_ROWS; by++)
    {
        for (bx = 0; bx < (words + DIRTY_BLOCK_WORDS - 1) / DIRTY_BLOCK_WORDS; bx++)
        {
            int u0 = bx * DIRTY_BLOCK_WORDS * texels_per_word;
            int u1 = u0 + DIRTY_BLOCK_WORDS * texels_per_word;
            int v0 = by * DIRTY_BLOCK_ROWS;
            unsigned long long t0;
            int u;

            if (vram_mirror_generation(e->tpage_x + bx * DIRTY_BLOCK_WORDS, e->tpage_y + v0, DIRTY_BLOCK_WORDS, DIRTY_BLOCK_ROWS) <= since)
            {
                continue;
            }
            u1 = u1 > TEXTURE_PAGE_SIZE ? TEXTURE_PAGE_SIZE : u1;
            t0 = dc_time_us();
            build_region(e, u0, u1, v0, v0 + DIRTY_BLOCK_ROWS, colours);
            s_build_us += dc_time_us() - t0;
            t0 = dc_time_us();
            for (u = u0; u < u1; u += SQUARE_TEXELS)
            {
                unsigned base = s_twiddle_u[u] | s_twiddle_v[v0];
                unsigned offset = wide ? base * 2u : (e->depth == DC_DEPTH_8 ? base : base / 2u);

                copy_texture((uint8_t *)target + offset, (uint8_t *)s_scratch + offset, square_bytes, settle);
            }
            s_upload_us += dc_time_us() - t0;
        }
    }
}

/*----------------------
 | flush_all
 | Description: Frees all texture memory and forgets every entry and remembered lookup.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_texture_ops, s_entries, s_memo_epoch
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
    s_retired_count = 0;
    s_memo_epoch++;
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
    free_entry->spare = 0;
    free_entry->spare_valid = 0;
    return free_entry;
}

/*----------------------
 | update_through_spare
 | Description: Brings a built entry up to page_gen without waiting on the screen: the changed squares go into its spare buffer, last read two scenes back, which then swaps in; a spare that is new or outdated by a whole rebuild is built whole, and with no memory for one the squares are rewritten in place after the wait.
 | Author: suinevere
 | Dependencies: rebuild_dirty, build_texture, copy_texture, entry_bytes, dc_time_us
 | Globals: s_texture_ops, s_scratch, s_builds, s_build_us, s_upload_us
 | Params: e -- built entry, not yet used this frame, with an unchanged CLUT; page_gen -- current page generation
 | Returns: 1
 ----------------------*/
static int update_through_spare(TextureEntry *e, uint32_t page_gen)
{
    void *previous = e->texture;
    unsigned long long t0;
    unsigned bytes;

    if (e->spare == 0)
    {
        e->spare = s_texture_ops->alloc(entry_bytes(e->kind, e->depth));
        if (e->spare == 0)
        {
            rebuild_dirty(e, e->texture, e->page_gen, 1);
            e->page_gen = page_gen;
            return 1;
        }
    }
    if (!e->spare_valid)
    {
        t0 = dc_time_us();
        bytes = build_texture(e);
        s_builds++;
        s_build_us += dc_time_us() - t0;
        t0 = dc_time_us();
        copy_texture(e->spare, s_scratch, bytes, 0);
        s_upload_us += dc_time_us() - t0;
    }
    else
    {
        rebuild_dirty(e, e->spare, e->spare_gen, 0);
    }
    e->texture = e->spare;
    e->spare = previous;
    e->spare_gen = e->page_gen;
    e->spare_valid = 1;
    e->page_gen = page_gen;
    return 1;
}

/*----------------------
 | ensure_built
 | Description: Builds a new entry into its fresh memory; a stale entry not used yet this frame with an unchanged CLUT updates through its spare buffer, and any other stale entry is rebuilt whole, into fresh memory when used this frame so earlier primitives keep the old bytes.
 | Author: suinevere
 | Dependencies: build_texture, update_through_spare, copy_texture, entry_bytes, make_room, dc_time_us
 | Globals: s_texture_ops, s_scratch, s_flushes, s_flush_pending, s_builds, s_build_us, s_upload_us
 | Params: e -- entry, page_gen -- current page generation, clut_gen -- current CLUT generation, 0 for pages, frame -- current frame
 | Returns: 1 if rebuilt, 0 if already current, -1 if fresh memory was needed and not available
 ----------------------*/
static int ensure_built(TextureEntry *e, uint32_t page_gen, uint32_t clut_gen, unsigned frame)
{
    unsigned bytes;
    unsigned long long t0;
    int in_place = e->built;

    if (e->built && e->page_gen == page_gen && e->clut_gen == clut_gen)
    {
        return 0;
    }
    if (e->built && e->clut_gen == clut_gen && !(e->used && e->last_frame == frame))
    {
        return update_through_spare(e, page_gen);
    }
    if (e->built && e->used && e->last_frame == frame)
    {
        void *texture = s_texture_ops->alloc(entry_bytes(e->kind, e->depth));

        if (texture == 0)
        {
            make_room(frame);
            return -1;
        }
        retire_texture(e->texture);
        e->texture = texture;
        in_place = 0;
    }
    t0 = dc_time_us();
    bytes = build_texture(e);
    s_builds++;
    s_build_us += dc_time_us() - t0;
    t0 = dc_time_us();
    copy_texture(e->texture, s_scratch, bytes, in_place);
    s_upload_us += dc_time_us() - t0;
    e->page_gen = page_gen;
    e->clut_gen = clut_gen;
    e->built = 1;
    e->spare_valid = 0;
    return 1;
}

/*----------------------
 | texture_cache_init
 | Description: Forgets every entry and zeroes the counters; texture memory is assumed empty.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_texture_ops, s_entries, s_flushes, s_expansions, s_skips, s_flush_pending, s_memo_epoch
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
    s_retired_count = 0;
    s_memo_epoch++;
}

/*----------------------
 | texture_cache_begin_frame
 | Description: Performs a flush the previous frame deferred and frees the memory earlier frames retired; call after the PowerVR finished that frame and before the new frame's first lookup.
 | Author: suinevere
 | Dependencies: flush_all, TextureOps.free
 | Globals: s_texture_ops, s_entries, s_flush_pending, s_retired, s_retired_count
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void texture_cache_begin_frame(void)
{
    if (s_flush_pending && s_texture_ops != 0)
    {
        flush_all();
    }
    while (s_retired_count > 0)
    {
        s_texture_ops->free(s_retired[--s_retired_count]);
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
 | A shared paletted page with a palette slot, a 15bpp page, or an ARGB1555 expansion when no slot is free; repeats within a frame with VRAM unchanged come from the memo.
 | Author: suinevere
 | Dependencies: lookup_entry, page_words_wide, palette_bank_acquire, vram_mirror_generation, vram_mirror_write_count
 | Globals: s_texture_ops, s_entries, s_flushes, s_expansions, s_skips, s_flush_pending, s_scratch, s_lookup_memo, s_memo_epoch
 | Params: tpage_x, tpage_y -- page origin in VRAM words, clut_x, clut_y -- CLUT position, depth -- DC_DEPTH_*, variant -- PAL_* alpha variant, frame -- current frame, out -- result
 | Returns: 0 on success, -1 (counted as a skip) on a bad depth or when memory ran out this frame
 ----------------------*/
int texture_cache_lookup(int tpage_x, int tpage_y, int clut_x, int clut_y, int depth, int variant, unsigned frame, DcSurfaceTexture *out)
{
    TextureEntry *e;
    LookupMemo *m;
    uint32_t page_gen;
    uint32_t writes;
    int slot;

    if (s_texture_ops == 0 || out == 0 || (depth != DC_DEPTH_4 && depth != DC_DEPTH_8 && depth != DC_DEPTH_15))
    {
        s_skips++;
        return -1;
    }
    writes = vram_mirror_write_count();
    m = &s_lookup_memo[((unsigned)tpage_x / 64u + (unsigned)tpage_y / 256u * 16u + (unsigned)clut_x / 16u * 5u
                        + (unsigned)clut_y * 3u + (unsigned)variant * 7u + (unsigned)depth) & (LOOKUP_MEMO_ENTRIES - 1)];
    if (m->valid && m->frame == frame && m->writes == writes && m->epoch == s_memo_epoch && m->tpage_x == tpage_x
        && m->tpage_y == tpage_y && m->clut_x == clut_x && m->clut_y == clut_y && m->depth == depth && m->variant == variant)
    {
        *out = m->out;
        return 0;
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
    m->out = *out;
    m->writes = writes;
    m->epoch = s_memo_epoch;
    m->frame = frame;
    m->tpage_x = (int16_t)tpage_x;
    m->tpage_y = (int16_t)tpage_y;
    m->clut_x = (int16_t)clut_x;
    m->clut_y = (int16_t)clut_y;
    m->depth = (uint8_t)depth;
    m->variant = (uint8_t)variant;
    m->valid = 1;
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

/*----------------------
 | texture_cache_builds
 | Description: Texture entries converted since start-up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_builds
 | Params: N/A
 | Returns: the count
 ----------------------*/
unsigned texture_cache_builds(void)
{
    return s_builds;
}

/*----------------------
 | texture_cache_build_us
 | Description: Microseconds spent converting texture entries since start-up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_build_us
 | Params: N/A
 | Returns: the total
 ----------------------*/
unsigned long long texture_cache_build_us(void)
{
    return s_build_us;
}

/*----------------------
 | texture_cache_upload_us
 | Description: Microseconds spent uploading texture entries since start-up, waits for the previous scene included.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_upload_us
 | Params: N/A
 | Returns: the total
 ----------------------*/
unsigned long long texture_cache_upload_us(void)
{
    return s_upload_us;
}

/*----------------------
 | texture_cache_flush_pending
 | Description: Reports whether a flush is waiting for texture_cache_begin_frame, so the backend knows the next scene must wait for the previous one to finish.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_flush_pending
 | Params: N/A
 | Returns: 1 if a deferred flush is pending, else 0
 ----------------------*/
int texture_cache_flush_pending(void)
{
    return s_flush_pending;
}
