/*----------------------
 | pad_dc.c
 | Description: Samples the Dreamcast Maple controller on port A through KallistiOS.
 | Author: suinevere
 | Dependencies: dc/maple.h, dc/maple/controller.h, pad_dc.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include "pad_dc.h"

/*----------------------
 | pad_dc_read
 | Description: Samples the first Maple controller found, if any, into a DcPadSample.
 | Author: suinevere
 | Dependencies: maple_enum_type, maple_dev_status
 | Globals: N/A
 | Params: out -- output sample, zeroed when no controller answers
 | Returns: 1 when a controller is on port A, else 0
 ----------------------*/
int pad_dc_read(DcPadSample *out)
{
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
}
