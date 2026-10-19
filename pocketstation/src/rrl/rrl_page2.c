/*----------------------
 | rrl_page2.c
 | Description: Ring Ring Land routines at 02002000-02002FFF as register-level C, until they are named and grouped by concern.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "rrl.h"

/*----------------------
 | rrl_func_02002BC4
 | Description: Register-level C for the ARM routine at 02002BC4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x208
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002BC4(PskCpu *c) {
    c->r[4] = 0x00000208u;
    psk_w32(c->r[4] + ((c->r[0] << 2u)), c->r[1]);
    c->r[15] = c->r[14] & ~3u;
    return;
}

/*----------------------
 | rrl_func_02002FE8
 | Description: Register-level C for the ARM routine at 02002FE8.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x240, WRAM 0x2C0, WRAM 0x340
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002FE8(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[11]);
        psk_w32(a - 8u, c->r[10]);
        psk_w32(a - 0xCu, c->r[9]);
        psk_w32(a - 0x10u, c->r[8]);
        psk_w32(a - 0x14u, c->r[7]);
        psk_w32(a - 0x18u, c->r[6]);
        psk_w32(a - 0x1Cu, c->r[4]);
        c->r[13] = a - 0x1Cu;
    }
    psk_sub_flags(c, c->r[2], 0x00000020u);
    if (psk_cond(c, PSK_EQ)) {
        psk_nz(c, c->r[5] & 0x00000004u);
    }
    if (psk_cond(c, PSK_EQ)) {
        goto a_020031C8;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[5]);
        psk_w32(a - 8u, c->r[1]);
        psk_w32(a - 0xCu, c->r[0]);
        c->r[13] = a - 0xCu;
    }
    c->r[6] = 0x000002C0u;
    c->r[9] = ~0x00000000u;
    c->r[9] = ~(psk_lsl(c->r[9], (c->r[2] & 0xFFu)));
a_02003008:
    c->r[0] = 0x00000000u;
    c->r[7] = 0x00000000u;
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[3]);
        c->r[13] = a - 4u;
    }
a_02003014:
    psk_sub_flags(c, c->r[7], c->r[2]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02003038;
    }
    {
        uint32_t a = c->r[4];
        uint32_t b = a + 4u;
        c->r[1] = psk_ldr(a);
        c->r[4] = b;
    }
    c->r[0] = c->r[0] | (psk_lsl(c->r[1], (c->r[7] & 0xFFu)));
    c->r[8] = c->r[0];
    c->r[10] = c->r[2] - c->r[7];
    c->r[0] = psk_lsr(c->r[1], (c->r[10] & 0xFFu));
    c->r[7] = c->r[7] + 0x00000020u;
    goto a_02003040;
a_02003038:
    c->r[8] = c->r[0];
    c->r[0] = psk_lsr(c->r[0], (c->r[2] & 0xFFu));
a_02003040:
    c->r[8] = c->r[8] & c->r[9];
    psk_nz(c, c->r[5] & 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003074;
    }
    c->r[10] = 0x00000001u;
    c->r[1] = c->r[2] - 0x00000001u;
    c->r[10] = psk_lsl(c->r[10], (c->r[1] & 0xFFu));
    c->r[11] = 0x00000000u;
a_0200305C:
    c->r[8] = psk_nz(c, psk_lsr_s(c, c->r[8], 1u));
    if (psk_cond(c, PSK_HS)) {
        c->r[11] = c->r[11] | c->r[10];
    }
    c->r[10] = (c->r[10] >> 1u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_PL)) {
        goto a_0200305C;
    }
    c->r[8] = c->r[11];
a_02003074:
    {
        uint32_t a = c->r[6];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[8]);
        c->r[6] = b;
    }
    c->r[7] = c->r[7] - c->r[2];
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003014;
    }
    {
        uint32_t a = c->r[13];
        c->r[3] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    psk_nz(c, c->r[5] & 0x00000001u);
    c->r[5] = c->r[5] & ~0x00000001u;
    c->r[6] = 0x00000340u;
    if (psk_cond(c, PSK_NE)) {
        goto a_02003008;
    }
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[5] = psk_r32(a + 8u);
        c->r[13] = a + 0xCu;
    }
    c->r[4] = 0x000002C0u;
    c->r[6] = 0x00000240u;
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_MI)) {
        goto a_020030C8;
    }
    c->r[6] = c->r[6] + (c->r[1] << 2u);
    c->r[7] = psk_sub_flags(c, 0x00000020u, c->r[1]);
    if (psk_cond(c, PSK_LE)) {
        goto a_020032A8;
    }
    psk_sub_flags(c, c->r[7], c->r[3]);
    if (psk_cond(c, PSK_HS)) {
        c->r[8] = c->r[3];
    }
    if (psk_cond(c, PSK_LO)) {
        c->r[8] = c->r[7];
    }
    goto a_020030E4;
a_020030C8:
    c->r[4] = c->r[4] - ((c->r[1] << 2u));
    c->r[7] = psk_add_flags(c, c->r[3], c->r[1]);
    if (psk_cond(c, PSK_LO)) {
        goto a_020032A8;
    }
    if (psk_cond(c, PSK_EQ)) {
        goto a_020032A8;
    }
    psk_sub_flags(c, c->r[7], 0x00000020u);
    if (psk_cond(c, PSK_LT)) {
        c->r[8] = c->r[7];
    }
    if (psk_cond(c, PSK_GE)) {
        c->r[8] = 0x00000020u;
    }
a_020030E4:
    c->r[11] = psk_add_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_MI)) {
        c->r[11] = 0x00000000u - c->r[11];
    }
    psk_nz(c, c->r[5] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003160;
    }
    psk_nz(c, c->r[5] & 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003138;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_PL)) {
        c->r[9] = psk_lsl(c->r[9], (c->r[11] & 0xFFu));
    }
    if (psk_cond(c, PSK_MI)) {
        c->r[9] = psk_lsr(c->r[9], (c->r[11] & 0xFFu));
    }
    c->r[9] = ~c->r[9];
a_0200310C:
    {
        uint32_t a = c->r[4];
        uint32_t b = a + 4u;
        c->r[7] = psk_ldr(a);
        c->r[4] = b;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_PL)) {
        c->r[7] = psk_lsl(c->r[7], (c->r[11] & 0xFFu));
    }
    if (psk_cond(c, PSK_MI)) {
        c->r[7] = psk_lsr(c->r[7], (c->r[11] & 0xFFu));
    }
    c->r[10] = psk_ldr(c->r[6]);
    c->r[10] = c->r[10] & c->r[9];
    c->r[10] = c->r[10] | c->r[7];
    {
        uint32_t a = c->r[6];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[10]);
        c->r[6] = b;
    }
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200310C;
    }
    goto a_020032A8;
a_02003138:
    {
        uint32_t a = c->r[4];
        uint32_t b = a + 4u;
        c->r[7] = psk_ldr(a);
        c->r[4] = b;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_PL)) {
        c->r[7] = psk_lsl(c->r[7], (c->r[11] & 0xFFu));
    }
    if (psk_cond(c, PSK_MI)) {
        c->r[7] = psk_lsr(c->r[7], (c->r[11] & 0xFFu));
    }
    c->r[10] = psk_ldr(c->r[6]);
    c->r[10] = c->r[10] | c->r[7];
    {
        uint32_t a = c->r[6];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[10]);
        c->r[6] = b;
    }
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003138;
    }
    goto a_020032A8;
a_02003160:
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_MI)) {
        goto a_02003198;
    }
a_02003168:
    c->r[9] = psk_ldr(c->r[4] + 0x80u);
    {
        uint32_t a = c->r[4];
        uint32_t b = a + 4u;
        c->r[7] = psk_ldr(a);
        c->r[4] = b;
    }
    c->r[7] = psk_lsl(c->r[7], (c->r[11] & 0xFFu));
    c->r[9] = psk_lsl(c->r[9], (c->r[11] & 0xFFu));
    c->r[9] = ~c->r[9];
    c->r[10] = psk_ldr(c->r[6]);
    c->r[10] = c->r[10] & c->r[9];
    c->r[10] = c->r[10] | c->r[7];
    {
        uint32_t a = c->r[6];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[10]);
        c->r[6] = b;
    }
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003168;
    }
    goto a_020032A8;
a_02003198:
    c->r[9] = psk_ldr(c->r[4] + 0x80u);
    {
        uint32_t a = c->r[4];
        uint32_t b = a + 4u;
        c->r[7] = psk_ldr(a);
        c->r[4] = b;
    }
    c->r[7] = psk_lsr(c->r[7], (c->r[11] & 0xFFu));
    c->r[9] = psk_lsr(c->r[9], (c->r[11] & 0xFFu));
    c->r[9] = ~c->r[9];
    c->r[10] = psk_ldr(c->r[6]);
    c->r[10] = c->r[10] & c->r[9];
    c->r[10] = c->r[10] | c->r[7];
    {
        uint32_t a = c->r[6];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[10]);
        c->r[6] = b;
    }
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003198;
    }
    goto a_020032A8;
a_020031C8:
    c->r[6] = 0x00000240u;
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_MI)) {
        goto a_020031F0;
    }
    c->r[6] = c->r[6] + (c->r[1] << 2u);
    c->r[7] = psk_sub_flags(c, 0x00000020u, c->r[1]);
    if (psk_cond(c, PSK_LE)) {
        goto a_020032A8;
    }
    psk_sub_flags(c, c->r[7], c->r[3]);
    if (psk_cond(c, PSK_HS)) {
        c->r[8] = c->r[3];
    }
    if (psk_cond(c, PSK_LO)) {
        c->r[8] = c->r[7];
    }
    goto a_0200320C;
a_020031F0:
    c->r[4] = c->r[4] - ((c->r[1] << 2u));
    c->r[7] = psk_add_flags(c, c->r[3], c->r[1]);
    if (psk_cond(c, PSK_LO)) {
        goto a_020032A8;
    }
    if (psk_cond(c, PSK_EQ)) {
        goto a_020032A8;
    }
    psk_sub_flags(c, c->r[7], 0x00000020u);
    if (psk_cond(c, PSK_LT)) {
        c->r[8] = c->r[7];
    }
    if (psk_cond(c, PSK_GE)) {
        c->r[8] = 0x00000020u;
    }
a_0200320C:
    c->r[11] = psk_add_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_MI)) {
        c->r[11] = 0x00000000u - c->r[11];
    }
    psk_nz(c, c->r[5] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003244;
    }
a_0200321C:
    {
        uint32_t a = c->r[4];
        uint32_t b = a + 4u;
        c->r[7] = psk_ldr(a);
        c->r[4] = b;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_PL)) {
        c->r[7] = psk_lsl(c->r[7], (c->r[11] & 0xFFu));
    }
    if (psk_cond(c, PSK_MI)) {
        c->r[7] = psk_lsr(c->r[7], (c->r[11] & 0xFFu));
    }
    c->r[10] = psk_ldr(c->r[6]);
    c->r[10] = c->r[10] | c->r[7];
    {
        uint32_t a = c->r[6];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[10]);
        c->r[6] = b;
    }
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200321C;
    }
    goto a_020032A8;
a_02003244:
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_MI)) {
        goto a_0200327C;
    }
a_0200324C:
    c->r[9] = psk_ldr(c->r[4] + ((c->r[3] << 2u)));
    {
        uint32_t a = c->r[4];
        uint32_t b = a + 4u;
        c->r[7] = psk_ldr(a);
        c->r[4] = b;
    }
    c->r[7] = psk_lsl(c->r[7], (c->r[11] & 0xFFu));
    c->r[9] = psk_lsl(c->r[9], (c->r[11] & 0xFFu));
    c->r[9] = ~c->r[9];
    c->r[10] = psk_ldr(c->r[6]);
    c->r[10] = c->r[10] & c->r[9];
    c->r[10] = c->r[10] | c->r[7];
    {
        uint32_t a = c->r[6];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[10]);
        c->r[6] = b;
    }
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200324C;
    }
    goto a_020032A8;
a_0200327C:
    c->r[9] = psk_ldr(c->r[4] + ((c->r[3] << 2u)));
    {
        uint32_t a = c->r[4];
        uint32_t b = a + 4u;
        c->r[7] = psk_ldr(a);
        c->r[4] = b;
    }
    c->r[7] = psk_lsr(c->r[7], (c->r[11] & 0xFFu));
    c->r[9] = psk_lsr(c->r[9], (c->r[11] & 0xFFu));
    c->r[9] = ~c->r[9];
    c->r[10] = psk_ldr(c->r[6]);
    c->r[10] = c->r[10] & c->r[9];
    c->r[10] = c->r[10] | c->r[7];
    {
        uint32_t a = c->r[6];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[10]);
        c->r[6] = b;
    }
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200327C;
    }
a_020032A8:
    {
        uint32_t a = c->r[13];
        c->r[4] = psk_r32(a);
        c->r[6] = psk_r32(a + 4u);
        c->r[7] = psk_r32(a + 8u);
        c->r[8] = psk_r32(a + 0xCu);
        c->r[9] = psk_r32(a + 0x10u);
        c->r[10] = psk_r32(a + 0x14u);
        c->r[11] = psk_r32(a + 0x18u);
        c->r[13] = a + 0x1Cu;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_0200265C
 | Description: Register-level C for the Thumb routine at 0200265C; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200265C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_r8(c->r[7] + 0x12u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002668;
    }
    c->r[1] = psk_nz(c, 0x00000028u);
    goto a_0200266A;
a_02002668:
    c->r[1] = psk_nz(c, 0x0000001Eu);
a_0200266A:
    c->r[0] = psk_r8(c->r[7] + 0x13u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200268C;
    }
    c->r[0] = psk_r8(c->r[7] + 0x11u);
    c->r[1] = psk_r8(c->r[7] + 0x12u);
    psk_w8(c->r[7] + 0x11u, (uint8_t)c->r[1]);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002686;
    }
    psk_sub_flags(c, c->r[1], c->r[0]);
    if (psk_cond(c, PSK_LO)) {
        goto a_02002690;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_call(c, 0x02003CA3u, 0x02002685u);
    goto a_02002690;
a_02002686:
    c->r[0] = psk_nz(c, 0x0000000Cu);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_02002690;
a_0200268C:
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 0x13u, (uint8_t)c->r[0]);
a_02002690:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002694
 | Description: Register-level C for the Thumb routine at 02002694; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002694(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7] + 0xBu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200270E;
    }
    c->r[0] = psk_r8(c->r[7] + 0xCu);
    c->r[1] = psk_r8(c->r[7] + 0xDu);
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_LO)) {
        goto a_020026BE;
    }
    if (psk_cond(c, PSK_NE)) {
        goto a_020026B4;
    }
    psk_sub_flags(c, c->r[0], 0x0000000Cu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200270E;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 0xCu, (uint8_t)c->r[0]);
    goto a_0200270E;
a_020026B4:
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 0xCu, (uint8_t)c->r[0]);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200270E;
    }
    psk_w8(c->r[7] + 0xBu, (uint8_t)c->r[0]);
    goto a_0200270E;
a_020026BE:
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 0xCu, (uint8_t)c->r[0]);
    psk_sub_flags(c, c->r[0], 0x00000010u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200270E;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0xBu, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 0xCu, (uint8_t)c->r[0]);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020026F0;
    }
    c->r[0] = psk_r8(c->r[7] + 6u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    c->r[1] = psk_nz(c, 0x0000001Fu);
    c->r[0] = psk_nz(c, c->r[0] & c->r[1]);
    psk_w8(c->r[7] + 6u, (uint8_t)c->r[0]);
    psk_call(c, 0x02004235u, 0x020026DFu);
    psk_sub_flags(c, c->r[0], 0x0000000Fu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200270E;
    }
    c->r[3] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[0] = psk_r8(c->r[7] + 7u);
    psk_call(c, 0x0200424Du, 0x020026EBu);
    c->r[3] = psk_nz(c, c->r[3] & ~c->r[0]);
    psk_w8(c->r[7] + 7u, (uint8_t)c->r[3]);
    goto a_0200270E;
a_020026F0:
    c->r[0] = psk_r8(c->r[7] + 6u);
    c->r[3] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    c->r[1] = psk_nz(c, 0x0000001Fu);
    c->r[3] = psk_nz(c, c->r[3] & c->r[1]);
    psk_w8(c->r[7] + 6u, (uint8_t)c->r[3]);
    psk_call(c, 0x02004235u, 0x02002701u);
    psk_sub_flags(c, c->r[0], 0x0000000Fu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200270E;
    }
    c->r[1] = psk_r8(c->r[7] + 7u);
    c->r[0] = psk_nz(c, c->r[0] & ~c->r[1]);
    psk_call(c, 0x0200424Du, 0x0200270Du);
    psk_w8(c->r[7] + 7u, (uint8_t)c->r[0]);
a_0200270E:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002712
 | Description: Register-level C for the Thumb routine at 02002712; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002712(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_r8(c->r[7] + 0xBu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200279A;
    }
    c->r[0] = psk_r8(c->r[7] + 6u);
    psk_call(c, 0x02004235u, 0x02002721u);
    c->r[4] = psk_r8(c->r[7] + 7u);
    psk_sub_flags(c, c->r[0], 0x0000000Fu);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200272E;
    }
    c->r[0] = psk_add_flags(c, c->r[4], 0x00000000u);
    psk_call(c, 0x0200424Du, 0x0200272Du);
    c->r[0] = psk_nz(c, c->r[0] | c->r[4]);
a_0200272E:
    c->r[3] = psk_r8(c->r[7] + 5u);
    psk_sub_flags(c, c->r[3], 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002736;
    }
    c->r[3] = psk_nz(c, 0x00000000u);
a_02002736:
    c->r[1] = psk_r8(c->r[6] + 2u);
    c->r[2] = psk_nz(c, 0x00000004u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200274A;
    }
    c->r[2] = psk_nz(c, 0x00000005u);
    c->r[2] = psk_nz(c, c->r[2] & c->r[0]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200274A;
    }
    psk_sub_flags(c, c->r[2], 0x00000005u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002784;
    }
    goto a_0200277E;
a_0200274A:
    c->r[2] = psk_nz(c, 0x00000002u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200275C;
    }
    c->r[2] = psk_nz(c, 0x0000000Au);
    c->r[2] = psk_nz(c, c->r[2] & c->r[0]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200275C;
    }
    psk_sub_flags(c, c->r[2], 0x0000000Au);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002784;
    }
    goto a_0200277E;
a_0200275C:
    c->r[2] = psk_nz(c, 0x00000010u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200276E;
    }
    c->r[2] = psk_nz(c, 0x00000003u);
    c->r[2] = psk_nz(c, c->r[2] & c->r[0]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200276E;
    }
    psk_sub_flags(c, c->r[2], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002784;
    }
    goto a_0200277E;
a_0200276E:
    c->r[2] = psk_nz(c, 0x00000008u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200279A;
    }
    c->r[2] = psk_nz(c, 0x0000000Cu);
    c->r[2] = psk_nz(c, c->r[2] & c->r[0]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200279A;
    }
    psk_sub_flags(c, c->r[2], 0x0000000Cu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002784;
    }
a_0200277E:
    psk_nz(c, c->r[2] & c->r[4]);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200278A;
    }
    goto a_0200278E;
a_02002784:
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200278E;
    }
    goto a_0200278A;
a_0200278A:
    c->r[0] = psk_nz(c, 0x00000000u);
    goto a_02002790;
a_0200278E:
    c->r[0] = psk_nz(c, 0x00000001u);
a_02002790:
    psk_w8(c->r[7] + 0xDu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[7] + 0xBu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0xCu, (uint8_t)c->r[0]);
a_0200279A:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002DE0
 | Description: Register-level C for the Thumb routine at 02002DE0.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002DE0(PskCpu *c) {
    c->r[1] = 0x02008094u;
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 1u));
    c->r[0] = psk_ldrh(c->r[1] + c->r[0]);
    c->r[1] = psk_nz(c, 0x00000080u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 8u));
    c->r[0] = psk_nz(c, c->r[0] & ~c->r[1]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02002CFE
 | Description: Register-level C for the Thumb routine at 02002CFE.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002CFE(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[5]);
        psk_w32(a - 8u, c->r[1]);
        c->r[13] = a - 8u;
    }
    c->r[5] = 0x000004C4u;
    c->r[1] = psk_ldr(c->r[5] + 4u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    psk_w32(c->r[5] + 4u, c->r[1]);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[1]);
    psk_w8(c->r[5] + 7u, (uint8_t)c->r[0]);
    psk_sub_flags(c, c->r[1], 0x00000033u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002D16;
    }
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_02002D1E;
a_02002D16:
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x1Eu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Eu);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
a_02002D1E:
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[5] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02002CD6
 | Description: Register-level C for the Thumb routine at 02002CD6; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002CD6(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x02002CDFu);
    c->r[2] = 0x02009EECu;
    c->r[1] = psk_r8(c->r[5]);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 2u));
    c->r[2] = psk_ldr(c->r[2] + c->r[1]);
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[3] = psk_nz(c, 0x000000FFu);
a_02002CEA:
    c->r[4] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[4] = psk_nz(c, c->r[4] & c->r[3]);
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[4]);
    if (psk_cond(c, PSK_LO)) {
        goto a_02002CF8;
    }
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 8u));
    goto a_02002CEA;
a_02002CF8:
    c->r[0] = psk_add_flags(c, c->r[1], 0x00000000u);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[1]);
    return;
}

/*----------------------
 | rrl_func_02002CA8
 | Description: Register-level C for the Thumb routine at 02002CA8; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002CA8(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x02002CB1u);
    c->r[2] = 0x02009EDCu;
    c->r[1] = psk_r8(c->r[5] + 4u);
    c->r[3] = psk_r8(c->r[5] + 5u);
    c->r[3] = psk_nz(c, psk_lsl_s(c, c->r[3], 1u));
    c->r[1] = psk_nz(c, c->r[1] | c->r[3]);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 2u));
    c->r[2] = psk_ldr(c->r[2] + c->r[1]);
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[3] = psk_nz(c, 0x000000FFu);
a_02002CC2:
    c->r[4] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[4] = psk_nz(c, c->r[4] & c->r[3]);
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[4]);
    if (psk_cond(c, PSK_LO)) {
        goto a_02002CD0;
    }
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 8u));
    goto a_02002CC2;
a_02002CD0:
    c->r[0] = psk_add_flags(c, c->r[1], 0x00000000u);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[1]);
    return;
}

/*----------------------
 | rrl_func_02002BDC
 | Description: Register-level C for the Thumb routine at 02002BDC; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002BDC(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[5] = psk_nz(c, 0x00000020u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C00;
    }
    psk_call(c, 0x02002CD7u, 0x02002BEBu);
    c->r[3] = psk_r8(c->r[6] + 0x14u);
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000005u);
    c->r[3] = psk_nz(c, psk_lsl_s(c, c->r[3], 2u));
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000070u);
    c->r[3] = psk_r8(c->r[6] + c->r[3]);
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002BFC;
    }
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000040u);
    goto a_02002CA2;
a_02002BFC:
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000048u);
    goto a_02002CA2;
a_02002C00:
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002C42;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C3C;
    }
    c->r[1] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[1], 0x0000000Du);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C3C;
    }
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x02002C15u);
    psk_sub_flags(c, c->r[0], 0x00000050u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02002C38;
    }
    c->r[0] = psk_r8(c->r[5] + 4u);
    c->r[1] = psk_r8(c->r[5] + 5u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C2C;
    }
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C28;
    }
    c->r[3] = psk_nz(c, 0x00000034u);
    goto a_02002CA2;
a_02002C28:
    c->r[3] = psk_nz(c, 0x00000015u);
    goto a_02002CA2;
a_02002C2C:
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C34;
    }
    c->r[3] = psk_nz(c, 0x00000008u);
    goto a_02002CA2;
a_02002C34:
    c->r[3] = psk_nz(c, 0x0000001Bu);
    goto a_02002CA2;
a_02002C38:
    c->r[0] = psk_nz(c, 0x00000003u);
    goto a_02002C52;
a_02002C3C:
    psk_call(c, 0x02002CD7u, 0x02002C41u);
    goto a_02002C52;
a_02002C42:
    c->r[1] = psk_r8(c->r[5]);
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        c->r[13] = a - 4u;
    }
    c->r[1] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5], (uint8_t)c->r[1]);
    psk_call(c, 0x02002CD7u, 0x02002C4Fu);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    psk_w8(c->r[5], (uint8_t)c->r[1]);
a_02002C52:
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[0]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02002CA9u, 0x02002C59u);
    c->r[2] = psk_add_flags(c, c->r[0], 0x00000000u);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[3] = psk_nz(c, 0x000000FFu);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[4] = 0x02008094u;
a_02002C62:
    c->r[5] = psk_ldrh(c->r[4]);
    c->r[7] = psk_nz(c, 0x000000FFu);
    c->r[7] = psk_nz(c, c->r[7] & c->r[5]);
    psk_sub_flags(c, c->r[7], c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C74;
    }
    c->r[5] = psk_nz(c, psk_lsr_s(c, c->r[5], 8u));
    psk_sub_flags(c, c->r[5], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C74;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
a_02002C74:
    c->r[4] = psk_add_flags(c, c->r[4], 0x00000002u);
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C62;
    }
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x02002C81u);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[3] = psk_nz(c, 0x00000000u);
    c->r[4] = 0x02008094u;
a_02002C86:
    c->r[5] = psk_ldrh(c->r[4]);
    c->r[7] = psk_nz(c, 0x000000FFu);
    c->r[7] = psk_nz(c, c->r[7] & c->r[5]);
    psk_sub_flags(c, c->r[7], c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C9C;
    }
    c->r[5] = psk_nz(c, psk_lsr_s(c, c->r[5], 8u));
    psk_sub_flags(c, c->r[5], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002C9C;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002CA2;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
a_02002C9C:
    c->r[4] = psk_add_flags(c, c->r[4], 0x00000002u);
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000001u);
    goto a_02002C86;
a_02002CA2:
    c->r[0] = psk_add_flags(c, c->r[3], 0x00000000u);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[1]);
    return;
}

/*----------------------
 | rrl_func_02002282
 | Description: Unverified (a random board event no script rolls). Register-level C for the Thumb routine at 02002282; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002282(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_sub_flags(c, c->r[0], 0x0000000Bu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020022C0;
    }
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020022B2;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x00000003u);
    c->r[1] = psk_nz(c, c->r[1] & c->r[0]);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020022A4;
    }
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020022A2;
    }
    c->r[1] = psk_nz(c, 0x00000002u);
    goto a_020022A4;
a_020022A2:
    c->r[1] = psk_nz(c, 0x00000003u);
a_020022A4:
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 3u));
    if (psk_cond(c, PSK_LO)) {
        goto a_020022AE;
    }
    c->r[0] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_sub_flags(c, c->r[1], c->r[0]);
a_020022AE:
    psk_w8(c->r[6] + 0x1Cu, (uint8_t)c->r[1]);
    goto a_02002346;
a_020022B2:
    c->r[0] = psk_nz(c, 0x0000000Bu);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x1Cu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000050u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    goto a_02002346;
a_020022C0:
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200230C;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[5] = psk_nz(c, 0x00000070u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 1u));
    c->r[3] = psk_nz(c, 0x00000003u);
    c->r[0] = psk_nz(c, c->r[0] & c->r[3]);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_HS)) {
        goto a_020022DC;
    }
    c->r[3] = psk_nz(c, 0x00000001u);
    c->r[0] = psk_nz(c, c->r[0] ^ c->r[3]);
a_020022DC:
    psk_w8(c->r[5] + 5u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000010u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02002306;
    }
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    c->r[1] = psk_nz(c, 0x00000003u);
    c->r[1] = psk_nz(c, c->r[1] & c->r[0]);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020022F8;
    }
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020022F6;
    }
    c->r[1] = psk_nz(c, 0x00000002u);
    goto a_020022F8;
a_020022F6:
    c->r[1] = psk_nz(c, 0x00000003u);
a_020022F8:
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 3u));
    if (psk_cond(c, PSK_HS)) {
        goto a_02002302;
    }
    c->r[0] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_sub_flags(c, c->r[1], c->r[0]);
a_02002302:
    psk_w8(c->r[6] + 0x1Du, (uint8_t)c->r[1]);
    c->r[0] = psk_nz(c, 0x00000000u);
a_02002306:
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 1u));
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    goto a_02002346;
a_0200230C:
    c->r[5] = psk_nz(c, 0x00000070u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_w8(c->r[5] + 5u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000028u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000014u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    psk_call(c, 0x020039F5u, 0x02002325u);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000062u);
    psk_call(c, 0x02003A07u, 0x0200232Du);
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, 0x00000069u);
    psk_call(c, 0x02003A07u, 0x02002335u);
    c->r[0] = psk_nz(c, 0x00000002u);
    c->r[1] = psk_nz(c, 0x00000063u);
    psk_call(c, 0x02003A07u, 0x0200233Du);
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_call(c, 0x02003D13u, 0x02002343u);
    c->r[0] = psk_nz(c, 0x00000000u);
    goto a_02002348;
a_02002346:
    c->r[0] = psk_nz(c, 0x00000001u);
a_02002348:
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[1]);
    return;
}

/*----------------------
 | rrl_func_0200234C
 | Description: Register-level C for the Thumb routine at 0200234C; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200234C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020039F5u, 0x02002353u);
    c->r[5] = psk_nz(c, 0x00000013u);
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 4u));
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[2] = psk_r8(c->r[5] + 0xDu);
    c->r[0] = psk_r8(c->r[6] + 0x19u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002390;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002372;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002390;
    }
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002396;
    }
    c->r[5] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000067u);
    goto a_0200237C;
a_02002372:
    c->r[5] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 3u));
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    c->r[1] = psk_nz(c, 0x00000066u);
a_0200237C:
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[2]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x02003A07u, 0x02002385u);
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, 0x00000068u);
    psk_call(c, 0x02003A07u, 0x0200238Du);
    {
        uint32_t a = c->r[13];
        c->r[2] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    goto a_020023DC;
a_02002390:
    c->r[5] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000067u);
    goto a_020023A0;
a_02002396:
    c->r[5] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 3u));
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    c->r[1] = psk_nz(c, 0x00000066u);
a_020023A0:
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[2]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[2] = psk_nz(c, 0x000000F9u);
    psk_call(c, 0x02003A25u, 0x020023ABu);
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, 0x00000068u);
    c->r[2] = psk_nz(c, 0x000000F9u);
    psk_call(c, 0x02003A25u, 0x020023B5u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[2] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020023C0;
    }
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    c->r[1] = psk_nz(c, 0x00000067u);
    goto a_020023CA;
a_020023C0:
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 1u));
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 2u));
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    c->r[1] = psk_nz(c, 0x00000066u);
a_020023CA:
    c->r[0] = psk_nz(c, 0x00000002u);
    c->r[2] = psk_nz(c, 0x00000008u);
    psk_call(c, 0x02003A25u, 0x020023D3u);
    c->r[0] = psk_nz(c, 0x00000003u);
    c->r[1] = psk_nz(c, 0x00000068u);
    c->r[2] = psk_nz(c, 0x00000008u);
    psk_call(c, 0x02003A25u, 0x020023DDu);
a_020023DC:
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[2] = psk_r8(c->r[7] + 1u);
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 5u));
    psk_sub_flags(c, c->r[2], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020023F6;
    }
    psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020023F4;
    }
    c->r[2] = psk_add_flags(c, c->r[5], 0x00000000u);
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 1u));
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    goto a_020023F6;
a_020023F4:
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 1u));
a_020023F6:
    c->r[2] = 0x000004C4u;
    c->r[1] = psk_ldr(c->r[2]);
    c->r[0] = 0x0001869Fu;
    c->r[1] = psk_add_flags(c, c->r[1], c->r[5]);
    psk_sub_flags(c, c->r[1], c->r[0]);
    if (psk_cond(c, PSK_LO)) {
        goto a_02002408;
    }
    c->r[5] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[5] = psk_sub_flags(c, c->r[5], c->r[1]);
    c->r[1] = psk_add_flags(c, c->r[0], 0x00000000u);
a_02002408:
    psk_w32(c->r[2], c->r[1]);
    psk_w16((c->r[6] + 0x1Cu) & ~1u, (uint16_t)c->r[5]);
    c->r[1] = psk_ldrh(c->r[7] + 0x1Au);
    c->r[1] = psk_add_flags(c, c->r[1], c->r[5]);
    psk_w16((c->r[7] + 0x1Au) & ~1u, (uint16_t)c->r[1]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_0200225C
 | Description: Register-level C for the Thumb routine at 0200225C; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200225C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_r8(c->r[5] + 5u);
    c->r[1] = psk_ldrh(c->r[5] + 0xCu);
    c->r[2] = psk_nz(c, 0x00000018u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 4u));
    c->r[2] = psk_add_flags(c, c->r[2], c->r[6]);
    psk_call(c, 0x0200454Bu, 0x0200226Du);
    psk_w8(c->r[6] + 0x17u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[5] + 6u);
    c->r[1] = psk_ldrh(c->r[5] + 0xEu);
    c->r[2] = psk_nz(c, 0x00000019u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 4u));
    c->r[2] = psk_add_flags(c, c->r[2], c->r[6]);
    psk_call(c, 0x0200454Bu, 0x0200227Du);
    psk_w8(c->r[6] + 0x18u, (uint8_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002102
 | Description: Register-level C for the Thumb routine at 02002102; it calls 10 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002102(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02002122;
    }
    psk_sub_flags(c, c->r[0], 0x0000000Au);
    if (psk_cond(c, PSK_HS)) {
        goto a_02002122;
    }
    c->r[3] = psk_r8(c->r[6] + 0x12u);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002122;
    }
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[3]);
    goto a_02002258;
a_02002122:
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002150;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002176;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200218A;
    }
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020021D8;
    }
    psk_sub_flags(c, c->r[0], 0x00000005u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020021E6;
    }
    psk_sub_flags(c, c->r[0], 0x00000006u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020021EC;
    }
    psk_sub_flags(c, c->r[0], 0x00000007u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200220C;
    }
    psk_sub_flags(c, c->r[0], 0x00000008u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002212;
    }
    psk_sub_flags(c, c->r[0], 0x00000009u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200222C;
    }
    psk_call(c, 0x02002283u, 0x0200214Bu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020021D6;
    }
    goto a_02002258;
a_02002150:
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200215C;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    goto a_02002166;
a_0200215C:
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002166;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000002u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
a_02002166:
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x02003A43u, 0x0200216Du);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002258;
    }
    psk_call(c, 0x020043D7u, 0x02002175u);
    goto a_0200224A;
a_02002176:
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_r8(c->r[5]);
    psk_sub_flags(c, c->r[0], 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002258;
    }
    psk_call(c, 0x0200225Du, 0x02002185u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000002u);
    goto a_0200224E;
a_0200218A:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002258;
    }
    psk_call(c, 0x020039F5u, 0x02002193u);
    c->r[1] = psk_r8(c->r[6] + 0x17u);
    c->r[2] = psk_r8(c->r[6] + 0x18u);
    psk_sub_flags(c, c->r[1], c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020021A8;
    }
    if (psk_cond(c, PSK_LO)) {
        goto a_020021B4;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x0000005Fu);
    psk_call(c, 0x02003A07u, 0x020021A5u);
    c->r[0] = psk_nz(c, 0x00000004u);
    goto a_0200224E;
a_020021A8:
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000060u);
    psk_call(c, 0x02003A07u, 0x020021B1u);
    c->r[0] = psk_nz(c, 0x00000007u);
    goto a_0200224E;
a_020021B4:
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x02003A07u, 0x020021BDu);
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, 0x00000065u);
    psk_call(c, 0x02003A07u, 0x020021C5u);
    c->r[0] = psk_nz(c, 0x00000002u);
    c->r[1] = psk_nz(c, 0x0000006Au);
    psk_call(c, 0x02003A07u, 0x020021CDu);
    c->r[5] = psk_nz(c, 0x00000070u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000008u);
a_020021D6:
    goto a_0200224E;
a_020021D8:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002258;
    }
    psk_call(c, 0x0200234Du, 0x020021E1u);
    c->r[3] = psk_nz(c, 0x00000040u);
    psk_w8(c->r[6] + 0x13u, (uint8_t)c->r[3]);
    goto a_0200224A;
a_020021E6:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002258;
    }
    goto a_0200224A;
a_020021EC:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002258;
    }
    c->r[1] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[1], 0x0000000Du);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002200;
    }
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x020021FDu);
    psk_sub_flags(c, c->r[0], 0x00000019u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02002238;
    }
a_02002200:
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_call(c, 0x02002BDDu, 0x02002207u);
    psk_w8(c->r[6] + 0x1Au, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000009u);
    goto a_0200224E;
a_0200220C:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002258;
    }
    goto a_02002238;
a_02002212:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002258;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x13u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[7] + 0x11u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002228;
    }
    c->r[1] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[1], 0x0000000Du);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002228;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
a_02002228:
    psk_w8(c->r[7] + 0x12u, (uint8_t)c->r[0]);
    goto a_02002238;
a_0200222C:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002258;
    }
    c->r[0] = psk_r8(c->r[6] + 0x1Au);
    psk_call(c, 0x02002CFFu, 0x02002237u);
    goto a_02002258;
a_02002238:
    c->r[1] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[1], 0x0000000Du);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002244;
    }
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_02002258;
a_02002244:
    c->r[0] = psk_nz(c, 0x00000008u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_02002258;
a_0200224A:
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
a_0200224E:
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02002258;
    }
    c->r[0] = psk_nz(c, 0x00000005u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
a_02002258:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002416
 | Description: Register-level C for the Thumb routine at 02002416; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002416(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[1] = psk_r8(c->r[6] + 4u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    c->r[2] = psk_nz(c, 0x00000010u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200242E;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200242A;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    goto a_0200243E;
a_0200242A:
    c->r[0] = psk_nz(c, 0x00000002u);
    goto a_0200243E;
a_0200242E:
    c->r[2] = psk_nz(c, 0x00000008u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200244E;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200243C;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    goto a_0200243E;
a_0200243C:
    c->r[0] = psk_nz(c, 0x00000000u);
a_0200243E:
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x02003CA3u, 0x02002447u);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
    goto a_0200245E;
a_0200244E:
    c->r[1] = psk_r8(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200245E;
    }
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[0] = psk_nz(c, c->r[0] ^ c->r[2]);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
a_0200245E:
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[1]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002D64
 | Description: Register-level C for the Thumb routine at 02002D64; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002D64(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[5] = 0x000004C4u;
    c->r[2] = psk_ldr(c->r[5] + 4u);
    c->r[7] = psk_r8(c->r[6] + 0x18u);
    c->r[4] = psk_nz(c, 0x000000FFu);
    c->r[3] = psk_nz(c, 0x00000000u);
a_02002D70:
    c->r[0] = psk_r8(c->r[5] + 8u);
    psk_call(c, 0x02002DE1u, 0x02002D77u);
    c->r[0] = psk_nz(c, c->r[0] & c->r[4]);
    psk_sub_flags(c, c->r[0], c->r[7]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002D7E;
    }
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000001u);
a_02002D7E:
    c->r[5] = psk_add_flags(c, c->r[5], 0x00000001u);
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002D70;
    }
    psk_w8(c->r[6] + 0x1Cu, (uint8_t)c->r[3]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002D8A
 | Description: Register-level C for the Thumb routine at 02002D8A; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002D8A(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[5] = 0x000004C4u;
    c->r[7] = psk_r8(c->r[6] + 0x18u);
    c->r[2] = psk_nz(c, 0x00000000u);
    c->r[4] = psk_nz(c, 0x000000FFu);
    c->r[3] = psk_r8(c->r[6] + 0x19u);
a_02002D96:
    c->r[0] = psk_r8(c->r[5] + 8u);
    psk_call(c, 0x02002DE1u, 0x02002D9Du);
    c->r[0] = psk_nz(c, c->r[0] & c->r[4]);
    psk_sub_flags(c, c->r[0], c->r[7]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002DA8;
    }
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002DAE;
    }
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
a_02002DA8:
    c->r[5] = psk_add_flags(c, c->r[5], 0x00000001u);
    c->r[2] = psk_add_flags(c, c->r[2], 0x00000001u);
    goto a_02002D96;
a_02002DAE:
    psk_w8(c->r[6] + 0x1Au, (uint8_t)c->r[2]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002DB4
 | Description: Register-level C for the Thumb routine at 02002DB4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002DB4(PskCpu *c) {
    psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002DC6;
    }
    c->r[5] = psk_nz(c, 0x00000020u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[1] = psk_r8(c->r[5] + 0x18u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02002DC6;
    }
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[5] + 0x18u, (uint8_t)c->r[1]);
a_02002DC6:
    c->r[5] = 0x000004C4u;
    c->r[2] = psk_ldr(c->r[5] + 4u);
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    psk_w32(c->r[5] + 4u, c->r[2]);
    c->r[2] = psk_sub_flags(c, c->r[2], c->r[0]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002DDE;
    }
    c->r[5] = psk_add_flags(c, c->r[5], c->r[0]);
a_02002DD4:
    c->r[0] = psk_r8(c->r[5] + 9u);
    psk_w8(c->r[5] + 8u, (uint8_t)c->r[0]);
    c->r[5] = psk_add_flags(c, c->r[5], 0x00000001u);
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002DD4;
    }
a_02002DDE:
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02002D22
 | Description: Register-level C for the Thumb routine at 02002D22; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002D22(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[5] = 0x000004C4u;
    c->r[2] = psk_ldr(c->r[5] + 4u);
    c->r[7] = psk_nz(c, 0x00000000u);
    c->r[4] = psk_nz(c, 0x000000FFu);
    c->r[3] = psk_nz(c, 0x00000000u);
a_02002D2E:
    c->r[0] = psk_r8(c->r[5] + 8u);
    psk_call(c, 0x02002DE1u, 0x02002D35u);
    c->r[0] = psk_nz(c, c->r[0] & c->r[4]);
    c->r[1] = psk_nz(c, 0x00000001u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002D42;
    }
a_02002D3C:
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 1u));
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002D3C;
    }
a_02002D42:
    psk_nz(c, c->r[3] & c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002D4A;
    }
    c->r[3] = psk_nz(c, c->r[3] | c->r[1]);
    c->r[7] = psk_add_flags(c, c->r[7], 0x00000001u);
a_02002D4A:
    c->r[5] = psk_add_flags(c, c->r[5], 0x00000001u);
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002D2E;
    }
    psk_w8(c->r[6] + 0x17u, (uint8_t)c->r[3]);
    psk_w8(c->r[6] + 0x1Bu, (uint8_t)c->r[7]);
    c->r[1] = psk_nz(c, 0x00000000u);
a_02002D56:
    c->r[3] = psk_nz(c, psk_lsr_s(c, c->r[3], 1u));
    if (psk_cond(c, PSK_HS)) {
        goto a_02002D5E;
    }
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    goto a_02002D56;
a_02002D5E:
    psk_w8(c->r[6] + 0x18u, (uint8_t)c->r[1]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002464
 | Description: Register-level C for the Thumb routine at 02002464; it calls 7 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002464(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[1] = psk_r8(c->r[6] + 4u);
    c->r[3] = psk_r8(c->r[6] + 3u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200247C;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020024DE;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002520;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002540;
    }
a_0200247C:
    c->r[0] = psk_r8(c->r[6] + 0x1Eu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002494;
    }
    c->r[2] = psk_nz(c, 0x00000018u);
    psk_nz(c, c->r[3] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002494;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_call(c, 0x02003D13u, 0x02002493u);
    goto a_02002638;
a_02002494:
    c->r[0] = psk_r8(c->r[6] + 0x1Bu);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200257E;
    }
    c->r[2] = psk_nz(c, 0x00000004u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020024BC;
    }
    c->r[0] = psk_r8(c->r[6] + 0x18u);
    c->r[2] = psk_r8(c->r[6] + 0x17u);
a_020024A4:
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020024AA;
    }
    c->r[0] = psk_nz(c, 0x00000004u);
a_020024AA:
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    c->r[1] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], c->r[0]));
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020024A4;
    }
    psk_w8(c->r[6] + 0x18u, (uint8_t)c->r[0]);
    psk_call(c, 0x02002D65u, 0x020024BBu);
    goto a_02002570;
a_020024BC:
    c->r[2] = psk_nz(c, 0x00000002u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200257E;
    }
    c->r[0] = psk_r8(c->r[6] + 0x18u);
    c->r[2] = psk_r8(c->r[6] + 0x17u);
a_020024C6:
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020024CE;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
a_020024CE:
    c->r[1] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], c->r[0]));
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020024C6;
    }
    psk_w8(c->r[6] + 0x18u, (uint8_t)c->r[0]);
    psk_call(c, 0x02002D65u, 0x020024DDu);
    goto a_02002570;
a_020024DE:
    c->r[2] = psk_nz(c, 0x00000018u);
    psk_nz(c, c->r[3] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020024F0;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_call(c, 0x02003D13u, 0x020024EFu);
    goto a_02002630;
a_020024F0:
    c->r[0] = psk_r8(c->r[6] + 0x1Cu);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200257E;
    }
    c->r[2] = psk_nz(c, 0x00000004u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002508;
    }
    c->r[2] = psk_r8(c->r[6] + 0x19u);
    psk_sub_flags(c, c->r[2], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002504;
    }
    c->r[2] = psk_add_flags(c, c->r[0], 0x00000000u);
a_02002504:
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    goto a_02002518;
a_02002508:
    c->r[2] = psk_nz(c, 0x00000002u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200257E;
    }
    c->r[2] = psk_r8(c->r[6] + 0x19u);
    c->r[2] = psk_add_flags(c, c->r[2], 0x00000001u);
    psk_sub_flags(c, c->r[2], c->r[0]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002518;
    }
    c->r[2] = psk_nz(c, 0x00000000u);
a_02002518:
    psk_w8(c->r[6] + 0x19u, (uint8_t)c->r[2]);
    psk_call(c, 0x02002D8Bu, 0x0200251Fu);
    goto a_02002570;
a_02002520:
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    c->r[2] = psk_nz(c, 0x00000010u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002530;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002530;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    goto a_0200253C;
a_02002530:
    c->r[2] = psk_nz(c, 0x00000008u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200257E;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200257E;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
a_0200253C:
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    goto a_02002570;
a_02002540:
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    c->r[2] = psk_nz(c, 0x00000010u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002550;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002550;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    goto a_0200255C;
a_02002550:
    c->r[2] = psk_nz(c, 0x00000008u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002564;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002564;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
a_0200255C:
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000014u);
    psk_w8(c->r[6] + 0x16u, (uint8_t)c->r[0]);
    goto a_02002570;
a_02002564:
    c->r[0] = psk_r8(c->r[6] + 0x16u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200256C;
    }
    c->r[0] = psk_nz(c, 0x00000014u);
a_0200256C:
    psk_w8(c->r[6] + 0x16u, (uint8_t)c->r[0]);
    goto a_0200257E;
a_02002570:
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x02003CA3u, 0x02002577u);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
    goto a_0200258E;
a_0200257E:
    c->r[1] = psk_r8(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200258E;
    }
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[0] = psk_nz(c, c->r[0] ^ c->r[2]);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
a_0200258E:
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[1]);
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002638;
    }
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_call(c, 0x02003D13u, 0x0200259Fu);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020025BE;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020025C8;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002618;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x19u, (uint8_t)c->r[0]);
    psk_call(c, 0x02002D65u, 0x020025B9u);
    psk_call(c, 0x02002D8Bu, 0x020025BDu);
    goto a_02002630;
a_020025BE:
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    goto a_02002630;
a_020025C8:
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002612;
    }
    c->r[0] = psk_r8(c->r[6] + 0x1Au);
    c->r[1] = psk_r8(c->r[6] + 0x1Eu);
    psk_call(c, 0x02002DB5u, 0x020025D7u);
    c->r[0] = psk_r8(c->r[6] + 0x1Eu);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002626;
    }
    psk_call(c, 0x0200421Du, 0x020025E1u);
    c->r[5] = 0x000004C4u;
    c->r[0] = psk_ldr(c->r[5] + 4u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020025EE;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_02002638;
a_020025EE:
    c->r[0] = psk_r8(c->r[6] + 0x18u);
    c->r[8] = c->r[0];
    psk_call(c, 0x02002D23u, 0x020025F7u);
    c->r[0] = psk_r8(c->r[6] + 0x19u);
    c->r[1] = psk_r8(c->r[6] + 0x1Cu);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200262C;
    }
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002608;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x19u, (uint8_t)c->r[0]);
a_02002608:
    psk_w8(c->r[6] + 0x1Cu, (uint8_t)c->r[1]);
    c->r[0] = c->r[8];
    psk_w8(c->r[6] + 0x18u, (uint8_t)c->r[0]);
    psk_call(c, 0x02002D8Bu, 0x02002613u);
a_02002612:
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    goto a_02002630;
a_02002618:
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200262C;
    }
    c->r[5] = 0x000004C4u;
    c->r[0] = psk_ldr(c->r[5] + 4u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w32(c->r[5] + 4u, c->r[0]);
a_02002626:
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_02002638;
a_0200262C:
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
a_02002630:
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Cu);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
a_02002638:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002814
 | Description: Register-level C for the Thumb routine at 02002814; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002814(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000001Eu);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    psk_call(c, 0x02004437u, 0x02002825u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002E00
 | Description: Register-level C for the ARM routine at 02002E00; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002E00(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[4] = 0x000003C4u;
    psk_w8(c->r[4] + 6u, (uint8_t)c->r[0]);
    psk_w8(c->r[4] + 7u, (uint8_t)c->r[0]);
    c->r[0] = 0x00000000u;
    psk_w8(c->r[4] + 5u, (uint8_t)c->r[0]);
    psk_w8(c->r[4] + 2u, (uint8_t)c->r[0]);
    psk_w8(c->r[4] + 3u, (uint8_t)c->r[0]);
    psk_w8(c->r[4] + 4u, (uint8_t)c->r[0]);
    c->r[0] = 0x00000000u;
    c->r[1] = 0x02002F8Cu;
    psk_call(c, 0x02002BC4u, 0x02002E30u);
    c->r[0] = 0x00000001u;
    psk_call(c, 0x02002BC4u, 0x02002E38u);
    c->r[0] = 0x00000002u;
    psk_call(c, 0x02002BC4u, 0x02002E40u);
    c->r[0] = 0x00000003u;
    psk_call(c, 0x02002BC4u, 0x02002E48u);
    c->r[0] = 0x00000004u;
    psk_call(c, 0x02002BC4u, 0x02002E50u);
    c->r[0] = 0x0A000000u;
    c->r[1] = 0x0000001Fu;
    psk_w32(c->r[0] + 8u, c->r[1]);
    {
        uint32_t a = c->r[13];
        c->r[14] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02002FC0
 | Description: Register-level C for the ARM routine at 02002FC0.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002FC0(PskCpu *c) {
    c->r[4] = 0x0D000000u;
    c->r[3] = psk_ldr(c->r[4]);
    c->r[3] = c->r[3] & ~0x0000007Fu;
    c->r[3] = c->r[3] | 0x00000048u;
    c->r[3] = c->r[3] | c->r[0];
    c->r[3] = c->r[3] | ((c->r[1] << 4u));
    psk_w32(c->r[4], c->r[3]);
    c->r[3] = 0x00000000u;
    psk_w32(c->r[4] + 0x20u, c->r[3]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_read_buttons
 | Description: Reads the buttons into 0x3C6-0x3C7 with new presses and auto-repeat; r3 is 1 after a long action hold when r0 is 1. Register-level C for the ARM routine at 02002E64.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_read_buttons(PskCpu *c) {
    c->r[4] = 0x000003C4u;
    c->r[3] = 0x00000000u;
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002E90;
    }
    c->r[0] = psk_r8(c->r[4] + 5u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002E90;
    }
    c->r[0] = psk_r8(c->r[4] + 7u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002E94;
    }
    c->r[3] = 0x00000001u;
a_02002E90:
    c->r[0] = psk_r8(c->r[4] + 6u);
a_02002E94:
    psk_w8(c->r[4] + 7u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[4] + 5u);
    c->r[1] = psk_r8(c->r[4] + 2u);
    psk_w8(c->r[4] + 2u, (uint8_t)c->r[0]);
    c->r[1] = c->r[1] ^ c->r[0];
    c->r[1] = c->r[1] & c->r[0];
    psk_w8(c->r[4] + 3u, (uint8_t)c->r[1]);
    psk_nz(c, c->r[0] & 0x00000010u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002EE0;
    }
    psk_nz(c, c->r[1] & 0x00000010u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002ED8;
    }
    c->r[2] = psk_r8(c->r[4] + 9u);
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002EDC;
    }
    c->r[1] = c->r[1] | 0x00000010u;
    c->r[2] = 0x00000006u;
    goto a_02002EDC;
a_02002ED8:
    c->r[2] = 0x00000016u;
a_02002EDC:
    psk_w8(c->r[4] + 9u, (uint8_t)c->r[2]);
a_02002EE0:
    psk_nz(c, c->r[0] & 0x00000008u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002F10;
    }
    psk_nz(c, c->r[1] & 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002F08;
    }
    c->r[2] = psk_r8(c->r[4] + 0xAu);
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002F0C;
    }
    c->r[1] = c->r[1] | 0x00000008u;
    c->r[2] = 0x00000006u;
    goto a_02002F0C;
a_02002F08:
    c->r[2] = 0x00000016u;
a_02002F0C:
    psk_w8(c->r[4] + 0xAu, (uint8_t)c->r[2]);
a_02002F10:
    psk_nz(c, c->r[0] & 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002F40;
    }
    psk_nz(c, c->r[1] & 0x00000004u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002F38;
    }
    c->r[2] = psk_r8(c->r[4] + 0xBu);
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002F3C;
    }
    c->r[1] = c->r[1] | 0x00000004u;
    c->r[2] = 0x00000006u;
    goto a_02002F3C;
a_02002F38:
    c->r[2] = 0x00000016u;
a_02002F3C:
    psk_w8(c->r[4] + 0xBu, (uint8_t)c->r[2]);
a_02002F40:
    psk_nz(c, c->r[0] & 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002F70;
    }
    psk_nz(c, c->r[1] & 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002F68;
    }
    c->r[2] = psk_r8(c->r[4] + 0xCu);
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002F6C;
    }
    c->r[1] = c->r[1] | 0x00000002u;
    c->r[2] = 0x00000006u;
    goto a_02002F6C;
a_02002F68:
    c->r[2] = 0x00000016u;
a_02002F6C:
    psk_w8(c->r[4] + 0xCu, (uint8_t)c->r[2]);
a_02002F70:
    psk_w8(c->r[4] + 4u, (uint8_t)c->r[1]);
    c->r[5] = 0x00000000u;
    psk_w8(c->r[4] + 5u, (uint8_t)c->r[5]);
    c->r[4] = 0x0A000000u;
    c->r[5] = 0x0000001Fu;
    psk_w32(c->r[4] + 8u, c->r[5]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_entry
 | Description: App entry: sets the stack, clears work RAM, sets interrupt masks and callbacks, then enters the main loop. Unverified (never returns, so only Task 8's all-C run checks it). Register-level C for the ARM routine at 020027A8; SWI 01, 04.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_entry(PskCpu *c) {
    c->r[13] = 0x00000800u;
    c->r[0] = 0x00000005u;
    c->r[0] = psk_swi(0x04u, c->r[0], c->r[1], c->r[2]);
    c->r[0] = 0x00000200u;
    c->r[1] = 0x00000000u;
    c->r[2] = 0x00000000u;
    c->r[3] = 0x00000000u;
    c->r[4] = 0x00000000u;
a_020027C8:
    {
        uint32_t a = c->r[0];
        psk_w32(a, c->r[1]);
        psk_w32(a + 4u, c->r[2]);
        psk_w32(a + 8u, c->r[3]);
        psk_w32(a + 0xCu, c->r[4]);
        c->r[0] = a + 0x10u;
    }
    psk_sub_flags(c, c->r[0], 0x00000800u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020027C8;
    }
    c->r[0] = 0x0A000000u;
    c->r[1] = 0x00003FFFu;
    psk_w32(c->r[0] + 0xCu, c->r[1]);
    c->r[0] = 0x00000002u;
    c->r[1] = 0x02002B30u;
    c->r[0] = psk_swi(0x01u, c->r[0], c->r[1], c->r[2]);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x02002B5Cu;
    c->r[0] = psk_swi(0x01u, c->r[0], c->r[1], c->r[2]);
    c->r[0] = 0x0A000000u;
    c->r[1] = 0x00000E00u;
    psk_w32(c->r[0] + 8u, c->r[1]);
    c->r[15] = 0x02003348u;
    return;
}

/*----------------------
 | rrl_func_02002828
 | Description: Register-level C for the Thumb routine at 02002828.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002828(PskCpu *c) {
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Cu);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02002838
 | Description: Register-level C for the Thumb routine at 02002838.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002838(PskCpu *c) {
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[7] + 4u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Cu);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_0200290A
 | Description: Register-level C for the Thumb routine at 0200290A; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200290A(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7] + 0x12u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200291A;
    }
    psk_call(c, 0x0200421Du, 0x0200291Bu);
a_0200291A:
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000005u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[7] + 0xBu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0xCu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_w8(c->r[7] + 0xDu, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[7] + 6u);
    psk_w8(c->r[7] + 8u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[7] + 7u);
    psk_w8(c->r[7] + 9u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0xAu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000005Eu);
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x17u, (uint8_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_0200284A
 | Description: Register-level C for the Thumb routine at 0200284A; it calls 6 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4, WRAM 0x4F4, WRAM 0x564
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200284A(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7] + 6u);
    psk_w8(c->r[7] + 8u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[7] + 7u);
    psk_w8(c->r[7] + 9u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0xAu, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 0xBu, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[7] + 5u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020028C0;
    }
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_w8(c->r[7], (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0xEu, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 1u, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[7] + 0x10u, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 0x11u, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x13u, (uint8_t)c->r[0]);
    c->r[5] = 0x000004C4u;
    c->r[0] = psk_ldr(c->r[5] + 4u);
    psk_w8(c->r[7] + 0x18u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[7] + 0x1Au) & ~1u, (uint16_t)c->r[0]);
    psk_call(c, 0x0200397Du, 0x0200288Bu);
    c->r[0] = psk_r8(c->r[7] + 4u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[2] = psk_sub_flags(c, c->r[2], c->r[0]);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 1u));
    c->r[7] = 0x000004F4u;
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_r8(c->r[7] + 0xCu);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000008u);
    psk_call(c, 0x0200398Fu, 0x0200289Fu);
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5] + 4u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000040u);
    psk_call(c, 0x020042A1u, 0x020028ADu);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000004u);
    c->r[1] = 0x00000564u;
    c->r[2] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x020049A7u, 0x020028B9u);
    psk_w8(c->r[5] + 7u, (uint8_t)c->r[0]);
    psk_call(c, 0x02003EA5u, 0x020028BFu);
    goto a_020028D4;
a_020028C0:
    c->r[1] = psk_r8(c->r[7]);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 5u));
    c->r[0] = psk_r8(c->r[7] + 1u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020028D4;
    }
    c->r[0] = psk_nz(c, 0x00000008u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    psk_call(c, 0x0200290Bu, 0x020028D5u);
a_020028D4:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020028D8
 | Description: Register-level C for the Thumb routine at 020028D8.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020028D8(PskCpu *c) {
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 5u, (uint8_t)c->r[0]);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Cu);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020028E8
 | Description: Register-level C for the Thumb routine at 020028E8.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020028E8(PskCpu *c) {
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020028EE
 | Description: Register-level C for the Thumb routine at 020028EE; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020028EE(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02004355u, 0x020028F5u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020028F8
 | Description: Register-level C for the Thumb routine at 020028F8.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020028F8(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[7] + 0xAu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_0200294E
 | Description: Register-level C for the Thumb routine at 0200294E; it calls 6 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200294E(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, 0x00000005u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[1] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020029A0;
    }
    psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002968;
    }
    psk_call(c, 0x020044B7u, 0x02002967u);
    goto a_020029C0;
a_02002968:
    c->r[5] = 0x000004C4u;
    c->r[0] = psk_ldr(c->r[5] + 4u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002986;
    }
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x02002977u);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[5] = psk_add_flags(c, c->r[5], 0x00000008u);
    c->r[1] = psk_r8(c->r[5] + c->r[0]);
    psk_w8(c->r[6] + 0x16u, (uint8_t)c->r[1]);
    c->r[1] = psk_nz(c, 0x00000001u);
    psk_call(c, 0x02002DB5u, 0x02002985u);
    goto a_0200298A;
a_02002986:
    c->r[1] = psk_nz(c, 0x000000FFu);
    psk_w8(c->r[6] + 0x16u, (uint8_t)c->r[1]);
a_0200298A:
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_call(c, 0x02002BDDu, 0x02002991u);
    psk_w8(c->r[6] + 0x17u, (uint8_t)c->r[0]);
    c->r[5] = 0x000004C4u;
    c->r[1] = psk_ldr(c->r[5] + 4u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    psk_w32(c->r[5] + 4u, c->r[1]);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[1]);
    psk_w8(c->r[5] + 7u, (uint8_t)c->r[0]);
    goto a_020029C0;
a_020029A0:
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7] + 0x10u);
    c->r[1] = psk_r8(c->r[7] + 0x11u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020029B6;
    }
    psk_sub_flags(c, c->r[0], 0x00000009u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020029BA;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 0x10u, (uint8_t)c->r[0]);
    goto a_020029BA;
a_020029B6:
    c->r[0] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
a_020029BA:
    psk_w8(c->r[7] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x13u, (uint8_t)c->r[0]);
a_020029C0:
    psk_call(c, 0x020039F5u, 0x020029C5u);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_r8(c->r[6] + 0x15u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000058u);
    psk_call(c, 0x02003A07u, 0x020029CFu);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020029D2
 | Description: Register-level C for the Thumb routine at 020029D2; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020029D2(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000005u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    psk_call(c, 0x020039F5u, 0x020029E1u);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x0000005Bu);
    psk_call(c, 0x02003A07u, 0x020029E9u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020029EC
 | Description: Register-level C for the Thumb routine at 020029EC; it calls 6 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4F4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020029EC(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[5] = psk_nz(c, 0x00000020u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5] + 0xAu, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[5] + 6u);
    psk_w8(c->r[5] + 8u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[5] + 7u);
    psk_w8(c->r[5] + 9u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x1Bu, (uint8_t)c->r[0]);
    c->r[1] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[1], 0x0000000Du);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002A7A;
    }
    c->r[5] = 0x000004F4u;
    c->r[0] = psk_r8(c->r[5] + 0xCu);
    c->r[4] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[5] = psk_add_flags(c, c->r[1], 0x00000000u);
    psk_call(c, 0x020041DFu, 0x02002A15u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002A1E;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x1Bu, (uint8_t)c->r[0]);
    goto a_02002A2E;
a_02002A1E:
    c->r[0] = psk_add_flags(c, c->r[5], 0x00000000u);
    c->r[1] = psk_add_flags(c, c->r[4], 0x00000000u);
    psk_call(c, 0x020041DFu, 0x02002A27u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002A2E;
    }
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_w8(c->r[6] + 0x1Bu, (uint8_t)c->r[0]);
a_02002A2E:
    psk_call(c, 0x020041BBu, 0x02002A33u);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[5] = psk_nz(c, 0x00000070u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_r8(c->r[5] + 1u);
    psk_w8(c->r[6] + 0x16u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_r8(c->r[6] + 0x15u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000008u);
    c->r[2] = psk_nz(c, 0x00000002u);
    psk_call(c, 0x0200398Fu, 0x02002A4Fu);
    c->r[0] = psk_nz(c, 0x00000028u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000014u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    psk_call(c, 0x020039F5u, 0x02002A5Bu);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000062u);
    psk_call(c, 0x02003A07u, 0x02002A63u);
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, 0x00000069u);
    psk_call(c, 0x02003A07u, 0x02002A6Bu);
    c->r[0] = psk_nz(c, 0x00000002u);
    c->r[1] = psk_nz(c, 0x00000063u);
    psk_call(c, 0x02003A07u, 0x02002A73u);
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_call(c, 0x02003D13u, 0x02002A79u);
    goto a_02002AA2;
a_02002A7A:
    c->r[1] = psk_nz(c, 0x00000004u);
    psk_w8(c->r[6] + 0x19u, (uint8_t)c->r[1]);
    c->r[0] = psk_nz(c, 0x0000000Au);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000020u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000040u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x1Cu, (uint8_t)c->r[0]);
    psk_w8(c->r[6] + 0x1Du, (uint8_t)c->r[0]);
    c->r[5] = psk_nz(c, 0x00000070u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_r8(c->r[5] + 1u);
    psk_w8(c->r[6] + 0x16u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, 0x00000015u);
    c->r[2] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x0200398Fu, 0x02002AA3u);
a_02002AA2:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002ABC
 | Description: Register-level C for the Thumb routine at 02002ABC.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002ABC(PskCpu *c) {
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7] + 4u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02002ACC;
    }
    c->r[0] = psk_nz(c, 0x00000008u);
    c->r[1] = psk_nz(c, 0x00000000u);
    goto a_02002AD0;
a_02002ACC:
    c->r[0] = psk_nz(c, 0x00000004u);
    c->r[1] = psk_nz(c, 0x00000010u);
a_02002AD0:
    psk_w8(c->r[7] + 7u, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 6u, (uint8_t)c->r[1]);
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_w8(c->r[7] + 5u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Cu);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02002AE4
 | Description: Register-level C for the Thumb routine at 02002AE4; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002AE4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[6] + 0x1Eu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002AF2;
    }
    c->r[0] = psk_nz(c, 0x00000003u);
a_02002AF2:
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000014u);
    psk_w8(c->r[6] + 0x16u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Cu);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    psk_call(c, 0x02002D23u, 0x02002B09u);
    psk_call(c, 0x02002D65u, 0x02002B0Du);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02002B88
 | Description: Register-level C for the ARM routine at 02002B88.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x240
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002B88(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        psk_w32(a - 8u, c->r[9]);
        c->r[13] = a - 8u;
    }
    c->r[10] = 0x00000240u;
    c->r[0] = 0x00001000u;
a_02002B94:
    psk_nz(c, c->r[9] & c->r[0]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002BB4;
    }
    c->r[11] = psk_ldr(c->r[10] - 8u);
    psk_sub_flags(c, c->r[11], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        c->r[14] = 0x02002BACu;
    }
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, psk_bx(c->r[11]), 0x02002BACu);
    }
    c->r[9] = psk_nz(c, c->r[9] & ~c->r[0]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002BC0;
    }
a_02002BB4:
    c->r[10] = c->r[10] - 0x00000004u;
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 1u));
    if (psk_cond(c, PSK_NE)) {
        goto a_02002B94;
    }
a_02002BC0:
    {
        uint32_t a = c->r[13];
        c->r[9] = psk_r32(a);
        c->r[15] = psk_r32(a + 4u) & ~3u;
        c->r[13] = a + 8u;
        return;
    }
}

/*----------------------
 | rrl_fiq_callback
 | Description: Kernel FIQ callback: acks Timer2 and jumps to the sound driver stored at 0x23C, or falls into the IRQ callback. Register-level C for the ARM routine at 02002B30; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x23C
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_fiq_callback(PskCpu *c) {
    c->r[8] = 0x0A000000u;
    c->r[9] = psk_ldr(c->r[8]);
    c->r[10] = psk_ldr(c->r[8] + 8u);
    c->r[9] = c->r[9] & c->r[10];
    psk_set_carry(c, 0u);
    c->r[9] = psk_nz(c, c->r[9] & 0x00002000u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[15] = c->r[14] & ~3u;
        return;
    }
    psk_w32(c->r[8] + 0x10u, c->r[9]);
    c->r[10] = 0x0000023Cu;
    c->r[11] = psk_ldr(c->r[10]);
    psk_sub_flags(c, c->r[11], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = psk_bx(c->r[11]);
        return;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        psk_w32(a - 8u, c->r[11]);
        psk_w32(a - 0xCu, c->r[10]);
        psk_w32(a - 0x10u, c->r[9]);
        psk_w32(a - 0x14u, c->r[8]);
        psk_w32(a - 0x18u, c->r[0]);
        c->r[13] = a - 0x18u;
    }
    c->r[8] = 0x0A000000u;
    c->r[9] = psk_ldr(c->r[8]);
    c->r[0] = psk_ldr(c->r[8] + 8u);
    c->r[9] = c->r[9] & c->r[0];
    c->r[9] = c->r[9] & ~0x00002040u;
    psk_call(c, 0x02002B88u, 0x02002B78u);
    c->r[9] = c->r[9] & ~0x00000A00u;
    psk_w32(c->r[8] + 0x10u, c->r[9]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[8] = psk_r32(a + 4u);
        c->r[9] = psk_r32(a + 8u);
        c->r[10] = psk_r32(a + 0xCu);
        c->r[11] = psk_r32(a + 0x10u);
        c->r[14] = psk_r32(a + 0x14u);
        c->r[13] = a + 0x18u;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02002F8C
 | Description: Register-level C for the ARM routine at 02002F8C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02002F8C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[4]);
        psk_w32(a - 8u, c->r[2]);
        psk_w32(a - 0xCu, c->r[1]);
        c->r[13] = a - 0xCu;
    }
    c->r[1] = c->r[0] & 0x0000001Fu;
    c->r[9] = c->r[9] & ~c->r[1];
    c->r[4] = 0x000003C4u;
    c->r[2] = psk_r8(c->r[4] + 5u);
    c->r[2] = c->r[2] | c->r[1];
    psk_w8(c->r[4] + 5u, (uint8_t)c->r[2]);
    c->r[4] = 0x0A000000u;
    psk_w32(c->r[4] + 0xCu, c->r[1]);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[2] = psk_r32(a + 4u);
        c->r[4] = psk_r32(a + 8u);
        c->r[13] = a + 0xCu;
    }
    c->r[15] = c->r[14] & ~3u;
    return;
}

/*----------------------
 | rrl_irq_callback
 | Description: Kernel IRQ callback: acks this IRQ's sources and runs each timer's handler through 02002B88. Register-level C for the ARM routine at 02002B5C; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_irq_callback(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        psk_w32(a - 8u, c->r[11]);
        psk_w32(a - 0xCu, c->r[10]);
        psk_w32(a - 0x10u, c->r[9]);
        psk_w32(a - 0x14u, c->r[8]);
        psk_w32(a - 0x18u, c->r[0]);
        c->r[13] = a - 0x18u;
    }
    c->r[8] = 0x0A000000u;
    c->r[9] = psk_ldr(c->r[8]);
    c->r[0] = psk_ldr(c->r[8] + 8u);
    c->r[9] = c->r[9] & c->r[0];
    c->r[9] = c->r[9] & ~0x00002040u;
    psk_call(c, 0x02002B88u, 0x02002B78u);
    c->r[9] = c->r[9] & ~0x00000A00u;
    psk_w32(c->r[8] + 0x10u, c->r[9]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[8] = psk_r32(a + 4u);
        c->r[9] = psk_r32(a + 8u);
        c->r[10] = psk_r32(a + 0xCu);
        c->r[11] = psk_r32(a + 0x10u);
        c->r[14] = psk_r32(a + 0x14u);
        c->r[13] = a + 0x18u;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}
