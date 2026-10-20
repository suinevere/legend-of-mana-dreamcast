/*----------------------
 | mode.c
 | Description: Ring Ring Land's mode-leave handlers, tabled at 0x020084B4 by the mode being left, and the frame step of a scene screen.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: mode, mode_phase, mode_wait, mode_done, scene_id, scratch_wait, scratch_first, scratch_count, lap_mark, bag_full, btn_new, sel_a, sel_b, sel_c, sel_d, sel_e
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
