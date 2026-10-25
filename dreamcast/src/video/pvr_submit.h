/*----------------------
 | pvr_submit.h
 | Description: Submits decoded PS1 primitives to one autosort-off PowerVR translucent list at 2x scale, opening a scene on first use and closing it at the flip. Also remembers the last finished frame's draw area.
 | Author: suinevere
 | Dependencies: stdint.h, gpu_decode.h (DcPrim, DcGpuState), pvr_backend.h (PvrBackend)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PVR_SUBMIT_H
#define LOM_PVR_SUBMIT_H

#include <stdint.h>

#include "gpu_decode.h"
#include "pvr_backend.h"

/*----------------------
 | pvr_submit_init
 | Description: Initialises the PowerVR with only the translucent list sized and autosort off, ARGB8888 palettes and a black background, and binds the caches and the KallistiOS backend.
 | Author: suinevere
 | Dependencies: pvr_backend_kos.c (pvr_init, pvr_set_pal_format, texture_cache_init, palette_bank_init, pvr_submit_set_backend)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void pvr_submit_init(void);

/*----------------------
 | pvr_submit_set_backend
 | Description: Binds the hardware hooks every later call goes through; pvr_submit_init binds KallistiOS, host tests a fake.
 | Author: suinevere
 | Dependencies: PvrBackend
 | Globals: N/A
 | Params: backend -- hooks, kept by pointer
 | Returns: N/A
 ----------------------*/
void pvr_submit_set_backend(const PvrBackend *backend);

/*----------------------
 | pvr_submit_prim
 | Description: Clips one primitive to the draw area and writes it as one or two blended triangle strips, opening the scene first if none is open.
 | Author: suinevere
 | Dependencies: clip_prim, texture_cache_lookup, blend_plan
 | Globals: N/A
 | Params: prim -- decoded primitive, state -- GPU state carrying the draw area
 | Returns: N/A
 ----------------------*/
void pvr_submit_prim(const DcPrim *prim, const DcGpuState *state);

/*----------------------
 | pvr_submit_flip
 | Description: Finishes the translucent list and the scene if one is open, so the PowerVR renders it.
 | Author: suinevere
 | Dependencies: PvrBackend.scene_finish, frame_probe_scene
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void pvr_submit_flip(void);

/*----------------------
 | pvr_submit_origin
 | Description: Sets where the draw area's top-left corner lands on the 320x240 screen, for every later primitive and rect.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x, y -- screen position in PS1 pixels
 | Returns: N/A
 ----------------------*/
void pvr_submit_origin(int x, int y);

/*----------------------
 | pvr_submit_background
 | Description: Opens the scene and sets the colour it clears to, so a frame with only a clear still renders at the next flip.
 | Author: suinevere
 | Dependencies: PvrBackend.background
 | Globals: N/A
 | Params: ps1_colour -- PS1 15-bit colour
 | Returns: N/A
 ----------------------*/
void pvr_submit_background(uint16_t ps1_colour);

/*----------------------
 | pvr_submit_rect
 | Description: Opens the scene and draws an opaque untextured rectangle, standing in for a ClearImage that only covers part of the draw area.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x, y -- top-left relative to the draw area in PS1 pixels, w, h -- size, ps1_colour -- PS1 15-bit colour
 | Returns: N/A
 ----------------------*/
void pvr_submit_rect(int x, int y, int w, int h, uint16_t ps1_colour);

/*----------------------
 | pvr_submit_scene_has_prims
 | Description: Whether the open scene has had a triangle strip sent since it opened, so a full-area ClearImage knows whether the background alone would hide earlier prims.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: 1 if a strip was sent this scene, else 0
 ----------------------*/
int pvr_submit_scene_has_prims(void);

/*----------------------
 | pvr_frame_rect
 | Description: Returns the PS1 draw-area rect and screen origin of the last scene confirmed on screen, as plain fields (not RECT) so this header still compiles under KallistiOS-only translation units.
 | May promote a still-pending scene first, if the backend now reports it reached the screen.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x, y, w, h, origin_x, origin_y -- outputs, filled when a confirmed frame exists
 | Returns: 1 if a confirmed frame's rect was returned, 0 if there is none yet
 ----------------------*/
int pvr_frame_rect(int16_t *x, int16_t *y, int16_t *w, int16_t *h, int16_t *origin_x, int16_t *origin_y);

/*----------------------
 | pvr_frame_settle
 | Description: Before VRAM at a rect is read back, waits for the last closed scene to reach the screen when it drew there and the frame on screen did not, so the read sees that frame as the PS1's VRAM would instead of nothing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x, y, w, h -- VRAM rect about to be read
 | Returns: N/A
 ----------------------*/
void pvr_frame_settle(int x, int y, int w, int h);

/*----------------------
 | pvr_submit_keep_screen
 | Description: Tells the submitter whether the current draw environment leaves the previous frame in place (isbg clear) or clears it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_keep_screen
 | Params: keep -- nonzero when the draw environment does not clear
 | Returns: N/A
 ----------------------*/
void pvr_submit_keep_screen(int keep);

/*----------------------
 | pvr_submit_rgb24
 | Description: Shows a 24-bit display area as one opaque full-screen textured quad in its own scene, counted by the frame probe like any flip.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: pixels -- RGB565 rows at a 512-pixel pitch, width, height -- frame size in pixels, at most 512x256
 | Returns: N/A
 ----------------------*/
void pvr_submit_rgb24(const uint16_t *pixels, unsigned width, unsigned height);

#endif
