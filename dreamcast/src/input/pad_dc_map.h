/*----------------------
 | pad_dc_map.h
 | Description: One Maple controller sample (buttons, stick and triggers), its mapping onto the game's PAD_BTN_* layout, its encoding as the PS1 driver's held-button word, the soft-reset chord and the Jump Pack level.
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
int pad_dc_soft_reset_chord(const DcPadSample *s);

/*----------------------
 | PAD_DC_RUMBLE_SMALL / PAD_DC_RUMBLE_MAX
 | Description: The Jump Pack level the PS1's small motor alone gives, a light buzz, and the strongest level.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_DC_RUMBLE_SMALL 2
#define PAD_DC_RUMBLE_MAX 7

/*----------------------
 | PAD_DC_RUMBLE_REFRESH / PAD_DC_RUMBLE_FREQ
 | Description: Frames between re-sends of a held vibration, under the Jump Pack's shortest auto-stop of 250 ms, and the vibration frequency sent, inside the pack's 4-30 Hz range.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_DC_RUMBLE_REFRESH 10
#define PAD_DC_RUMBLE_FREQ 0x1A

int pad_dc_rumble_level(int enabled, unsigned small_motor, unsigned large_motor);
unsigned pad_dc_rumble_effect(int level);
int pad_dc_rumble_due(int level, int sent, int age);

#endif
