/*----------------------
 | rgb24_display.h
 | Description: Converts a PS1 24-bit display area in the VRAM mirror to RGB565 rows for a PowerVR texture.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_RGB24_DISPLAY_H
#define LOM_RGB24_DISPLAY_H

#include <stdint.h>

/*----------------------
 | rgb24_to_rgb565
 | Description: Reads R, G, B bytes per pixel from the halfword mirror starting at halfword column x_hw, wrapping at the mirror's edges, and writes RGB565.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: vram -- 1024x512 halfword mirror, vram_pitch_hw -- its row pitch, x_hw, y -- top-left in halfwords and rows, width_px, height -- pixels to convert, dst -- output, dst_pitch_px -- output row pitch
 | Returns: N/A
 ----------------------*/
void rgb24_to_rgb565(const uint16_t *vram, unsigned vram_pitch_hw, unsigned x_hw, unsigned y, unsigned width_px, unsigned height,
                     uint16_t *dst, unsigned dst_pitch_px);

#endif
