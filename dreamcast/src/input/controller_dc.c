/*----------------------
 | controller_dc.c
 | Description: Dreamcast stand-ins for the main executable's controller service, publishing the Maple pad on port A into port 0 and leaving port 1 disconnected.
 | Author: suinevere
 | Dependencies: lom_prefix.h, common/pad.h, main/controller.h, main/controller_internal.h, pad_dc.h, pad_dc_map.h, pad_publish.h, vsync_pacer.h
 | Globals: s_last_vblank
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stddef.h>

#include "common/pad.h"
#include "main/controller.h"
#include "main/controller_internal.h"

#include "pad_dc.h"
#include "pad_dc_map.h"
#include "pad_publish.h"
#include "vsync_pacer.h"

/*----------------------
 | s_last_vblank
 | Description: vsync_count at the previous update_controllers call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_last_vblank;

/*----------------------
 | scd_regs_device_type_overlaps_port0
 | Description: SCD_REGS.device_type is port 0's published_sample.device_type, so one write serves the title and FIELD readers.
 | Author: suinevere
 | Dependencies: common/pad.h, main/controller_internal.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
_Static_assert(offsetof(ControllerState, ports) + offsetof(ControllerPortState, published_sample) + offsetof(ControllerSample, device_type) == offsetof(SCDRegs, device_type), "SCD_REGS device_type must alias port 0");

/*----------------------
 | scd_regs_held_buttons_overlaps_port0
 | Description: SCD_REGS.held_buttons is port 0's published_sample.held_buttons.
 | Author: suinevere
 | Dependencies: common/pad.h, main/controller_internal.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
_Static_assert(offsetof(ControllerState, ports) + offsetof(ControllerPortState, published_sample) + offsetof(ControllerSample, held_buttons) == offsetof(SCDRegs, held_buttons), "SCD_REGS held_buttons must alias port 0");

/*----------------------
 | dc_pad_encode_sample
 | Description: Builds the PadSample pad_publish_port expects from a Maple read: a digital pad with the PS1 logical buttons, or a disconnected port.
 | Author: suinevere
 | Dependencies: pad_dc_logical_buttons
 | Globals: N/A
 | Params: connected -- nonzero when a Maple controller answered, sample -- its Maple read
 | Returns: PadSample for ControllerSample.device_type and held_buttons
 ----------------------*/
static PadSample dc_pad_encode_sample(int connected, const DcPadSample *sample)
{
    PadSample out;

    out.device_type = connected ? PAD_SAMPLE_DIGITAL : PAD_SAMPLE_DISCONNECTED;
    out.held_buttons = connected ? pad_dc_logical_buttons(sample) : 0;
    return out;
}

/*----------------------
 | update_controllers
 | Description: Publishes the Maple pad on port A as port 0 and an empty port 1, advancing repeat timers by the vblanks elapsed since the last call.
 | Author: suinevere
 | Dependencies: vsync_count, pad_dc_read, dc_pad_encode_sample, pad_publish_port
 | Globals: CONTROLLER_STATE, SCD_REGS, s_last_vblank
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void update_controllers(void)
{
    ControllerState *state = CONTROLLER_STATE;
    unsigned now = vsync_count();
    int vblanks = (int)(now - s_last_vblank);
    DcPadSample sample;
    int connected = pad_dc_read(&sample);

    s_last_vblank = now;
    pad_publish_port(&state->ports[0], dc_pad_encode_sample(connected, &sample), vblanks, state->fast_button_repeat, state->sample_unavailable);
    pad_publish_port(&state->ports[1], dc_pad_encode_sample(0, &sample), vblanks, state->fast_button_repeat, state->sample_unavailable);
}

/*----------------------
 | set_controller_vsync_interval
 | Description: Pad polling is tied to the Dreamcast frame, so the PS1 polling interval is ignored.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: interval -- ignored
 | Returns: N/A
 ----------------------*/
void set_controller_vsync_interval(u32 interval)
{
    (void)interval;
}

/*----------------------
 | reset_controller_vsync_state
 | Description: Nothing to reset on the Dreamcast.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void reset_controller_vsync_state(void)
{
}

/*----------------------
 | initialize_controllers
 | Description: KallistiOS enumerates Maple devices itself; nothing to set up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: enable_actuators -- ignored
 | Returns: N/A
 ----------------------*/
void initialize_controllers(s8 enable_actuators)
{
    (void)enable_actuators;
}

/*----------------------
 | initialize_controller_vsync
 | Description: Pad polling is tied to the Dreamcast frame; nothing to hook.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void initialize_controller_vsync(void)
{
}
