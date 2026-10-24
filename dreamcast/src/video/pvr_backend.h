/*----------------------
 | pvr_backend.h
 | Description: The hardware side of PowerVR submission as a table of hooks, so pvr_submit's scene policy is pure C and host tests can fake the hardware.
 | Author: suinevere
 | Dependencies: stdint.h, texture_cache.h (DcSurfaceTexture), blend.h (BlendPass)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PVR_BACKEND_H
#define LOM_PVR_BACKEND_H

#include <stdint.h>

#include "texture_cache.h"
#include "blend.h"

/*----------------------
 | PvrVertex
 | Description: One strip vertex in screen pixels with page-normalised texel coordinates and its colour.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    float x, y, u, v;
    uint32_t argb;
} PvrVertex;

/*----------------------
 | PvrBackend
 | Description: Starts and finishes a scene, sets the background, and writes a polygon header or a whole triangle strip of PvrVertex in one call.
 | scene_begin returns 0 when the previous flip is not yet confirmed, leaving a pending texture flush for later; scene_shown checks the same without waiting, and wait_shown waits a few vblanks at most for it.
 | frame_texture loads the RGB565 rows (512-pixel pitch) of a 24-bit display frame into a texture of its own and returns it, or NULL when texture memory has no room.
 | strip_mark returns a handle to the next strip while the scene is still being recorded, or NULL once it streams; patch_strip rewrites that strip's vertex positions.
 | capture_screen copies the shown frame at PS1 resolution into screen slot 0 or 1, returning 0 when it cannot; screen_texture returns that slot's last capture, or NULL.
 | Author: suinevere
 | Dependencies: DcSurfaceTexture, BlendPass
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    int (*scene_begin)(unsigned scenes_closed);
    void (*scene_finish)(void);
    void (*background)(uint8_t r, uint8_t g, uint8_t b);
    void (*header)(const DcSurfaceTexture *tex, const BlendPass *pass);
    void (*strip)(const PvrVertex *verts, unsigned count);
    int (*scene_shown)(unsigned scenes_closed);
    int (*wait_shown)(unsigned scenes_closed);
    const DcSurfaceTexture *(*frame_texture)(const uint16_t *pixels, unsigned width, unsigned height);
    void *(*strip_mark)(void);
    void (*patch_strip)(void *mark, const PvrVertex *verts, unsigned count);
    int (*capture_screen)(unsigned slot);
    const DcSurfaceTexture *(*screen_texture)(unsigned slot);
} PvrBackend;

/*----------------------
 | pvr_backend_wait_us
 | Description: Microseconds the KallistiOS backend has spent waiting for the PowerVR to finish a scene before opening the next, for the speed readout.
 | Author: suinevere
 | Dependencies: pvr_backend_kos.c
 | Globals: s_pvr_wait_us
 | Params: N/A
 | Returns: the total since start-up
 ----------------------*/
unsigned long long pvr_backend_wait_us(void);

/*----------------------
 | pvr_backend_take_render_max_us
 | Description: The longest PowerVR render the KallistiOS backend has seen since the last call, then starts a new period, for the speed readout.
 | Author: suinevere
 | Dependencies: pvr_backend_kos.c
 | Globals: s_render_max_us
 | Params: N/A
 | Returns: microseconds
 ----------------------*/
unsigned pvr_backend_take_render_max_us(void);

/*----------------------
 | pvr_backend_take_flip_expired
 | Description: How many texture or palette writes went ahead since the last call because the previous scene had not reached the screen in time, then starts a new period, for the speed readout.
 | Author: suinevere
 | Dependencies: pvr_backend_kos.c
 | Globals: s_flip_expired
 | Params: N/A
 | Returns: count
 ----------------------*/
unsigned pvr_backend_take_flip_expired(void);

/*----------------------
 | pvr_backend_frame_pixels
 | Description: Returns the displayed 640x480 RGB565 buffer and its pitch, for reading rendered PS1 pixels back into the VRAM mirror; a host test fakes it with a test buffer.
 | Author: suinevere
 | Dependencies: pvr_backend_kos.c (real), host test (fake)
 | Globals: N/A
 | Params: pitch_px -- output, pixels per row
 | Returns: pointer to the displayed buffer, or null if none is available
 ----------------------*/
const uint16_t *pvr_backend_frame_pixels(unsigned *pitch_px);

#endif
