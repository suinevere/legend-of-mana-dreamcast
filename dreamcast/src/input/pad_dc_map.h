/*----------------------
 | pad_dc_map.h
 | Description: One Maple controller sample (buttons, stick and triggers) and its mapping onto the game's PAD_BTN_* layout.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PAD_DC_MAP_H
#define LOM_PAD_DC_MAP_H

#include <stdint.h>

/*----------------------
 | DcPadSample
 | Description: One Maple cont_state_t sample: KallistiOS CONT_* button bits, joystick axes (-128..127) and trigger pressures (0..255).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct DcPadSample
{
    unsigned buttons;
    int joyx;
    int joyy;
    int ltrig;
    int rtrig;
} DcPadSample;

uint16_t pad_dc_logical_buttons(const DcPadSample *s);

#endif
