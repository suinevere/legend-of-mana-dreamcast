/*----------------------
 | pad_dc_script_map.h
 | Description: Maps a pad script's PS1 button mask back onto a Maple-shaped DcPadSample, so pad_dc_map.c's unchanged forward mapping reproduces the same PS1 buttons.
 | Author: suinevere
 | Dependencies: pad_dc_map.h, pad_script.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PAD_DC_SCRIPT_MAP_H
#define LOM_PAD_DC_SCRIPT_MAP_H

#include "pad_dc_map.h"
#include "pad_script.h"

/*----------------------
 | pad_dc_script_fill
 | Description: Maps a PAD_SCRIPT_BTN_* mask back onto a Maple-shaped DcPadSample, so the unchanged pad_dc_logical_buttons forward mapping reproduces exactly the same PS1 buttons.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: out -- sample to fill, want -- PAD_SCRIPT_BTN_* mask held this vblank
 | Returns: N/A
 ----------------------*/
void pad_dc_script_fill(DcPadSample *out, uint16_t want);

#endif
