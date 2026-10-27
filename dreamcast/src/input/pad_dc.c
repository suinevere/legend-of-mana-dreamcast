/*----------------------
 | pad_dc.c
 | Description: Samples the Dreamcast Maple controllers on ports A and B through KallistiOS, or, under LOM_PAD_SCRIPT, a compiled-in pad script on port A instead.
 | Author: suinevere
 | Dependencies: dc/maple.h, dc/maple/controller.h, pad_dc.h, pad_script.h, pad_dc_script_map.h (LOM_PAD_SCRIPT)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include "pad_dc.h"

#ifdef LOM_PAD_SCRIPT
#include "pad_script.h"
#include "pad_dc_script_map.h"
#include "dc_system.h"

/*----------------------
 | s_script_started
 | Description: Set once pad_script_start has been called for the chosen script.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_script_started;
#endif

/*----------------------
 | pad_dc_read_port
 | Description: Samples the controller on a Maple port, if any, into a DcPadSample with the stick steering; under LOM_PAD_SCRIPT, port A samples the compiled-in pad script instead, reusing the script dc_system_init already selected through pad_script_selected, and port B has no controller.
 | Author: suinevere
 | Dependencies: maple_enum_dev, maple_dev_status, pad_script_selected, dc_log, pad_script_start, pad_script_buttons (LOM_PAD_SCRIPT), pad_dc_script_fill (LOM_PAD_SCRIPT)
 | Globals: N/A
 | Params: port -- Maple port, 0 for A, out -- output sample, zeroed when no controller answers
 | Returns: 1 when a controller is on the port, else 0
 ----------------------*/
int pad_dc_read_port(int port, DcPadSample *out)
{
#ifdef LOM_PAD_SCRIPT
    if (port != 0)
    {
        return 0;
    }
    if (!s_script_started)
    {
        const PadScriptChoice *script = pad_script_selected();

        dc_log("pad script %s", script->name);
        pad_script_start(script->steps, script->count);
        s_script_started = 1;
    }
    pad_dc_script_fill(out, pad_script_buttons());
    return 1;
#else
    maple_device_t *dev = maple_enum_dev(port, 0);
    cont_state_t *state;

    out->buttons = 0;
    out->joyx = 0;
    out->joyy = 0;
    out->ltrig = 0;
    out->rtrig = 0;
    out->stick_moves = 1;
    out->dpad_buttons = 0;
    if (dev == NULL || !dev->valid || !(dev->info.functions & MAPLE_FUNC_CONTROLLER))
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

/*----------------------
 | pad_dc_read
 | Description: pad_dc_read_port for port A, the first player's controller.
 | Author: suinevere
 | Dependencies: pad_dc_read_port
 | Globals: N/A
 | Params: out -- output sample
 | Returns: 1 when a controller is on port A, else 0
 ----------------------*/
int pad_dc_read(DcPadSample *out)
{
    return pad_dc_read_port(0, out);
}
