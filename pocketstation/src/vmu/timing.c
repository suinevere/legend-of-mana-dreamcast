/*----------------------
 | timing.c
 | Description: Ring Ring Land's tick counters: three (count, seen) byte pairs at 0x5D4 that a started timer's callback advances and the frame wait consumes.
 | Author: suinevere
 | Dependencies: game.h
 | Globals: ticks, irq_handler
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | game_wait_ticks
 | Description: Marks the frame, then idles until tick counter which has advanced by at least n since it was last seen.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: ticks
 | Params: which -- counter 0-2; n -- ticks to wait for
 | Returns: the counter's value when the wait ended
 ----------------------*/
uint8_t game_wait_ticks(uint8_t which, uint32_t n) {
    uint8_t seen, now;
    vmu_present();
    seen = ram_ticks((uint8_t)(which * 2u + 1u));
    for (;;) {
        now = ram_ticks((uint8_t)(which * 2u));
        if ((uint8_t)(now - seen) >= n) {
            break;
        }
        vmu_idle();
    }
    set_ticks((uint8_t)(which * 2u + 1u), now);
    return now;
}

/*----------------------
 | game_timer_tick
 | Description: Timer callback: advances counter 1 for line 0x100, counter 2 for line 0x2000, else counter 0.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: ticks
 | Params: line -- interrupt line that fired
 | Returns: N/A
 ----------------------*/
void game_timer_tick(uint32_t line) {
    uint8_t i = line == 0x100u ? 2u : line == 0x2000u ? 4u : 0u;
    set_ticks(i, (uint8_t)(ram_ticks(i) + 1u));
}

/*----------------------
 | TIMER_TICK
 | Description: The ARM address of game_timer_tick, which a started timer's interrupt slot holds.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TIMER_TICK HANDLER_TIMER_TICK

/*----------------------
 | game_ticks_start
 | Description: Resets timer n with reload and divider, zeroes its tick counter pair and starts it with game_timer_tick on its interrupt line (7, 8 or 13). Reload and divider are narrowed as in game_timer_reset.
 | Author: suinevere
 | Dependencies: rrl_ram.h
 | Globals: ticks, irq_handler
 | Params: n -- timer 0-2; reload -- reload value; divider -- low mode bits
 | Returns: the timer mode written to start it
 ----------------------*/
void game_ticks_start(uint8_t n, uint16_t reload, uint8_t divider) {
    game_timer_reset(n, reload, divider);
    set_ticks((uint8_t)(n * 2u), 0);
    set_ticks((uint8_t)(n * 2u + 1u), 0);
    game_set_irq_handler(n == 1u ? 8u : n == 2u ? 13u : 7u, TIMER_TICK);
    game_timer_irq(n, 1);
    game_timer_run(n, 1);
}
