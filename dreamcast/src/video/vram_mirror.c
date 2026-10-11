/*----------------------
 | vram_mirror.c
 | Description: Keeps a 1024x512 16-bit mirror of PS1 VRAM so LoadImage, StoreImage, MoveImage and ClearImage behave the same without a GPU; coordinates wrap like the PS1.
 | Author: suinevere
 | Dependencies: vram_mirror.h
 | Globals: s_mirror
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "vram_mirror.h"

/*----------------------
 | VRAM_MIRROR_WIDTH
 | Description: Mirror width in 16-bit pixels, matching the PS1's 1024-word VRAM line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VRAM_MIRROR_WIDTH 1024

/*----------------------
 | VRAM_MIRROR_HEIGHT
 | Description: Mirror height in 16-bit pixels, matching the PS1's 512-line VRAM.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VRAM_MIRROR_HEIGHT 512

/*----------------------
 | s_mirror
 | Description: The 1 MB 16-bit VRAM mirror, row-major, VRAM_MIRROR_WIDTH pixels per row.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned short s_mirror[VRAM_MIRROR_WIDTH * VRAM_MIRROR_HEIGHT];

/*----------------------
 | wrap_coord
 | Description: Wraps a coordinate into [0, mod), the way PS1 VRAM addressing wraps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- coordinate, mod -- VRAM_MIRROR_WIDTH or VRAM_MIRROR_HEIGHT
 | Returns: wrapped coordinate
 ----------------------*/
static int wrap_coord(int v, int mod)
{
    int r = v % mod;

    return r < 0 ? r + mod : r;
}

/*----------------------
 | mirror_at
 | Description: Resolves a wrapped (x, y) pixel to its mutable slot in the mirror.
 | Author: suinevere
 | Dependencies: wrap_coord
 | Globals: s_mirror
 | Params: x, y -- pixel coordinates, wrapped on use
 | Returns: mutable pointer to the pixel
 ----------------------*/
static unsigned short *mirror_at(int x, int y)
{
    return &s_mirror[wrap_coord(y, VRAM_MIRROR_HEIGHT) * VRAM_MIRROR_WIDTH + wrap_coord(x, VRAM_MIRROR_WIDTH)];
}

/*----------------------
 | vram_mirror_at
 | Description: Resolves a wrapped (x, y) pixel to its slot in the mirror, for read-only inspection.
 | Author: suinevere
 | Dependencies: mirror_at
 | Globals: s_mirror
 | Params: x, y -- pixel coordinates, wrapped on use
 | Returns: pointer to the pixel
 ----------------------*/
const unsigned short *vram_mirror_at(int x, int y)
{
    return mirror_at(x, y);
}

/*----------------------
 | vram_mirror_load
 | Description: Copies rect->w by rect->h 16-bit pixels from pixels into the mirror at rect's wrapped position, matching LoadImage.
 | Author: suinevere
 | Dependencies: mirror_at
 | Globals: s_mirror
 | Params: rect -- destination rectangle in the mirror, pixels -- rect->w * rect->h source pixels
 | Returns: N/A
 ----------------------*/
void vram_mirror_load(const RECT *rect, const u_long *pixels)
{
    const unsigned short *src = (const unsigned short *)pixels;
    int row;
    int col;

    for (row = 0; row < rect->h; row++)
    {
        for (col = 0; col < rect->w; col++)
        {
            *mirror_at(rect->x + col, rect->y + row) = src[row * rect->w + col];
        }
    }
}

/*----------------------
 | vram_mirror_store
 | Description: Copies rect->w by rect->h 16-bit pixels from the mirror at rect's wrapped position into pixels, matching StoreImage.
 | Author: suinevere
 | Dependencies: mirror_at
 | Globals: s_mirror
 | Params: rect -- source rectangle in the mirror, pixels -- rect->w * rect->h destination pixels
 | Returns: N/A
 ----------------------*/
void vram_mirror_store(const RECT *rect, u_long *pixels)
{
    unsigned short *dst = (unsigned short *)pixels;
    int row;
    int col;

    for (row = 0; row < rect->h; row++)
    {
        for (col = 0; col < rect->w; col++)
        {
            dst[row * rect->w + col] = *mirror_at(rect->x + col, rect->y + row);
        }
    }
}

/*----------------------
 | vram_mirror_move
 | Description: Copies rect's pixels to a destination at (x, y) in the mirror, buffering one row at a time so an overlapping source and destination still copy correctly, matching MoveImage.
 | Author: suinevere
 | Dependencies: mirror_at
 | Globals: s_mirror
 | Params: rect -- source rectangle, x, y -- destination origin
 | Returns: N/A
 ----------------------*/
void vram_mirror_move(const RECT *rect, int x, int y)
{
    unsigned short row_buf[VRAM_MIRROR_WIDTH];
    int row;
    int col;

    if (y <= rect->y)
    {
        for (row = 0; row < rect->h; row++)
        {
            for (col = 0; col < rect->w; col++)
            {
                row_buf[col] = *mirror_at(rect->x + col, rect->y + row);
            }
            for (col = 0; col < rect->w; col++)
            {
                *mirror_at(x + col, y + row) = row_buf[col];
            }
        }
    }
    else
    {
        for (row = rect->h - 1; row >= 0; row--)
        {
            for (col = 0; col < rect->w; col++)
            {
                row_buf[col] = *mirror_at(rect->x + col, rect->y + row);
            }
            for (col = 0; col < rect->w; col++)
            {
                *mirror_at(x + col, y + row) = row_buf[col];
            }
        }
    }
}

/*----------------------
 | vram_mirror_clear
 | Description: Fills rect in the mirror with the PS1 15-bit colour packed from r, g, b, matching ClearImage.
 | Author: suinevere
 | Dependencies: mirror_at
 | Globals: s_mirror
 | Params: rect -- area to fill, r, g, b -- 8-bit colour components
 | Returns: N/A
 ----------------------*/
void vram_mirror_clear(const RECT *rect, int r, int g, int b)
{
    unsigned short colour = (unsigned short)(((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3));
    int row;
    int col;

    for (row = 0; row < rect->h; row++)
    {
        for (col = 0; col < rect->w; col++)
        {
            *mirror_at(rect->x + col, rect->y + row) = colour;
        }
    }
}
