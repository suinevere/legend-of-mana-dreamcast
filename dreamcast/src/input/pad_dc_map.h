/*----------------------
 | pad_dc_map.h
 | Description: One Maple controller sample (buttons, stick and triggers), its mapping onto the game's PAD_BTN_* layout and its encoding as the PS1 driver's held-button word.
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
 | Description: One Maple cont_state_t sample: KallistiOS CONT_* button bits, joystick axes (-128..127) and trigger pressures (0..255),
 |              plus stick_moves, nonzero when the stick steers like the D-pad, and dpad_buttons, nonzero when the D-pad then gives L2, R2 and Select instead of directions.
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
    int stick_moves;
    int dpad_buttons;
} DcPadSample;

/*----------------------
 | PAD_DC_STATE_FIELD / PAD_DC_STATE_WORLD_MAP / PAD_DC_STATE_ATTRACT_1 / PAD_DC_STATE_ATTRACT_2 / PadDcWalkingQuery
 | Description: The g_game_state values pad_dc_dpad_as_buttons tells apart (main/game_state.h's), where FIELD runs under FIELD, ATTRACT_1 and ATTRACT_2 alike, and FIELD's answer to whether nothing modal is open.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_DC_STATE_FIELD 0
#define PAD_DC_STATE_WORLD_MAP 1
#define PAD_DC_STATE_ATTRACT_1 9
#define PAD_DC_STATE_ATTRACT_2 10
typedef int (*PadDcWalkingQuery)(void);

uint16_t pad_dc_logical_buttons(const DcPadSample *s);
int pad_dc_dpad_as_buttons(unsigned game_state, int field_frame, PadDcWalkingQuery walking);
uint16_t pad_dc_held_word(const DcPadSample *s);

#endif
