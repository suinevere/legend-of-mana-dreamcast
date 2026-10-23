/*----------------------
 | test_rgb24_display.c
 | Description: Host test for rgb24_display.c: RGB24 bytes packed in VRAM halfwords become RGB565, the source wraps at the VRAM edges, and rows land at the destination pitch.
 | Author: suinevere
 | Dependencies: rgb24_display.h
 | Globals: s_fail, s_vram, s_dst
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "rgb24_display.h"

static int s_fail;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); s_fail = 1; } } while (0)

static uint16_t s_vram[512 * 1024];
static uint16_t s_dst[512 * 256];

/*----------------------
 | test_two_pixels
 | Description: Bytes F8 00 00 | 00 FC 00 (pure red, pure green) convert to 0xF800 and 0x07E0.
 | Author: suinevere
 | Dependencies: rgb24_to_rgb565
 | Globals: s_vram, s_dst
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_two_pixels(void)
{
    memset(s_vram, 0, sizeof(s_vram));
    s_vram[0] = 0x00F8;
    s_vram[1] = 0x0000;
    s_vram[2] = 0x00FC;
    rgb24_to_rgb565(s_vram, 1024, 0, 0, 2, 1, s_dst, 512);
    CHECK(s_dst[0] == 0xF800);
    CHECK(s_dst[1] == 0x07E0);
}

/*----------------------
 | test_blue_and_white
 | Description: Bytes 00 00 F8 | FF FF FF convert to 0x001F and 0xFFFF.
 | Author: suinevere
 | Dependencies: rgb24_to_rgb565
 | Globals: s_vram, s_dst
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_blue_and_white(void)
{
    memset(s_vram, 0, sizeof(s_vram));
    s_vram[0] = 0x0000;
    s_vram[1] = 0xFFF8;
    s_vram[2] = 0xFFFF;
    rgb24_to_rgb565(s_vram, 1024, 0, 0, 2, 1, s_dst, 512);
    CHECK(s_dst[0] == 0x001F);
    CHECK(s_dst[1] == 0xFFFF);
}

/*----------------------
 | test_wraps_at_vram_edges
 | Description: A rect starting at halfword column 976 and row 511 has pixel 32 start at byte 0 of the wrapped row, and its second row is VRAM row 0.
 | Author: suinevere
 | Dependencies: rgb24_to_rgb565
 | Globals: s_vram, s_dst
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_wraps_at_vram_edges(void)
{
    memset(s_vram, 0, sizeof(s_vram));
    s_vram[0] = 0xF8F8;
    s_vram[1] = 0x00F8;
    rgb24_to_rgb565(s_vram, 1024, 976, 511, 320, 2, s_dst, 512);
    CHECK(s_dst[512 + 32] == 0xFFDF);
    CHECK(s_dst[512 + 31] == 0);
}

/*----------------------
 | test_rows_land_at_pitch
 | Description: Row y of the source lands at dst + y * pitch.
 | Author: suinevere
 | Dependencies: rgb24_to_rgb565
 | Globals: s_vram, s_dst
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_rows_land_at_pitch(void)
{
    memset(s_vram, 0, sizeof(s_vram));
    memset(s_dst, 0, sizeof(s_dst));
    s_vram[1024 * 3] = 0x00F8;
    rgb24_to_rgb565(s_vram, 1024, 0, 0, 320, 240, s_dst, 512);
    CHECK(s_dst[3 * 512] == 0xF800);
    CHECK(s_dst[3 * 320] == 0);
}

/*----------------------
 | test_matches_per_byte_formula
 | Description: Noise converted at aligned, odd and wrapping origins and widths not a multiple of four matches the per-byte RGB565 formula everywhere.
 | Author: suinevere
 | Dependencies: rgb24_to_rgb565
 | Globals: s_vram, s_dst
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_matches_per_byte_formula(void)
{
    static const unsigned cases[][4] = {{0, 0, 320, 240}, {2, 5, 318, 7}, {1, 9, 255, 3}, {6, 510, 101, 4}, {980, 0, 30, 2}};
    const uint8_t *bytes = (const uint8_t *)s_vram;
    unsigned seed = 12345u;
    unsigned i, c, r, p;

    for (i = 0; i < sizeof(s_vram) / sizeof(s_vram[0]); i++)
    {
        seed = seed * 1103515245u + 12345u;
        s_vram[i] = (uint16_t)(seed >> 8);
    }
    for (c = 0; c < sizeof(cases) / sizeof(cases[0]); c++)
    {
        unsigned x_hw = cases[c][0], y = cases[c][1], w = cases[c][2], h = cases[c][3];

        memset(s_dst, 0, sizeof(s_dst));
        rgb24_to_rgb565(s_vram, 1024, x_hw, y, w, h, s_dst, 512);
        for (r = 0; r < h; r++)
        {
            const uint8_t *row = bytes + ((y + r) % 512u) * 2048u;

            for (p = 0; p < w; p++)
            {
                unsigned at = x_hw * 2u + p * 3u;
                unsigned red = row[at % 2048u], green = row[(at + 1u) % 2048u], blue = row[(at + 2u) % 2048u];

                CHECK(s_dst[r * 512 + p] == (uint16_t)(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)));
            }
        }
    }
}

/*----------------------
 | main
 | Description: Runs every rgb24_display test and prints ok when all pass.
 | Author: suinevere
 | Dependencies: the tests above
 | Globals: s_fail
 | Params: N/A
 | Returns: 0 on success, 1 on any failure
 ----------------------*/
int main(void)
{
    test_two_pixels();
    test_blue_and_white();
    test_wraps_at_vram_edges();
    test_rows_land_at_pitch();
    test_matches_per_byte_formula();
    if (!s_fail)
    {
        printf("ok\n");
    }
    return s_fail;
}
