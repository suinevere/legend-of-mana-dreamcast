/*----------------------
 | rrl_page1.c
 | Description: Ring Ring Land routines at 02001000-02001FFF as register-level C, until they are named and grouped by concern.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "rrl.h"

/*----------------------
 | rrl_func_02001AE0
 | Description: Register-level C for the Thumb routine at 02001AE0; it calls 4 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001AE0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001B0E;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001B22;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001B4A;
    }
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001B6C;
    }
    psk_sub_flags(c, c->r[0], 0x00000005u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001B22;
    }
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001BA8;
    }
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001BC8;
    }
    psk_call(c, 0x02004437u, 0x02001B0Du);
    goto a_02001BC2;
a_02001B0E:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001BA8;
    }
    psk_call(c, 0x0200486Fu, 0x02001B17u);
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_r8(c->r[5]);
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001BAE;
    }
    goto a_02001BC8;
a_02001B22:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001B3E;
    }
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001BC8;
    }
    c->r[2] = psk_nz(c, 0x00000000u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001B8C;
    }
    c->r[0] = psk_nz(c, 0x0000001Eu);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    goto a_02001BC6;
a_02001B3E:
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_call(c, 0x02003D13u, 0x02001B49u);
    goto a_02001BC8;
a_02001B4A:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001BA8;
    }
    c->r[5] = psk_nz(c, 0x00000016u);
    c->r[0] = psk_ldrsh(c->r[6] + c->r[5]);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w16((c->r[6] + c->r[5]) & ~1u, (uint16_t)c->r[0]);
    c->r[5] = psk_nz(c, 0x00000018u);
    c->r[1] = psk_ldrsh(c->r[6] + c->r[5]);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_GE)) {
        goto a_02001BC8;
    }
    c->r[2] = psk_r8(c->r[6] + 0x15u);
    c->r[2] = psk_add_flags(c, c->r[2], 0x00000001u);
    psk_sub_flags(c, c->r[2], 0x00000004u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001B8C;
    }
    c->r[0] = psk_nz(c, 0x00000078u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    goto a_02001BC2;
a_02001B6C:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001BA8;
    }
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001BC8;
    }
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[5] + 0xCu) & ~1u, (uint16_t)c->r[0]);
    psk_w16((c->r[5] + 0xEu) & ~1u, (uint16_t)c->r[0]);
    psk_w16((c->r[5] + 0x12u) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x000000F0u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000005u);
    goto a_02001BC6;
a_02001B8C:
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[2]);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 1u));
    c->r[2] = psk_add_flags(c, c->r[2], 0x00000050u);
    psk_call(c, 0x02003313u, 0x02001B97u);
    c->r[2] = psk_add_flags(c, c->r[2], 0x00000020u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 1u));
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[2]);
    psk_w16((c->r[6] + 0x18u) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000040u);
    psk_w16((c->r[6] + 0x16u) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000003u);
    goto a_02001BC6;
a_02001BA8:
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_call(c, 0x02003D13u, 0x02001BAFu);
a_02001BAE:
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[5] + 0xCu) & ~1u, (uint16_t)c->r[0]);
    psk_w16((c->r[5] + 0xEu) & ~1u, (uint16_t)c->r[0]);
    psk_w16((c->r[5] + 0x12u) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x000000F0u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000002u);
    goto a_02001BC6;
a_02001BC2:
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
a_02001BC6:
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
a_02001BC8:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020017E0
 | Description: Register-level C for the ARM routine at 020017E0; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020017E0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = (c->r[7] >> 6u);
    c->r[1] = 0x00000008u;
    psk_call(c, 0x020083FCu, 0x020017F0u);
    c->r[10] = c->r[0];
    c->r[0] = 0x00000000u - c->r[1];
    c->r[1] = c->r[3];
    c->r[2] = 0x00000008u;
    c->r[3] = 0x0000000Bu;
    c->r[5] = 0x00000000u;
a_02001808:
    c->r[4] = 0x02007DBCu;
    c->r[11] = psk_r8(c->r[9] + c->r[10]);
    c->r[4] = c->r[4] + (c->r[11] << 3u);
    c->r[4] = c->r[4] + (c->r[11] << 2u);
    psk_call(c, 0x02002FE8u, 0x0200181Cu);
    c->r[10] = c->r[10] + 0x00000001u;
    psk_sub_flags(c, c->r[10], c->r[8]);
    if (psk_cond(c, PSK_EQ)) {
        c->r[10] = 0x00000000u;
    }
    c->r[0] = c->r[0] + c->r[2];
    psk_sub_flags(c, c->r[0], 0x00000020u);
    if (psk_cond(c, PSK_LT)) {
        goto a_02001808;
    }
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02001A8C
 | Description: Register-level C for the ARM routine at 02001A8C; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001A8C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[2] = c->r[2] + 0x0000004Fu;
    psk_call(c, 0x020082F4u, 0x02001A98u);
    c->r[7] = c->r[2];
    c->r[2] = 0x00000020u;
a_02001AA0:
    psk_sub_flags(c, c->r[0], 0x00000020u);
    if (psk_cond(c, PSK_GE)) {
        goto a_02001ABC;
    }
    psk_call(c, 0x02002FE8u, 0x02001AACu);
    c->r[4] = c->r[4] + 0x00000020u;
    c->r[0] = c->r[0] + c->r[2];
    c->r[7] = psk_sub_flags(c, c->r[7], c->r[2]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02001AA0;
    }
a_02001ABC:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02001BCC
 | Description: Register-level C for the Thumb routine at 02001BCC; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001BCC(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    c->r[2] = psk_nz(c, 0x00000010u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001BE0;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001BE0;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    goto a_02001BEC;
a_02001BE0:
    c->r[2] = psk_nz(c, 0x00000008u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001BFC;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001BFC;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
a_02001BEC:
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x02003CA3u, 0x02001BF5u);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
    goto a_02001C0C;
a_02001BFC:
    c->r[1] = psk_r8(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001C0C;
    }
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[0] = psk_nz(c, c->r[0] ^ c->r[2]);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
a_02001C0C:
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[1]);
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001C3A;
    }
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001C20;
    }
    c->r[0] = psk_nz(c, 0x00000002u);
    goto a_02001C32;
a_02001C20:
    c->r[5] = psk_nz(c, 0x00000010u);
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 4u));
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_ldr(c->r[5] + 4u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001C34;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x1Eu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Eu);
a_02001C32:
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
a_02001C34:
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_call(c, 0x02003D13u, 0x02001C3Bu);
a_02001C3A:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020019A0
 | Description: Register-level C for the ARM routine at 020019A0; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020019A0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = c->r[2];
    c->r[8] = c->r[3];
    c->r[2] = 0x00000036u;
    psk_call(c, 0x020082F4u, 0x020019B4u);
    c->r[9] = c->r[4];
    psk_sub_flags(c, c->r[7], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        goto a_020019E0;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = c->r[7];
    c->r[1] = 0x0000000Au;
    psk_call(c, 0x020083FCu, 0x020019D0u);
    c->r[7] = c->r[1];
    c->r[4] = c->r[9] + (c->r[0] << 2u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    psk_call(c, 0x02002FE8u, 0x020019E0u);
a_020019E0:
    c->r[0] = c->r[0] + 0x00000004u;
    c->r[4] = c->r[9] + (c->r[7] << 2u);
    psk_call(c, 0x02002FE8u, 0x020019ECu);
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = c->r[0] + 0x00000008u;
    psk_sub_flags(c, c->r[8], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        goto a_02001A20;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = c->r[8];
    c->r[1] = 0x0000000Au;
    psk_call(c, 0x020083FCu, 0x02001A0Cu);
    c->r[8] = c->r[1];
    c->r[4] = c->r[9] + (c->r[0] << 2u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    psk_call(c, 0x02002FE8u, 0x02001A1Cu);
    c->r[0] = c->r[0] + 0x00000004u;
a_02001A20:
    c->r[4] = c->r[9] + (c->r[8] << 2u);
    psk_call(c, 0x02002FE8u, 0x02001A28u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[0] = c->r[0] + 0x00000004u;
    c->r[4] = c->r[9] + 0x00000028u;
    psk_call(c, 0x02002FE8u, 0x02001A38u);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02001C3E
 | Description: Register-level C for the Thumb routine at 02001C3E; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001C3E(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[0] = psk_r8(c->r[7] + 4u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001C56;
    }
    c->r[2] = psk_nz(c, 0x00000004u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001C6E;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    goto a_02001C5E;
a_02001C56:
    c->r[2] = psk_nz(c, 0x00000002u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001C6E;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
a_02001C5E:
    psk_w8(c->r[7] + 4u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x02003CA3u, 0x02001C67u);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
    goto a_02001C7E;
a_02001C6E:
    c->r[1] = psk_r8(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001C7E;
    }
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[0] = psk_nz(c, c->r[0] ^ c->r[2]);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
a_02001C7E:
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
 | rrl_func_02001C84
 | Description: Register-level C for the Thumb routine at 02001C84; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001C84(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7] + 0x11u);
    c->r[1] = psk_r8(c->r[7] + 0x12u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001C98;
    }
    psk_call(c, 0x0200265Du, 0x02001C97u);
    goto a_02001C9C;
a_02001C98:
    psk_call(c, 0x02002713u, 0x02001C9Du);
a_02001C9C:
    psk_call(c, 0x02002695u, 0x02001CA1u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020012C4
 | Description: Register-level C for the ARM routine at 020012C4; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020012C4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        psk_w32(a - 8u, c->r[11]);
        psk_w32(a - 0xCu, c->r[10]);
        psk_w32(a - 0x10u, c->r[9]);
        psk_w32(a - 0x14u, c->r[8]);
        psk_w32(a - 0x18u, c->r[4]);
        psk_w32(a - 0x1Cu, c->r[3]);
        psk_w32(a - 0x20u, c->r[2]);
        psk_w32(a - 0x24u, c->r[1]);
        psk_w32(a - 0x28u, c->r[0]);
        c->r[13] = a - 0x28u;
    }
    psk_nz(c, c->r[3] & 0x00000005u);
    if (psk_cond(c, PSK_NE)) {
        c->r[0] = c->r[0] + 0x00000010u;
    }
    if (psk_cond(c, PSK_EQ)) {
        c->r[0] = c->r[0] - 0x00000010u;
    }
    psk_nz(c, c->r[3] & 0x00000003u);
    if (psk_cond(c, PSK_NE)) {
        c->r[1] = c->r[1] + 0x00000008u;
    }
    if (psk_cond(c, PSK_EQ)) {
        c->r[1] = c->r[1] - 0x00000008u;
    }
    psk_nz(c, c->r[3] & 0x00000080u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[2] = c->r[2] + 0x00000001u;
    }
    if (psk_cond(c, PSK_NE)) {
        c->r[2] = c->r[2] - 0x00000001u;
    }
    c->r[2] = c->r[2] & 0x0000001Fu;
    psk_sub_flags(c, c->r[0], 0x00000020u);
    if (psk_cond(c, PSK_LT)) {
        psk_sub_flags(c, c->r[1], 0x00000020u);
    }
    if (psk_cond(c, PSK_GE)) {
        goto a_020013E0;
    }
    psk_add_flags(c, c->r[0], 0x00000020u);
    if (psk_cond(c, PSK_GT)) {
        psk_add_flags(c, c->r[1], 0x00000020u);
    }
    if (psk_cond(c, PSK_LE)) {
        goto a_020013E0;
    }
    c->r[11] = c->r[5] - 0x00000001u;
    c->r[11] = c->r[11] + (c->r[11] << 1u);
    c->r[8] = 0x00000000u;
a_02001314:
    c->r[9] = psk_ldrsb(c->r[7] + c->r[11]);
    psk_sub_flags(c, c->r[9], c->r[1]);
    if (psk_cond(c, PSK_LE)) {
        goto a_02001330;
    }
    c->r[8] = c->r[8] + 0x00000001u;
    psk_sub_flags(c, c->r[11], 0x00000000u);
    c->r[11] = c->r[11] - 0x00000003u;
    if (psk_cond(c, PSK_NE)) {
        goto a_02001314;
    }
a_02001330:
    psk_sub_flags(c, c->r[8], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001368;
    }
    c->r[10] = c->r[5] - 0x00000001u;
    c->r[10] = c->r[10] + (c->r[10] << 1u);
    c->r[10] = c->r[10] + c->r[7];
a_02001344:
    c->r[9] = psk_r8(c->r[10]);
    psk_w8(c->r[10] + 3u, (uint8_t)c->r[9]);
    c->r[9] = psk_r8(c->r[10] + 1u);
    psk_w8(c->r[10] + 4u, (uint8_t)c->r[9]);
    c->r[9] = psk_r8(c->r[10] + 2u);
    psk_w8(c->r[10] + 5u, (uint8_t)c->r[9]);
    c->r[10] = c->r[10] - 0x00000003u;
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001344;
    }
a_02001368:
    c->r[11] = c->r[11] + c->r[7];
    psk_w8(c->r[11] + 3u, (uint8_t)c->r[1]);
    psk_w8(c->r[11] + 4u, (uint8_t)c->r[0]);
    psk_w8(c->r[11] + 5u, (uint8_t)c->r[2]);
    c->r[5] = c->r[5] + 0x00000001u;
a_0200137C:
    psk_sub_flags(c, c->r[4], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200142C;
    }
    c->r[4] = c->r[4] - 0x00000001u;
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[2]);
        psk_w32(a - 8u, c->r[1]);
        psk_w32(a - 0xCu, c->r[0]);
        c->r[13] = a - 0xCu;
    }
    c->r[0] = c->r[2];
    psk_call(c, 0x020083E4u, 0x02001394u);
    c->r[11] = c->r[0];
    c->r[10] = c->r[0] & ~c->r[3];
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[2] = psk_r32(a + 8u);
        c->r[13] = a + 0xCu;
    }
    c->r[9] = c->r[3] & 0x00000080u;
    psk_sub_flags(c, c->r[11], 0x0000000Fu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020013F4;
    }
    psk_nz(c, c->r[10] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = c->r[9] | 0x00000008u;
    }
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x020012C4u, 0x020013B8u);
    }
    psk_nz(c, c->r[10] & 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = c->r[9] | 0x00000004u;
    }
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x020012C4u, 0x020013C4u);
    }
    psk_nz(c, c->r[10] & 0x00000004u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = c->r[9] | 0x00000002u;
    }
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x020012C4u, 0x020013D0u);
    }
    psk_nz(c, c->r[10] & 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = c->r[9] | 0x00000001u;
    }
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x020012C4u, 0x020013DCu);
    }
    goto a_0200142C;
a_020013E0:
    psk_sub_flags(c, c->r[2], 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        psk_sub_flags(c, c->r[2], 0x00000018u);
    }
    if (psk_cond(c, PSK_NE)) {
        goto a_0200137C;
    }
    c->r[10] = 0x0000000Fu;
    c->r[10] = c->r[10] & ~c->r[3];
a_020013F4:
    c->r[2] = 0x00000008u;
    psk_nz(c, c->r[10] & 0x00000004u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = 0x00000082u;
    }
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x020012C4u, 0x02001404u);
    }
    psk_nz(c, c->r[10] & 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = 0x00000004u;
    }
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x020012C4u, 0x02001410u);
    }
    c->r[2] = 0x00000018u;
    psk_nz(c, c->r[10] & 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = 0x00000081u;
    }
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x020012C4u, 0x02001420u);
    }
    psk_nz(c, c->r[10] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        c->r[3] = 0x00000008u;
    }
    if (psk_cond(c, PSK_NE)) {
        psk_call(c, 0x020012C4u, 0x0200142Cu);
    }
a_0200142C:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[2] = psk_r32(a + 8u);
        c->r[3] = psk_r32(a + 0xCu);
        c->r[4] = psk_r32(a + 0x10u);
        c->r[8] = psk_r32(a + 0x14u);
        c->r[9] = psk_r32(a + 0x18u);
        c->r[10] = psk_r32(a + 0x1Cu);
        c->r[11] = psk_r32(a + 0x20u);
        c->r[15] = psk_r32(a + 0x24u) & ~3u;
        c->r[13] = a + 0x28u;
        return;
    }
}

/*----------------------
 | rrl_func_02001430
 | Description: Register-level C for the ARM routine at 02001430; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001430(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        psk_w32(a - 8u, c->r[9]);
        psk_w32(a - 0xCu, c->r[8]);
        psk_w32(a - 0x10u, c->r[7]);
        psk_w32(a - 0x14u, c->r[1]);
        psk_w32(a - 0x18u, c->r[0]);
        c->r[13] = a - 0x18u;
    }
    c->r[3] = c->r[6] + (c->r[2] << 2u);
    c->r[7] = psk_r8(c->r[3] + 0x72u);
    c->r[2] = psk_r8(c->r[3] + 0x70u);
    psk_sub_flags(c, c->r[2], 0x00000008u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02001460;
    }
    psk_sub_flags(c, c->r[2], 0x00000016u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001460;
    }
    c->r[8] = psk_r8(c->r[3] + 0x71u);
    psk_nz(c, c->r[8] & 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        c->r[7] = c->r[7] + 0x00000002u;
    }
    goto a_02001464;
a_02001460:
    c->r[8] = 0x00000000u;
a_02001464:
    c->r[2] = c->r[2] + 0x0000001Cu;
    psk_call(c, 0x020082F4u, 0x0200146Cu);
    psk_nz(c, c->r[8] & 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        c->r[5] = c->r[5] | 0x00000004u;
    }
    c->r[8] = c->r[2] * c->r[3];
    c->r[8] = c->r[8] + 0x0000001Fu;
    c->r[8] = (c->r[8] >> 5u);
    c->r[8] = (c->r[8] << 3u);
    c->r[9] = c->r[8] * c->r[7];
    c->r[4] = c->r[4] + c->r[9];
    psk_call(c, 0x02002FE8u, 0x02001490u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[7] = psk_r32(a + 8u);
        c->r[8] = psk_r32(a + 0xCu);
        c->r[9] = psk_r32(a + 0x10u);
        c->r[15] = psk_r32(a + 0x14u) & ~3u;
        c->r[13] = a + 0x18u;
        return;
    }
}

/*----------------------
 | rrl_func_02001838
 | Description: Register-level C for the ARM routine at 02001838; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001838(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_r8(c->r[6] + 0x31u);
    c->r[8] = psk_r8(c->r[6] + 0x32u);
    c->r[0] = psk_r8(c->r[6] + 0x33u);
    c->r[1] = 0x00000014u;
    psk_call(c, 0x020083FCu, 0x02001850u);
    c->r[9] = c->r[1];
    psk_sub_flags(c, c->r[7], c->r[8]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02001864;
    }
    psk_sub_flags(c, c->r[9], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        goto a_02001878;
    }
a_02001864:
    c->r[2] = 0x00000038u;
    c->r[0] = 0x00000000u;
    c->r[1] = c->r[10];
    psk_call(c, 0x020082F4u, 0x02001874u);
    psk_call(c, 0x02002FE8u, 0x02001878u);
a_02001878:
    c->r[0] = 0x0000001Cu;
    c->r[1] = c->r[10] + 0x00000018u;
    c->r[2] = 0x00000035u;
    psk_call(c, 0x020082F4u, 0x02001888u);
    psk_sub_flags(c, c->r[8], c->r[7]);
    if (psk_cond(c, PSK_HS)) {
        goto a_020018B4;
    }
    psk_sub_flags(c, c->r[9], 0x0000000Au);
    if (psk_cond(c, PSK_HS)) {
        goto a_020018A0;
    }
    c->r[4] = c->r[4] + 0x00000028u;
    goto a_020018BC;
a_020018A0:
    c->r[7] = psk_r8(c->r[6] + 0x33u);
    psk_sub_flags(c, c->r[7], 0x0000001Eu);
    if (psk_cond(c, PSK_LO)) {
        goto a_020018B4;
    }
    c->r[7] = psk_r8(c->r[6] + 0x32u);
    goto a_020018B8;
a_020018B4:
    c->r[7] = psk_r8(c->r[6] + 0x31u);
a_020018B8:
    c->r[4] = c->r[4] + (c->r[7] << 2u);
a_020018BC:
    c->r[5] = c->r[5] | 0x00000002u;
    psk_call(c, 0x02002FE8u, 0x020018C4u);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02001CA4
 | Description: Register-level C for the Thumb routine at 02001CA4; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001CA4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7] + 5u);
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000010u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001CC2;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001CC2;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x02003CA3u, 0x02001CBFu);
    c->r[0] = psk_nz(c, 0x00000001u);
    goto a_02001CD4;
a_02001CC2:
    c->r[2] = psk_nz(c, 0x00000008u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001CEE;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001CEE;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x02003CA3u, 0x02001CD3u);
    c->r[0] = psk_nz(c, 0x00000000u);
a_02001CD4:
    psk_w8(c->r[7] + 5u, (uint8_t)c->r[0]);
    c->r[1] = psk_r8(c->r[7] + 4u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[2] = psk_sub_flags(c, c->r[2], c->r[1]);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 1u));
    c->r[2] = psk_add_flags(c, c->r[2], c->r[0]);
    c->r[7] = psk_nz(c, 0x00000070u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    psk_w8(c->r[7] + 1u, (uint8_t)c->r[2]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
    goto a_02001CFE;
a_02001CEE:
    c->r[1] = psk_r8(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001CFE;
    }
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[0] = psk_nz(c, c->r[0] ^ c->r[2]);
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x0000000Cu);
a_02001CFE:
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
 | rrl_func_020016F8
 | Description: Register-level C for the ARM routine at 020016F8; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x240
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020016F8(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = 0x00000240u;
    c->r[2] = ~0x00000000u;
    psk_sub_flags(c, c->r[3], 0x00000010u);
    if (psk_cond(c, PSK_GE)) {
        goto a_02001724;
    }
    c->r[1] = psk_add_flags(c, c->r[3], 0x00000009u);
    if (psk_cond(c, PSK_MI)) {
        goto a_020017DC;
    }
    psk_w32(c->r[0] + ((c->r[1] << 2u)), c->r[2]);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020017DC;
    }
    goto a_02001734;
a_02001724:
    c->r[0] = c->r[0] + (c->r[3] << 2u);
    {
        uint32_t a = c->r[0];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[2]);
        c->r[0] = b;
    }
    c->r[1] = c->r[3] - 0x00000001u;
    c->r[3] = c->r[3] + 0x00000001u;
a_02001734:
    c->r[2] = 0x00000000u;
    c->r[4] = c->r[1];
a_0200173C:
    {
        uint32_t a = c->r[0];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[2]);
        c->r[0] = b;
    }
    c->r[4] = psk_sub_flags(c, c->r[4], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200173C;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[3]);
        psk_w32(a - 8u, c->r[1]);
        c->r[13] = a - 8u;
    }
    c->r[0] = (c->r[7] >> 6u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001760;
    }
    c->r[0] = (c->r[8] << 3u);
    c->r[0] = c->r[0] + (c->r[8] << 1u);
a_02001760:
    c->r[0] = c->r[0] - 0x00000001u;
    c->r[1] = 0x0000000Au;
    psk_call(c, 0x020083FCu, 0x0200176Cu);
    c->r[10] = c->r[0];
    c->r[0] = 0x00000000u - c->r[1];
    c->r[1] = c->r[3];
    c->r[2] = 0x0000000Au;
    c->r[3] = 0x00000009u;
    c->r[5] = 0x00000000u;
a_02001784:
    c->r[4] = 0x02006520u;
    c->r[11] = psk_r8(c->r[9] + c->r[10]);
    c->r[4] = c->r[4] + (c->r[11] << 3u);
    c->r[4] = c->r[4] + (c->r[11] << 2u);
    psk_call(c, 0x02002FE8u, 0x02001798u);
    c->r[10] = c->r[10] + 0x00000001u;
    psk_sub_flags(c, c->r[10], c->r[8]);
    if (psk_cond(c, PSK_EQ)) {
        c->r[10] = 0x00000000u;
    }
    c->r[0] = c->r[0] + c->r[2];
    psk_sub_flags(c, c->r[0], 0x00000020u);
    if (psk_cond(c, PSK_LT)) {
        goto a_02001784;
    }
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[3] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[0] = 0x00000240u;
    psk_sub_flags(c, c->r[3], 0x00000010u);
    if (psk_cond(c, PSK_GE)) {
        c->r[0] = c->r[0] + (c->r[3] << 2u);
    }
    c->r[2] = 0x001F0000u;
    c->r[2] = c->r[2] | 0x0000F800u;
a_020017C8:
    c->r[3] = psk_ldr(c->r[0]);
    c->r[3] = c->r[3] ^ c->r[2];
    {
        uint32_t a = c->r[0];
        uint32_t b = a + 4u;
        psk_w32(a, c->r[3]);
        c->r[0] = b;
    }
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020017C8;
    }
a_020017DC:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_0200153C
 | Description: Register-level C for the ARM routine at 0200153C; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4, WRAM 0x544
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200153C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[6] = 0x000003C4u;
    c->r[0] = psk_r8(c->r[6] + 0x40u);
    psk_sub_flags(c, c->r[0], 0x00000008u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001580;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001568;
    }
    psk_sub_flags(c, c->r[0], 0x00000007u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001568;
    }
    c->r[3] = 0x00000000u;
    goto a_02001570;
a_02001568:
    c->r[3] = psk_r8(c->r[6] + 0x42u);
    c->r[3] = c->r[3] - 0x0000000Cu;
a_02001570:
    c->r[7] = psk_ldrh(c->r[6] + 0x4Cu);
    c->r[8] = psk_r8(c->r[6] + 0x45u);
    c->r[9] = 0x00000544u;
    psk_call(c, 0x020016F8u, 0x02001580u);
a_02001580:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02001D04
 | Description: Register-level C for the Thumb routine at 02001D04; it calls 4 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001D04(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7] + 0x11u);
    c->r[1] = psk_r8(c->r[7] + 0x12u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001D18;
    }
    psk_call(c, 0x0200265Du, 0x02001D17u);
    goto a_02001E3E;
a_02001D18:
    c->r[0] = psk_r8(c->r[7] + 0xBu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001DB2;
    }
    c->r[1] = psk_r8(c->r[7] + 0x15u);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001DB2;
    }
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[7] + 0x15u, (uint8_t)c->r[1]);
    c->r[0] = psk_r8(c->r[7] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001D82;
    }
    c->r[0] = psk_r8(c->r[7] + 2u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001D3E;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_w8(c->r[7] + 0x16u, (uint8_t)c->r[0]);
    goto a_02001DA2;
a_02001D3E:
    c->r[3] = psk_r8(c->r[7] + 1u);
    c->r[2] = psk_nz(c, 0x0000001Fu);
    c->r[2] = psk_nz(c, c->r[2] & c->r[3]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001D64;
    }
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5] + 4u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001D64;
    }
    c->r[1] = psk_r8(c->r[7] + 0x10u);
    c->r[0] = psk_r8(c->r[7] + 0x11u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001D64;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x13u, (uint8_t)c->r[0]);
a_02001D64:
    c->r[2] = psk_r8(c->r[7]);
    c->r[2] = psk_add_flags(c, c->r[2], 0x00000001u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 5u));
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000001u);
    psk_w8(c->r[7] + 1u, (uint8_t)c->r[3]);
    psk_sub_flags(c, c->r[3], c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001D7C;
    }
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[7] + 0x16u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x15u, (uint8_t)c->r[0]);
    goto a_02001DA2;
a_02001D7C:
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[7] + 0x16u, (uint8_t)c->r[0]);
    goto a_02001DA2;
a_02001D82:
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x16u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[7] + 2u);
    c->r[1] = psk_r8(c->r[7] + 1u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001D9C;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[0]);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001DA2;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x15u, (uint8_t)c->r[0]);
    goto a_02001DA2;
a_02001D9C:
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x15u, (uint8_t)c->r[0]);
    goto a_02001DB2;
a_02001DA2:
    c->r[1] = psk_r8(c->r[7] + 0x14u);
    c->r[0] = psk_r8(c->r[7] + 5u);
    c->r[0] = psk_nz(c, c->r[0] ^ c->r[1]);
    psk_w8(c->r[7] + 0xDu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[7] + 0xBu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0xCu, (uint8_t)c->r[0]);
a_02001DB2:
    psk_call(c, 0x02002695u, 0x02001DB7u);
    c->r[0] = psk_r8(c->r[7] + 0xBu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001DC2;
    }
    c->r[0] = psk_r8(c->r[7] + 0xCu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001E3A;
    }
a_02001DC2:
    c->r[0] = psk_r8(c->r[7] + 0x16u);
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001E3A;
    }
    c->r[0] = psk_r8(c->r[7] + 0x15u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001DD4;
    }
    c->r[0] = psk_r8(c->r[7] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001DF6;
    }
a_02001DD4:
    c->r[0] = psk_r8(c->r[7] + 5u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001DF2;
    }
    c->r[0] = psk_r8(c->r[7] + 6u);
    psk_call(c, 0x02004235u, 0x02001DE1u);
    psk_sub_flags(c, c->r[0], 0x0000000Fu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001DEA;
    }
    c->r[1] = psk_r8(c->r[7] + 7u);
    c->r[0] = psk_nz(c, c->r[0] & ~c->r[1]);
    goto a_02001E1A;
a_02001DEA:
    c->r[0] = psk_r8(c->r[7] + 7u);
    psk_call(c, 0x0200424Du, 0x02001DF1u);
    goto a_02001E1A;
a_02001DF2:
    c->r[0] = psk_r8(c->r[7] + 7u);
    goto a_02001E1A;
a_02001DF6:
    c->r[0] = psk_r8(c->r[7] + 5u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001E14;
    }
    c->r[0] = psk_r8(c->r[7] + 6u);
    psk_call(c, 0x02004235u, 0x02001E03u);
    psk_sub_flags(c, c->r[0], 0x0000000Fu);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001E10;
    }
    c->r[1] = psk_r8(c->r[7] + 7u);
    c->r[0] = psk_nz(c, c->r[0] & ~c->r[1]);
    psk_call(c, 0x0200424Du, 0x02001E0Fu);
    goto a_02001E1A;
a_02001E10:
    c->r[0] = psk_r8(c->r[7] + 7u);
    goto a_02001E1A;
a_02001E14:
    c->r[0] = psk_r8(c->r[7] + 7u);
    psk_call(c, 0x0200424Du, 0x02001E1Bu);
a_02001E1A:
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 1u));
    if (psk_cond(c, PSK_HS)) {
        goto a_02001E32;
    }
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 1u));
    if (psk_cond(c, PSK_HS)) {
        goto a_02001E2E;
    }
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 1u));
    if (psk_cond(c, PSK_HS)) {
        goto a_02001E2A;
    }
    c->r[0] = psk_nz(c, 0x00000002u);
    goto a_02001E34;
a_02001E2A:
    c->r[0] = psk_nz(c, 0x00000000u);
    goto a_02001E34;
a_02001E2E:
    c->r[0] = psk_nz(c, 0x00000003u);
    goto a_02001E34;
a_02001E32:
    c->r[0] = psk_nz(c, 0x00000001u);
a_02001E34:
    c->r[5] = psk_nz(c, 0x00000070u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
a_02001E3A:
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
a_02001E3E:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02001F28
 | Description: Register-level C for the Thumb routine at 02001F28; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001F28(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[3] = psk_r8(c->r[6] + 0x12u);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F34;
    }
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[3]);
a_02001F34:
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000001u);
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F4C;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F8E;
    }
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001FD4;
    }
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001FD4;
    }
    goto a_02001FD0;
a_02001F4C:
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F70;
    }
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001FD4;
    }
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001FD4;
    }
    c->r[2] = psk_nz(c, 0x00000000u);
    c->r[0] = psk_r8(c->r[6] + 0x16u);
    psk_sub_flags(c, c->r[0], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001F64;
    }
    c->r[2] = psk_nz(c, 0x00000068u);
a_02001F64:
    psk_w8(c->r[6] + 0x18u, (uint8_t)c->r[2]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000014u);
    psk_w8(c->r[6] + 0x19u, (uint8_t)c->r[0]);
    goto a_02001FD4;
a_02001F70:
    c->r[4] = psk_r8(c->r[6] + 0x18u);
    psk_sub_flags(c, c->r[4], 0x00000068u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F7A;
    }
    c->r[4] = psk_add_flags(c, c->r[4], 0x00000001u);
    psk_w8(c->r[6] + 0x18u, (uint8_t)c->r[4]);
a_02001F7A:
    c->r[0] = psk_r8(c->r[6] + 0x19u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001F82;
    }
    c->r[0] = psk_nz(c, 0x00000014u);
a_02001F82:
    psk_w8(c->r[6] + 0x19u, (uint8_t)c->r[0]);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001FD4;
    }
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001FD4;
    }
    goto a_02001FD0;
a_02001F8E:
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001FA6;
    }
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001FD4;
    }
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001FD4;
    }
    c->r[0] = psk_nz(c, 0x00000028u);
    psk_w8(c->r[6] + 0x19u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    goto a_02001FD4;
a_02001FA6:
    c->r[3] = psk_r8(c->r[6] + 0x19u);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001FCC;
    }
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    psk_w8(c->r[6] + 0x19u, (uint8_t)c->r[3]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001FD4;
    }
    c->r[7] = psk_nz(c, 0x00000040u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[3] = psk_r8(c->r[7] + 4u);
    psk_sub_flags(c, c->r[3], 0x00000009u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001FBE;
    }
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000001u);
a_02001FBE:
    psk_w8(c->r[7] + 4u, (uint8_t)c->r[3]);
    psk_call(c, 0x020044B7u, 0x02001FC5u);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_call(c, 0x02003CA3u, 0x02001FCBu);
    goto a_02001FD4;
a_02001FCC:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001FD4;
    }
a_02001FD0:
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
a_02001FD4:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02001494
 | Description: Register-level C for the ARM routine at 02001494; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001494(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[3]);
        psk_w32(a - 8u, c->r[2]);
        psk_w32(a - 0xCu, c->r[1]);
        psk_w32(a - 0x10u, c->r[0]);
        c->r[13] = a - 0x10u;
    }
    c->r[2] = c->r[2] + 0x00000039u;
    psk_call(c, 0x020082F4u, 0x020014A4u);
    psk_call(c, 0x02002FE8u, 0x020014A8u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[2] = psk_r32(a + 8u);
        c->r[8] = psk_r32(a + 0xCu);
        c->r[13] = a + 0x10u;
    }
    psk_sub_flags(c, c->r[8], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001520;
    }
    psk_sub_flags(c, c->r[8], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020014CC;
    }
    c->r[2] = c->r[2] + 0x0000003Du;
    psk_call(c, 0x020082F4u, 0x020014C4u);
    psk_call(c, 0x02002FE8u, 0x020014C8u);
    goto a_02001538;
a_020014CC:
    c->r[2] = 0x00000065u;
    psk_call(c, 0x020082F4u, 0x020014D4u);
    c->r[7] = psk_r8(c->r[6] + 0x1Fu);
    psk_sub_flags(c, c->r[7], 0x00000008u);
    if (psk_cond(c, PSK_HS)) {
        c->r[4] = c->r[4] + 0x00000020u;
    }
    c->r[7] = 0x0200A7A4u;
    psk_sub_flags(c, c->r[8], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[7] = c->r[7] + 0x00000002u;
    }
    psk_sub_flags(c, c->r[8], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        c->r[7] = c->r[7] + 0x00000006u;
    }
a_020014F4:
    {
        uint32_t a = c->r[7];
        uint32_t b = a + 2u;
        c->r[9] = psk_ldrh(a);
        c->r[7] = b;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[10] = (c->r[9] << 24u);
    c->r[0] = c->r[0] + psk_asr(c->r[10], 24u);
    c->r[9] = (c->r[9] << 16u);
    c->r[1] = c->r[1] + psk_asr(c->r[9], 24u);
    psk_call(c, 0x02002FE8u, 0x02001510u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[8] = psk_sub_flags(c, c->r[8], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020014F4;
    }
    goto a_02001538;
a_02001520:
    c->r[2] = 0x00000041u;
    psk_call(c, 0x020082F4u, 0x02001528u);
    c->r[7] = psk_r8(c->r[6] + 0x1Fu);
    psk_sub_flags(c, c->r[7], 0x00000008u);
    if (psk_cond(c, PSK_HS)) {
        c->r[4] = c->r[4] + 0x00000050u;
    }
    psk_call(c, 0x02002FE8u, 0x02001538u);
a_02001538:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02001FD8
 | Description: Register-level C for the Thumb routine at 02001FD8; it calls 8 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001FD8(PskCpu *c) {
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
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200207E;
    }
    c->r[3] = psk_r8(c->r[6] + 0x12u);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001FF4;
    }
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[3]);
    goto a_020020FE;
a_02001FF4:
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200200E;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002032;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002040;
    }
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002054;
    }
    psk_sub_flags(c, c->r[0], 0x00000005u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02002072;
    }
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_020020FE;
a_0200200E:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020020FE;
    }
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x02002019u);
    psk_sub_flags(c, c->r[0], 0x00000050u);
    if (psk_cond(c, PSK_HS)) {
        goto a_020020A8;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000005u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    psk_call(c, 0x020039F5u, 0x02002029u);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x0000005Cu);
    psk_call(c, 0x02003A07u, 0x02002031u);
    goto a_020020FE;
a_02002032:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020020FE;
    }
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    psk_call(c, 0x02004397u, 0x0200203Fu);
    goto a_020020FE;
a_02002040:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020020FE;
    }
    c->r[0] = psk_r8(c->r[7] + 0x11u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x13u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_020020FE;
a_02002054:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020020FE;
    }
    c->r[0] = psk_nz(c, 0x00000030u);
    psk_call(c, 0x020042A1u, 0x0200205Fu);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000003u);
    c->r[5] = psk_nz(c, 0x00000020u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    psk_w8(c->r[5] + 0x15u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[5] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000007u);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_020020FE;
a_02002072:
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020020FE;
    }
    c->r[0] = psk_r8(c->r[6] + 0x16u);
    psk_call(c, 0x02002CFFu, 0x0200207Du);
    goto a_020020FE;
a_0200207E:
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_r8(c->r[5]);
    psk_sub_flags(c, c->r[0], 0x00000008u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020020FE;
    }
    c->r[0] = psk_r8(c->r[5] + 5u);
    c->r[1] = psk_ldrh(c->r[5] + 0xCu);
    c->r[2] = psk_nz(c, 0x00000018u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 4u));
    c->r[2] = psk_add_flags(c, c->r[2], c->r[6]);
    psk_call(c, 0x0200454Bu, 0x02002097u);
    c->r[1] = psk_r8(c->r[6] + 0x17u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020020BA;
    }
    c->r[1] = psk_r8(c->r[6] + 0x18u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020020BA;
    }
    c->r[1] = psk_r8(c->r[6] + 0x19u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020020BA;
    }
a_020020A8:
    c->r[0] = psk_nz(c, 0x00000005u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000005u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x02002BDDu, 0x020020B7u);
    psk_w8(c->r[6] + 0x16u, (uint8_t)c->r[0]);
    goto a_020020FE;
a_020020BA:
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x020020C1u);
    psk_sub_flags(c, c->r[0], 0x00000014u);
    if (psk_cond(c, PSK_LO)) {
        goto a_020020D2;
    }
    psk_sub_flags(c, c->r[0], 0x00000046u);
    if (psk_cond(c, PSK_LO)) {
        goto a_020020E8;
    }
    psk_call(c, 0x0200418Fu, 0x020020CDu);
    c->r[0] = psk_nz(c, 0x0000000Bu);
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    goto a_020020FE;
a_020020D2:
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000005u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    psk_call(c, 0x020039F5u, 0x020020DFu);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x0000005Du);
    psk_call(c, 0x02003A07u, 0x020020E7u);
    goto a_020020FE;
a_020020E8:
    c->r[0] = psk_nz(c, 0x00000004u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Au);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    psk_call(c, 0x020039F5u, 0x020020F5u);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x0000005Eu);
    psk_call(c, 0x02003A07u, 0x020020FDu);
    goto a_020020FE;
a_020020FE:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02001584
 | Description: Register-level C for the ARM routine at 02001584; it calls 4 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x544
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001584(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[2] = 0x0000004Au;
    psk_call(c, 0x020082DCu, 0x02001590u);
    psk_call(c, 0x02002FE8u, 0x02001594u);
    c->r[7] = 0x00000017u;
    c->r[8] = 0x00000005u;
a_0200159C:
    c->r[0] = c->r[8];
    c->r[1] = 0x00000016u;
    c->r[2] = 0x00000035u;
    psk_call(c, 0x020082F4u, 0x020015ACu);
    c->r[9] = psk_r8(c->r[6] + c->r[7]);
    c->r[4] = c->r[4] + (c->r[9] << 2u);
    c->r[5] = c->r[5] | 0x00000002u;
    psk_call(c, 0x02002FE8u, 0x020015BCu);
    c->r[8] = c->r[8] + 0x00000009u;
    c->r[7] = c->r[7] + 0x00000001u;
    psk_sub_flags(c, c->r[7], 0x0000001Au);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200159C;
    }
    c->r[3] = 0x00000000u;
    c->r[7] = psk_ldrh(c->r[6] + 0x4Cu);
    c->r[8] = psk_r8(c->r[6] + 0x45u);
    c->r[9] = 0x00000544u;
    psk_call(c, 0x020016F8u, 0x020015E0u);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020018C8
 | Description: Register-level C for the ARM routine at 020018C8; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020018C8(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = c->r[2];
    c->r[2] = 0x00000035u;
    psk_call(c, 0x020082F4u, 0x020018D8u);
    c->r[8] = c->r[4];
    c->r[9] = 0x00002710u;
    psk_sub_flags(c, c->r[7], c->r[9]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02001904;
    }
    psk_sub_flags(c, c->r[7], 0x000003E8u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02001928;
    }
    psk_sub_flags(c, c->r[7], 0x00000064u);
    if (psk_cond(c, PSK_HS)) {
        goto a_0200194C;
    }
    psk_sub_flags(c, c->r[7], 0x0000000Au);
    if (psk_cond(c, PSK_HS)) {
        goto a_02001970;
    }
    goto a_02001994;
a_02001904:
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = c->r[7];
    c->r[1] = c->r[9];
    psk_call(c, 0x020083FCu, 0x02001914u);
    c->r[7] = c->r[1];
    c->r[4] = c->r[8] + (c->r[0] << 2u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    psk_call(c, 0x02002FE8u, 0x02001924u);
    c->r[0] = c->r[0] + c->r[2];
a_02001928:
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = c->r[7];
    c->r[1] = 0x000003E8u;
    psk_call(c, 0x020083FCu, 0x02001938u);
    c->r[7] = c->r[1];
    c->r[4] = c->r[8] + (c->r[0] << 2u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    psk_call(c, 0x02002FE8u, 0x02001948u);
    c->r[0] = c->r[0] + c->r[2];
a_0200194C:
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = c->r[7];
    c->r[1] = 0x00000064u;
    psk_call(c, 0x020083FCu, 0x0200195Cu);
    c->r[7] = c->r[1];
    c->r[4] = c->r[8] + (c->r[0] << 2u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    psk_call(c, 0x02002FE8u, 0x0200196Cu);
    c->r[0] = c->r[0] + c->r[2];
a_02001970:
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = c->r[7];
    c->r[1] = 0x0000000Au;
    psk_call(c, 0x020083FCu, 0x02001980u);
    c->r[7] = c->r[1];
    c->r[4] = c->r[8] + (c->r[0] << 2u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    psk_call(c, 0x02002FE8u, 0x02001990u);
    c->r[0] = c->r[0] + c->r[2];
a_02001994:
    c->r[4] = c->r[8] + (c->r[7] << 2u);
    psk_call(c, 0x02002FE8u, 0x0200199Cu);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_020015E4
 | Description: Register-level C for the ARM routine at 020015E4; it calls 5 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x544, WRAM 0x554
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020015E4(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x02008384u, 0x020015ECu);
    c->r[0] = psk_r8(c->r[6] + 0x40u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_HS)) {
        goto a_0200162C;
    }
    c->r[1] = 0x00000004u;
    c->r[2] = 0x00000000u;
    c->r[0] = psk_r8(c->r[6] + 0x1Bu);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001618;
    }
    c->r[0] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x02001614u);
    goto a_020016AC;
a_02001618:
    c->r[0] = ~0x00000004u;
    psk_call(c, 0x02001430u, 0x02001620u);
    c->r[0] = 0x00000016u;
    c->r[1] = 0x00000010u;
    goto a_02001664;
a_0200162C:
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_HS)) {
        goto a_0200168C;
    }
    c->r[1] = ~0x00000003u;
    c->r[2] = 0x00000001u;
    c->r[0] = psk_r8(c->r[6] + 0x1Bu);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001654;
    }
    c->r[0] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x02001650u);
    goto a_020016AC;
a_02001654:
    c->r[0] = 0x00000005u;
    psk_call(c, 0x02001430u, 0x0200165Cu);
    c->r[0] = 0x00000000u;
    c->r[1] = 0x00000008u;
a_02001664:
    c->r[2] = 0x00000037u;
    psk_call(c, 0x020082F4u, 0x0200166Cu);
    c->r[4] = c->r[4] + 0x00000008u;
    psk_call(c, 0x02002FE8u, 0x02001674u);
    c->r[0] = c->r[0] + c->r[2];
    c->r[2] = 0x00000035u;
    psk_call(c, 0x020082F4u, 0x02001680u);
    c->r[4] = c->r[4] + 0x00000004u;
    psk_call(c, 0x02002FE8u, 0x02001688u);
    goto a_020016AC;
a_0200168C:
    c->r[0] = ~0x00000009u;
    c->r[1] = 0x00000000u;
    c->r[2] = 0x00000001u;
    psk_call(c, 0x02001430u, 0x0200169Cu);
    c->r[0] = 0x0000000Au;
    c->r[1] = 0x00000000u;
    c->r[2] = 0x00000000u;
    psk_call(c, 0x02001430u, 0x020016ACu);
a_020016AC:
    c->r[0] = psk_r8(c->r[6] + 0x40u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020016D4;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020016D4;
    }
    c->r[3] = 0x00000000u;
    c->r[7] = psk_ldrh(c->r[6] + 0x4Cu);
    c->r[8] = psk_r8(c->r[6] + 0x45u);
    c->r[9] = 0x00000544u;
    psk_call(c, 0x020016F8u, 0x020016D4u);
a_020016D4:
    c->r[0] = psk_r8(c->r[6] + 0x40u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_LO)) {
        goto a_020016F4;
    }
    c->r[3] = 0x00000016u;
    c->r[7] = psk_ldrh(c->r[6] + 0x4Eu);
    c->r[8] = psk_r8(c->r[6] + 0x46u);
    c->r[9] = 0x00000554u;
    psk_call(c, 0x020016F8u, 0x020016F4u);
a_020016F4:
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02001A3C
 | Description: Register-level C for the ARM routine at 02001A3C; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001A3C(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = c->r[2];
    c->r[2] = 0x00000036u;
    psk_call(c, 0x020082F4u, 0x02001A4Cu);
    c->r[8] = c->r[4];
    c->r[5] = c->r[5] | 0x00000002u;
    psk_sub_flags(c, c->r[7], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        goto a_02001A7C;
    }
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
    c->r[0] = c->r[7];
    c->r[1] = 0x0000000Au;
    psk_call(c, 0x020083FCu, 0x02001A6Cu);
    c->r[7] = c->r[1];
    c->r[4] = c->r[8] + (c->r[0] << 2u);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    psk_call(c, 0x02002FE8u, 0x02001A7Cu);
a_02001A7C:
    c->r[0] = c->r[0] + 0x00000004u;
    c->r[4] = c->r[8] + (c->r[7] << 2u);
    psk_call(c, 0x02002FE8u, 0x02001A88u);
    {
        uint32_t a = c->r[13];
        c->r[15] = psk_r32(a) & ~3u;
        c->r[13] = a + 4u;
        return;
    }
}

/*----------------------
 | rrl_func_02001E42
 | Description: Register-level C for the Thumb routine at 02001E42; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02001E42(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7] + 0x11u);
    c->r[1] = psk_r8(c->r[7] + 0x12u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001E56;
    }
    psk_call(c, 0x0200265Du, 0x02001E55u);
    goto a_02001F24;
a_02001E56:
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001E8E;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001EB4;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001EBE;
    }
    c->r[1] = psk_r8(c->r[7] + 0xCu);
    psk_sub_flags(c, c->r[1], 0x00000040u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001E6E;
    }
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[7] + 0xCu, (uint8_t)c->r[1]);
a_02001E6E:
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001E96;
    }
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001E8A;
    }
    c->r[0] = psk_r8(c->r[6] + 0x11u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001E86;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
a_02001E86:
    psk_w8(c->r[6] + 0x11u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000005u);
a_02001E8A:
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    goto a_02001F24;
a_02001E8E:
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02001EB0;
    }
    goto a_02001E9C;
a_02001E96:
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
a_02001E9C:
    c->r[5] = psk_nz(c, 0x00000010u);
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 4u));
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[1] = psk_ldr(c->r[5] + 4u);
    c->r[0] = psk_r8(c->r[7] + 0x18u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F1A;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 0x18u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000014u);
a_02001EB0:
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    goto a_02001F24;
a_02001EB4:
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F0A;
    }
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    goto a_02001F24;
a_02001EBE:
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x000000D0u);
    c->r[5] = psk_nz(c, 0x00000018u);
    c->r[0] = psk_ldrsh(c->r[6] + c->r[5]);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_GE)) {
        goto a_02001ECE;
    }
    c->r[0] = psk_nz(c, 0x00000060u);
a_02001ECE:
    psk_w16((c->r[6] + c->r[5]) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000048u);
    if (psk_cond(c, PSK_MI)) {
        goto a_02001F04;
    }
    c->r[1] = psk_nz(c, 0x00000007u);
    c->r[1] = psk_nz(c, c->r[1] & c->r[0]);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    if (psk_cond(c, PSK_HS)) {
        goto a_02001F04;
    }
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F04;
    }
    psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001EFC;
    }
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F00;
    }
    psk_sub_flags(c, c->r[1], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001EF8;
    }
    psk_sub_flags(c, c->r[1], 0x00000006u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001F00;
    }
    psk_sub_flags(c, c->r[1], 0x00000007u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02001EFC;
    }
    c->r[0] = psk_nz(c, 0x0000000Au);
    goto a_02001F06;
a_02001EF8:
    c->r[0] = psk_nz(c, 0x0000000Bu);
    goto a_02001F06;
a_02001EFC:
    c->r[0] = psk_nz(c, 0x00000005u);
    goto a_02001F06;
a_02001F00:
    c->r[0] = psk_nz(c, 0x00000008u);
    goto a_02001F06;
a_02001F04:
    c->r[0] = psk_nz(c, 0x00000000u);
a_02001F06:
    psk_w8(c->r[6] + 0x17u, (uint8_t)c->r[0]);
    goto a_02001F24;
a_02001F0A:
    c->r[5] = psk_nz(c, 0x00000070u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_r8(c->r[5] + 1u);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    psk_call(c, 0x0200448Bu, 0x02001F19u);
    goto a_02001F1E;
a_02001F1A:
    c->r[0] = psk_nz(c, 0x0000003Cu);
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
a_02001F1E:
    c->r[0] = psk_r8(c->r[6] + 0x14u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[0]);
a_02001F24:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}
