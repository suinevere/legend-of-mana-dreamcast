/*----------------------
 | dc_video_mode.h
 | Description: Picks the Dreamcast boot video mode and its refresh rate from the connected cable, console region and whether 50 Hz is being asked for.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_DC_VIDEO_MODE_H
#define LOM_DC_VIDEO_MODE_H

/*----------------------
 | DC_CABLE_VGA
 | Description: vid_check_cable()'s VGA box value.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DC_CABLE_VGA 0

/*----------------------
 | DC_REGION_EUROPE
 | Description: flashrom_get_region()'s PAL/European region value.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DC_REGION_EUROPE 3

/*----------------------
 | DC_VIDEO_VGA / DC_VIDEO_NTSC / DC_VIDEO_PAL
 | Description: The chosen video mode: 60 Hz VGA, 60 Hz NTSC interlaced, or 50 Hz PAL interlaced.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DC_VIDEO_VGA 0
#define DC_VIDEO_NTSC 1
#define DC_VIDEO_PAL 2

/*----------------------
 | DcVideoChoice
 | Description: A chosen video mode and the refresh rate it runs at.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct DcVideoChoice
{
    int mode;
    int hz;
} DcVideoChoice;

/*----------------------
 | dc_video_choose
 | Description: A VGA cable always picks 60 Hz VGA; otherwise a European console with hold_50hz set picks 50 Hz PAL; anything else picks 60 Hz NTSC.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: cable -- vid_check_cable() value, region -- flashrom_get_region() value, hold_50hz -- 1 to ask for 50 Hz on a European console
 | Returns: the chosen mode and its refresh rate
 ----------------------*/
DcVideoChoice dc_video_choose(int cable, int region, int hold_50hz);

#endif
