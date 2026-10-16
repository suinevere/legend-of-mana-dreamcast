/*----------------------
 | pad_script.h
 | Description: A compiled-in pad script interpreter: waits for a game state or a vblank count, holds buttons for a count then releases, and logs capture markers.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PAD_SCRIPT_H
#define LOM_PAD_SCRIPT_H

#include <stdint.h>

/*----------------------
 | PAD_OP_WAIT_STATE
 | Description: Step op: hold no buttons until g_game_state equals arg, then advance.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_WAIT_STATE 0

/*----------------------
 | PAD_OP_WAIT
 | Description: Step op: hold no buttons for arg vblanks, then advance.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_WAIT 1

/*----------------------
 | PAD_OP_PRESS
 | Description: Step op: hold the step's buttons for arg vblanks, release for PAD_SCRIPT_RELEASE_VBLANKS, then advance.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_PRESS 2

/*----------------------
 | PAD_OP_CAPTURE
 | Description: Step op: log the capture marker once, then advance immediately.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_CAPTURE 3

/*----------------------
 | PAD_OP_MASH
 | Description: Step op: every arg vblanks hold the step's buttons for 6 vblanks, but check the stop first: game_state == stop_state, or (when name is non-empty) watch_seen(name).
 | Author: suinevere
 | Dependencies: watch_seen
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_MASH 4

/*----------------------
 | PAD_SCRIPT_BTN_L2
 | Description: PS1 active-high L2 bit, numerically equal to common/pad.h's PAD_BTN_L2.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_L2 0x0001u

/*----------------------
 | PAD_SCRIPT_BTN_R2
 | Description: PS1 active-high R2 bit, numerically equal to common/pad.h's PAD_BTN_R2.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_R2 0x0002u

/*----------------------
 | PAD_SCRIPT_BTN_L1
 | Description: PS1 active-high L1 bit, numerically equal to common/pad.h's PAD_BTN_L1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_L1 0x0004u

/*----------------------
 | PAD_SCRIPT_BTN_R1
 | Description: PS1 active-high R1 bit, numerically equal to common/pad.h's PAD_BTN_R1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_R1 0x0008u

/*----------------------
 | PAD_SCRIPT_BTN_SQUARE
 | Description: PS1 active-high Square bit, numerically equal to common/pad.h's PAD_BTN_SQUARE.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_SQUARE 0x0010u

/*----------------------
 | PAD_SCRIPT_BTN_CROSS
 | Description: PS1 active-high Cross bit, numerically equal to common/pad.h's PAD_BTN_CROSS.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_CROSS 0x0020u

/*----------------------
 | PAD_SCRIPT_BTN_CIRCLE
 | Description: PS1 active-high Circle bit, numerically equal to common/pad.h's PAD_BTN_CIRCLE.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_CIRCLE 0x0040u

/*----------------------
 | PAD_SCRIPT_BTN_TRIANGLE
 | Description: PS1 active-high Triangle bit, numerically equal to common/pad.h's PAD_BTN_TRIANGLE.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_TRIANGLE 0x0080u

/*----------------------
 | PAD_SCRIPT_BTN_SELECT
 | Description: PS1 active-high Select bit, numerically equal to common/pad.h's PAD_BTN_SELECT.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_SELECT 0x0100u

/*----------------------
 | PAD_SCRIPT_BTN_START
 | Description: PS1 active-high Start bit, numerically equal to common/pad.h's PAD_BTN_START.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_START 0x0800u

/*----------------------
 | PAD_SCRIPT_BTN_UP
 | Description: PS1 active-high D-pad Up bit, numerically equal to common/pad.h's PAD_BTN_UP.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_UP 0x1000u

/*----------------------
 | PAD_SCRIPT_BTN_RIGHT
 | Description: PS1 active-high D-pad Right bit, numerically equal to common/pad.h's PAD_BTN_RIGHT.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_RIGHT 0x2000u

/*----------------------
 | PAD_SCRIPT_BTN_DOWN
 | Description: PS1 active-high D-pad Down bit, numerically equal to common/pad.h's PAD_BTN_DOWN.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_DOWN 0x4000u

/*----------------------
 | PAD_SCRIPT_BTN_LEFT
 | Description: PS1 active-high D-pad Left bit, numerically equal to common/pad.h's PAD_BTN_LEFT.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_BTN_LEFT 0x8000u

/*----------------------
 | PadScriptStep
 | Description: One pad script step: an op, its wait/hold/period count, the buttons a press or mash step holds, a capture or mash-marker name, and a mash step's stop state.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    uint8_t op;
    uint16_t arg;
    uint16_t buttons;
    const char *name;
    uint16_t stop_state;
} PadScriptStep;

/*----------------------
 | pad_script_start
 | Description: Resets the interpreter to run steps from the beginning.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: steps -- step table, count -- number of steps
 | Returns: N/A
 ----------------------*/
void pad_script_start(const PadScriptStep *steps, unsigned count);

/*----------------------
 | pad_script_poll
 | Description: Advances the current step by one vblank: counts down waits and holds, checks the wait-state condition, and logs a capture marker once.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: vblank -- current vblank count, unused by every op but kept for callers that log it, game_state -- current g_game_state
 | Returns: N/A
 ----------------------*/
void pad_script_poll(unsigned vblank, int game_state);

/*----------------------
 | pad_script_buttons
 | Description: Returns the PS1 active-high button mask the script is holding this vblank.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: PAD_SCRIPT_BTN_* bits held this vblank, 0 when not pressing or done
 ----------------------*/
uint16_t pad_script_buttons(void);

/*----------------------
 | pad_script_done
 | Description: Reports whether the script has run past its last step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 1 once every step has completed, else 0
 ----------------------*/
int pad_script_done(void);

#ifdef LOM_HOST_TEST
/*----------------------
 | pad_script_set_printer
 | Description: Host-test-only hook: routes capture-marker lines to a callback instead of dc_log.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: printer -- callback receiving each full "LOM: capture <name>" line
 | Returns: N/A
 ----------------------*/
void pad_script_set_printer(void (*printer)(const char *));
#endif

#endif
