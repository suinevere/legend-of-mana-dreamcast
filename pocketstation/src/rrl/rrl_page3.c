/*----------------------
 | rrl_page3.c
 | Description: Ring Ring Land routines at 02003000-02003FFF as register-level C, until they are named and grouped by concern.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "rrl.h"

/*----------------------
 | rrl_func_02003858
 | Description: Unverified (quit path, needs the dock edge or YES in the quit dialog). Register-level C for the ARM routine at 02003858; it calls 1 routine and SWI 01.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003858(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = 0x00000001u;
    psk_call(c, 0x02008294u, 0x02003864u);
    c->r[0] = 0x00000002u;
    psk_call(c, 0x02008294u, 0x0200386Cu);
    c->r[0] = 0x0D800000u;
    c->r[1] = 0x00000002u;
    psk_w32(c->r[0], c->r[1]);
    c->r[1] = 0x00000062u;
    psk_w32(c->r[0] + 4u, c->r[1]);
    c->r[1] = 0x00000000u;
    psk_w32(c->r[0] + 0x10u, c->r[1]);
    c->r[0] = 0x0C800000u;
    c->r[1] = 0x00000002u;
    psk_w32(c->r[0], c->r[1]);
    c->r[0] = 0x0A000000u;
    c->r[1] = 0x00003FFFu;
    psk_w32(c->r[0] + 0xCu, c->r[1]);
    c->r[0] = 0x00000002u;
    c->r[1] = 0x00000000u;
    c->r[0] = psk_swi(0x01u, c->r[0], c->r[1], c->r[2]);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00000000u;
    c->r[0] = psk_swi(0x01u, c->r[0], c->r[1], c->r[2]);
    c->r[0] = 0x0A000000u;
    c->r[1] = 0x00000A00u;
    psk_w32(c->r[0] + 8u, c->r[1]);
a_020038C4:
    c->r[0] = 0x0A000000u;
    c->r[1] = psk_ldr(c->r[0] + 4u);
    psk_nz(c, c->r[1] & 0x0000003Fu);
    if (psk_cond(c, PSK_NE)) {
        psk_idle();
        goto a_020038C4;
    }
    {
        uint32_t a = c->r[13];
        c->r[14] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02003DB4
 | Description: Register-level C for the Thumb routine at 02003DB4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003DB4(PskCpu *c) {
    c->r[4] = psk_nz(c, 0x000000D8u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 20u));
    c->r[0] = psk_nz(c, 0x00000020u);
    psk_w32(c->r[4] + 4u, c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w32(c->r[4] + 0x10u, c->r[0]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02003D84
 | Description: Register-level C for the Thumb routine at 02003D84; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x5C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003D84(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_call(c, 0x02004311u, 0x02003D8Du);
    psk_call(c, 0x0200433Bu, 0x02003D91u);
    c->r[1] = 0x000005C4u;
    c->r[2] = psk_nz(c, 0x000000FFu);
    c->r[0] = psk_r8(c->r[1] + 0xCu);
    psk_w8(c->r[1] + 0xCu, (uint8_t)c->r[2]);
    psk_w8(c->r[1] + 0xDu, (uint8_t)c->r[2]);
    psk_sub_flags(c, c->r[0], c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003DA2;
    }
    psk_call(c, 0x02003DB5u, 0x02003DA3u);
a_02003DA2:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02003304
 | Description: Register-level C for the Thumb routine at 02003304.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x240
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003304(PskCpu *c) {
    c->r[0] = 0x00000240u;
    c->r[1] = psk_nz(c, 0x00000080u);
    c->r[2] = psk_nz(c, 0x00000000u);
a_0200330A:
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000004u);
    psk_w32(c->r[0] + c->r[1], c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200330A;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020032D4
 | Description: Register-level C for the ARM routine at 020032D4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x240
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020032D4(PskCpu *c) {
    c->r[0] = 0x00000240u;
    c->r[1] = 0x0D000000u;
    c->r[1] = c->r[1] + 0x00000100u;
    c->r[2] = 0x00000004u;
a_020032E4:
    {
        uint32_t a = c->r[0];
        c->r[3] = psk_r32(a);
        c->r[4] = psk_r32(a + 4u);
        c->r[5] = psk_r32(a + 8u);
        c->r[6] = psk_r32(a + 0xCu);
        c->r[7] = psk_r32(a + 0x10u);
        c->r[8] = psk_r32(a + 0x14u);
        c->r[9] = psk_r32(a + 0x18u);
        c->r[10] = psk_r32(a + 0x1Cu);
        c->r[0] = a + 0x20u;
    }
    {
        uint32_t a = c->r[1];
        psk_w32(a, c->r[3]);
        psk_w32(a + 4u, c->r[4]);
        psk_w32(a + 8u, c->r[5]);
        psk_w32(a + 0xCu, c->r[6]);
        psk_w32(a + 0x10u, c->r[7]);
        psk_w32(a + 0x14u, c->r[8]);
        psk_w32(a + 0x18u, c->r[9]);
        psk_w32(a + 0x1Cu, c->r[10]);
        c->r[1] = a + 0x20u;
    }
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020032E4;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02003DD8
 | Description: Register-level C for the ARM routine at 02003DD8; it calls 6 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x5D4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003DD8(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008294u, 0x02003DE0u);
    psk_call(c, 0x020082A0u, 0x02003DE4u);
    c->r[3] = 0x00000000u;
    psk_call(c, 0x0200845Cu, 0x02003DECu);
    c->r[4] = 0x000005D4u;
    c->r[5] = 0x00000000u;
    {
        uint32_t b = c->r[4] + ((c->r[0] << 1u));
        psk_w8(b, (uint8_t)c->r[5]);
        c->r[4] = b;
    }
    psk_w8(c->r[4] + 1u, (uint8_t)c->r[5]);
    c->r[4] = 0x00000007u;
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[4] = 0x00000008u;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[4] = 0x0000000Du;
    }
    c->r[1] = 0x02003E70u;
    c->r[5] = c->r[0];
    c->r[0] = c->r[4];
    psk_call(c, 0x02002BC4u, 0x02003E20u);
    c->r[0] = c->r[5];
    psk_call(c, 0x02008468u, 0x02003E28u);
    psk_call(c, 0x020082C4u, 0x02003E2Cu);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_wait_ticks
 | Description: Marks the frame, then idles until tick counter r0 has advanced by r1. Register-level C for the ARM routine at 02003E48.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x5D4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_wait_ticks(PskCpu *c) {
    psk_frame();
    c->r[4] = 0x000005D4u;
    c->r[4] = c->r[4] + (c->r[0] << 1u);
    c->r[6] = psk_r8(c->r[4] + 1u);
a_02003E54:
    c->r[5] = psk_r8(c->r[4]);
    c->r[7] = c->r[5] - c->r[6];
    c->r[7] = c->r[7] & 0x000000FFu;
    psk_sub_flags(c, c->r[7], c->r[1]);
    if (psk_cond(c, PSK_LO)) {
        psk_idle();
        goto a_02003E54;
    }
    psk_w8(c->r[4] + 1u, (uint8_t)c->r[5]);
    c->r[15] = c->r[14] & ~3u;
    return;
}

/*----------------------
 | rrl_func_02003DA6
 | Description: Register-level C for the Thumb routine at 02003DA6.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003DA6(PskCpu *c) {
    c->r[4] = psk_nz(c, 0x000000D8u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 20u));
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w32(c->r[4] + 0x10u, c->r[0]);
    c->r[0] = psk_nz(c, 0x00000020u);
    psk_w32(c->r[4] + 8u, c->r[0]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02003D44
 | Description: Register-level C for the Thumb routine at 02003D44; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x5C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003D44(PskCpu *c) {
    c->r[1] = 0x000005C4u;
    c->r[0] = psk_r8(c->r[1] + 0xFu);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003D80;
    }
    c->r[0] = psk_ldr(c->r[1] + 8u);
    psk_w32(c->r[1] + 4u, c->r[0]);
    psk_w8(c->r[1] + 0xDu, (uint8_t)c->r[5]);
    c->r[2] = psk_nz(c, 0x000000FFu);
    c->r[0] = psk_r8(c->r[1] + 0xCu);
    psk_sub_flags(c, c->r[0], c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003D80;
    }
    psk_w8(c->r[1] + 0xDu, (uint8_t)c->r[2]);
    psk_w8(c->r[1] + 0xCu, (uint8_t)c->r[5]);
    c->r[3] = 0x0200A7C8u;
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 2u));
    c->r[3] = psk_add_flags(c, c->r[3], c->r[5]);
    c->r[0] = psk_ldr(c->r[3]);
    c->r[2] = psk_ldr(c->r[3] + 4u);
    psk_w32(c->r[1], c->r[0]);
    c->r[2] = psk_sub_flags(c, c->r[2], c->r[0]);
    psk_w32(c->r[1] + 8u, c->r[2]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w32(c->r[1] + 4u, c->r[0]);
    psk_call(c, 0x02003DA7u, 0x02003D77u);
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_call(c, 0x02004323u, 0x02003D7Du);
    psk_call(c, 0x020042FFu, 0x02003D81u);
a_02003D80:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02003D42
 | Description: Register-level C for the Thumb routine at 02003D42.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003D42(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[15] = 0x02003D45u;
    return;
}

/*----------------------
 | rrl_func_02003D12
 | Description: Register-level C for the Thumb routine at 02003D12; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x204, WRAM 0x3C0, WRAM 0x5C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003D12(PskCpu *c) {
    c->r[5] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[1] = 0x00000204u;
    c->r[0] = psk_r8(c->r[1]);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x02003D85u;
        return;
    }
    c->r[1] = 0x000003C0u;
    c->r[2] = psk_ldr(c->r[1]);
    c->r[1] = psk_ldr(c->r[2]);
    c->r[2] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x02003D85u;
        return;
    }
    c->r[1] = 0x000005C4u;
    c->r[3] = psk_r8(c->r[1] + 0xCu);
    psk_sub_flags(c, c->r[3], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x02003D43u;
        return;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, 0x00000002u);
    c->r[1] = psk_nz(c, 0x000000FAu);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 1u));
    c->r[2] = psk_nz(c, 0x00000000u);
    c->r[3] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x020042EDu, 0x02003D41u);
    c->r[15] = 0x02003D45u;
    return;
}

/*----------------------
 | rrl_func_02003316
 | Description: Register-level C for the Thumb routine at 02003316.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003316(PskCpu *c) {
    c->r[4] = 0x0200A03Cu;
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 3u));
    c->r[4] = psk_add_flags(c, c->r[4], c->r[2]);
    c->r[5] = psk_ldr(c->r[4]);
    c->r[2] = psk_add_flags(c, c->r[5], 0x00000000u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 26u));
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 26u));
    c->r[0] = psk_add_flags(c, c->r[0], c->r[2]);
    c->r[2] = psk_add_flags(c, c->r[5], 0x00000000u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 16u));
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 24u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[2]);
    c->r[2] = psk_add_flags(c, c->r[5], 0x00000000u);
    c->r[3] = psk_add_flags(c, c->r[5], 0x00000000u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 8u));
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 24u));
    c->r[3] = psk_nz(c, psk_lsr_s(c, c->r[3], 24u));
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 24u));
    c->r[5] = psk_nz(c, psk_lsr_s(c, c->r[5], 30u));
    c->r[4] = psk_ldr(c->r[4] + 4u);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02003312
 | Description: Register-level C for the Thumb routine at 02003312.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003312(PskCpu *c) {
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[15] = 0x02003317u;
    return;
}

/*----------------------
 | rrl_func_02003CA2
 | Description: Register-level C for the Thumb routine at 02003CA2.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x5C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003CA2(PskCpu *c) {
    c->r[1] = 0x000005C4u;
    psk_w8(c->r[1] + 0xEu, (uint8_t)c->r[0]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020039C2
 | Description: Register-level C for the Thumb routine at 020039C2.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020039C2(PskCpu *c) {
    c->r[5] = psk_nz(c, 0x00000070u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[7] = 0x0200A7B0u;
    c->r[3] = psk_nz(c, 0x0000000Au);
a_020039CA:
    c->r[0] = psk_r8(c->r[5]);
    psk_sub_flags(c, c->r[0], 0x000000FFu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020039EC;
    }
    c->r[1] = psk_r8(c->r[5] + 3u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020039EA;
    }
    c->r[1] = psk_r8(c->r[5] + 2u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, c->r[1] ^ c->r[2]);
    psk_w8(c->r[5] + 2u, (uint8_t)c->r[1]);
    c->r[1] = psk_nz(c, 0x000000FEu);
    c->r[1] = psk_nz(c, c->r[1] & c->r[0]);
    c->r[1] = psk_ldrh(c->r[7] + c->r[1]);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 1u));
    if (psk_cond(c, PSK_LO)) {
        goto a_020039EA;
    }
    c->r[1] = psk_nz(c, psk_lsr_s(c, c->r[1], 8u));
a_020039EA:
    psk_w8(c->r[5] + 3u, (uint8_t)c->r[1]);
a_020039EC:
    c->r[5] = psk_add_flags(c, c->r[5], 0x00000004u);
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020039CA;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02003A54
 | Description: Register-level C for the Thumb routine at 02003A54.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003A54(PskCpu *c) {
    c->r[1] = 0x0200A384u;
    c->r[0] = psk_r8(c->r[7]);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000058u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 2u));
    c->r[0] = psk_ldr(c->r[1] + c->r[0]);
    c->r[2] = psk_r8(c->r[7] + 1u);
    c->r[4] = psk_ldr(c->r[0]);
    c->r[3] = psk_nz(c, 0x000000FFu);
    c->r[3] = psk_nz(c, c->r[3] & c->r[4]);
    psk_sub_flags(c, c->r[3], 0x00000080u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003A86;
    }
    psk_sub_flags(c, c->r[3], 0x00000081u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003A72;
    }
    c->r[4] = psk_nz(c, 0x00000000u);
    goto a_02003A76;
a_02003A72:
    c->r[4] = psk_nz(c, psk_lsr_s(c, c->r[4], 8u));
    c->r[2] = psk_add_flags(c, c->r[2], 0x00000001u);
a_02003A76:
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 2u));
    c->r[2] = psk_ldr(c->r[0] + c->r[2]);
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 1u));
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 25u));
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[3] = psk_nz(c, 0x00000000u);
    c->r[15] = psk_bx(c->r[14]);
    return;
a_02003A86:
    c->r[4] = psk_nz(c, psk_lsr_s(c, c->r[4], 8u));
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 3u));
    c->r[0] = psk_add_flags(c, c->r[0], c->r[2]);
    c->r[2] = psk_ldr(c->r[0] + 4u);
    c->r[3] = psk_ldr(c->r[0] + 8u);
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[0] = psk_nz(c, psk_asr_s(c, c->r[0], 24u));
    c->r[1] = psk_add_flags(c, c->r[3], 0x00000000u);
    c->r[1] = psk_nz(c, psk_asr_s(c, c->r[1], 24u));
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 1u));
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 1u));
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02003A9E
 | Description: Register-level C for the Thumb routine at 02003A9E; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003A9E(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        psk_w32(a - 8u, c->r[6]);
        c->r[13] = a - 8u;
    }
    psk_call(c, 0x02003A55u, 0x02003AA5u);
    c->r[5] = psk_r8(c->r[7] + 2u);
    psk_sub_flags(c, c->r[5], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003ABC;
    }
    c->r[5] = psk_r8(c->r[7] + 1u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    psk_w8(c->r[7] + 1u, (uint8_t)c->r[5]);
    psk_call(c, 0x02003A55u, 0x02003AB5u);
    psk_sub_flags(c, c->r[2], 0x00000080u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003B00;
    }
    c->r[5] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[5] = psk_nz(c, psk_lsr_s(c, c->r[5], 16u));
a_02003ABC:
    c->r[5] = psk_sub_flags(c, c->r[5], 0x00000001u);
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[5]);
    c->r[5] = psk_r8(c->r[7] + 3u);
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 24u));
    c->r[5] = psk_nz(c, psk_asr_s(c, c->r[5], 24u));
    c->r[0] = psk_add_flags(c, c->r[0], c->r[5]);
    c->r[8] = c->r[2];
    c->r[9] = c->r[3];
    c->r[2] = psk_r8(c->r[7]);
    c->r[2] = psk_sub_flags(c, c->r[2], c->r[4]);
    psk_call(c, 0x02003317u, 0x02003AD5u);
    c->r[6] = c->r[8];
    psk_sub_flags(c, c->r[6], 0x00000000u);
    if (psk_cond(c, PSK_PL)) {
        goto a_02003ADC;
    }
    c->r[5] = psk_add_flags(c, c->r[5], 0x00000004u);
a_02003ADC:
    c->r[6] = psk_nz(c, psk_lsr_s(c, c->r[6], 8u));
    c->r[7] = psk_nz(c, 0x000000FFu);
    c->r[6] = psk_nz(c, c->r[6] & c->r[7]);
    psk_sub_flags(c, c->r[6], c->r[7]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003B04;
    }
    c->r[7] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[7] = psk_nz(c, c->r[7] * c->r[3]);
    c->r[7] = psk_add_flags(c, c->r[7], 0x0000001Fu);
    c->r[7] = psk_nz(c, psk_lsr_s(c, c->r[7], 5u));
    c->r[7] = psk_nz(c, psk_lsl_s(c, c->r[7], 2u));
    c->r[6] = psk_nz(c, c->r[6] * c->r[7]);
    c->r[7] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[5] & c->r[7]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003AFA;
    }
    c->r[6] = psk_nz(c, psk_lsl_s(c, c->r[6], 1u));
a_02003AFA:
    c->r[4] = psk_add_flags(c, c->r[4], c->r[6]);
    {
        uint32_t a = c->r[13];
        c->r[6] = psk_r32(a);
        c->r[7] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[15] = psk_bx(c->r[7]);
    return;
a_02003B00:
    c->r[5] = psk_nz(c, 0x000000FFu);
    psk_w8(c->r[7], (uint8_t)c->r[5]);
a_02003B04:
    c->r[4] = psk_nz(c, 0x00000000u);
    {
        uint32_t a = c->r[13];
        c->r[6] = psk_r32(a);
        c->r[7] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[15] = psk_bx(c->r[7]);
    return;
}

/*----------------------
 | rrl_func_020039F4
 | Description: Register-level C for the Thumb routine at 020039F4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020039F4(PskCpu *c) {
    c->r[7] = psk_nz(c, 0x00000060u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_nz(c, 0x000000FFu);
    c->r[1] = psk_nz(c, 0x00000004u);
a_020039FC:
    psk_w8(c->r[7], (uint8_t)c->r[0]);
    c->r[7] = psk_add_flags(c, c->r[7], 0x00000004u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020039FC;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02003A06
 | Description: Register-level C for the Thumb routine at 02003A06; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003A06(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[7] = psk_nz(c, psk_lsl_s(c, c->r[7], 2u));
    c->r[7] = psk_add_flags(c, c->r[7], 0x00000060u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    psk_w8(c->r[7], (uint8_t)c->r[1]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 1u, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 3u, (uint8_t)c->r[0]);
    psk_call(c, 0x02003A55u, 0x02003A1Du);
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 16u));
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[2]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02003A24
 | Description: Register-level C for the Thumb routine at 02003A24; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003A24(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[7] = psk_nz(c, psk_lsl_s(c, c->r[7], 2u));
    c->r[7] = psk_add_flags(c, c->r[7], 0x00000060u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    psk_w8(c->r[7], (uint8_t)c->r[1]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 1u, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 3u, (uint8_t)c->r[2]);
    psk_call(c, 0x02003A55u, 0x02003A3Bu);
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 16u));
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[2]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02003A42
 | Description: Register-level C for the Thumb routine at 02003A42.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003A42(PskCpu *c) {
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 2u));
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000060u);
    c->r[0] = psk_r8(c->r[6] + c->r[0]);
    psk_sub_flags(c, c->r[0], 0x000000FFu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003A50;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[15] = psk_bx(c->r[14]);
    return;
a_02003A50:
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02003E30
 | Description: Register-level C for the ARM routine at 02003E30; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003E30(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008294u, 0x02003E38u);
    psk_call(c, 0x020082A0u, 0x02003E3Cu);
    c->r[3] = 0x00000000u;
    psk_call(c, 0x0200845Cu, 0x02003E44u);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02003B14
 | Description: Register-level C for the ARM routine at 02003B14; it calls 3 routines and SWI 13.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x5C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003B14(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_swi(0x13u, c->r[0], c->r[1], c->r[2]);
    c->r[1] = psk_ldr(c->r[0]);
    c->r[0] = 0x00000003u;
    c->r[0] = c->r[0] & ((c->r[1] >> 18u));
    c->r[1] = 0x000005C4u;
    psk_w8(c->r[1] + 0xFu, (uint8_t)c->r[0]);
    c->r[2] = 0x000000FFu;
    psk_w8(c->r[1] + 0xCu, (uint8_t)c->r[2]);
    psk_w8(c->r[1] + 0xDu, (uint8_t)c->r[2]);
    psk_w8(c->r[1] + 0xEu, (uint8_t)c->r[2]);
    psk_call(c, 0x02008450u, 0x02003B44u);
    c->r[0] = 0x00000002u;
    c->r[1] = 0x00000047u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02003E30u, 0x02003B54u);
    c->r[0] = 0x0000000Du;
    c->r[1] = 0x02003B64u;
    psk_call(c, 0x02002BC4u, 0x02003B60u);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020038DC
 | Description: Register-level C for the ARM routine at 020038DC.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020038DC(PskCpu *c) {
    c->r[0] = 0x02000300u;
    c->r[1] = 0x000004C4u;
    c->r[2] = c->r[1] + 0x00000080u;
a_020038E8:
    {
        uint32_t a = c->r[0];
        c->r[3] = psk_r32(a);
        c->r[4] = psk_r32(a + 4u);
        c->r[5] = psk_r32(a + 8u);
        c->r[6] = psk_r32(a + 0xCu);
        c->r[7] = psk_r32(a + 0x10u);
        c->r[8] = psk_r32(a + 0x14u);
        c->r[9] = psk_r32(a + 0x18u);
        c->r[10] = psk_r32(a + 0x1Cu);
        c->r[0] = a + 0x20u;
    }
    {
        uint32_t a = c->r[1];
        psk_w32(a, c->r[3]);
        psk_w32(a + 4u, c->r[4]);
        psk_w32(a + 8u, c->r[5]);
        psk_w32(a + 0xCu, c->r[6]);
        psk_w32(a + 0x10u, c->r[7]);
        psk_w32(a + 0x14u, c->r[8]);
        psk_w32(a + 0x18u, c->r[9]);
        psk_w32(a + 0x1Cu, c->r[10]);
        c->r[1] = a + 0x20u;
    }
    psk_sub_flags(c, c->r[1], c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020038E8;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_quit
 | Description: Quits the app back to the kernel. Unverified (quit path, needs the dock edge or YES in the quit dialog). Register-level C for the ARM routine at 0200382C; it calls 1 routine and SWI 08, 09, 11, 16.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_quit(PskCpu *c) {
    c->r[0] = 0x00000000u;
    c->r[0] = psk_swi(0x11u, c->r[0], c->r[1], c->r[2]);
    psk_call(c, 0x02003858u, 0x02003838u);
    c->r[0] = psk_swi(0x16u, c->r[0], c->r[1], c->r[2]);
    c->r[2] = c->r[0];
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00000000u;
    c->r[2] = c->r[2] | 0x00000030u;
    c->r[0] = psk_swi(0x08u, c->r[0], c->r[1], c->r[2]);
    c->r[0] = 0x00000000u;
    c->r[0] = psk_swi(0x09u, c->r[0], c->r[1], c->r[2]);
    c->r[15] = 0x02003858u;
    return;
}

/*----------------------
 | rrl_func_02003CA8
 | Description: Register-level C for the Thumb routine at 02003CA8; it calls 3 routines and SWI 04.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x204, WRAM 0x3C0, WRAM 0x5C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003CA8(PskCpu *c) {
    c->r[1] = 0x000005C4u;
    c->r[0] = psk_r8(c->r[1] + 0xEu);
    c->r[2] = psk_nz(c, 0x000000FFu);
    psk_w8(c->r[1] + 0xEu, (uint8_t)c->r[2]);
    psk_sub_flags(c, c->r[0], c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003CB6;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
a_02003CB6:
    c->r[5] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[1] = 0x00000204u;
    c->r[0] = psk_r8(c->r[1]);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x02003D85u;
        return;
    }
    c->r[1] = 0x000003C0u;
    c->r[2] = psk_ldr(c->r[1]);
    c->r[1] = psk_ldr(c->r[2]);
    c->r[2] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x02003D85u;
        return;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[1] = 0x000005C4u;
    c->r[3] = psk_r8(c->r[1] + 0xCu);
    psk_sub_flags(c, c->r[3], 0x000000FFu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003CDE;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[5]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02003D85u, 0x02003CDDu);
    {
        uint32_t a = c->r[13];
        c->r[5] = psk_r32(a);
        c->r[13] = a + 4u;
    }
a_02003CDE:
    c->r[0] = psk_nz(c, 0x00000006u);
    c->r[0] = psk_swi(0x04u, c->r[0], c->r[1], c->r[2]);
    c->r[0] = psk_nz(c, 0x00000002u);
    c->r[1] = psk_nz(c, 0x00000047u);
    c->r[2] = psk_nz(c, 0x00000000u);
    c->r[3] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x020042EDu, 0x02003CEFu);
    psk_call(c, 0x02003D43u, 0x02003CF3u);
    c->r[1] = 0x000005C4u;
    c->r[2] = 0x00000204u;
a_02003CF6:
    c->r[0] = psk_r8(c->r[2]);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003D0A;
    }
    c->r[0] = psk_r8(c->r[1] + 0xCu);
    psk_sub_flags(c, c->r[0], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        psk_idle();
        goto a_02003CF6;
    }
    c->r[0] = psk_nz(c, 0x00000005u);
    c->r[0] = psk_swi(0x04u, c->r[0], c->r[1], c->r[2]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
a_02003D0A:
    psk_call(c, 0x02003D85u, 0x02003D0Fu);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02003924
 | Description: Unverified (quit dialog, needs a 96-pass action hold). Register-level C for the Thumb routine at 02003924; SWI 04.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003924(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        psk_w32(a - 8u, c->r[7]);
        psk_w32(a - 0xCu, c->r[6]);
        psk_w32(a - 0x10u, c->r[5]);
        psk_w32(a - 0x14u, c->r[4]);
        psk_w32(a - 0x18u, c->r[3]);
        psk_w32(a - 0x1Cu, c->r[2]);
        psk_w32(a - 0x20u, c->r[1]);
        c->r[13] = a - 0x20u;
    }
    c->r[2] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[3] = 0x0B000000u;
    c->r[5] = psk_ldr(c->r[3]);
    c->r[1] = psk_nz(c, 0x0000000Fu);
    c->r[5] = psk_nz(c, c->r[5] & c->r[1]);
    c->r[0] = psk_nz(c, 0x00000003u);
    c->r[0] = psk_swi(0x04u, c->r[0], c->r[1], c->r[2]);
    c->r[0] = psk_add_flags(c, c->r[3], 0x00000000u);
    c->r[7] = 0x0D800000u;
    c->r[6] = psk_nz(c, 0x00000000u);
a_0200393A:
    c->r[6] = psk_add_flags(c, c->r[6], 0x00000001u);
    c->r[4] = psk_nz(c, 0x00000002u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 8u));
    c->r[1] = psk_nz(c, 0x00000001u);
    c->r[3] = psk_add_flags(c, c->r[6], 0x00000000u);
    c->r[3] = psk_nz(c, c->r[3] & c->r[1]);
    c->r[4] = psk_sub_flags(c, c->r[4], c->r[3]);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 22u));
    c->r[3] = psk_nz(c, 0x00000010u);
    c->r[1] = psk_add_flags(c, c->r[2], c->r[3]);
    c->r[4] = psk_nz(c, psk_asr_s(c, c->r[4], c->r[1]));
    psk_w32(c->r[7] + 0x14u, c->r[4]);
    c->r[3] = psk_nz(c, 0x00000000u);
a_02003954:
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000001u);
    c->r[1] = psk_nz(c, 0x0000000Eu);
    psk_sub_flags(c, c->r[3], c->r[1]);
    if (psk_cond(c, PSK_LT)) {
        goto a_02003954;
    }
    c->r[1] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 6u));
    psk_sub_flags(c, c->r[6], c->r[1]);
    if (psk_cond(c, PSK_LT)) {
        goto a_0200393A;
    }
    c->r[3] = psk_nz(c, 0x00000000u);
    psk_sub_flags(c, c->r[5], c->r[3]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003970;
    }
    c->r[0] = psk_add_flags(c, c->r[5], 0x00000000u);
    c->r[0] = psk_swi(0x04u, c->r[0], c->r[1], c->r[2]);
    goto a_02003972;
a_02003970:
    psk_w32(c->r[0], c->r[3]);
a_02003972:
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[2] = psk_r32(a + 4u);
        c->r[3] = psk_r32(a + 8u);
        c->r[4] = psk_r32(a + 0xCu);
        c->r[5] = psk_r32(a + 0x10u);
        c->r[6] = psk_r32(a + 0x14u);
        c->r[7] = psk_r32(a + 0x18u);
        c->r[15] = (psk_r32(a + 0x1Cu) & ~1u) | 1u;
        c->r[13] = a + 0x20u;
        return;
    }
}

/*----------------------
 | rrl_func_02003C5C
 | Description: Unverified (quit dialog, needs a 96-pass action hold). Register-level C for the Thumb routine at 02003C5C; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x204, WRAM 0x3C0, WRAM 0x5C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003C5C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[1] = 0x00000204u;
    c->r[0] = psk_r8(c->r[1]);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003C9E;
    }
    c->r[1] = 0x000003C0u;
    c->r[2] = psk_ldr(c->r[1]);
    c->r[1] = psk_ldr(c->r[2]);
    c->r[2] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003C9E;
    }
    c->r[1] = 0x000005C4u;
    c->r[2] = psk_r8(c->r[1] + 0xFu);
    psk_sub_flags(c, c->r[2], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003C9E;
    }
    c->r[1] = psk_nz(c, 0x000000A0u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 20u));
    c->r[0] = psk_ldr(c->r[1] + 8u);
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = 0x000035FFu;
    psk_w32(c->r[1] + 0xCu, c->r[0]);
    psk_call(c, 0x02003DA7u, 0x02003C8Bu);
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003C92;
    }
    c->r[0] = psk_nz(c, 0x00000002u);
a_02003C92:
    psk_call(c, 0x02003925u, 0x02003C97u);
    psk_call(c, 0x02003DB5u, 0x02003C9Bu);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    psk_w32(c->r[1] + 8u, c->r[0]);
a_02003C9E:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020032B0
 | Description: Unverified (quit dialog, needs a 96-pass action hold). Register-level C for the ARM routine at 020032B0.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x240
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020032B0(PskCpu *c) {
    c->r[6] = 0x00000240u;
    c->r[6] = c->r[6] + (c->r[1] << 2u);
a_020032B8:
    {
        uint32_t a = c->r[4];
        uint32_t b = a + 4u;
        c->r[7] = psk_ldr(a);
        c->r[4] = b;
    }
    psk_sub_flags(c, c->r[5], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        c->r[7] = ~c->r[7];
    }
    {
        uint32_t a = c->r[6];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[7]);
        c->r[6] = b;
    }
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020032B8;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_quit_dialog
 | Description: Two-choice quit dialog: left and right toggle, action confirms, YES quits the app. Unverified (quit dialog, needs a 96-pass action hold). Register-level C for the ARM routine at 02003760; it calls 8 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C0
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_quit_dialog(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020082B8u, 0x02003768u);
    psk_call(c, 0x02008444u, 0x0200376Cu);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00000000u;
    psk_call(c, 0x02003E48u, 0x02003778u);
    c->r[0] = 0x00000000u;
    psk_call(c, 0x02002E64u, 0x02003780u);
    c->r[8] = 0x00000000u;
a_02003784:
    c->r[1] = 0x000003C0u;
    c->r[0] = psk_ldr(c->r[1]);
    c->r[1] = psk_ldr(c->r[0]);
    psk_set_carry(c, 0u);
    psk_nz(c, c->r[1] & 0x00000100u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x0200382Cu;
        return;
    }
    psk_call(c, 0x020082ACu, 0x0200379Cu);
    psk_call(c, 0x02000440u, 0x020037A0u);
    c->r[1] = 0x00000008u;
    c->r[3] = 0x00000007u;
    c->r[4] = 0x020064B0u;
    psk_sub_flags(c, c->r[8], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[5] = 0x00000001u;
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[5] = 0x00000000u;
    }
    psk_call(c, 0x020032B0u, 0x020037BCu);
    c->r[1] = 0x00000011u;
    c->r[3] = 0x00000007u;
    c->r[4] = 0x020064CCu;
    psk_sub_flags(c, c->r[8], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[5] = 0x00000000u;
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[5] = 0x00000001u;
    }
    psk_call(c, 0x020032B0u, 0x020037D8u);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00000001u;
    psk_call(c, 0x02003E48u, 0x020037E4u);
    c->r[11] = c->r[8];
    psk_call(c, 0x020032D4u, 0x020037ECu);
    c->r[8] = c->r[11];
    c->r[0] = 0x00000000u;
    psk_call(c, 0x02002E64u, 0x020037F8u);
    psk_nz(c, c->r[1] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003814;
    }
    psk_nz(c, c->r[1] & 0x00000018u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003784;
    }
    c->r[8] = c->r[8] ^ 0x00000001u;
    psk_call(c, 0x02008444u, 0x02003810u);
    goto a_02003784;
a_02003814:
    psk_call(c, 0x02008444u, 0x02003818u);
    {
        uint32_t a = c->r[13];
        c->r[14] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    psk_sub_flags(c, c->r[8], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x0200382Cu;
        return;
    }
    c->r[0] = 0x00000000u;
    c->r[15] = 0x02002E64u;
    return;
}

/*----------------------
 | rrl_main_loop
 | Description: Main loop: runs the current screen's handler from the state table, counts inactivity toward standby, and opens the quit dialog. Unverified (never returns, so only Task 8's all-C run checks it). Register-level C for the ARM routine at 02003348; it calls 17 routines and SWI 06.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C0, WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_main_loop(PskCpu *c) {
    psk_call(c, 0x02000400u, 0x0200334Cu);
    psk_call(c, 0x020082ACu, 0x02003350u);
    psk_call(c, 0x020032D4u, 0x02003354u);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00003D09u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02003DD8u, 0x02003364u);
    c->r[0] = 0x00000000u;
    psk_call(c, 0x02008294u, 0x0200336Cu);
    psk_call(c, 0x02003B14u, 0x02003370u);
    c->r[0] = psk_swi(0x06u, c->r[0], c->r[1], c->r[2]);
    c->r[1] = 0x000003C0u;
    psk_w32(c->r[1], c->r[0]);
    psk_call(c, 0x020038DCu, 0x02003380u);
    c->r[6] = 0x000003C4u;
    c->r[0] = 0x00000000u;
    psk_w16(c->r[6], (uint16_t)c->r[0]);
    c->r[0] = 0x00000010u;
    psk_w8(c->r[6] + 0x1Fu, (uint8_t)c->r[0]);
    psk_call(c, 0x02008408u, 0x02003398u);
a_02003398:
    c->r[6] = 0x000003C4u;
    c->r[0] = 0x00000F00u;
    psk_w16(c->r[6] + 8u, (uint16_t)c->r[0]);
    c->r[0] = 0x00000060u;
    psk_call(c, 0x02002E00u, 0x020033ACu);
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000002u;
    psk_call(c, 0x02002FC0u, 0x020033B8u);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00000000u;
    psk_call(c, 0x02003E48u, 0x020033C4u);
    c->r[0] = 0x00000000u;
    psk_call(c, 0x02002E64u, 0x020033CCu);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00000001u;
    psk_call(c, 0x02003E48u, 0x020033D8u);
    c->r[0] = 0x00000000u;
    psk_call(c, 0x02002E64u, 0x020033E0u);
a_020033E0:
    c->r[1] = 0x000003C0u;
    c->r[0] = psk_ldr(c->r[1]);
    c->r[1] = psk_ldr(c->r[0]);
    psk_set_carry(c, 0u);
    psk_nz(c, c->r[1] & 0x00000100u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x0200382Cu;
        return;
    }
    psk_call(c, 0x020082ACu, 0x020033F8u);
    psk_call(c, 0x02000440u, 0x020033FCu);
    psk_call(c, 0x02008414u, 0x02003400u);
    c->r[6] = 0x000003C4u;
    c->r[0] = psk_r8(c->r[6] + 0x1Fu);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[0] = 0x00000010u;
    }
    psk_w8(c->r[6] + 0x1Fu, (uint8_t)c->r[0]);
    c->r[1] = psk_r8(c->r[6] + 0x10u);
    c->r[3] = 0x02008474u;
    c->r[0] = psk_ldr(c->r[3] + ((c->r[1] << 2u)));
    c->r[14] = 0x02003428u;
    psk_call(c, psk_bx(c->r[0]), 0x02003428u);
    c->r[0] = 0x00000100u;
    psk_call(c, 0x02008420u, 0x02003430u);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00000001u;
    psk_call(c, 0x02003E48u, 0x0200343Cu);
    psk_call(c, 0x020032D4u, 0x02003440u);
    c->r[0] = 0x00000001u;
    psk_call(c, 0x02002E64u, 0x02003448u);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x02003760u, 0x02003450u);
    }
    c->r[6] = 0x000003C4u;
    c->r[0] = psk_r8(c->r[6] + 2u);
    psk_nz(c, c->r[0] & 0x0000001Fu);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200347C;
    }
    c->r[0] = psk_ldrh(c->r[6] + 8u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003480;
    }
    psk_call(c, 0x020082ACu, 0x02003470u);
    psk_call(c, 0x020032D4u, 0x02003474u);
    psk_call(c, 0x02000518u, 0x02003478u);
    goto a_02003398;
a_0200347C:
    c->r[0] = 0x00000F00u;
a_02003480:
    psk_w16(c->r[6] + 8u, (uint16_t)c->r[0]);
    c->r[0] = psk_r8(c->r[6] + 0x10u);
    psk_sub_flags(c, c->r[0], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        goto a_020033E0;
    }
    c->r[0] = 0x00000000u;
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    psk_call(c, 0x02008408u, 0x0200349Cu);
    psk_call(c, 0x02003760u, 0x020034A0u);
    goto a_020033E0;
}

/*----------------------
 | rrl_func_0200397C
 | Description: Register-level C for the Thumb routine at 0200397C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200397C(PskCpu *c) {
    c->r[5] = psk_nz(c, 0x00000070u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x000000FFu);
    c->r[1] = psk_nz(c, 0x0000000Au);
a_02003984:
    psk_w8(c->r[5], (uint8_t)c->r[0]);
    c->r[5] = psk_add_flags(c, c->r[5], 0x00000004u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003984;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_0200398E
 | Description: Register-level C for the Thumb routine at 0200398E; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200398E(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[7] = psk_nz(c, psk_lsl_s(c, c->r[7], 2u));
    c->r[7] = psk_add_flags(c, c->r[7], 0x00000070u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    psk_w8(c->r[7], (uint8_t)c->r[1]);
    psk_w8(c->r[7] + 1u, (uint8_t)c->r[2]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[0]);
    c->r[2] = 0x0200A7B0u;
    c->r[0] = psk_nz(c, 0x000000FEu);
    c->r[0] = psk_nz(c, c->r[0] & c->r[1]);
    c->r[0] = psk_ldrh(c->r[2] + c->r[0]);
    c->r[1] = psk_nz(c, psk_lsr_s(c, c->r[1], 1u));
    if (psk_cond(c, PSK_LO)) {
        goto a_020039AE;
    }
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 8u));
a_020039AE:
    c->r[1] = psk_nz(c, 0x000000FFu);
    c->r[0] = psk_nz(c, c->r[0] & c->r[1]);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x020039B9u);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 3u, (uint8_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02003EA4
 | Description: Register-level C for the Thumb routine at 02003EA4; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4, WRAM 0x3E4, WRAM 0x4F4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003EA4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[5] = psk_nz(c, 0x000000C0u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[6] = psk_add_flags(c, c->r[6], 0x000000E0u);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
a_02003EB6:
    psk_w32(c->r[5] + c->r[0], c->r[1]);
    psk_w32(c->r[6] + c->r[0], c->r[1]);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000004u);
    psk_sub_flags(c, c->r[0], 0x00000020u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003EB6;
    }
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[0] = psk_r8(c->r[7] + 4u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003ECC;
    }
    psk_w8(c->r[5], (uint8_t)c->r[1]);
    goto a_02003ECE;
a_02003ECC:
    psk_w8(c->r[5] + 0x10u, (uint8_t)c->r[1]);
a_02003ECE:
    c->r[7] = 0x000004F4u;
    c->r[3] = psk_r8(c->r[7] + 0xEu);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003F10;
    }
    c->r[4] = psk_ldr(c->r[7] + 0x10u);
    c->r[2] = psk_nz(c, 0x00000006u);
    c->r[1] = psk_nz(c, 0x00000000u);
a_02003EDC:
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    c->r[4] = psk_nz(c, psk_lsr_s(c, c->r[4], 1u));
    if (psk_cond(c, PSK_LO)) {
        goto a_02003EDC;
    }
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x02003EEBu);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[4]);
        psk_w32(a - 8u, c->r[3]);
        c->r[13] = a - 8u;
    }
    c->r[3] = psk_nz(c, 0x00000000u);
a_02003EF0:
    c->r[4] = psk_r8(c->r[5] + c->r[3]);
    psk_sub_flags(c, c->r[4], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003EFC;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003F06;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
a_02003EFC:
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000004u);
    psk_sub_flags(c, c->r[3], 0x00000018u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003EF0;
    }
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000004u);
    goto a_02003EF0;
a_02003F06:
    psk_w8(c->r[5] + c->r[3], (uint8_t)c->r[1]);
    {
        uint32_t a = c->r[13];
        c->r[3] = psk_r32(a);
        c->r[4] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003EDC;
    }
a_02003F10:
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[11] = c->r[0];
    c->r[3] = psk_r8(c->r[7] + 0xFu);
    c->r[1] = psk_r8(c->r[7] + 0xEu);
    c->r[10] = c->r[1];
    c->r[3] = psk_sub_flags(c, c->r[3], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003FFE;
    }
    c->r[4] = psk_ldr(c->r[7] + 0x10u);
    c->r[4] = psk_nz(c, psk_lsr_s(c, c->r[4], 6u));
    c->r[0] = psk_nz(c, 0x000000FFu);
    c->r[8] = c->r[0];
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[9] = c->r[0];
    c->r[7] = psk_add_flags(c, c->r[7], 0x0000000Du);
    c->r[2] = psk_nz(c, 0x0000000Eu);
    c->r[2] = psk_sub_flags(c, c->r[2], c->r[1]);
    c->r[1] = psk_nz(c, 0x00000006u);
a_02003F32:
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    c->r[4] = psk_nz(c, psk_lsr_s(c, c->r[4], 1u));
    if (psk_cond(c, PSK_LO)) {
        goto a_02003F32;
    }
    c->r[0] = psk_add_flags(c, c->r[3], 0x00000000u);
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[2]);
    if (psk_cond(c, PSK_LS)) {
        goto a_02003F68;
    }
    psk_sub_flags(c, c->r[0], 0x00000007u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02003F5E;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02003F52;
    }
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x02003F4Du);
    psk_sub_flags(c, c->r[0], 0x00000046u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02003FFA;
    }
    goto a_02003F68;
a_02003F52:
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x02003F59u);
    psk_sub_flags(c, c->r[0], 0x00000032u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02003FFA;
    }
    goto a_02003F68;
a_02003F5E:
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x02003F65u);
    psk_sub_flags(c, c->r[0], 0x0000001Eu);
    if (psk_cond(c, PSK_HS)) {
        goto a_02003FFA;
    }
a_02003F68:
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x02003F71u);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[4]);
        psk_w32(a - 8u, c->r[3]);
        psk_w32(a - 0xCu, c->r[2]);
        psk_w32(a - 0x10u, c->r[1]);
        c->r[13] = a - 0x10u;
    }
    c->r[3] = psk_nz(c, 0x00000000u);
a_02003F76:
    c->r[4] = psk_r8(c->r[5] + c->r[3]);
    psk_sub_flags(c, c->r[4], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003F82;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003F8C;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
a_02003F82:
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000002u);
    psk_sub_flags(c, c->r[3], 0x00000018u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003F76;
    }
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000002u);
    goto a_02003F76;
a_02003F8C:
    psk_w8(c->r[5] + c->r[3], (uint8_t)c->r[1]);
    c->r[10] = c->r[11] + c->r[10];
    c->r[0] = psk_r8(c->r[7] + c->r[1]);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003FF4;
    }
    c->r[4] = c->r[9];
    psk_sub_flags(c, c->r[4], 0x00000005u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003FF4;
    }
    c->r[4] = c->r[8];
    c->r[0] = psk_nz(c, c->r[0] & c->r[4]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003FF4;
    }
    c->r[2] = psk_nz(c, 0x00000008u);
    c->r[4] = psk_nz(c, 0x00000000u);
a_02003FA6:
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 1u));
    if (psk_cond(c, PSK_LO)) {
        goto a_02003FAC;
    }
    c->r[4] = psk_add_flags(c, c->r[4], 0x00000001u);
a_02003FAC:
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003FA6;
    }
    c->r[0] = psk_add_flags(c, c->r[4], 0x00000000u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x0000001Eu);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x02003FBBu);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    psk_sub_flags(c, c->r[0], c->r[4]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02003FF4;
    }
    c->r[2] = psk_r8(c->r[7] + c->r[1]);
    c->r[4] = c->r[8];
    c->r[2] = psk_nz(c, c->r[2] & c->r[4]);
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[4] = psk_nz(c, 0x00000001u);
a_02003FCA:
    psk_nz(c, c->r[2] & c->r[4]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003FD4;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003FDA;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
a_02003FD4:
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 1u));
    goto a_02003FCA;
a_02003FDA:
    c->r[0] = c->r[9];
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000005u);
    psk_w8(c->r[6] + c->r[3], (uint8_t)c->r[0]);
    c->r[2] = c->r[8];
    c->r[2] = psk_nz(c, c->r[2] & ~c->r[4]);
    c->r[8] = c->r[2];
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[7]);
        psk_w32(a - 8u, c->r[6]);
        c->r[13] = a - 8u;
    }
    c->r[2] = psk_nz(c, 0x00000000u);
    c->r[6] = 0x000003C4u;
    psk_call(c, 0x0200398Fu, 0x02003FF1u);
    {
        uint32_t a = c->r[13];
        c->r[6] = psk_r32(a);
        c->r[7] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[9] = c->r[11] + c->r[9];
a_02003FF4:
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[2] = psk_r32(a + 4u);
        c->r[3] = psk_r32(a + 8u);
        c->r[4] = psk_r32(a + 0xCu);
        c->r[13] = a + 0x10u;
    }
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003FFE;
    }
a_02003FFA:
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003F32;
    }
a_02003FFE:
    c->r[7] = 0x000003E4u;
    c->r[7] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[7], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004012;
    }
    psk_sub_flags(c, c->r[7], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200400E;
    }
    c->r[7] = psk_nz(c, 0x00000023u);
    goto a_02004014;
a_0200400E:
    c->r[7] = psk_nz(c, 0x0000001Eu);
    goto a_02004014;
a_02004012:
    c->r[7] = psk_nz(c, 0x00000014u);
a_02004014:
    c->r[0] = c->r[10];
    c->r[2] = psk_nz(c, 0x0000001Eu);
    c->r[2] = psk_sub_flags(c, c->r[2], c->r[0]);
    c->r[1] = psk_nz(c, 0x00000002u);
a_0200401C:
    c->r[0] = psk_nz(c, 0x00000064u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x02004025u);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    psk_sub_flags(c, c->r[1], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004030;
    }
    psk_sub_flags(c, c->r[0], c->r[7]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004070;
    }
    goto a_02004034;
a_02004030:
    psk_sub_flags(c, c->r[0], 0x00000050u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004070;
    }
a_02004034:
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x0200403Du);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[3] = psk_nz(c, 0x00000000u);
a_02004040:
    c->r[4] = psk_r8(c->r[5] + c->r[3]);
    psk_sub_flags(c, c->r[4], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004052;
    }
    c->r[4] = psk_r8(c->r[6] + c->r[3]);
    psk_sub_flags(c, c->r[4], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004052;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200405C;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
a_02004052:
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000001u);
    psk_sub_flags(c, c->r[3], 0x00000018u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004040;
    }
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000001u);
    goto a_02004040;
a_0200405C:
    psk_w8(c->r[6] + c->r[3], (uint8_t)c->r[1]);
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[7]);
        psk_w32(a - 8u, c->r[6]);
        psk_w32(a - 0xCu, c->r[2]);
        psk_w32(a - 0x10u, c->r[1]);
        c->r[13] = a - 0x10u;
    }
    c->r[0] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000016u);
    c->r[2] = psk_nz(c, 0x00000000u);
    c->r[6] = 0x000003C4u;
    psk_call(c, 0x0200398Fu, 0x0200406Du);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[2] = psk_r32(a + 4u);
        c->r[6] = psk_r32(a + 8u);
        c->r[7] = psk_r32(a + 0xCu);
        c->r[13] = a + 0x10u;
    }
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000001u);
a_02004070:
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    psk_sub_flags(c, c->r[1], 0x00000005u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200401C;
    }
    goto a_020040B4;
a_020040B4:
    c->r[1] = psk_r8(c->r[5] + 8u);
    psk_w8(c->r[5] + 0x18u, (uint8_t)c->r[1]);
    c->r[1] = psk_r8(c->r[6] + 8u);
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
 | rrl_func_0200375C
 | Description: Register-level C for the ARM routine at 0200375C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200375C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_0200374C
 | Description: Register-level C for the ARM routine at 0200374C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200374C(PskCpu *c) {
    c->r[3] = 0x020084B4u;
    c->r[0] = psk_ldr(c->r[3] + ((c->r[0] << 2u)));
    c->r[14] = 0x0200375Cu;
    psk_call(c, psk_bx(c->r[0]), 0x0200375Cu);
    c->r[15] = 0x0200375Cu;
    return;
}

/*----------------------
 | rrl_func_0200373C
 | Description: Register-level C for the ARM routine at 0200373C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200373C(PskCpu *c) {
    c->r[6] = 0x000003C4u;
    c->r[0] = psk_r8(c->r[6] + 0x10u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        c->r[15] = 0x0200375Cu;
        return;
    }
    c->r[15] = 0x0200374Cu;
    return;
}

/*----------------------
 | rrl_func_020034A4
 | Description: Register-level C for the ARM routine at 020034A4; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020034A4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02000614u, 0x020034ACu);
    c->r[1] = 0x00000000u;
    c->r[15] = 0x0200373Cu;
    return;
}

/*----------------------
 | rrl_func_020034B4
 | Description: Register-level C for the ARM routine at 020034B4; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020034B4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020006BCu, 0x020034BCu);
    c->r[1] = 0x00000001u;
    c->r[15] = 0x0200373Cu;
    return;
}

/*----------------------
 | rrl_func_020036C0
 | Description: Register-level C for the ARM routine at 020036C0.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020036C0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020036C4
 | Description: Register-level C for the ARM routine at 020036C4; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020036C4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[6] = 0x000003C4u;
    c->r[0] = psk_r8(c->r[6] + 0x10u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003700;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003724;
    }
    psk_sub_flags(c, c->r[0], 0x00000008u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200371C;
    }
    psk_sub_flags(c, c->r[0], 0x0000000Cu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003708;
    }
    psk_sub_flags(c, c->r[0], 0x00000007u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02003734;
    }
    c->r[0] = 0x00000003u;
    goto a_02003738;
a_02003700:
    c->r[0] = 0x0000000Du;
    goto a_02003738;
a_02003708:
    psk_call(c, 0x020038DCu, 0x0200370Cu);
    c->r[6] = 0x000003C4u;
    c->r[0] = 0x000000FFu;
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
a_0200371C:
    c->r[0] = 0x00000000u;
    goto a_02003738;
a_02003724:
    c->r[1] = psk_r8(c->r[6] + 0x25u);
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003734;
    }
    c->r[0] = c->r[0] + 0x00000002u;
a_02003734:
    c->r[0] = c->r[0] + 0x00000001u;
a_02003738:
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    c->r[15] = 0x0200373Cu;
    return;
}

/*----------------------
 | rrl_func_020036BC
 | Description: Register-level C for the ARM routine at 020036BC; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020036BC(PskCpu *c) {
    psk_call(c, 0x020036C4u, 0x020036C0u);
    c->r[15] = 0x020036C0u;
    return;
}

/*----------------------
 | rrl_func_020036B4
 | Description: Register-level C for the ARM routine at 020036B4; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020036B4(PskCpu *c) {
    c->r[0] = 0x00000003u;
    psk_call(c, 0x02008438u, 0x020036BCu);
    c->r[15] = 0x020036BCu;
    return;
}

/*----------------------
 | rrl_func_020036A4
 | Description: Register-level C for the ARM routine at 020036A4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020036A4(PskCpu *c) {
    c->r[6] = 0x000003C4u;
    c->r[1] = psk_r8(c->r[6] + 3u);
    psk_nz(c, c->r[1] & 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[15] = 0x020036C0u;
        return;
    }
    c->r[15] = 0x020036B4u;
    return;
}

/*----------------------
 | rrl_func_020034C4
 | Description: Register-level C for the ARM routine at 020034C4; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020034C4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02000714u, 0x020034CCu);
    c->r[15] = 0x020036A4u;
    return;
}

/*----------------------
 | rrl_func_020034D0
 | Description: Register-level C for the ARM routine at 020034D0; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020034D0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02000760u, 0x020034D8u);
    c->r[6] = 0x000003C4u;
    c->r[0] = psk_r8(c->r[6] + 0x10u);
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x0200374Cu;
        return;
    }
    c->r[1] = psk_r8(c->r[6] + 3u);
    psk_nz(c, c->r[1] & 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003548;
    }
    c->r[0] = psk_r8(c->r[6] + 0x31u);
    c->r[1] = psk_r8(c->r[6] + 0x32u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003548;
    }
    c->r[0] = psk_r8(c->r[6] + 0x28u);
    c->r[1] = psk_r8(c->r[6] + 0x26u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003530;
    }
    c->r[1] = psk_r8(c->r[6] + 0x2Bu);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003530;
    }
    c->r[0] = psk_r8(c->r[6] + 0x25u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[15] = 0x020036BCu;
        return;
    }
    c->r[15] = 0x020036B4u;
    return;
a_02003530:
    psk_w8(c->r[6] + 0x26u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[6] + 0x29u);
    psk_w8(c->r[6] + 0x27u, (uint8_t)c->r[0]);
    c->r[0] = 0x00000000u;
    psk_w8(c->r[6] + 0x2Bu, (uint8_t)c->r[0]);
    psk_w8(c->r[6] + 0x2Cu, (uint8_t)c->r[0]);
a_02003548:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_0200354C
 | Description: Register-level C for the ARM routine at 0200354C; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200354C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02000778u, 0x02003554u);
    c->r[15] = 0x020036A4u;
    return;
}

/*----------------------
 | rrl_func_02003558
 | Description: Register-level C for the ARM routine at 02003558; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003558(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020007D0u, 0x02003560u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[15] = 0x020036BCu;
        return;
    }
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_0200356C
 | Description: Register-level C for the ARM routine at 0200356C; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200356C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x0200081Cu, 0x02003574u);
    c->r[6] = 0x000003C4u;
    c->r[0] = psk_r8(c->r[6] + 0x40u);
    psk_sub_flags(c, c->r[0], 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200358C;
    }
    c->r[0] = psk_r8(c->r[6] + 0x2Bu);
    if (psk_cond(c, PSK_EQ)) {
        c->r[15] = 0x020036BCu;
        return;
    }
a_0200358C:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02003590
 | Description: Register-level C for the ARM routine at 02003590; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003590(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x0200083Cu, 0x02003598u);
    c->r[6] = 0x000003C4u;
    c->r[0] = psk_r8(c->r[6] + 0x35u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020035BC;
    }
    c->r[0] = psk_r8(c->r[6] + 0x2Bu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020035BC;
    }
    psk_call(c, 0x0200842Cu, 0x020035B8u);
    c->r[15] = 0x0200374Cu;
    return;
a_020035BC:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020035C0
 | Description: Register-level C for the ARM routine at 020035C0; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020035C0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02000E2Cu, 0x020035C8u);
    c->r[6] = 0x000003C4u;
    c->r[0] = psk_r8(c->r[6] + 0x10u);
    psk_sub_flags(c, c->r[0], 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x0200374Cu;
        return;
    }
    c->r[1] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x020036A4u;
        return;
    }
    c->r[1] = psk_r8(c->r[6] + 0x2Cu);
    psk_sub_flags(c, c->r[1], 0x00000040u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[15] = 0x020036A4u;
        return;
    }
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020035F4
 | Description: Register-level C for the ARM routine at 020035F4; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020035F4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02000854u, 0x020035FCu);
    c->r[1] = 0x00000009u;
    c->r[15] = 0x0200373Cu;
    return;
}

/*----------------------
 | rrl_func_02003604
 | Description: Register-level C for the ARM routine at 02003604; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003604(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x0200094Cu, 0x0200360Cu);
    c->r[1] = 0x0000000Au;
    c->r[15] = 0x0200373Cu;
    return;
}

/*----------------------
 | rrl_func_02003614
 | Description: Register-level C for the ARM routine at 02003614; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003614(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020009A0u, 0x0200361Cu);
    c->r[1] = 0x0000000Bu;
    c->r[15] = 0x0200373Cu;
    return;
}

/*----------------------
 | rrl_func_02003630
 | Description: Register-level C for the ARM routine at 02003630; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02003630(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02000BD8u, 0x02003638u);
    c->r[15] = 0x020036A4u;
    return;
}

/*----------------------
 | rrl_func_0200363C
 | Description: Register-level C for the ARM routine at 0200363C; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200363C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02000C1Cu, 0x02003644u);
    c->r[1] = 0x0000000Eu;
    c->r[15] = 0x0200373Cu;
    return;
}

/*----------------------
 | rrl_sound_fiq
 | Description: Timer2 FIQ callback: the sound driver, advancing playback state at 0x5C4. Register-level C for the ARM routine at 02003B64.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x5C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_sound_fiq(PskCpu *c) {
    c->r[8] = 0x000005C4u;
    c->r[10] = psk_ldr(c->r[8] + 4u);
    c->r[11] = psk_ldr(c->r[8] + 8u);
    psk_sub_flags(c, c->r[11], c->r[10]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003BF4;
    }
    c->r[9] = 0x000000FFu;
    c->r[10] = psk_r8(c->r[8] + 0xDu);
    psk_sub_flags(c, c->r[10], c->r[9]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02003BB4;
    }
    psk_w8(c->r[8] + 0xCu, (uint8_t)c->r[10]);
    psk_w8(c->r[8] + 0xDu, (uint8_t)c->r[9]);
    c->r[11] = 0x0200A7C8u;
    {
        uint32_t b = c->r[11] + ((c->r[10] << 2u));
        c->r[9] = psk_ldr(b);
        c->r[11] = b;
    }
    c->r[10] = psk_ldr(c->r[11] + 4u);
    psk_w32(c->r[8], c->r[9]);
    c->r[10] = c->r[10] - c->r[9];
    psk_w32(c->r[8] + 8u, c->r[10]);
    c->r[10] = 0x00000000u;
    psk_w32(c->r[8] + 4u, c->r[10]);
    goto a_02003BF8;
a_02003BB4:
    psk_w8(c->r[8] + 0xCu, (uint8_t)c->r[9]);
    psk_w8(c->r[8] + 0xDu, (uint8_t)c->r[9]);
    c->r[8] = 0x0A800008u;
    c->r[9] = psk_ldr(c->r[8] + 0x20u);
    c->r[9] = c->r[9] & ~0x00000004u;
    psk_w32(c->r[8] + 0x20u, c->r[9]);
    c->r[8] = 0x0A000000u;
    c->r[9] = 0x00002000u;
    psk_w32(c->r[8] + 0xCu, c->r[9]);
    psk_w32(c->r[8] + 0x10u, c->r[9]);
    c->r[8] = 0x0D800000u;
    c->r[9] = 0x00000020u;
    psk_w32(c->r[8] + 4u, c->r[9]);
    c->r[9] = 0x00000000u;
    psk_w32(c->r[8] + 0x10u, c->r[9]);
    c->r[15] = c->r[14] & ~3u;
    return;
a_02003BF4:
    c->r[9] = psk_ldr(c->r[8]);
a_02003BF8:
    c->r[11] = c->r[10] & ~0x00000001u;
    c->r[11] = psk_ldrh(c->r[9] + c->r[11]);
    c->r[9] = psk_r8(c->r[8] + 0xFu);
    psk_sub_flags(c, c->r[9], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02003C24;
    }
    psk_nz(c, c->r[10] & 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[11] = (c->r[11] << 24u);
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[11] = (c->r[11] << 16u);
    }
    c->r[9] = 0x0000FFC0u;
    c->r[11] = c->r[9] & (psk_asr(c->r[11], 18u));
    goto a_02003C30;
a_02003C24:
    psk_nz(c, c->r[10] & 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[11] = (c->r[11] << 8u);
    }
    c->r[11] = c->r[11] & 0x0000FF00u;
a_02003C30:
    c->r[10] = c->r[10] + 0x00000001u;
    psk_w32(c->r[8] + 4u, c->r[10]);
    c->r[9] = 0x0D800014u;
    psk_w32(c->r[9], c->r[11]);
    c->r[15] = c->r[14] & ~3u;
    return;
}

/*----------------------
 | rrl_timer_callback
 | Description: Timer interrupt callback that advances the tick counters at 0x5D4. Register-level C for the ARM routine at 02003E70.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x5D4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_timer_callback(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[4]);
        psk_w32(a - 8u, c->r[3]);
        c->r[13] = a - 8u;
    }
    c->r[3] = 0x000005D4u;
    psk_sub_flags(c, c->r[0], 0x00000100u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[3] = c->r[3] + 0x00000002u;
    }
    psk_sub_flags(c, c->r[0], 0x00002000u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[3] = c->r[3] + 0x00000004u;
    }
    c->r[4] = psk_r8(c->r[3]);
    c->r[4] = c->r[4] + 0x00000001u;
    psk_w8(c->r[3], (uint8_t)c->r[4]);
    {
        uint32_t a = c->r[13];
        c->r[3] = psk_r32(a);
        c->r[4] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[15] = c->r[14] & ~3u;
    return;
}
