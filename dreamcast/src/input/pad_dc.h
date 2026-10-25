/*----------------------
 | pad_dc.h
 | Description: Reads the Dreamcast Maple controller on port A into a DcPadSample.
 | Author: suinevere
 | Dependencies: pad_dc_map.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PAD_DC_H
#define LOM_PAD_DC_H

#include "pad_dc_map.h"

int pad_dc_read(DcPadSample *out);
void dc_pad_field_frame(void);
void dc_pad_set_field_walking(PadDcWalkingQuery walking);

#endif
