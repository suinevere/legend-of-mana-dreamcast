/*----------------------
 | pad_dc_map.c
 | Description: Maps a Dreamcast Maple controller sample onto the game's PAD_BTN_* layout, with every PS1 button reachable from the Dreamcast pad, and encodes it as the PS1 held-button word.
 | Author: suinevere
 | Dependencies: pad_dc_map.h, common/pad.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define INCLUDE_ASM_H
#include "common/pad.h"

#include "pad_dc_map.h"

/*----------------------
 | DcContBits
 | Description: KallistiOS CONT_* bit positions for cont_state_t.buttons (dc/maple/controller.h), copied here so this file builds and host-tests without KallistiOS headers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum DcContBits
{
    DC_CONT_C = 1 << 0,
    DC_CONT_B = 1 << 1,
    DC_CONT_A = 1 << 2,
    DC_CONT_START = 1 << 3,
    DC_CONT_DPAD_UP = 1 << 4,
    DC_CONT_DPAD_DOWN = 1 << 5,
    DC_CONT_DPAD_LEFT = 1 << 6,
    DC_CONT_DPAD_RIGHT = 1 << 7,
    DC_CONT_Z = 1 << 8,
    DC_CONT_Y = 1 << 9,
    DC_CONT_X = 1 << 10
};

/*----------------------
 | PAD_DC_TRIGGER_THRESHOLD
 | Description: Trigger pressure (0..255) at or above which a Dreamcast trigger counts as held.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_DC_TRIGGER_THRESHOLD 128

/*----------------------
 | PAD_DC_STICK_THRESHOLD
 | Description: Joystick axis magnitude (0..127) at or beyond which a stick direction counts as held.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_DC_STICK_THRESHOLD 64

/*----------------------
 | pad_dc_logical_buttons
 | Description: Converts a Maple controller sample to the game's PAD_BTN_* layout: A/B/X/Y to Cross/Circle/Square/Triangle, triggers to L1/R1, Start as is, C/Z to L2/R2,
 |              the D-pad to directions and the stick's left, right and bottom edges to L2, R2 and Select; with stick_moves the stick steers too, and with dpad_buttons as well
 |              the D-pad's left, right and bottom give L2, R2 and Select, D-pad up doing nothing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: s -- sampled buttons, stick and trigger values
 | Returns: PAD_BTN_* bit set
 ----------------------*/
uint16_t pad_dc_logical_buttons(const DcPadSample *s)
{
    uint16_t out = 0;
    unsigned buttons = s->buttons;

    if (buttons & DC_CONT_A) out |= PAD_BTN_CROSS;
    if (buttons & DC_CONT_B) out |= PAD_BTN_CIRCLE;
    if (buttons & DC_CONT_X) out |= PAD_BTN_SQUARE;
    if (buttons & DC_CONT_Y) out |= PAD_BTN_TRIANGLE;
    if (buttons & DC_CONT_START) out |= PAD_BTN_START;
    if (s->ltrig >= PAD_DC_TRIGGER_THRESHOLD) out |= PAD_BTN_L1;
    if (s->rtrig >= PAD_DC_TRIGGER_THRESHOLD) out |= PAD_BTN_R1;
    if (buttons & DC_CONT_C) out |= PAD_BTN_L2;
    if (buttons & DC_CONT_Z) out |= PAD_BTN_R2;
    if (s->stick_moves)
    {
        if (s->joyy <= -PAD_DC_STICK_THRESHOLD) out |= PAD_BTN_UP;
        if (s->joyx >= PAD_DC_STICK_THRESHOLD) out |= PAD_BTN_RIGHT;
        if (s->joyy >= PAD_DC_STICK_THRESHOLD) out |= PAD_BTN_DOWN;
        if (s->joyx <= -PAD_DC_STICK_THRESHOLD) out |= PAD_BTN_LEFT;
        if (!s->dpad_buttons)
        {
            if (buttons & DC_CONT_DPAD_UP) out |= PAD_BTN_UP;
            if (buttons & DC_CONT_DPAD_RIGHT) out |= PAD_BTN_RIGHT;
            if (buttons & DC_CONT_DPAD_DOWN) out |= PAD_BTN_DOWN;
            if (buttons & DC_CONT_DPAD_LEFT) out |= PAD_BTN_LEFT;
            return out;
        }
        if (buttons & DC_CONT_DPAD_LEFT) out |= PAD_BTN_L2;
        if (buttons & DC_CONT_DPAD_RIGHT) out |= PAD_BTN_R2;
        if (buttons & DC_CONT_DPAD_DOWN) out |= PAD_BTN_SELECT;
        return out;
    }
    if (buttons & DC_CONT_DPAD_UP) out |= PAD_BTN_UP;
    if (buttons & DC_CONT_DPAD_RIGHT) out |= PAD_BTN_RIGHT;
    if (buttons & DC_CONT_DPAD_DOWN) out |= PAD_BTN_DOWN;
    if (buttons & DC_CONT_DPAD_LEFT) out |= PAD_BTN_LEFT;
    if (s->joyx <= -PAD_DC_STICK_THRESHOLD) out |= PAD_BTN_L2;
    if (s->joyx >= PAD_DC_STICK_THRESHOLD) out |= PAD_BTN_R2;
    if (s->joyy >= PAD_DC_STICK_THRESHOLD) out |= PAD_BTN_SELECT;
    return out;
}

/*----------------------
 | pad_dc_held_word
 | Description: Encodes a Maple sample as the PS1 driver's SCDRegs.held_buttons word, undoing the readers' byte swap and face-button remap as saturn/src/input/pad_encode.c does.
 | Author: suinevere
 | Dependencies: pad_dc_logical_buttons
 | Globals: N/A
 | Params: s -- Maple controller sample
 | Returns: value to store in held_buttons
 ----------------------*/
uint16_t pad_dc_held_word(const DcPadSample *s)
{
    uint16_t b = pad_dc_logical_buttons(s);
    uint16_t raw = (uint16_t)(((b & 0x40) >> 1) | ((b & 0x20) << 1) | ((b & 0x80) >> 3) | ((b & 0x10) << 3) | (b & ~0xF0));

    return (uint16_t)((raw >> 8) | (raw << 8));
}

/*----------------------
 | pad_dc_dpad_as_buttons
 | Description: Whether the D-pad should give L2, R2 and Select this read: always on the world map, and in FIELD only when FIELD's own frame loop is reading the pad and FIELD reports nothing modal open.
 |              Secondary screens run their own loops over FIELD, so field_frame stays clear there and the D-pad steers their menus.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: game_state -- g_game_state, field_frame -- nonzero when FIELD's frame loop is the reader, walking -- FIELD's query, or NULL when FIELD has none registered
 | Returns: 1 for L2/R2/Select on the D-pad, 0 for directions
 ----------------------*/
int pad_dc_dpad_as_buttons(unsigned game_state, int field_frame, PadDcWalkingQuery walking)
{
    if (game_state == PAD_DC_STATE_WORLD_MAP)
    {
        return 1;
    }
    return (game_state == PAD_DC_STATE_FIELD || game_state == PAD_DC_STATE_ATTRACT_1 || game_state == PAD_DC_STATE_ATTRACT_2) && field_frame &&
           walking != NULL && walking();
}
