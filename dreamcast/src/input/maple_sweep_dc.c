/*----------------------
 | maple_sweep_dc.c
 | Description: Clears frames KallistiOS left queued after a timed-out request marked them vacant; such a frame is never sent or dequeued again, so its device could not be locked, polled or reattached.
 | Author: suinevere
 | Dependencies: maple_sweep_dc.h, dc/maple.h
 | Globals: maple_state
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <dc/maple.h>

#include "maple_sweep_dc.h"

/*----------------------
 | maple_sweep_dc
 | Description: From the vblank interrupt, while no Maple DMA is out, takes every vacant frame off the queue; a live queued frame is always unsent or sent, and with no DMA out no late reply can land in a freed frame.
 | Author: suinevere
 | Dependencies: maple_queue_remove
 | Globals: maple_state
 | Params: N/A (interrupt context)
 | Returns: N/A
 ----------------------*/
void maple_sweep_dc(void)
{
    maple_frame_t *frame;
    maple_frame_t *next;

    if (maple_state.dma_in_progress)
    {
        return;
    }
    for (frame = TAILQ_FIRST(&maple_state.frame_queue); frame != NULL; frame = next)
    {
        next = TAILQ_NEXT(frame, frameq);
        if (frame->state == MAPLE_FRAME_VACANT)
        {
            maple_queue_remove(frame);
        }
    }
}
