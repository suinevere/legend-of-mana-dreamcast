/*----------------------
 | mdec.c
 | Description: Software PS1 MDEC following psx-spx's rl_decode_block and yuv_to_rgb, with libjpeg's fast integer IDCT in place of the hardware's matrix passes.
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
 | mdec_init_tables
 | Description: Builds the zigzag quant table and the DCT basis scaled to signed 16 bits, rounded down as psx-spx lists it.
 | Author: suinevere
 | Dependencies: cos, floor
 | Globals: s_quant, s_scale, s_clamp
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
 | IDCT_CONST_BITS / IDCT_PASS1_BITS / IDCT_FIX_*
 | Description: Fixed-point scale of libjpeg's accurate integer IDCT and its rotation constants (value * 2^13).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define IDCT_CONST_BITS 13
#define IDCT_PASS1_BITS 2
#define IDCT_FIX_0_298631336 2446
#define IDCT_FIX_0_390180644 3196
#define IDCT_FIX_0_541196100 4433
#define IDCT_FIX_0_765366865 6270
#define IDCT_FIX_0_899976223 7373
#define IDCT_FIX_1_175875602 9633
#define IDCT_FIX_1_501321110 12299
#define IDCT_FIX_1_847759065 15137
#define IDCT_FIX_1_961570560 16069
#define IDCT_FIX_2_053119869 16819
#define IDCT_FIX_2_562915447 20995
#define IDCT_FIX_3_072711026 25172

/*----------------------
 | idct_1d
 | Description: One 8-point Loeffler IDCT along a stride, as libjpeg's jidctint does it, rounding its outputs down by shift.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: in -- 8 inputs at in[0], in[stride], ..., out -- 8 outputs at the same stride, stride -- element step, shift -- descale bits, pre -- input shift for the DC-only shortcut
 | Returns: N/A
 ----------------------*/
static void idct_1d(const int32_t *in, int32_t *out, int stride, int shift, int pre)
{
    int32_t tmp0, tmp1, tmp2, tmp3, tmp10, tmp11, tmp12, tmp13, z1, z2, z3, z4, z5;
    int32_t round = (int32_t)1 << (shift - 1);
    int i;

    if (in[stride] == 0 && in[2 * stride] == 0 && in[3 * stride] == 0 && in[4 * stride] == 0 && in[5 * stride] == 0 &&
        in[6 * stride] == 0 && in[7 * stride] == 0)
    {
        int32_t dc = pre >= 0 ? in[0] << pre : (in[0] + ((int32_t)1 << (-pre - 1))) >> -pre;

        for (i = 0; i < 8; i++)
        {
            out[i * stride] = dc;
        }
        return;
    }
    z2 = in[2 * stride];
    z3 = in[6 * stride];
    z1 = (z2 + z3) * IDCT_FIX_0_541196100;
    tmp2 = z1 - z3 * IDCT_FIX_1_847759065;
    tmp3 = z1 + z2 * IDCT_FIX_0_765366865;
    tmp0 = (in[0] + in[4 * stride]) * ((int32_t)1 << IDCT_CONST_BITS);
    tmp1 = (in[0] - in[4 * stride]) * ((int32_t)1 << IDCT_CONST_BITS);
    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;
    tmp0 = in[7 * stride];
    tmp1 = in[5 * stride];
    tmp2 = in[3 * stride];
    tmp3 = in[stride];
    z1 = tmp0 + tmp3;
    z2 = tmp1 + tmp2;
    z3 = tmp0 + tmp2;
    z4 = tmp1 + tmp3;
    z5 = (z3 + z4) * IDCT_FIX_1_175875602;
    tmp0 *= IDCT_FIX_0_298631336;
    tmp1 *= IDCT_FIX_2_053119869;
    tmp2 *= IDCT_FIX_3_072711026;
    tmp3 *= IDCT_FIX_1_501321110;
    z1 *= -IDCT_FIX_0_899976223;
    z2 *= -IDCT_FIX_2_562915447;
    z3 = z3 * -IDCT_FIX_1_961570560 + z5;
    z4 = z4 * -IDCT_FIX_0_390180644 + z5;
    tmp0 += z1 + z3;
    tmp1 += z2 + z4;
    tmp2 += z2 + z3;
    tmp3 += z1 + z4;
    out[0] = (tmp10 + tmp3 + round) >> shift;
    out[7 * stride] = (tmp10 - tmp3 + round) >> shift;
    out[stride] = (tmp11 + tmp2 + round) >> shift;
    out[6 * stride] = (tmp11 - tmp2 + round) >> shift;
    out[2 * stride] = (tmp12 + tmp1 + round) >> shift;
    out[5 * stride] = (tmp12 - tmp1 + round) >> shift;
    out[3 * stride] = (tmp13 + tmp0 + round) >> shift;
    out[4 * stride] = (tmp13 - tmp0 + round) >> shift;
}

/*----------------------
 | idct
 | Description: The orthonormal 2-D IDCT the MDEC's two matrix passes compute (each pass is the 1-D IDCT times one half), done with libjpeg's fast integer algorithm; differs from the hardware only in rounding.
 | Author: suinevere
 | Dependencies: idct_1d
 | Globals: N/A
 | Params: blk -- 64 coefficients in raster order, replaced by samples
 | Returns: N/A
 ----------------------*/
static void idct(int32_t blk[64])
{
    int32_t ws[64];
    int i;

    for (i = 0; i < 8; i++)
    {
        idct_1d(&blk[i], &ws[i], 8, IDCT_CONST_BITS - IDCT_PASS1_BITS, IDCT_PASS1_BITS);
    }
    for (i = 0; i < 8; i++)
    {
        idct_1d(&ws[i * 8], &blk[i * 8], 1, IDCT_CONST_BITS + IDCT_PASS1_BITS + 3, -(IDCT_PASS1_BITS + 3));
    }
}

/*----------------------
 | decode_block
 | Description: Dequantises one block's run-length codes into raster order and runs the IDCT.
 | Author: suinevere
 | Dependencies: signed10, idct
 | Globals: s_quant, s_zagzig
 | Params: s -- stream, blk -- 64 outputs
 | Returns: 0, or -1 when the stream ends inside the block
 ----------------------*/
static int decode_block(MdecStream *s, int32_t blk[64])
{
    int k = 0;
    int q, val, n;

    memset(blk, 0, 64 * sizeof(int32_t));
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
        blk[q ? s_zagzig[k] : k] = val;
        if (s->src >= s->end)
        {
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
    idct(blk);
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
 | put_luma_block
 | Description: Converts one 8x8 Y block at (xx, yy) of the macroblock to RGB with the shared chroma offsets.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_clamp
 | Params: s -- stream (output format), y -- Y block, deltas -- chroma offsets, xx, yy -- block origin, out -- macroblock
 | Returns: N/A
 ----------------------*/
static void put_luma_block(const MdecStream *s, const int32_t *y, const ChromaDelta *deltas, int xx, int yy, uint8_t *out)
{
    const uint8_t *clamp = s_clamp + CLAMP_BIAS;
    int px, py;

    for (py = 0; py < 8; py++)
    {
        const ChromaDelta *row = &deltas[((py + yy) >> 1) * 8 + (xx >> 1)];

        for (px = 0; px < 8; px++)
        {
            const ChromaDelta *d = &row[px >> 1];
            int lum = y[px + py * 8];
            int r = clamp[lum + d->r];
            int g = clamp[lum + d->g];
            int b = clamp[lum + d->b];
            int at = (px + xx) + (py + yy) * 16;

            if (s->depth24)
            {
                out[at * 3] = (uint8_t)r;
                out[at * 3 + 1] = (uint8_t)g;
                out[at * 3 + 2] = (uint8_t)b;
            }
            else
            {
                unsigned v = (unsigned)(r >> 3) | ((unsigned)(g >> 3) << 5) | ((unsigned)(b >> 3) << 10) | ((unsigned)s->stp << 15);

                out[at * 2] = (uint8_t)v;
                out[at * 2 + 1] = (uint8_t)(v >> 8);
            }
        }
    }
}

/*----------------------
 | mdec_decode_macroblock
 | Description: Decodes Cr, Cb and Y blocks at (0,0), (8,0), (0,8), (8,8) into one macroblock.
 | Author: suinevere
 | Dependencies: decode_block, chroma_deltas, put_luma_block
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
        put_luma_block(s, y, deltas, origin[i][0], origin[i][1], out);
    }
    return 0;
}
