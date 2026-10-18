/*----------------------
 | test_str_vlc.c
 | Description: Host test for str_vlc.c: a real STR frame decodes to whole blocks of in-range run-length codes, and truncated or oversized input stops safely.
 | Author: suinevere
 | Dependencies: str_vlc.h, mdec.h
 | Globals: N/A
 | Params: argv -- optional frame file, width, height
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mdec.h"
#include "str_vlc.h"

static int s_fail;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); s_fail = 1; } } while (0)

static uint32_t s_dst[81920 / 4];

/*----------------------
 | test_truncated_stream_pads_and_fails
 | Description: A frame whose bits run out before its codes do returns -1 with the rest padded by EOB.
 | Author: suinevere
 | Dependencies: str_vlc_decode_frame
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_truncated_stream_pads_and_fails(void)
{
    static uint32_t dst[1 + 64];
    uint16_t bs[6] = {32, 0x3800, 1, 2, 0x0000, 0x0000};

    CHECK(str_vlc_decode_frame(bs, 6, dst, 1 + 64) == -1);
    CHECK((dst[0] & 0xFFFF0000u) == 0x38000000u);
    CHECK(((uint16_t *)&dst[1])[63] == MDEC_EOB);
}

/*----------------------
 | test_dst_too_small_never_overruns
 | Description: A frame claiming more codes than the output holds returns -1 without writing past it.
 | Author: suinevere
 | Dependencies: str_vlc_decode_frame
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_dst_too_small_never_overruns(void)
{
    static uint32_t dst[1 + 4 + 1];
    uint16_t bs[6] = {200, 0x3800, 1, 2, 0x5555, 0x5555};

    dst[5] = 0xDEADBEEF;
    CHECK(str_vlc_decode_frame(bs, 6, dst, 1 + 4) == -1);
    CHECK(dst[5] == 0xDEADBEEF);
}

/*----------------------
 | test_not_v2_fails
 | Description: A version-3 header is refused.
 | Author: suinevere
 | Dependencies: str_vlc_decode_frame
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_not_v2_fails(void)
{
    static uint32_t dst[1 + 8];
    uint16_t bs[6] = {4, 0x3800, 1, 3, 0, 0};

    CHECK(str_vlc_decode_frame(bs, 6, dst, 1 + 8) == -1);
}

/*----------------------
 | check_real_frame
 | Description: Decodes a frame file and checks every block closes with EOB, no coefficient index passes 63, and the block count fits the picture.
 | Author: suinevere
 | Dependencies: str_vlc_decode_frame
 | Globals: s_dst
 | Params: path -- frame bitstream file, w, h -- picture size
 | Returns: N/A
 ----------------------*/
static void check_real_frame(const char *path, int w, int h)
{
    static uint16_t bs[81920 / 2];
    FILE *f = fopen(path, "rb");
    size_t n;
    const uint16_t *hw;
    unsigned count, i, blocks = 0;
    int k = -1;
    int index_ok = 1;

    CHECK(f != NULL);
    if (f == NULL)
    {
        return;
    }
    n = fread(bs, 1, sizeof(bs), f);
    fclose(f);
    CHECK(str_vlc_decode_frame(bs, (unsigned)(n / 2), s_dst, sizeof(s_dst) / 4) == 0);
    count = 2u * (s_dst[0] & 0xFFFFu);
    hw = (const uint16_t *)&s_dst[1];
    for (i = 0; i < count; i++)
    {
        if (k < 0)
        {
            if (hw[i] != MDEC_EOB)
            {
                k = 0;
            }
            continue;
        }
        k += ((hw[i] >> 10) & 0x3F) + 1;
        if (hw[i] == MDEC_EOB)
        {
            blocks++;
            k = -1;
        }
        else if (k > 63)
        {
            index_ok = 0;
        }
    }
    CHECK(index_ok);
    CHECK(k < 0);
    printf("%s: %u blocks, %u expected\n", path, blocks, 6u * ((w + 15) / 16) * ((h + 15) / 16));
    CHECK(blocks == 6u * (unsigned)((w + 15) / 16) * (unsigned)((h + 15) / 16));
}

/*----------------------
 | main
 | Description: Runs the synthetic tests, then checks the frame file named on the command line, if any.
 | Author: suinevere
 | Dependencies: str_vlc_init
 | Globals: s_fail
 | Params: argc, argv -- [frame width height]
 | Returns: 0 on success, 1 on any failure
 ----------------------*/
int main(int argc, char **argv)
{
    str_vlc_init();
    test_truncated_stream_pads_and_fails();
    test_dst_too_small_never_overruns();
    test_not_v2_fails();
    if (argc == 4)
    {
        check_real_frame(argv[1], atoi(argv[2]), atoi(argv[3]));
    }
    if (!s_fail)
    {
        printf("ok\n");
    }
    return s_fail;
}
