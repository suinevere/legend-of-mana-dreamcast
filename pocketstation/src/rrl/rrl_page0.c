/*----------------------
 | rrl_page0.c
 | Description: Ring Ring Land routines at 02000000-02000FFF as register-level C, until they are named and grouped by concern.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "rrl.h"

/*----------------------
 | rrl_standby
 | Description: Standby after inactivity: silences sound, turns peripherals and the LCD off, and stops the clock until a button. Unverified (standby, the runner cannot sample a stopped clock). Register-level C for the ARM routine at 02000518; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_standby(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020082B8u, 0x02000520u);
    c->r[0] = 0x00000001u;
    psk_call(c, 0x02008294u, 0x02000528u);
    c->r[0] = 0x00000002u;
    psk_call(c, 0x02008294u, 0x02000530u);
    c->r[0] = 0x0D800000u;
    c->r[1] = 0x00000002u;
    psk_w32(c->r[0], c->r[1]);
    c->r[1] = 0x00000062u;
    psk_w32(c->r[0] + 4u, c->r[1]);
    c->r[1] = 0x00000000u;
    psk_w32(c->r[0] + 0x10u, c->r[1]);
    c->r[1] = 0x00000004u;
    psk_w32(c->r[0] + 0x20u, c->r[1]);
    c->r[0] = 0x0C800000u;
    c->r[1] = 0x00000002u;
    psk_w32(c->r[0], c->r[1]);
    c->r[0] = 0x0D000000u;
    c->r[1] = psk_ldr(c->r[0]);
    c->r[1] = c->r[1] & ~0x00000048u;
    psk_w32(c->r[0], c->r[1]);
    c->r[0] = 0x0A000000u;
    c->r[1] = 0x00000200u;
    psk_w32(c->r[0] + 0xCu, c->r[1]);
    c->r[0] = 0x0B000000u;
    c->r[1] = 0x00000001u;
    psk_w32(c->r[0] + 4u, c->r[1]);
    c->r[0] = 0x0A000000u;
    c->r[1] = 0x00000200u;
    psk_w32(c->r[0] + 8u, c->r[1]);
    c->r[0] = 0x00000001u;
    psk_call(c, 0x020082C4u, 0x0200059Cu);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020004F4
 | Description: Unverified (quit path, needs the dock edge or YES in the quit dialog). Register-level C for the ARM routine at 020004F4; it calls 1 routine and SWI 08, 09, 11.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020004F4(PskCpu *c) {
    c->r[0] = 0x00000000u;
    c->r[0] = psk_swi(0x11u, c->r[0], c->r[1], c->r[2]);
    psk_call(c, 0x02003858u, 0x02000500u);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00000000u;
    c->r[2] = 0x00000010u;
    c->r[0] = psk_swi(0x08u, c->r[0], c->r[1], c->r[2]);
    c->r[0] = 0x00000000u;
    c->r[0] = psk_swi(0x09u, c->r[0], c->r[1], c->r[2]);
    c->r[15] = 0x02000518u;
    return;
}

/*----------------------
 | rrl_func_02000400
 | Description: Register-level C for the ARM routine at 02000400; it calls 1 routine and SWI 14.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x200, WRAM 0x204
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000400(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = 0x0A000000u;
    c->r[1] = psk_ldr(c->r[0] + 4u);
    psk_set_carry(c, 0u);
    psk_nz(c, c->r[1] & 0x00000400u);
    if (psk_cond(c, PSK_NE)) {
        c->r[15] = 0x020004F4u;
        return;
    }
    c->r[0] = 0x00000204u;
    c->r[1] = 0x00000000u;
    psk_w8(c->r[0], (uint8_t)c->r[1]);
    c->r[0] = psk_swi(0x14u, c->r[0], c->r[1], c->r[2]);
    c->r[1] = psk_ldr(c->r[0]);
    c->r[2] = 0x00000200u;
    psk_w32(c->r[2], c->r[1]);
    c->r[0] = 0x0000000Au;
    c->r[1] = 0x020005A0u;
    psk_call(c, 0x02002BC4u, 0x0200043Cu);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000440
 | Description: Register-level C for the ARM routine at 02000440; it calls 7 routines and SWI 04, 13.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x204, WRAM 0x7A1
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000440(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = 0x00000204u;
    c->r[1] = psk_r8(c->r[0]);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000498;
    }
    c->r[0] = 0x0A000000u;
    c->r[1] = psk_ldr(c->r[0] + 4u);
    psk_set_carry(c, 0u);
    psk_nz(c, c->r[1] & 0x00000400u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000468;
    }
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
a_02000468:
    c->r[0] = 0x00000002u;
    psk_call(c, 0x02008294u, 0x02000470u);
    psk_call(c, 0x020082A0u, 0x02000474u);
    c->r[0] = 0x0D800000u;
    c->r[1] = 0x00000002u;
    psk_w32(c->r[0], c->r[1]);
    c->r[1] = 0x00000062u;
    psk_w32(c->r[0] + 4u, c->r[1]);
    c->r[1] = 0x00000000u;
    psk_w32(c->r[0] + 0x10u, c->r[1]);
    c->r[0] = 0x00000002u;
    c->r[0] = psk_swi(0x04u, c->r[0], c->r[1], c->r[2]);
a_02000498:
    psk_call(c, 0x020082ACu, 0x0200049Cu);
    c->r[0] = psk_swi(0x13u, c->r[0], c->r[1], c->r[2]);
    c->r[1] = psk_ldrh(c->r[0] + 4u);
    c->r[4] = 0x04000000u;
    c->r[4] = c->r[4] + c->r[1];
    c->r[4] = c->r[4] + 0x00000080u;
    c->r[0] = 0x00000018u;
    c->r[1] = 0x00000000u;
    c->r[2] = 0x00000008u;
    c->r[3] = 0x00000008u;
    c->r[5] = 0x00000000u;
    psk_call(c, 0x02002FE8u, 0x020004C8u);
    psk_call(c, 0x020032D4u, 0x020004CCu);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x000007A1u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02003DD8u, 0x020004DCu);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x00000000u;
    psk_call(c, 0x02003E48u, 0x020004E8u);
    c->r[0] = 0x00000001u;
    c->r[1] = 0x0000005Au;
    psk_call(c, 0x02003E48u, 0x020004F4u);
    c->r[15] = 0x020004F4u;
    return;
}

/*----------------------
 | rrl_func_02000614
 | Description: Register-level C for the ARM routine at 02000614; it calls 5 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000614(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020082D0u, 0x0200061Cu);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000670;
    }
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020006AC;
    }
    c->r[3] = 0x00000000u;
    c->r[7] = psk_ldrh(c->r[6] + 0x4Cu);
    c->r[8] = psk_r8(c->r[6] + 0x45u);
    c->r[9] = c->r[6] + 0x00000180u;
    psk_call(c, 0x020017E0u, 0x02000644u);
    c->r[3] = 0x0000000Bu;
    c->r[7] = psk_ldrh(c->r[6] + 0x4Eu);
    c->r[8] = psk_r8(c->r[6] + 0x45u);
    c->r[9] = c->r[6] + 0x00000190u;
    psk_call(c, 0x020017E0u, 0x02000658u);
    c->r[3] = 0x00000016u;
    c->r[7] = psk_ldrh(c->r[6] + 0x52u);
    c->r[8] = psk_r8(c->r[6] + 0x45u);
    c->r[9] = c->r[6] + 0x000001A0u;
    psk_call(c, 0x020017E0u, 0x0200066Cu);
    goto a_020006B8;
a_02000670:
    c->r[0] = psk_ldrsh(c->r[6] + 0x16u);
    c->r[0] = psk_asr(c->r[0], 1u);
    c->r[1] = 0x00000000u;
    c->r[2] = psk_r8(c->r[6] + 0x15u);
    c->r[2] = (c->r[2] << 1u);
    psk_call(c, 0x02001A8Cu, 0x02000688u);
    c->r[0] = psk_ldrsh(c->r[6] + 0x16u);
    c->r[0] = psk_asr(c->r[0], 1u);
    c->r[0] = c->r[0] + 0x00000010u;
    c->r[1] = 0x00000000u;
    c->r[2] = psk_r8(c->r[6] + 0x15u);
    c->r[2] = (c->r[2] << 1u);
    c->r[2] = c->r[2] + 0x00000001u;
    psk_call(c, 0x02001A8Cu, 0x020006A8u);
    goto a_020006B8;
a_020006AC:
    c->r[2] = 0x00000057u;
    psk_call(c, 0x020082DCu, 0x020006B4u);
    psk_call(c, 0x02002FE8u, 0x020006B8u);
a_020006B8:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020006BC
 | Description: Register-level C for the ARM routine at 020006BC; it calls 5 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020006BC(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020082E8u, 0x020006C4u);
    c->r[2] = 0x00000042u;
    psk_call(c, 0x020082DCu, 0x020006CCu);
    psk_call(c, 0x02002FE8u, 0x020006D0u);
    c->r[0] = 0x00000006u;
    c->r[1] = 0x0000001Au;
    c->r[2] = psk_ldr(c->r[6] + 0x104u);
    c->r[3] = 0x00000032u;
    psk_call(c, 0x020019A0u, 0x020006E4u);
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000710;
    }
    c->r[1] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[1] = 0x00000003u;
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[1] = 0x0000000Eu;
    }
    c->r[0] = 0x00000000u;
    c->r[2] = 0x00000033u;
    psk_call(c, 0x020082F4u, 0x0200070Cu);
    psk_call(c, 0x02002FE8u, 0x02000710u);
a_02000710:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000714
 | Description: Register-level C for the ARM routine at 02000714; it calls 4 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000714(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008300u, 0x0200071Cu);
    c->r[2] = 0x00000043u;
    psk_call(c, 0x020082DCu, 0x02000724u);
    psk_call(c, 0x02002FE8u, 0x02000728u);
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200075C;
    }
    c->r[7] = psk_r8(c->r[6] + 0x24u);
    psk_sub_flags(c, c->r[7], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[0] = 0x00000002u;
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[0] = 0x00000016u;
    }
    c->r[1] = 0x00000010u;
    c->r[2] = 0x00000034u;
    psk_call(c, 0x020082F4u, 0x02000750u);
    psk_sub_flags(c, c->r[7], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        c->r[5] = c->r[5] | 0x00000004u;
    }
    psk_call(c, 0x02002FE8u, 0x0200075Cu);
a_0200075C:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000FC8
 | Description: Register-level C for the ARM routine at 02000FC8; it calls 8 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000FC8(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008384u, 0x02000FD0u);
    c->r[7] = c->r[6] + 0x000000A0u;
    c->r[0] = psk_r8(c->r[6] + 0x26u);
    psk_call(c, 0x020083E4u, 0x02000FDCu);
    c->r[11] = c->r[0];
    c->r[8] = 0x00000000u;
    c->r[3] = psk_r8(c->r[6] + 0x10u);
    psk_sub_flags(c, c->r[3], 0x0000000Bu);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200100C;
    }
    c->r[3] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[3], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200100C;
    }
    c->r[0] = psk_ldrsb(c->r[6] + 0x1Cu);
    c->r[1] = psk_ldrsb(c->r[6] + 0x1Du);
    c->r[10] = 0x00000000u;
    goto a_020010A0;
a_0200100C:
    c->r[3] = psk_r8(c->r[6] + 0x2Bu);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001094;
    }
    c->r[3] = psk_r8(c->r[6] + 0x2Cu);
    c->r[3] = (c->r[3] >> 1u);
    c->r[10] = psk_r8(c->r[6] + 0x27u);
    c->r[4] = psk_r8(c->r[6] + 0x2Du);
    psk_sub_flags(c, c->r[4], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001054;
    }
    psk_sub_flags(c, c->r[4], 0x00000002u);
    if (psk_cond(c, PSK_HS)) {
        goto a_0200108C;
    }
    psk_sub_flags(c, c->r[11], 0x0000000Fu);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001050;
    }
    c->r[0] = c->r[10];
    psk_call(c, 0x020083F0u, 0x02001048u);
    c->r[10] = c->r[0];
    goto a_02001054;
a_02001050:
    c->r[10] = c->r[11] & ~c->r[10];
a_02001054:
    psk_nz(c, c->r[10] & 0x00000003u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200106C;
    }
    c->r[1] = 0x00000000u - c->r[3];
    psk_sub_flags(c, c->r[3], 0x00000004u);
    if (psk_cond(c, PSK_HS)) {
        c->r[8] = 0x00000008u - c->r[3];
    }
    goto a_02001078;
a_0200106C:
    c->r[1] = c->r[3];
    psk_sub_flags(c, c->r[3], 0x00000004u);
    if (psk_cond(c, PSK_LO)) {
        c->r[8] = c->r[3];
    }
a_02001078:
    c->r[3] = (c->r[3] << 1u);
    psk_nz(c, c->r[10] & 0x00000005u);
    if (psk_cond(c, PSK_NE)) {
        c->r[0] = c->r[3];
    }
    if (psk_cond(c, PSK_EQ)) {
        c->r[0] = 0x00000000u - c->r[3];
    }
    goto a_020010A0;
a_0200108C:
    c->r[1] = c->r[3];
    goto a_02001098;
a_02001094:
    c->r[1] = 0x00000000u;
a_02001098:
    c->r[0] = 0x00000000u;
    c->r[10] = 0x00000000u;
a_020010A0:
    c->r[2] = psk_r8(c->r[6] + 0x26u);
    psk_w8(c->r[7], (uint8_t)c->r[1]);
    psk_w8(c->r[7] + 1u, (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[2]);
    c->r[4] = 0x00000002u;
    c->r[5] = 0x00000001u;
    c->r[9] = psk_r8(c->r[6] + 0x27u);
    psk_sub_flags(c, c->r[11], 0x0000000Fu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001168;
    }
    psk_nz(c, c->r[11] & 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020010EC;
    }
    psk_nz(c, c->r[9] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = 0x00000008u;
    }
    if (psk_cond(c, PSK_EQ)) {
        c->r[3] = 0x00000088u;
    }
    psk_nz(c, c->r[10] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        c->r[4] = c->r[4] + 0x00000001u;
    }
    psk_call(c, 0x020012C4u, 0x020010E4u);
    psk_nz(c, c->r[10] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        c->r[4] = c->r[4] - 0x00000001u;
    }
a_020010EC:
    psk_nz(c, c->r[11] & 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001114;
    }
    psk_nz(c, c->r[9] & 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = 0x00000004u;
    }
    if (psk_cond(c, PSK_EQ)) {
        c->r[3] = 0x00000084u;
    }
    psk_nz(c, c->r[10] & 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        c->r[4] = c->r[4] + 0x00000001u;
    }
    psk_call(c, 0x020012C4u, 0x0200110Cu);
    psk_nz(c, c->r[10] & 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        c->r[4] = c->r[4] - 0x00000001u;
    }
a_02001114:
    psk_nz(c, c->r[11] & 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200113C;
    }
    psk_nz(c, c->r[9] & 0x00000004u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = 0x00000002u;
    }
    if (psk_cond(c, PSK_EQ)) {
        c->r[3] = 0x00000082u;
    }
    psk_nz(c, c->r[10] & 0x00000004u);
    if (psk_cond(c, PSK_NE)) {
        c->r[4] = c->r[4] + 0x00000001u;
    }
    psk_call(c, 0x020012C4u, 0x02001134u);
    psk_nz(c, c->r[10] & 0x00000004u);
    if (psk_cond(c, PSK_NE)) {
        c->r[4] = c->r[4] - 0x00000001u;
    }
a_0200113C:
    psk_nz(c, c->r[11] & 0x00000008u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001190;
    }
    psk_nz(c, c->r[9] & 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = 0x00000001u;
    }
    if (psk_cond(c, PSK_EQ)) {
        c->r[3] = 0x00000081u;
    }
    psk_nz(c, c->r[10] & 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        c->r[4] = c->r[4] + 0x00000001u;
    }
    psk_call(c, 0x020012C4u, 0x0200115Cu);
    psk_nz(c, c->r[10] & 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        c->r[4] = c->r[4] - 0x00000001u;
    }
    goto a_02001190;
a_02001168:
    c->r[2] = 0x00000008u;
    c->r[3] = 0x00000082u;
    psk_call(c, 0x020012C4u, 0x02001174u);
    c->r[3] = 0x00000004u;
    psk_call(c, 0x020012C4u, 0x0200117Cu);
    c->r[2] = 0x00000018u;
    c->r[3] = 0x00000081u;
    psk_call(c, 0x020012C4u, 0x02001188u);
    c->r[3] = 0x00000008u;
    psk_call(c, 0x020012C4u, 0x02001190u);
a_02001190:
    c->r[9] = c->r[5];
    c->r[11] = c->r[5];
    c->r[2] = 0x00000000u;
    psk_call(c, 0x020082DCu, 0x020011A0u);
a_020011A0:
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[10] = psk_ldrsb(c->r[7]);
    c->r[1] = c->r[1] + c->r[10];
    c->r[10] = psk_ldrsb(c->r[7] + 1u);
    c->r[0] = c->r[0] + c->r[10];
    c->r[7] = c->r[7] + 0x00000003u;
    psk_call(c, 0x02002FE8u, 0x020011BCu);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[9] = psk_sub_flags(c, c->r[9], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020011A0;
    }
    c->r[10] = psk_r8(c->r[6] + 0x2Au);
    c->r[7] = c->r[6] + 0x000000A0u;
a_020011D0:
    psk_sub_flags(c, c->r[10], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020011F8;
    }
    c->r[1] = psk_ldrsb(c->r[7]);
    psk_sub_flags(c, c->r[1], c->r[8]);
    if (psk_cond(c, PSK_LE)) {
        goto a_020011F8;
    }
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000000u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x020011F4u);
    c->r[10] = 0x00000000u;
a_020011F8:
    c->r[9] = psk_r8(c->r[7] + 2u);
    c->r[9] = c->r[6] + c->r[9];
    c->r[2] = psk_r8(c->r[9] + 0xC0u);
    psk_sub_flags(c, c->r[2], 0x000000FFu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001220;
    }
    c->r[2] = c->r[2] + 0x00000001u;
    c->r[1] = psk_ldrsb(c->r[7]);
    c->r[0] = psk_ldrsb(c->r[7] + 1u);
    psk_call(c, 0x020082F4u, 0x0200121Cu);
    psk_call(c, 0x02002FE8u, 0x02001220u);
a_02001220:
    c->r[2] = psk_r8(c->r[9] + 0xE0u);
    psk_sub_flags(c, c->r[2], 0x000000FFu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001240;
    }
    psk_sub_flags(c, c->r[2], 0x00000010u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001240;
    }
    c->r[1] = psk_ldrsb(c->r[7]);
    c->r[0] = psk_ldrsb(c->r[7] + 1u);
    psk_call(c, 0x02001430u, 0x02001240u);
a_02001240:
    c->r[2] = psk_r8(c->r[6] + 0x2Au);
    psk_sub_flags(c, c->r[2], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200129C;
    }
    c->r[8] = psk_r8(c->r[6] + 0x28u);
    c->r[9] = psk_r8(c->r[7] + 2u);
    psk_sub_flags(c, c->r[9], c->r[8]);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200129C;
    }
    c->r[1] = psk_ldrsb(c->r[7]);
    c->r[0] = psk_ldrsb(c->r[7] + 1u);
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x0200126Cu);
    c->r[2] = psk_r8(c->r[6] + 0x10u);
    psk_sub_flags(c, c->r[2], 0x0000000Bu);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200129C;
    }
    c->r[2] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[2], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200129C;
    }
    c->r[1] = psk_ldrsb(c->r[7]);
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    c->r[1] = c->r[1] - c->r[0];
    c->r[0] = psk_ldrsb(c->r[7] + 1u);
    c->r[2] = 0x00000001u;
    psk_call(c, 0x02001430u, 0x0200129Cu);
a_0200129C:
    c->r[7] = c->r[7] + 0x00000003u;
    c->r[11] = psk_sub_flags(c, c->r[11], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020011D0;
    }
    psk_sub_flags(c, c->r[10], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020012C0;
    }
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000000u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x020012C0u);
a_020012C0:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000760
 | Description: Register-level C for the ARM routine at 02000760; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000760(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x0200830Cu, 0x02000768u);
    psk_call(c, 0x02000FC8u, 0x0200076Cu);
    c->r[10] = 0x00000000u;
    psk_call(c, 0x02001838u, 0x02000774u);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000778
 | Description: Register-level C for the ARM routine at 02000778; it calls 4 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000778(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008318u, 0x02000780u);
    psk_call(c, 0x02000FC8u, 0x02000784u);
    c->r[10] = 0x00000000u;
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020007CC;
    }
    c->r[7] = psk_r8(c->r[6] + 0x71u);
    psk_nz(c, c->r[7] & 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[0] = 0x00000000u;
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[0] = 0x00000018u;
    }
    psk_nz(c, c->r[7] & 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[1] = 0x00000018u;
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[1] = 0x0000000Au;
    }
    c->r[2] = 0x00000034u;
    psk_call(c, 0x020082F4u, 0x020007B8u);
    psk_nz(c, c->r[7] & 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        c->r[5] = c->r[5] | 0x00000004u;
    }
    psk_nz(c, c->r[7] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        c->r[4] = c->r[4] + 0x00000010u;
    }
    psk_call(c, 0x02002FE8u, 0x020007CCu);
a_020007CC:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020007D0
 | Description: Register-level C for the ARM routine at 020007D0; it calls 4 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020007D0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02000FC8u, 0x020007D8u);
    c->r[10] = 0x00000000u;
    psk_call(c, 0x02001838u, 0x020007E0u);
    c->r[1] = psk_r8(c->r[6] + 0x12u);
    c->r[1] = c->r[1] + 0x00000001u;
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[1]);
    c->r[0] = 0x00000000u;
    psk_sub_flags(c, c->r[1], 0x00000046u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02000818;
    }
    psk_sub_flags(c, c->r[1], 0x0000002Du);
    if (psk_cond(c, PSK_HS)) {
        goto a_02000814;
    }
    c->r[0] = 0x00000000u;
    c->r[1] = 0x0000000Au;
    c->r[2] = 0x00000045u;
    psk_call(c, 0x020082F4u, 0x02000810u);
    psk_call(c, 0x02002FE8u, 0x02000814u);
a_02000814:
    c->r[0] = 0x00000001u;
a_02000818:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_0200081C
 | Description: Register-level C for the ARM routine at 0200081C; it calls 5 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200081C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008324u, 0x02000824u);
    psk_call(c, 0x02008330u, 0x02000828u);
    psk_call(c, 0x02000FC8u, 0x0200082Cu);
    psk_call(c, 0x0200153Cu, 0x02000830u);
    c->r[10] = 0x00000000u;
    psk_call(c, 0x02001838u, 0x02000838u);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_0200083C
 | Description: Register-level C for the ARM routine at 0200083C; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200083C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x0200833Cu, 0x02000844u);
    psk_call(c, 0x02000FC8u, 0x02000848u);
    c->r[10] = 0x00000000u;
    psk_call(c, 0x02001838u, 0x02000850u);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000F88
 | Description: Register-level C for the ARM routine at 02000F88; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000F88(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = 0x00000060u;
    c->r[7] = c->r[7] + c->r[6];
    c->r[11] = 0x00000004u;
a_02000F98:
    c->r[0] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[0], 0x000000FFu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000FB8;
    }
    c->r[10] = c->r[7];
    psk_call(c, 0x020083D8u, 0x02000FACu);
    psk_sub_flags(c, c->r[4], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x02002FE8u, 0x02000FB4u);
    }
    c->r[7] = c->r[10];
a_02000FB8:
    c->r[7] = c->r[7] + 0x00000004u;
    c->r[11] = psk_sub_flags(c, c->r[11], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000F98;
    }
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000854
 | Description: Register-level C for the ARM routine at 02000854; it calls 7 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000854(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008348u, 0x0200085Cu);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000870;
    }
    psk_call(c, 0x02000F88u, 0x0200086Cu);
    goto a_02000948;
a_02000870:
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020008FC;
    }
    c->r[11] = psk_r8(c->r[6] + 0x18u);
    c->r[11] = (c->r[11] >> 1u);
    c->r[0] = psk_r8(c->r[6] + 0x16u);
    psk_sub_flags(c, c->r[0], 0x000000FFu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020008D4;
    }
    psk_call(c, 0x02008354u, 0x02000894u);
    c->r[2] = c->r[0] & 0x000000FFu;
    c->r[3] = (c->r[0] >> 8u);
    c->r[3] = c->r[3] + 0x00000001u;
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000000u;
    c->r[0] = c->r[0] - c->r[11];
    psk_call(c, 0x02001494u, 0x020008B0u);
    c->r[0] = 0x00000016u;
    c->r[0] = c->r[0] - c->r[11];
    c->r[1] = 0x00000009u;
    c->r[2] = 0x0000004Du;
    psk_call(c, 0x020082F4u, 0x020008C4u);
    c->r[7] = psk_r8(c->r[6] + 0x19u);
    psk_sub_flags(c, c->r[7], 0x0000000Au);
    if (psk_cond(c, PSK_LS)) {
        c->r[4] = c->r[4] + 0x0000001Cu;
    }
    psk_call(c, 0x02002FE8u, 0x020008D4u);
a_020008D4:
    c->r[0] = psk_r8(c->r[6] + 0x17u);
    psk_call(c, 0x02008354u, 0x020008DCu);
    c->r[2] = c->r[0] & 0x000000FFu;
    c->r[3] = (c->r[0] >> 8u);
    c->r[3] = c->r[3] + 0x00000001u;
    c->r[0] = 0x00000034u;
    c->r[1] = 0x00000000u;
    c->r[0] = c->r[0] - c->r[11];
    psk_call(c, 0x02001494u, 0x020008F8u);
    goto a_02000948;
a_020008FC:
    c->r[3] = 0x00000000u;
    c->r[7] = 0x00000000u;
    c->r[8] = psk_r8(c->r[6] + 0x45u);
    c->r[9] = c->r[6] + 0x00000180u;
    psk_call(c, 0x020016F8u, 0x02000910u);
    c->r[0] = psk_r8(c->r[6] + 0x19u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000948;
    }
    c->r[0] = 0x0000000Bu;
    c->r[1] = 0x00000011u;
    c->r[2] = 0x00000037u;
    psk_call(c, 0x020082F4u, 0x0200092Cu);
    c->r[4] = c->r[4] + 0x00000008u;
    psk_call(c, 0x02002FE8u, 0x02000934u);
    c->r[0] = c->r[0] + c->r[2];
    c->r[2] = 0x00000035u;
    psk_call(c, 0x020082F4u, 0x02000940u);
    c->r[4] = c->r[4] + 0x00000004u;
    psk_call(c, 0x02002FE8u, 0x02000948u);
a_02000948:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_0200094C
 | Description: Register-level C for the ARM routine at 0200094C; it calls 6 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200094C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008360u, 0x02000954u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000970;
    }
    psk_sub_flags(c, c->r[0], 0x00000005u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200097C;
    }
    psk_call(c, 0x02000F88u, 0x0200096Cu);
    goto a_0200099C;
a_02000970:
    psk_call(c, 0x0200836Cu, 0x02000974u);
    psk_call(c, 0x02001584u, 0x02000978u);
    goto a_0200099C;
a_0200097C:
    c->r[0] = psk_r8(c->r[6] + 0x16u);
    psk_call(c, 0x02008354u, 0x02000984u);
    c->r[2] = c->r[0] & 0x000000FFu;
    c->r[3] = (c->r[0] >> 8u);
    c->r[3] = c->r[3] + 0x00000001u;
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000000u;
    psk_call(c, 0x02001494u, 0x0200099Cu);
a_0200099C:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020009A0
 | Description: Register-level C for the ARM routine at 020009A0; it calls 15 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020009A0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008378u, 0x020009A8u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020009FC;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000A34;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000A40;
    }
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000AB8;
    }
    psk_sub_flags(c, c->r[0], 0x00000006u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000AD4;
    }
    psk_sub_flags(c, c->r[0], 0x00000007u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000B20;
    }
    psk_sub_flags(c, c->r[0], 0x00000008u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000B4C;
    }
    psk_sub_flags(c, c->r[0], 0x00000009u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000B74;
    }
    psk_sub_flags(c, c->r[0], 0x0000000Au);
    if (psk_cond(c, PSK_HS)) {
        goto a_02000B98;
    }
    psk_call(c, 0x02000F88u, 0x020009F8u);
    goto a_02000BA8;
a_020009FC:
    psk_call(c, 0x02008384u, 0x02000A00u);
    psk_call(c, 0x02000F88u, 0x02000A04u);
    c->r[8] = psk_r8(c->r[6] + 0x11u);
    c->r[0] = ~0x00000009u;
    c->r[0] = c->r[0] - c->r[8];
    c->r[1] = 0x00000007u;
    c->r[2] = 0x00000001u;
    psk_call(c, 0x02001430u, 0x02000A1Cu);
    c->r[0] = 0x0000000Au;
    c->r[0] = c->r[0] + c->r[8];
    c->r[1] = 0x00000007u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x02000A30u);
    goto a_02000BA8;
a_02000A34:
    psk_call(c, 0x02008390u, 0x02000A38u);
    psk_call(c, 0x020015E4u, 0x02000A3Cu);
    goto a_02000BA8;
a_02000A40:
    psk_call(c, 0x02008384u, 0x02000A44u);
    c->r[0] = ~0x00000009u;
    c->r[1] = 0x00000007u;
    c->r[2] = 0x00000001u;
    psk_call(c, 0x02001430u, 0x02000A54u);
    c->r[0] = 0x0000000Au;
    c->r[1] = 0x00000007u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x02000A64u);
    c->r[2] = 0x00000048u;
    psk_call(c, 0x020082DCu, 0x02000A6Cu);
    psk_call(c, 0x02002FE8u, 0x02000A70u);
    c->r[0] = c->r[0] + 0x00000010u;
    c->r[4] = c->r[4] + 0x00000038u;
    psk_call(c, 0x02002FE8u, 0x02000A7Cu);
    c->r[0] = 0x00000018u;
    c->r[1] = 0x00000001u;
    c->r[2] = 0x00000035u;
    psk_call(c, 0x020082F4u, 0x02000A8Cu);
    c->r[8] = psk_r8(c->r[6] + 0x17u);
    c->r[4] = c->r[4] + (c->r[8] << 2u);
    psk_call(c, 0x02002FE8u, 0x02000A98u);
    c->r[0] = 0x00000005u;
    c->r[1] = 0x00000001u;
    c->r[2] = 0x00000035u;
    psk_call(c, 0x020082F4u, 0x02000AA8u);
    c->r[8] = psk_r8(c->r[6] + 0x18u);
    c->r[4] = c->r[4] + (c->r[8] << 2u);
    psk_call(c, 0x02002FE8u, 0x02000AB4u);
    goto a_02000BA8;
a_02000AB8:
    psk_call(c, 0x02008384u, 0x02000ABCu);
    psk_call(c, 0x02000F88u, 0x02000AC0u);
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000007u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x02000AD0u);
    goto a_02000BA8;
a_02000AD4:
    c->r[2] = 0x0000004Cu;
    psk_call(c, 0x020082DCu, 0x02000ADCu);
    psk_call(c, 0x02002FE8u, 0x02000AE0u);
    c->r[0] = 0x00000004u;
    c->r[1] = 0x0000000Cu;
    c->r[2] = 0x00000037u;
    psk_call(c, 0x020082F4u, 0x02000AF0u);
    c->r[4] = c->r[4] + 0x00000008u;
    psk_call(c, 0x02002FE8u, 0x02000AF8u);
    c->r[0] = c->r[0] + c->r[2];
    c->r[2] = psk_ldrh(c->r[6] + 0x1Cu);
    psk_call(c, 0x0200839Cu, 0x02000B04u);
    psk_call(c, 0x020018C8u, 0x02000B08u);
    c->r[0] = 0x0000000Au;
    c->r[1] = 0x00000018u;
    c->r[2] = psk_ldrh(c->r[6] + 0x3Au);
    psk_call(c, 0x0200839Cu, 0x02000B18u);
    psk_call(c, 0x020018C8u, 0x02000B1Cu);
    goto a_02000BA8;
a_02000B20:
    psk_call(c, 0x02008384u, 0x02000B24u);
    c->r[0] = ~0x00000009u;
    c->r[1] = 0x00000007u;
    c->r[2] = 0x00000001u;
    psk_call(c, 0x02001430u, 0x02000B34u);
    c->r[0] = 0x0000000Au;
    c->r[1] = 0x00000007u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x02000B44u);
    psk_call(c, 0x02000F88u, 0x02000B48u);
    goto a_02000BA8;
a_02000B4C:
    psk_call(c, 0x02008384u, 0x02000B50u);
    c->r[2] = 0x00000049u;
    psk_call(c, 0x020082DCu, 0x02000B58u);
    psk_call(c, 0x02002FE8u, 0x02000B5Cu);
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000007u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x02000B6Cu);
    psk_call(c, 0x02000F88u, 0x02000B70u);
    goto a_02000BA8;
a_02000B74:
    c->r[0] = psk_r8(c->r[6] + 0x1Au);
    psk_call(c, 0x02008354u, 0x02000B7Cu);
    c->r[2] = c->r[0] & 0x000000FFu;
    c->r[3] = (c->r[0] >> 8u);
    c->r[3] = c->r[3] + 0x00000001u;
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000000u;
    psk_call(c, 0x02001494u, 0x02000B94u);
    goto a_02000BA8;
a_02000B98:
    psk_call(c, 0x02000FC8u, 0x02000B9Cu);
    c->r[10] = 0x00000000u;
    psk_call(c, 0x02001838u, 0x02000BA4u);
    goto a_02000BA8;
a_02000BA8:
    c->r[0] = psk_r8(c->r[6] + 0x10u);
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_NE)) {
        psk_sub_flags(c, c->r[0], 0x0000000Eu);
    }
    if (psk_cond(c, PSK_NE)) {
        goto a_02000BC8;
    }
    c->r[0] = psk_r8(c->r[6] + 0x16u);
    psk_w8(c->r[6] + 0x71u, (uint8_t)c->r[0]);
    c->r[0] = 0x000000FFu;
    psk_w8(c->r[5] + 0x74u, (uint8_t)c->r[0]);
a_02000BC8:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000BD8
 | Description: Register-level C for the ARM routine at 02000BD8; it calls 4 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000BD8(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020083A8u, 0x02000BE0u);
    c->r[2] = 0x00000044u;
    psk_call(c, 0x020082DCu, 0x02000BE8u);
    psk_call(c, 0x02002FE8u, 0x02000BECu);
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000C18;
    }
    c->r[1] = psk_r8(c->r[6] + 0x14u);
    c->r[0] = (c->r[1] << 3u);
    c->r[1] = c->r[0] + (c->r[1] << 1u);
    c->r[0] = 0x00000000u;
    c->r[1] = c->r[1] + 0x00000002u;
    c->r[2] = 0x00000033u;
    psk_call(c, 0x020082F4u, 0x02000C14u);
    psk_call(c, 0x02002FE8u, 0x02000C18u);
a_02000C18:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000C1C
 | Description: Register-level C for the ARM routine at 02000C1C; it calls 7 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000C1C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020083B4u, 0x02000C24u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000CA4;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000D34;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000D70;
    }
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000000u;
    c->r[2] = psk_r8(c->r[6] + 0x18u);
    c->r[3] = 0x00000000u;
    psk_call(c, 0x02001494u, 0x02000C54u);
    c->r[0] = 0x00000000u;
    c->r[1] = 0x0000001Au;
    c->r[2] = psk_r8(c->r[6] + 0x1Cu);
    psk_call(c, 0x02001A3Cu, 0x02000C64u);
    c->r[0] = 0x00000017u;
    c->r[2] = psk_ldr(c->r[6] + 0x104u);
    psk_sub_flags(c, c->r[2], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        c->r[0] = c->r[0] + 0x00000004u;
    }
    c->r[1] = 0x0000001Au;
    c->r[2] = 0x00000035u;
    psk_call(c, 0x020082F4u, 0x02000C80u);
    c->r[4] = c->r[4] + 0x00000028u;
    c->r[5] = c->r[5] | 0x00000002u;
    psk_call(c, 0x02002FE8u, 0x02000C8Cu);
    c->r[0] = 0x00000018u;
    c->r[1] = 0x0000001Au;
    c->r[2] = psk_ldr(c->r[6] + 0x104u);
    psk_call(c, 0x02001A3Cu, 0x02000C9Cu);
    c->r[0] = psk_r8(c->r[6] + 0x1Bu);
    goto a_02000CF0;
a_02000CA4:
    c->r[0] = psk_r8(c->r[6] + 0x1Au);
    c->r[0] = c->r[0] + 0x00000108u;
    c->r[0] = psk_r8(c->r[6] + c->r[0]);
    psk_call(c, 0x02008354u, 0x02000CB4u);
    c->r[2] = c->r[0] & 0x000000FFu;
    c->r[3] = (c->r[0] >> 8u);
    c->r[3] = c->r[3] + 0x00000001u;
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000000u;
    psk_call(c, 0x02001494u, 0x02000CCCu);
    c->r[0] = 0x00000000u;
    c->r[1] = 0x0000001Au;
    c->r[2] = psk_r8(c->r[6] + 0x19u);
    c->r[2] = c->r[2] + 0x00000001u;
    psk_call(c, 0x02001A3Cu, 0x02000CE0u);
    c->r[2] = 0x00000047u;
    psk_call(c, 0x020082DCu, 0x02000CE8u);
    psk_call(c, 0x02002FE8u, 0x02000CECu);
    c->r[0] = psk_r8(c->r[6] + 0x1Cu);
a_02000CF0:
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000DE4;
    }
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000DE4;
    }
    c->r[0] = 0x00000000u;
    c->r[1] = 0x0000000Cu;
    c->r[2] = 0x00000033u;
    psk_call(c, 0x020082F4u, 0x02000D14u);
    c->r[5] = c->r[5] | 0x00000004u;
    psk_call(c, 0x02002FE8u, 0x02000D1Cu);
    c->r[0] = 0x00000018u;
    c->r[1] = 0x0000000Cu;
    c->r[2] = 0x00000033u;
    psk_call(c, 0x020082F4u, 0x02000D2Cu);
    psk_call(c, 0x02002FE8u, 0x02000D30u);
    goto a_02000DE4;
a_02000D34:
    c->r[2] = 0x00000046u;
    psk_call(c, 0x020082DCu, 0x02000D3Cu);
    psk_call(c, 0x02002FE8u, 0x02000D40u);
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000DE4;
    }
    c->r[0] = 0x00000000u;
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[1] = 0x00000000u;
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[1] = 0x00000007u;
    }
    c->r[2] = 0x00000033u;
    psk_call(c, 0x020082F4u, 0x02000D68u);
    psk_call(c, 0x02002FE8u, 0x02000D6Cu);
    goto a_02000DE4;
a_02000D70:
    c->r[2] = 0x0000004Du;
    psk_call(c, 0x020082DCu, 0x02000D78u);
    c->r[7] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[7], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000D90;
    }
    c->r[7] = psk_r8(c->r[6] + 0x16u);
    psk_sub_flags(c, c->r[7], 0x0000000Au);
    if (psk_cond(c, PSK_LS)) {
        c->r[4] = c->r[4] + 0x0000001Cu;
    }
a_02000D90:
    psk_call(c, 0x02002FE8u, 0x02000D94u);
    c->r[2] = 0x0000004Eu;
    psk_call(c, 0x020082DCu, 0x02000D9Cu);
    c->r[7] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[7], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000DB4;
    }
    c->r[7] = psk_r8(c->r[6] + 0x16u);
    psk_sub_flags(c, c->r[7], 0x0000000Au);
    if (psk_cond(c, PSK_LS)) {
        c->r[4] = c->r[4] + 0x00000018u;
    }
a_02000DB4:
    psk_call(c, 0x02002FE8u, 0x02000DB8u);
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000DE4;
    }
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[1] = 0x00000004u;
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[1] = 0x00000015u;
    }
    c->r[0] = 0x00000000u;
    c->r[2] = 0x00000033u;
    psk_call(c, 0x020082F4u, 0x02000DE0u);
    psk_call(c, 0x02002FE8u, 0x02000DE4u);
a_02000DE4:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02000E2C
 | Description: Register-level C for the ARM routine at 02000E2C; it calls 12 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x240
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02000E2C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020083CCu, 0x02000E34u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000EA4;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000EE4;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000F1C;
    }
    psk_call(c, 0x02000FC8u, 0x02000E54u);
    c->r[10] = psk_r8(c->r[6] + 0x2Cu);
    psk_call(c, 0x02001838u, 0x02000E5Cu);
    c->r[7] = psk_r8(c->r[6] + 0x2Cu);
    c->r[0] = 0x00000240u;
    c->r[1] = psk_nz(c, psk_lsr_s(c, c->r[7], 1u));
    if (psk_cond(c, PSK_EQ)) {
        goto a_02000E7C;
    }
    c->r[2] = 0x00000000u;
a_02000E70:
    {
        uint32_t a = c->r[0];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[2]);
        c->r[0] = b;
    }
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000E70;
    }
a_02000E7C:
    c->r[1] = ~0x0000001Fu;
    c->r[1] = c->r[1] + (c->r[7] >> 1u);
    c->r[0] = 0x00000000u;
    c->r[2] = 0x0000004Bu;
    psk_call(c, 0x020082F4u, 0x02000E90u);
    c->r[7] = psk_r8(c->r[6] + 0x11u);
    c->r[4] = c->r[4] + (c->r[7] << 7u);
    c->r[4] = c->r[4] - ((c->r[7] << 4u));
    psk_call(c, 0x02002FE8u, 0x02000EA0u);
    goto a_02000F84;
a_02000EA4:
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[0], 0x00000008u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02000F84;
    }
    c->r[0] = psk_r8(c->r[6] + 0x38u);
    c->r[0] = c->r[0] - 0x00000001u;
    c->r[1] = 0x00000108u;
    c->r[0] = c->r[0] + c->r[1];
    c->r[0] = psk_r8(c->r[6] + c->r[0]);
    psk_call(c, 0x02008354u, 0x02000EC8u);
    c->r[2] = c->r[0] & 0x000000FFu;
    c->r[3] = (c->r[0] >> 8u);
    c->r[3] = c->r[3] + 0x00000001u;
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000000u;
    psk_call(c, 0x02001494u, 0x02000EE0u);
    goto a_02000F84;
a_02000EE4:
    c->r[2] = 0x0000004Cu;
    psk_call(c, 0x020082DCu, 0x02000EECu);
    psk_call(c, 0x02002FE8u, 0x02000EF0u);
    c->r[0] = 0x00000004u;
    c->r[1] = 0x00000014u;
    c->r[2] = 0x00000037u;
    psk_call(c, 0x020082F4u, 0x02000F00u);
    c->r[4] = c->r[4] + 0x00000008u;
    psk_call(c, 0x02002FE8u, 0x02000F08u);
    c->r[0] = c->r[0] + c->r[2];
    c->r[2] = psk_ldrh(c->r[6] + 0x3Au);
    psk_call(c, 0x0200839Cu, 0x02000F14u);
    psk_call(c, 0x020018C8u, 0x02000F18u);
    goto a_02000F84;
a_02000F1C:
    psk_call(c, 0x02008384u, 0x02000F20u);
    c->r[0] = psk_ldrsh(c->r[6] + 0x18u);
    c->r[0] = psk_asr(c->r[0], 1u);
    c->r[8] = psk_r8(c->r[6] + 0x45u);
    c->r[9] = c->r[6] + 0x00000180u;
    c->r[1] = 0x00000000u;
    c->r[2] = 0x00000008u;
    c->r[3] = 0x0000000Bu;
    c->r[5] = 0x00000000u;
a_02000F40:
    psk_sub_flags(c, c->r[0], 0x00000020u);
    if (psk_cond(c, PSK_GE)) {
        goto a_02000F6C;
    }
    c->r[4] = 0x02007DBCu;
    c->r[11] = psk_r8(c->r[9]);
    c->r[4] = c->r[4] + (c->r[11] << 3u);
    c->r[4] = c->r[4] + (c->r[11] << 2u);
    psk_call(c, 0x02002FE8u, 0x02000F5Cu);
    c->r[9] = c->r[9] + 0x00000001u;
    c->r[0] = c->r[0] + c->r[2];
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02000F40;
    }
a_02000F6C:
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000007u;
    c->r[2] = psk_r8(c->r[6] + 0x17u);
    c->r[1] = c->r[1] - c->r[2];
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x02000F84u);
a_02000F84:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}
