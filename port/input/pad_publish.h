/*----------------------
 | pad_publish.h
 | Description: The PadSample a backend's pad driver produces and pad_publish_port, which replays it into a decomp ControllerPortState with the PS1 edge and d-pad repeat rules.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PAD_PUBLISH_H
#define LOM_PAD_PUBLISH_H

#include <stdint.h>

/*----------------------
 | PAD_SAMPLE_DIGITAL
 | Description: PS1 device_type a connected digital pad publishes, equal to CONTROLLER_DEVICE_DIGITAL.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SAMPLE_DIGITAL 0x00

/*----------------------
 | PAD_SAMPLE_DISCONNECTED
 | Description: PS1 device_type an empty port publishes, equal to CONTROLLER_DEVICE_DISCONNECTED.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAD_SAMPLE_DISCONNECTED 0xFF

/*----------------------
 | PadSample
 | Description: The device_type and held_buttons a backend's pad driver publishes into a port's ControllerSample.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct PadSample
{
    uint8_t device_type;
    uint16_t held_buttons;
} PadSample;

/*----------------------
 | pad_publish_port
 | Description: Replays a frame's sample into a decomp ControllerPortState, defined by main/controller.h, which this header only forward-declares to avoid depending on it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: port -- ControllerPortState, sample -- encoded sample, vblanks -- vblanks since the last publish, fast_repeat, sample_unavailable -- CONTROLLER_STATE flags
 | Returns: N/A
 ----------------------*/
struct ControllerPortState;
void pad_publish_port(struct ControllerPortState *port, PadSample sample, int vblanks, int fast_repeat, int sample_unavailable);

#endif
