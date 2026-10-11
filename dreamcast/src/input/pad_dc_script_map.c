/*----------------------
 | pad_dc_script_map.c
 | Description: Reverse of pad_dc_map.c: maps a pad script's PS1 button mask back onto the Maple-shaped DcPadSample fields pad_dc_logical_buttons reads, so the two agree on every button.
 | Author: suinevere
 | Dependencies: pad_dc_script_map.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "pad_dc_script_map.h"

/*----------------------
 | DC_SCRIPT_CONT_A / _B / _X / _Y / _START / _UP / _DOWN / _LEFT / _RIGHT
 | Description: KallistiOS CONT_* bit positions pad_dc_map.c's pad_dc_logical_buttons reads, duplicated here (matching pad_dc_map.c's internal DcContBits) so this file needs no KallistiOS headers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define DC_SCRIPT_CONT_A 0x0004u
#define DC_SCRIPT_CONT_B 0x0002u
#define DC_SCRIPT_CONT_X 0x0400u
#define DC_SCRIPT_CONT_Y 0x0200u
#define DC_SCRIPT_CONT_START 0x0008u
#define DC_SCRIPT_CONT_UP 0x0010u
#define DC_SCRIPT_CONT_DOWN 0x0020u
#define DC_SCRIPT_CONT_LEFT 0x0040u
#define DC_SCRIPT_CONT_RIGHT 0x0080u

/*----------------------
 | pad_dc_script_fill
 | Description: Maps a PAD_SCRIPT_BTN_* mask back onto a Maple-shaped DcPadSample, so the unchanged pad_dc_logical_buttons forward mapping reproduces exactly the same PS1 buttons.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: out -- sample to fill, want -- PAD_SCRIPT_BTN_* mask held this vblank
 | Returns: N/A
 ----------------------*/
void pad_dc_script_fill(DcPadSample *out, uint16_t want)
{
    out->buttons = 0;
    out->joyx = 0;
    out->joyy = 0;
    out->ltrig = 0;
    out->rtrig = 0;
    if (want & PAD_SCRIPT_BTN_CROSS) out->buttons |= DC_SCRIPT_CONT_A;
    if (want & PAD_SCRIPT_BTN_CIRCLE) out->buttons |= DC_SCRIPT_CONT_B;
    if (want & PAD_SCRIPT_BTN_SQUARE) out->buttons |= DC_SCRIPT_CONT_X;
    if (want & PAD_SCRIPT_BTN_TRIANGLE) out->buttons |= DC_SCRIPT_CONT_Y;
    if (want & PAD_SCRIPT_BTN_START) out->buttons |= DC_SCRIPT_CONT_START;
    if (want & PAD_SCRIPT_BTN_UP) out->buttons |= DC_SCRIPT_CONT_UP;
    if (want & PAD_SCRIPT_BTN_DOWN) out->buttons |= DC_SCRIPT_CONT_DOWN;
    if (want & PAD_SCRIPT_BTN_LEFT) out->buttons |= DC_SCRIPT_CONT_LEFT;
    if (want & PAD_SCRIPT_BTN_RIGHT) out->buttons |= DC_SCRIPT_CONT_RIGHT;
    if (want & PAD_SCRIPT_BTN_L1) out->ltrig = 255;
    if (want & PAD_SCRIPT_BTN_R1) out->rtrig = 255;
    if (want & PAD_SCRIPT_BTN_L2) out->joyx = -128;
    else if (want & PAD_SCRIPT_BTN_R2) out->joyx = 127;
    if (want & PAD_SCRIPT_BTN_SELECT) out->joyy = 127;
}
