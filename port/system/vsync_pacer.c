/*----------------------
 | vsync_pacer.c
 | Description: PS1 libetc VSync semantics on a vblank counter ticked by the Saturn vblank interrupt.
 | Author: suinevere
 | Dependencies: vsync_pacer.h
 | Globals: s_vblanks, s_last_vsync
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "vsync_pacer.h"

/*----------------------
 | s_vblanks
 | Description: Vblanks seen since boot, written only by the vblank interrupt.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static volatile unsigned s_vblanks;

/*----------------------
 | s_last_vsync
 | Description: Vblank count at which the previous waiting VSync returned.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_last_vsync;

/*----------------------
 | vsync_tick
 | Description: Counts one vblank; registered on the SaturnRingLib vblank event.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_vblanks
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void vsync_tick(void)
{
    s_vblanks++;
}

/*----------------------
 | vsync_count
 | Description: Returns the vblanks seen since boot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_vblanks
 | Params: N/A
 | Returns: vblank count
 ----------------------*/
unsigned vsync_count(void)
{
    return s_vblanks;
}

/*----------------------
 | vsync_wait
 | Description: VSync(mode): 0 waits for the next vblank, n >= 2 waits until n vblanks have passed since the last wait returned, without catching up missed ones.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_last_vsync
 | Params: mode -- PS1 VSync mode, clock -- vblank counter to poll
 | Returns: vblank count for mode < 0, else 0 (no horizontal counter)
 ----------------------*/
int vsync_wait(int mode, VsyncClock clock)
{
    unsigned now;
    unsigned target;

    if (mode == 1)
    {
        return 0;
    }
    now = clock();
    if (mode < 0)
    {
        return (int)now;
    }
    target = mode == 0 ? now + 1u : s_last_vsync + (unsigned)mode;
    while ((int)(now - target) < 0)
    {
        now = clock();
    }
    s_last_vsync = now;
    return 0;
}

/*----------------------
 | vsync_wait_after
 | Description: Returns once a vblank has passed since the last wait returned, at once if one already has, so a frame that overran is shown without waiting a further vblank.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_last_vsync
 | Params: clock -- vblank counter to poll
 | Returns: N/A
 ----------------------*/
void vsync_wait_after(VsyncClock clock)
{
    unsigned now = clock();

    while ((int)(now - (s_last_vsync + 1u)) < 0)
    {
        now = clock();
    }
    s_last_vsync = now;
}
