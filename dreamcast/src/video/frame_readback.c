/*----------------------
 | frame_readback.c
 | Description: Pure conversion of 2x point-sampled RGB565 framebuffer pixels back to PS1 15-bit colour for a VRAM mirror rectangle.
 | Author: suinevere
 | Dependencies: frame_readback.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "frame_readback.h"

/*----------------------
 | frame_readback_copy
 | Description: Reads one PS1 VRAM rectangle back from a 2x point-sampled RGB565 framebuffer, converting each sampled pixel to a PS1 15-bit colour with bit 15 clear.
 | Expanding and truncating a 5-bit channel is the identity, and doing so for 6 bits is a right shift by one, so no division is needed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: rgb565 -- framebuffer pixels, pitch_px -- pixels per framebuffer row, fx, fy -- VRAM coordinate the framebuffer's (0,0) corresponds to, mirror -- destination buffer sized w*h, x, y, w, h -- VRAM rectangle to read
 | Returns: N/A
 ----------------------*/
void frame_readback_copy(const uint16_t *rgb565, unsigned pitch_px, int fx, int fy,
                          uint16_t *mirror, int x, int y, int w, int h)
{
    int row;
    int col;

    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            int sx = 2 * (x + col - fx);
            int sy = 2 * (y + row - fy);
            uint16_t src = rgb565[(unsigned)sy * pitch_px + (unsigned)sx];
            unsigned r5 = (src >> 11) & 0x1Fu;
            unsigned g5 = (src >> 6) & 0x1Fu;
            unsigned b5 = src & 0x1Fu;

            mirror[row * w + col] = (uint16_t)(r5 | (g5 << 5) | (b5 << 10));
        }
    }
}
