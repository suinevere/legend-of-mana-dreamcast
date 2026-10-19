/*----------------------
 | test_dc_video_mode.c
 | Description: Host test for dc_video_mode.c: a VGA cable always wins, a held button drops a European console to 50 Hz PAL, and every other case is 60 Hz NTSC.
 | Author: suinevere
 | Dependencies: dc_video_mode.h
 ----------------------*/
#include <stdio.h>

#include "dc_video_mode.h"

static int s_fail;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); s_fail = 1; } } while (0)

#define CT_RGB 2
#define CT_COMPOSITE 3
#define FLASHROM_REGION_US 2

static void check(int cable, int region, int hold, int mode, int hz)
{
    DcVideoChoice choice = dc_video_choose(cable, region, hold);

    CHECK(choice.mode == mode);
    CHECK(choice.hz == hz);
}

int main(void)
{
    check(DC_CABLE_VGA, DC_REGION_EUROPE, 1, DC_VIDEO_VGA, 60);
    check(CT_RGB, DC_REGION_EUROPE, 0, DC_VIDEO_NTSC, 60);
    check(CT_RGB, DC_REGION_EUROPE, 1, DC_VIDEO_PAL, 50);
    check(CT_COMPOSITE, FLASHROM_REGION_US, 1, DC_VIDEO_NTSC, 60);
    check(CT_COMPOSITE, DC_REGION_EUROPE, 1, DC_VIDEO_PAL, 50);
    if (!s_fail)
    {
        printf("OK\n");
    }
    return s_fail;
}
