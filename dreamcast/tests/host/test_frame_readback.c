/*----------------------
 | test_frame_readback.c
 | Description: Host tests for frame_readback_copy: 2x point-sampled RGB565 conversion back into PS1 15-bit mirror pixels, and that a clipped call only touches its own rows.
 | Author: suinevere
 | Dependencies: frame_readback.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdint.h>
#include <stdio.h>

#include "frame_readback.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static uint16_t s_frame[640 * 480];

static void fill_frame(void)
{
    int x;
    int y;

    for (y = 0; y < 480; y++)
    {
        for (x = 0; x < 640; x++)
        {
            s_frame[y * 640 + x] = (uint16_t)((((x / 2) & 0x1F) << 11) | (((y / 2) & 0x3F) << 5));
        }
    }
}

static void test_reads_back_a_known_pixel(void)
{
    uint16_t mirror[320 * 224];

    frame_readback_copy(s_frame, 640, 0, 240, mirror, 0, 240, 320, 224);
    CHECK((mirror[(250 - 240) * 320 + (10 - 0)] & 0x1Fu) == 10u);
    CHECK(((mirror[(250 - 240) * 320 + (10 - 0)] >> 5) & 0x1Fu) == ((10u * 255u / 63u) >> 3));
}

static void test_straddling_rect_only_touches_the_overlap(void)
{
    uint16_t mirror[8];
    uint16_t before[8];
    int row;

    for (row = 0; row < 8; row++)
    {
        mirror[row] = (uint16_t)(0xDEAD + row);
        before[row] = mirror[row];
    }
    frame_readback_copy(s_frame, 640, 0, 240, mirror, 5, 460, 1, 4);
    for (row = 0; row < 4; row++)
    {
        CHECK(mirror[row] != before[row]);
    }
    for (row = 4; row < 8; row++)
    {
        CHECK(mirror[row] == before[row]);
    }
}

int main(void)
{
    fill_frame();
    test_reads_back_a_known_pixel();
    test_straddling_rect_only_touches_the_overlap();

    if (failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
