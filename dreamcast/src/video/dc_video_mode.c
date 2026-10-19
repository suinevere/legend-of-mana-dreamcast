/*----------------------
 | dc_video_mode.c
 | Description: Pure logic for the Dreamcast boot video mode choice: VGA always wins, a PAL console needs a held button to drop to 50 Hz, everything else is 60 Hz NTSC.
 | Author: suinevere
 | Dependencies: dc_video_mode.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "dc_video_mode.h"

/*----------------------
 | dc_video_choose
 | Description: A VGA cable always picks 60 Hz VGA; otherwise a European console with hold_50hz set picks 50 Hz PAL; anything else picks 60 Hz NTSC.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: cable -- vid_check_cable() value, region -- flashrom_get_region() value, hold_50hz -- 1 to ask for 50 Hz on a European console
 | Returns: the chosen mode and its refresh rate
 ----------------------*/
DcVideoChoice dc_video_choose(int cable, int region, int hold_50hz)
{
    DcVideoChoice choice;

    if (cable == DC_CABLE_VGA)
    {
        choice.mode = DC_VIDEO_VGA;
        choice.hz = 60;
        return choice;
    }
    if (region == DC_REGION_EUROPE && hold_50hz)
    {
        choice.mode = DC_VIDEO_PAL;
        choice.hz = 50;
        return choice;
    }
    choice.mode = DC_VIDEO_NTSC;
    choice.hz = 60;
    return choice;
}
