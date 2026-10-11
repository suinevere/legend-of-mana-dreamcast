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
 | Author: suinevere
 | Dependencies: DcSurfaceTexture, BlendPass
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    void (*scene_begin)(unsigned scenes_closed);
    void (*scene_finish)(void);
    void (*background)(uint8_t r, uint8_t g, uint8_t b);
    void (*header)(const DcSurfaceTexture *tex, const BlendPass *pass);
    void (*vertex)(float x, float y, float u, float v, uint32_t argb, int last);
} PvrBackend;

#endif
