/*----------------------
 | gpu_decode.c
 | Description: Decodes PS1 GP0 ordering-table packets into shape-neutral DcPrim primitives, tracking draw area, offset, tpage and texture window across E1-E5 commands.
 | Author: suinevere
 | Dependencies: gpu_decode.h, gpu_census.h, gpu_tag.h, dc_system.c (dc_halt)
 | Globals: s_host_base (LOM_HOST_TEST only)
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "gpu_decode.h"
#include "gpu_census.h"
#include "gpu_tag.h"

/*----------------------
 | dc_halt
 | Description: Declared locally without dc_system.h's noreturn attribute, so the code gpu_decode_ot runs after calling it stays live when a host test mocks it to return.
 | Author: suinevere
 | Dependencies: dc_system.c (dc_halt)
 | Globals: N/A
 | Params: message -- halt reason
 | Returns: N/A
 ----------------------*/
extern void dc_halt(const char *message);

/*----------------------
 | GPU_DECODE_MAX_PACKETS
 | Description: Most OT packets gpu_decode_ot visits before treating the chain as a non-terminating loop, matching ot_walk's guard.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GPU_DECODE_MAX_PACKETS 65536

#ifdef LOM_HOST_TEST

/*----------------------
 | s_host_base
 | Description: Host address the 24-bit tag field 0 maps to, set by gpu_decode_set_base for host tests.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uintptr_t s_host_base;

/*----------------------
 | gpu_decode_set_base
 | Description: Sets s_host_base, the host address the 24-bit tag field 0 maps to, for host tests.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_host_base
 | Params: base -- host address field 0 maps to
 | Returns: N/A
 ----------------------*/
void gpu_decode_set_base(uintptr_t base)
{
    s_host_base = base;
}

/*----------------------
 | gpu_decode_pointer
 | Description: Resolves a tag's 24-bit address field to a host pointer relative to s_host_base, standing in for gpu_tag_pointer under a host test.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_host_base
 | Params: addr -- 24-bit tag address field, already checked against GPU_TAG_END
 | Returns: pointer to the next packet
 ----------------------*/
static const u_long *gpu_decode_pointer(uint32_t addr)
{
    return (const u_long *)(s_host_base + addr);
}

#else

/*----------------------
 | gpu_decode_pointer
 | Description: Resolves a tag's 24-bit address field to a Dreamcast RAM pointer.
 | Author: suinevere
 | Dependencies: gpu_tag_pointer
 | Globals: N/A
 | Params: addr -- 24-bit tag address field, already checked against GPU_TAG_END
 | Returns: pointer to the next packet
 ----------------------*/
static const u_long *gpu_decode_pointer(uint32_t addr)
{
    return (const u_long *)gpu_tag_pointer(addr);
}

#endif

/*----------------------
 | sign_extend11
 | Description: Interprets a 16-bit field's low 11 bits as a two's complement value and sign-extends it to 16 bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: raw -- 16-bit field whose bit 10 is the sign bit
 | Returns: sign-extended value
 ----------------------*/
static int16_t sign_extend11(uint16_t raw)
{
    return (int16_t)(((int16_t)(uint16_t)(raw << 5)) >> 5);
}

/*----------------------
 | decode_xy
 | Description: Splits a packed x/y word into sign-extended, offset-adjusted VRAM coordinates.
 | Author: suinevere
 | Dependencies: sign_extend11
 | Globals: N/A
 | Params: state -- current draw offset, word -- packed x (low half) and y (high half), x, y -- outputs
 | Returns: N/A
 ----------------------*/
static void decode_xy(const DcGpuState *state, uint32_t word, int16_t *x, int16_t *y)
{
    *x = (int16_t)(sign_extend11((uint16_t)word) + state->ofs_x);
    *y = (int16_t)(sign_extend11((uint16_t)(word >> 16)) + state->ofs_y);
}

/*----------------------
 | fill_color
 | Description: Copies a vertex's r/g/b tint from a P_TAG-style colour word's low three bytes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- vertex to fill, word -- colour word (r at byte 0, g at byte 1, b at byte 2)
 | Returns: N/A
 ----------------------*/
static void fill_color(DcVertex *v, uint32_t word)
{
    v->r = (uint8_t)word;
    v->g = (uint8_t)(word >> 8);
    v->b = (uint8_t)(word >> 16);
}

/*----------------------
 | fill_uv
 | Description: Copies a vertex's texel coordinates from a packed u/v word's low two bytes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- vertex to fill, word -- u/v word (u at byte 0, v at byte 1)
 | Returns: N/A
 ----------------------*/
static void fill_uv(DcVertex *v, uint32_t word)
{
    v->u = (uint8_t)word;
    v->v = (uint8_t)(word >> 8);
}

/*----------------------
 | apply_tpage
 | Description: Splits a resolved tpage value into VRAM texture-page origin, CLUT origin, colour depth and blend mode, and flags the semi-transparent subtractive trait.
 | Author: suinevere
 | Dependencies: gpu_census_flag
 | Globals: N/A
 | Params: prim -- primitive to fill, tpage -- resolved texture page, clut -- CLUT id
 | Returns: N/A
 ----------------------*/
static void apply_tpage(DcPrim *prim, uint16_t tpage, uint16_t clut)
{
    prim->tpage_x = (uint16_t)((tpage & 0x0Fu) * 64u);
    prim->tpage_y = (uint16_t)(((tpage >> 4) & 1u) * 256u);
    prim->depth = (uint8_t)((tpage >> 7) & 3u);
    prim->abr = (uint8_t)((tpage >> 5) & 3u);
    prim->clut_x = (uint16_t)((clut & 0x3Fu) * 16u);
    prim->clut_y = (uint16_t)((clut >> 6) & 0x1FFu);
    if (prim->semi && prim->abr == 2)
    {
        gpu_census_flag(CENSUS_SUBTRACT);
    }
}

/*----------------------
 | decode_poly
 | Description: Decodes a flat or Gouraud, textured or untextured triangle or quad into a DcPrim and calls sink; textured polygons update state->tpage.
 | Author: suinevere
 | Dependencies: fill_color, fill_uv, decode_xy, apply_tpage, gpu_census_flag
 | Globals: N/A
 | Params: state -- GPU state, w -- command words, nverts -- 3 or 4, textured, gouraud -- shape flags, code -- GP0 command byte, sink, ctx -- primitive callback
 | Returns: N/A
 ----------------------*/
static void decode_poly(DcGpuState *state, const uint32_t *w, unsigned nverts, int textured, int gouraud,
                         unsigned code, DcPrimSink sink, void *ctx)
{
    DcPrim prim;
    unsigned idx = 1;
    unsigned i;
    uint16_t tpage = state->tpage;
    uint16_t clut = 0;
    int bright = 0;

    prim.shape = (uint8_t)(nverts == 3 ? DC_SHAPE_TRI : DC_SHAPE_QUAD);
    prim.nverts = (uint8_t)nverts;
    prim.textured = (uint8_t)(textured != 0);
    prim.raw = (uint8_t)(code & 1u);
    prim.semi = (uint8_t)((code >> 1) & 1u);
    prim.gouraud = (uint8_t)(gouraud != 0);
    fill_color(&prim.v[0], w[0]);
    for (i = 0; i < nverts; i++)
    {
        DcVertex *v = &prim.v[i];

        if (i != 0)
        {
            if (gouraud)
            {
                fill_color(v, w[idx]);
                idx++;
            }
            else
            {
                v->r = prim.v[0].r;
                v->g = prim.v[0].g;
                v->b = prim.v[0].b;
            }
        }
        decode_xy(state, w[idx], &v->x, &v->y);
        idx++;
        if (textured)
        {
            fill_uv(v, w[idx]);
            if (i == 0)
            {
                clut = (uint16_t)(w[idx] >> 16);
            }
            else if (i == 1)
            {
                tpage = (uint16_t)(w[idx] >> 16);
            }
            idx++;
        }
        else
        {
            v->u = 0;
            v->v = 0;
        }
        if (v->r > 128 || v->g > 128 || v->b > 128)
        {
            bright = 1;
        }
    }
    if (textured)
    {
        state->tpage = tpage;
    }
    apply_tpage(&prim, tpage, clut);
    if (prim.textured && !prim.raw && bright)
    {
        gpu_census_flag(CENSUS_BRIGHTEN);
    }
    sink(&prim, state, ctx);
}

/*----------------------
 | decode_rect
 | Description: Decodes a SPRT, SPRT_8, SPRT_16, TILE, TILE_1, TILE_8 or TILE_16 into a two-vertex DcPrim (top-left, bottom-right) and calls sink; sprites read the current tpage without updating it.
 | Author: suinevere
 | Dependencies: fill_color, fill_uv, decode_xy, apply_tpage, gpu_census_flag
 | Globals: N/A
 | Params: state -- GPU state, w -- command words, fixed_size -- 0 to read width/height from the packet, else the fixed edge length, textured -- shape flag, code -- GP0 command byte, sink, ctx -- primitive callback
 | Returns: N/A
 ----------------------*/
static void decode_rect(DcGpuState *state, const uint32_t *w, int fixed_size, int textured, unsigned code,
                         DcPrimSink sink, void *ctx)
{
    DcPrim prim;
    int16_t width;
    int16_t height;
    uint16_t clut = 0;
    uint16_t tpage = state->tpage;
    unsigned size_idx = 2;
    int bright;

    prim.shape = DC_SHAPE_RECT;
    prim.nverts = 2;
    prim.textured = (uint8_t)(textured != 0);
    prim.raw = (uint8_t)(code & 1u);
    prim.semi = (uint8_t)((code >> 1) & 1u);
    prim.gouraud = 0;
    fill_color(&prim.v[0], w[0]);
    prim.v[1].r = prim.v[0].r;
    prim.v[1].g = prim.v[0].g;
    prim.v[1].b = prim.v[0].b;
    decode_xy(state, w[1], &prim.v[0].x, &prim.v[0].y);
    if (textured)
    {
        fill_uv(&prim.v[0], w[2]);
        clut = (uint16_t)(w[2] >> 16);
        size_idx = 3;
    }
    else
    {
        prim.v[0].u = 0;
        prim.v[0].v = 0;
    }
    if (fixed_size > 0)
    {
        width = (int16_t)fixed_size;
        height = (int16_t)fixed_size;
    }
    else
    {
        width = (int16_t)w[size_idx];
        height = (int16_t)(w[size_idx] >> 16);
    }
    prim.v[1].x = (int16_t)(prim.v[0].x + width);
    prim.v[1].y = (int16_t)(prim.v[0].y + height);
    if (textured)
    {
        prim.v[1].u = (uint8_t)(prim.v[0].u + width);
        prim.v[1].v = (uint8_t)(prim.v[0].v + height);
    }
    else
    {
        prim.v[1].u = 0;
        prim.v[1].v = 0;
    }
    bright = prim.v[0].r > 128 || prim.v[0].g > 128 || prim.v[0].b > 128;
    apply_tpage(&prim, tpage, clut);
    if (prim.textured && !prim.raw && bright)
    {
        gpu_census_flag(CENSUS_BRIGHTEN);
    }
    sink(&prim, state, ctx);
}

/*----------------------
 | decode_line_fixed
 | Description: Decodes a fixed LINE_F2 or LINE_G2 into a two-vertex DcPrim and calls sink.
 | Author: suinevere
 | Dependencies: fill_color, decode_xy, gpu_census_flag
 | Globals: N/A
 | Params: state -- GPU state, w -- command words, gouraud -- shape flag, code -- GP0 command byte, sink, ctx -- primitive callback
 | Returns: N/A
 ----------------------*/
static void decode_line_fixed(DcGpuState *state, const uint32_t *w, int gouraud, unsigned code,
                               DcPrimSink sink, void *ctx)
{
    DcPrim prim;
    unsigned idx = 1;

    prim.shape = DC_SHAPE_LINE;
    prim.nverts = 2;
    prim.textured = 0;
    prim.raw = (uint8_t)(code & 1u);
    prim.semi = (uint8_t)((code >> 1) & 1u);
    prim.gouraud = (uint8_t)(gouraud != 0);
    prim.abr = (uint8_t)((state->tpage >> 5) & 3u);
    prim.depth = 0;
    prim.tpage_x = 0;
    prim.tpage_y = 0;
    prim.clut_x = 0;
    prim.clut_y = 0;
    fill_color(&prim.v[0], w[0]);
    decode_xy(state, w[idx], &prim.v[0].x, &prim.v[0].y);
    idx++;
    if (gouraud)
    {
        fill_color(&prim.v[1], w[idx]);
        idx++;
    }
    else
    {
        prim.v[1].r = prim.v[0].r;
        prim.v[1].g = prim.v[0].g;
        prim.v[1].b = prim.v[0].b;
    }
    decode_xy(state, w[idx], &prim.v[1].x, &prim.v[1].y);
    prim.v[0].u = 0;
    prim.v[0].v = 0;
    prim.v[1].u = 0;
    prim.v[1].v = 0;
    if (prim.semi && prim.abr == 2)
    {
        gpu_census_flag(CENSUS_SUBTRACT);
    }
    sink(&prim, state, ctx);
}

/*----------------------
 | decode_polyline
 | Description: Decodes a variable-length flat or Gouraud polyline into one DC_SHAPE_LINE per segment, stopping at the GP0 polyline terminator pattern.
 | Author: suinevere
 | Dependencies: fill_color, decode_xy, gpu_census_flag
 | Globals: N/A
 | Params: state -- GPU state, w -- command words, remaining -- words available from w[0], gouraud -- shape flag, code -- GP0 command byte, sink, ctx -- primitive callback
 | Returns: words consumed, including the terminator
 ----------------------*/
static unsigned decode_polyline(DcGpuState *state, const uint32_t *w, unsigned remaining, int gouraud,
                                 unsigned code, DcPrimSink sink, void *ctx)
{
    DcPrim prim;
    unsigned idx = 1;
    int have_prev = 0;

    prim.shape = DC_SHAPE_LINE;
    prim.nverts = 2;
    prim.textured = 0;
    prim.raw = (uint8_t)(code & 1u);
    prim.semi = (uint8_t)((code >> 1) & 1u);
    prim.gouraud = (uint8_t)(gouraud != 0);
    prim.abr = (uint8_t)((state->tpage >> 5) & 3u);
    prim.depth = 0;
    prim.tpage_x = 0;
    prim.tpage_y = 0;
    prim.clut_x = 0;
    prim.clut_y = 0;
    prim.v[0].u = 0;
    prim.v[0].v = 0;
    prim.v[1].u = 0;
    prim.v[1].v = 0;
    fill_color(&prim.v[0], w[0]);
    fill_color(&prim.v[1], w[0]);
    while (idx < remaining)
    {
        uint32_t first_word = w[idx];
        uint32_t xy_word;
        DcVertex next;

        if ((first_word & 0xF000F000u) == 0x50005000u)
        {
            idx++;
            break;
        }
        next.u = 0;
        next.v = 0;
        if (gouraud && have_prev)
        {
            fill_color(&next, first_word);
            idx++;
            if (idx >= remaining)
            {
                break;
            }
            xy_word = w[idx];
        }
        else
        {
            next.r = prim.v[0].r;
            next.g = prim.v[0].g;
            next.b = prim.v[0].b;
            xy_word = first_word;
        }
        idx++;
        decode_xy(state, xy_word, &next.x, &next.y);
        if (have_prev)
        {
            prim.v[1] = next;
            if (prim.semi && prim.abr == 2)
            {
                gpu_census_flag(CENSUS_SUBTRACT);
            }
            sink(&prim, state, ctx);
            prim.v[0] = next;
        }
        else
        {
            prim.v[0] = next;
            have_prev = 1;
        }
    }
    return idx;
}

/*----------------------
 | decode_e1
 | Description: Applies GP0 0xE1 (draw mode: tpage, dtd, dfe) to state, flagging a set dither bit.
 | Author: suinevere
 | Dependencies: gpu_census_flag
 | Globals: N/A
 | Params: state -- GPU state, word -- 0xE1 command word
 | Returns: N/A
 ----------------------*/
static void decode_e1(DcGpuState *state, uint32_t word)
{
    state->tpage = (uint16_t)(word & 0xFFFFu);
    if (word & 0x200u)
    {
        gpu_census_flag(CENSUS_DITHER);
    }
}

/*----------------------
 | decode_e2
 | Description: Applies GP0 0xE2 (texture window) to state, flagging a non-zero window.
 | Author: suinevere
 | Dependencies: gpu_census_flag
 | Globals: N/A
 | Params: state -- GPU state, word -- 0xE2 command word
 | Returns: N/A
 ----------------------*/
static void decode_e2(DcGpuState *state, uint32_t word)
{
    uint32_t param = word & 0x00FFFFFFu;

    state->tex_window = param;
    if (param != 0)
    {
        gpu_census_flag(CENSUS_TEX_WINDOW);
    }
}

/*----------------------
 | decode_e3
 | Description: Applies GP0 0xE3 (draw area top-left) to state.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: state -- GPU state, word -- 0xE3 command word
 | Returns: N/A
 ----------------------*/
static void decode_e3(DcGpuState *state, uint32_t word)
{
    state->area_x1 = (int16_t)(word & 0x3FFu);
    state->area_y1 = (int16_t)((word >> 10) & 0x1FFu);
}

/*----------------------
 | decode_e4
 | Description: Applies GP0 0xE4 (draw area bottom-right) to state.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: state -- GPU state, word -- 0xE4 command word
 | Returns: N/A
 ----------------------*/
static void decode_e4(DcGpuState *state, uint32_t word)
{
    state->area_x2 = (int16_t)(word & 0x3FFu);
    state->area_y2 = (int16_t)((word >> 10) & 0x1FFu);
}

/*----------------------
 | decode_e5
 | Description: Applies GP0 0xE5 (draw offset) to state, sign-extending each 11-bit field.
 | Author: suinevere
 | Dependencies: sign_extend11
 | Globals: N/A
 | Params: state -- GPU state, word -- 0xE5 command word
 | Returns: N/A
 ----------------------*/
static void decode_e5(DcGpuState *state, uint32_t word)
{
    state->ofs_x = sign_extend11((uint16_t)(word & 0x7FFu));
    state->ofs_y = sign_extend11((uint16_t)((word >> 11) & 0x7FFu));
}

/*----------------------
 | decode_one
 | Description: Dispatches the single GP0 command at w[0] to its decoder, noting its code in the census; 0x02 (fill rectangle) is skipped without emitting a primitive.
 | Author: suinevere
 | Dependencies: gpu_census_note, gpu_census_unhandled, decode_e1..decode_e5, decode_poly, decode_rect, decode_line_fixed, decode_polyline
 | Globals: N/A
 | Params: state -- GPU state, w -- command words starting at the command's own word, remaining -- words available from w[0], sink, ctx -- primitive callback
 | Returns: words consumed by this command
 ----------------------*/
static unsigned decode_one(DcGpuState *state, const uint32_t *w, unsigned remaining, DcPrimSink sink, void *ctx)
{
    unsigned code = (unsigned)(w[0] >> 24) & 0xFFu;

    gpu_census_note(code);
    if (code == 0x00u)
    {
        return 1;
    }
    if (code == 0xE1u)
    {
        decode_e1(state, w[0]);
        return 1;
    }
    if (code == 0xE2u)
    {
        decode_e2(state, w[0]);
        return 1;
    }
    if (code == 0xE3u)
    {
        decode_e3(state, w[0]);
        return 1;
    }
    if (code == 0xE4u)
    {
        decode_e4(state, w[0]);
        return 1;
    }
    if (code == 0xE5u)
    {
        decode_e5(state, w[0]);
        return 1;
    }
    if (code == 0xE6u)
    {
        return 1;
    }
    if (code == 0x02u)
    {
        return 3;
    }
    switch (code & 0xFCu)
    {
    case 0x20: decode_poly(state, w, 3, 0, 0, code, sink, ctx); return 4;
    case 0x24: decode_poly(state, w, 3, 1, 0, code, sink, ctx); return 7;
    case 0x30: decode_poly(state, w, 3, 0, 1, code, sink, ctx); return 6;
    case 0x34: decode_poly(state, w, 3, 1, 1, code, sink, ctx); return 9;
    case 0x28: decode_poly(state, w, 4, 0, 0, code, sink, ctx); return 5;
    case 0x2C: decode_poly(state, w, 4, 1, 0, code, sink, ctx); return 9;
    case 0x38: decode_poly(state, w, 4, 0, 1, code, sink, ctx); return 8;
    case 0x3C: decode_poly(state, w, 4, 1, 1, code, sink, ctx); return 12;
    case 0x40: decode_line_fixed(state, w, 0, code, sink, ctx); return 3;
    case 0x50: decode_line_fixed(state, w, 1, code, sink, ctx); return 4;
    case 0x60: decode_rect(state, w, 0, 0, code, sink, ctx); return 3;
    case 0x64: decode_rect(state, w, 0, 1, code, sink, ctx); return 4;
    case 0x68: decode_rect(state, w, 1, 0, code, sink, ctx); return 2;
    case 0x70: decode_rect(state, w, 8, 0, code, sink, ctx); return 2;
    case 0x74: decode_rect(state, w, 8, 1, code, sink, ctx); return 3;
    case 0x78: decode_rect(state, w, 16, 0, code, sink, ctx); return 2;
    case 0x7C: decode_rect(state, w, 16, 1, code, sink, ctx); return 3;
    default: break;
    }
    if ((code & 0xF8u) == 0x48u)
    {
        return decode_polyline(state, w, remaining, 0, code, sink, ctx);
    }
    if ((code & 0xF8u) == 0x58u)
    {
        return decode_polyline(state, w, remaining, 1, code, sink, ctx);
    }
    gpu_census_unhandled(code);
    return 1;
}

/*----------------------
 | gpu_decode_reset
 | Description: Zeroes a GPU state: no draw area, no offset, tpage 0 and no texture window.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: state -- state to reset
 | Returns: N/A
 ----------------------*/
void gpu_decode_reset(DcGpuState *state)
{
    state->area_x1 = 0;
    state->area_y1 = 0;
    state->area_x2 = 0;
    state->area_y2 = 0;
    state->ofs_x = 0;
    state->ofs_y = 0;
    state->tpage = 0;
    state->tex_window = 0;
}

/*----------------------
 | gpu_decode_packet
 | Description: Decodes every GP0 command packed into one ordering-table packet's body, updating state and calling sink for each primitive.
 | Author: suinevere
 | Dependencies: decode_one
 | Globals: N/A
 | Params: state -- GPU state, words -- packet body (no tag word), len -- word count, sink -- primitive callback, ctx -- sink context
 | Returns: words consumed (equal to len on a well-formed packet)
 ----------------------*/
unsigned gpu_decode_packet(DcGpuState *state, const uint32_t *words, unsigned len, DcPrimSink sink, void *ctx)
{
    unsigned i = 0;

    while (i < len)
    {
        unsigned consumed = decode_one(state, words + i, len - i, sink, ctx);

        i += (consumed != 0) ? consumed : 1;
    }
    return i;
}

/*----------------------
 | gpu_decode_ot
 | Description: Walks an ordering-table packet chain and decodes every command in every non-empty packet, in chain order.
 | Author: suinevere
 | Dependencies: gpu_tag_addr, gpu_tag_len, gpu_decode_pointer, gpu_decode_packet, dc_halt
 | Globals: N/A
 | Params: state -- GPU state, ot_head -- first packet of the chain, sink -- primitive callback, ctx -- sink context
 | Returns: N/A
 ----------------------*/
void gpu_decode_ot(DcGpuState *state, const u_long *ot_head, DcPrimSink sink, void *ctx)
{
    const u_long *p = ot_head;
    unsigned visited = 0;

    for (;;)
    {
        uint32_t tag = (uint32_t)p[0];
        uint32_t raw = gpu_tag_addr(tag);
        unsigned len = gpu_tag_len(tag);

        if (len != 0)
        {
            gpu_decode_packet(state, (const uint32_t *)(p + 1), len, sink, ctx);
        }
        if (raw == GPU_TAG_END)
        {
            break;
        }
        if (++visited >= GPU_DECODE_MAX_PACKETS)
        {
            dc_halt("ot_walk: loop");
            return;
        }
        p = gpu_decode_pointer(raw);
    }
}
