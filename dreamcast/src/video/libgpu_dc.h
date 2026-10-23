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

/*----------------------
 | dc_otag_us
 | Description: Microseconds DrawOTag has spent walking ordering tables and submitting primitives since start-up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_otag_us
 | Params: N/A
 | Returns: the total
 ----------------------*/
unsigned long long dc_otag_us(void);

/*----------------------
 | dc_set_vsync_idle
 | Description: Registers the function VSync calls on every pass of its vblank wait, so the platform can hand the CPU to other threads; null for a plain spin.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_vsync_idle
 | Params: idle -- function to call, or null
 | Returns: N/A
 ----------------------*/
void dc_set_vsync_idle(DcVsyncHook idle);

/*----------------------
 | dc_set_async_poll
 | Description: Registers the function DrawSync and VSync call to deliver work a worker thread finished, such as an MDEC decode; null for none.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_async_poll
 | Params: poll -- function to call, or null
 | Returns: N/A
 ----------------------*/
void dc_set_async_poll(DcVsyncHook poll);
unsigned long long vsync_wait_us(void);

#endif
