/*----------------------
 | test_libpress_dc.c
 | Description: Host test for libpress_dc.c: decodes one STR frame column by column through the output callback, as movie.c chains DecDCTout, and writes the picture as raw RGB24.
 | Author: suinevere
 | Dependencies: libpress.h, libpress_dc.c, mdec.c, str_vlc.c
 | Globals: s_fail, s_image, s_slices, s_width, s_height, s_depth24, s_column, s_out_idx, s_depth, s_max_depth, s_columns
 | Params: argv -- frame file, width, height, output file, depth (24 or 15)
 | Returns: N/A
 ----------------------*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*----------------------
 | u_char / u_short / u_long
 | Description: The Psy-Q short type names libpress.h uses, repeated identically to include/common.h so this file builds without it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_long;

#include "sdk/libpress.h"

static int s_fail;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); s_fail = 1; } } while (0)

static uint8_t s_image[320 * 240 * 3];
static u_long s_slices[2][16 * 240 * 3 / 4];
static u_long s_vlc[81920 / 4];
static int s_width, s_height, s_depth24, s_column, s_out_idx, s_depth, s_max_depth, s_columns;

/*----------------------
 | dc_log / dc_time_us
 | Description: Host stand-ins for the Dreamcast logger and timer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: format -- ignored
 | Returns: dc_time_us: 0
 ----------------------*/
void dc_log(const char *format, ...)
{
    (void)format;
}

unsigned long long dc_time_us(void)
{
    return 0;
}

/*----------------------
 | slice_size_words
 | Description: Words in one 16-pixel-wide output column, as movie.c computes w * h / 2 from the slice rect in VRAM halfwords.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_depth24, s_height
 | Params: N/A
 | Returns: word count
 ----------------------*/
static int slice_size_words(void)
{
    return (s_depth24 ? 24 : 16) * s_height / 2;
}

/*----------------------
 | store_column
 | Description: Copies the finished column into the RGB24 picture, widening 15-bit pixels to 8 bits per channel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_slices, s_out_idx, s_image, s_column, s_width, s_height, s_depth24
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void store_column(void)
{
    const uint8_t *src = (const uint8_t *)s_slices[s_out_idx];
    int x, y;

    for (y = 0; y < s_height; y++)
    {
        for (x = 0; x < 16; x++)
        {
            uint8_t *dst = &s_image[(y * s_width + s_column + x) * 3];

            if (s_depth24)
            {
                memcpy(dst, &src[(y * 16 + x) * 3], 3);
            }
            else
            {
                unsigned v = (unsigned)src[(y * 16 + x) * 2] | ((unsigned)src[(y * 16 + x) * 2 + 1] << 8);

                dst[0] = (uint8_t)((v & 31) << 3);
                dst[1] = (uint8_t)(((v >> 5) & 31) << 3);
                dst[2] = (uint8_t)(((v >> 10) & 31) << 3);
            }
        }
    }
}

/*----------------------
 | on_out
 | Description: Output callback: stores the column, swaps slice buffers and asks for the next column, like movie_schedule_next_decode.
 | Author: suinevere
 | Dependencies: store_column, DecDCTout
 | Globals: s_depth, s_max_depth, s_columns, s_column, s_out_idx, s_width
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void on_out(void)
{
    s_depth++;
    s_max_depth = s_depth > s_max_depth ? s_depth : s_max_depth;
    store_column();
    s_columns++;
    s_out_idx = 1 - s_out_idx;
    s_column += 16;
    if (s_column < s_width)
    {
        DecDCTout(s_slices[s_out_idx], slice_size_words());
    }
    s_depth--;
}

/*----------------------
 | main
 | Description: Decodes the frame file, checks the callback chain, and writes the picture.
 | Author: suinevere
 | Dependencies: DecDCTReset, DecDCTvlcBuild, DecDCToutCallback, DecDCTvlc2, DecDCTin, DecDCTout
 | Globals: all of the above
 | Params: argc, argv -- frame width height out depth
 | Returns: 0 on success, 1 on any failure
 ----------------------*/
int main(int argc, char **argv)
{
    static u_long bs[81920 / 4];
    FILE *f;

    if (argc != 6)
    {
        printf("usage: test_libpress_dc frame width height out depth\n");
        return 2;
    }
    f = fopen(argv[1], "rb");
    CHECK(f != NULL);
    if (f == NULL)
    {
        return 1;
    }
    CHECK(fread(bs, 1, sizeof(bs), f) > 0);
    fclose(f);
    s_width = atoi(argv[2]);
    s_height = atoi(argv[3]);
    s_depth24 = atoi(argv[5]) == 24;
    DecDCTReset(0);
    DecDCTvlcBuild(0);
    CHECK(DecDCToutCallback(on_out) == 0);
    CHECK(DecDCTvlc2(bs, s_vlc, 0) == 0);
    DecDCTin(s_vlc, s_depth24);
    DecDCTout(s_slices[s_out_idx], slice_size_words());
    CHECK(s_max_depth == 1);
    CHECK(s_columns == (s_width + 15) / 16);
    CHECK(DecDCToutCallback(0) == (int)(intptr_t)on_out);
    f = fopen(argv[4], "wb");
    CHECK(f != NULL);
    if (f != NULL)
    {
        fwrite(s_image, 3, (size_t)(s_width * s_height), f);
        fclose(f);
    }
    if (!s_fail)
    {
        printf("ok\n");
    }
    return s_fail;
}
