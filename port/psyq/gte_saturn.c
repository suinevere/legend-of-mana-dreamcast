/*----------------------
 | gte_saturn.c
 | Description: Bit-exact C model of the PS1 GTE operations the decomp uses, from psx-spx, plus libgte translated from the SLUS_010.13 MIPS.
 | Author: suinevere
 | Dependencies: gte_saturn.h, libgte.h, libgte_tables.h (generated), string.h
 | Globals: lom_gte, lom_libgte_tables, gte_unr_table, gte_unr_ready, gte_matrix_stack, gte_matrix_depth
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>

#include "gte_saturn.h"
#include "libgte.h"
#include "libgte_tables.h"

/*----------------------
 | GTE_FLAG_ERROR_MASK
 | Description: FLAG bits 30-23 and 18-13, whose OR sets the error bit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_ERROR_MASK 0x7F87E000u

/*----------------------
 | GTE_FLAG_ERROR
 | Description: FLAG bit 31, the error summary bit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_ERROR 0x80000000u

/*----------------------
 | GTE_FLAG_COLOR_R
 | Description: FLAG bit for red colour FIFO saturation; green and blue are the next two bits down.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_COLOR_R 21

/*----------------------
 | GTE_FLAG_SZ_OTZ
 | Description: FLAG bit for SZ3 or OTZ saturation to 0..0xFFFF.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_SZ_OTZ 18

/*----------------------
 | GTE_FLAG_DIVIDE
 | Description: FLAG bit for RTP divide overflow.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_DIVIDE 17

/*----------------------
 | GTE_FLAG_MAC0_POS
 | Description: FLAG bit for MAC0 above 0x7FFFFFFF.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_MAC0_POS 16

/*----------------------
 | GTE_FLAG_MAC0_NEG
 | Description: FLAG bit for MAC0 below -0x80000000.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_MAC0_NEG 15

/*----------------------
 | GTE_FLAG_SX
 | Description: FLAG bit for SX2 saturation to -0x400..0x3FF.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_SX 14

/*----------------------
 | GTE_FLAG_SY
 | Description: FLAG bit for SY2 saturation to -0x400..0x3FF.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_SY 13

/*----------------------
 | GTE_FLAG_IR0
 | Description: FLAG bit for IR0 saturation to 0..0x1000.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FLAG_IR0 12

/*----------------------
 | GTE_MAC44_MAX
 | Description: Largest value a MAC1-MAC3 accumulation step may reach without the positive overflow flag.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_MAC44_MAX 0x7FFFFFFFFFFLL

/*----------------------
 | GTE_MAC44_MIN
 | Description: Smallest value a MAC1-MAC3 accumulation step may reach without the negative overflow flag.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_MAC44_MIN (-0x80000000000LL)

/*----------------------
 | GTE_MATRIX_STACK_DEPTH
 | Description: Entries in the libgte PushMatrix/PopMatrix stack.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_MATRIX_STACK_DEPTH 20

/*----------------------
 | GTE_ANGLE_MASK
 | Description: One full turn of a libgte angle minus one; angles wrap modulo 4096.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_ANGLE_MASK 0xFFF

/*----------------------
 | GTE_QUARTER_TURN
 | Description: A quarter turn in libgte angle units.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_QUARTER_TURN 1024

/*----------------------
 | GTE_HALF_TURN
 | Description: A half turn in libgte angle units.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_HALF_TURN 2048

/*----------------------
 | GTE_THREE_QUARTER_TURN
 | Description: Three quarter turns in libgte angle units.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_THREE_QUARTER_TURN 3072

/*----------------------
 | GTE_FULL_TURN
 | Description: A full turn in libgte angle units.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FULL_TURN 4096

/*----------------------
 | GTE_CORDIC_GAIN
 | Description: csincos's starting x, 0.607 (the CORDIC gain) in 4.12 fixed point.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_CORDIC_GAIN 2487

/*----------------------
 | GTE_CORDIC_STEPS
 | Description: Rotation steps csincos runs before its small-angle correction.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_CORDIC_STEPS 6

/*----------------------
 | GTE_LZC_ZERO
 | Description: LZCR for an input of 0 or -1, which SquareRoot0 answers with 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_LZC_ZERO 32

/*----------------------
 | GTE_ROOT_BIAS
 | Description: First index of the square-root tables; a normalised input lies in 64..255.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_ROOT_BIAS 64

/*----------------------
 | GTE_ROOT_NORM
 | Description: Bit position that normalisation moves the input's leading pair of bits to.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_ROOT_NORM 24

/*----------------------
 | GTE_RATAN_WIDE
 | Description: Bits of a ratan2 operand above 0x1FFFFF, where it divides by the other shifted down instead of up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_RATAN_WIDE 0x7FE00000

/*----------------------
 | GTE_RATAN_SHIFT
 | Description: Fraction bits of the ratan2 table index.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_RATAN_SHIFT 10

/*----------------------
 | GTE_LV_SPLIT
 | Description: ApplyMatrixLV splits each 32-bit component into a high part and these many low bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_LV_SPLIT 15

/*----------------------
 | GTE_LV_LOW_MASK
 | Description: Mask of ApplyMatrixLV's 15 low bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_LV_LOW_MASK 0x7FFF

/*----------------------
 | GTE_LV_HIGH_SCALE
 | Description: Shift that rescales the high-part product: 15 split bits less the 12-bit fixed point.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_LV_HIGH_SCALE 3

/*----------------------
 | GTE_FIXED_SHIFT
 | Description: Fraction bits of libgte's 4.12 fixed point.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_FIXED_SHIFT 12

/*----------------------
 | GTE_MATRIX_PAD_OFFSET
 | Description: Byte offset of the padding after m[2][2], which libgte's word stores of m[2][2] overwrite.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GTE_MATRIX_PAD_OFFSET 18

/*----------------------
 | GteMatrixFrame
 | Description: One PushMatrix entry holding the rotation matrix and translation vector.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct GteMatrixFrame
{
    int16_t rt[3][3];
    int32_t tr[3];
} GteMatrixFrame;

/*----------------------
 | lom_gte
 | Description: The emulated GTE register file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
GteRegisters lom_gte;

/*----------------------
 | gte_unr_table
 | Description: The 0x101-entry UNR reciprocal seed table, filled from the psx-spx generator formula.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint8_t gte_unr_table[0x101];

/*----------------------
 | gte_unr_ready
 | Description: Non-zero once gte_unr_table has been filled.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int gte_unr_ready;

/*----------------------
 | gte_matrix_stack
 | Description: The libgte PushMatrix/PopMatrix stack.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static GteMatrixFrame gte_matrix_stack[GTE_MATRIX_STACK_DEPTH];

/*----------------------
 | gte_matrix_depth
 | Description: Number of entries currently on gte_matrix_stack.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int gte_matrix_depth;

/*----------------------
 | gte_set_flag
 | Description: Raises one FLAG bit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: bit -- FLAG bit number
 | Returns: N/A
 ----------------------*/
static void gte_set_flag(int bit)
{
    lom_gte.flag |= 1u << bit;
}

/*----------------------
 | gte_begin
 | Description: Clears FLAG at the start of a command.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void gte_begin(void)
{
    lom_gte.flag = 0;
}

/*----------------------
 | gte_finish
 | Description: Sets FLAG bit 31 when any of bits 30-23 or 18-13 is set.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void gte_finish(void)
{
    if (lom_gte.flag & GTE_FLAG_ERROR_MASK)
    {
        lom_gte.flag |= GTE_FLAG_ERROR;
    }
}

/*----------------------
 | gte_mac44
 | Description: Checks one MAC1-MAC3 accumulation step against the 44-bit range, flags overflow and wraps to 44 bits.
 | Author: suinevere
 | Dependencies: gte_set_flag
 | Globals: lom_gte
 | Params: index -- 1..3, value -- accumulated sum
 | Returns: the sum sign-extended from 44 bits
 ----------------------*/
static int64_t gte_mac44(int index, int64_t value)
{
    int64_t wrapped;

    if (value > GTE_MAC44_MAX)
    {
        gte_set_flag(31 - index);
    }
    else if (value < GTE_MAC44_MIN)
    {
        gte_set_flag(28 - index);
    }
    wrapped = value & 0xFFFFFFFFFFFLL;
    if (wrapped & 0x80000000000LL)
    {
        wrapped -= 0x100000000000LL;
    }
    return wrapped;
}

/*----------------------
 | gte_mac0
 | Description: Flags a MAC0 result outside the signed 32-bit range and stores its low 32 bits in MAC0.
 | Author: suinevere
 | Dependencies: gte_set_flag
 | Globals: lom_gte
 | Params: value -- full MAC0 sum
 | Returns: the full, untruncated sum
 ----------------------*/
static int64_t gte_mac0(int64_t value)
{
    if (value > 0x7FFFFFFFLL)
    {
        gte_set_flag(GTE_FLAG_MAC0_POS);
    }
    else if (value < -0x80000000LL)
    {
        gte_set_flag(GTE_FLAG_MAC0_NEG);
    }
    lom_gte.mac[0] = (int32_t)(uint32_t)value;
    return value;
}

/*----------------------
 | gte_clamp
 | Description: Saturates a value to a range and raises a FLAG bit when it was out of range.
 | Author: suinevere
 | Dependencies: gte_set_flag
 | Globals: lom_gte
 | Params: value -- input, lo/hi -- inclusive range, bit -- FLAG bit
 | Returns: the saturated value
 ----------------------*/
static int32_t gte_clamp(int64_t value, int32_t lo, int32_t hi, int bit)
{
    if (value < lo)
    {
        gte_set_flag(bit);
        return lo;
    }
    if (value > hi)
    {
        gte_set_flag(bit);
        return hi;
    }
    return (int32_t)value;
}

/*----------------------
 | gte_set_ir
 | Description: Saturates a shifted MAC value into IR1-IR3, flagging from a separately supplied value for the RTP IR3 quirk.
 | Author: suinevere
 | Dependencies: gte_set_flag
 | Globals: lom_gte
 | Params: index -- 1..3, value -- shifted MAC, flag_value -- value checked for the flag, lm -- clamp at 0
 | Returns: N/A
 ----------------------*/
static void gte_set_ir(int index, int64_t value, int64_t flag_value, int lm)
{
    int32_t lo = lm ? 0 : -0x8000;

    if (flag_value < lo || flag_value > 0x7FFF)
    {
        gte_set_flag(25 - index);
    }
    if (value < lo)
    {
        value = lo;
    }
    else if (value > 0x7FFF)
    {
        value = 0x7FFF;
    }
    lom_gte.ir[index] = (int16_t)value;
}

/*----------------------
 | gte_matrix_vector
 | Description: Computes MAC1-MAC3 = (add*0x1000 + matrix*vector) >> (sf*12) with per-step 44-bit checks and saturates IR1-IR3.
 | Author: suinevere
 | Dependencies: gte_mac44, gte_set_ir
 | Globals: lom_gte
 | Params: matrix -- 3x3 matrix, vector -- 3 inputs, add -- translation or NULL, sf -- shift flag, lm -- clamp at 0, rtp -- flag IR3 from MAC3 SAR 12
 | Returns: the unshifted MAC3 sum
 ----------------------*/
static int64_t gte_matrix_vector(const int16_t matrix[3][3], const int16_t vector[3], const int32_t *add, int sf, int lm, int rtp)
{
    int64_t sums[3];
    int i;
    int j;

    for (i = 0; i < 3; i++)
    {
        sums[i] = add != NULL ? (int64_t)add[i] * 0x1000 : 0;
        for (j = 0; j < 3; j++)
        {
            sums[i] = gte_mac44(i + 1, sums[i] + (int64_t)matrix[i][j] * vector[j]);
        }
    }
    for (i = 0; i < 3; i++)
    {
        int64_t shifted = sums[i] >> (sf * 12);

        lom_gte.mac[i + 1] = (int32_t)(uint32_t)shifted;
        gte_set_ir(i + 1, shifted, (rtp && i == 2) ? sums[i] >> 12 : shifted, lm);
    }
    return sums[2];
}

/*----------------------
 | gte_saturn_unr_entry
 | Description: Returns one UNR table entry, max(0, (0x40000/(i+0x100)+1)/2 - 0x101), building the table on first use.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: gte_unr_table, gte_unr_ready
 | Params: index -- 0..0x100
 | Returns: the table entry
 ----------------------*/
int gte_saturn_unr_entry(int index)
{
    if (!gte_unr_ready)
    {
        int i;

        for (i = 0; i <= 0x100; i++)
        {
            int value = (0x40000 / (i + 0x100) + 1) / 2 - 0x101;

            gte_unr_table[i] = (uint8_t)(value > 0 ? value : 0);
        }
        gte_unr_ready = 1;
    }
    return gte_unr_table[index];
}

/*----------------------
 | gte_divide
 | Description: The GTE's UNR Newton-Raphson division H*0x20000/SZ3 rounded, or 0x1FFFF with FLAG bit 17 when H >= 2*SZ3.
 | Author: suinevere
 | Dependencies: gte_saturn_unr_entry, gte_set_flag
 | Globals: lom_gte
 | Params: h -- projection distance, sz -- SZ3
 | Returns: the 17-bit quotient
 ----------------------*/
static uint32_t gte_divide(uint32_t h, uint32_t sz)
{
    uint32_t z = 0;
    uint32_t n;
    uint32_t d;
    int64_t u;
    int64_t step;
    uint64_t quotient;

    if (h >= sz * 2)
    {
        gte_set_flag(GTE_FLAG_DIVIDE);
        return 0x1FFFF;
    }
    while (((sz << z) & 0x8000) == 0)
    {
        z++;
    }
    n = h << z;
    d = sz << z;
    u = gte_saturn_unr_entry((int)((d - 0x7FC0) >> 7)) + 0x101;
    step = ((int64_t)0x2000080 - (int64_t)d * u) >> 8;
    step = ((int64_t)0x80 + step * u) >> 8;
    quotient = ((uint64_t)n * (uint64_t)step + 0x8000) >> 16;
    return quotient > 0x1FFFF ? 0x1FFFF : (uint32_t)quotient;
}

/*----------------------
 | gte_rtp
 | Description: One RTPS step: transform a V register by RT/TR, push SZ and the projected SXY, and optionally update MAC0/IR0 depth cueing.
 | Author: suinevere
 | Dependencies: gte_matrix_vector, gte_clamp, gte_divide, gte_mac0
 | Globals: lom_gte
 | Params: slot -- V0..V2, sf -- shift flag, lm -- clamp at 0, depth_cue -- compute IR0
 | Returns: N/A
 ----------------------*/
static void gte_rtp(int slot, int sf, int lm, int depth_cue)
{
    int64_t z;
    uint32_t n;
    int64_t x;
    int64_t y;

    z = gte_matrix_vector(lom_gte.rt, lom_gte.v[slot], lom_gte.tr, sf, lm, 1);
    lom_gte.sz[0] = lom_gte.sz[1];
    lom_gte.sz[1] = lom_gte.sz[2];
    lom_gte.sz[2] = lom_gte.sz[3];
    lom_gte.sz[3] = (uint16_t)gte_clamp(z >> 12, 0, 0xFFFF, GTE_FLAG_SZ_OTZ);
    n = gte_divide(lom_gte.h, lom_gte.sz[3]);
    x = gte_mac0((int64_t)n * lom_gte.ir[1] + lom_gte.ofx);
    lom_gte.sxy[0][0] = lom_gte.sxy[1][0];
    lom_gte.sxy[0][1] = lom_gte.sxy[1][1];
    lom_gte.sxy[1][0] = lom_gte.sxy[2][0];
    lom_gte.sxy[1][1] = lom_gte.sxy[2][1];
    lom_gte.sxy[2][0] = (int16_t)gte_clamp(x >> 16, -0x400, 0x3FF, GTE_FLAG_SX);
    y = gte_mac0((int64_t)n * lom_gte.ir[2] + lom_gte.ofy);
    lom_gte.sxy[2][1] = (int16_t)gte_clamp(y >> 16, -0x400, 0x3FF, GTE_FLAG_SY);
    if (depth_cue)
    {
        int64_t cue = gte_mac0((int64_t)n * lom_gte.dqa + lom_gte.dqb);

        lom_gte.ir[0] = (int16_t)gte_clamp(cue >> 12, 0, 0x1000, GTE_FLAG_IR0);
    }
}

/*----------------------
 | gte_saturn_reset
 | Description: Zeroes every GTE register and empties the matrix stack.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte, gte_matrix_depth
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void gte_saturn_reset(void)
{
    memset(&lom_gte, 0, sizeof(lom_gte));
    gte_matrix_depth = 0;
}

/*----------------------
 | gte_saturn_ldv
 | Description: Loads VXn, VYn, VZn from three consecutive shorts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: slot -- 0..2, vector -- SVECTOR address
 | Returns: N/A
 ----------------------*/
void gte_saturn_ldv(int slot, const void *vector)
{
    memcpy(lom_gte.v[slot], vector, sizeof(lom_gte.v[slot]));
}

/*----------------------
 | gte_saturn_ldlvl
 | Description: Loads IR1-IR3 from the low 16 bits of three 32-bit words.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: vector -- VECTOR address
 | Returns: N/A
 ----------------------*/
void gte_saturn_ldlvl(const void *vector)
{
    int32_t words[3];
    int i;

    memcpy(words, vector, sizeof(words));
    for (i = 0; i < 3; i++)
    {
        lom_gte.ir[i + 1] = (int16_t)(uint16_t)((uint32_t)words[i] & 0xFFFFu);
    }
}

/*----------------------
 | gte_saturn_ldsxy3
 | Description: Loads SXY0-SXY2 from packed words whose bytes are an in-memory x,y short pair, matching words read back from gte_stsxy output.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: sxy0, sxy1, sxy2 -- packed screen words
 | Returns: N/A
 ----------------------*/
void gte_saturn_ldsxy3(uint32_t sxy0, uint32_t sxy1, uint32_t sxy2)
{
    memcpy(lom_gte.sxy[0], &sxy0, sizeof(sxy0));
    memcpy(lom_gte.sxy[1], &sxy1, sizeof(sxy1));
    memcpy(lom_gte.sxy[2], &sxy2, sizeof(sxy2));
}

/*----------------------
 | gte_saturn_set_matrix
 | Description: Loads the rotation, light or colour matrix from the first nine shorts of a MATRIX.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: mx -- GTE_MX_RT, GTE_MX_LLM or GTE_MX_LCM, matrix -- MATRIX address
 | Returns: N/A
 ----------------------*/
void gte_saturn_set_matrix(int mx, const void *matrix)
{
    int16_t (*target)[3] = mx == GTE_MX_RT ? lom_gte.rt : mx == GTE_MX_LLM ? lom_gte.llm : lom_gte.lcm;

    memcpy(target, matrix, sizeof(lom_gte.rt));
}

/*----------------------
 | gte_saturn_set_trans
 | Description: Loads TRX, TRY, TRZ from the 32-bit t[] words at byte offset 20 of a MATRIX.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: matrix -- MATRIX address
 | Returns: N/A
 ----------------------*/
void gte_saturn_set_trans(const void *matrix)
{
    memcpy(lom_gte.tr, (const uint8_t *)matrix + 20, sizeof(lom_gte.tr));
}

/*----------------------
 | gte_saturn_set_back_color
 | Description: Loads RBK, GBK, BBK as each component shifted left by four.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: r, g, b -- colour components
 | Returns: N/A
 ----------------------*/
void gte_saturn_set_back_color(int32_t r, int32_t g, int32_t b)
{
    lom_gte.bk[0] = (int32_t)((uint32_t)r << 4);
    lom_gte.bk[1] = (int32_t)((uint32_t)g << 4);
    lom_gte.bk[2] = (int32_t)((uint32_t)b << 4);
}

/*----------------------
 | gte_saturn_rtps
 | Description: RTPS (sf=1, lm=0): perspective-transforms V0 with depth cueing.
 | Author: suinevere
 | Dependencies: gte_rtp
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void gte_saturn_rtps(void)
{
    gte_begin();
    gte_rtp(0, 1, 0, 1);
    gte_finish();
}

/*----------------------
 | gte_saturn_rtpt
 | Description: RTPT (sf=1, lm=0): perspective-transforms V0, V1, V2 in order, depth cueing only the last.
 | Author: suinevere
 | Dependencies: gte_rtp
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void gte_saturn_rtpt(void)
{
    gte_begin();
    gte_rtp(0, 1, 0, 0);
    gte_rtp(1, 1, 0, 0);
    gte_rtp(2, 1, 0, 1);
    gte_finish();
}

/*----------------------
 | gte_saturn_mvmva
 | Description: MVMVA: multiplies V0-V2 or IR by RT, LLM or LCM, adds TR, BK or nothing, and saturates into IR.
 | Author: suinevere
 | Dependencies: gte_matrix_vector
 | Globals: lom_gte
 | Params: sf -- shift flag, mx -- matrix, v -- vector (3 = IR), cv -- translation, lm -- clamp at 0
 | Returns: N/A
 ----------------------*/
void gte_saturn_mvmva(int sf, int mx, int v, int cv, int lm)
{
    const int16_t (*matrix)[3] = mx == GTE_MX_RT ? lom_gte.rt : mx == GTE_MX_LLM ? lom_gte.llm : lom_gte.lcm;
    const int32_t *add = cv == GTE_CV_TR ? lom_gte.tr : cv == GTE_CV_BK ? lom_gte.bk : NULL;
    int16_t vector[3];

    if (v == GTE_V_IR)
    {
        memcpy(vector, &lom_gte.ir[1], sizeof(vector));
    }
    else
    {
        memcpy(vector, lom_gte.v[v], sizeof(vector));
    }
    gte_begin();
    gte_matrix_vector(matrix, vector, add, sf, lm, 0);
    gte_finish();
}

/*----------------------
 | gte_saturn_nclip
 | Description: NCLIP: MAC0 = SX0*SY1 + SX1*SY2 + SX2*SY0 - SX0*SY2 - SX1*SY0 - SX2*SY1.
 | Author: suinevere
 | Dependencies: gte_mac0
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void gte_saturn_nclip(void)
{
    const int16_t (*s)[2] = lom_gte.sxy;
    int64_t sum;

    gte_begin();
    sum = (int64_t)s[0][0] * s[1][1] + (int64_t)s[1][0] * s[2][1] + (int64_t)s[2][0] * s[0][1]
        - (int64_t)s[0][0] * s[2][1] - (int64_t)s[1][0] * s[0][1] - (int64_t)s[2][0] * s[1][1];
    gte_mac0(sum);
    gte_finish();
}

/*----------------------
 | gte_push_color
 | Description: Shifts the colour FIFO and pushes MAC1-MAC3 divided by 16, saturated to 0..255, with the RGBC code byte.
 | Author: suinevere
 | Dependencies: gte_clamp
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void gte_push_color(void)
{
    int i;

    memcpy(lom_gte.rgb[0], lom_gte.rgb[1], sizeof(lom_gte.rgb[0]));
    memcpy(lom_gte.rgb[1], lom_gte.rgb[2], sizeof(lom_gte.rgb[1]));
    for (i = 0; i < 3; i++)
    {
        lom_gte.rgb[2][i] = (uint8_t)gte_clamp(lom_gte.mac[i + 1] >> 4, 0, 0xFF, GTE_FLAG_COLOR_R - i);
    }
    lom_gte.rgb[2][3] = lom_gte.rgbc[3];
}

/*----------------------
 | gte_saturn_ncs
 | Description: NCS (sf=1, lm=1): IR = LLM*V0, then IR = BK + LCM*IR, then pushes MAC/16 with the RGBC code onto the colour FIFO.
 | Author: suinevere
 | Dependencies: gte_matrix_vector, gte_clamp
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void gte_saturn_ncs(void)
{
    int16_t lit[3];

    gte_begin();
    gte_matrix_vector(lom_gte.llm, lom_gte.v[0], NULL, 1, 1, 0);
    memcpy(lit, &lom_gte.ir[1], sizeof(lit));
    gte_matrix_vector(lom_gte.lcm, lit, lom_gte.bk, 1, 1, 0);
    gte_push_color();
    gte_finish();
}

/*----------------------
 | gte_saturn_sqr
 | Description: SQR (lm=1): MAC1-MAC3 = IR1-IR3 squared, shifted right by sf*12, saturated into IR.
 | Author: suinevere
 | Dependencies: gte_mac44, gte_set_ir
 | Globals: lom_gte
 | Params: sf -- shift flag
 | Returns: N/A
 ----------------------*/
void gte_saturn_sqr(int sf)
{
    int i;

    gte_begin();
    for (i = 1; i <= 3; i++)
    {
        int64_t shifted = gte_mac44(i, (int64_t)lom_gte.ir[i] * lom_gte.ir[i]) >> (sf * 12);

        lom_gte.mac[i] = (int32_t)(uint32_t)shifted;
        gte_set_ir(i, shifted, shifted, 1);
    }
    gte_finish();
}

/*----------------------
 | gte_saturn_gpf
 | Description: GPF: MAC1-MAC3 = IR0*IR1-IR3 shifted right by sf*12, saturated into IR, and pushed onto the colour FIFO.
 | Author: suinevere
 | Dependencies: gte_mac44, gte_set_ir, gte_push_color
 | Globals: lom_gte
 | Params: sf -- shift flag, lm -- clamp at 0
 | Returns: N/A
 ----------------------*/
void gte_saturn_gpf(int sf, int lm)
{
    int i;

    gte_begin();
    for (i = 1; i <= 3; i++)
    {
        int64_t shifted = gte_mac44(i, (int64_t)lom_gte.ir[0] * lom_gte.ir[i]) >> (sf * 12);

        lom_gte.mac[i] = (int32_t)(uint32_t)shifted;
        gte_set_ir(i, shifted, shifted, lm);
    }
    gte_push_color();
    gte_finish();
}

/*----------------------
 | gte_lzc
 | Description: Writes LZCS and sets LZCR to the count of leading bits equal to its sign bit, 32 for 0 and -1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: value -- LZCS input
 | Returns: LZCR
 ----------------------*/
static int gte_lzc(int32_t value)
{
    uint32_t bits = (uint32_t)value;
    uint32_t sign = bits >> 31;
    int count = 0;

    while (count < 32 && ((bits >> (31 - count)) & 1u) == sign)
    {
        count++;
    }
    lom_gte.lzcs = value;
    lom_gte.lzcr = count;
    return count;
}

/*----------------------
 | gte_saturn_stsxy
 | Description: Stores one SXY FIFO entry as x then y shorts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: index -- 0..2, out -- destination
 | Returns: N/A
 ----------------------*/
void gte_saturn_stsxy(int index, void *out)
{
    memcpy(out, lom_gte.sxy[index], sizeof(lom_gte.sxy[index]));
}

/*----------------------
 | gte_saturn_stflg
 | Description: Stores FLAG as a 32-bit word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: out -- destination
 | Returns: N/A
 ----------------------*/
void gte_saturn_stflg(void *out)
{
    memcpy(out, &lom_gte.flag, sizeof(lom_gte.flag));
}

/*----------------------
 | gte_saturn_stszotz
 | Description: Stores SZ3 shifted right by two as a 32-bit word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: out -- destination
 | Returns: N/A
 ----------------------*/
void gte_saturn_stszotz(void *out)
{
    int32_t value = (int32_t)lom_gte.sz[3] >> 2;

    memcpy(out, &value, sizeof(value));
}

/*----------------------
 | gte_saturn_stopz
 | Description: Stores MAC0 as a 32-bit word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: out -- destination
 | Returns: N/A
 ----------------------*/
void gte_saturn_stopz(void *out)
{
    memcpy(out, &lom_gte.mac[0], sizeof(lom_gte.mac[0]));
}

/*----------------------
 | gte_saturn_stlvnl
 | Description: Stores MAC1-MAC3 as three 32-bit words.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: out -- destination
 | Returns: N/A
 ----------------------*/
void gte_saturn_stlvnl(void *out)
{
    memcpy(out, &lom_gte.mac[1], 3 * sizeof(lom_gte.mac[1]));
}

/*----------------------
 | gte_saturn_stsv
 | Description: Stores IR1-IR3 as three shorts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: out -- destination
 | Returns: N/A
 ----------------------*/
void gte_saturn_stsv(void *out)
{
    memcpy(out, &lom_gte.ir[1], 3 * sizeof(lom_gte.ir[1]));
}

/*----------------------
 | gte_saturn_strgb
 | Description: Stores the newest colour FIFO entry as r, g, b, code bytes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: out -- destination
 | Returns: N/A
 ----------------------*/
void gte_saturn_strgb(void *out)
{
    memcpy(out, lom_gte.rgb[2], sizeof(lom_gte.rgb[2]));
}

/*----------------------
 | SetGeomOffset
 | Description: libgte: sets OFX and OFY to the screen offset in 16.16 fixed point.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: ofx, ofy -- screen offset in pixels
 | Returns: N/A
 ----------------------*/
void SetGeomOffset(long ofx, long ofy)
{
    lom_gte.ofx = (int32_t)((uint32_t)ofx << 16);
    lom_gte.ofy = (int32_t)((uint32_t)ofy << 16);
}

/*----------------------
 | SetGeomScreen
 | Description: libgte: sets H, the projection plane distance.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: h -- projection distance
 | Returns: N/A
 ----------------------*/
void SetGeomScreen(long h)
{
    lom_gte.h = (uint16_t)h;
}

/*----------------------
 | SetRotMatrix
 | Description: libgte: loads the rotation matrix.
 | Author: suinevere
 | Dependencies: gte_saturn_set_matrix
 | Globals: lom_gte
 | Params: m -- matrix
 | Returns: N/A
 ----------------------*/
void SetRotMatrix(MATRIX *m)
{
    gte_saturn_set_matrix(GTE_MX_RT, m);
}

/*----------------------
 | SetTransMatrix
 | Description: libgte: loads the translation vector.
 | Author: suinevere
 | Dependencies: gte_saturn_set_trans
 | Globals: lom_gte
 | Params: m -- matrix whose t[] is used
 | Returns: N/A
 ----------------------*/
void SetTransMatrix(MATRIX *m)
{
    gte_saturn_set_trans(m);
}

/*----------------------
 | PushMatrix
 | Description: libgte: saves the rotation matrix and translation vector; a push beyond 20 entries is dropped.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte, gte_matrix_stack, gte_matrix_depth
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void PushMatrix(void)
{
    if (gte_matrix_depth < GTE_MATRIX_STACK_DEPTH)
    {
        memcpy(gte_matrix_stack[gte_matrix_depth].rt, lom_gte.rt, sizeof(lom_gte.rt));
        memcpy(gte_matrix_stack[gte_matrix_depth].tr, lom_gte.tr, sizeof(lom_gte.tr));
        gte_matrix_depth++;
    }
}

/*----------------------
 | PopMatrix
 | Description: libgte: restores the most recently pushed rotation matrix and translation vector; popping an empty stack does nothing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte, gte_matrix_stack, gte_matrix_depth
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void PopMatrix(void)
{
    if (gte_matrix_depth > 0)
    {
        gte_matrix_depth--;
        memcpy(lom_gte.rt, gte_matrix_stack[gte_matrix_depth].rt, sizeof(lom_gte.rt));
        memcpy(lom_gte.tr, gte_matrix_stack[gte_matrix_depth].tr, sizeof(lom_gte.tr));
    }
}

/*----------------------
 | NormalClip
 | Description: libgte: loads three packed screen words into SXY0-SXY2 and runs NCLIP.
 | Author: suinevere
 | Dependencies: gte_saturn_ldsxy3, gte_saturn_nclip
 | Globals: lom_gte
 | Params: sxy0, sxy1, sxy2 -- packed screen words
 | Returns: MAC0
 ----------------------*/
long NormalClip(long sxy0, long sxy1, long sxy2)
{
    gte_saturn_ldsxy3((uint32_t)sxy0, (uint32_t)sxy1, (uint32_t)sxy2);
    gte_saturn_nclip();
    return lom_gte.mac[0];
}

/*----------------------
 | TransMatrix
 | Description: libgte: copies a vector into a matrix's translation.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: m -- matrix, v -- translation
 | Returns: m
 ----------------------*/
MATRIX *TransMatrix(MATRIX *m, VECTOR *v)
{
    m->t[0] = v->vx;
    m->t[1] = v->vy;
    m->t[2] = v->vz;
    return m;
}

/*----------------------
 | InitGeom
 | Description: libgte: sets the GTE defaults the PS1 library loads (ZSF3 341, ZSF4 256, H 1000, DQA -4194, DQB 0x1400000, zero offset); the BIOS patch and COP2 enable are PS1-only.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_gte
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void InitGeom(void)
{
    lom_gte.zsf3 = 341;
    lom_gte.zsf4 = 256;
    lom_gte.h = 1000;
    lom_gte.dqa = -4194;
    lom_gte.dqb = 0x1400000;
    lom_gte.ofx = 0;
    lom_gte.ofy = 0;
}

/*----------------------
 | gte_table_s16
 | Description: Reads one little-endian signed halfword of the table blob.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_libgte_tables
 | Params: offset -- byte offset into the blob
 | Returns: the sign-extended halfword
 ----------------------*/
static int32_t gte_table_s16(int32_t offset)
{
    const uint8_t *p = lom_libgte_tables + offset;

    return (int16_t)(uint16_t)(p[0] | (p[1] << 8));
}

/*----------------------
 | gte_table_u32
 | Description: Reads one little-endian word of the table blob.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_libgte_tables
 | Params: offset -- byte offset into the blob
 | Returns: the word
 ----------------------*/
static uint32_t gte_table_u32(int32_t offset)
{
    const uint8_t *p = lom_libgte_tables + offset;

    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/*----------------------
 | gte_mul
 | Description: The low 32 bits of a product, as MIPS mult/multu leave in LO.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: a, b -- factors
 | Returns: the wrapped product
 ----------------------*/
static int32_t gte_mul(int32_t a, int32_t b)
{
    return (int32_t)((uint32_t)a * (uint32_t)b);
}

/*----------------------
 | gte_fx
 | Description: A 4.12 fixed-point product: the wrapped 32-bit product shifted right by 12.
 | Author: suinevere
 | Dependencies: gte_mul
 | Globals: N/A
 | Params: a, b -- factors
 | Returns: the product
 ----------------------*/
static int32_t gte_fx(int32_t a, int32_t b)
{
    return gte_mul(a, b) >> GTE_FIXED_SHIFT;
}

/*----------------------
 | gte_neg_fx
 | Description: The negated product shifted right by 12, negating before the shift as libgte does.
 | Author: suinevere
 | Dependencies: gte_mul
 | Globals: N/A
 | Params: a, b -- factors
 | Returns: (-(a*b)) >> 12
 ----------------------*/
static int32_t gte_neg_fx(int32_t a, int32_t b)
{
    return (int32_t)(0u - (uint32_t)gte_mul(a, b)) >> GTE_FIXED_SHIFT;
}

/*----------------------
 | gte_abs_angle
 | Description: The magnitude of an angle reduced modulo 4096, with -0x80000000 reducing to 0 as on the PS1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: angle -- libgte angle
 | Returns: 0..4095
 ----------------------*/
static uint32_t gte_abs_angle(int32_t angle)
{
    return (angle < 0 ? 0u - (uint32_t)angle : (uint32_t)angle) & GTE_ANGLE_MASK;
}

/*----------------------
 | gte_rsin_entry
 | Description: One entry of the 1025-entry quarter-wave sine table.
 | Author: suinevere
 | Dependencies: gte_table_s16
 | Globals: lom_libgte_tables
 | Params: index -- 0..1024
 | Returns: sine in 4.12 fixed point
 ----------------------*/
static int32_t gte_rsin_entry(uint32_t index)
{
    return gte_table_s16(LOM_LIBGTE_RSIN_OFFSET + 2 * (int32_t)index);
}

/*----------------------
 | gte_sin_1
 | Description: libgte sin_1: the sine of a 0..4095 angle folded onto the quarter-wave table.
 | Author: suinevere
 | Dependencies: gte_rsin_entry
 | Globals: lom_libgte_tables
 | Params: angle -- 0..4095
 | Returns: sine in 4.12 fixed point
 ----------------------*/
static int32_t gte_sin_1(uint32_t angle)
{
    if (angle <= GTE_HALF_TURN)
    {
        return angle <= GTE_QUARTER_TURN ? gte_rsin_entry(angle) : gte_rsin_entry(GTE_HALF_TURN - angle);
    }
    return angle <= GTE_THREE_QUARTER_TURN ? -gte_rsin_entry(angle - GTE_HALF_TURN) : -gte_rsin_entry(GTE_FULL_TURN - angle);
}

/*----------------------
 | rsin
 | Description: libgte: table sine of an angle where 4096 is a full turn.
 | Author: suinevere
 | Dependencies: gte_sin_1
 | Globals: lom_libgte_tables
 | Params: angle -- libgte angle
 | Returns: sine in 4.12 fixed point
 ----------------------*/
int rsin(int angle)
{
    if (angle >= 0)
    {
        return gte_sin_1((uint32_t)angle & GTE_ANGLE_MASK);
    }
    return -gte_sin_1((0u - (uint32_t)angle) & GTE_ANGLE_MASK);
}

/*----------------------
 | rcos
 | Description: libgte: table cosine of an angle where 4096 is a full turn.
 | Author: suinevere
 | Dependencies: gte_abs_angle, gte_rsin_entry
 | Globals: lom_libgte_tables
 | Params: angle -- libgte angle
 | Returns: cosine in 4.12 fixed point
 ----------------------*/
int rcos(int angle)
{
    uint32_t a = gte_abs_angle(angle);

    if (a <= GTE_HALF_TURN)
    {
        return a <= GTE_QUARTER_TURN ? gte_rsin_entry(GTE_QUARTER_TURN - a) : -gte_rsin_entry(a - GTE_QUARTER_TURN);
    }
    return a <= GTE_THREE_QUARTER_TURN ? -gte_rsin_entry(GTE_THREE_QUARTER_TURN - a) : gte_rsin_entry(a - GTE_THREE_QUARTER_TURN);
}

/*----------------------
 | gte_csincos
 | Description: libgte csincos: six CORDIC steps from the gain constant, then a first-order correction by the residual angle.
 | Author: suinevere
 | Dependencies: gte_table_u32, gte_fx
 | Globals: lom_libgte_tables
 | Params: angle -- 0..1023, sine/cosine -- outputs
 | Returns: N/A
 ----------------------*/
static void gte_csincos(int32_t angle, int32_t *sine, int32_t *cosine)
{
    int32_t x = GTE_CORDIC_GAIN;
    int32_t y = 0;
    int32_t z = angle;
    int i;

    for (i = 0; i < GTE_CORDIC_STEPS; i++)
    {
        int32_t dx = y >> i;
        int32_t dy = x >> i;
        int32_t step = (int32_t)gte_table_u32(LOM_LIBGTE_CORDIC_OFFSET + 4 * i);

        if (z >= 0)
        {
            x -= dx;
            y += dy;
            z -= step;
        }
        else
        {
            x += dx;
            y -= dy;
            z += step;
        }
    }
    *cosine = x - gte_fx(z, y);
    *sine = y + gte_fx(z, x);
}

/*----------------------
 | gte_cordic_fold
 | Description: Folds a 0..4095 angle onto csincos's 0..1023 range, reflecting about 2047 and 4095 (one unit short of the true mirror) as libgte does.
 | Author: suinevere
 | Dependencies: gte_csincos
 | Globals: lom_libgte_tables
 | Params: angle -- 0..4095, sine/cosine -- outputs of the folded angle
 | Returns: the quadrant, 0..3
 ----------------------*/
static int gte_cordic_fold(int32_t angle, int32_t *sine, int32_t *cosine)
{
    int quadrant = angle / GTE_QUARTER_TURN;
    int32_t folded = quadrant == 0 ? angle
                   : quadrant == 1 ? GTE_HALF_TURN - 1 - angle
                   : quadrant == 2 ? angle - GTE_HALF_TURN
                   : GTE_FULL_TURN - 1 - angle;

    gte_csincos(folded, sine, cosine);
    return quadrant;
}

/*----------------------
 | ccos
 | Description: libgte: CORDIC cosine of an angle where 4096 is a full turn.
 | Author: suinevere
 | Dependencies: gte_abs_angle, gte_cordic_fold
 | Globals: lom_libgte_tables
 | Params: angle -- libgte angle
 | Returns: cosine in 4.12 fixed point
 ----------------------*/
int ccos(int angle)
{
    int32_t sine;
    int32_t cosine;
    int quadrant = gte_cordic_fold((int32_t)gte_abs_angle(angle), &sine, &cosine);

    return quadrant == 1 || quadrant == 2 ? -cosine : cosine;
}

/*----------------------
 | csin
 | Description: libgte: CORDIC sine of an angle where 4096 is a full turn, odd in the angle.
 | Author: suinevere
 | Dependencies: gte_abs_angle, gte_cordic_fold
 | Globals: lom_libgte_tables
 | Params: angle -- libgte angle
 | Returns: sine in 4.12 fixed point
 ----------------------*/
int csin(int angle)
{
    int32_t sine;
    int32_t cosine;
    int quadrant = gte_cordic_fold((int32_t)gte_abs_angle(angle), &sine, &cosine);

    if (quadrant >= 2)
    {
        sine = -sine;
    }
    return angle < 0 ? -sine : sine;
}

/*----------------------
 | gte_ratan_quotient
 | Description: ratan2's table index: the smaller operand over the larger in 0.10 fixed point, shifting the divisor down for operands above 0x1FFFFF.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: small, large -- the operands, small < large or both equal
 | Returns: 0..1024, or 0 where the PS1 would trap on a zero divisor
 ----------------------*/
static int32_t gte_ratan_quotient(int32_t small, int32_t large)
{
    int32_t num = (small & GTE_RATAN_WIDE) ? small : (int32_t)((uint32_t)small << GTE_RATAN_SHIFT);
    int32_t den = (small & GTE_RATAN_WIDE) ? large >> GTE_RATAN_SHIFT : large;

    return den != 0 ? num / den : 0;
}

/*----------------------
 | ratan2
 | Description: libgte: table arctangent of y/x over all four quadrants, 0 for (0,0). ratan2(0, -0x80000000) returns 1024 where the PS1 divides by zero and hits break.
 | Author: suinevere
 | Dependencies: gte_ratan_quotient, gte_table_s16
 | Globals: lom_libgte_tables
 | Params: y, x -- vector components
 | Returns: angle in -2048..2048, 4096 per turn
 ----------------------*/
long ratan2(long y, long x)
{
    int32_t sy = (int32_t)y;
    int32_t sx = (int32_t)x;
    int negative_x = sx < 0;
    int negative_y = sy < 0;
    int32_t angle;

    if (negative_x)
    {
        sx = (int32_t)(0u - (uint32_t)sx);
    }
    if (negative_y)
    {
        sy = (int32_t)(0u - (uint32_t)sy);
    }
    if (sx == 0 && sy == 0)
    {
        return 0;
    }
    if (sy < sx)
    {
        angle = gte_table_s16(LOM_LIBGTE_RATAN_OFFSET + 2 * gte_ratan_quotient(sy, sx));
    }
    else
    {
        angle = GTE_QUARTER_TURN - gte_table_s16(LOM_LIBGTE_RATAN_OFFSET + 2 * gte_ratan_quotient(sx, sy));
    }
    if (negative_x)
    {
        angle = GTE_HALF_TURN - angle;
    }
    return negative_y ? -angle : angle;
}

/*----------------------
 | gte_root_entry
 | Description: Normalises a value by its LZCR and reads a square-root table entry; negative inputs index below the table, into the sine table, as on the PS1.
 | Author: suinevere
 | Dependencies: gte_table_s16
 | Globals: lom_libgte_tables
 | Params: value -- LZCS input, lzcr -- its leading count, base -- table byte offset
 | Returns: the table entry
 ----------------------*/
static int32_t gte_root_entry(int32_t value, int lzcr, int32_t base)
{
    int32_t even = lzcr & ~1;
    int32_t up = even - GTE_ROOT_NORM;
    int32_t index = up >= 0 ? (int32_t)((uint32_t)value << (up & 31)) : value >> ((GTE_ROOT_NORM - even) & 31);

    return gte_table_s16(base + 2 * (index - GTE_ROOT_BIAS));
}

/*----------------------
 | gte_root_shift
 | Description: The exponent shift libgte applies after a square-root lookup, (31 - (lzcr & ~1)) >> 1, -1 for an LZCR of 32.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: lzcr -- leading count
 | Returns: the shift; MIPS shifts use its low five bits
 ----------------------*/
static int32_t gte_root_shift(int lzcr)
{
    return (31 - (lzcr & ~1)) >> 1;
}

/*----------------------
 | SquareRoot0
 | Description: libgte: table square root of a 32-bit value through the GTE leading-zero counter; 0 and -1 give 0.
 | Author: suinevere
 | Dependencies: gte_lzc, gte_root_entry, gte_root_shift
 | Globals: lom_gte, lom_libgte_tables
 | Params: value -- radicand
 | Returns: the integer square root
 ----------------------*/
long SquareRoot0(long value)
{
    int lzcr = gte_lzc((int32_t)value);
    uint32_t entry;

    if (lzcr == GTE_LZC_ZERO)
    {
        return 0;
    }
    entry = (uint32_t)gte_root_entry((int32_t)value, lzcr, LOM_LIBGTE_SQRT_OFFSET);
    return (long)((entry << (gte_root_shift(lzcr) & 31)) >> GTE_FIXED_SHIFT);
}

/*----------------------
 | gte_vector_normal
 | Description: libgte's shared VectorNormal core: SQR the 16-bit components, inverse-square-root the sum, GPF and shift back.
 | Author: suinevere
 | Dependencies: gte_saturn_sqr, gte_lzc, gte_root_entry, gte_root_shift, gte_saturn_gpf
 | Globals: lom_gte, lom_libgte_tables
 | Params: in -- three components, out -- unit vector in 4.12 fixed point
 | Returns: the sum of the squared 16-bit components, wrapped where the PS1 would trap
 ----------------------*/
static int32_t gte_vector_normal(const int32_t in[3], int32_t out[3])
{
    int32_t sum;
    int32_t shift;
    int lzcr;
    int i;

    for (i = 0; i < 3; i++)
    {
        lom_gte.ir[i + 1] = (int16_t)(uint16_t)(uint32_t)in[i];
    }
    gte_saturn_sqr(0);
    sum = (int32_t)((uint32_t)lom_gte.mac[1] + (uint32_t)lom_gte.mac[2] + (uint32_t)lom_gte.mac[3]);
    lzcr = gte_lzc(sum);
    shift = gte_root_shift(lzcr);
    lom_gte.ir[0] = (int16_t)gte_root_entry(sum, lzcr, LOM_LIBGTE_INV_SQRT_OFFSET);
    for (i = 0; i < 3; i++)
    {
        lom_gte.ir[i + 1] = (int16_t)(uint16_t)(uint32_t)in[i];
    }
    gte_saturn_gpf(0, 0);
    for (i = 0; i < 3; i++)
    {
        out[i] = lom_gte.mac[i + 1] >> (shift & 31);
    }
    return sum;
}

/*----------------------
 | VectorNormal
 | Description: libgte: normalises a vector to 4.12 fixed point; components are taken as 16-bit. A squared sum above 0x7FFFFFFF wraps here where the PS1 add at 0x8001CE2C traps.
 | Author: suinevere
 | Dependencies: gte_vector_normal
 | Globals: lom_gte, lom_libgte_tables
 | Params: v0 -- input, v1 -- output
 | Returns: the sum of squares
 ----------------------*/
long VectorNormal(VECTOR *v0, VECTOR *v1)
{
    int32_t in[3] = { (int32_t)v0->vx, (int32_t)v0->vy, (int32_t)v0->vz };
    int32_t out[3];
    int32_t sum = gte_vector_normal(in, out);

    v1->vx = out[0];
    v1->vy = out[1];
    v1->vz = out[2];
    return sum;
}

/*----------------------
 | VectorNormalS
 | Description: libgte: normalises a vector to a short vector in 4.12 fixed point. Like VectorNormal, an overflowing squared sum wraps where the PS1 traps.
 | Author: suinevere
 | Dependencies: gte_vector_normal
 | Globals: lom_gte, lom_libgte_tables
 | Params: v0 -- input, v1 -- output
 | Returns: the sum of squares
 ----------------------*/
long VectorNormalS(VECTOR *v0, SVECTOR *v1)
{
    int32_t in[3] = { (int32_t)v0->vx, (int32_t)v0->vy, (int32_t)v0->vz };
    int32_t out[3];
    int32_t sum = gte_vector_normal(in, out);

    v1->vx = (short)out[0];
    v1->vy = (short)out[1];
    v1->vz = (short)out[2];
    return sum;
}

/*----------------------
 | gte_store_pad
 | Description: Writes the halfword after m[2][2], which libgte's 32-bit store of m[2][2] fills with the value's high half.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: m -- matrix, pad -- high half
 | Returns: N/A
 ----------------------*/
static void gte_store_pad(MATRIX *m, int16_t pad)
{
    memcpy((uint8_t *)m + GTE_MATRIX_PAD_OFFSET, &pad, sizeof(pad));
}

/*----------------------
 | gte_split_lv
 | Description: Splits a 32-bit component into ApplyMatrixLV's sign-matched high part (bits 15 up) and 15-bit low part.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: value -- component, high/low -- outputs
 | Returns: N/A
 ----------------------*/
static void gte_split_lv(int32_t value, int32_t *high, int32_t *low)
{
    uint32_t magnitude = value >= 0 ? (uint32_t)value : 0u - (uint32_t)value;
    int32_t part_high = (int32_t)magnitude >> GTE_LV_SPLIT;
    int32_t part_low = (int32_t)(magnitude & GTE_LV_LOW_MASK);

    *high = value >= 0 ? part_high : (int32_t)(0u - (uint32_t)part_high);
    *low = value >= 0 ? part_low : -part_low;
}

/*----------------------
 | ApplyMatrixLV
 | Description: libgte: multiplies a 32-bit vector by a matrix on the GTE, as a high part (sf=0, scaled by 8) plus a 15-bit low part (sf=1).
 | Author: suinevere
 | Dependencies: gte_saturn_set_matrix, gte_split_lv, gte_saturn_mvmva
 | Globals: lom_gte
 | Params: m -- matrix (loaded into RT), v0 -- input, v1 -- output
 | Returns: v1
 ----------------------*/
VECTOR *ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1)
{
    int32_t in[3] = { (int32_t)v0->vx, (int32_t)v0->vy, (int32_t)v0->vz };
    int32_t high[3];
    int32_t low[3];
    uint32_t mac_high[3];
    int i;

    gte_saturn_set_matrix(GTE_MX_RT, m);
    for (i = 0; i < 3; i++)
    {
        gte_split_lv(in[i], &high[i], &low[i]);
        lom_gte.ir[i + 1] = (int16_t)(uint16_t)(uint32_t)high[i];
    }
    gte_saturn_mvmva(0, GTE_MX_RT, GTE_V_IR, GTE_CV_NONE, 0);
    for (i = 0; i < 3; i++)
    {
        mac_high[i] = (uint32_t)lom_gte.mac[i + 1];
        lom_gte.ir[i + 1] = (int16_t)low[i];
    }
    gte_saturn_mvmva(1, GTE_MX_RT, GTE_V_IR, GTE_CV_NONE, 0);
    v1->vx = (int32_t)((uint32_t)lom_gte.mac[1] + (mac_high[0] << GTE_LV_HIGH_SCALE));
    v1->vy = (int32_t)((uint32_t)lom_gte.mac[2] + (mac_high[1] << GTE_LV_HIGH_SCALE));
    v1->vz = (int32_t)((uint32_t)lom_gte.mac[3] + (mac_high[2] << GTE_LV_HIGH_SCALE));
    return v1;
}

/*----------------------
 | CompMatrix
 | Description: libgte: m2 = m0*m1 by RT*V0 per column (IR-saturated) and t2 = m0*(16-bit m1.t) + m0.t; m2's pad gets the sign of m2[2][2]. An overflowing t2 wraps here where the PS1 add at 0x8001D0F8 traps.
 | Author: suinevere
 | Dependencies: gte_saturn_set_matrix, gte_saturn_ldv, gte_saturn_mvmva, gte_store_pad
 | Globals: lom_gte
 | Params: m0 -- left matrix (loaded into RT), m1 -- right matrix, m2 -- result, may alias either
 | Returns: m2
 ----------------------*/
MATRIX *CompMatrix(MATRIX *m0, MATRIX *m1, MATRIX *m2)
{
    int16_t column[3][3];
    int16_t trans[3];
    int32_t base[3];
    int16_t result[3][3];
    int r;
    int c;

    for (c = 0; c < 3; c++)
    {
        for (r = 0; r < 3; r++)
        {
            column[c][r] = m1->m[r][c];
        }
        trans[c] = (int16_t)(uint16_t)(uint32_t)m1->t[c];
        base[c] = (int32_t)m0->t[c];
    }
    gte_saturn_set_matrix(GTE_MX_RT, m0);
    for (c = 0; c < 3; c++)
    {
        gte_saturn_ldv(0, column[c]);
        gte_saturn_mvmva(1, GTE_MX_RT, 0, GTE_CV_NONE, 0);
        for (r = 0; r < 3; r++)
        {
            result[r][c] = lom_gte.ir[r + 1];
        }
    }
    gte_saturn_ldv(0, trans);
    gte_saturn_mvmva(1, GTE_MX_RT, 0, GTE_CV_NONE, 0);
    memcpy(m2->m, result, sizeof(result));
    gte_store_pad(m2, result[2][2] < 0 ? -1 : 0);
    for (r = 0; r < 3; r++)
    {
        m2->t[r] = (int32_t)((uint32_t)lom_gte.mac[r + 1] + (uint32_t)base[r]);
    }
    return m2;
}

/*----------------------
 | ScaleMatrix
 | Description: libgte: scales each column of m by the matching component of v in 4.12 fixed point; m's pad gets m[2][2]'s high half.
 | Author: suinevere
 | Dependencies: gte_fx, gte_store_pad
 | Globals: N/A
 | Params: m -- matrix, v -- column scales
 | Returns: m
 ----------------------*/
MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v)
{
    int32_t scale[3] = { (int32_t)v->vx, (int32_t)v->vy, (int32_t)v->vz };
    int32_t value = 0;
    int r;
    int c;

    for (r = 0; r < 3; r++)
    {
        for (c = 0; c < 3; c++)
        {
            value = gte_fx(m->m[r][c], scale[c]);
            m->m[r][c] = (short)value;
        }
    }
    gte_store_pad(m, (int16_t)(value >> 16));
    return m;
}

/*----------------------
 | gte_rcossin
 | Description: Sine and cosine of an angle from the packed 4096-entry rcossin table, the sine negated for negative angles.
 | Author: suinevere
 | Dependencies: gte_table_u32, gte_abs_angle
 | Globals: lom_libgte_tables
 | Params: angle -- libgte angle, sine/cosine -- outputs
 | Returns: N/A
 ----------------------*/
static void gte_rcossin(int32_t angle, int32_t *sine, int32_t *cosine)
{
    uint32_t entry = gte_table_u32(LOM_LIBGTE_RCOSSIN_OFFSET + 4 * (int32_t)gte_abs_angle(angle));
    int32_t s = (int16_t)(uint16_t)(entry & 0xFFFFu);

    *sine = angle < 0 ? -s : s;
    *cosine = (int16_t)(uint16_t)(entry >> 16);
}

/*----------------------
 | RotMatrix
 | Description: libgte: the rotation matrix for X, then Y, then Z angles, computed on the CPU from the rcossin table.
 | Author: suinevere
 | Dependencies: gte_rcossin, gte_fx, gte_neg_fx
 | Globals: lom_libgte_tables
 | Params: r -- angles, m -- output rotation (t and pad untouched)
 | Returns: m
 ----------------------*/
MATRIX *RotMatrix(SVECTOR *r, MATRIX *m)
{
    int32_t sx, cx, sy, cy, sz, cz;
    int32_t czsy;
    int32_t szsy;

    gte_rcossin(r->vx, &sx, &cx);
    gte_rcossin(r->vy, &sy, &cy);
    gte_rcossin(r->vz, &sz, &cz);
    czsy = gte_fx(cz, -sy);
    szsy = gte_fx(sz, -sy);
    m->m[0][0] = (short)gte_fx(cz, cy);
    m->m[0][1] = (short)gte_neg_fx(sz, cy);
    m->m[0][2] = (short)sy;
    m->m[1][0] = (short)(gte_fx(sz, cx) - gte_fx(czsy, sx));
    m->m[1][1] = (short)(gte_fx(cz, cx) + gte_fx(szsy, sx));
    m->m[1][2] = (short)gte_neg_fx(cy, sx);
    m->m[2][0] = (short)(gte_fx(sz, sx) + gte_fx(czsy, cx));
    m->m[2][1] = (short)(gte_fx(cz, sx) - gte_fx(szsy, cx));
    m->m[2][2] = (short)gte_fx(cy, cx);
    return m;
}

/*----------------------
 | gte_rotate_rows
 | Description: Rotates two rows of m: a' = (c*a - k*b) >> 12 and b' = (k*a + c*b) >> 12, each sum wrapped to 32 bits.
 | Author: suinevere
 | Dependencies: gte_mul
 | Globals: N/A
 | Params: m -- matrix, row_a/row_b -- rows, c -- cosine, k -- signed sine
 | Returns: m
 ----------------------*/
static MATRIX *gte_rotate_rows(MATRIX *m, int row_a, int row_b, int32_t c, int32_t k)
{
    int j;

    for (j = 0; j < 3; j++)
    {
        int32_t a = m->m[row_a][j];
        int32_t b = m->m[row_b][j];

        m->m[row_a][j] = (short)((int32_t)((uint32_t)gte_mul(c, a) - (uint32_t)gte_mul(k, b)) >> GTE_FIXED_SHIFT);
        m->m[row_b][j] = (short)((int32_t)((uint32_t)gte_mul(k, a) + (uint32_t)gte_mul(c, b)) >> GTE_FIXED_SHIFT);
    }
    return m;
}

/*----------------------
 | RotMatrixX
 | Description: libgte: premultiplies m by a rotation about X (rows 1 and 2).
 | Author: suinevere
 | Dependencies: gte_rcossin, gte_rotate_rows
 | Globals: lom_libgte_tables
 | Params: r -- angle, m -- matrix
 | Returns: m
 ----------------------*/
MATRIX *RotMatrixX(long r, MATRIX *m)
{
    int32_t s;
    int32_t c;

    gte_rcossin((int32_t)r, &s, &c);
    return gte_rotate_rows(m, 1, 2, c, s);
}

/*----------------------
 | RotMatrixY
 | Description: libgte: premultiplies m by a rotation about Y (rows 0 and 2, sine negated).
 | Author: suinevere
 | Dependencies: gte_rcossin, gte_rotate_rows
 | Globals: lom_libgte_tables
 | Params: r -- angle, m -- matrix
 | Returns: m
 ----------------------*/
MATRIX *RotMatrixY(long r, MATRIX *m)
{
    int32_t s;
    int32_t c;

    gte_rcossin((int32_t)r, &s, &c);
    return gte_rotate_rows(m, 0, 2, c, -s);
}

/*----------------------
 | RotMatrixZ
 | Description: libgte: premultiplies m by a rotation about Z (rows 0 and 1).
 | Author: suinevere
 | Dependencies: gte_rcossin, gte_rotate_rows
 | Globals: lom_libgte_tables
 | Params: r -- angle, m -- matrix
 | Returns: m
 ----------------------*/
MATRIX *RotMatrixZ(long r, MATRIX *m)
{
    int32_t s;
    int32_t c;

    gte_rcossin((int32_t)r, &s, &c);
    return gte_rotate_rows(m, 0, 1, c, s);
}

/*----------------------
 | gte_gpf_ir
 | Description: Loads IR0-IR3 and runs GPF with sf=1, lm=0, as RotMatrix_gte does.
 | Author: suinevere
 | Dependencies: gte_saturn_gpf
 | Globals: lom_gte
 | Params: ir0..ir3 -- register values, taken as 16-bit
 | Returns: N/A
 ----------------------*/
static void gte_gpf_ir(int32_t ir0, int32_t ir1, int32_t ir2, int32_t ir3)
{
    lom_gte.ir[0] = (int16_t)ir0;
    lom_gte.ir[1] = (int16_t)ir1;
    lom_gte.ir[2] = (int16_t)ir2;
    lom_gte.ir[3] = (int16_t)ir3;
    gte_saturn_gpf(1, 0);
}

/*----------------------
 | RotMatrix_gte
 | Description: libgte: RotMatrix with most products on the GTE (four GPFs), which rounds some entries differently from RotMatrix.
 | Author: suinevere
 | Dependencies: gte_rcossin, gte_gpf_ir, gte_fx, gte_neg_fx
 | Globals: lom_gte, lom_libgte_tables
 | Params: r -- angles, m -- output rotation (t and pad untouched)
 | Returns: m
 ----------------------*/
MATRIX *RotMatrix_gte(SVECTOR *r, MATRIX *m)
{
    int32_t sx, cx, sy, cy, sz, cz;
    int32_t cxsy, cxsz, cxcz;
    int32_t sxsy, sxsz, sxcz;
    int32_t czcy, czsxsy, czcxsy;
    int32_t szcy, szsxsy, szcxsy;

    gte_rcossin(r->vx, &sx, &cx);
    gte_rcossin(r->vy, &sy, &cy);
    gte_rcossin(r->vz, &sz, &cz);
    gte_gpf_ir(cx, sy, sz, cz);
    cxsy = lom_gte.ir[1];
    cxsz = lom_gte.ir[2];
    cxcz = lom_gte.ir[3];
    gte_gpf_ir(sx, sy, sz, cz);
    sxsy = lom_gte.ir[1];
    sxsz = lom_gte.ir[2];
    sxcz = lom_gte.ir[3];
    gte_gpf_ir(cz, cy, sxsy, cxsy);
    czcy = lom_gte.ir[1];
    czsxsy = lom_gte.ir[2];
    czcxsy = lom_gte.ir[3];
    gte_gpf_ir(sz, cy, sxsy, cxsy);
    szcy = lom_gte.ir[1];
    szsxsy = lom_gte.ir[2];
    szcxsy = lom_gte.ir[3];
    m->m[0][0] = (short)czcy;
    m->m[0][1] = (short)-szcy;
    m->m[0][2] = (short)sy;
    m->m[1][0] = (short)(czsxsy + cxsz);
    m->m[1][1] = (short)(cxcz - szsxsy);
    m->m[1][2] = (short)-gte_fx(cy, sx);
    m->m[2][0] = (short)(sxsz - czcxsy);
    m->m[2][1] = (short)(sxcz + szcxsy);
    m->m[2][2] = (short)gte_fx(cy, cx);
    return m;
}
