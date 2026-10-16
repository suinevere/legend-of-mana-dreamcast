/*----------------------
 | test_pad_script.c
 | Description: Host tests for the pad script interpreter: wait-state holding and advance, press hold/release timing, a plain wait, a single capture log line, and mash's stop-then-period presses.
 | Also checks done() going to 1 with buttons staying 0.
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
        pad_script_poll((unsigned)i, 8);
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

int main(void)
{
    pad_script_set_printer(record_capture);
    test_wait_state_holds_then_advances();
    test_press_holds_then_releases();
    test_wait_lasts_exact_vblanks();
    test_capture_prints_exactly_once();
    test_done_buttons_stay_zero_after_last_step();
    test_mash_checks_stop_before_pressing();
    test_mash_presses_every_period_until_state_matches();
    test_mash_marker_stop_uses_watch_seen();
    if (s_failures == 0)
    {
        printf("OK\n");
    }
    return s_failures != 0;
}
