/*----------------------
 | vram_mirror.c
 | Description: Keeps a 1024x512 16-bit mirror of PS1 VRAM so LoadImage, StoreImage, MoveImage and ClearImage behave the same without a GPU; coordinates wrap like the PS1.
 | Author: suinevere
 | Dependencies: vram_mirror.h
 | Globals: s_mirror, s_block_gen, s_gen_counter, s_row_buf, s_cycle_buf, s_row_done
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
 | VRAM_BLOCK_WIDTH
 | Description: Width in 16-bit words of one generation block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VRAM_BLOCK_WIDTH 64

/*----------------------
 | VRAM_BLOCK_HEIGHT
 | Description: Height in rows of one generation block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VRAM_BLOCK_HEIGHT 16

/*----------------------
 | VRAM_BLOCKS_X
 | Description: Generation blocks across one mirror row.
 | Author: suinevere
 | Dependencies: VRAM_MIRROR_WIDTH, VRAM_BLOCK_WIDTH
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VRAM_BLOCKS_X (VRAM_MIRROR_WIDTH / VRAM_BLOCK_WIDTH)

/*----------------------
 | VRAM_BLOCKS_Y
 | Description: Generation blocks down the mirror.
 | Author: suinevere
 | Dependencies: VRAM_MIRROR_HEIGHT, VRAM_BLOCK_HEIGHT
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VRAM_BLOCKS_Y (VRAM_MIRROR_HEIGHT / VRAM_BLOCK_HEIGHT)

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
 | s_block_gen
 | Description: Per-block value of s_gen_counter at the block's last write; 0 means never written.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint32_t s_block_gen[VRAM_BLOCKS_Y][VRAM_BLOCKS_X];

/*----------------------
 | s_gen_counter
 | Description: Global write counter, bumped once per load, move or clear.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint32_t s_gen_counter;

/*----------------------
 | s_row_buf
 | Description: One mirror row of scratch so a row copy reads its whole source before writing.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned short s_row_buf[VRAM_MIRROR_WIDTH];

/*----------------------
 | s_cycle_buf
 | Description: Holds the source row that breaks a closed cycle of rows, the one case a chain walk cannot order.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned short s_cycle_buf[VRAM_MIRROR_WIDTH];

/*----------------------
 | s_row_done
 | Description: Marks destination row indices already written by the current move.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_row_done[VRAM_MIRROR_HEIGHT];

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
 | blocks_visit
 | Description: Walks the generation blocks under a wrapped rect, stamping each with stamp, or when stamp is 0 finding the newest block value.
 | Author: suinevere
 | Dependencies: wrap_coord
 | Globals: s_block_gen
 | Params: x, y, w, h -- rect in mirror pixels, stamp -- value to write, or 0 to read
 | Returns: largest block value seen when reading, else 0
 ----------------------*/
static uint32_t blocks_visit(int x, int y, int w, int h, uint32_t stamp)
{
    uint32_t newest = 0;
    int rows_done;
    int cols_done;

    if (w <= 0 || h <= 0)
    {
        return 0;
    }
    if (w > VRAM_MIRROR_WIDTH)
    {
        w = VRAM_MIRROR_WIDTH;
    }
    if (h > VRAM_MIRROR_HEIGHT)
    {
        h = VRAM_MIRROR_HEIGHT;
    }
    for (rows_done = 0; rows_done < h;)
    {
        int wy = wrap_coord(y + rows_done, VRAM_MIRROR_HEIGHT);

        for (cols_done = 0; cols_done < w;)
        {
            int wx = wrap_coord(x + cols_done, VRAM_MIRROR_WIDTH);
            uint32_t *block = &s_block_gen[wy / VRAM_BLOCK_HEIGHT][wx / VRAM_BLOCK_WIDTH];

            if (stamp != 0)
            {
                *block = stamp;
            }
            else if (*block > newest)
            {
                newest = *block;
            }
            cols_done += VRAM_BLOCK_WIDTH - (wx % VRAM_BLOCK_WIDTH);
        }
        rows_done += VRAM_BLOCK_HEIGHT - (wy % VRAM_BLOCK_HEIGHT);
    }
    return newest;
}

/*----------------------
 | blocks_bump
 | Description: Advances the global write counter and stamps every block under the wrapped rect with it.
 | Author: suinevere
 | Dependencies: blocks_visit
 | Globals: s_gen_counter, s_block_gen
 | Params: x, y, w, h -- written rect in mirror pixels
 | Returns: N/A
 ----------------------*/
static void blocks_bump(int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    s_gen_counter++;
    blocks_visit(x, y, w, h, s_gen_counter);
}

/*----------------------
 | vram_mirror_generation
 | Description: Returns the newest write generation of any 64x16 block under the wrapped rect, so callers can tell whether its contents changed.
 | Author: suinevere
 | Dependencies: blocks_visit
 | Globals: s_block_gen
 | Params: x, y, w, h -- rect in mirror pixels, wrapped like PS1 VRAM
 | Returns: largest block generation under the rect, 0 if never written
 ----------------------*/
uint32_t vram_mirror_generation(int x, int y, int w, int h)
{
    return blocks_visit(x, y, w, h, 0);
}

/*----------------------
 | vram_mirror_write_count
 | Description: Returns the global write counter, which moves whenever any mirror pixel changes, so callers can cache results until VRAM changes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_gen_counter
 | Params: N/A
 | Returns: current write counter
 ----------------------*/
uint32_t vram_mirror_write_count(void)
{
    return s_gen_counter;
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
 | Description: Copies rect->w by rect->h 16-bit pixels from pixels into the mirror at rect's wrapped position, matching LoadImage; only a load that changes a pixel bumps the generations.
 | Author: suinevere
 | Dependencies: mirror_at, blocks_bump
 | Globals: s_mirror, s_block_gen, s_gen_counter
 | Params: rect -- destination rectangle in the mirror, pixels -- rect->w * rect->h source pixels
 | Returns: N/A
 ----------------------*/
void vram_mirror_load(const RECT *rect, const u_long *pixels)
{
    const unsigned short *src = (const unsigned short *)pixels;
    int changed = 0;
    int row;
    int col;

    for (row = 0; row < rect->h; row++)
    {
        for (col = 0; col < rect->w; col++)
        {
            unsigned short *dst = mirror_at(rect->x + col, rect->y + row);

            changed |= *dst != src[row * rect->w + col];
            *dst = src[row * rect->w + col];
        }
    }
    if (changed)
    {
        blocks_bump(rect->x, rect->y, rect->w, rect->h);
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
 | copy_row
 | Description: Copies w pixels of one wrapped mirror row to another through s_row_buf, so horizontal overlap is safe.
 | Author: suinevere
 | Dependencies: mirror_at
 | Globals: s_mirror, s_row_buf
 | Params: sx, sy -- source start, dx, dy -- destination start, w -- pixels, at most VRAM_MIRROR_WIDTH
 | Returns: N/A
 ----------------------*/
static void copy_row(int sx, int sy, int dx, int dy, int w)
{
    int col;

    for (col = 0; col < w; col++)
    {
        s_row_buf[col] = *mirror_at(sx + col, sy);
    }
    for (col = 0; col < w; col++)
    {
        *mirror_at(dx + col, dy) = s_row_buf[col];
    }
}

/*----------------------
 | move_cycle
 | Description: Moves the closed cycle of destination rows through start, whose sources are all destination rows, by saving start's source row in s_cycle_buf first.
 | Author: suinevere
 | Dependencies: copy_row, wrap_coord, mirror_at
 | Globals: s_mirror, s_row_buf, s_cycle_buf, s_row_done
 | Params: rect -- source rectangle, x, y -- destination origin, w -- clamped width, shift -- wrapped y - rect->y, start -- first destination row index of the cycle
 | Returns: N/A
 ----------------------*/
static void move_cycle(const RECT *rect, int x, int y, int w, int shift, int start)
{
    int cur = start;
    int col;

    for (col = 0; col < w; col++)
    {
        s_cycle_buf[col] = *mirror_at(rect->x + col, y + start);
    }
    for (;;)
    {
        int next = wrap_coord(cur - shift, VRAM_MIRROR_HEIGHT);

        s_row_done[cur] = 1;
        if (next == start)
        {
            for (col = 0; col < w; col++)
            {
                *mirror_at(x + col, y + cur) = s_cycle_buf[col];
            }
            return;
        }
        copy_row(rect->x, rect->y + cur, x, y + cur, w);
        cur = next;
    }
}

/*----------------------
 | vram_mirror_move
 | Description: Copies rect's pixels to (x, y) as if through a temporary, matching MoveImage; rows are walked along wrapped overlap chains, then any closed row cycles are broken through s_cycle_buf.
 | Author: suinevere
 | Dependencies: copy_row, move_cycle, wrap_coord, blocks_bump
 | Globals: s_mirror, s_row_buf, s_cycle_buf, s_row_done, s_block_gen, s_gen_counter
 | Params: rect -- source rectangle, x, y -- destination origin
 | Returns: N/A
 ----------------------*/
void vram_mirror_move(const RECT *rect, int x, int y)
{
    int w = rect->w < VRAM_MIRROR_WIDTH ? rect->w : VRAM_MIRROR_WIDTH;
    int h = rect->h < VRAM_MIRROR_HEIGHT ? rect->h : VRAM_MIRROR_HEIGHT;
    int shift;
    int row;

    if (w <= 0 || h <= 0)
    {
        return;
    }
    blocks_bump(x, y, w, h);
    shift = wrap_coord(y - rect->y, VRAM_MIRROR_HEIGHT);
    if (shift == 0)
    {
        for (row = 0; row < h; row++)
        {
            copy_row(rect->x, rect->y + row, x, y + row, w);
        }
        return;
    }
    for (row = 0; row < h; row++)
    {
        s_row_done[row] = 0;
    }
    for (row = 0; row < h; row++)
    {
        int cur = row;

        if (wrap_coord(row + shift, VRAM_MIRROR_HEIGHT) < h)
        {
            continue;
        }
        while (cur < h)
        {
            copy_row(rect->x, rect->y + cur, x, y + cur, w);
            s_row_done[cur] = 1;
            cur = wrap_coord(cur - shift, VRAM_MIRROR_HEIGHT);
        }
    }
    for (row = 0; row < h; row++)
    {
        if (!s_row_done[row])
        {
            move_cycle(rect, x, y, w, shift, row);
        }
    }
}

/*----------------------
 | vram_mirror_clear
 | Description: Fills rect in the mirror with the PS1 15-bit colour packed from r, g, b, matching ClearImage.
 | Author: suinevere
 | Dependencies: mirror_at, blocks_bump
 | Globals: s_mirror, s_block_gen, s_gen_counter
 | Params: rect -- area to fill, r, g, b -- 8-bit colour components
 | Returns: N/A
 ----------------------*/
void vram_mirror_clear(const RECT *rect, int r, int g, int b)
{
    unsigned short colour = (unsigned short)(((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3));
    int row;
    int col;

    blocks_bump(rect->x, rect->y, rect->w, rect->h);
    for (row = 0; row < rect->h; row++)
    {
        for (col = 0; col < rect->w; col++)
        {
            *mirror_at(rect->x + col, rect->y + row) = colour;
        }
    }
}

/*----------------------
 | vram_mirror_fill
 | Description: Fills a w by h rect at (x, y) with an already-packed 15-bit colour, matching GP0 0x02 fill rectangle, and bumps the overwritten blocks' generations.
 | Author: suinevere
 | Dependencies: mirror_at, blocks_bump
 | Globals: s_mirror, s_block_gen, s_gen_counter
 | Params: x, y, w, h -- rect in mirror pixels, colour -- packed 15-bit PS1 colour
 | Returns: N/A
 ----------------------*/
void vram_mirror_fill(int x, int y, int w, int h, uint16_t colour)
{
    int row;
    int col;

    blocks_bump(x, y, w, h);
    for (row = 0; row < h; row++)
    {
        for (col = 0; col < w; col++)
        {
            *mirror_at(x + col, y + row) = colour;
        }
    }
}

/*----------------------
 | vram_mirror_invalidate
 | Description: Overwrites a w by h rect at (x, y) with pixels and bumps the overwritten blocks' generations, skipping wraparound since a PS1 draw-area rect is always in VRAM bounds.
 | Author: suinevere
 | Dependencies: blocks_bump
 | Globals: s_mirror, s_block_gen, s_gen_counter
 | Params: x, y, w, h -- rect already inside [0, VRAM_MIRROR_WIDTH) and [0, VRAM_MIRROR_HEIGHT), pixels -- w*h source pixels, row-major
 | Returns: N/A
 ----------------------*/
void vram_mirror_invalidate(int x, int y, int w, int h, const unsigned short *pixels)
{
    int row;
    int col;

    blocks_bump(x, y, w, h);
    for (row = 0; row < h; row++)
    {
        unsigned short *dst = &s_mirror[(y + row) * VRAM_MIRROR_WIDTH + x];
        const unsigned short *src = &pixels[row * w];

        for (col = 0; col < w; col++)
        {
            dst[col] = src[col];
        }
    }
}
