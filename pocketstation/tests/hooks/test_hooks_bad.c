/*----------------------
 | test_hooks_bad.c
 | Description: 020028E8 deliberately wrong (stores 1, not 0) to prove the trace comparison catches a bad replacement.
 | Author: suinevere
 | Dependencies: psk_hw.h, psk_cpu.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "psk_cpu.h"

void rrl_func_020028E8(PskCpu *c) {
    c->r[0] = psk_nz(c, 0);
    psk_w8(c->r[6] + 0x12, (uint8_t)(c->r[0] + 1));
    c->r[15] = c->r[14];
}
