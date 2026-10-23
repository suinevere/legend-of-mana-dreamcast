/*----------------------
 | frame.c
 | Description: Ring Ring Land's per-mode frame handlers, tabled at 0x02008474 by mode, and their dispatch: each draws its screen, then leaves a mode that changed or advances on a press.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: mode, mode_done, mode_wait, btn_new, reel_phase, walk_frame, walk_busy, meter_shown, meter, cell_saved, facing_saved, sel_b, sel_c, sel_d, scene_id, marker
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | game_mode0_frame
 | Description: Mode 0: the reel race.
 | Author: suinevere
 | Dependencies: game_reel_race_screen, game_mode_leave_changed
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode0_frame(void) {
    game_reel_race_screen();
    game_mode_leave_changed(0);
}

/*----------------------
 | game_mode1_frame
 | Description: Mode 1: the two-row menu.
 | Author: suinevere
 | Dependencies: game_menu_two_screen, game_mode_leave_changed
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode1_frame(void) {
    game_menu_two_screen();
    game_mode_leave_changed(1);
}

/*----------------------
 | game_mode2_frame
 | Description: Mode 2: the first yes/no toggle, advancing on a press.
 | Author: suinevere
 | Dependencies: game_menu_a_draw, game_mode_press
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode2_frame(void) {
    game_menu_a_draw();
    game_mode_press();
}

/*----------------------
 | game_mode3_frame
 | Description: Mode 3: walking by the buttons. Leaves a mode the walk changed; otherwise a press with the meter caught up either puts the walker back on its saved cell (when it moved or is still stepping) or advances, with sound 3 except on a new game.
 | Author: suinevere
 | Dependencies: game_board_walk_screen, game_mode_leave, game_mode_advance, game_sound_begin
 | Globals: mode, btn_new, meter_shown, meter, cell_saved, facing_saved, sel_b, sel_c, sel_d, walk_busy, walk_frame
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode3_frame(void) {
    game_board_walk_screen();
    if (ram_mode() != 3u) {
        game_mode_leave(ram_mode());
        return;
    }
    if (!(ram_btn_new() & BTN_FIRE) || ram_meter_shown() != ram_meter()) {
        return;
    }
    if (ram_cell_saved() != ram_sel_c() || ram_walk_busy() != 0u) {
        set_sel_c(ram_cell_saved());
        set_sel_d(ram_facing_saved());
        set_walk_busy(0);
        set_walk_frame(0);
        return;
    }
    if (ram_sel_b() != 2u) {
        game_sound_begin(3);
        game_mode_advance(0x5C4u);
    } else {
        game_mode_advance(0);
    }
}

/*----------------------
 | game_mode4_frame
 | Description: Mode 4: the board's yes/no choice, advancing on a press.
 | Author: suinevere
 | Dependencies: game_board_choice_draw, game_mode_press
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode4_frame(void) {
    game_board_choice_draw();
    game_mode_press();
}

/*----------------------
 | game_mode5_frame
 | Description: Mode 5: the board's 70-frame wait, then advance; the original compares the new mode with the wait, which never matches.
 | Author: suinevere
 | Dependencies: game_board_wait_screen, game_mode_advance
 | Globals: mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode5_frame(void) {
    if (game_board_wait_screen() == 0u) {
        game_mode_advance(ram_mode_wait());
    }
}

/*----------------------
 | game_mode6_frame
 | Description: Mode 6: the board with reel A, advancing once the reel stops (phase 8); as in the original the new mode's leave handler is skipped if it equals the meter's y.
 | Author: suinevere
 | Dependencies: game_board_reel_screen, game_mode_advance
 | Globals: reel_phase
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode6_frame(void) {
    SpriteInfo meter = game_board_reel_screen();
    if (ram_reel_phase() == 8u) {
        game_mode_advance((uint32_t)(int32_t)meter.y);
    }
}

/*----------------------
 | game_mode8_frame
 | Description: Mode 8: the item list screen; leaves a mode it changed, else advances on a press once the list is shown or the panel has fully lowered.
 | Author: suinevere
 | Dependencies: game_bag_screen_draw, game_mode_leave, game_mode_press
 | Globals: mode, mode_done, walk_frame
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode8_frame(void) {
    game_bag_screen_draw();
    if (ram_mode() != 8u) {
        game_mode_leave(ram_mode());
        return;
    }
    if (ram_mode_done() != 0u || ram_walk_frame() == 0x40u) {
        game_mode_press();
    }
}

/*----------------------
 | game_mode9_frame
 | Description: Mode 9: a scene.
 | Author: suinevere
 | Dependencies: game_scene_screen_draw, game_mode_leave_changed
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode9_frame(void) {
    game_scene_screen_draw();
    game_mode_leave_changed(9);
}

/*----------------------
 | game_mode10_frame
 | Description: Mode 10: the reel game.
 | Author: suinevere
 | Dependencies: game_reel_game_screen, game_mode_leave_changed
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode10_frame(void) {
    game_reel_game_screen();
    game_mode_leave_changed(10);
}

/*----------------------
 | game_mode11_frame
 | Description: Mode 11: the two-reel game.
 | Author: suinevere
 | Dependencies: game_reel_pair_screen, game_mode_leave_changed
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode11_frame(void) {
    game_reel_pair_screen();
    game_mode_leave_changed(11);
}

/*----------------------
 | mode12_frame
 | Description: Mode 12: draws the markers, runs out mode_wait, then advances on a press. No script reaches mode 12 and R1 never lifted it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_markers_draw, game_mode_advance
 | Globals: mode_wait, btn_new, marker
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void mode12_frame(void) {
    uint8_t last;
    game_markers_draw(&last);
    if (ram_mode_wait() != 0u) {
        set_mode_wait((uint8_t)(ram_mode_wait() - 1u));
        return;
    }
    if (ram_btn_new() & BTN_FIRE) {
        game_mode_advance(ram_btn_new());
    }
}

/*----------------------
 | game_mode13_frame
 | Description: Mode 13: the three-row menu, advancing on a press.
 | Author: suinevere
 | Dependencies: game_menu_row_draw, game_mode_press
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode13_frame(void) {
    game_menu_row_draw();
    game_mode_press();
}

/*----------------------
 | game_mode14_frame
 | Description: Mode 14: the item trade menu.
 | Author: suinevere
 | Dependencies: game_bag_menu_screen, game_mode_leave_changed
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode14_frame(void) {
    game_bag_menu_screen();
    game_mode_leave_changed(14);
}

/*----------------------
 | mode15_frame
 | Description: Mode 15: runs out mode_wait and on a press adds the item in scene_id, then shows that item with three marks beside the character scene_id picked. No script reaches mode 15 and R1 never lifted it: converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: game_bag_add, game_anims_tick, game_rom_word_8094, game_icon_draw, game_anim_draw, game_mode_leave_changed
 | Globals: mode_wait, btn_new, scene_id, mode_done
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void mode15_frame(void) {
    uint8_t wait = ram_mode_wait(), ready = 1;
    if (wait != 0u) {
        set_mode_wait((uint8_t)(wait - 1u));
        ready = wait == 1u;
    }
    if (ready && (ram_btn_new() & BTN_FIRE)) {
        game_bag_add(ram_scene_id());
    }
    game_anims_tick();
    game_icon_draw(5, 0, 3, (uint8_t)((game_rom_word_8094(ram_scene_id()) >> 8) + 1u));
    game_anim_draw(-6, 6, (uint8_t)(ram_mode_done() + 5u));
    game_mode_leave_changed(15);
}

/*----------------------
 | game_mode_frame
 | Description: Runs the current mode's frame handler, as the card's table at 0x02008474 lists them.
 | Author: suinevere
 | Dependencies: the mode frame handlers, game_board_auto_frame
 | Globals: mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_mode_frame(void) {
    static void (*const frame[16])(void) = {
        game_mode0_frame, game_mode1_frame, game_mode2_frame, game_mode3_frame, game_mode4_frame, game_mode5_frame,
        game_mode6_frame, game_board_auto_frame, game_mode8_frame, game_mode9_frame, game_mode10_frame, game_mode11_frame,
        mode12_frame, game_mode13_frame, game_mode14_frame, mode15_frame,
    };
    frame[ram_mode() & 15u]();
}
