/*----------------------
 | reel.c
 | Description: Ring Ring Land's three spinning reels: loading strips from the card, speeding, turning and stopping them, shuffling and picking strip symbols and stops, drawing them and paying out results.
 | Author: suinevere
 | Dependencies: game.h, rrl_rom.h
 | Globals: fb, walk_busy, walk_kind, walk_frame, reel_slide, bag_kind_count, reel_phase, reel_count, reel_len, reel_speed_a, reel_speed_b, reel_speed_c, reel_pos_a, reel_pos_b, reel_pos_c, strip_a, strip_b, strip_c, reel_len_b, mode_wait, strip_c_len, lap_mark, seed, scratch_kinds, scratch_first, scratch_count, scratch_lo, scratch_hi, payout_unit, steps, points, lap_points, marker, reel_delay, walk_left, walk_back, btn_new
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "rrl_rom.h"

/*----------------------
 | REEL_CRAWL
 | Description: Speed at or below which a slowing reel may stop when it passes its strip's start.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define REEL_CRAWL 0x38u

/*----------------------
 | game_strip_load
 | Description: Copies symbol strip n from the card to dst; a nonzero add raises each symbol by add, capped at 9. The add != 0 path is converted from the listing and unverified by the test scripts.
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: N/A
 | Params: strip -- strip index; dst -- RAM address; add -- amount added to each symbol
 | Returns: the strip's length in bytes
 ----------------------*/
uint16_t game_strip_load(uint16_t strip, uint16_t dst, uint8_t add) {
    uint32_t src = rom32(ROM_STRIPS + strip * 4u);
    uint16_t len = (uint16_t)(rom32(ROM_STRIPS + strip * 4u + 4u) - src);
    uint16_t left = len;
    if (add) {
        game_add_capped(len, dst, add, src);
        return len;
    }
    do {
        vmu_w16(dst, rom16(src));
        src += 2u;
        dst = (uint16_t)(dst + 2u);
        left -= 2u;
    } while (left);
    return len;
}

/*----------------------
 | speed_up
 | Description: Adds step to a reel speed, capping the 32-bit sum at cap.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: speed -- current speed; step -- increase; cap -- top speed
 | Returns: the new speed
 ----------------------*/
static uint16_t speed_up(uint16_t speed, uint8_t step, uint16_t cap) {
    uint32_t s = (uint32_t)speed + step;
    return s >= cap ? cap : (uint16_t)s;
}

/*----------------------
 | game_reels_steer
 | Description: Runs one frame of the spin's phase, using mode_wait as its frame count: speed up, hold, slow down, then advance reel_phase.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: reel_phase, reel_speed_a, reel_speed_b, reel_speed_c, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reels_steer(void) {
    uint8_t phase = ram_reel_phase();
    uint8_t t = ram_mode_wait();
    uint16_t a, b, c;
    if (phase == 0u) {
        if (t < 0x28u) {
            set_mode_wait(++t);
        }
        a = speed_up(ram_reel_speed_a(), 4u, 0x60u);
        set_reel_speed_a(a);
        if (t < 0x14u) {
            return;
        }
        c = speed_up(ram_reel_speed_c(), 5u, 0x70u);
        set_reel_speed_c(c);
        if (t < 0x28u) {
            return;
        }
        b = speed_up(ram_reel_speed_b(), 4u, 0x80u);
        set_reel_speed_b(b);
        if (b != 0x80u || a < 0x60u || c < 0x70u) {
            return;
        }
        set_mode_wait(0xA0u);
    } else if (phase == 1u) {
        set_mode_wait(--t);
        if (t) {
            return;
        }
    } else if (phase == 2u) {
        if (t < 0x54u) {
            set_mode_wait(++t);
        }
        a = ram_reel_speed_a();
        if (a > REEL_CRAWL) {
            a -= 6u;
            set_reel_speed_a(a);
        }
        if (t < 0x3Cu) {
            return;
        }
        c = ram_reel_speed_c();
        if (c > REEL_CRAWL) {
            c -= 5u;
            set_reel_speed_c(c);
        }
        if (t < 0x54u) {
            return;
        }
        b = ram_reel_speed_b();
        if (b > REEL_CRAWL) {
            set_reel_speed_b((uint16_t)(b - 4u));
            return;
        }
        if (a > REEL_CRAWL || c > REEL_CRAWL) {
            return;
        }
    } else {
        return;
    }
    set_reel_phase((uint8_t)(ram_reel_phase() + 1u));
}

/*----------------------
 | reel_advance
 | Description: Moves one reel by its speed, wrapping at limit; once slowing, a crawling reel that wraps stops at position 0.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: reel_phase
 | Params: speed -- the reel's speed, set to 0 when it stops; pos -- its position; limit -- wrap point (32 bits)
 | Returns: the new position
 ----------------------*/
static uint16_t reel_advance(uint16_t *speed, uint16_t pos, uint32_t limit) {
    uint32_t p = (uint32_t)pos + *speed;
    if (p < limit) {
        return (uint16_t)p;
    }
    if (ram_reel_phase() >= 2u && *speed <= REEL_CRAWL) {
        *speed = 0;
        return 0;
    }
    return (uint16_t)(p - limit);
}

/*----------------------
 | game_reels_turn
 | Description: Moves each turning reel by its speed and marks the spin stopped once all three reels rest while stopping.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: reel_phase, reel_len, reel_speed_a, reel_speed_b, reel_speed_c, reel_pos_a, reel_pos_b, reel_pos_c
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reels_turn(void) {
    uint32_t limit = (uint32_t)ram_reel_len() << 9;
    uint16_t s, p;
    if ((s = ram_reel_speed_a()) != 0u) {
        p = reel_advance(&s, ram_reel_pos_a(), limit);
        set_reel_speed_a(s);
        set_reel_pos_a(p);
    }
    if ((s = ram_reel_speed_b()) != 0u) {
        p = reel_advance(&s, ram_reel_pos_b(), limit);
        set_reel_speed_b(s);
        set_reel_pos_b(p);
    }
    if ((s = ram_reel_speed_c()) != 0u) {
        p = reel_advance(&s, ram_reel_pos_c(), limit);
        set_reel_speed_c(s);
        set_reel_pos_c(p);
    }
    if (ram_reel_phase() == 3u && !ram_reel_speed_a() && !ram_reel_speed_b() && !ram_reel_speed_c()) {
        set_reel_phase(4u);
    }
}

/*----------------------
 | game_reels_step
 | Description: One frame of the reels: steers the spin's phase, then turns the reels.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reels_step(void) {
    game_reels_steer();
    game_reels_turn();
}

/*----------------------
 | REEL_STRIPS
 | Description: RAM address of strip_a; strip_b and strip_c follow 16 bytes apart.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define REEL_STRIPS RAM_STRIP_A

/*----------------------
 | game_reels_load
 | Description: Copies strips 0-2 into strip_a-c, takes reel_len from the first, and sets every reel stopped at position 0x800 in phase 0.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: reel_phase, reel_count, reel_len, strip_a, strip_b, strip_c, reel_pos_a, reel_pos_b, reel_pos_c, reel_speed_a, reel_speed_b, reel_speed_c
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reels_load(void) {
    set_reel_phase(0);
    set_reel_count(3u);
    set_reel_len((uint8_t)game_strip_load(0u, REEL_STRIPS, 0u));
    game_strip_load(1u, REEL_STRIPS + 0x10u, 0u);
    game_strip_load(2u, REEL_STRIPS + 0x20u, 0u);
    set_reel_pos_a(0x800u);
    set_reel_pos_b(0x800u);
    set_reel_pos_c(0x800u);
    set_reel_speed_a(0);
    set_reel_speed_b(0);
    set_reel_speed_c(0);
}

/*----------------------
 | game_reels_load_strip3
 | Description: Loads card strip 3 into strip_a as reel layout 4 in phase 0, one symbol short, and sets scratch_first to 0x40. A random board event no script rolls: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: reel_phase, reel_count, reel_len, strip_a, scratch_first, scratch_count
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reels_load_strip3(void) {
    set_reel_phase(0);
    set_reel_count(4u);
    set_reel_len((uint8_t)(game_strip_load(3u, REEL_STRIPS, 0u) - 1u));
    set_scratch_first(0x40u);
    set_scratch_count(0);
}

/*----------------------
 | game_reels_load_raised
 | Description: Copies strip_c into strip_a with each symbol raised by lap_mark and capped at 9, as reel layout 5 with reel_a stopped at 0. A random board event no script rolls: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: reel_phase, reel_count, reel_len, strip_c_len, lap_mark, strip_a, strip_c, reel_pos_a, reel_speed_a
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reels_load_raised(void) {
    uint8_t len = ram_strip_c_len();
    set_reel_phase(0);
    set_reel_count(5u);
    set_reel_len(len);
    game_add_capped(len, REEL_STRIPS, ram_lap_mark(), REEL_STRIPS + 0x20u);
    set_reel_pos_a(0);
    set_reel_speed_a(0);
}

/*----------------------
 | reel_a_raised
 | Description: Copies strip_c into strip_a with each symbol raised by add and capped at 9, sets reel_len, and stops reel_a at a random symbol.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: strip_c_len, reel_len, strip_a, strip_c, reel_pos_a, seed
 | Params: add -- amount added to each symbol
 | Returns: the symbols copied
 ----------------------*/
static uint8_t reel_a_raised(uint32_t add) {
    uint8_t len = ram_strip_c_len();
    set_reel_len(len);
    game_add_capped(len, REEL_STRIPS, add, REEL_STRIPS + 0x20u);
    set_reel_pos_a(game_reel_random_pos(len));
    return len;
}

/*----------------------
 | game_reels_load_single
 | Description: Sets reel layout 0 in phase 0: strip_c raised by lap_mark into strip_a, reel_a stopped at a random symbol, no second reel. It also starts the walker's opening count.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: walk_busy, walk_kind, walk_frame, reel_phase, reel_count, reel_slide, reel_len, reel_len_b, strip_c_len, lap_mark, strip_a, strip_c, reel_pos_a, reel_speed_a, seed
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reels_load_single(void) {
    set_walk_busy(1u);
    set_walk_kind(2u);
    set_walk_frame(0);
    set_reel_phase(0);
    set_reel_count(0);
    set_reel_slide(0);
    reel_a_raised(ram_lap_mark());
    set_reel_speed_a(0);
    set_reel_len_b(0);
}

/*----------------------
 | game_reels_load_pick
 | Description: Sets reel layout 1 in phase 1: strip_c raised by lap_mark into strip_a, reel_a stopped at a random symbol, and three different symbols picked from it.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: reel_phase, reel_count, reel_len, reel_len_b, strip_c_len, lap_mark, strip_a, strip_c, reel_pos_a, reel_speed_a, scratch_kinds, scratch_first, scratch_count, seed
 | Params: drawn -- receives the three picks in draw order
 | Returns: what game_strip_pick_three returns
 ----------------------*/
uint8_t game_reels_load_pick(uint8_t drawn[3]) {
    uint8_t count;
    set_reel_phase(1u);
    set_reel_count(1u);
    reel_a_raised(ram_lap_mark());
    count = game_strip_pick_three(ram_reel_len(), REEL_STRIPS, drawn);
    set_reel_speed_a(0);
    set_reel_len_b(0);
    return count;
}

/*----------------------
 | game_reels_load_pair
 | Description: Loads reel layout 2: strip_a is strip_c raised by lap_mark plus bag_kind_count (0 when 2); strip_b is card strip scratch_count + 8 raised by half bag_kind_count, then shuffled. A bag_kind_count of 2 or more is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: reel_phase, reel_count, reel_len, reel_len_b, strip_c_len, lap_mark, bag_kind_count, scratch_count, strip_a, strip_b, strip_c, reel_pos_a, reel_pos_b, reel_speed_a, reel_speed_b, seed
 | Params: last_roll -- receives the shuffle's last first draw
 | Returns: what game_strip_shuffle returns for strip_b
 ----------------------*/
int16_t game_reels_load_pair(uint8_t *last_roll) {
    uint8_t kinds = ram_bag_kind_count();
    uint8_t len;
    int16_t moved;
    set_reel_phase(0);
    set_reel_count(2u);
    reel_a_raised((uint32_t)ram_lap_mark() + (kinds == 2u ? 0u : kinds));
    len = (uint8_t)game_strip_load((uint16_t)(ram_scratch_count() + 8u), REEL_STRIPS + 0x10u, (uint8_t)(kinds >> 1));
    set_reel_len_b(len);
    moved = game_strip_shuffle(len, REEL_STRIPS + 0x10u, last_roll);
    set_reel_pos_b(game_reel_random_pos(len));
    set_reel_speed_a(0);
    set_reel_speed_b(0);
    return moved;
}

/*----------------------
 | strip_roll
 | Description: Draws a random symbol index below len, as random(len * 16) / 16.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: seed
 | Params: len -- symbols to choose from
 | Returns: an index in [0, len)
 ----------------------*/
static uint8_t strip_roll(uint8_t len) {
    return (uint8_t)(game_random((uint16_t)(len * 16u)) >> 4);
}

/*----------------------
 | game_strip_shuffle
 | Description: Makes eight draws of two random symbols of a strip and swaps each pair that differs. No script makes eight equal pairs, so the -1 result is unverified.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: seed
 | Params: len -- symbols on the strip; strip -- its RAM address; last_roll -- receives the last pair's first draw
 | Returns: the symbol the last swap took from the second draw's slot, or -1 when no pair swapped
 ----------------------*/
int16_t game_strip_shuffle(uint8_t len, uint16_t strip, uint8_t *last_roll) {
    int16_t moved = -1;
    uint8_t draws = 8u;
    do {
        uint8_t a = strip_roll(len);
        uint8_t b = strip_roll(len);
        if (a != b) {
            uint8_t at_b = vmu_r8((uint16_t)(strip + b));
            vmu_w8((uint16_t)(strip + b), vmu_r8((uint16_t)(strip + a)));
            vmu_w8((uint16_t)(strip + a), at_b);
            moved = at_b;
        }
        *last_roll = a;
    } while (--draws);
    return moved;
}

/*----------------------
 | REEL_STEP
 | Description: Reel position per symbol in the board's random stops and symbol lookups, while reel turning wraps at reel_len * 0x200.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define REEL_STEP 0x280u

/*----------------------
 | game_reel_random_pos
 | Description: Picks a random symbol below len and returns its reel position, 0x280 per symbol. Strips hold at most 16 symbols, so the ARM's 32-bit product fits 16 bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: seed
 | Params: len -- symbols on the strip
 | Returns: the position
 ----------------------*/
uint16_t game_reel_random_pos(uint8_t len) {
    return (uint16_t)(strip_roll(len) * REEL_STEP);
}

/*----------------------
 | game_strip_next_symbol
 | Description: Returns the strip symbol after the one at reel position pos (0x280 per symbol), wrapping to the first after the last. The divide is the ARM's 32-bit routine.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: len -- symbols on the strip; pos -- reel position; strip -- the strip's address
 | Returns: the symbol
 ----------------------*/
uint8_t game_strip_next_symbol(uint8_t len, uint16_t pos, uint16_t strip) {
    uint32_t rem;
    uint8_t next = (uint8_t)(game_divide((uint32_t)(pos >> 6), 10u, &rem) + 1u);
    if (next == len) {
        next = 0;
    }
    return vmu_r8((uint16_t)(strip + next));
}

/*----------------------
 | game_strip_pick_three
 | Description: Picks three different random symbols (0-9) among those on a strip and stores them lowest first in scratch_kinds, scratch_first and scratch_count. The symbol set is 32 bits, as in the ARM. Converted from the listing and unverified: fewer than three distinct symbols hangs as in the original, and len 0 reads 256 bytes where the ARM loops on. With symbols of 10 or more and under three below 10, left wraps to 255 here and to 0xFFFFFFFF in the ARM. The bit search stops at bit 32, where it spins without shifting past 31.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: seed, scratch_kinds, scratch_first, scratch_count
 | Params: len -- symbols on the strip; strip -- its RAM address; drawn -- receives the three picks in draw order
 | Returns: how many distinct symbols 0-9 the strip holds
 ----------------------*/
uint8_t game_strip_pick_three(uint8_t len, uint16_t strip, uint8_t drawn[3]) {
    uint32_t set = 0;
    uint8_t count = 0, left, k, lo, mid, hi, t;
    do {
        uint8_t s = vmu_r8(strip++);
        set |= s < 32u ? 1u << s : 0u;
    } while (--len);
    for (k = 0; k < 10u; k++) {
        count += (uint8_t)((set >> k) & 1u);
    }
    left = count;
    for (k = 0; k < 3u; k++) {
        uint8_t n = strip_roll(left), bit = 0;
        for (;;) {
            if (bit < 32u && ((set >> bit) & 1u)) {
                if (!n) {
                    break;
                }
                n--;
            }
            if (bit < 32u) {
                bit++;
            }
        }
        drawn[k] = bit;
        set &= ~(1u << bit);
        left--;
    }
    lo = drawn[0];
    mid = drawn[1];
    hi = drawn[2];
    if (lo >= mid) {
        t = lo;
        lo = mid;
        mid = t;
    }
    if (lo >= hi) {
        t = lo;
        lo = hi;
        hi = t;
    }
    if (mid >= hi) {
        t = mid;
        mid = hi;
        hi = t;
    }
    set_scratch_kinds(lo);
    set_scratch_first(mid);
    set_scratch_count(hi);
    return count;
}

/*----------------------
 | symbols_draw
 | Description: Draws strip symbols left to right from x, wrapping the strip index at len, until x reaches 32.
 | Author: suinevere
 | Dependencies: vmu_hw.h
 | Globals: fb
 | Params: x, y -- first symbol's position; index -- first strip entry; len -- strip length; strip -- symbol bytes; table, width, height -- symbol bitmaps (12 bytes each) and size
 | Returns: the last symbol's bitmap address
 ----------------------*/
static uint32_t symbols_draw(int16_t x, int16_t y, uint8_t index, uint8_t len, uint16_t strip, uint32_t table, uint8_t width, uint8_t height) {
    uint32_t bitmap;
    do {
        bitmap = table + vmu_r8((uint16_t)(strip + index)) * 12u;
        game_sprite_draw(x, y, width, height, bitmap, 0);
        index = (uint8_t)(index + 1u);
        if (index == len) {
            index = 0;
        }
        x = (int16_t)(x + width);
    } while (x < 32);
    return bitmap;
}

/*----------------------
 | game_reel_draw
 | Description: Draws a reel's 8x11 symbols across the screen at row y, scrolled by the reel position (one symbol per 0x200).
 | Author: suinevere
 | Dependencies: rrl_rom.h
 | Globals: fb
 | Params: y -- top row; pos -- reel position; len -- strip length; strip -- symbol bytes
 | Returns: the last symbol's bitmap address
 ----------------------*/
uint32_t game_reel_draw(int16_t y, uint16_t pos, uint8_t len, uint16_t strip) {
    uint32_t rem;
    uint8_t index = (uint8_t)game_divide(pos >> 6, 8u, &rem);
    return symbols_draw((int16_t)-(int16_t)rem, y, index, len, strip, ROM_SYMBOLS_8X11, 8u, 11u);
}

/*----------------------
 | game_reel_window_draw
 | Description: Clears a band edged by a solid row, draws a reel's 10x9 symbols and inverts bits 11-20, alike in either bit order; rows past 31 run into blit_a and blit_b. Rows past 255, and a zero len at position 0, narrow to bytes where the ARM stays 32-bit; unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h, rrl_rom.h, fb_run, set_fb_run, set_fb_row
 | Globals: the VMU frame buffer, blit_a, blit_b
 | Params: y -- band position (16 and up: solid row on top); pos -- reel position; len -- strip length; strip -- symbol bytes
 | Returns: the last symbol's bitmap address, or 0 when the band is off screen
 ----------------------*/
uint32_t game_reel_window_draw(int16_t y, uint16_t pos, uint8_t len, uint16_t strip) {
    uint16_t top, rows, r;
    uint32_t n = pos >> 6, rem, last;
    uint8_t index;
    if (y >= 16) {
        set_fb_run((uint8_t)y, 0xFFFFFFFFu);
        top = (uint16_t)(y + 1);
        rows = (uint16_t)(y - 1);
        y = (int16_t)(y + 1);
    } else {
        if (y + 9 < 0) {
            return 0;
        }
        set_fb_row((uint8_t)(y + 9), 0xFFFFFFFFu);
        if (y + 9 == 0) {
            return 0;
        }
        top = 0;
        rows = (uint16_t)(y + 9);
    }
    for (r = 0; r < rows; r++) {
        set_fb_run((uint8_t)(top + r), 0);
    }
    if (n == 0u) {
        n = len * 10u;
    }
    index = (uint8_t)game_divide(n - 1u, 10u, &rem);
    last = symbols_draw((int16_t)-(int16_t)rem, y, index, len, strip, ROM_SYMBOLS_10X9, 10u, 9u);
    for (r = 0; r < rows; r++) {
        set_fb_run((uint8_t)(top + r), fb_run((uint8_t)(top + r)) ^ 0x001FF800u);
    }
    return last;
}

/*----------------------
 | game_reels_next_two
 | Description: Records the symbols after the first two reels' stops: strip_a's in scratch_kinds and strip_b's in scratch_first.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: reel_len, reel_len_b, reel_pos_a, reel_pos_b, strip_a, strip_b, scratch_kinds, scratch_first
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reels_next_two(void) {
    set_scratch_kinds(game_strip_next_symbol(ram_reel_len(), ram_reel_pos_a(), REEL_STRIPS));
    set_scratch_first(game_strip_next_symbol(ram_reel_len_b(), ram_reel_pos_b(), REEL_STRIPS + 0x10u));
}

/*----------------------
 | POINTS_MAX
 | Description: The cap on the points total.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define POINTS_MAX 99999u

/*----------------------
 | PAYOUT_ONE
 | Description: Marker type a payout shows for a single unit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAYOUT_ONE 0x67u

/*----------------------
 | PAYOUT_TEN
 | Description: Marker type a payout shows for ten units.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAYOUT_TEN 0x66u

/*----------------------
 | PAYOUT_TAIL
 | Description: Marker type a payout shows after each one- or ten-unit marker.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAYOUT_TAIL 0x68u

/*----------------------
 | PAYOUT_LEFT
 | Description: Signed x offset (-7) of a two-row payout's first pair of markers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAYOUT_LEFT 0xF9u

/*----------------------
 | PAYOUT_RIGHT
 | Description: Signed x offset of a two-row payout's second pair of markers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAYOUT_RIGHT 8u

/*----------------------
 | game_payout
 | Description: Shows result kind scratch_count's markers and adds payout_unit times 2, 10, 11 or 20 for kinds 1-4 (else 1), times 1-3 by laps walked, to points capped at 99999. Kinds 3 and 4, the lap multipliers and the cap are converted from the listing and unverified; points is the original's 32-bit word.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: marker, scratch_count, payout_unit, steps, points, scratch_lo, scratch_hi, lap_points
 | Params: N/A
 | Returns: the points added, 32-bit as in the original, where past the cap it is 99999 - points - payout (0 or negative)
 ----------------------*/
uint32_t game_payout(void) {
    uint8_t kind, unit;
    uint16_t gain;
    uint32_t total, won;
    game_markers_clear();
    kind = ram_scratch_count();
    unit = ram_payout_unit();
    if (kind == 1u || kind == 3u || kind == 4u) {
        game_marker_start(0, kind == 4u ? PAYOUT_TEN : PAYOUT_ONE, PAYOUT_LEFT);
        game_marker_start(1, PAYOUT_TAIL, PAYOUT_LEFT);
        gain = (uint16_t)((kind == 4u ? unit * 10u : unit) + (kind == 1u ? unit : unit * 10u));
        game_marker_start(2, kind == 1u ? PAYOUT_ONE : PAYOUT_TEN, PAYOUT_RIGHT);
        game_marker_start(3, PAYOUT_TAIL, PAYOUT_RIGHT);
    } else {
        gain = (uint16_t)(kind == 2u ? unit * 10u : unit);
        game_marker_start(0, kind == 2u ? PAYOUT_TEN : PAYOUT_ONE, 0);
        game_marker_start(1, PAYOUT_TAIL, 0);
    }
    gain = (uint16_t)(gain * (ram_steps() < 32u ? 1u : ram_steps() < 64u ? 2u : 3u));
    total = ram_points() + gain;
    won = gain;
    if (total >= POINTS_MAX) {
        won = POINTS_MAX - total;
        total = POINTS_MAX;
    }
    set_points(total);
    set_scratch_lo((uint8_t)won);
    set_scratch_hi((uint8_t)(won >> 8));
    set_lap_points((uint16_t)(ram_lap_points() + won));
    return won;
}

/*----------------------
 | WAIT16
 | Description: mode_wait read as a 16-bit word with the byte above it, as reel A's spin times its 300-frame top speed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define WAIT16 RAM_MODE_WAIT

/*----------------------
 | game_reel_spin_step
 | Description: One frame of reel A's spin, by reel_phase: 0 slides the window in, 1 speeds up to 0x80 and holds it 300 frames (2) or until a press, 3 spins on a random 1-14 frames, 4 slows to 0x30, 5 stops on a symbol boundary, 6 waits 90 frames or a press, then moves on two phases with reels left or else sets walk_kind 3, and 7 slides the window out and turns the symbol into the walk. The reel then advances by its speed, wrapping over its strip.
 | Author: suinevere
 | Dependencies: game_strip_next_symbol, game_sound_begin, game_random, game_divide
 | Globals: reel_phase, reel_count, reel_slide, reel_delay, reel_len, reel_speed_a, reel_pos_a, strip_a, mode_wait, btn_new, walk_left, walk_back, walk_kind
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reel_spin_step(void) {
    uint8_t phase = ram_reel_phase(), next = 0, pressed = ram_btn_new() & 1u, r;
    uint32_t speed, stride, rem, pos;
    switch (phase) {
    case 0:
        set_reel_slide((uint8_t)(ram_reel_slide() + 1u));
        next = ram_reel_slide() == 12u;
        break;
    case 7:
        set_reel_slide((uint8_t)(ram_reel_slide() - 1u));
        if (ram_reel_slide() == 0u) {
            set_walk_left(game_strip_next_symbol(ram_reel_len(), ram_reel_pos_a(), RAM_STRIP_A));
            set_walk_back(0);
            next = 1;
        }
        break;
    case 1:
        speed = ram_reel_speed_a() + 4u;
        if (speed < 0x80u) {
            set_reel_speed_a((uint16_t)speed);
        } else {
            set_reel_speed_a(0x80u);
            vmu_w16(WAIT16, 300u);
            next = 1;
        }
        break;
    case 2:
        if (pressed) {
            game_sound_begin(3);
        } else {
            vmu_w16(WAIT16, (uint16_t)(vmu_r16(WAIT16) - 1u));
            if (vmu_r16(WAIT16) != 0u) {
                break;
            }
        }
        r = (uint8_t)game_random(0x1Eu);
        set_reel_delay(r > 15u ? (uint8_t)(r - 15u) : 1u);
        next = 1;
        break;
    case 3:
        set_reel_delay((uint8_t)(ram_reel_delay() - 1u));
        next = ram_reel_delay() == 0u;
        break;
    case 4:
        if (ram_reel_speed_a() <= 0x30u) {
            next = 1;
        } else {
            set_reel_speed_a((uint16_t)(ram_reel_speed_a() - 6u));
        }
        break;
    case 6:
        if (!pressed) {
            set_mode_wait((uint8_t)(ram_mode_wait() - 1u));
            if (ram_mode_wait() != 0u) {
                break;
            }
        }
        if (ram_reel_count() != 0u) {
            next = 2;
        } else {
            set_walk_kind(3);
            next = 1;
        }
        break;
    default:
        break;
    }
    if (next) {
        set_reel_phase((uint8_t)(phase + next));
    }
    speed = ram_reel_speed_a();
    if (speed == 0u) {
        return;
    }
    stride = ram_reel_len() * 640u;
    if (ram_reel_phase() == 5u) {
        game_divide(ram_reel_pos_a(), 640u, &rem);
        if (rem <= speed) {
            set_reel_phase(6);
            set_reel_speed_a(0);
            set_mode_wait(0x5A);
            speed = rem;
        }
    }
    pos = ram_reel_pos_a();
    pos = pos >= speed ? pos - speed : pos - speed + stride;
    set_reel_pos_a((uint16_t)pos);
}

/*----------------------
 | wait16_done
 | Description: Counts the 16-bit wait down.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: mode_wait
 | Params: N/A
 | Returns: 1 when it reached 0, else 0
 ----------------------*/
static uint8_t wait16_done(void) {
    vmu_w16(WAIT16, (uint16_t)(vmu_r16(WAIT16) - 1u));
    return vmu_r16(WAIT16) == 0u;
}

/*----------------------
 | game_reel_pair_spin_step
 | Description: One frame of reels A and B spinning together, by reel_phase: 0 speeds A to 0x80, 2 speeds B to 0x70, each held 10 frames (1, 3) before 300 frames at speed, 4 runs out the wait or a press, 5 spins on a random 1-14 frames, 6 slows both to 0x30 and stops each on a symbol boundary, and 7 waits 90 frames or a press. A turns backward outside phases 2-3 and B forward from phase 2, each wrapping over its strip.
 | Author: suinevere
 | Dependencies: wait16_done, game_sound_begin, game_random, game_divide
 | Globals: reel_phase, reel_delay, reel_len, reel_len_b, reel_speed_a, reel_speed_b, reel_pos_a, reel_pos_b, mode_wait, btn_new
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reel_pair_spin_step(void) {
    uint8_t phase = ram_reel_phase(), next = 0, pressed = ram_btn_new() & 1u, r;
    uint32_t speed, stride, rem, pos;
    switch (phase) {
    case 0:
        speed = ram_reel_speed_a() + 4u;
        if (speed < 0x80u) {
            set_reel_speed_a((uint16_t)speed);
        } else {
            set_reel_speed_a(0x80u);
            vmu_w16(WAIT16, 10u);
            next = 1;
        }
        break;
    case 2:
        speed = ram_reel_speed_b() + 6u;
        if (speed < 0x70u) {
            set_reel_speed_b((uint16_t)speed);
        } else {
            set_reel_speed_b(0x70u);
            vmu_w16(WAIT16, 10u);
            next = 1;
        }
        break;
    case 1:
    case 3:
        if (wait16_done()) {
            vmu_w16(WAIT16, 300u);
            next = 1;
        }
        break;
    case 4:
        if (pressed) {
            game_sound_begin(3);
        } else if (!wait16_done()) {
            break;
        }
        r = (uint8_t)game_random(0x1Eu);
        set_reel_delay(r > 15u ? (uint8_t)(r - 15u) : 1u);
        next = 1;
        break;
    case 5:
        set_reel_delay((uint8_t)(ram_reel_delay() - 1u));
        next = ram_reel_delay() == 0u;
        break;
    case 6:
        if (ram_reel_speed_a() > 0x30u) {
            set_reel_speed_a((uint16_t)(ram_reel_speed_a() - 6u));
        }
        if (ram_reel_speed_b() > 0x30u) {
            set_reel_speed_b((uint16_t)(ram_reel_speed_b() - 8u));
        }
        break;
    case 7:
        if (!pressed) {
            set_mode_wait((uint8_t)(ram_mode_wait() - 1u));
            next = ram_mode_wait() == 0u;
        } else {
            next = 1;
        }
        break;
    default:
        break;
    }
    if (next) {
        set_reel_phase((uint8_t)(phase + 1u));
    }
    phase = ram_reel_phase();
    speed = ram_reel_speed_a();
    if (speed != 0u && phase != 2u && phase != 3u) {
        stride = ram_reel_len() * 640u;
        if (phase == 6u && speed <= 0x30u) {
            game_divide(ram_reel_pos_a(), 640u, &rem);
            if (rem <= speed) {
                speed = rem;
                set_reel_speed_a(0);
            }
        }
        pos = ram_reel_pos_a();
        pos = pos >= speed ? pos - speed : pos - speed + stride;
        set_reel_pos_a((uint16_t)pos);
    }
    speed = ram_reel_speed_b();
    if (speed != 0u && phase >= 2u) {
        stride = ram_reel_len_b() * 640u;
        if (phase == 6u && speed <= 0x30u) {
            game_divide(ram_reel_pos_b(), 640u, &rem);
            if (640u - rem <= speed) {
                speed = 640u - rem;
                set_reel_speed_b(0);
            }
        }
        pos = ram_reel_pos_b() + speed;
        if (pos >= stride) {
            pos -= stride;
        }
        set_reel_pos_b((uint16_t)pos);
    }
    if (ram_reel_phase() == 6u && ram_reel_speed_a() == 0u && ram_reel_speed_b() == 0u) {
        set_reel_phase(7);
        set_mode_wait(0x5A);
    }
}
