/*----------------------
 | pad_publish.c
 | Description: Replays a backend's encoded pad sample into a decomp ControllerPortState once per elapsed vblank, publishing held, pressed and repeat buttons with the PS1 accumulate-and-merge and d-pad auto-repeat rules.
 | Author: suinevere
 | Dependencies: pad_publish.h, main/controller_internal.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define INCLUDE_ASM_H
#include "main/controller_internal.h"

#include "pad_publish.h"
#include "libetc.h"

/*----------------------
 | PAD_MAX_VBLANK_STEPS
 | Description: Most vblanks replayed for one published sample, so a long stall cannot loop for long.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_MAX_VBLANK_STEPS 8

/*----------------------
 | repeat_dpad_button
 | Description: Runs one d-pad direction's PS1 auto-repeat timer for one vblank: a fresh press arms the delay, holding counts down and sets the repeat bit each interval.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: timer -- the port's timer for this button, bit -- PADR* bit, sample -- current sample, delay, interval -- repeat timing, sample_unavailable -- CONTROLLER_STATE flag
 | Returns: N/A
 ----------------------*/
static void repeat_dpad_button(u8 *timer, uint16_t bit, ControllerSample *sample, int delay, int interval, int sample_unavailable)
{
    int delta;

    if (!(sample->held_buttons & bit))
    {
        return;
    }
    if ((sample->pressed_buttons & bit) && sample_unavailable == 0)
    {
        *timer = (u8)delay;
        return;
    }
    delta = *timer - 1;
    if (delta <= 0)
    {
        delta = interval;
        sample->repeat_buttons |= bit;
    }
    *timer = (u8)delta;
}

/*----------------------
 | step_sample
 | Description: Applies one vblank's digital sample to a port's current sample the way controller_poll does: edges become pressed and repeat, then d-pad directions auto-repeat.
 | Author: suinevere
 | Dependencies: repeat_dpad_button
 | Globals: N/A
 | Params: port -- port state, sample -- encoded sample, fast_repeat -- CONTROLLER_STATE flag, sample_unavailable -- CONTROLLER_STATE flag
 | Returns: N/A
 ----------------------*/
static void step_sample(ControllerPortState *port, PadSample sample, int fast_repeat, int sample_unavailable)
{
    ControllerSample *cur = &port->current_sample;
    u16 held = sample.held_buttons;
    int delay = fast_repeat ? CONTROLLER_FAST_REPEAT_DELAY : CONTROLLER_NORMAL_REPEAT_DELAY;
    int interval = fast_repeat ? CONTROLLER_FAST_REPEAT_INTERVAL : CONTROLLER_NORMAL_REPEAT_INTERVAL;

    if (cur->device_type == sample.device_type)
    {
        cur->pressed_buttons = cur->repeat_buttons = (u16)(held & (cur->held_buttons ^ held));
    }
    else
    {
        cur->pressed_buttons = cur->repeat_buttons = held;
        cur->device_type = sample.device_type;
    }
    cur->held_buttons = held;
    repeat_dpad_button(&port->face_repeat_timer_up, PADRup, cur, delay, interval, sample_unavailable);
    repeat_dpad_button(&port->face_repeat_timer_right, PADRright, cur, delay, interval, sample_unavailable);
    repeat_dpad_button(&port->face_repeat_timer_down, PADRdown, cur, delay, interval, sample_unavailable);
    repeat_dpad_button(&port->face_repeat_timer_left, PADRleft, cur, delay, interval, sample_unavailable);
}

/*----------------------
 | pad_publish_port
 | Description: Replays a frame's sample once per elapsed vblank and publishes held, pressed and repeat buttons OR-ed over those vblanks, as the PS1 accumulate-and-merge does.
 | Author: suinevere
 | Dependencies: step_sample
 | Globals: N/A
 | Params: port -- ControllerPortState, sample -- encoded sample, vblanks -- vblanks since the last publish, fast_repeat, sample_unavailable -- CONTROLLER_STATE flags
 | Returns: N/A
 ----------------------*/
void pad_publish_port(struct ControllerPortState *port, PadSample sample, int vblanks, int fast_repeat, int sample_unavailable)
{
    ControllerSample *cur = &port->current_sample;
    u16 pressed = 0;
    u16 repeat = 0;
    int i;

    if (sample.device_type != PAD_SAMPLE_DIGITAL)
    {
        cur->device_type = sample.device_type;
        cur->held_buttons = cur->pressed_buttons = cur->repeat_buttons = 0;
        port->published_sample.device_type = sample.device_type;
        port->published_sample.held_buttons = port->published_sample.pressed_buttons = port->published_sample.repeat_buttons = 0;
        return;
    }
    if (vblanks < 1)
    {
        vblanks = 1;
    }
    if (vblanks > PAD_MAX_VBLANK_STEPS)
    {
        vblanks = PAD_MAX_VBLANK_STEPS;
    }
    for (i = 0; i < vblanks; i++)
    {
        step_sample(port, sample, fast_repeat, sample_unavailable);
        pressed |= cur->pressed_buttons;
        repeat |= cur->repeat_buttons;
    }
    port->published_sample.device_type = cur->device_type;
    port->published_sample.held_buttons = cur->held_buttons;
    port->published_sample.pressed_buttons = pressed;
    port->published_sample.repeat_buttons = repeat;
}
