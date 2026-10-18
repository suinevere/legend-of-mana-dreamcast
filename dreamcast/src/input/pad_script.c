/*----------------------
 | pad_script.c
 | Description: Interpreter for a compiled-in pad script: one step runs per vblank, advancing on a wait, a state match, a hold-then-release, a one-shot capture log, a mash's stop check, or a test-only scene jump or battle start.
 | Author: suinevere
 | Dependencies: pad_script.h, watch.h, dc_system.h (LOM_HOST_TEST off), stdio.h (LOM_HOST_TEST on)
 | Globals: s_steps, s_count, s_index, s_remaining, s_releasing, s_buttons, s_printer
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "pad_script.h"
#include "watch.h"

#ifdef LOM_HOST_TEST
#include <stdio.h>
#else
#include "dc_system.h"
#endif

/*----------------------
 | PAD_SCRIPT_RELEASE_VBLANKS
 | Description: Vblanks a press step holds buttons at 0 after its hold count elapses, before advancing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_RELEASE_VBLANKS 8u

/*----------------------
 | PAD_SCRIPT_MASH_PRESS_VBLANKS
 | Description: Vblanks a mash step holds buttons at the start of each period before idling.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SCRIPT_MASH_PRESS_VBLANKS 6u

/*----------------------
 | s_steps
 | Description: Step table the running script was started with.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const PadScriptStep *s_steps;

/*----------------------
 | s_count
 | Description: Number of steps in s_steps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_count;

/*----------------------
 | s_index
 | Description: Index of the step currently running; equal to s_count once the script is done.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_index;

/*----------------------
 | s_remaining
 | Description: Vblanks left in the current step's wait, hold or release phase.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_remaining;

/*----------------------
 | s_releasing
 | Description: For the current press step: 0 while holding its buttons, 1 while releasing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_releasing;

/*----------------------
 | s_buttons
 | Description: PS1 active-high button mask held this vblank, returned by pad_script_buttons.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint16_t s_buttons;

/*----------------------
 | s_movies_on / s_game_state
 | Description: Whether a movies step has run since the script started, and the game state of the last poll.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_movies_on;
static int s_game_state;

#ifdef LOM_HOST_TEST
/*----------------------
 | s_printer
 | Description: Host-test capture-line callback set by pad_script_set_printer, NULL by default.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void (*s_printer)(const char *);
#endif

/*----------------------
 | enter_step
 | Description: Initializes the wait/hold counters for whichever step is now current, after pad_script_start or advance.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_steps, s_index, s_count, s_remaining, s_releasing
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void enter_step(void)
{
    if (s_index >= s_count)
    {
        return;
    }
    s_releasing = 0;
    switch (s_steps[s_index].op)
    {
    case PAD_OP_WAIT:
    case PAD_OP_PRESS:
        s_remaining = s_steps[s_index].arg;
        break;
    case PAD_OP_MASH:
        s_remaining = PAD_SCRIPT_MASH_PRESS_VBLANKS;
        break;
    default:
        s_remaining = 0;
        break;
    }
}

/*----------------------
 | mash_stopped
 | Description: Reports whether a mash step's stop condition holds: a watcher marker seen in step->name, or else game_state equal to step->stop_state.
 | Author: suinevere
 | Dependencies: watch_seen
 | Globals: N/A
 | Params: step -- the running PAD_OP_MASH step, game_state -- current g_game_state
 | Returns: 1 if the step should stop mashing and advance, else 0
 ----------------------*/
static int mash_stopped(const PadScriptStep *step, int game_state)
{
    if (step->name[0] != '\0')
    {
        return watch_seen(step->name);
    }
    return game_state == (int)step->stop_state;
}

/*----------------------
 | advance
 | Description: Moves to the next step, or marks the script done when none remain.
 | Author: suinevere
 | Dependencies: enter_step
 | Globals: s_index
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void advance(void)
{
    s_index++;
    enter_step();
}

/*----------------------
 | log_capture
 | Description: Prints "LOM: capture <name>" once, via dc_log on target or the host-test printer hook.
 | Author: suinevere
 | Dependencies: dc_log (LOM_HOST_TEST off), s_printer (LOM_HOST_TEST on)
 | Globals: N/A
 | Params: name -- capture marker name
 | Returns: N/A
 ----------------------*/
static void log_capture(const char *name)
{
#ifdef LOM_HOST_TEST
    char line[64];

    if (s_printer == NULL)
    {
        return;
    }
    snprintf(line, sizeof(line), "LOM: capture %s", name);
    s_printer(line);
#else
    dc_log("capture %s", name);
#endif
}

/*----------------------
 | pad_script_start
 | Description: Resets the interpreter to run steps from the beginning.
 | Author: suinevere
 | Dependencies: enter_step
 | Globals: s_steps, s_count, s_index, s_buttons
 | Params: steps -- step table, count -- number of steps
 | Returns: N/A
 ----------------------*/
void pad_script_start(const PadScriptStep *steps, unsigned count)
{
    s_steps = steps;
    s_count = count;
    s_index = 0;
    s_buttons = 0;
    s_movies_on = 0;
    s_game_state = -1;
    enter_step();
}

/*----------------------
 | pad_script_poll
 | Description: Feeds the test tone, then advances the current step by one vblank: counts down waits and holds, checks the wait-state condition, logs a capture marker once, mashes until its stop condition holds, calls a test-only hook once, starts a tone, long voice or song, stops the song, or uploads an effect set.
 | Author: suinevere
 | Dependencies: advance, log_capture, mash_stopped, sound_test_poll, sound_test_tone, sound_test_longvoice, sound_test_music, sound_test_music_off, sound_test_sfxset, g_dc_field_scene_jump, g_dc_field_battle, g_dc_field_open, g_dc_field_gameover, g_dc_movie_probe, g_dc_field_movie
 | Globals: s_steps, s_index, s_count, s_remaining, s_releasing, s_buttons, g_dc_field_scene_jump, g_dc_field_battle, g_dc_field_open, g_dc_field_gameover, g_dc_movie_probe, g_dc_field_movie
 | Params: vblank -- current vblank count (unused), game_state -- current g_game_state
 | Returns: N/A
 ----------------------*/
void pad_script_poll(unsigned vblank, int game_state)
{
    const PadScriptStep *step;

    (void)vblank;
    s_game_state = game_state;
    sound_test_poll();
    if (s_index >= s_count)
    {
        s_buttons = 0;
        return;
    }
    step = &s_steps[s_index];
    switch (step->op)
    {
    case PAD_OP_WAIT_STATE:
        s_buttons = 0;
        if (game_state == (int)step->arg)
        {
            advance();
        }
        break;
    case PAD_OP_WAIT:
        s_buttons = 0;
        if (--s_remaining == 0)
        {
            advance();
        }
        break;
    case PAD_OP_PRESS:
        if (!s_releasing)
        {
            s_buttons = step->buttons;
            if (--s_remaining == 0)
            {
                s_releasing = 1;
                s_remaining = PAD_SCRIPT_RELEASE_VBLANKS;
            }
        }
        else
        {
            s_buttons = 0;
            if (--s_remaining == 0)
            {
                advance();
            }
        }
        break;
    case PAD_OP_MASH:
        if (mash_stopped(step, game_state))
        {
            s_buttons = 0;
            advance();
            break;
        }
        if (!s_releasing)
        {
            s_buttons = step->buttons;
            if (--s_remaining == 0)
            {
                s_releasing = 1;
                s_remaining = step->arg - PAD_SCRIPT_MASH_PRESS_VBLANKS;
            }
        }
        else
        {
            s_buttons = 0;
            if (--s_remaining == 0)
            {
                s_releasing = 0;
                s_remaining = PAD_SCRIPT_MASH_PRESS_VBLANKS;
            }
        }
        break;
    case PAD_OP_SCENE:
        s_buttons = 0;
        if (g_dc_field_scene_jump != 0)
        {
            g_dc_field_scene_jump((int)step->arg, (int)step->stop_state);
        }
        advance();
        break;
    case PAD_OP_BATTLE:
        s_buttons = 0;
        if (g_dc_field_battle != 0)
        {
            g_dc_field_battle();
        }
        advance();
        break;
    case PAD_OP_TONE:
        s_buttons = 0;
        sound_test_tone((int)step->arg, (int)step->stop_state);
        advance();
        break;
    case PAD_OP_LONGVOICE:
        s_buttons = 0;
        sound_test_longvoice((int)step->arg);
        advance();
        break;
    case PAD_OP_MUSIC:
        s_buttons = 0;
        sound_test_music((int)step->arg);
        advance();
        break;
    case PAD_OP_MUSICOFF:
        s_buttons = 0;
        sound_test_music_off();
        advance();
        break;
    case PAD_OP_SFXSET:
        s_buttons = 0;
        sound_test_sfxset((int)step->arg);
        advance();
        break;
    case PAD_OP_GAMEOVER:
        s_buttons = 0;
        if (g_dc_field_gameover != 0)
        {
            g_dc_field_gameover();
        }
        advance();
        break;
    case PAD_OP_PROBE_MOVIE:
        s_buttons = 0;
        if (g_dc_movie_probe != 0)
        {
            g_dc_movie_probe();
        }
        advance();
        break;
    case PAD_OP_FIELD_MOVIE:
        s_buttons = 0;
        if (g_dc_field_movie != 0)
        {
            g_dc_field_movie();
        }
        advance();
        break;
    case PAD_OP_MOVIES:
        s_buttons = 0;
        s_movies_on = 1;
        advance();
        break;
    case PAD_OP_OPEN:
        s_buttons = 0;
        advance();
        if (g_dc_field_open != 0)
        {
            g_dc_field_open((int)step->arg);
        }
        break;
    case PAD_OP_CAPTURE:
    default:
        s_buttons = 0;
        log_capture(step->name);
        advance();
        break;
    }
}

/*----------------------
 | pad_script_buttons
 | Description: Returns the buttons the current step holds this vblank, plus Start through the intro movie until a movies step has run, so the game's own held-button check skips the intro.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_buttons, s_movies_on, s_game_state
 | Params: N/A
 | Returns: PAD_SCRIPT_BTN_* bits held this vblank, 0 when not pressing or done
 ----------------------*/
uint16_t pad_script_buttons(void)
{
    if (!s_movies_on && s_game_state == PAD_SCRIPT_INTRO_MOVIE_STATE)
    {
        return (uint16_t)(s_buttons | PAD_SCRIPT_BTN_START);
    }
    return s_buttons;
}

/*----------------------
 | pad_script_done
 | Description: Reports whether the script has run past its last step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_index, s_count
 | Params: N/A
 | Returns: 1 once every step has completed, else 0
 ----------------------*/
int pad_script_done(void)
{
    return s_index >= s_count;
}

#ifdef LOM_HOST_TEST
/*----------------------
 | pad_script_set_printer
 | Description: Host-test-only hook: routes capture-marker lines to a callback instead of dc_log.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_printer
 | Params: printer -- callback receiving each full "LOM: capture <name>" line
 | Returns: N/A
 ----------------------*/
void pad_script_set_printer(void (*printer)(const char *))
{
    s_printer = printer;
}
#endif
