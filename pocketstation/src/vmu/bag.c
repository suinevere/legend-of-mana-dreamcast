/*----------------------
 | bag.c
 | Description: Ring Ring Land's item list: adding items (which picks the next mode), removing them, summing their kinds, finding items of the lowest kind, revealing the list and its trade menu.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: bag_count, bag_items, bag_cursor, bag_full, mode, scratch_kinds, bag_kind_count, scratch_first, scratch_lo, scratch_count, bag_nth_index, meter, meter_shown, meter_delay, sound_req, mode_done, scene_id, mode_wait, mode_phase, walk_frame, anim, reel_phase, reel_count, reel_len, strip_a, btn_new, btn_press, mode_phase, scratch_count, scratch_wait, scene_id
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

/*----------------------
 | MENU_BLINK
 | Description: Frames between the item menu's blink toggles.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MENU_BLINK 12u

/*----------------------
 | menu_moved
 | Description: After the item menu's cursor moved: requests sound 0 and shows the cursor for a fresh blink.
 | Author: suinevere
 | Dependencies: game_sound_request
 | Globals: mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void menu_moved(void) {
    game_sound_request(0);
    set_mode_phase(0);
    set_mode_wait(MENU_BLINK);
}

/*----------------------
 | menu_blink
 | Description: Counts the item menu's blink down, flipping mode_phase each time it runs out.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void menu_blink(void) {
    uint8_t wait = (uint8_t)(ram_mode_wait() - 1u);
    if (wait == 0u) {
        set_mode_phase((uint8_t)(ram_mode_phase() ^ 1u));
        wait = MENU_BLINK;
    }
    set_mode_wait(wait);
}

/*----------------------
 | menu_restart
 | Description: Shows the item menu's cursor for a fresh blink without a sound.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void menu_restart(void) {
    set_mode_phase(0);
    set_mode_wait(MENU_BLINK);
}

/*----------------------
 | menu_row
 | Description: Moves the item menu's yes/no row (scene_id) up to 0 or down to 1 on a press.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: scene_id
 | Params: press -- buttons pressed or repeating
 | Returns: 1 when it moved, else 0
 ----------------------*/
static uint8_t menu_row(uint8_t press) {
    uint8_t row = ram_scene_id();
    if ((press & BTN_UP) && row != 0u) {
        set_scene_id(0);
        return 1;
    }
    if ((press & BTN_DOWN) && row != 1u) {
        set_scene_id(1);
        return 1;
    }
    return 0;
}

/*----------------------
 | game_bag_menu_step
 | Description: One frame of the item menu, by mode_done: 0 picks a kind with left and right (up or down leaves to mode 1 unless the list is full), 1 picks an item of that kind (up or down goes back), 2 asks whether to trade it and 3 whether to give up, the last with its own 20-frame blink. A press confirms: a traded item is removed, the state block saved and the menu rebuilt, giving up drops one item and returns to mode 3. Only the cycle-timed runs open the menu.
 | Author: suinevere
 | Dependencies: menu_moved, menu_blink, menu_restart, menu_row, game_sound_begin, game_bag_count_first, game_bag_find_nth, game_bag_remove, game_save, game_bag_kinds
 | Globals: btn_new, btn_press, mode, mode_done, mode_phase, mode_wait, bag_full, bag_kind_count, bag_count, bag_nth_index, scratch_first, scratch_kinds, scratch_lo, scratch_count, scratch_wait, scene_id
 | Params: N/A
 | Returns: 0x100 plus the kind cursor kept across a trade's rebuild, else 0
 ----------------------*/
uint32_t game_bag_menu_step(void) {
    uint8_t press = ram_btn_press(), fresh = ram_btn_new(), state = ram_mode_done(), moved = 0, k;
    uint32_t kept = 0, count;
    if (state == 1u) {
        if (fresh & (BTN_UP | BTN_DOWN)) {
            set_mode_done(0);
            game_sound_begin(3);
            menu_restart();
            return 0;
        }
        if (ram_scratch_lo() != 1u) {
            k = ram_scratch_count();
            if (press & BTN_LEFT) {
                set_scratch_count((uint8_t)((k == 0u ? ram_scratch_lo() : k) - 1u));
                moved = 1;
            } else if (press & BTN_RIGHT) {
                k++;
                set_scratch_count(k == ram_scratch_lo() ? 0u : k);
                moved = 1;
            }
            if (moved) {
                game_bag_find_nth();
            }
        }
    } else if (state == 2u) {
        moved = menu_row(press);
    } else if (state == 3u) {
        if (menu_row(press)) {
            set_scratch_wait(20);
            moved = 1;
        } else {
            k = (uint8_t)(ram_scratch_wait() - 1u);
            set_scratch_wait(k == 0u ? 20u : k);
        }
    } else {
        if (ram_bag_full() == 0u && (fresh & (BTN_UP | BTN_DOWN))) {
            set_mode(1);
            game_sound_begin(3);
            return 0;
        }
        if (ram_bag_kind_count() != 1u && (press & (BTN_LEFT | BTN_RIGHT))) {
            k = ram_scratch_first();
            do {
                if (press & BTN_LEFT) {
                    k = (uint8_t)((k == 0u ? 4u : k) - 1u);
                } else if (++k == 4u) {
                    k = 0;
                }
            } while (((1u << k) & ram_scratch_kinds()) == 0u);
            set_scratch_first(k);
            game_bag_count_first();
            moved = 1;
        }
    }
    if (moved) {
        menu_moved();
    } else {
        menu_blink();
    }
    if (!(ram_btn_new() & BTN_FIRE)) {
        return 0;
    }
    game_sound_begin(3);
    state = ram_mode_done();
    if (state == 1u) {
        set_mode_done(2);
        set_scene_id(1);
    } else if (state == 2u) {
        if (ram_scene_id() == 0u) {
            game_bag_remove(ram_bag_nth_index(), ram_bag_full());
            if (ram_bag_full() == 1u) {
                set_mode(3);
                return 0;
            }
            game_save();
            if (ram_bag_count() == 0u) {
                set_mode(1);
                return 0;
            }
            k = ram_scratch_first();
            kept = 0x100u | k;
            game_bag_kinds();
            count = ram_scratch_lo() - 1u;
            if (count == 0u) {
                set_mode_done(0);
                menu_restart();
                return kept;
            }
            if (ram_scratch_count() == count) {
                set_scratch_count((uint8_t)(count - 1u));
            }
            set_scratch_lo((uint8_t)count);
            set_scratch_first(k);
            game_bag_find_nth();
        }
        set_mode_done(1);
    } else if (state == 3u) {
        if (ram_scene_id() == 0u) {
            set_mode_done(0);
        } else {
            set_bag_count(ram_bag_count() - 1u);
            set_mode(3);
            return 0;
        }
    } else {
        set_mode_done(1);
        set_scratch_count(0);
        game_bag_count_first();
        game_bag_find_nth();
    }
    menu_restart();
    return kept;
}
