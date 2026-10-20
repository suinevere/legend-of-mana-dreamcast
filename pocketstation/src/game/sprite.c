/*----------------------
 | sprite.c
 | Description: Ring Ring Land's sprite tables (descriptors at 0x0200A03C, frame sets at 0x0200A384) and the sprite blitter that draws into the frame buffer.
 | Author: suinevere
 | Dependencies: game.h, psk_cpu.h
 | Globals: fb, blit_a, blit_b
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "psk_cpu.h"

/*----------------------
 | ROM_SPRITES
 | Description: Sprite descriptors: a packed word (x offset bits 0-5, flags 6-7, y offset 8-15, width 16-23, height 24-31) and a bitmap address.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_SPRITES 0x0200A03Cu

/*----------------------
 | ROM_FRAME_SETS
 | Description: Frame set addresses by object type, from type 0x58.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_FRAME_SETS 0x0200A384u

/*----------------------
 | game_sprite_info
 | Description: Reads sprite id's descriptor and offsets the position by it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x, y -- position; id -- sprite, past 0xFF for a free anim slot as on the ARM
 | Returns: the placed sprite
 ----------------------*/
SpriteInfo game_sprite_info(int16_t x, int16_t y, uint16_t id) {
    SpriteInfo s;
    uint32_t w = psk_ldr(ROM_SPRITES + id * 8u);
    s.x = (int16_t)(x + (w & 0x3Fu));
    s.y = (int16_t)(y + ((w >> 8) & 0xFFu));
    s.width = (uint8_t)(w >> 16);
    s.height = (uint8_t)(w >> 24);
    s.flags = (uint8_t)((w >> 6) & 3u);
    s.bitmap = psk_ldr(ROM_SPRITES + id * 8u + 4u);
    return s;
}

/*----------------------
 | game_frame_info
 | Description: Looks up frame of an object type's frame set; kind 0x80 sets carry offsets and two words, 0x81 sets skip a leading word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: type -- object type (0x58 up); frame -- frame number
 | Returns: the frame's offsets and words
 ----------------------*/
FrameInfo game_frame_info(uint8_t type, uint8_t frame) {
    FrameInfo f;
    uint32_t set = psk_ldr(ROM_FRAME_SETS + (uint32_t)(type - 0x58) * 4u);
    uint32_t head = psk_ldr(set), w;
    uint8_t kind = (uint8_t)head;
    if (kind == 0x80u) {
        uint32_t e = set + frame * 8u;
        w = psk_ldr(e + 4u);
        f.word = psk_ldr(e + 8u);
        f.dx = (int8_t)(w >> 24);
        f.dy = (int8_t)(f.word >> 24);
        f.bitmap = w & 0x7FFFFFFFu;
        f.extra = head >> 8;
        return f;
    }
    f.extra = kind == 0x81u ? head >> 8 : 0;
    w = psk_ldr(set + (uint32_t)(frame + (kind == 0x81u)) * 4u);
    f.bitmap = w;
    f.dx = (int8_t)((w << 1) >> 25);
    f.dy = 0;
    f.word = 0;
    return f;
}

/*----------------------
 | shift_left
 | Description: ARM register-specified LSL: amounts of 32 and above give zero.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value; n -- low byte of the shift register
 | Returns: the shifted value
 ----------------------*/
static uint32_t shift_left(uint32_t v, uint8_t n) {
    return n >= 32u ? 0u : v << n;
}

/*----------------------
 | shift_right
 | Description: ARM register-specified LSR: amounts of 32 and above give zero.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value; n -- low byte of the shift register
 | Returns: the shifted value
 ----------------------*/
static uint32_t shift_right(uint32_t v, uint8_t n) {
    return n >= 32u ? 0u : v >> n;
}

/*----------------------
 | mirror_row
 | Description: Reverses the low width bits of a row.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- row bits; width -- bits in the row
 | Returns: the mirrored row
 ----------------------*/
static uint32_t mirror_row(uint32_t v, uint8_t width) {
    uint32_t out = 0, bit = shift_left(1u, (uint8_t)(width - 1u));
    for (uint8_t i = 0; i < width; i++) {
        if (v & 1u) out |= bit;
        v >>= 1;
        bit >>= 1;
    }
    return out;
}

/*----------------------
 | unpack_rows
 | Description: Unpacks height rows of width bits, packed LSB first across 32-bit words from src, into blit_a or blit_b, one word per row, mirrored when asked. Height 0 is unreached, converted from the listing, and differs: 256 rows here, an endless loop on the ARM.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: blit_a, blit_b
 | Params: src -- packed bitmap; width -- bits per row; height -- rows; mirror -- nonzero to mirror; to_mask -- nonzero for blit_b
 | Returns: the address after the last word read
 ----------------------*/
static uint32_t unpack_rows(uint32_t src, uint8_t width, uint8_t height, uint8_t mirror, uint8_t to_mask) {
    uint32_t keep = ~shift_left(0xFFFFFFFFu, width), acc = 0, row;
    uint16_t have = 0;
    uint8_t i = 0;
    do {
        if (have < width) {
            uint32_t w = psk_ldr(src);
            src += 4u;
            row = acc | shift_left(w, (uint8_t)have);
            acc = shift_right(w, (uint8_t)(width - have));
            have = (uint16_t)(have + 32u);
        } else {
            row = acc;
            acc = shift_right(acc, width);
        }
        row &= keep;
        if (mirror) row = mirror_row(row, width);
        if (to_mask) set_blit_b(i, row); else set_blit_a(i, row);
        have = (uint16_t)(have - width);
        i++;
    } while (--height);
    return src;
}

/*----------------------
 | plot_rows
 | Description: Clips to rows 0-31, shifts rows by x (left when x >= 0) and merges them into fb by OR, mask-clear then OR, or box-clear then OR. Capping at 32 rows from above, box-clear with x < 0 and height 0 (256 rows here, endless on the ARM) are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: fb, blit_a, blit_b
 | Params: x, y -- position; width, height -- size; bitmap -- packed rows (mask after them) when not unpacked; flags -- SPRITE_*; unpacked -- rows are in blit_a/blit_b
 | Returns: N/A
 ----------------------*/
static void plot_rows(int16_t x, int16_t y, uint8_t width, uint8_t height, uint32_t bitmap, uint8_t flags, uint8_t unpacked) {
    uint8_t row = 0, skip = 0, count, n = (uint8_t)(x < 0 ? -x : x);
    if (y >= 0) {
        if (y >= 32) return;
        row = (uint8_t)y;
        count = (uint8_t)(32 - y) < height ? (uint8_t)(32 - y) : height;
    } else {
        if (height <= -y) return;
        skip = (uint8_t)-y;
        count = height + y < 32 ? (uint8_t)(height + y) : 32u;
    }
    uint32_t box = ~shift_left(0xFFFFFFFFu, width);
    uint32_t clear = ~(x >= 0 ? shift_left(box, n) : shift_right(box, n));
    do {
        uint32_t img = unpacked ? ram_blit_a(skip) : psk_ldr(bitmap + (uint32_t)skip * 4u);
        uint32_t out = ram_fb(row);
        img = x >= 0 ? shift_left(img, n) : shift_right(img, n);
        if (flags & SPRITE_MASKED) {
            uint32_t m = unpacked ? ram_blit_b(skip) : psk_ldr(bitmap + ((uint32_t)height + skip) * 4u);
            m = x >= 0 ? shift_left(m, n) : shift_right(m, n);
            out = (out & ~m) | img;
        } else if (unpacked && (flags & SPRITE_CLEAR)) {
            out = (out & clear) | img;
        } else {
            out |= img;
        }
        set_fb(row, out);
        row++;
        skip++;
    } while (--count);
}

/*----------------------
 | game_sprite_draw
 | Description: Draws a sprite into fb, rows being 32-bit words as in the original; sprites not 32 wide or mirrored are unpacked into blit_a (mask blit_b) first. Mirroring a 32-wide sprite and SPRITE_CLEAR on packed sprites (ignored) are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: fb, blit_a, blit_b
 | Params: x, y -- position; width -- 1..32 bits per row; height -- rows (nonzero); bitmap -- image rows, then mask rows when masked; flags -- SPRITE_MASKED, SPRITE_CLEAR, SPRITE_MIRROR
 | Returns: N/A
 ----------------------*/
void game_sprite_draw(int16_t x, int16_t y, uint8_t width, uint8_t height, uint32_t bitmap, uint8_t flags) {
    uint8_t unpacked = width != 32u || (flags & SPRITE_MIRROR);
    if (unpacked) {
        uint32_t next = unpack_rows(bitmap, width, height, flags & SPRITE_MIRROR, 0);
        if (flags & SPRITE_MASKED) unpack_rows(next, width, height, flags & SPRITE_MIRROR, 1);
    }
    plot_rows(x, y, width, height, bitmap, flags, unpacked);
}
