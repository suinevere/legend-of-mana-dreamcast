/*----------------------
 | hw.c
 | Description: Ring Ring Land's timer helpers (reload, mode, start, stop, interrupt enable, reset) and its arithmetic helpers (a 16-bit divide and a capped digit add).
 | Author: suinevere
 | Dependencies: game.h, psk_cpu.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "game.h"
#include "psk_cpu.h"

/*----------------------
 | timer_line
 | Description: The interrupt line of timer n: 0x80, 0x100, else 0x2000.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: n -- timer
 | Returns: the line bit
 ----------------------*/
static uint32_t timer_line(uint32_t n) {
    return n == 0 ? 0x80u : n == 1 ? 0x100u : 0x2000u;
}

/*----------------------
 | game_timer_setup
 | Description: Sets timer n's reload and its mode to (flags << 2) | divider.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: n -- timer; reload -- reload value; divider -- low mode bits; flags -- mode bits from bit 2
 | Returns: the mode written
 ----------------------*/
uint32_t game_timer_setup(uint32_t n, uint32_t reload, uint32_t divider, uint32_t flags) {
    uint32_t mode = (flags << 2) | divider;
    psk_w32(TIMER_BASE + n * 16u, reload);
    psk_w32(TIMER_BASE + n * 16u + 8u, mode);
    return mode;
}

/*----------------------
 | game_timer_run
 | Description: Starts or stops timer n through mode bit 2.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: n -- timer; on -- nonzero to start
 | Returns: the mode written
 ----------------------*/
uint32_t game_timer_run(uint32_t n, uint8_t on) {
    uint32_t mode = psk_ldr(TIMER_BASE + n * 16u + 8u);
    mode = on ? mode | 4u : mode & ~4u;
    psk_w32(TIMER_BASE + n * 16u + 8u, mode);
    return mode;
}

/*----------------------
 | game_timer_irq
 | Description: Enables (after acknowledging) or disables (then acknowledges) timer n's interrupt line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: n -- timer; on -- nonzero to enable
 | Returns: the line bit
 ----------------------*/
uint32_t game_timer_irq(uint32_t n, uint8_t on) {
    uint32_t line = timer_line(n);
    if (on) {
        psk_w32(0x0A000010u, line);
        psk_w32(INT_MASK_SET, line);
    } else {
        psk_w32(INT_MASK_CLR, line);
        psk_w32(0x0A000010u, line);
    }
    return line;
}

/*----------------------
 | game_timer_reset
 | Description: Stops timer n and its interrupt, then sets its reload and divider with the other mode bits clear. Reload and divider are narrowed from the ARM's words; every caller passes constants that fit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: n -- timer; reload -- reload value; divider -- low mode bits
 | Returns: N/A
 ----------------------*/
void game_timer_reset(uint8_t n, uint16_t reload, uint8_t divider) {
    game_timer_run(n, 0);
    game_timer_irq(n, 0);
    game_timer_setup(n, reload, divider, 0);
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
uint32_t game_add_capped(uint32_t count, uint32_t dst, uint32_t add, uint32_t src) {
    uint32_t v = 0;
    do {
        v = psk_r8(src++) + add;
        if (v >= 10u) {
            v = 9u;
        }
        psk_w8(dst++, (uint8_t)v);
    } while (--count);
    return v;
}
