/*----------------------
 | frame_readback.h
 | Description: Converts 2x point-sampled RGB565 framebuffer pixels back to PS1 15-bit colour for a VRAM mirror rectangle.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_FRAME_READBACK_H
#define LOM_FRAME_READBACK_H

#include <stdint.h>

/*----------------------
 | frame_readback_copy
 | Description: Reads one PS1 VRAM rectangle back from a 2x point-sampled RGB565 framebuffer, converting each sampled pixel to a PS1 15-bit colour with bit 15 clear.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: rgb565 -- framebuffer pixels, pitch_px -- pixels per framebuffer row, fx, fy -- VRAM coordinate the framebuffer's (0,0) corresponds to, mirror -- destination buffer sized w*h, x, y, w, h -- VRAM rectangle to read
 | Returns: N/A
 ----------------------*/
void frame_readback_copy(const uint16_t *rgb565, unsigned pitch_px, int fx, int fy,
                          uint16_t *mirror, int x, int y, int w, int h);

#endif
