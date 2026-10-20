/*----------------------
 | hud.c
 | Description: Ring Ring Land's drawn screen items: animation slots, item icons with their marks, the meter digit, numbers, small counts and wide pictures.
 | Author: suinevere
 | Dependencies: game.h, rrl_rom.h
 | Globals: anim, cycle16, meter, meter_shown, meter_delay, fb
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "rrl_rom.h"

/*----------------------
 | ANIM_SPRITES
 | Description: Sprite id of animation id 0; animation id n draws sprite ANIM_SPRITES + n.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ANIM_SPRITES 0x1Cu

/*----------------------
 | ICON_SPRITES, ICON_PLAIN_SPRITES, SPRITE_ICON_FULL, SPRITE_MARK
 | Description: Item icon backgrounds by type, their unmarked overlays by type, the four-mark overlay and the single mark sprite.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ICON_SPRITES 0x39u
#define ICON_PLAIN_SPRITES 0x3Du
#define SPRITE_ICON_FULL 0x41u
#define SPRITE_MARK 0x65u

/*----------------------
 | SPRITE_METER_LABEL, METER_DIGIT_X, DIGIT_BLANK
 | Description: The meter's label sprite, the meter digit's x and the digit sheet's blank entry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPRITE_METER_LABEL 0x38u
#define METER_DIGIT_X 0x1C
#define DIGIT_BLANK 10u

/*----------------------
 | NUMBER_PLACES
 | Description: Place values game_number_draw divides by, highest first.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint16_t NUMBER_PLACES[4] = {10000u, 1000u, 100u, 10u};

/*----------------------
 | SPRITE_SMALL_DIGITS, SMALL_DIGIT_W, SMALL_SLASH
 | Description: The 4-pixel digit sheet (digit d 4 * d bytes in), its digit step and the slash entry after 9.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPRITE_SMALL_DIGITS 0x36u
#define SMALL_DIGIT_W 4
#define SMALL_SLASH 10u

/*----------------------
 | WIDE_SPRITES, WIDE_COLUMN
 | Description: Sprite id of wide picture 0, and the column width such pictures are drawn in, which is also the screen width.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define WIDE_SPRITES 0x4Fu
#define WIDE_COLUMN 0x20

/*----------------------
 | sprite_put
 | Description: Draws a placed sprite.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: fb
 | Params: s -- the sprite
 | Returns: N/A
 ----------------------*/
static void sprite_put(SpriteInfo s) {
    game_sprite_draw(s.x, s.y, s.width, s.height, s.bitmap, s.flags);
}

/*----------------------
 | game_anim_draw
 | Description: Draws animation slot's sprite at its frame; ids 8 and up except 0x16 take slot byte 1 (bit 0 two frames on, bit 1 mirrored). Slots past 63 wrap the byte offset where the ARM stays 32-bit; unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: anim, fb
 | Params: x, y -- position; slot -- animation slot
 | Returns: the sprite as drawn
 ----------------------*/
SpriteInfo game_anim_draw(int16_t x, int16_t y, uint8_t slot) {
    uint8_t at = (uint8_t)(slot << 2), id = ram_anim(at), arg = 0;
    uint16_t frame = ram_anim((uint8_t)(at + 2u)), stride;
    SpriteInfo s;
    if (id >= 8u && id != 0x16u) {
        arg = ram_anim((uint8_t)(at + 1u));
        if (arg & 1u) {
            frame = (uint16_t)(frame + 2u);
        }
    }
    s = game_sprite_info(x, y, (uint16_t)(id + ANIM_SPRITES));
    if (arg & 2u) {
        s.flags |= SPRITE_MIRROR;
    }
    stride = (uint16_t)(((s.width * s.height + 31u) >> 5) << 3);
    s.bitmap += (uint32_t)stride * frame;
    sprite_put(s);
    return s;
}

/*----------------------
 | game_icon_draw
 | Description: Draws item icon type with an overlay: a plain one for no marks, a full one for four, else that many marks; overlays blink with cycle16. The 0, 3 and 4-mark paths, marks past 4 and types past 0xC2 (bytes where the ARM stays 32-bit) are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: psk_hw.h, rrl_ram.h, rrl_rom.h
 | Globals: cycle16, fb
 | Params: x, y -- position; type -- icon type; marks -- 0-4
 | Returns: the overlay as drawn
 ----------------------*/
SpriteInfo game_icon_draw(int16_t x, int16_t y, uint8_t type, uint8_t marks) {
    SpriteInfo s = game_sprite_info(x, y, (uint8_t)(type + ICON_SPRITES));
    uint32_t at = ROM_MARK_OFFSETS;
    sprite_put(s);
    if (marks == 4u) {
        s = game_sprite_info(x, y, SPRITE_ICON_FULL);
        if (ram_cycle16() >= 8u) {
            s.bitmap += 0x50u;
        }
        sprite_put(s);
        return s;
    }
    if (marks == 0u) {
        s = game_sprite_info(x, y, (uint8_t)(type + ICON_PLAIN_SPRITES));
        sprite_put(s);
        return s;
    }
    s = game_sprite_info(x, y, SPRITE_MARK);
    if (ram_cycle16() >= 8u) {
        s.bitmap += 0x20u;
    }
    if (marks == 2u) {
        at += 2u;
    } else if (marks == 3u) {
        at += 6u;
    }
    for (; marks != 0u; marks--) {
        uint16_t h = psk_r16(at);
        at += 2u;
        game_sprite_draw((int16_t)(s.x + (int8_t)h), (int16_t)(s.y + (int8_t)(h >> 8)), s.width, s.height, s.bitmap, s.flags);
    }
    return s;
}

/*----------------------
 | game_meter_draw
 | Description: Draws the meter's label and digit at row y; while the meter falls, label and digit blink, the digit between blank and meter_shown, or meter once meter_delay reaches 30. Showing the new value (meter_delay 30 and up) is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: meter, meter_shown, meter_delay, fb
 | Params: y -- label row; the digit sits 0x18 below
 | Returns: the digit as drawn
 ----------------------*/
SpriteInfo game_meter_draw(int16_t y) {
    uint8_t shown = ram_meter_shown(), meter = ram_meter(), digit;
    uint32_t phase;
    SpriteInfo s;
    game_divide(ram_meter_delay(), 20u, &phase);
    if (shown >= meter || phase >= 10u) {
        sprite_put(game_sprite_info(0, y, SPRITE_METER_LABEL));
    }
    s = game_sprite_info(METER_DIGIT_X, (int16_t)(y + 0x18), SPRITE_DIGITS);
    if (meter >= shown) {
        digit = shown;
    } else if (phase < 10u) {
        digit = DIGIT_BLANK;
    } else {
        digit = ram_meter_delay() < 30u ? shown : meter;
    }
    s.bitmap += digit * 4u;
    s.flags |= SPRITE_CLEAR;
    sprite_put(s);
    return s;
}

/*----------------------
 | game_number_draw
 | Description: Draws n left-aligned from x without leading zeros, one digit-sheet sprite per digit; n stays 32-bit as the ARM passes it. Numbers of three or more digits are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: fb
 | Params: x, y -- position; n -- the number
 | Returns: the units digit's bitmap address
 ----------------------*/
uint32_t game_number_draw(int16_t x, int16_t y, uint32_t n) {
    SpriteInfo s = game_sprite_info(x, y, SPRITE_DIGITS);
    uint32_t base = s.bitmap, rem;
    uint8_t i = 0;
    while (i < 4u && n < NUMBER_PLACES[i]) {
        i++;
    }
    for (; i < 4u; i++) {
        s.bitmap = base + game_divide(n, NUMBER_PLACES[i], &rem) * 4u;
        n = rem;
        sprite_put(s);
        s.x = (int16_t)(s.x + s.width);
    }
    s.bitmap = base + n * 4u;
    sprite_put(s);
    return s.bitmap;
}

/*----------------------
 | small_number_put
 | Description: Draws n with the small digit sheet right-aligned in a two-digit field from s.x, the tens digit only when n is 10 or more.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: fb
 | Params: s -- the digit sheet placed at the field; n -- the number
 | Returns: the units digit as drawn
 ----------------------*/
static SpriteInfo small_number_put(SpriteInfo s, uint32_t n) {
    uint32_t base = s.bitmap, units = n;
    if (n >= 10u) {
        s.bitmap = base + game_divide(n, 10u, &units) * 4u;
        sprite_put(s);
    }
    s.x = (int16_t)(s.x + SMALL_DIGIT_W);
    s.bitmap = base + units * 4u;
    sprite_put(s);
    return s;
}

/*----------------------
 | game_count_of_draw
 | Description: Draws "count/total" in small digits: count right-aligned in two digits, a slash, then total left-aligned. count stays 32-bit and total is narrowed to a byte (its one caller passes 50); a total below 10 is unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: fb
 | Params: x, y -- position; count -- the count; total -- the total
 | Returns: the slash as drawn
 ----------------------*/
SpriteInfo game_count_of_draw(int16_t x, int16_t y, uint32_t count, uint8_t total) {
    SpriteInfo s = game_sprite_info(x, y, SPRITE_SMALL_DIGITS);
    SpriteInfo units = small_number_put(s, count), t = s;
    t.x = (int16_t)(units.x + 2 * SMALL_DIGIT_W);
    if (total < 10u) {
        t.bitmap += total * 4u;
        sprite_put(t);
    } else {
        small_number_put(t, total);
    }
    s.x = (int16_t)(units.x + SMALL_DIGIT_W);
    s.bitmap += SMALL_SLASH * 4u;
    sprite_put(s);
    return s;
}

/*----------------------
 | game_small_number_draw
 | Description: Draws n in small digits right-aligned in a two-digit field, clearing each digit's box first. n stays 32-bit as the ARM passes it; no script reaches this, so it is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: fb
 | Params: x, y -- position; n -- the number
 | Returns: the units digit as drawn
 ----------------------*/
SpriteInfo game_small_number_draw(int16_t x, int16_t y, uint32_t n) {
    SpriteInfo s = game_sprite_info(x, y, SPRITE_SMALL_DIGITS);
    s.flags |= SPRITE_CLEAR;
    return small_number_put(s, n);
}

/*----------------------
 | game_wide_draw
 | Description: Draws wide picture id in 32-pixel columns from x, stopping at the screen's right edge or once its width is used up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: fb
 | Params: x, y -- position; id -- picture number
 | Returns: the column sprite, its bitmap one column past the last drawn
 ----------------------*/
SpriteInfo game_wide_draw(int16_t x, int16_t y, uint16_t id) {
    SpriteInfo s = game_sprite_info(x, y, (uint16_t)(id + WIDE_SPRITES));
    uint8_t left = s.width;
    s.width = WIDE_COLUMN;
    while (s.x < WIDE_COLUMN) {
        sprite_put(s);
        s.bitmap += WIDE_COLUMN;
        s.x = (int16_t)(s.x + WIDE_COLUMN);
        if (left < WIDE_COLUMN) {
            break;
        }
        left = (uint8_t)(left - WIDE_COLUMN);
    }
    return s;
}
