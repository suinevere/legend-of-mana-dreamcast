/*----------------------
 | bag.c
 | Description: Ring Ring Land's item list: adding items (which picks the next mode), removing them, summing their kinds, finding items of the lowest kind and revealing the list.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: bag_count, bag_items, bag_cursor, bag_full, mode, scratch_kinds, bag_kind_count, scratch_first, scratch_lo, scratch_count, bag_nth_index, meter, meter_shown, meter_delay, sound_req, mode_done, scene_id, mode_wait, mode_phase, walk_frame, anim, reel_phase, reel_count, reel_len, strip_a
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | game_bag_add
 | Description: Appends an item; a full list sets bag_full and mode 14, otherwise the mode becomes 3.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: bag_count, bag_items, bag_full, mode
 | Params: item -- item id
 | Returns: N/A
 ----------------------*/
void game_bag_add(uint8_t item) {
    uint32_t count = ram_bag_count() + 1u;
    set_bag_count(count);
    set_bag_items((uint8_t)(count - 1u), item);
    if (count == BAG_SIZE) {
        set_bag_full(1);
        set_mode(14);
    } else {
        set_mode(3);
    }
}

/*----------------------
 | game_bag_remove
 | Description: Removes the item at index, closing the gap; when asked, moves the cursor back if it was past the item.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: bag_count, bag_items, bag_cursor
 | Params: index -- item to remove; move_cursor -- 1 to keep the cursor on the same item
 | Returns: N/A
 ----------------------*/
void game_bag_remove(uint8_t index, uint8_t move_cursor) {
    uint32_t count, n;
    uint8_t i = index;
    if (move_cursor == 1 && index < ram_bag_cursor()) {
        set_bag_cursor((uint8_t)(ram_bag_cursor() - 1u));
    }
    count = ram_bag_count() - 1u;
    set_bag_count(count);
    for (n = count - index; n != 0; n--, i++) {
        set_bag_items(i, ram_bag_items((uint8_t)(i + 1u)));
    }
}

/*----------------------
 | game_bag_kinds
 | Description: Records which item kinds the list holds, how many distinct ones and the lowest; kinds is the ARM's 32-bit bit set and bag_count the ARM's 32-bit word, an empty list loops near forever and no kinds hangs, as in the original. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: bag_count, bag_items, scratch_kinds, bag_kind_count, scratch_first
 | Params: N/A
 | Returns: the bit set of kinds held
 ----------------------*/
uint32_t game_bag_kinds(void) {
    uint32_t n = ram_bag_count(), kinds = 0u, bit, rest;
    uint8_t i = 0u, count = 0u, first = 0u, kind;
    do {
        kind = (uint8_t)game_rom_word_8094(ram_bag_items(i));
        bit = kind < 32u ? 1u << kind : 0u;
        if ((kinds & bit) == 0u) {
            kinds |= bit;
            count++;
        }
        i++;
    } while (--n != 0u);
    set_scratch_kinds((uint8_t)kinds);
    set_bag_kind_count(count);
    for (rest = kinds; (rest & 1u) == 0u; rest >>= 1) {
        first++;
    }
    set_scratch_first(first);
    return kinds;
}

/*----------------------
 | game_bag_count_first
 | Description: Counts the items of kind scratch_first into scratch_lo. bag_count is the ARM's 32-bit word, and an empty list loops near forever as in the original. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: bag_count, bag_items, scratch_first, scratch_lo
 | Params: N/A
 | Returns: the count
 ----------------------*/
uint8_t game_bag_count_first(void) {
    uint32_t n = ram_bag_count();
    uint8_t kind = ram_scratch_first(), i = 0u, count = 0u;
    do {
        if ((uint8_t)game_rom_word_8094(ram_bag_items(i)) == kind) {
            count++;
        }
        i++;
    } while (--n != 0u);
    set_scratch_lo(count);
    return count;
}

/*----------------------
 | game_bag_find_nth
 | Description: Stores in bag_nth_index the list index of item scratch_count (0 the first) of kind scratch_first. With too few such items the ARM reads on past the list, where the C wraps at 256. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: bag_items, scratch_first, scratch_count, bag_nth_index
 | Params: N/A
 | Returns: the index
 ----------------------*/
uint8_t game_bag_find_nth(void) {
    uint8_t kind = ram_scratch_first(), left = ram_scratch_count(), i = 0u;
    for (;; i++) {
        if ((uint8_t)game_rom_word_8094(ram_bag_items(i)) != kind) {
            continue;
        }
        if (left == 0u) {
            break;
        }
        left--;
    }
    set_bag_nth_index(i);
    return i;
}

/*----------------------
 | BAG_SCROLL_FRAMES
 | Description: The scroll frame shown by the low three bits of the scroll position, rising and falling.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const uint8_t BAG_SCROLL_FRAMES[8] = {0u, 5u, 8u, 0xAu, 0xBu, 0xAu, 8u, 5u};

/*----------------------
 | bag_reveal_next
 | Description: Moves bag_cursor on to the next item and waits 20 frames; once it reaches bag_count, waits 60 frames and moves to the next phase. bag_count is the ARM's 32-bit word; no script reaches this, so it is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: bag_cursor, bag_count, scene_id, mode_done
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void bag_reveal_next(void) {
    uint8_t cursor = ram_bag_cursor();
    if (cursor == ram_bag_count()) {
        set_scene_id(0x3Cu);
        set_mode_done((uint8_t)(ram_mode_done() + 1u));
        return;
    }
    set_bag_cursor((uint8_t)(cursor + 1u));
    set_scene_id(0x14u);
}

/*----------------------
 | bag_reveal_scroll
 | Description: Steps the signed scroll position in scratch_first and scratch_count down by one, wrapping below -0xD0 to 0x60, and sets scratch_kinds to the frame it shows. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: scratch_first, scratch_count, scratch_kinds
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void bag_reveal_scroll(void) {
    int16_t pos = (int16_t)(ram_scratch_first() | (ram_scratch_count() << 8));
    int16_t shown;
    pos = pos < -0xCF ? 0x60 : (int16_t)(pos - 1);
    set_scratch_first((uint8_t)pos);
    set_scratch_count((uint8_t)((uint16_t)pos >> 8));
    shown = (int16_t)(pos - 0x48);
    set_scratch_kinds(shown < 0 || (shown & 8) ? 0u : BAG_SCROLL_FRAMES[shown & 7]);
}

/*----------------------
 | game_bag_reveal
 | Description: One frame of the item list reveal once the meter has caught up, phased by mode_done: 0 waits animating, 1 steps bag_cursor, 2 waits then loads strip 3, 3 scrolls. Scripts reach only phase 0's waiting with walk_frame below 0x40; the rest is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: meter, meter_shown, meter_delay, mode, sound_req, mode_done, scene_id, mode_wait, mode_phase, walk_frame, bag_cursor, bag_count, anim, scratch_kinds, scratch_first, scratch_count, reel_phase, reel_count, reel_len, strip_a
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_bag_reveal(void) {
    uint8_t phase = ram_mode_done(), wait = (uint8_t)(ram_scene_id() - 1u), t;
    if (ram_meter_shown() != ram_meter()) {
        game_meter_update();
        return;
    }
    if (phase == 3u) {
        bag_reveal_scroll();
        return;
    }
    if (phase == 0u || phase > 3u) {
        if (ram_walk_frame() != 0x40u) {
            set_walk_frame((uint8_t)(ram_walk_frame() + 1u));
        }
    }
    if (wait != 0u) {
        set_scene_id(wait);
        if (phase == 0u || phase > 3u) {
            t = (uint8_t)(ram_mode_wait() - 1u);
            if (t == 0u) {
                t = (uint8_t)(ram_mode_phase() + 1u);
                set_mode_phase(t == 4u ? 0u : t);
                t = 5u;
            }
            set_mode_wait(t);
        }
        return;
    }
    if (phase == 2u) {
        set_anim(1, 0);
        game_reels_load_strip3();
        set_mode_done((uint8_t)(ram_mode_done() + 1u));
        return;
    }
    if (phase != 1u) {
        set_mode_done(1);
        set_scene_id(1);
    }
    bag_reveal_next();
}
