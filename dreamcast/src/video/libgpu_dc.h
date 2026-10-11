/*----------------------
 | libgpu_dc.h
 | Description: Dreamcast-only entry points of the counting libgpu backend: per-frame primitive counts and the per-VSync hook.
 | Author: suinevere
 | Dependencies: libgpu_dc.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_LIBGPU_DC_H
#define LOM_LIBGPU_DC_H

/*----------------------
 | DcVsyncHook
 | Description: A function VSync calls after each wait.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef void (*DcVsyncHook)(void);

unsigned dc_frame_primitives(void);
void dc_set_vsync_hook(DcVsyncHook hook);

#endif
