/*----------------------
 | input.c
 | Description: Ring Ring Land's buttons: their setup, the interrupt that latches presses and the per-frame read with edges, auto-repeat and the long fire hold.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: btn_prev, btn_new, btn_press, btn_latch, hold_reload, hold_count, repeat_up, repeat_down, repeat_left, repeat_right, irq_handler
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | repeat_step
 | Description: Runs one direction's auto-repeat: a new press arms 0x16 frames, a held button counts down and repeats every 6 frames.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: count -- the direction's countdown; bit -- its button; held, fresh -- buttons held and newly pressed; press -- presses to add a repeat to
 | Returns: the new countdown
 ----------------------*/
static uint8_t repeat_step(uint8_t count, uint8_t bit, uint8_t held, uint8_t fresh, uint8_t *press) {
    if (!(held & bit)) {
        return count;
    }
    if (fresh & bit) {
        return 0x16u;
    }
    count = (uint8_t)(count - 1u);
    if (count == 0) {
        *press = (uint8_t)(*press | bit);
        return 6u;
    }
    return count;
}

/*----------------------
 | game_read_buttons
 | Description: Takes the buttons the interrupt latched, records edges and auto-repeats, and re-enables the button interrupts.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: btn_prev, btn_new, btn_press, btn_latch, hold_reload, hold_count, repeat_*
 | Params: check_hold -- nonzero to count down a long hold of fire alone
 | Returns: 1 when a long fire hold just completed, else 0
 ----------------------*/
uint8_t game_read_buttons(uint8_t check_hold) {
    uint8_t held = ram_btn_latch(), fresh, press, hold, done = 0;
    if (check_hold && held == BTN_FIRE) {
        hold = (uint8_t)(ram_hold_count() - 1u);
        if (hold == 0) {
            done = 1;
            hold = ram_hold_reload();
        }
    } else {
        hold = ram_hold_reload();
    }
    set_hold_count(hold);
    fresh = (uint8_t)((ram_btn_prev() ^ held) & held);
    set_btn_prev(held);
    set_btn_new(fresh);
    press = fresh;
    set_repeat_up(repeat_step(ram_repeat_up(), BTN_UP, held, fresh, &press));
    set_repeat_down(repeat_step(ram_repeat_down(), BTN_DOWN, held, fresh, &press));
    set_repeat_left(repeat_step(ram_repeat_left(), BTN_LEFT, held, fresh, &press));
    set_repeat_right(repeat_step(ram_repeat_right(), BTN_RIGHT, held, fresh, &press));
    set_btn_press(press);
    set_btn_latch(0);
    psk_w32(INT_MASK_SET, 0x1Fu);
    return done;
}

/*----------------------
 | game_buttons_irq
 | Description: Button interrupt: latches the pressed lines and masks them until the next read.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: btn_latch
 | Params: lines -- pending interrupt lines
 | Returns: N/A
 ----------------------*/
void game_buttons_irq(uint32_t lines) {
    uint8_t pressed = (uint8_t)(lines & 0x1Fu);
    set_btn_latch((uint8_t)(ram_btn_latch() | pressed));
    psk_w32(INT_MASK_CLR, pressed);
}

/*----------------------
 | BUTTONS_IRQ
 | Description: Code address of the button interrupt handler (game_buttons_irq's ARM entry) that lines 0-4 call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BUTTONS_IRQ 0x02002F8Cu

/*----------------------
 | game_buttons_init
 | Description: Clears the button state, sets the long-hold length, points interrupt lines 0-4 at the button handler and enables them.
 | Author: suinevere
 | Dependencies: rrl_ram.h, psk_hw.h
 | Globals: hold_reload, hold_count, btn_latch, btn_prev, btn_new, btn_press, irq_handler
 | Params: hold -- frames of fire hold that count as a long hold
 | Returns: N/A
 ----------------------*/
void game_buttons_init(uint8_t hold) {
    uint8_t line;
    set_hold_reload(hold);
    set_hold_count(hold);
    set_btn_latch(0);
    set_btn_prev(0);
    set_btn_new(0);
    set_btn_press(0);
    for (line = 0u; line < 5u; line++) {
        game_set_irq_handler(line, BUTTONS_IRQ);
    }
    psk_w32(INT_MASK_SET, 0x1Fu);
}
