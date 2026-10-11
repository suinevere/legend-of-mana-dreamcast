/*----------------------
 | pad_dc.c
 | Description: Samples the Dreamcast Maple controller on port A through KallistiOS, or, under LOM_PAD_SCRIPT, a compiled-in pad script instead.
 | Author: suinevere
 | Dependencies: dc/maple.h, dc/maple/controller.h, pad_dc.h, pad_script.h, pad_script_table.h, pad_dc_script_map.h (LOM_PAD_SCRIPT)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include "pad_dc.h"

#ifdef LOM_PAD_SCRIPT
#include "pad_script.h"
#include "pad_script_table.h"
#include "pad_dc_script_map.h"

/*----------------------
 | s_script_started
 | Description: Set once pad_script_start has been called for pad_script_steps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_script_started;
#endif

/*----------------------
 | pad_dc_read
 | Description: Samples the first Maple controller found, if any, into a DcPadSample; under LOM_PAD_SCRIPT, samples the compiled-in pad script instead.
 | Author: suinevere
 | Dependencies: maple_enum_type, maple_dev_status, pad_script_start, pad_script_buttons (LOM_PAD_SCRIPT), pad_dc_script_fill (LOM_PAD_SCRIPT)
 | Globals: N/A
 | Params: out -- output sample, zeroed when no controller answers
 | Returns: 1 when a controller is on port A, else 0
 ----------------------*/
int pad_dc_read(DcPadSample *out)
{
#ifdef LOM_PAD_SCRIPT
    if (!s_script_started)
    {
        pad_script_start(pad_script_steps, pad_script_step_count);
        s_script_started = 1;
    }
    pad_dc_script_fill(out, pad_script_buttons());
    return 1;
#else
    maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    cont_state_t *state;

    out->buttons = 0;
    out->joyx = 0;
    out->joyy = 0;
    out->ltrig = 0;
    out->rtrig = 0;
    if (dev == NULL || dev->port != 0)
    {
        return 0;
    }
    state = (cont_state_t *)maple_dev_status(dev);
    if (state == NULL)
    {
        return 0;
    }
    out->buttons = state->buttons;
    out->joyx = state->joyx;
    out->joyy = state->joyy;
    out->ltrig = state->ltrig;
    out->rtrig = state->rtrig;
    return 1;
#endif
}
