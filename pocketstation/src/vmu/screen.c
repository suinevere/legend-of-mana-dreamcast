/*----------------------
 | screen.c
 | Description: Ring Ring Land's per-mode screen draws: the two menus' toggles and cursor, the active markers, the reel window, the reel screen with its three counts and the reel screen with its characters and price tags, and the board screens around them.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: mode_phase, sel_a, mode_done, marker, reel_phase, reel_slide, reel_pos_a, reel_pos_b, reel_len, reel_len_b, strip_a, strip_b, scratch_kinds, scratch_first, scratch_count, bag_kind_count, mode_wait, anim, scene_id, scratch_wait, bag_cursor, bag_items, walk_frame, fb, lap_points, bag_nth_index
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | STRIP_A
 | Description: Address of reel strip A, which the reel window draws.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STRIP_A RAM_STRIP_A

/*----------------------
 | MARKER_SLOTS
 | Description: Number of marker slots, four bytes each.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MARKER_SLOTS 4u

/*----------------------
 | draw
 | Description: Draws a sprite as game_sprite_info described it.
 | Author: suinevere
 | Dependencies: game_sprite_draw
 | Globals: N/A
 | Params: s -- the sprite
 | Returns: N/A
 ----------------------*/
static void draw(SpriteInfo s) {
    game_sprite_draw(s.x, s.y, s.width, s.height, s.bitmap, s.flags);
}

/*----------------------
 | game_menu_a_draw
 | Description: Steps the first yes/no toggle, draws its panel and, while the blink shows it, the pointer at the left (no) or mirrored at the right (yes).
 | Author: suinevere
 | Dependencies: game_menu_pick_a, game_sprite_info, draw
 | Globals: mode_phase, sel_a
 | Params: N/A
 | Returns: the last sprite drawn
 ----------------------*/
SpriteInfo game_menu_a_draw(void) {
    SpriteInfo s;
    uint8_t yes;
    game_menu_pick_a();
    s = game_sprite_info(0, 0, 0x43u);
    draw(s);
    if (ram_mode_phase() != 0u) {
        return s;
    }
    yes = ram_sel_a();
    s = game_sprite_info(yes ? 0x16 : 2, 0x10, 0x34u);
    if (yes) {
        s.flags |= SPRITE_MIRROR;
    }
    draw(s);
    return s;
}

/*----------------------
 | game_menu_row_draw
 | Description: Steps the three-row cursor, draws its panel and, while the blink shows it, the cursor beside row mode_done.
 | Author: suinevere
 | Dependencies: game_menu_pick_row, game_sprite_info, draw
 | Globals: mode_phase, mode_done
 | Params: N/A
 | Returns: the last sprite drawn
 ----------------------*/
SpriteInfo game_menu_row_draw(void) {
    SpriteInfo s;
    game_menu_pick_row();
    s = game_sprite_info(0, 0, 0x44u);
    draw(s);
    if (ram_mode_phase() != 0u) {
        return s;
    }
    s = game_sprite_info(0, (int16_t)(ram_mode_done() * 10u + 2u), 0x33u);
    draw(s);
    return s;
}

/*----------------------
 | game_markers_draw
 | Description: Steps every marker slot in use and draws the frames that have a bitmap.
 | Author: suinevere
 | Dependencies: game_marker_frame, draw
 | Globals: marker
 | Params: last -- receives the last slot stepped, 0xFF when none was in use
 | Returns: the last slot's sprite
 ----------------------*/
SpriteInfo game_markers_draw(uint8_t *last) {
    SpriteInfo s = {0, 0, 0, 0, 0, 0};
    uint8_t slot;
    *last = 0xFFu;
    for (slot = 0; slot < MARKER_SLOTS; slot++) {
        if (ram_marker((uint8_t)(slot * 4u)) == 0xFFu) {
            continue;
        }
        s = game_marker_frame(slot);
        if (s.bitmap != 0u) {
            draw(s);
        }
        *last = slot;
    }
    return s;
}

/*----------------------
 | game_reel_window_show
 | Description: Draws reel A's window unless the reel phase is 8; in phases 0 and 7 it slides down by reel_slide less 12.
 | Author: suinevere
 | Dependencies: game_reel_window_draw
 | Globals: reel_phase, reel_slide, reel_pos_a, reel_len
 | Params: y -- receives the window's y as the ARM computes it, untouched in phase 8
 | Returns: game_reel_window_draw's last symbol, 0 when off screen or not drawn
 ----------------------*/
uint32_t game_reel_window_show(uint32_t *y) {
    uint8_t phase = ram_reel_phase();
    if (phase == 8u) {
        return 0;
    }
    *y = (phase == 0u || phase == 7u) ? ram_reel_slide() - 12u : 0u;
    return game_reel_window_draw((int16_t)*y, ram_reel_pos_a(), ram_reel_len(), STRIP_A);
}

/*----------------------
 | game_reel_screen_draw
 | Description: Draws the reel screen's panel, its three counts (scratch_kinds, scratch_first, scratch_count) as digits nine pixels apart, and reel A's window at the top.
 | Author: suinevere
 | Dependencies: game_sprite_info, draw, game_reel_window_draw
 | Globals: scratch_kinds, scratch_first, scratch_count, reel_pos_a, reel_len
 | Params: N/A
 | Returns: game_reel_window_draw's last symbol, 0 when off screen
 ----------------------*/
uint32_t game_reel_screen_draw(void) {
    const uint8_t counts[3] = { ram_scratch_kinds(), ram_scratch_first(), ram_scratch_count() };
    SpriteInfo s = game_sprite_info(0, 0, 0x4Au);
    uint8_t i;
    draw(s);
    for (i = 0; i < 3u; i++) {
        s = game_sprite_info((int16_t)(5 + i * 9), 0x16, SPRITE_DIGITS);
        s.bitmap += (uint32_t)counts[i] << 2;
        s.flags |= SPRITE_CLEAR;
        draw(s);
    }
    return game_reel_window_draw(0, ram_reel_pos_a(), ram_reel_len(), STRIP_A);
}

/*----------------------
 | STRIP_B
 | Description: Address of reel strip B, drawn in the lower window.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STRIP_B RAM_STRIP_B

/*----------------------
 | price_draw
 | Description: Draws the reel screen's price tag at x, y and the digit after it.
 | Author: suinevere
 | Dependencies: game_sprite_info, draw
 | Globals: N/A
 | Params: x, y -- the tag's position
 | Returns: N/A
 ----------------------*/
static void price_draw(int16_t x, int16_t y) {
    SpriteInfo s = game_sprite_info(x, y, 0x37u);
    s.bitmap += 8u;
    draw(s);
    s = game_sprite_info((int16_t)(s.x + s.width), s.y, SPRITE_DIGITS);
    s.bitmap += 4u;
    draw(s);
}

/*----------------------
 | game_reel_anim_screen_draw
 | Description: Steps the animations and draws the reel screen by phase: phases 0-1 the first character (and the price tag when one kind is held), 2-3 the second (tag at two kinds), 4 and up both; then window A outside phases 2-3 and window B from phase 2.
 | Author: suinevere
 | Dependencies: game_anims_tick, game_anim_draw, price_draw, game_reel_window_draw
 | Globals: reel_phase, bag_kind_count, reel_pos_a, reel_len, reel_pos_b, reel_len_b
 | Params: N/A
 | Returns: the last window's last symbol, 0 when off screen
 ----------------------*/
uint32_t game_reel_anim_screen_draw(void) {
    uint8_t phase, kinds = ram_bag_kind_count();
    uint32_t last = 0;
    game_anims_tick();
    phase = ram_reel_phase();
    if (phase < 2u) {
        if (kinds != 1u) {
            game_anim_draw(0, 4, 0);
        } else {
            game_anim_draw(-5, 4, 0);
            price_draw(0x16, 0x10);
        }
    } else if (phase < 4u) {
        if (kinds != 2u) {
            game_anim_draw(0, -4, 1);
        } else {
            game_anim_draw(5, -4, 1);
            price_draw(0, 8);
        }
    } else {
        game_anim_draw(-10, 0, 1);
        game_anim_draw(10, 0, 0);
    }
    if (phase != 2u && phase != 3u) {
        last = game_reel_window_draw(0, ram_reel_pos_a(), ram_reel_len(), STRIP_A);
    }
    if (phase >= 2u) {
        last = game_reel_window_draw(0x16, ram_reel_pos_b(), ram_reel_len_b(), STRIP_B);
    }
    return last;
}

/*----------------------
 | game_board_walk_screen
 | Description: One frame of walking by the buttons: steps the walk, then draws the board and the meter.
 | Author: suinevere
 | Dependencies: game_walk_by_buttons, game_board_draw, game_meter_draw
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_board_walk_screen(void) {
    game_walk_by_buttons();
    game_board_draw();
    game_meter_draw(0);
}

/*----------------------
 | game_board_auto_screen
 | Description: One frame of a walk that runs on its own: steps the pending walk, then draws the board and the meter.
 | Author: suinevere
 | Dependencies: game_walk_pending, game_board_draw, game_meter_draw
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_board_auto_screen(void) {
    game_walk_pending();
    game_board_draw();
    game_meter_draw(0);
}

/*----------------------
 | game_board_wait_screen
 | Description: Draws the board and the meter while mode_wait counts up to 70, with the banner shown for the first 45 frames.
 | Author: suinevere
 | Dependencies: game_board_draw, game_meter_draw, game_sprite_info, draw
 | Globals: mode_wait
 | Params: N/A
 | Returns: 0 once 70 frames have passed, else 1
 ----------------------*/
uint32_t game_board_wait_screen(void) {
    uint32_t wait;
    game_board_draw();
    game_meter_draw(0);
    wait = ram_mode_wait() + 1u;
    set_mode_wait((uint8_t)wait);
    if (wait >= 70u) {
        return 0;
    }
    if (wait < 45u) {
        draw(game_sprite_info(0, 10, 0x45u));
    }
    return 1;
}

/*----------------------
 | game_board_choice_draw
 | Description: Steps the board's yes/no choice, draws the board and, while the blink shows it, the pointer by slot 0's argument: bit 1 mirrors it to the right, bit 0 raises it to the second row's bitmap.
 | Author: suinevere
 | Dependencies: game_menu_pick_b, game_board_draw, game_sprite_info, draw
 | Globals: mode_phase, anim
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_board_choice_draw(void) {
    SpriteInfo s;
    uint8_t arg;
    game_menu_pick_b();
    game_board_draw();
    if (ram_mode_phase() != 0u) {
        return;
    }
    arg = ram_anim(1);
    s = game_sprite_info((arg & 2u) ? 0x18 : 0, (arg & 1u) ? 0x0A : 0x18, 0x34u);
    if (arg & 2u) {
        s.flags |= SPRITE_MIRROR;
    }
    if (arg & 1u) {
        s.bitmap += 0x10u;
    }
    draw(s);
}

/*----------------------
 | item_icon_draw
 | Description: Draws item's icon at x, 0 from its 0x02008094 entry: the low byte the icon, the high byte one less than its marks.
 | Author: suinevere
 | Dependencies: game_rom_word_8094, game_icon_draw
 | Globals: N/A
 | Params: x -- icon x; item -- item id
 | Returns: N/A
 ----------------------*/
static void item_icon_draw(int16_t x, uint8_t item) {
    uint16_t entry = game_rom_word_8094(item);
    game_icon_draw(x, 0, (uint8_t)entry, (uint8_t)((entry >> 8) + 1u));
}

/*----------------------
 | game_scene_screen_draw
 | Description: Steps the scene and, until its result shows (mode_done 1), draws only the markers; then scene 2 shows reel A from its start with the price tag while nothing is counted, and other scenes slide the traded item, the arrow (lit past 10) and the new item in from the right by half scratch_first.
 | Author: suinevere
 | Dependencies: game_scene_step, game_markers_draw, game_reel_window_draw, price_draw, item_icon_draw, game_sprite_info, draw
 | Globals: mode_done, scene_id, scratch_first, scratch_wait, scratch_kinds, scratch_count, reel_len
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_scene_screen_draw(void) {
    SpriteInfo s;
    int16_t slide;
    uint8_t last;
    game_scene_step();
    if (ram_mode_done() != 1u) {
        game_markers_draw(&last);
        return;
    }
    if (ram_scene_id() == 2u) {
        game_reel_window_draw(0, 0, ram_reel_len(), STRIP_A);
        if (ram_scratch_count() == 0u) {
            price_draw(0x0B, 0x11);
        }
        return;
    }
    slide = (int16_t)(ram_scratch_first() >> 1);
    if (ram_scratch_wait() != 0xFFu) {
        item_icon_draw((int16_t)-slide, ram_scratch_wait());
        s = game_sprite_info((int16_t)(0x16 - slide), 9, 0x4Du);
        if (ram_scratch_count() <= 10u) {
            s.bitmap += 0x1Cu;
        }
        draw(s);
    }
    item_icon_draw((int16_t)(0x34 - slide), ram_scratch_kinds());
}

/*----------------------
 | ROM_SYMBOLS_8X11
 | Description: The reel symbols' 8x11 bitmaps, 12 bytes each.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ROM_SYMBOLS_8X11 0x02007DBCu

/*----------------------
 | game_bag_screen_draw
 | Description: Steps the item list reveal and draws by mode_done: 1 the item at the cursor from scene 8 on, 2 the lap price tag with lap_points, 3 reel A's symbols sliding in by half the 16-bit scratch_first word with the first character, otherwise the board and meter with the bag panel lowered by half walk_frame over blank rows.
 | Author: suinevere
 | Dependencies: game_bag_reveal, game_board_draw, game_meter_draw, game_sprite_info, draw, item_icon_draw, game_digits_x, game_number_draw, game_anims_tick, game_sprite_draw, game_anim_draw
 | Globals: mode_done, scene_id, bag_cursor, bag_items, walk_frame, mode_phase, fb, lap_points, scratch_first, scratch_count, scratch_kinds, reel_len, strip_a
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_bag_screen_draw(void) {
    SpriteInfo s;
    uint8_t done, frame, i;
    uint32_t left, at;
    int16_t x;
    game_bag_reveal();
    done = ram_mode_done();
    if (done == 1u) {
        if (ram_scene_id() >= 8u) {
            item_icon_draw(0, ram_bag_items((uint8_t)(ram_bag_cursor() - 1u)));
        }
    } else if (done == 2u) {
        draw(game_sprite_info(0, 0, 0x4Cu));
        s = game_sprite_info(4, 0x14, 0x37u);
        s.bitmap += 8u;
        draw(s);
        x = (int16_t)game_digits_x((uint32_t)(int32_t)(s.x + s.width), ram_lap_points());
        game_number_draw(x, s.y, ram_lap_points());
    } else if (done == 3u) {
        game_anims_tick();
        x = (int16_t)((int16_t)(ram_scratch_first() | (ram_scratch_count() << 8)) >> 1);
        left = ram_reel_len();
        for (at = RAM_STRIP_A; x < 32; at++) {
            game_sprite_draw(x, 0, 8, 11, ROM_SYMBOLS_8X11 + vmu_r8((uint16_t)at) * 12u, 0);
            x = (int16_t)(x + 8);
            if (--left == 0u) {
                break;
            }
        }
        game_anim_draw(0, (int16_t)(7 - ram_scratch_kinds()), 0);
    } else {
        game_board_draw();
        game_meter_draw(ram_walk_frame());
        frame = ram_walk_frame();
        for (i = 0; i < (uint8_t)(frame >> 1); i++) {
            set_fb_run(i, 0);
        }
        s = game_sprite_info(0, (int16_t)(-32 + (frame >> 1)), 0x4Bu);
        s.bitmap += ram_mode_phase() * 0x70u;
        draw(s);
    }
}

/*----------------------
 | marker_base
 | Description: What the markers' draw leaves in the ARM's r5: unchanged when no slot was in use, 0xFF when the last slot stepped was freed, else that slot's sprite flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: marker
 | Params: before -- r5 before the draw; s -- the last slot's sprite; last -- the last slot stepped, 0xFF for none
 | Returns: the register value
 ----------------------*/
static uint32_t marker_base(uint32_t before, SpriteInfo s, uint8_t last) {
    if (last == 0xFFu) {
        return before;
    }
    return ram_marker((uint8_t)(last * 4u)) == 0xFFu ? 0xFFu : s.flags;
}

/*----------------------
 | pair_number_draw
 | Description: Draws n right-aligned for five digits from x at y.
 | Author: suinevere
 | Dependencies: game_digits_x, game_number_draw, game_sprite_info
 | Globals: N/A
 | Params: x, y -- position; n -- the number
 | Returns: the digits sprite's flags, as the number draw leaves them in the ARM's r5
 ----------------------*/
static uint32_t pair_number_draw(int16_t x, int16_t y, uint32_t n) {
    x = (int16_t)game_digits_x((uint32_t)(int32_t)x, n);
    game_number_draw(x, y, n);
    return game_sprite_info(x, y, SPRITE_DIGITS).flags;
}

/*----------------------
 | game_reel_pair_screen
 | Description: Steps the two-reel game and draws it by mode_done: the two characters (pulled apart by mode_phase in state 0), the reels while they spin (1), the drawn symbols' panel (3), the payout with the lap points (6), the win and loss pictures (7, 8), the prize (9), the board from state 10, else the markers. When the game ended this frame (mode 3 or 14) slot 0 takes scratch_wait as its argument; the original's write of 0xFF to 0x74 past the value the last draw left in r5 lands in kernel RAM, which the VMU has none of, so it is dropped.
 | Author: suinevere
 | Dependencies: game_reel_pair_step, game_anims_tick, game_markers_draw, marker_base, game_anim_draw, game_reel_pair_spin_step, game_reel_anim_screen_draw, game_sprite_info, draw, pair_number_draw, game_rom_word_8094, game_icon_draw, game_board_draw, game_meter_draw
 | Globals: mode, mode_done, mode_phase, scratch_kinds, scratch_first, scratch_lo, scratch_hi, lap_points, bag_nth_index, scratch_wait, anim, marker
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reel_pair_screen(void) {
    SpriteInfo s;
    uint32_t base = 0;
    uint16_t entry;
    uint8_t last, phase, mode;
    game_reel_pair_step();
    switch (ram_mode_done()) {
    case 0:
        game_anims_tick();
        game_markers_draw(&last);
        phase = ram_mode_phase();
        game_anim_draw((int16_t)(-10 - phase), 7, 1);
        base = game_anim_draw((int16_t)(10 + phase), 7, 0).flags;
        break;
    case 1:
        game_reel_pair_spin_step();
        game_reel_anim_screen_draw();
        break;
    case 3:
        game_anims_tick();
        game_anim_draw(-10, 7, 1);
        game_anim_draw(10, 7, 0);
        s = game_sprite_info(0, 0, 0x48u);
        draw(s);
        s.x = (int16_t)(s.x + 0x10);
        s.bitmap += 0x38u;
        draw(s);
        s = game_sprite_info(0x18, 1, SPRITE_DIGITS);
        s.bitmap += (uint32_t)ram_scratch_kinds() << 2;
        draw(s);
        s = game_sprite_info(5, 1, SPRITE_DIGITS);
        s.bitmap += (uint32_t)ram_scratch_first() << 2;
        draw(s);
        base = s.flags;
        break;
    case 4:
        game_anims_tick();
        game_markers_draw(&last);
        base = game_anim_draw(0, 7, 0).flags;
        break;
    case 6:
        draw(game_sprite_info(0, 0, 0x4Cu));
        s = game_sprite_info(4, 0x0C, 0x37u);
        s.bitmap += 8u;
        draw(s);
        pair_number_draw((int16_t)(s.x + s.width), s.y, (uint32_t)(ram_scratch_lo() | (ram_scratch_hi() << 8)));
        base = pair_number_draw(10, 0x18, ram_lap_points());
        break;
    case 7:
        game_anims_tick();
        game_anim_draw(-10, 7, 1);
        base = game_anim_draw(10, 7, 0).flags;
        s = game_markers_draw(&last);
        base = marker_base(base, s, last);
        break;
    case 8:
        game_anims_tick();
        draw(game_sprite_info(0, 0, 0x49u));
        base = game_anim_draw(0, 7, 0).flags;
        s = game_markers_draw(&last);
        base = marker_base(base, s, last);
        break;
    case 9:
        entry = game_rom_word_8094(ram_bag_nth_index());
        base = game_icon_draw(0, 0, (uint8_t)entry, (uint8_t)((entry >> 8) + 1u)).flags;
        break;
    default:
        if (ram_mode_done() >= 10u) {
            game_board_draw();
            game_meter_draw(0);
        } else {
            game_markers_draw(&last);
        }
        break;
    }
    mode = ram_mode();
    if (mode == 3u || mode == 14u) {
        set_anim(1, ram_scratch_wait());
    }
}

/*----------------------
 | game_reel_game_screen
 | Description: Steps the reel game and draws it: the spinning reel screen while reel A spins (mode_done 2), the consolation item (5), else the markers.
 | Author: suinevere
 | Dependencies: game_reel_game_step, game_reel_spin_step, game_reel_screen_draw, item_icon_draw, game_markers_draw
 | Globals: mode_done, scratch_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reel_game_screen(void) {
    uint8_t last;
    game_reel_game_step();
    if (ram_mode_done() == 2u) {
        game_reel_spin_step();
        game_reel_screen_draw();
    } else if (ram_mode_done() == 5u) {
        item_icon_draw(0, ram_scratch_wait());
    } else {
        game_markers_draw(&last);
    }
}

/*----------------------
 | STRIP_C
 | Description: Address of reel strip C, drawn in the bottom reel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STRIP_C RAM_STRIP_C

/*----------------------
 | game_board_reel_screen
 | Description: One frame of the board with the reel window: spins reel A, steps the walk, draws the board, the window and the meter.
 | Author: suinevere
 | Dependencies: game_reel_spin_step, game_walk_step, game_board_draw, game_reel_window_show, game_meter_draw
 | Globals: N/A
 | Params: N/A
 | Returns: the meter's sprite
 ----------------------*/
SpriteInfo game_board_reel_screen(void) {
    uint32_t y = 0;
    game_reel_spin_step();
    game_walk_step();
    game_board_draw();
    game_reel_window_show(&y);
    return game_meter_draw(0);
}

/*----------------------
 | game_reel_race_screen
 | Description: Steps the reel race and draws it: the lap picture's two halves at half the race position while it runs (mode_done 3), the finish picture (4), else the three reels.
 | Author: suinevere
 | Dependencies: game_reel_race_step, game_wide_draw, game_sprite_info, draw, game_reel_draw
 | Globals: mode_done, scene_id, reel_pos_a, reel_pos_b, reel_pos_c, reel_len, strip_a, strip_b, strip_c
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reel_race_screen(void) {
    int16_t x;
    uint8_t lap;
    game_reel_race_step();
    if (ram_mode_done() == 3u) {
        x = (int16_t)((int16_t)vmu_r16(RAM_SCRATCH_WAIT) >> 1);
        lap = ram_scene_id();
        game_wide_draw(x, 0, (uint16_t)(lap * 2u));
        game_wide_draw((int16_t)(x + 16), 0, (uint16_t)(lap * 2u + 1u));
    } else if (ram_mode_done() == 4u) {
        draw(game_sprite_info(0, 0, 0x57u));
    } else {
        game_reel_draw(0, ram_reel_pos_a(), ram_reel_len(), STRIP_A);
        game_reel_draw(0x0B, ram_reel_pos_b(), ram_reel_len(), STRIP_B);
        game_reel_draw(0x16, ram_reel_pos_c(), ram_reel_len(), STRIP_C);
    }
}

/*----------------------
 | game_menu_two_screen
 | Description: Steps the two-row menu and draws its panel, the items held out of 50 and, while the blink shows it, the cursor on row 0 or 1.
 | Author: suinevere
 | Dependencies: game_menu_pick_two, game_sprite_info, draw, game_count_of_draw
 | Globals: mode_phase, mode_done, bag_count
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_menu_two_screen(void) {
    game_menu_pick_two();
    draw(game_sprite_info(0, 0, 0x42u));
    game_count_of_draw(6, 0x1A, ram_bag_count(), 0x32u);
    if (ram_mode_phase() == 0u) {
        draw(game_sprite_info(0, ram_mode_done() == 0u ? 3 : 0x0E, 0x33u));
    }
}

/*----------------------
 | bag_arrows_draw
 | Description: Draws the item menu's left and right arrows while the blink shows them, unless only one kind or item is on offer.
 | Author: suinevere
 | Dependencies: game_sprite_info, draw
 | Globals: mode_phase
 | Params: offered -- kinds or items on offer
 | Returns: N/A
 ----------------------*/
static void bag_arrows_draw(uint8_t offered) {
    SpriteInfo s;
    if (offered == 1u || ram_mode_phase() != 0u) {
        return;
    }
    s = game_sprite_info(0, 0x0C, 0x33u);
    s.flags |= SPRITE_MIRROR;
    draw(s);
    draw(game_sprite_info(0x18, 0x0C, 0x33u));
}

/*----------------------
 | game_bag_menu_screen
 | Description: Steps the item trade menu and draws it by mode_done: 1 the chosen item with its number and panel, 2 the trade question with the cursor (whose x, as in the original, is the row), 3 the give-up question with its lit answer and cursor, else the kind with its count of items and the total held; the arrows show while more than one is on offer. Only the cycle-timed runs open the menu.
 | Author: suinevere
 | Dependencies: game_bag_menu_step, item_icon_draw, game_icon_draw, game_small_number_draw, game_sprite_info, draw, bag_arrows_draw
 | Globals: mode_done, mode_phase, bag_items, bag_nth_index, bag_count, bag_kind_count, scratch_first, scratch_lo, scratch_count, scratch_wait, scene_id
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_bag_menu_screen(void) {
    SpriteInfo s;
    uint8_t row;
    game_bag_menu_step();
    switch (ram_mode_done()) {
    case 1:
        item_icon_draw(0, ram_bag_items(ram_bag_nth_index()));
        game_small_number_draw(0, 0x1A, (uint32_t)ram_scratch_count() + 1u);
        draw(game_sprite_info(0, 0, 0x47u));
        bag_arrows_draw(ram_scratch_lo());
        break;
    case 2:
        draw(game_sprite_info(0, 0, 0x46u));
        if (ram_mode_phase() == 0u) {
            row = ram_scene_id();
            draw(game_sprite_info(row, row == 0u ? 0 : 7, 0x33u));
        }
        break;
    case 3:
        row = ram_scene_id();
        s = game_sprite_info(0, 0, 0x4Du);
        if (row == 0u && ram_scratch_wait() <= 10u) {
            s.bitmap += 0x1Cu;
        }
        draw(s);
        s = game_sprite_info(0, 0, 0x4Eu);
        if (row == 1u && ram_scratch_wait() <= 10u) {
            s.bitmap += 0x18u;
        }
        draw(s);
        if (ram_mode_phase() == 0u) {
            draw(game_sprite_info(0, row == 0u ? 4 : 0x15, 0x33u));
        }
        break;
    default:
        game_icon_draw(0, 0, ram_scratch_first(), 0);
        game_small_number_draw(0, 0x1A, ram_scratch_lo());
        s = game_sprite_info(ram_bag_count() < 10u ? 0x1B : 0x17, 0x1A, SPRITE_DIGITS);
        s.bitmap += 0x28u;
        s.flags |= SPRITE_CLEAR;
        draw(s);
        game_small_number_draw(0x18, 0x1A, ram_bag_count());
        bag_arrows_draw(ram_bag_kind_count());
        break;
    }
}
