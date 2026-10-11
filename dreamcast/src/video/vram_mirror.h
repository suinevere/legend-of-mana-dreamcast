/*----------------------
 | vram_mirror.h
 | Description: A 1024x512 16-bit mirror of PS1 VRAM for the Dreamcast counting libgpu backend, with PS1-style coordinate wraparound.
 | Author: suinevere
 | Dependencies: lom_prefix.h (RECT, u_long)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_VRAM_MIRROR_H
#define LOM_VRAM_MIRROR_H

void vram_mirror_load(const RECT *rect, const u_long *pixels);
void vram_mirror_store(const RECT *rect, u_long *pixels);
void vram_mirror_move(const RECT *rect, int x, int y);
void vram_mirror_clear(const RECT *rect, int r, int g, int b);
const unsigned short *vram_mirror_at(int x, int y);

#endif
