/*----------------------
 | mdec.c
 | Description: Software PS1 MDEC following psx-spx's rl_decode_block and yuv_to_rgb, with libjpeg's float AAN IDCT in place of the hardware's matrix passes.
 | Author: suinevere
 | Dependencies: mdec.h, math.h, string.h
 | Globals: s_zagzig, s_default_quant, s_quant, s_scale
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <math.h>
#include <string.h>

#include "mdec.h"

/*----------------------
 | s_zagzig
 | Description: Raster position of each zigzag index.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint8_t s_zagzig[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

/*----------------------
 | s_default_quant
 | Description: The quant matrix libpress DecDCTReset uploads, in raster order.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint8_t s_default_quant[64] = {
    2, 16, 19, 22, 26, 27, 29, 34, 16, 16, 22, 24, 27, 29, 34, 37, 19, 22, 26, 27, 29, 34, 34, 38, 22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48, 26, 27, 29, 32, 35, 40, 48, 58, 26, 27, 29, 34, 38, 46, 56, 69, 27, 29, 35, 38, 46, 56, 69, 83};

/*----------------------
 | s_quant / s_scale
 | Description: Quant table in zigzag order, and the hardware's IDCT scale table, kept for reference and tests.
 | Author: suinevere
 | Dependencies: mdec_init_tables
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint8_t s_quant[64];
static int16_t s_scale[64];

/*----------------------
 | CLAMP_BIAS / s_clamp
 | Description: Lookup clamping a signed luma-plus-chroma sum to -128..127 and offsetting it to 0..255; index is the sum plus CLAMP_BIAS.
 | Author: suinevere
 | Dependencies: mdec_init_tables
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CLAMP_BIAS 1024
static uint8_t s_clamp[2 * CLAMP_BIAS];

/*----------------------
 | s_prescale
 | Description: Per raster position, the AAN scale of its row and column over 8, folding the IDCT's prescale and final descale into one multiply at dequantisation.
 | Author: suinevere
 | Dependencies: mdec_init_tables
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static float s_prescale[64];

/*----------------------
 | mdec_init_tables
 | Description: Builds the zigzag quant table and the DCT basis scaled to signed 16 bits, rounded down as psx-spx lists it, and the AAN prescale per coefficient.
 | Author: suinevere
 | Dependencies: cos, floor
 | Globals: s_quant, s_scale, s_prescale, s_clamp
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void mdec_init_tables(void)
{
    int k, n;

    for (k = 0; k < 64; k++)
    {
        s_quant[k] = s_default_quant[s_zagzig[k]];
    }
    for (k = 0; k < 8; k++)
    {
        for (n = 0; n < 8; n++)
        {
            double c = (k == 0) ? 0.70710678118654752 : 1.0;
            long r = (long)floor(c * cos((2 * n + 1) * k * 3.14159265358979323846 / 16.0) * 32768.0);

            s_scale[k * 8 + n] = (int16_t)(r > 32767 ? 32767 : r);
        }
    }
    for (k = 0; k < 64; k++)
    {
        double row = (k >> 3) == 0 ? 1.0 : cos((k >> 3) * 3.14159265358979323846 / 16.0) * 1.41421356237309504880;
        double col = (k & 7) == 0 ? 1.0 : cos((k & 7) * 3.14159265358979323846 / 16.0) * 1.41421356237309504880;

        s_prescale[k] = (float)(row * col / 8.0);
    }
    for (k = 0; k < 2 * CLAMP_BIAS; k++)
    {
        int v = k - CLAMP_BIAS;

        s_clamp[k] = (uint8_t)((v < -128 ? -128 : (v > 127 ? 127 : v)) + 128);
    }
}

/*----------------------
 | mdec_scale_table
 | Description: Returns the IDCT scale table.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_scale
 | Params: N/A
 | Returns: s_scale
 ----------------------*/
const int16_t *mdec_scale_table(void)
{
    return s_scale;
}

/*----------------------
 | mdec_quant_table
 | Description: Returns the zigzag quant table.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_quant
 | Params: N/A
 | Returns: s_quant
 ----------------------*/
const uint8_t *mdec_quant_table(void)
{
    return s_quant;
}

/*----------------------
 | mdec_begin
 | Description: Points the stream at the halfwords after the command word and records the output format.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: s -- stream, command -- MDEC decode command, depth24 -- 24-bit output, stp -- 15-bit bit 15
 | Returns: N/A
 ----------------------*/
void mdec_begin(MdecStream *s, const uint32_t *command, int depth24, int stp)
{
    s->src = (const uint16_t *)(command + 1);
    s->end = s->src + 2u * (command[0] & 0xFFFFu);
    s->depth24 = depth24;
    s->stp = stp;
}

/*----------------------
 | signed10
 | Description: Sign-extends a 10-bit field.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- 10-bit value
 | Returns: -512..511
 ----------------------*/
static int signed10(int v)
{
    return (v & 0x200) ? v - 0x400 : v;
}

/*----------------------
 | AAN_SQRT2 / AAN_C2 / AAN_C6_PLUS / AAN_C6_MINUS / OUTPUT_BIAS
 | Description: libjpeg jidctflt's AAN rotation constants, and the bias that makes truncation round non-negative.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AAN_SQRT2 1.414213562f
#define AAN_C2 1.847759065f
#define AAN_C6_PLUS 1.082392200f
#define AAN_C6_MINUS 2.613125930f
#define OUTPUT_BIAS 8192.5f

/*----------------------
 | idct_1d
 | Description: One 8-point AAN IDCT along a stride, as libjpeg's jidctflt does it, in place; five multiplies against the integer version's twelve, on the pipelined FPU.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- 8 values at v[0], v[stride], ..., stride -- element step
 | Returns: N/A
 ----------------------*/
static inline void idct_1d(float *v, int stride)
{
    float tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp10, tmp11, tmp12, tmp13, z5, z10, z11, z12, z13;

    tmp10 = v[0] + v[4 * stride];
    tmp11 = v[0] - v[4 * stride];
    tmp13 = v[2 * stride] + v[6 * stride];
    tmp12 = (v[2 * stride] - v[6 * stride]) * AAN_SQRT2 - tmp13;
    tmp0 = tmp10 + tmp13;
    tmp3 = tmp10 - tmp13;
    tmp1 = tmp11 + tmp12;
    tmp2 = tmp11 - tmp12;
    z13 = v[5 * stride] + v[3 * stride];
    z10 = v[5 * stride] - v[3 * stride];
    z11 = v[stride] + v[7 * stride];
    z12 = v[stride] - v[7 * stride];
    tmp7 = z11 + z13;
    tmp11 = (z11 - z13) * AAN_SQRT2;
    z5 = (z10 + z12) * AAN_C2;
    tmp10 = AAN_C6_PLUS * z12 - z5;
    tmp12 = z5 - AAN_C6_MINUS * z10;
    tmp6 = tmp12 - tmp7;
    tmp5 = tmp11 - tmp6;
    tmp4 = tmp10 + tmp5;
    v[0] = tmp0 + tmp7;
    v[7 * stride] = tmp0 - tmp7;
    v[stride] = tmp1 + tmp6;
    v[6 * stride] = tmp1 - tmp6;
    v[2 * stride] = tmp2 + tmp5;
    v[5 * stride] = tmp2 - tmp5;
    v[4 * stride] = tmp3 + tmp4;
    v[3 * stride] = tmp3 - tmp4;
}

/*----------------------
 | idct
 | Description: The 2-D IDCT of prescaled coefficients, rounded to the nearest integer; columns holding nothing beyond DC skip their pass, and a block whose terms all sit in column 0 fills each row from its first value.
 | Author: suinevere
 | Dependencies: idct_1d
 | Globals: N/A
 | Params: coef -- 64 prescaled coefficients in raster order, left all zero, ac_cols -- bit c set when column c has a term below row 0, any_cols -- bit c set when column c has any term, blk -- receives the 64 samples
 | Returns: N/A
 ----------------------*/
static void idct(float coef[64], unsigned ac_cols, unsigned any_cols, int32_t blk[64])
{
    float ws[64];
    int i, c;

    for (c = 0; c < 8; c++)
    {
        if (ac_cols & (1u << c))
        {
            for (i = 0; i < 8; i++)
            {
                ws[i * 8 + c] = coef[i * 8 + c];
            }
            idct_1d(&ws[c], 8);
        }
        else
        {
            float dc = coef[c];

            for (i = 0; i < 8; i++)
            {
                ws[i * 8 + c] = dc;
            }
        }
    }
    for (i = 0; i < 8; i++)
    {
        float *row = &ws[i * 8];
        int32_t *out = &blk[i * 8];

        if (any_cols != 1u)
        {
            idct_1d(row, 1);
            for (c = 0; c < 8; c++)
            {
                out[c] = (int32_t)(row[c] + OUTPUT_BIAS) - 8192;
            }
        }
        else
        {
            int32_t v = (int32_t)(row[0] + OUTPUT_BIAS) - 8192;

            for (c = 0; c < 8; c++)
            {
                out[c] = v;
            }
        }
    }
}

/*----------------------
 | s_coef / s_coef_written / s_coef_count
 | Description: Prescaled coefficient workspace kept all zero between blocks, the raster positions the current block wrote, and how many, so a block clears only what it set instead of all 64.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static float s_coef[64];
static uint8_t s_coef_written[64];
static int s_coef_count;

/*----------------------
 | clear_coefficients
 | Description: Zeroes the positions the last block wrote, returning the workspace to all zero.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_coef, s_coef_written, s_coef_count
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void clear_coefficients(void)
{
    int i;

    for (i = 0; i < s_coef_count; i++)
    {
        s_coef[s_coef_written[i]] = 0;
    }
    s_coef_count = 0;
}

/*----------------------
 | decode_block
 | Description: Dequantises one block's run-length codes into raster order and runs the IDCT; a block holding only its DC term is filled straight with the value the IDCT would give every pixel.
 | Author: suinevere
 | Dependencies: signed10, idct, clear_coefficients
 | Globals: s_quant, s_zagzig, s_prescale, s_coef, s_coef_written, s_coef_count
 | Params: s -- stream, blk -- 64 outputs
 | Returns: 0, or -1 when the stream ends inside the block
 ----------------------*/
static int decode_block(MdecStream *s, int32_t blk[64])
{
    int k = 0;
    int q, val, n, at;
    int dc_val = 0;
    unsigned ac_cols = 0;
    unsigned any_cols = 0;

    do
    {
        if (s->src >= s->end)
        {
            return -1;
        }
        n = *s->src++;
    } while (n == MDEC_EOB);
    q = (n >> 10) & 0x3F;
    val = signed10(n & 0x3FF) * s_quant[0];
    for (;;)
    {
        if (q == 0)
        {
            val = signed10(n & 0x3FF) * 2;
        }
        val = val < -0x400 ? -0x400 : (val > 0x3FF ? 0x3FF : val);
        at = q ? s_zagzig[k] : k;
        s_coef[at] = (float)val * s_prescale[at];
        s_coef_written[s_coef_count++] = (uint8_t)at;
        any_cols |= 1u << (at & 7);
        if (at >= 8)
        {
            ac_cols |= 1u << (at & 7);
        }
        if (at == 0)
        {
            dc_val = val;
        }
        if (s->src >= s->end)
        {
            clear_coefficients();
            return -1;
        }
        n = *s->src++;
        k += ((n >> 10) & 0x3F) + 1;
        if (k > 63)
        {
            break;
        }
        val = (signed10(n & 0x3FF) * s_quant[k] * q + 4) / 8;
    }
    if (s_coef_count == 1 && s_coef_written[0] == 0)
    {
        int32_t dc = (dc_val + 4) >> 3;
        int i;

        for (i = 0; i < 64; i++)
        {
            blk[i] = dc;
        }
    }
    else
    {
        idct(s_coef, ac_cols, any_cols, blk);
    }
    clear_coefficients();
    return 0;
}

/*----------------------
 | ChromaDelta
 | Description: The red, green and blue offsets one chroma sample adds to the four luma pixels it covers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    int16_t r;
    int16_t g;
    int16_t b;
} ChromaDelta;

/*----------------------
 | chroma_deltas
 | Description: Converts the macroblock's 64 chroma samples to colour offsets once, shared by its four luma blocks.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: cr, cb -- chroma blocks, out -- 64 offsets
 | Returns: N/A
 ----------------------*/
static void chroma_deltas(const int32_t *cr, const int32_t *cb, ChromaDelta out[64])
{
    int c;

    for (c = 0; c < 64; c++)
    {
        int vr = cr[c];
        int vb = cb[c];

        out[c].r = (int16_t)((vr * 1435) >> 10);
        out[c].g = (int16_t)(-((vb * 352 + vr * 731) >> 10));
        out[c].b = (int16_t)((vb * 1814) >> 10);
    }
}

/*----------------------
 | put_luma_block_24
 | Description: Converts one 8x8 Y block at (xx, yy) of the macroblock to 24-bit RGB with the shared chroma offsets, two pixels per chroma sample.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_clamp
 | Params: y -- Y block, deltas -- chroma offsets, xx, yy -- block origin, out -- macroblock
 | Returns: N/A
 ----------------------*/
static void put_luma_block_24(const int32_t *y, const ChromaDelta *deltas, int xx, int yy, uint8_t *out)
{
    const uint8_t *clamp = s_clamp + CLAMP_BIAS;
    int px, py;

    for (py = 0; py < 8; py++)
    {
        const ChromaDelta *d = &deltas[((py + yy) >> 1) * 8 + (xx >> 1)];
        const int32_t *row = &y[py * 8];
        uint8_t *dst = out + ((py + yy) * 16 + xx) * 3;

        for (px = 0; px < 8; px += 2, d++, row += 2, dst += 6)
        {
            int dr = d->r;
            int dg = d->g;
            int db = d->b;
            int l0 = row[0];
            int l1 = row[1];

            dst[0] = clamp[l0 + dr];
            dst[1] = clamp[l0 + dg];
            dst[2] = clamp[l0 + db];
            dst[3] = clamp[l1 + dr];
            dst[4] = clamp[l1 + dg];
            dst[5] = clamp[l1 + db];
        }
    }
}

/*----------------------
 | put_luma_block_15
 | Description: Converts one 8x8 Y block at (xx, yy) of the macroblock to 15-bit pixels with the stream's STP bit, two pixels per chroma sample.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_clamp
 | Params: y -- Y block, deltas -- chroma offsets, xx, yy -- block origin, stp -- bit 15 of each pixel, out -- macroblock
 | Returns: N/A
 ----------------------*/
static void put_luma_block_15(const int32_t *y, const ChromaDelta *deltas, int xx, int yy, unsigned stp, uint8_t *out)
{
    const uint8_t *clamp = s_clamp + CLAMP_BIAS;
    unsigned top = stp << 15;
    int px, py;

    for (py = 0; py < 8; py++)
    {
        const ChromaDelta *d = &deltas[((py + yy) >> 1) * 8 + (xx >> 1)];
        const int32_t *row = &y[py * 8];
        uint8_t *dst = out + ((py + yy) * 16 + xx) * 2;

        for (px = 0; px < 8; px += 2, d++, row += 2, dst += 4)
        {
            int dr = d->r;
            int dg = d->g;
            int db = d->b;
            unsigned v0 = (unsigned)(clamp[row[0] + dr] >> 3) | ((unsigned)(clamp[row[0] + dg] >> 3) << 5) | ((unsigned)(clamp[row[0] + db] >> 3) << 10) | top;
            unsigned v1 = (unsigned)(clamp[row[1] + dr] >> 3) | ((unsigned)(clamp[row[1] + dg] >> 3) << 5) | ((unsigned)(clamp[row[1] + db] >> 3) << 10) | top;

            dst[0] = (uint8_t)v0;
            dst[1] = (uint8_t)(v0 >> 8);
            dst[2] = (uint8_t)v1;
            dst[3] = (uint8_t)(v1 >> 8);
        }
    }
}

/*----------------------
 | mdec_decode_macroblock
 | Description: Decodes Cr, Cb and Y blocks at (0,0), (8,0), (0,8), (8,8) into one macroblock.
 | Author: suinevere
 | Dependencies: decode_block, chroma_deltas, put_luma_block_24, put_luma_block_15
 | Globals: N/A
 | Params: s -- stream, out -- 768 or 512 bytes
 | Returns: 0, or -1 with out cleared once the data runs out
 ----------------------*/
int mdec_decode_macroblock(MdecStream *s, uint8_t *out)
{
    static const int origin[4][2] = {{0, 0}, {8, 0}, {0, 8}, {8, 8}};
    int32_t cr[64], cb[64], y[64];
    ChromaDelta deltas[64];
    int i;

    if (decode_block(s, cr) != 0 || decode_block(s, cb) != 0)
    {
        memset(out, 0, s->depth24 ? MDEC_MB_BYTES_24 : MDEC_MB_BYTES_15);
        return -1;
    }
    chroma_deltas(cr, cb, deltas);
    for (i = 0; i < 4; i++)
    {
        if (decode_block(s, y) != 0)
        {
            memset(out, 0, s->depth24 ? MDEC_MB_BYTES_24 : MDEC_MB_BYTES_15);
            return -1;
        }
        if (s->depth24)
        {
            put_luma_block_24(y, deltas, origin[i][0], origin[i][1], out);
        }
        else
        {
            put_luma_block_15(y, deltas, origin[i][0], origin[i][1], (unsigned)s->stp, out);
        }
    }
    return 0;
}
