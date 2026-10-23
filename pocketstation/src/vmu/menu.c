/*----------------------
 | menu.c
 | Description: Ring Ring Land's menu choices: two yes/no toggles, a two-row and a three-row cursor, each blinking while it waits for a press.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: btn_new, btn_press, sel_a, sel_b, mode_done, mode_phase, mode_wait, anim, bag_count, bag_full, mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | BLINK_FRAMES
 | Description: Frames between blink toggles, and the wait a choice restarts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BLINK_FRAMES 12u

/*----------------------
 | blink
 | Description: Counts mode_wait down and flips mode_phase each time it runs out, restarting it at 12.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: mode_wait, mode_phase
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void blink(void) {
    uint8_t wait = (uint8_t)(ram_mode_wait() - 1u);
    if (wait == 0u) {
        set_mode_phase((uint8_t)(ram_mode_phase() ^ 1u));
        wait = BLINK_FRAMES;
    }
    set_mode_wait(wait);
}

/*----------------------
 | picked
 | Description: After a choice: requests sound 0 and restarts the blink with the item shown.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: sound_req, mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void picked(void) {
    game_sound_request(0);
    set_mode_phase(0);
    set_mode_wait(BLINK_FRAMES);
}

/*----------------------
 | game_menu_pick_a
 | Description: Toggles sel_a: a fresh right press sets it, a fresh left press clears it; otherwise blinks.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: btn_new, sel_a, sound_req, mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_menu_pick_a(void) {
    uint8_t fresh = ram_btn_new(), on = ram_sel_a();
    if ((on != 0u && (fresh & BTN_LEFT)) || (on == 0u && (fresh & BTN_RIGHT))) {
        set_sel_a(on != 0u ? 0u : 1u);
        picked();
        return;
    }
    blink();
}

/*----------------------
 | game_menu_pick_b
 | Description: Sets sel_b to 1 on a fresh up press or 0 on a fresh down press, showing anim byte 1 as (1 - sel_a) * 2 + sel_b; otherwise blinks. The down-press path is converted from the listing and unverified.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: btn_new, sel_a, sel_b, anim, sound_req, mode_phase, mode_wait
 | Params: N/A
 | Returns: 1 when sel_b was set, 0 when it blinked
 ----------------------*/
uint8_t game_menu_pick_b(void) {
    uint8_t on = ram_sel_b(), fresh = ram_btn_new(), pick;
    if ((fresh & BTN_UP) && on != 1u) {
        pick = 1u;
    } else if ((fresh & BTN_DOWN) && on != 0u) {
        pick = 0u;
    } else {
        blink();
        return 0u;
    }
    game_sound_request(0);
    set_sel_b(pick);
    set_anim(1u, (uint8_t)(((1u - ram_sel_a()) << 1) + pick));
    set_mode_phase(0);
    set_mode_wait(BLINK_FRAMES);
    return 1u;
}

/*----------------------
 | game_menu_pick_row
 | Description: Moves the three-row cursor in mode_done up or down with wrap-around on presses and auto-repeats; otherwise blinks.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: btn_press, mode_done, sound_req, mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_menu_pick_row(void) {
    uint8_t press = ram_btn_press(), row = ram_mode_done();
    if (press & BTN_UP) {
        row = row != 0u ? (uint8_t)(row - 1u) : 2u;
    } else if (press & BTN_DOWN) {
        row = row == 2u ? 0u : (uint8_t)(row + 1u);
    } else {
        blink();
        return;
    }
    set_mode_done(row);
    picked();
}

/*----------------------
 | game_menu_pick_two
 | Description: The two-row choice: up and down move mode_done between rows 0 and 1, otherwise it blinks; a press goes to mode 2 on row 0, or on row 1 to mode 14 with bag_full cleared when items are held, with sound 3 either way.
 | Author: suinevere
 | Dependencies: picked, blink, game_sound_begin
 | Globals: btn_new, mode_done, mode, bag_count, bag_full, mode_phase, mode_wait
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void game_menu_pick_two(void) {
    uint8_t buttons = ram_btn_new(), row = ram_mode_done();
    if ((buttons & BTN_UP) && row != 0u) {
        set_mode_done((uint8_t)(row - 1u));
        picked();
    } else if ((buttons & BTN_DOWN) && row != 1u) {
        set_mode_done((uint8_t)(row + 1u));
        picked();
    } else {
        blink();
    }
    if (!(ram_btn_new() & BTN_FIRE)) {
        return;
    }
    if (ram_mode_done() == 0u) {
        set_mode(2);
    } else if (ram_bag_count() != 0u) {
        set_bag_full(0);
        set_mode(14);
    }
    game_sound_begin(3);
}
