/*----------------------
 | test_hooks_callout.c
 | Description: Two veneers into still-ARM Thumb code: 02008294 through psk_call_arm (argument style) and 020082C4 through psk_call (register level).
 | Author: suinevere
 | Dependencies: psk_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "psk_hw.h"

uint32_t rrl_func_02008294(uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3) {
    return psk_call_arm(0x02004311u, r0, r1, r2, r3);
}

void rrl_func_020082C4(PskCpu *c) {
    c->r[12] = 0x020042FFu;
    psk_call(c, c->r[12], c->r[14]);
}
