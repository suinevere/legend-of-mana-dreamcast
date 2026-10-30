/*----------------------
 | texture_cache.h
 | Description: Caches PS1 texture pages from the VRAM mirror as twiddled PowerVR textures, paired with palette slots or expanded to ARGB1555 when palette RAM is full.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_TEXTURE_CACHE_H
#define LOM_TEXTURE_CACHE_H

#include <stdint.h>

/*----------------------
 | TextureOps
 | Description: Backend for texture memory: allocate, free everything at once, copy a built texture in once no submitted scene can read the old bytes, write without that wait into memory no submitted scene reads (NULL falls back to upload), free one allocation (NULL keeps it until the next flush),
 |              and write into memory only scenes before the last closed one read, once those reached the screen (NULL falls back to upload); host tests fake it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct {
    void *(*alloc)(unsigned bytes);
    void (*free_all)(void);
    void (*upload)(void *dst, const void *src, unsigned bytes);
    void (*write)(void *dst, const void *src, unsigned bytes);
    void (*free)(void *ptr);
    void (*write_old)(void *dst, const void *src, unsigned bytes);
} TextureOps;

/*----------------------
 | DcSurfaceTexture
 | Description: A lookup result: the texture in texture memory, its TEX_FMT_* format and its palette slot, -1 for ARGB1555.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct { void *texture; int format; int palette_slot; } DcSurfaceTexture;

/*----------------------
 | TEX_FMT_PAL4, TEX_FMT_PAL8, TEX_FMT_ARGB1555, TEX_FMT_RGB565_FRAME
 | Description: Formats of a cached 256x256 twiddled texture: 4bpp or 8bpp palette indices, or direct ARGB1555; and the 512x256 non-twiddled RGB565 frame a 24-bit display area is shown from.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { TEX_FMT_PAL4 = 0, TEX_FMT_PAL8 = 1, TEX_FMT_ARGB1555 = 2, TEX_FMT_RGB565_FRAME = 3 };

void texture_cache_init(const TextureOps *ops);
int texture_cache_lookup(int tpage_x, int tpage_y, int clut_x, int clut_y, int depth, int variant, unsigned frame, DcSurfaceTexture *out);
unsigned texture_cache_flushes(void);
unsigned texture_cache_expansions(void);
unsigned texture_cache_skips(void);
unsigned texture_cache_builds(void);
unsigned long long texture_cache_build_us(void);
unsigned long long texture_cache_upload_us(void);
int texture_cache_flush_pending(void);
void texture_cache_begin_frame(void);
void texture_cache_flush_now(void);
uint32_t texture_twiddle_index(unsigned x, unsigned y);
void texture_read_page(int tpage_x, int tpage_y, int depth, uint8_t *out_indices, uint16_t *out_direct);

#endif
