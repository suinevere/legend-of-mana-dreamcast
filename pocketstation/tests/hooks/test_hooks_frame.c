/*----------------------
 | test_hooks_frame.c
 | Description: 02003E48, the frame wait, in C: marks the frame, then idles until tick counter r0 has advanced r1 times.
 | Author: suinevere
 | Dependencies: psk_hw.h, psk_cpu.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "psk_cpu.h"

void rrl_func_02003E48(PskCpu *c) {
    psk_frame();
    c->r[4] = 0x000005D4u + (c->r[0] << 1);
    c->r[6] = psk_r8(c->r[4] + 1);
    for (;;) {
        c->r[5] = psk_r8(c->r[4]);
        c->r[7] = (c->r[5] - c->r[6]) & 0xFFu;
        psk_sub_flags(c, c->r[7], c->r[1]);
        if (c->r[7] >= c->r[1]) {
            break;
        }
        psk_idle();
    }
    psk_w8(c->r[4] + 1, (uint8_t)c->r[5]);
    c->r[15] = c->r[14];
}
