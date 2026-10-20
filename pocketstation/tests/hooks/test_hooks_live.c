/*----------------------
 | test_hooks_live.c
 | Description: Hook test leaf for 020028E8 that matches the ARM's memory, r0 and flags but leaves r1 wrong.
 | Author: suinevere
 | Dependencies: psk_cpu.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "psk_cpu.h"

void rrl_func_020028E8(PskCpu *c) {
    c->r[1] = 0xDEADu;
    c->r[0] = psk_nz(c, 0);
    psk_w8(c->r[6] + 0x12, (uint8_t)c->r[0]);
    c->r[15] = c->r[14];
}
