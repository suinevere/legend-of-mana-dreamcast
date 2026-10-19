/*----------------------
 | test_mdec.c
 | Description: Host test for mdec.c: default tables, flat blocks, colour conversion in both depths, and running out of data.
 | Author: suinevere
 | Dependencies: mdec.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mdec.h"

static int s_fail;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); s_fail = 1; } } while (0)

static uint32_t s_cmd[1 + 256];

#define VARIED_HASH_15 0xF3C57E3Eu
#define VARIED_HASH_24 0xA7047607u

/*----------------------
 | put_block
 | Description: Appends a DC-only block (one DC halfword, then EOB) to a run-length halfword list.
 | Author: suinevere
 | Dependencies: MDEC_EOB
 | Globals: N/A
 | Params: hw -- halfword list, n -- halfwords so far, q -- quant scale, dc -- 10-bit DC value
 | Returns: new halfword count
 ----------------------*/
static int put_block(uint16_t *hw, int n, int q, int dc)
{
    hw[n++] = (uint16_t)((q << 10) | (dc & 0x3FF));
    hw[n++] = MDEC_EOB;
    return n;
}

/*----------------------
 | build_flat
 | Description: Builds a one-macroblock MDEC command of flat Cr, Cb and four Y blocks in s_cmd.
 | Author: suinevere
 | Dependencies: put_block
 | Globals: s_cmd
 | Params: y_dc, cb_dc, cr_dc -- DC values of the luma and chroma blocks
 | Returns: N/A
 ----------------------*/
static void build_flat(int y_dc, int cb_dc, int cr_dc)
{
    uint16_t *hw = (uint16_t *)&s_cmd[1];
    int n = 0, i;

    n = put_block(hw, n, 1, cr_dc);
    n = put_block(hw, n, 1, cb_dc);
    for (i = 0; i < 4; i++)
    {
        n = put_block(hw, n, 1, y_dc);
    }
    s_cmd[0] = 0x38000000u | (uint32_t)((n + 1) / 2);
}

/*----------------------
 | test_tables
 | Description: The scale table matches psx-spx's MDEC table and the quant table is the PS1 default in zigzag order.
 | Author: suinevere
 | Dependencies: mdec_scale_table, mdec_quant_table
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_tables(void)
{
    const int16_t *st = mdec_scale_table();
    const uint8_t *qt = mdec_quant_table();

    CHECK(st[0] == 0x5A82);
    CHECK(st[8] == 0x7D8A);
    CHECK(st[9] == 0x6A6D);
    CHECK(st[16] == 0x7641);
    CHECK(st[15] == (int16_t)0x8275);
    CHECK(qt[0] == 2);
    CHECK(qt[1] == 16);
    CHECK(qt[2] == 16);
    CHECK(qt[63] == 83);
}

/*----------------------
 | test_grey_24
 | Description: An all-zero macroblock decodes to mid grey in 24-bit mode.
 | Author: suinevere
 | Dependencies: build_flat, mdec_begin, mdec_decode_macroblock
 | Globals: s_cmd
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_grey_24(void)
{
    MdecStream s;
    uint8_t mb[MDEC_MB_BYTES_24];
    int i;

    build_flat(0, 0, 0);
    mdec_begin(&s, s_cmd, 1, 0);
    CHECK(mdec_decode_macroblock(&s, mb) == 0);
    for (i = 0; i < MDEC_MB_BYTES_24; i++)
    {
        CHECK(mb[i] == 128);
    }
}

/*----------------------
 | test_bright_luma_24
 | Description: A positive luma DC with no chroma gives a uniform grey brighter than mid grey.
 | Author: suinevere
 | Dependencies: build_flat, mdec_begin, mdec_decode_macroblock
 | Globals: s_cmd
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_bright_luma_24(void)
{
    MdecStream s;
    uint8_t mb[MDEC_MB_BYTES_24];

    build_flat(64, 0, 0);
    mdec_begin(&s, s_cmd, 1, 0);
    CHECK(mdec_decode_macroblock(&s, mb) == 0);
    CHECK(mb[0] > 128 && mb[0] == mb[1] && mb[1] == mb[2]);
    CHECK(mb[767] == mb[0]);
}

/*----------------------
 | test_red_15
 | Description: A positive Cr gives a red-leaning 15-bit pixel with the STP bit set when asked.
 | Author: suinevere
 | Dependencies: build_flat, mdec_begin, mdec_decode_macroblock
 | Globals: s_cmd
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_red_15(void)
{
    MdecStream s;
    uint8_t mb[MDEC_MB_BYTES_15];
    uint16_t px;

    build_flat(0, 0, 100);
    mdec_begin(&s, s_cmd, 0, 1);
    CHECK(mdec_decode_macroblock(&s, mb) == 0);
    px = (uint16_t)(mb[0] | (mb[1] << 8));
    CHECK((px & 0x8000) != 0);
    CHECK((px & 0x1F) > ((px >> 10) & 0x1F));
}

/*----------------------
 | test_runs_out
 | Description: A command too short for six blocks returns -1 and a black macroblock.
 | Author: suinevere
 | Dependencies: build_flat, mdec_begin, mdec_decode_macroblock
 | Globals: s_cmd
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_runs_out(void)
{
    MdecStream s;
    uint8_t mb[MDEC_MB_BYTES_24];

    build_flat(0, 0, 0);
    s_cmd[0] = 0x38000000u | 2u;
    mdec_begin(&s, s_cmd, 1, 0);
    CHECK(mdec_decode_macroblock(&s, mb) == -1);
    CHECK(mb[0] == 0 && mb[767] == 0);
}

/*----------------------
 | build_varied
 | Description: Builds a one-macroblock command whose six blocks each hold a random DC and up to ten small random AC codes at random runs, from rand()'s current state; values stay small enough that the IDCT output stays inside the clamp table.
 | Author: suinevere
 | Dependencies: MDEC_EOB
 | Globals: s_cmd
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void build_varied(void)
{
    uint16_t *hw = (uint16_t *)&s_cmd[1];
    int n = 0, b, k, pos;

    for (b = 0; b < 6; b++)
    {
        hw[n++] = (uint16_t)(((1 + rand() % 20) << 10) | ((rand() % 256 - 128) & 0x3FF));
        pos = 0;
        for (k = rand() % 11; k > 0; k--)
        {
            int run = rand() % 6;

            if (pos + run + 1 > 63)
            {
                break;
            }
            pos += run + 1;
            hw[n++] = (uint16_t)((run << 10) | ((rand() % 16 - 8) & 0x3FF));
        }
        hw[n++] = MDEC_EOB;
    }
    if (n & 1)
    {
        hw[n++] = MDEC_EOB;
    }
    s_cmd[0] = 0x38000000u | (uint32_t)(n / 2);
}

/*----------------------
 | test_varied_output_is_unchanged
 | Description: FNV-1a hashes of 200 varied macroblocks in each output depth match the values the per-pixel reference loop produced, pinning the colour conversion exactly.
 | Author: suinevere
 | Dependencies: build_varied, mdec_begin, mdec_decode_macroblock
 | Globals: s_cmd, s_fail
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_varied_output_is_unchanged(void)
{
    uint8_t mb[MDEC_MB_BYTES_24];
    uint32_t hash[2] = {2166136261u, 2166136261u};
    int depth, i, j;

    for (depth = 0; depth < 2; depth++)
    {
        srand(99);
        for (i = 0; i < 200; i++)
        {
            MdecStream s;
            int bytes = depth ? MDEC_MB_BYTES_24 : MDEC_MB_BYTES_15;

            build_varied();
            mdec_begin(&s, s_cmd, depth, i & 1);
            CHECK(mdec_decode_macroblock(&s, mb) == 0);
            for (j = 0; j < bytes; j++)
            {
                hash[depth] = (hash[depth] ^ mb[j]) * 16777619u;
            }
        }
    }
    printf("varied hashes 15 0x%08X 24 0x%08X\n", hash[0], hash[1]);
    CHECK(hash[0] == VARIED_HASH_15);
    CHECK(hash[1] == VARIED_HASH_24);
}

/*----------------------
 | main
 | Description: Runs every mdec test and prints ok when all pass.
 | Author: suinevere
 | Dependencies: mdec_init_tables
 | Globals: s_fail
 | Params: N/A
 | Returns: 0 on success, 1 on any failure
 ----------------------*/
int main(void)
{
    mdec_init_tables();
    test_tables();
    test_grey_24();
    test_bright_luma_24();
    test_red_15();
    test_runs_out();
    test_varied_output_is_unchanged();
    if (!s_fail)
    {
        printf("ok\n");
    }
    return s_fail;
}
