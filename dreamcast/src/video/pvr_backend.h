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
 | PvrBackend
 | Description: Starts and finishes a scene, sets the background, and writes a polygon header or one strip vertex in screen pixels with page-normalised texel coordinates.
 | scene_begin returns 0 when the previous flip is not yet confirmed, leaving a pending texture flush for later; scene_shown checks the same without waiting.
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
    void (*vertex)(float x, float y, float u, float v, uint32_t argb, int last);
    int (*scene_shown)(unsigned scenes_closed);
} PvrBackend;

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
