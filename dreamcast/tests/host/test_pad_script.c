/*----------------------
 | test_pad_script.c
 | Description: Host tests for the pad script interpreter: wait-state holding and advance, press hold/release timing, a plain wait, a single capture log line, and mash's stop-then-period presses.
 | Also checks done() going to 1 with buttons staying 0, and that scene and battle steps call their hooks once.
 | Author: suinevere
 | Dependencies: pad_script.h, pad_script.c, watch.h, watch.c
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "pad_script.h"
#include "watch.h"

static int s_failures;
static int s_capture_lines;
static char s_last_capture[64];

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); s_failures++; } } while (0)

static void record_capture(const char *line)
{
    s_capture_lines++;
    strncpy(s_last_capture, line, sizeof(s_last_capture) - 1);
    s_last_capture[sizeof(s_last_capture) - 1] = '\0';
}

static void test_wait_state_holds_then_advances(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_WAIT_STATE, 2, 0, "", 0 },
        { PAD_OP_CAPTURE, 0, 0, "after_state", 0 },
    };
    int i;

    pad_script_start(steps, 2);
    for (i = 0; i < 5; i++)
    {
        pad_script_poll((unsigned)i, 7);
        CHECK(pad_script_buttons() == 0);
        CHECK(!pad_script_done());
    }
    pad_script_poll(100, 2);
    CHECK(pad_script_buttons() == 0);
    CHECK(!pad_script_done());
    s_capture_lines = 0;
    pad_script_poll(101, 2);
    CHECK(pad_script_buttons() == 0);
    CHECK(s_capture_lines == 1);
    CHECK(strcmp(s_last_capture, "LOM: capture after_state") == 0);
    CHECK(pad_script_done());
}

static void test_press_holds_then_releases(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_PRESS, 3, PAD_SCRIPT_BTN_START, "", 0 },
    };
    int i;

    pad_script_start(steps, 1);
    for (i = 0; i < 3; i++)
    {
        pad_script_poll((unsigned)i, 0);
        CHECK(pad_script_buttons() == PAD_SCRIPT_BTN_START);
    }
    for (i = 0; i < 8; i++)
    {
        CHECK(!pad_script_done());
        pad_script_poll((unsigned)(3 + i), 0);
        CHECK(pad_script_buttons() == 0);
    }
    CHECK(pad_script_done());
}

static void test_wait_lasts_exact_vblanks(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_WAIT, 5, 0, "", 0 },
    };
    int i;

    pad_script_start(steps, 1);
    for (i = 0; i < 4; i++)
    {
        pad_script_poll((unsigned)i, 0);
        CHECK(pad_script_buttons() == 0);
        CHECK(!pad_script_done());
    }
    pad_script_poll(4, 0);
    CHECK(pad_script_buttons() == 0);
    CHECK(pad_script_done());
}

static void test_capture_prints_exactly_once(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_CAPTURE, 0, 0, "title", 0 },
    };

    s_capture_lines = 0;
    pad_script_start(steps, 1);
    pad_script_poll(0, 0);
    CHECK(s_capture_lines == 1);
    CHECK(strcmp(s_last_capture, "LOM: capture title") == 0);
    CHECK(pad_script_done());
    CHECK(pad_script_buttons() == 0);
}

static void test_done_buttons_stay_zero_after_last_step(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_WAIT, 1, 0, "", 0 },
    };
    int i;

    pad_script_start(steps, 1);
    pad_script_poll(0, 0);
    CHECK(pad_script_done());
    for (i = 0; i < 4; i++)
    {
        pad_script_poll((unsigned)(1 + i), 0);
        CHECK(pad_script_buttons() == 0);
        CHECK(pad_script_done());
    }
}

static void test_mash_checks_stop_before_pressing(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_MASH, 30, PAD_SCRIPT_BTN_CROSS, "", 7 },
    };

    pad_script_start(steps, 1);
    pad_script_poll(0, 7);
    CHECK(pad_script_buttons() == 0);
    CHECK(pad_script_done());
}

static void test_mash_presses_every_period_until_state_matches(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_MASH, 30, PAD_SCRIPT_BTN_CROSS, "", 7 },
    };
    int i;

    pad_script_start(steps, 1);
    for (i = 0; i < 120; i++)
    {
        int cycle = i % 30;
        int expect_press = cycle < 6;

        pad_script_poll((unsigned)i, 6);
        CHECK(!pad_script_done());
        if (expect_press)
        {
            CHECK(pad_script_buttons() == PAD_SCRIPT_BTN_CROSS);
        }
        else
        {
            CHECK(pad_script_buttons() == 0);
        }
    }
    pad_script_poll(120, 7);
    CHECK(pad_script_buttons() == 0);
    CHECK(pad_script_done());
    for (i = 0; i < 5; i++)
    {
        pad_script_poll((unsigned)(121 + i), 7);
        CHECK(pad_script_buttons() == 0);
        CHECK(pad_script_done());
    }
}

static void test_mash_marker_stop_uses_watch_seen(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_MASH, 30, PAD_SCRIPT_BTN_CROSS, "scene 857", 0 },
    };
    static int32_t scene = 0;
    WatchEntry entries[1] = { { "scene", &scene, 0, 0 } };
    int i;

    watch_init(entries, 1, record_capture);
    watch_poll();
    pad_script_start(steps, 1);
    for (i = 0; i < 10; i++)
    {
        pad_script_poll((unsigned)i, 0);
        CHECK(!pad_script_done());
    }
    scene = 857;
    watch_poll();
    pad_script_poll(10, 0);
    CHECK(pad_script_buttons() == 0);
    CHECK(pad_script_done());
}

static int s_jumps;
static int s_jump_scene;
static int s_jump_spawn;
static int s_battles;

void (*g_dc_field_scene_jump)(int scene, int spawn);
void (*g_dc_field_battle)(void);
void (*g_dc_field_open)(int target);
void (*g_dc_field_gameover)(void);
void (*g_dc_movie_probe)(void);
void (*g_dc_field_movie)(void);

static int s_gameovers;
static int s_field_movies;

static void record_field_movie(void)
{
    s_field_movies++;
}
static int s_tone_hz;
static int s_tone_seconds;
static int s_sound_polls;
static int s_longvoice_seconds;
static int s_music_resource;
static int s_music_offs;
static int s_sfxset_resource;

void sound_test_music(int resource)
{
    s_music_resource = resource;
}

void sound_test_music_off(void)
{
    s_music_offs++;
}

void sound_test_sfxset(int resource)
{
    s_sfxset_resource = resource;
}

void sound_test_longvoice(int seconds)
{
    s_longvoice_seconds = seconds;
}

void sound_test_tone(int hz, int seconds)
{
    s_tone_hz = hz;
    s_tone_seconds = seconds;
}

void sound_test_poll(void)
{
    s_sound_polls++;
}

static void record_gameover(void)
{
    s_gameovers++;
}

static int s_opens;
static int s_open_target;

static void record_open(int target)
{
    s_opens++;
    s_open_target = target;
}

static void record_jump(int scene, int spawn)
{
    s_jumps++;
    s_jump_scene = scene;
    s_jump_spawn = spawn;
}

static void record_battle(void)
{
    s_battles++;
}

static void test_scene_and_battle_call_their_hooks_once(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_SCENE, 71, 0, "", 2 },
        { PAD_OP_BATTLE, 0, 0, "", 0 },
    };

    g_dc_field_scene_jump = record_jump;
    g_dc_field_battle = record_battle;
    pad_script_start(steps, 2);
    pad_script_poll(0, 0);
    CHECK(s_jumps == 1 && s_jump_scene == 71 && s_jump_spawn == 2);
    CHECK(s_battles == 0);
    pad_script_poll(1, 0);
    CHECK(s_battles == 1);
    CHECK(pad_script_done());
    CHECK(pad_script_buttons() == 0);
    pad_script_poll(2, 0);
    CHECK(s_jumps == 1 && s_battles == 1);
}

static void test_scene_and_battle_advance_without_hooks(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_SCENE, 71, 0, "", 0 },
        { PAD_OP_BATTLE, 0, 0, "", 0 },
    };

    g_dc_field_scene_jump = 0;
    g_dc_field_battle = 0;
    pad_script_start(steps, 2);
    pad_script_poll(0, 0);
    pad_script_poll(1, 0);
    CHECK(pad_script_done());
}

static void test_open_calls_its_hook_once_and_advances(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_OPEN, 2, 0, "", 0 },
    };

    g_dc_field_open = record_open;
    pad_script_start(steps, 1);
    pad_script_poll(0, 0);
    CHECK(s_opens == 1 && s_open_target == 2);
    CHECK(pad_script_done());
    pad_script_poll(1, 0);
    CHECK(s_opens == 1);
    g_dc_field_open = 0;
}

static void test_field_movie_calls_its_hook_once_and_advances(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_FIELD_MOVIE, 0, 0, "", 0 },
    };

    g_dc_field_movie = record_field_movie;
    pad_script_start(steps, 1);
    pad_script_poll(0, 0);
    CHECK(s_field_movies == 1);
    CHECK(pad_script_done());
    g_dc_field_movie = 0;
}

static void test_gameover_calls_its_hook_once_and_advances(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_GAMEOVER, 0, 0, "", 0 },
    };

    g_dc_field_gameover = record_gameover;
    pad_script_start(steps, 1);
    pad_script_poll(0, 0);
    CHECK(s_gameovers == 1);
    CHECK(pad_script_done());
    g_dc_field_gameover = 0;
}

static void test_tone_starts_and_advances_while_polling_sound(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_TONE, 1000, 0, "", 3 },
        { PAD_OP_WAIT, 2, 0, "", 0 },
    };

    s_sound_polls = 0;
    pad_script_start(steps, 2);
    pad_script_poll(0, 0);
    CHECK(s_tone_hz == 1000 && s_tone_seconds == 3);
    pad_script_poll(1, 0);
    pad_script_poll(2, 0);
    CHECK(pad_script_done());
    pad_script_poll(3, 0);
    CHECK(s_sound_polls == 4);
}

static void test_longvoice_starts_and_advances(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_LONGVOICE, 3, 0, "", 0 },
    };

    pad_script_start(steps, 1);
    pad_script_poll(0, 0);
    CHECK(s_longvoice_seconds == 3);
    CHECK(pad_script_done());
}

static void test_music_ops_call_their_hooks_and_advance(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_MUSIC, 23, 0, "", 0 },
        { PAD_OP_MUSICOFF, 0, 0, "", 0 },
        { PAD_OP_SFXSET, 81, 0, "", 0 },
    };

    s_music_offs = 0;
    pad_script_start(steps, 3);
    pad_script_poll(0, 0);
    CHECK(s_music_resource == 23 && s_music_offs == 0);
    pad_script_poll(1, 0);
    CHECK(s_music_offs == 1 && s_sfxset_resource == 0);
    pad_script_poll(2, 0);
    CHECK(s_sfxset_resource == 81);
    CHECK(pad_script_done());
}

/*----------------------
 | test_intro_movie_holds_start_until_a_movies_step
 | Description: While the intro movie state (8) runs and no movies step has run, the script holds Start so the game skips the intro; other states never get it.
 | Author: suinevere
 | Dependencies: pad_script_start, pad_script_poll, pad_script_buttons
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_intro_movie_holds_start_until_a_movies_step(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_WAIT, 10, 0, "", 0 },
    };

    pad_script_start(steps, 1);
    pad_script_poll(0, 8);
    CHECK((pad_script_buttons() & PAD_SCRIPT_BTN_START) != 0);
    test_field_movie_calls_its_hook_once_and_advances();
    pad_script_poll(1, 2);
    CHECK(pad_script_buttons() == 0);
}

/*----------------------
 | test_movies_step_lets_the_intro_play
 | Description: After a movies step, the intro movie state gets no held Start, and a restarted script holds it again.
 | Author: suinevere
 | Dependencies: pad_script_start, pad_script_poll, pad_script_buttons
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void test_movies_step_lets_the_intro_play(void)
{
    PadScriptStep steps[] = {
        { PAD_OP_MOVIES, 0, 0, "", 0 },
        { PAD_OP_WAIT, 10, 0, "", 0 },
    };
    PadScriptStep plain[] = {
        { PAD_OP_WAIT, 10, 0, "", 0 },
    };

    pad_script_start(steps, 2);
    pad_script_poll(0, 8);
    pad_script_poll(1, 8);
    CHECK(pad_script_buttons() == 0);
    pad_script_start(plain, 1);
    pad_script_poll(0, 8);
    CHECK((pad_script_buttons() & PAD_SCRIPT_BTN_START) != 0);
}

int main(void)
{
    pad_script_set_printer(record_capture);
    test_wait_state_holds_then_advances();
    test_intro_movie_holds_start_until_a_movies_step();
    test_movies_step_lets_the_intro_play();
    test_press_holds_then_releases();
    test_wait_lasts_exact_vblanks();
    test_capture_prints_exactly_once();
    test_done_buttons_stay_zero_after_last_step();
    test_mash_checks_stop_before_pressing();
    test_mash_presses_every_period_until_state_matches();
    test_mash_marker_stop_uses_watch_seen();
    test_scene_and_battle_call_their_hooks_once();
    test_scene_and_battle_advance_without_hooks();
    test_open_calls_its_hook_once_and_advances();
    test_gameover_calls_its_hook_once_and_advances();
    test_tone_starts_and_advances_while_polling_sound();
    test_longvoice_starts_and_advances();
    test_music_ops_call_their_hooks_and_advance();
    if (s_failures == 0)
    {
        printf("OK\n");
    }
    return s_failures != 0;
}
