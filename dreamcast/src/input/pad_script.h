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
 | PAD_OP_SCENE
 | Description: Test-only step op: asks FIELD to change to scene arg at spawn stop_state through g_dc_field_scene_jump, the way a door does, then advances.
 | Author: suinevere
 | Dependencies: g_dc_field_scene_jump
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_SCENE 5

/*----------------------
 | PAD_OP_BATTLE
 | Description: Test-only step op: starts the current scene's first monster group through g_dc_field_battle, then advances.
 | Author: suinevere
 | Dependencies: g_dc_field_battle
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_BATTLE 6

/*----------------------
 | PAD_OP_OPEN
 | Description: Test-only step op: asks FIELD to open secondary overlay arg (an OPEN_TARGETS index) through g_dc_field_open, then advances.
 | Author: suinevere
 | Dependencies: g_dc_field_open
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_OPEN 7

/*----------------------
 | PAD_OP_GAMEOVER
 | Description: Test-only step op: starts a game over through g_dc_field_gameover, then advances.
 | Author: suinevere
 | Dependencies: g_dc_field_gameover
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_GAMEOVER 8

/*----------------------
 | PAD_OP_TONE
 | Description: Test-only step op: starts a sine of arg Hz for stop_state seconds on the AICA stream channel through sound_test_tone, then advances at once.
 | Author: suinevere
 | Dependencies: sound_test_tone
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_TONE 9

/*----------------------
 | PAD_OP_LONGVOICE
 | Description: Test-only step op: plays a sample longer than one AICA channel reaches on SPU voice 23 for arg seconds through sound_test_longvoice, then advances at once.
 | Author: suinevere
 | Dependencies: sound_test_longvoice
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_LONGVOICE 10

/*----------------------
 | PAD_OP_MUSIC
 | Description: Test-only step op: loads music resource arg and plays its first song at full volume through sound_test_music, then advances at once.
 | Author: suinevere
 | Dependencies: sound_test_music
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_MUSIC 11

/*----------------------
 | PAD_OP_MUSICOFF
 | Description: Test-only step op: stops the playing song through sound_test_music_off, then advances at once.
 | Author: suinevere
 | Dependencies: sound_test_music_off
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_MUSICOFF 12

/*----------------------
 | PAD_OP_SFXSET
 | Description: Test-only step op: loads effect resource arg and uploads every bank it holds through sound_test_sfxset, then advances at once.
 | Author: suinevere
 | Dependencies: sound_test_sfxset
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_OP_SFXSET 13

/*----------------------
 | sound_test_tone / sound_test_longvoice / sound_test_music / sound_test_music_off / sound_test_sfxset / sound_test_poll
 | Description: The test image's sound checks: start a stream-channel tone, a streamed long voice or a music resource's song, stop the song, upload an effect resource's banks, and feed or end them once per vblank.
 | Author: suinevere
 | Dependencies: src/sound/sound_test_dc.c
 | Globals: N/A
 | Params: hz -- tone frequency, seconds -- length, resource -- CD resource
 | Returns: N/A
 ----------------------*/
void sound_test_tone(int hz, int seconds);
void sound_test_longvoice(int seconds);
void sound_test_music(int resource);
void sound_test_music_off(void);
void sound_test_sfxset(int resource);
void sound_test_poll(void);

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
 | PadScriptChoice
 | Description: One compiled-in pad script: the name a disc's PADSEL.TXT picks it by, and its steps.
 | Author: suinevere
 | Dependencies: PadScriptStep
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    const char *name;
    const PadScriptStep *steps;
    unsigned count;
} PadScriptChoice;

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
