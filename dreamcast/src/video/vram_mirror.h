/*----------------------
 | vram_mirror.h
 | Description: A 1024x512 16-bit mirror of PS1 VRAM for the Dreamcast libgpu backend, with PS1-style coordinate wraparound and per-block write generations.
 | Author: suinevere
 | Dependencies: stdint.h, lom_prefix.h (RECT, u_long)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_VRAM_MIRROR_H
#define LOM_VRAM_MIRROR_H

#include <stdint.h>

void vram_mirror_load(const RECT *rect, const u_long *pixels);
void vram_mirror_store(const RECT *rect, u_long *pixels);
void vram_mirror_move(const RECT *rect, int x, int y);
void vram_mirror_clear(const RECT *rect, int r, int g, int b);
void vram_mirror_fill(int x, int y, int w, int h, uint16_t colour);
void vram_mirror_invalidate(int x, int y, int w, int h, const unsigned short *pixels);
const unsigned short *vram_mirror_at(int x, int y);
uint32_t vram_mirror_generation(int x, int y, int w, int h);
uint32_t vram_mirror_write_count(void);

#endif
