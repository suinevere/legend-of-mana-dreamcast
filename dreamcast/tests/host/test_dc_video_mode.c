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

/*----------------------
 | CT_RGB / CT_COMPOSITE / FLASHROM_REGION_US
 | Description: vid_check_cable()/flashrom_get_region() values the test needs, copied so this host test stays KOS-header free.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CT_RGB 2
#define CT_COMPOSITE 3
#define FLASHROM_REGION_US 2

/*----------------------
 | check
 | Description: Checks dc_video_choose(cable, region, hold) against an expected mode and hz.
 | Author: suinevere
 | Dependencies: dc_video_choose
 | Globals: N/A
 | Params: cable, region, hold -- dc_video_choose arguments, mode, hz -- expected result
 | Returns: N/A
 ----------------------*/
static void check(int cable, int region, int hold, int mode, int hz)
{
    DcVideoChoice choice = dc_video_choose(cable, region, hold);

    CHECK(choice.mode == mode);
    CHECK(choice.hz == hz);
}

/*----------------------
 | main
 | Description: Runs every dc_video_choose case and prints OK when all pass.
 | Author: suinevere
 | Dependencies: check
 | Globals: s_fail
 | Params: N/A
 | Returns: 0 on success, 1 on any failure
 ----------------------*/
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
