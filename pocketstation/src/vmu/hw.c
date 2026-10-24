/*----------------------
 | hw.c
 | Description: Ring Ring Land's timer interrupt enable and its arithmetic helpers (a 16-bit divide and a capped digit add).
 | Author: suinevere
 | Dependencies: game.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"

/*----------------------
 | timer_line
 | Description: The interrupt line of the original's timer n: the tick's for Timer 1, else the sound end's.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: n -- timer 1 or 2
 | Returns: the line bit
 ----------------------*/
static uint32_t timer_line(uint32_t n) {
    return n == 1 ? VMU_LINE_TICK : VMU_LINE_SOUND;
}

/*----------------------
 | game_timer_irq
 | Description: Enables (after acknowledging) or disables (then acknowledges) the interrupt line of the original's timer n.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: n -- timer; on -- nonzero to enable
 | Returns: the line bit
 ----------------------*/
void game_timer_irq(uint32_t n, uint8_t on) {
    uint16_t line = (uint16_t)timer_line(n);
    if (on) {
        vmu_irq_ack(line);
        vmu_irq_enable(line);
    } else {
        vmu_irq_disable(line);
        vmu_irq_ack(line);
    }
}

/*----------------------
 | game_divide
 | Description: The game's 16-step shift-subtract divide of n by d; with n below 0x10000 it gives n / d and n % d.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: n -- dividend; d -- divisor; rem -- receives the remainder
 | Returns: the quotient
 ----------------------*/
uint32_t game_divide(uint32_t n, uint32_t d, uint32_t *rem) {
    uint32_t q = 0, dd = d << 16;
    for (uint8_t i = 0; i < 16u; i++) {
        uint32_t carry = n >> 31;
        q <<= 1;
        n <<= 1;
        if (carry || (n >> 16) >= d) {
            n -= dd;
            q |= 1u;
        }
    }
    *rem = n >> 16;
    return q;
}

/*----------------------
 | game_add_capped
 | Description: Writes count bytes of src plus add to dst, each capped at 9.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: count -- bytes; dst, src -- addresses; add -- amount
 | Returns: the last byte written
 ----------------------*/
uint32_t game_add_capped(uint32_t count, uint16_t dst, uint32_t add, uint32_t src) {
    uint32_t v = 0;
    do {
        v = ref8(src++) + add;
        if (v >= 10u) {
            v = 9u;
        }
        vmu_w8(dst++, (uint8_t)v);
    } while (--count);
    return v;
}
