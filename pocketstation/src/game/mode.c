/*----------------------
 | mode.c
 | Description: Ring Ring Land's mode-leave handlers, tabled at 0x020084B4 by the mode being left, their dispatch, the frame steps of a scene screen, the reel game and the reel race, and a board event's shake.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: mode, mode_phase, mode_wait, mode_done, scene_id, scratch_wait, scratch_first, scratch_count, lap_mark, bag_full, btn_new, sel_a, sel_b, sel_c, sel_d, sel_e, cell_saved, facing_saved, walk_busy, odds_set, steps, bonus_done, walk_behind, meter_max, meter_shown, meter, meter_delay, bag_cursor, bag_count, lap_points, pet_kind, strip_c_len, bag_items, scratch_kinds, btn_new, reel_phase, reel_len, reel_pos_a, strip_a, scratch_first, scratch_count, walk_left, walk_back, scratch_lo, scratch_hi, reel_pos_b, reel_pos_c, wait_hi, bag_nth_index
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | restart_wait
 | Description: Starts the next mode at phase 0 with a 12-frame wait.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void restart_wait(void) {
    set_mode_phase(0);
    set_mode_wait(12);
}

/*----------------------
 | game_mode1_leave
 | Description: Leaving mode 1: clears mode_done and restarts the wait.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: mode_done, mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode1_leave(void) {
    set_mode_done(0);
    restart_wait();
}

/*----------------------
 | game_mode2_leave
 | Description: Leaving mode 2: sets sel_a and restarts the wait.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: sel_a, mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode2_leave(void) {
    set_sel_a(1);
    restart_wait();
}

/*----------------------
 | game_mode4_leave
 | Description: Leaving mode 4: clears sel_b and restarts the wait.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: sel_b, mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode4_leave(void) {
    set_sel_b(0);
    restart_wait();
}

/*----------------------
 | game_mode5_leave
 | Description: Leaving mode 5: clears the wait.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode5_leave(void) {
    set_mode_wait(0);
}

/*----------------------
 | game_mode7_leave
 | Description: Leaving mode 7: sets sel_e and mode_done.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: sel_e, mode_done
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode7_leave(void) {
    set_sel_e(1);
    set_mode_done(1);
}

/*----------------------
 | game_mode13_leave
 | Description: Leaving mode 13: sel_d/sel_c become 8/0 when sel_a is clear, else 4/0x10; sel_b becomes 2, then the wait restarts.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: sel_a, sel_b, sel_c, sel_d, mode_done, mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode13_leave(void) {
    if (ram_sel_a() == 0) {
        set_sel_d(8);
        set_sel_c(0);
    } else {
        set_sel_d(4);
        set_sel_c(0x10);
    }
    set_sel_b(2);
    set_mode_done(0);
    restart_wait();
}

/*----------------------
 | SCENE_SCROLL_END
 | Description: Where scene 1's scroll in scratch_first stops.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SCENE_SCROLL_END 0x68u

/*----------------------
 | game_mode0_leave
 | Description: Leaving mode 0: clears mode and mode_done, waits 30 frames and loads the reels.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: mode, mode_done, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode0_leave(void) {
    set_mode(0);
    set_mode_done(0);
    set_mode_wait(30u);
    game_reels_load();
}

/*----------------------
 | game_mode10_leave
 | Description: Leaving mode 10: clears mode_done, waits 5 frames and shows marker type 0x5B alone in slot 0.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: mode_done, mode_wait, marker
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode10_leave(void) {
    set_mode_done(0);
    set_mode_wait(5u);
    game_markers_clear();
    game_marker_start(0, 0x5Bu, 0);
}

/*----------------------
 | game_mode14_leave
 | Description: Leaving mode 14: mode_done becomes 3 with the item list full, else 0; a 20-frame countdown starts, the wait restarts and the list's kinds are counted. No script reaches it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: bag_full, mode_done, scene_id, scratch_wait, mode_phase, mode_wait, scratch_kinds, scratch_first, scratch_lo
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode14_leave(void) {
    set_mode_done(ram_bag_full() != 0u ? 3u : 0u);
    set_scene_id(0);
    set_scratch_wait(20u);
    restart_wait();
    game_bag_kinds();
    game_bag_count_first();
}

/*----------------------
 | game_scene_step
 | Description: A scene screen's frame: fire after mode_wait sets mode 3, but scene 1 first scrolls scratch_first to 0x68 and scene 2 raises lap_mark after 40 frames. Scene 2 and the scroll's step are converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: mode_wait, btn_new, scene_id, mode_done, mode, scratch_wait, scratch_first, scratch_count, lap_mark
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_scene_step(void) {
    uint8_t wait = ram_mode_wait(), fire = ram_btn_new() & BTN_FIRE, scene = ram_scene_id(), started = ram_mode_done() == 1u;
    uint8_t ready, count;
    if (wait != 0u) {
        wait--;
        set_mode_wait(wait);
    }
    ready = wait == 0u && fire != 0u;
    if (scene == 1u && !started) {
        if (ready) {
            set_scratch_first(ram_scratch_wait() == 0xFFu ? SCENE_SCROLL_END : 0u);
            set_mode_done(1);
            set_scratch_count(20u);
        }
        return;
    }
    if (scene == 1u) {
        if (ram_scratch_first() != SCENE_SCROLL_END) {
            set_scratch_first((uint8_t)(ram_scratch_first() + 1u));
        }
        count = (uint8_t)(ram_scratch_count() - 1u);
        set_scratch_count(count == 0u ? 20u : count);
    } else if (scene == 2u && !started) {
        if (ready) {
            set_scratch_count(40u);
            set_mode_done(1);
        }
        return;
    } else if (scene == 2u) {
        count = ram_scratch_count();
        if (count != 0u) {
            set_scratch_count(--count);
            if (count == 0u) {
                set_lap_mark(ram_lap_mark() == 9u ? 9u : (uint8_t)(ram_lap_mark() + 1u));
                game_reels_load_raised();
                game_sound_request(1);
            }
            return;
        }
        ready = fire != 0u;
    }
    if (ready) {
        set_mode(3);
    }
}

/*----------------------
 | game_mode3_leave
 | Description: Leaving mode 3: saves the cell and facing and clears sel_e and walk_busy; on a new game (sel_b 2) it starts the lap at odds set mode_done with a full meter, the first character and a fresh board, otherwise it enters the board once steps reaches 32 per odds set step.
 | Author: suinevere
 | Dependencies: game_board_enter, game_anims_clear, game_anim_start, game_random, game_strip_load, game_board_fill
 | Globals: sel_a, sel_b, sel_c, sel_d, sel_e, cell_saved, facing_saved, walk_busy, odds_set, mode_done, steps, bonus_done, walk_behind, meter_max, meter_shown, meter, meter_delay, bag_cursor, bag_count, lap_points, pet_kind, lap_mark, strip_c_len, mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode3_leave(void) {
    set_cell_saved(ram_sel_c());
    set_facing_saved(ram_sel_d());
    set_sel_e(0);
    set_walk_busy(0);
    if (ram_sel_b() != 2u) {
        if (ram_steps() == ((uint32_t)ram_odds_set() + 1u) << 5) {
            set_mode(8);
            game_board_enter();
        }
        return;
    }
    set_odds_set(ram_mode_done());
    set_bonus_done(0);
    set_steps(0);
    set_walk_behind(0);
    set_meter_max(3);
    set_meter_shown(3);
    set_meter(3);
    set_meter_delay(0);
    set_bag_cursor((uint8_t)ram_bag_count());
    set_lap_points(0);
    game_anims_clear();
    game_anim_start(0, (uint8_t)(ram_pet_kind() + 8u), (uint8_t)((1u - ram_sel_a()) << 1));
    set_lap_mark(0);
    set_strip_c_len((uint8_t)game_strip_load((uint16_t)((game_random(0x40u) >> 4) + 4u), 0x564u, 0));
    game_board_fill();
}

/*----------------------
 | game_mode9_leave
 | Description: Leaving mode 9 after a scene: waits 5 frames and clears mode_done; scene 0 refills the meter by one (raising its top up to 9 when full), scene 1 swaps a random item for a new pick, others raise the reels; then shows marker 0x58 + scene_id alone.
 | Author: suinevere
 | Dependencies: game_random, game_bag_remove, game_pick_item, game_reels_load_raised, game_markers_clear, game_marker_start
 | Globals: mode_wait, mode_done, scene_id, meter_max, meter_shown, meter, meter_delay, bag_count, bag_items, scratch_wait, scratch_kinds
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode9_leave(void) {
    uint8_t scene, top, item;
    uint32_t count;
    set_mode_wait(5);
    set_mode_done(0);
    scene = ram_scene_id();
    if (scene == 0u) {
        top = ram_meter_max();
        if (top != ram_meter_shown()) {
            top = (uint8_t)(ram_meter_shown() + 1u);
        } else if (top != 9u) {
            top++;
            set_meter_max(top);
        }
        set_meter(top);
        set_meter_delay(0);
    } else if (scene == 1u) {
        count = ram_bag_count();
        if (count != 0u) {
            uint8_t at = (uint8_t)(game_random((uint16_t)(count << 4)) >> 4);
            set_scratch_wait(ram_bag_items(at));
            game_bag_remove(at, 1);
        } else {
            set_scratch_wait(0xFFu);
        }
        item = (uint8_t)game_pick_item(3);
        set_scratch_kinds(item);
        count = ram_bag_count() + 1u;
        set_bag_count(count);
        set_bag_items((uint8_t)(count - 1u), item);
    } else {
        game_reels_load_raised();
    }
    game_markers_clear();
    game_marker_start(0, (uint8_t)(ram_scene_id() + 0x58u), 0);
}

/*----------------------
 | reel_miss
 | Description: The reel game's miss: mode_done 5 after 5 frames, with a consolation item picked into scratch_wait.
 | Author: suinevere
 | Dependencies: game_pick_item
 | Globals: mode_done, mode_wait, scratch_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void reel_miss(void) {
    set_mode_done(5);
    set_mode_wait(5);
    set_scratch_wait((uint8_t)game_pick_item(0));
}

/*----------------------
 | reel_marker
 | Description: Moves the reel game to state done after wait frames and shows marker type alone.
 | Author: suinevere
 | Dependencies: game_markers_clear, game_marker_start
 | Globals: mode_done, mode_wait, marker
 | Params: done -- next state; wait -- frames; type -- marker type
 | Returns: N/A
 ----------------------*/
static void reel_marker(uint8_t done, uint8_t wait, uint8_t type) {
    set_mode_done(done);
    set_mode_wait(wait);
    game_markers_clear();
    game_marker_start(0, type, 0);
}

/*----------------------
 | game_reel_game_step
 | Description: One frame of the reel game, by mode_done: 0 a press rolls the start (1 in 5 misses), 1 a press spins the reels, 2 waits for reel A to stop and checks its symbol against the three drawn, 3 a press costs a meter point, 4 a press sends the walker back 3-5 cells, 5 a press takes the consolation item; any other state returns to mode 3. A stopped matching reel wins a meter refill (1 in 5), a walk (1 in 2) or a random event.
 | Author: suinevere
 | Dependencies: game_random, reel_marker, reel_miss, game_reels_load_pick, game_strip_next_symbol, game_event_roll, game_bag_add
 | Globals: btn_new, mode, mode_done, mode_wait, reel_phase, reel_len, reel_pos_a, strip_a, scratch_kinds, scratch_first, scratch_count, scratch_wait, meter_shown, meter, meter_delay, walk_left, walk_back
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reel_game_step(void) {
    uint8_t state = ram_mode_done(), pressed = ram_btn_new() & 1u, symbol, roll;
    uint8_t drawn[3];
    if (state == 2u) {
        if (ram_reel_phase() != 8u) {
            return;
        }
        symbol = game_strip_next_symbol(ram_reel_len(), ram_reel_pos_a(), 0x544u);
        if (symbol != ram_scratch_kinds() && symbol != ram_scratch_first() && symbol != ram_scratch_count()) {
            reel_miss();
            return;
        }
        roll = (uint8_t)game_random(100u);
        if (roll < 20u) {
            reel_marker(3, 5, 0x5Du);
        } else if (roll < 70u) {
            reel_marker(4, 10, 0x5Eu);
        } else {
            game_event_roll();
            set_mode(11);
        }
        return;
    }
    if (ram_mode_wait() != 0u) {
        set_mode_wait((uint8_t)(ram_mode_wait() - 1u));
        return;
    }
    if (state > 5u) {
        set_mode(3);
        return;
    }
    if (!pressed) {
        return;
    }
    if (state == 0u) {
        if (game_random(100u) >= 80u) {
            reel_miss();
        } else {
            reel_marker(1, 5, 0x5Cu);
        }
    } else if (state == 1u) {
        set_mode_done(2);
        game_reels_load_pick(drawn);
    } else if (state == 3u) {
        set_meter((uint8_t)(ram_meter_shown() - 1u));
        set_meter_delay(0);
        set_mode(3);
    } else if (state == 4u) {
        set_walk_left((uint8_t)((game_random(0x30u) >> 4) + 3u));
        set_walk_back(1);
        set_mode(7);
    } else {
        game_bag_add(ram_scratch_wait());
    }
}

/*----------------------
 | game_mode11_leave
 | Description: Leaving mode 11 for a scene: saves the cell and facing, clears sel_e and the kind count, and keeps slot 0's argument in scratch_wait. Scene 13 starts the reel slide with the second character; any other scene notes whether pet_kind and the scene match one way (1) or the other (2), picks a bag item, starts the scene's character with three markers and sound 2.
 | Author: suinevere
 | Dependencies: game_byte_is, game_pick_bag_nth, game_anim_start, game_markers_clear, game_marker_start, game_sound_begin
 | Globals: sel_c, sel_d, sel_e, cell_saved, facing_saved, bag_kind_count, scene_id, pet_kind, anim, scratch_wait, scratch_count, scratch_lo, scratch_hi, mode_done, mode_wait, mode_phase
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode11_leave(void) {
    uint8_t scene, other;
    set_sel_e(0);
    set_cell_saved(ram_sel_c());
    set_facing_saved(ram_sel_d());
    set_bag_kind_count(0);
    scene = ram_scene_id();
    if (scene == 13u) {
        set_scratch_count(4);
        set_mode_done(10);
        set_mode_wait(0x20);
        set_mode_phase(0x40);
        set_scratch_lo(0);
        set_scratch_hi(0);
        set_scratch_wait(ram_anim(1));
        game_anim_start(1, 0x15u, 0);
        return;
    }
    other = ram_pet_kind();
    if (game_byte_is(other, scene)) {
        set_bag_kind_count(1);
    } else if (game_byte_is(scene, other)) {
        set_bag_kind_count(2);
    }
    game_pick_bag_nth();
    set_mode_done(0);
    set_scratch_wait(ram_anim(1));
    set_anim(1, 0);
    game_anim_start(1, (uint8_t)(ram_scene_id() + 8u), 2);
    set_mode_wait(0x28);
    set_mode_phase(0x14);
    game_markers_clear();
    game_marker_start(0, 0x62u, 0);
    game_marker_start(1, 0x69u, 0);
    game_marker_start(2, 0x63u, 0);
    game_sound_begin(2);
}

/*----------------------
 | shake_offset
 | Description: The shake for a countdown value: 0, 2, 3, 2 by its low two bits, negated when bit 2 is set.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: wait -- countdown
 | Returns: the offset as a byte
 ----------------------*/
static uint8_t shake_offset(uint32_t wait) {
    uint8_t q = (uint8_t)(wait & 3u);
    q = q == 0u ? 0u : q == 2u ? 3u : 2u;
    return (wait & 4u) ? (uint8_t)(0u - q) : q;
}

/*----------------------
 | game_event_shake
 | Description: One frame of a board event's shake. Type 11 counts mode_wait down flipping slot 1's frame, sliding mode_phase in, then shaking scratch_hi, and at 0 starts the scene with three markers and sound 2; other types shake scratch_lo until mode_wait runs out, then set mode_done 11 with an 80-frame wait. No script rolls an event: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: shake_offset, game_markers_clear, game_marker_start, game_sound_begin
 | Globals: mode_wait, mode_done, mode_phase, scratch_lo, scratch_hi, anim
 | Params: type -- the event
 | Returns: 0 once type 11's scene starts, else 1
 ----------------------*/
uint8_t game_event_shake(uint8_t type) {
    uint32_t wait = ram_mode_wait();
    uint8_t frame;
    if (type != 11u) {
        if (wait == 0u) {
            set_mode_done(11);
            set_scratch_lo(0);
            set_mode_wait(0x50);
            return 1;
        }
        wait--;
        set_mode_wait((uint8_t)wait);
        set_scratch_lo(shake_offset(wait));
        return 1;
    }
    if (wait == 0u) {
        set_anim(1, 0);
        set_anim(5, 2);
        set_mode_wait(0x28);
        set_mode_phase(0x14);
        game_markers_clear();
        game_marker_start(0, 0x62u, 0);
        game_marker_start(1, 0x69u, 0);
        game_marker_start(2, 0x63u, 0);
        game_sound_begin(2);
        return 0;
    }
    wait--;
    set_mode_wait((uint8_t)wait);
    frame = (uint8_t)((wait >> 1) & 3u);
    set_anim(5, frame < 2u ? (uint8_t)(frame ^ 1u) : frame);
    if (wait >= 16u) {
        set_mode_phase((uint8_t)((wait - 16u) >> 1));
    } else {
        set_scratch_hi(shake_offset(wait));
        set_mode_phase(0);
    }
    return 1;
}

/*----------------------
 | RACE_POS / RACE_LIMIT
 | Description: The reel race's position and finish line, signed 16-bit words over scratch_wait-scratch_kinds and scratch_first-scratch_count.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RACE_POS 0x3DAu
#define RACE_LIMIT 0x3DCu

/*----------------------
 | race_reset
 | Description: Puts the three reels back at their start and waits 240 frames in state 2.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: reel_pos_a, reel_pos_b, reel_pos_c, mode_wait, mode_done
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void race_reset(void) {
    set_reel_pos_a(0);
    set_reel_pos_b(0);
    set_reel_pos_c(0);
    set_mode_wait(0xF0);
    set_mode_done(2);
}

/*----------------------
 | race_lap
 | Description: Starts lap lap of the reel race: scene_id holds it, the position restarts at 64 and the finish line sits twice the lap picture's width plus 64 to the left.
 | Author: suinevere
 | Dependencies: game_sprite_info
 | Globals: scene_id, mode_done
 | Params: lap -- lap number
 | Returns: N/A
 ----------------------*/
static void race_lap(uint8_t lap) {
    SpriteInfo s;
    set_scene_id(lap);
    s = game_sprite_info(0, 0, (uint16_t)(lap * 2u + 0x50u));
    psk_w16(RACE_LIMIT, (uint16_t)(0u - ((s.width + 0x20u) << 1)));
    psk_w16(RACE_POS, 0x40u);
    set_mode_done(3);
}

/*----------------------
 | race_wait
 | Description: Counts mode_wait down.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: mode_wait
 | Params: N/A
 | Returns: 1 when it reached 0, else 0
 ----------------------*/
static uint8_t race_wait(void) {
    uint8_t wait = (uint8_t)(ram_mode_wait() - 1u);
    set_mode_wait(wait);
    return wait == 0u;
}

/*----------------------
 | game_reel_race_step
 | Description: One frame of the reel race, by mode_done: 0 waits then loads the reels, 1 steps them until they reach phase 4, 2 and 5 wait (a press quits to mode 1) before lap 0 or a 30-frame restart, 3 moves the lap picture left to its finish line and on to the next lap until the fourth, 4 waits 120 frames then resets. A press in any other state cuts it short with sound 3 and a reset.
 | Author: suinevere
 | Dependencies: game_reels_load, game_reels_step, game_sound_begin, race_reset, race_lap, race_wait
 | Globals: btn_new, mode, mode_done, mode_wait, scene_id, reel_phase, reel_pos_a, reel_pos_b, reel_pos_c
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reel_race_step(void) {
    uint8_t state = ram_mode_done(), pressed = ram_btn_new() & 1u, lap;
    int32_t pos;
    if (pressed && state != 2u && state != 5u) {
        game_sound_begin(3);
        race_reset();
        return;
    }
    switch (state) {
    case 1:
        game_reels_step();
        if (ram_reel_phase() == 4u) {
            race_reset();
        }
        return;
    case 2:
    case 5:
        if (pressed) {
            set_mode(1);
            game_sound_begin(3);
            return;
        }
        if (!race_wait()) {
            return;
        }
        if (state == 2u) {
            race_lap(0);
        } else {
            set_mode_wait(0x1E);
            set_mode_done(0);
        }
        return;
    case 3:
        pos = (int16_t)psk_r16(RACE_POS) - 1;
        psk_w16(RACE_POS, (uint16_t)pos);
        if (pos >= (int16_t)psk_r16(RACE_LIMIT)) {
            return;
        }
        lap = (uint8_t)(ram_scene_id() + 1u);
        if (lap != 4u) {
            race_lap(lap);
            return;
        }
        set_mode_wait(0x78);
        set_mode_done(4);
        return;
    case 4:
        if (race_wait()) {
            set_reel_pos_a(0);
            set_reel_pos_b(0);
            set_reel_pos_c(0);
            set_mode_wait(0xF0);
            set_mode_done(5);
        }
        return;
    default:
        if (race_wait()) {
            game_reels_load();
            set_mode_done((uint8_t)(state + 1u));
        }
        return;
    }
}

/*----------------------
 | mode12_leave
 | Description: Leaving mode 12: waits 5 frames and shows marker 0x61 alone. No script reaches mode 12 and R1 never lifted it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_markers_clear, game_marker_start
 | Globals: mode_wait, marker
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void mode12_leave(void) {
    set_mode_wait(5);
    game_markers_clear();
    game_marker_start(0, 0x61u, 0);
}

/*----------------------
 | mode15_leave
 | Description: Leaving mode 15: scene_id becomes an item picked with method 2 and the wait 5 frames. No script reaches mode 15 and R1 never lifted it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_pick_item
 | Globals: scene_id, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void mode15_leave(void) {
    set_scene_id((uint8_t)game_pick_item(2));
    set_mode_wait(5);
}

/*----------------------
 | game_mode_leave
 | Description: Runs the leave handler of mode, as the card's table at 0x020084B4 lists them.
 | Author: suinevere
 | Dependencies: the mode leave handlers, game_reels_load_single, game_board_enter
 | Globals: N/A
 | Params: mode -- the mode being left, 0-15
 | Returns: N/A
 ----------------------*/
void game_mode_leave(uint8_t mode) {
    static void (*const leave[16])(void) = {
        game_mode0_leave, game_mode1_leave, game_mode2_leave, game_mode3_leave, game_mode4_leave, game_mode5_leave,
        game_reels_load_single, game_mode7_leave, game_board_enter, game_mode9_leave, game_mode10_leave, game_mode11_leave,
        mode12_leave, game_mode13_leave, game_mode14_leave, mode15_leave,
    };
    leave[mode & 15u]();
}

/*----------------------
 | pair_end
 | Description: Ends the two-reel game: back to the board in mode 3, or mode 8 from scene 13.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: scene_id, mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void pair_end(void) {
    set_mode(ram_scene_id() == 13u ? 8u : 3u);
}

/*----------------------
 | game_reel_pair_step
 | Description: One frame of the two-reel game, by mode_done: states 3-9 first run out mode_wait; 0 slides in and loads the reels once marker 0 is done, 1 waits for them to stop and records the symbols after, 3 compares the two on a press (a win, a draw or a loss marker), 4 pays out, 5-8 wait for presses, a draw (7) or loss (8, costing a meter point and another from scene 13) end the game, 6 ends it 3 times in 4 or picks a prize, 9 adds the prize; other states shake the board until the event ends. Reaching state 3 or later restarts a 5-frame wait.
 | Author: suinevere
 | Dependencies: game_marker_used, game_reels_load_pair, game_reels_next_two, game_markers_clear, game_marker_start, game_payout, game_random, game_pick_item, game_bag_add, game_event_shake, pair_end
 | Globals: mode_done, mode_wait, wait_hi, mode_phase, btn_new, reel_phase, scratch_kinds, scratch_first, anim, scene_id, bag_nth_index, meter_shown, meter, meter_delay
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_reel_pair_step(void) {
    uint8_t state = ram_mode_done(), pressed = ram_btn_new() & 1u, done, kinds, first, roll;
    uint32_t point;
    if (state >= 3u && state < 10u && ram_mode_wait() != 0u) {
        set_mode_wait((uint8_t)(ram_mode_wait() - 1u));
        return;
    }
    if (state >= 3u && state <= 9u && !pressed) {
        return;
    }
    switch (state) {
    case 0:
        if (ram_mode_wait() != 0u) {
            set_mode_wait((uint8_t)(ram_mode_wait() - 1u));
        } else if (ram_mode_phase() != 0u) {
            set_mode_phase((uint8_t)(ram_mode_phase() - 2u));
        }
        if (game_marker_used(0)) {
            return;
        }
        game_reels_load_pair(&roll);
        done = 1;
        break;
    case 1:
        if (ram_reel_phase() != 8u) {
            return;
        }
        game_reels_next_two();
        done = 3;
        break;
    case 3:
        game_markers_clear();
        kinds = ram_scratch_kinds();
        first = ram_scratch_first();
        if (kinds == first) {
            game_marker_start(0, 0x60u, 0);
            done = 7;
        } else if (kinds < first) {
            game_marker_start(0, 0x64u, 0);
            game_marker_start(1, 0x65u, 0);
            game_marker_start(2, 0x6Au, 0);
            set_anim(1, 1);
            done = 8;
        } else {
            game_marker_start(0, 0x5Fu, 0);
            done = 4;
        }
        break;
    case 4:
        game_payout();
        set_wait_hi(0x40);
        done = 5;
        break;
    case 5:
        done = 6;
        break;
    case 6:
        if (ram_scene_id() != 13u && game_random(100u) >= 25u) {
            pair_end();
            return;
        }
        set_bag_nth_index((uint8_t)game_pick_item(1));
        done = 9;
        break;
    case 7:
        pair_end();
        return;
    case 8:
        set_meter_delay(0);
        point = ram_meter_shown() - 1u;
        if (point != 0u && ram_scene_id() == 13u) {
            point--;
        }
        set_meter((uint8_t)point);
        pair_end();
        return;
    case 9:
        game_bag_add(ram_bag_nth_index());
        return;
    default:
        if (game_event_shake(state) != 0u) {
            return;
        }
        done = 0;
        break;
    }
    set_mode_done(done);
    if (done >= 3u) {
        set_mode_wait(5);
    }
}

/*----------------------
 | game_mode_leave_changed
 | Description: Runs the current mode's leave handler unless the mode equals was.
 | Author: suinevere
 | Dependencies: game_mode_leave
 | Globals: mode
 | Params: was -- the value the original compares the mode with
 | Returns: N/A
 ----------------------*/
void game_mode_leave_changed(uint32_t was) {
    if (ram_mode() != was) {
        game_mode_leave(ram_mode());
    }
}

/*----------------------
 | game_mode_advance
 | Description: Moves to the mode after the current one and runs its leave handler: 2 goes to 13, 3 to 4 on a new game (sel_b 2) or else 6, 8 to 0, 0-6 to the next, others to 3; mode 12 resets the state block and parks the mode at 0xFF instead.
 | Author: suinevere
 | Dependencies: game_state_reset, game_mode_leave_changed
 | Globals: mode, sel_b
 | Params: was -- compared with the new mode as the original does; its callers pass 0x5C4, which no mode matches, and mode 3 compares with sel_b
 | Returns: N/A
 ----------------------*/
void game_mode_advance(uint32_t was) {
    uint8_t mode = ram_mode(), next;
    if (mode == 12u) {
        game_state_reset();
        set_mode(0xFFu);
        return;
    }
    if (mode == 2u) {
        next = 13;
    } else if (mode == 3u) {
        was = ram_sel_b();
        next = was == 2u ? 4u : 6u;
    } else if (mode == 8u) {
        next = 0;
    } else if (mode < 7u) {
        next = (uint8_t)(mode + 1u);
    } else {
        next = 3;
    }
    set_mode(next);
    game_mode_leave_changed(was);
}

/*----------------------
 | game_mode_press
 | Description: The end of a screen's frame: a press plays sound 3 and advances the mode.
 | Author: suinevere
 | Dependencies: game_sound_begin, game_mode_advance
 | Globals: btn_new
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode_press(void) {
    if (!(ram_btn_new() & BTN_FIRE)) {
        return;
    }
    game_sound_begin(3);
    game_mode_advance(0x5C4u);
}

/*----------------------
 | game_board_auto_frame
 | Description: One frame of an automatic walk: steps and draws it, and once the walk has stopped (walk_left and walk_busy clear) leaves for the mode its cell's event chose.
 | Author: suinevere
 | Dependencies: game_board_auto_screen, game_cell_event, game_mode_leave
 | Globals: walk_left, walk_busy
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_board_auto_frame(void) {
    game_board_auto_screen();
    if (ram_walk_left() != 0u || ram_walk_busy() != 0u) {
        return;
    }
    game_mode_leave(game_cell_event());
}
