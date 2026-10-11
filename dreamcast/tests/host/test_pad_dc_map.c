/*----------------------
 | test_pad_dc_map.c
 | Description: Host checks for the Dreamcast Maple-to-PS1 button mapping, its L1/R1/L2/R2/Select thresholds, the C/Z alias buttons, and that pad_dc_script_fill's reverse mapping agrees with this forward mapping for every scripted button.
 | Author: suinevere
 | Dependencies: pad_dc_map.h, pad_dc_script_map.h, common/pad.h
 ----------------------*/
#include <stdio.h>

#define INCLUDE_ASM_H
#include "common/pad.h"

#include "pad_dc_map.h"
#include "pad_dc_script_map.h"

#define DC_CONT_C (1 << 0)
#define DC_CONT_B (1 << 1)
#define DC_CONT_A (1 << 2)
#define DC_CONT_START (1 << 3)
#define DC_CONT_DPAD_UP (1 << 4)
#define DC_CONT_DPAD_DOWN (1 << 5)
#define DC_CONT_DPAD_LEFT (1 << 6)
#define DC_CONT_DPAD_RIGHT (1 << 7)
#define DC_CONT_Z (1 << 8)
#define DC_CONT_Y (1 << 9)
#define DC_CONT_X (1 << 10)

static int s_failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); s_failures++; } } while (0)

static DcPadSample zero_sample(void)
{
    DcPadSample s;

    s.buttons = 0;
    s.joyx = 0;
    s.joyy = 0;
    s.ltrig = 0;
    s.rtrig = 0;
    return s;
}

static void test_all_zero_gives_nothing(void)
{
    DcPadSample s = zero_sample();

    CHECK(pad_dc_logical_buttons(&s) == 0);
}

static void test_face_buttons(void)
{
    DcPadSample s = zero_sample();

    s.buttons = DC_CONT_A;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_CROSS);
    s.buttons = DC_CONT_B;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_CIRCLE);
    s.buttons = DC_CONT_X;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_SQUARE);
    s.buttons = DC_CONT_Y;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_TRIANGLE);
}

static void test_start_and_dpad(void)
{
    DcPadSample s = zero_sample();

    s.buttons = DC_CONT_START;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_START);
    s.buttons = DC_CONT_DPAD_UP;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_UP);
    s.buttons = DC_CONT_DPAD_DOWN;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_DOWN);
    s.buttons = DC_CONT_DPAD_LEFT;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_LEFT);
    s.buttons = DC_CONT_DPAD_RIGHT;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_RIGHT);
}

static void test_trigger_thresholds(void)
{
    DcPadSample s = zero_sample();

    s.ltrig = 127;
    CHECK(pad_dc_logical_buttons(&s) == 0);
    s.ltrig = 128;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_L1);

    s = zero_sample();
    s.rtrig = 127;
    CHECK(pad_dc_logical_buttons(&s) == 0);
    s.rtrig = 128;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_R1);
}

static void test_stick_thresholds(void)
{
    DcPadSample s = zero_sample();

    s.joyx = -63;
    CHECK(pad_dc_logical_buttons(&s) == 0);
    s.joyx = -64;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_L2);

    s = zero_sample();
    s.joyx = 63;
    CHECK(pad_dc_logical_buttons(&s) == 0);
    s.joyx = 64;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_R2);

    s = zero_sample();
    s.joyy = 63;
    CHECK(pad_dc_logical_buttons(&s) == 0);
    s.joyy = 64;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_SELECT);

    s = zero_sample();
    s.joyy = -64;
    CHECK(pad_dc_logical_buttons(&s) == 0);
}

static void test_c_and_z_alias_l2_r2(void)
{
    DcPadSample s = zero_sample();

    s.buttons = DC_CONT_C;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_L2);
    s.buttons = DC_CONT_C;
    s.joyx = -64;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_L2);

    s = zero_sample();
    s.buttons = DC_CONT_Z;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_R2);
    s.buttons = DC_CONT_Z;
    s.joyx = 64;
    CHECK(pad_dc_logical_buttons(&s) == PAD_BTN_R2);
}

static void test_script_fill_round_trips_through_real_map(void)
{
    static const uint16_t buttons[] = {
        PAD_SCRIPT_BTN_UP, PAD_SCRIPT_BTN_DOWN, PAD_SCRIPT_BTN_LEFT, PAD_SCRIPT_BTN_RIGHT,
        PAD_SCRIPT_BTN_CROSS, PAD_SCRIPT_BTN_CIRCLE, PAD_SCRIPT_BTN_SQUARE, PAD_SCRIPT_BTN_TRIANGLE,
        PAD_SCRIPT_BTN_START, PAD_SCRIPT_BTN_SELECT,
        PAD_SCRIPT_BTN_L1, PAD_SCRIPT_BTN_R1, PAD_SCRIPT_BTN_L2, PAD_SCRIPT_BTN_R2
    };
    int i;

    for (i = 0; i < (int)(sizeof(buttons) / sizeof(buttons[0])); i++)
    {
        DcPadSample s = zero_sample();

        pad_dc_script_fill(&s, buttons[i]);
        CHECK(pad_dc_logical_buttons(&s) == buttons[i]);
    }
}

static uint16_t title_read(uint16_t held)
{
    uint16_t buttons = (uint16_t)((held >> 8) | (held << 8));

    return (uint16_t)PAD_REMAP_FACE_BITS(buttons);
}

static void test_held_word_reads_back_as_logical_buttons(void)
{
    static const unsigned dc_buttons[] = {
        DC_CONT_A, DC_CONT_B, DC_CONT_X, DC_CONT_Y, DC_CONT_START,
        DC_CONT_DPAD_UP, DC_CONT_DPAD_DOWN, DC_CONT_DPAD_LEFT, DC_CONT_DPAD_RIGHT
    };
    int i;

    for (i = 0; i < (int)(sizeof(dc_buttons) / sizeof(dc_buttons[0])); i++)
    {
        DcPadSample s = zero_sample();

        s.buttons = dc_buttons[i];
        CHECK(title_read(pad_dc_held_word(&s)) == pad_dc_logical_buttons(&s));
    }
    {
        DcPadSample s = zero_sample();

        s.buttons = DC_CONT_A;
        CHECK(pad_dc_held_word(&s) == 0x4000);
    }
}

int main(void)
{
    test_all_zero_gives_nothing();
    test_face_buttons();
    test_start_and_dpad();
    test_trigger_thresholds();
    test_stick_thresholds();
    test_c_and_z_alias_l2_r2();
    test_script_fill_round_trips_through_real_map();
    test_held_word_reads_back_as_logical_buttons();
    if (s_failures == 0)
    {
        printf("OK\n");
    }
    return s_failures != 0;
}
