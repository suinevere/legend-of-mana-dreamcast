/*----------------------
 | vsync_pacer.h
 | Description: PS1 libetc VSync semantics on a vblank counter ticked by the vblank interrupt (vblank_dc.c).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_VSYNC_PACER_H
#define LOM_VSYNC_PACER_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | VsyncClock
 | Description: Reads the current vblank count; vsync_count on the Dreamcast, a fake counter in host tests.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: vblanks elapsed
 ----------------------*/
typedef unsigned (*VsyncClock)(void);

void vsync_tick(void);
unsigned vsync_count(void);
int vsync_wait(int mode, VsyncClock clock);
void vsync_wait_after(VsyncClock clock);

#ifdef __cplusplus
}
#endif

#endif
