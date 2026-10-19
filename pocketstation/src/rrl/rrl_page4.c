/*----------------------
 | rrl_page4.c
 | Description: Ring Ring Land routines at 02004000-02004FFF as register-level C, until they are named and grouped by concern.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "rrl.h"

/*----------------------
 | rrl_func_02004310
 | Description: Register-level C for the Thumb routine at 02004310.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004310(PskCpu *c) {
    c->r[4] = psk_nz(c, 0x000000A8u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 16u));
    c->r[4] = psk_add_flags(c, c->r[4], c->r[0]);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 4u));
    c->r[3] = psk_nz(c, 0x00000004u);
    c->r[5] = psk_ldr(c->r[4] + 8u);
    c->r[5] = psk_nz(c, c->r[5] & ~c->r[3]);
    psk_w32(c->r[4] + 8u, c->r[5]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_0200433A
 | Description: Register-level C for the Thumb routine at 0200433A.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200433A(PskCpu *c) {
    c->r[4] = psk_nz(c, 0x000000A0u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 20u));
    c->r[5] = psk_nz(c, 0x00000080u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200434C;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200434A;
    }
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 5u));
a_0200434A:
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 1u));
a_0200434C:
    psk_w32(c->r[4] + 0xCu, c->r[5]);
    psk_w32(c->r[4] + 0x10u, c->r[5]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020042FE
 | Description: Register-level C for the Thumb routine at 020042FE.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020042FE(PskCpu *c) {
    c->r[4] = psk_nz(c, 0x000000A8u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 16u));
    c->r[4] = psk_add_flags(c, c->r[4], c->r[0]);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 4u));
    c->r[3] = psk_nz(c, 0x00000004u);
    c->r[5] = psk_ldr(c->r[4] + 8u);
    c->r[5] = psk_nz(c, c->r[5] | c->r[3]);
    psk_w32(c->r[4] + 8u, c->r[5]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020042EC
 | Description: Register-level C for the Thumb routine at 020042EC.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020042EC(PskCpu *c) {
    c->r[4] = psk_nz(c, 0x000000A8u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 16u));
    c->r[4] = psk_add_flags(c, c->r[4], c->r[0]);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 4u));
    psk_w32(c->r[4], c->r[1]);
    c->r[3] = psk_nz(c, psk_lsl_s(c, c->r[3], 2u));
    c->r[3] = psk_nz(c, c->r[3] | c->r[2]);
    psk_w32(c->r[4] + 8u, c->r[3]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02004322
 | Description: Register-level C for the Thumb routine at 02004322.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004322(PskCpu *c) {
    c->r[4] = psk_nz(c, 0x000000A0u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 20u));
    c->r[5] = psk_nz(c, 0x00000080u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004334;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004332;
    }
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 5u));
a_02004332:
    c->r[5] = psk_nz(c, psk_lsl_s(c, c->r[5], 1u));
a_02004334:
    psk_w32(c->r[4] + 0x10u, c->r[5]);
    psk_w32(c->r[4] + 8u, c->r[5]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020049A6
 | Description: Register-level C for the Thumb routine at 020049A6.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020049A6(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[7]);
        psk_w32(a - 8u, c->r[6]);
        psk_w32(a - 0xCu, c->r[5]);
        psk_w32(a - 0x10u, c->r[1]);
        c->r[13] = a - 0x10u;
    }
    c->r[3] = 0x02009E00u;
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 2u));
    c->r[4] = psk_ldr(c->r[3] + c->r[0]);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000004u);
    c->r[3] = psk_ldr(c->r[3] + c->r[0]);
    c->r[3] = psk_sub_flags(c, c->r[3], c->r[4]);
    c->r[0] = psk_add_flags(c, c->r[3], 0x00000000u);
    psk_sub_flags(c, c->r[2], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020049CA;
    }
a_020049BA:
    c->r[5] = psk_ldrh(c->r[4]);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    psk_w16(c->r[1] & ~1u, (uint16_t)c->r[5]);
    c->r[4] = psk_add_flags(c, c->r[4], 0x00000002u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000002u);
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020049BA;
    }
    goto a_020049EE;
a_020049CA:
    c->r[5] = psk_ldrh(c->r[4]);
    c->r[6] = psk_nz(c, 0x000000FFu);
    c->r[6] = psk_nz(c, c->r[6] & c->r[5]);
    c->r[6] = psk_add_flags(c, c->r[6], c->r[2]);
    psk_sub_flags(c, c->r[6], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        goto a_020049D8;
    }
    c->r[6] = psk_nz(c, 0x00000009u);
a_020049D8:
    psk_w8(c->r[1], (uint8_t)c->r[6]);
    c->r[5] = psk_nz(c, psk_lsr_s(c, c->r[5], 8u));
    c->r[5] = psk_add_flags(c, c->r[5], c->r[2]);
    psk_sub_flags(c, c->r[5], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        goto a_020049E4;
    }
    c->r[5] = psk_nz(c, 0x00000009u);
a_020049E4:
    psk_w8(c->r[1] + 1u, (uint8_t)c->r[5]);
    c->r[4] = psk_add_flags(c, c->r[4], 0x00000002u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000002u);
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020049CA;
    }
a_020049EE:
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[5] = psk_r32(a + 4u);
        c->r[6] = psk_r32(a + 8u);
        c->r[7] = psk_r32(a + 0xCu);
        c->r[13] = a + 0x10u;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02004436
 | Description: Register-level C for the Thumb routine at 02004436; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004436(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5], (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x00000018u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 4u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[6]);
    c->r[2] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x020049A7u, 0x02004457u);
    psk_w8(c->r[5] + 5u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[1] = psk_nz(c, 0x00000019u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 4u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[6]);
    c->r[2] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x020049A7u, 0x02004467u);
    c->r[0] = psk_nz(c, 0x00000002u);
    c->r[1] = psk_nz(c, 0x0000001Au);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 4u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[6]);
    c->r[2] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x020049A7u, 0x02004475u);
    c->r[0] = psk_nz(c, 0x00000020u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 6u));
    psk_w16((c->r[5] + 0xCu) & ~1u, (uint16_t)c->r[0]);
    psk_w16((c->r[5] + 0xEu) & ~1u, (uint16_t)c->r[0]);
    psk_w16((c->r[5] + 0x12u) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[5] + 8u) & ~1u, (uint16_t)c->r[0]);
    psk_w16((c->r[5] + 0xAu) & ~1u, (uint16_t)c->r[0]);
    psk_w16((c->r[5] + 0x10u) & ~1u, (uint16_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_0200486E
 | Description: Register-level C for the Thumb routine at 0200486E.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200486E(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000040u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004884;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020048CC;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020048DA;
    }
    goto a_0200491A;
a_02004884:
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    psk_sub_flags(c, c->r[0], 0x00000028u);
    if (psk_cond(c, PSK_HS)) {
        goto a_0200488E;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
a_0200488E:
    c->r[1] = psk_ldrh(c->r[7] + 8u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000004u);
    psk_sub_flags(c, c->r[1], 0x00000060u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004898;
    }
    c->r[1] = psk_nz(c, 0x00000060u);
a_02004898:
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[1]);
    psk_sub_flags(c, c->r[0], 0x00000014u);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200491A;
    }
    c->r[3] = psk_ldrh(c->r[7] + 0x10u);
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000005u);
    psk_sub_flags(c, c->r[3], 0x00000070u);
    if (psk_cond(c, PSK_LO)) {
        goto a_020048A8;
    }
    c->r[3] = psk_nz(c, 0x00000070u);
a_020048A8:
    psk_w16((c->r[7] + 0x10u) & ~1u, (uint16_t)c->r[3]);
    psk_sub_flags(c, c->r[0], 0x00000028u);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200491A;
    }
    c->r[2] = psk_ldrh(c->r[7] + 0xAu);
    c->r[2] = psk_add_flags(c, c->r[2], 0x00000004u);
    psk_sub_flags(c, c->r[2], 0x00000080u);
    if (psk_cond(c, PSK_HS)) {
        goto a_020048BA;
    }
    psk_w16((c->r[7] + 0xAu) & ~1u, (uint16_t)c->r[2]);
    goto a_0200491A;
a_020048BA:
    c->r[2] = psk_nz(c, 0x00000080u);
    psk_w16((c->r[7] + 0xAu) & ~1u, (uint16_t)c->r[2]);
    psk_sub_flags(c, c->r[1], 0x00000060u);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200491A;
    }
    psk_sub_flags(c, c->r[3], 0x00000070u);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200491A;
    }
    c->r[0] = psk_nz(c, 0x000000A0u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    goto a_02004914;
a_020048CC:
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200491A;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    goto a_02004914;
a_020048DA:
    c->r[0] = psk_r8(c->r[6] + 0x12u);
    psk_sub_flags(c, c->r[0], 0x00000054u);
    if (psk_cond(c, PSK_HS)) {
        goto a_020048E4;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
a_020048E4:
    c->r[1] = psk_ldrh(c->r[7] + 8u);
    psk_sub_flags(c, c->r[1], 0x00000038u);
    if (psk_cond(c, PSK_LS)) {
        goto a_020048EE;
    }
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000006u);
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[1]);
a_020048EE:
    psk_sub_flags(c, c->r[0], 0x0000003Cu);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200491A;
    }
    c->r[3] = psk_ldrh(c->r[7] + 0x10u);
    psk_sub_flags(c, c->r[3], 0x00000038u);
    if (psk_cond(c, PSK_LS)) {
        goto a_020048FC;
    }
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000005u);
    psk_w16((c->r[7] + 0x10u) & ~1u, (uint16_t)c->r[3]);
a_020048FC:
    psk_sub_flags(c, c->r[0], 0x00000054u);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200491A;
    }
    c->r[2] = psk_ldrh(c->r[7] + 0xAu);
    psk_sub_flags(c, c->r[2], 0x00000038u);
    if (psk_cond(c, PSK_LS)) {
        goto a_0200490C;
    }
    c->r[2] = psk_sub_flags(c, c->r[2], 0x00000004u);
    psk_w16((c->r[7] + 0xAu) & ~1u, (uint16_t)c->r[2]);
    goto a_0200491A;
a_0200490C:
    psk_sub_flags(c, c->r[1], 0x00000038u);
    if (psk_cond(c, PSK_HI)) {
        goto a_0200491A;
    }
    psk_sub_flags(c, c->r[3], 0x00000038u);
    if (psk_cond(c, PSK_HI)) {
        goto a_0200491A;
    }
a_02004914:
    c->r[0] = psk_r8(c->r[7]);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7], (uint8_t)c->r[0]);
a_0200491A:
    c->r[0] = psk_r8(c->r[7] + 5u);
    c->r[2] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 9u));
    c->r[3] = psk_ldrh(c->r[7] + 8u);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004942;
    }
    c->r[0] = psk_ldrh(c->r[7] + 0xCu);
    c->r[0] = psk_add_flags(c, c->r[0], c->r[3]);
    psk_sub_flags(c, c->r[0], c->r[2]);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004940;
    }
    c->r[1] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200493E;
    }
    psk_sub_flags(c, c->r[3], 0x00000038u);
    if (psk_cond(c, PSK_HI)) {
        goto a_0200493E;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[0]);
    goto a_02004940;
a_0200493E:
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[2]);
a_02004940:
    psk_w16((c->r[7] + 0xCu) & ~1u, (uint16_t)c->r[0]);
a_02004942:
    c->r[3] = psk_ldrh(c->r[7] + 0xAu);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004964;
    }
    c->r[0] = psk_ldrh(c->r[7] + 0xEu);
    c->r[0] = psk_add_flags(c, c->r[0], c->r[3]);
    psk_sub_flags(c, c->r[0], c->r[2]);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004962;
    }
    c->r[1] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004960;
    }
    psk_sub_flags(c, c->r[3], 0x00000038u);
    if (psk_cond(c, PSK_HI)) {
        goto a_02004960;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[7] + 0xAu) & ~1u, (uint16_t)c->r[0]);
    goto a_02004962;
a_02004960:
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[2]);
a_02004962:
    psk_w16((c->r[7] + 0xEu) & ~1u, (uint16_t)c->r[0]);
a_02004964:
    c->r[3] = psk_ldrh(c->r[7] + 0x10u);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004986;
    }
    c->r[0] = psk_ldrh(c->r[7] + 0x12u);
    c->r[0] = psk_add_flags(c, c->r[0], c->r[3]);
    psk_sub_flags(c, c->r[0], c->r[2]);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004984;
    }
    c->r[1] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004982;
    }
    psk_sub_flags(c, c->r[3], 0x00000038u);
    if (psk_cond(c, PSK_HI)) {
        goto a_02004982;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[7] + 0x10u) & ~1u, (uint16_t)c->r[0]);
    goto a_02004984;
a_02004982:
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[2]);
a_02004984:
    psk_w16((c->r[7] + 0x12u) & ~1u, (uint16_t)c->r[0]);
a_02004986:
    c->r[1] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[1], 0x00000003u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020049A2;
    }
    c->r[0] = psk_ldrh(c->r[7] + 8u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020049A2;
    }
    c->r[0] = psk_ldrh(c->r[7] + 0xAu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020049A2;
    }
    c->r[0] = psk_ldrh(c->r[7] + 0x10u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020049A2;
    }
    c->r[0] = psk_nz(c, 0x00000004u);
    psk_w8(c->r[7], (uint8_t)c->r[0]);
a_020049A2:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02004274
 | Description: Register-level C for the Thumb routine at 02004274.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004274(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[6]);
        psk_w32(a - 8u, c->r[5]);
        psk_w32(a - 0xCu, c->r[4]);
        psk_w32(a - 0x10u, c->r[3]);
        psk_w32(a - 0x14u, c->r[2]);
        c->r[13] = a - 0x14u;
    }
    c->r[4] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 16u));
    c->r[2] = psk_nz(c, 0x00000000u);
    c->r[3] = psk_nz(c, 0x00000010u);
    c->r[6] = psk_nz(c, 0x00000001u);
a_02004280:
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 1u));
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 1u));
    if (psk_cond(c, PSK_HS)) {
        goto a_0200428E;
    }
    c->r[5] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[5] = psk_nz(c, psk_lsr_s(c, c->r[5], 16u));
    psk_sub_flags(c, c->r[5], c->r[1]);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004292;
    }
a_0200428E:
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[4]);
    c->r[2] = psk_nz(c, c->r[2] | c->r[6]);
a_02004292:
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004280;
    }
    c->r[1] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[1] = psk_nz(c, psk_lsr_s(c, c->r[1], 16u));
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    {
        uint32_t a = c->r[13];
        c->r[2] = psk_r32(a);
        c->r[3] = psk_r32(a + 4u);
        c->r[4] = psk_r32(a + 8u);
        c->r[5] = psk_r32(a + 0xCu);
        c->r[6] = psk_r32(a + 0x10u);
        c->r[13] = a + 0x14u;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02004234
 | Description: Register-level C for the Thumb routine at 02004234.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004234(PskCpu *c) {
    c->r[2] = 0x02009DF0u;
    c->r[1] = psk_nz(c, 0x000000F8u);
    c->r[1] = psk_nz(c, c->r[1] & c->r[0]);
    c->r[1] = psk_nz(c, psk_lsr_s(c, c->r[1], 1u));
    c->r[2] = psk_ldr(c->r[2] + c->r[1]);
    c->r[1] = psk_nz(c, 0x00000007u);
    c->r[1] = psk_nz(c, c->r[1] & c->r[0]);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 2u));
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], c->r[1]));
    c->r[0] = psk_nz(c, 0x0000000Fu);
    c->r[0] = psk_nz(c, c->r[0] & c->r[2]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_0200424C
 | Description: Register-level C for the Thumb routine at 0200424C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200424C(PskCpu *c) {
    c->r[1] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[0] & c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004256;
    }
    c->r[0] = psk_nz(c, 0x00000008u);
    goto a_02004272;
a_02004256:
    c->r[1] = psk_nz(c, 0x00000002u);
    psk_nz(c, c->r[0] & c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004260;
    }
    c->r[0] = psk_nz(c, 0x00000004u);
    goto a_02004272;
a_02004260:
    c->r[1] = psk_nz(c, 0x00000004u);
    psk_nz(c, c->r[0] & c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200426A;
    }
    c->r[0] = psk_nz(c, 0x00000002u);
    goto a_02004272;
a_0200426A:
    c->r[1] = psk_nz(c, 0x00000008u);
    psk_nz(c, c->r[0] & c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004272;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
a_02004272:
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_0200454A
 | Description: Register-level C for the Thumb routine at 0200454A; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200454A(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[3] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[0] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 6u));
    c->r[1] = psk_nz(c, 0x0000000Au);
    psk_call(c, 0x02004275u, 0x02004559u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_sub_flags(c, c->r[0], c->r[3]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004560;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
a_02004560:
    c->r[0] = psk_r8(c->r[2] + c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[1]);
    return;
}

/*----------------------
 | rrl_random
 | Description: Steps the 16-bit random seed at 0x3C4 and returns a value below r0. Register-level C for the Thumb routine at 020042A0.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_random(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[3]);
        psk_w32(a - 8u, c->r[2]);
        psk_w32(a - 0xCu, c->r[1]);
        c->r[13] = a - 0xCu;
    }
    c->r[3] = 0x000003C4u;
    c->r[1] = 0x00003D09u;
    c->r[2] = psk_ldrh(c->r[3]);
    c->r[2] = psk_add_flags(c, c->r[2], 0x00000001u);
    c->r[2] = psk_nz(c, c->r[2] * c->r[1]);
    psk_w16(c->r[3] & ~1u, (uint16_t)c->r[2]);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 16u));
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 16u));
    c->r[0] = psk_nz(c, c->r[0] * c->r[2]);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 16u));
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[2] = psk_r32(a + 4u);
        c->r[3] = psk_r32(a + 8u);
        c->r[13] = a + 0xCu;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020045F0
 | Description: Register-level C for the Thumb routine at 020045F0; it calls 4 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x12C, WRAM 0x544
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020045F0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000040u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200461A;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004648;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200465E;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004688;
    }
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004692;
    }
    psk_sub_flags(c, c->r[0], 0x00000005u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020046C4;
    }
    psk_sub_flags(c, c->r[0], 0x00000006u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200469E;
    }
    psk_sub_flags(c, c->r[0], 0x00000007u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004626;
    }
    goto a_020046C4;
a_0200461A:
    c->r[1] = psk_r8(c->r[7] + 2u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[1]);
    psk_sub_flags(c, c->r[1], 0x0000000Cu);
    if (psk_cond(c, PSK_NE)) {
        goto a_020046C4;
    }
    goto a_020046C0;
a_02004626:
    c->r[1] = psk_r8(c->r[7] + 2u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[1]);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020046C4;
    }
    c->r[0] = psk_r8(c->r[7] + 5u);
    c->r[1] = psk_ldrh(c->r[7] + 0xCu);
    c->r[2] = 0x00000544u;
    psk_call(c, 0x0200454Bu, 0x0200463Bu);
    c->r[5] = psk_nz(c, 0x00000020u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    psk_w8(c->r[5] + 0x15u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5] + 0x14u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000007u);
    goto a_020046C0;
a_02004648:
    c->r[1] = psk_ldrh(c->r[7] + 8u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000004u);
    psk_sub_flags(c, c->r[1], 0x00000080u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004654;
    }
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[1]);
    goto a_020046C4;
a_02004654:
    c->r[1] = psk_nz(c, 0x00000080u);
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[1]);
    c->r[1] = 0x0000012Cu;
    psk_w16((c->r[6] + 0x12u) & ~1u, (uint16_t)c->r[1]);
    goto a_020046C0;
a_0200465E:
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004670;
    }
    c->r[1] = psk_ldrh(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w16((c->r[6] + 0x12u) & ~1u, (uint16_t)c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020046C4;
    }
    goto a_02004676;
a_02004670:
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_call(c, 0x02003D13u, 0x02004677u);
a_02004676:
    c->r[0] = psk_nz(c, 0x0000001Eu);
    psk_call(c, 0x020042A1u, 0x0200467Du);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x0000000Fu);
    if (psk_cond(c, PSK_HI)) {
        goto a_02004682;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
a_02004682:
    psk_w8(c->r[7] + 3u, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[7]);
    goto a_020046C0;
a_02004688:
    c->r[1] = psk_r8(c->r[7] + 3u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[7] + 3u, (uint8_t)c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020046C4;
    }
    goto a_020046C0;
a_02004692:
    c->r[1] = psk_ldrh(c->r[7] + 8u);
    psk_sub_flags(c, c->r[1], 0x00000030u);
    if (psk_cond(c, PSK_LS)) {
        goto a_020046C0;
    }
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000006u);
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[1]);
    goto a_020046C4;
a_0200469E:
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020046AE;
    }
    c->r[1] = psk_r8(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020046C4;
    }
a_020046AE:
    c->r[1] = psk_r8(c->r[7] + 1u);
    psk_sub_flags(c, c->r[1], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020046B8;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    goto a_020046C0;
a_020046B8:
    c->r[5] = psk_nz(c, 0x00000020u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[1] = psk_nz(c, 0x00000003u);
    psk_w8(c->r[5] + 0xDu, (uint8_t)c->r[1]);
a_020046C0:
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7], (uint8_t)c->r[0]);
a_020046C4:
    c->r[3] = psk_ldrh(c->r[7] + 8u);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004702;
    }
    c->r[0] = psk_r8(c->r[7] + 5u);
    c->r[2] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 3u));
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 1u));
    c->r[2] = psk_add_flags(c, c->r[2], c->r[0]);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 6u));
    c->r[0] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[0], 0x00000005u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020046F8;
    }
    c->r[0] = psk_ldrh(c->r[7] + 0xCu);
    c->r[1] = psk_nz(c, 0x0000000Au);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 6u));
    psk_call(c, 0x02004275u, 0x020046E7u);
    psk_sub_flags(c, c->r[1], c->r[3]);
    if (psk_cond(c, PSK_HI)) {
        goto a_020046F8;
    }
    c->r[0] = psk_nz(c, 0x00000006u);
    psk_w8(c->r[7], (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000005Au);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
    c->r[3] = psk_add_flags(c, c->r[1], 0x00000000u);
a_020046F8:
    c->r[0] = psk_ldrh(c->r[7] + 0xCu);
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[3]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004700;
    }
    c->r[0] = psk_add_flags(c, c->r[0], c->r[2]);
a_02004700:
    psk_w16((c->r[7] + 0xCu) & ~1u, (uint16_t)c->r[0]);
a_02004702:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020044EA
 | Description: Register-level C for the Thumb routine at 020044EA.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020044EA(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        psk_w32(a - 8u, c->r[0]);
        c->r[13] = a - 8u;
    }
a_020044EC:
    c->r[4] = psk_r8(c->r[3]);
    c->r[4] = psk_add_flags(c, c->r[4], c->r[2]);
    psk_sub_flags(c, c->r[4], 0x0000000Au);
    if (psk_cond(c, PSK_LO)) {
        goto a_020044F6;
    }
    c->r[4] = psk_nz(c, 0x00000009u);
a_020044F6:
    psk_w8(c->r[1], (uint8_t)c->r[4]);
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000001u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020044EC;
    }
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020044B6
 | Description: Unverified (a random board event no script rolls). Register-level C for the Thumb routine at 020044B6; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020044B6(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5], (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000005u);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x00000018u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 4u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[6]);
    c->r[3] = psk_nz(c, 0x0000001Au);
    c->r[3] = psk_nz(c, psk_lsl_s(c, c->r[3], 4u));
    c->r[3] = psk_add_flags(c, c->r[3], c->r[6]);
    c->r[2] = psk_r8(c->r[5] + 4u);
    c->r[0] = psk_r8(c->r[5] + 7u);
    psk_w8(c->r[5] + 5u, (uint8_t)c->r[0]);
    psk_call(c, 0x020044EBu, 0x020044DFu);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[5] + 0xCu) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[5] + 8u) & ~1u, (uint16_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02004532
 | Description: Register-level C for the Thumb routine at 02004532; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004532(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x0200453Bu);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[1] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 3u));
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 1u));
    c->r[0] = psk_add_flags(c, c->r[0], c->r[1]);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 6u));
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[1]);
    return;
}

/*----------------------
 | rrl_func_02004566
 | Description: Register-level C for the Thumb routine at 02004566; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004566(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        psk_w32(a - 8u, c->r[5]);
        c->r[13] = a - 8u;
    }
    c->r[7] = psk_nz(c, 0x00000017u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[2] = psk_nz(c, 0x00000000u);
a_0200456E:
    c->r[4] = psk_nz(c, 0x00000001u);
    c->r[3] = psk_r8(c->r[1]);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200457C;
    }
a_02004576:
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], 1u));
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004576;
    }
a_0200457C:
    c->r[2] = psk_nz(c, c->r[2] | c->r[4]);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200456E;
    }
    c->r[4] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[3] = psk_nz(c, 0x00000000u);
    c->r[1] = psk_nz(c, 0x0000000Au);
a_0200458A:
    c->r[4] = psk_nz(c, psk_lsr_s(c, c->r[4], 1u));
    if (psk_cond(c, PSK_LO)) {
        goto a_02004590;
    }
    c->r[3] = psk_add_flags(c, c->r[3], 0x00000001u);
a_02004590:
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200458A;
    }
    c->r[1] = psk_nz(c, 0x00000000u);
a_02004596:
    c->r[0] = psk_add_flags(c, c->r[3], 0x00000000u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x0200459Fu);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[4] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[5] = psk_nz(c, 0x00000000u);
a_020045A4:
    c->r[4] = psk_nz(c, psk_lsr_s(c, c->r[4], 1u));
    if (psk_cond(c, PSK_LO)) {
        goto a_020045AE;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020045B2;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
a_020045AE:
    c->r[5] = psk_add_flags(c, c->r[5], 0x00000001u);
    goto a_020045A4;
a_020045B2:
    psk_w8(c->r[7] + c->r[1], (uint8_t)c->r[5]);
    c->r[4] = psk_nz(c, 0x00000001u);
    c->r[4] = psk_nz(c, psk_lsl_s(c, c->r[4], c->r[5]));
    c->r[2] = psk_nz(c, c->r[2] & ~c->r[4]);
    c->r[3] = psk_sub_flags(c, c->r[3], 0x00000001u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    psk_sub_flags(c, c->r[1], 0x00000003u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004596;
    }
    c->r[0] = psk_r8(c->r[7]);
    c->r[1] = psk_r8(c->r[7] + 1u);
    c->r[2] = psk_r8(c->r[7] + 2u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_LO)) {
        goto a_020045D2;
    }
    c->r[3] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[0] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[1] = psk_add_flags(c, c->r[3], 0x00000000u);
a_020045D2:
    psk_sub_flags(c, c->r[0], c->r[2]);
    if (psk_cond(c, PSK_LO)) {
        goto a_020045DC;
    }
    c->r[3] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[2] = psk_add_flags(c, c->r[3], 0x00000000u);
a_020045DC:
    psk_sub_flags(c, c->r[1], c->r[2]);
    if (psk_cond(c, PSK_LO)) {
        goto a_020045E6;
    }
    c->r[3] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[1] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[2] = psk_add_flags(c, c->r[3], 0x00000000u);
a_020045E6:
    psk_w8(c->r[7], (uint8_t)c->r[0]);
    psk_w8(c->r[7] + 1u, (uint8_t)c->r[1]);
    psk_w8(c->r[7] + 2u, (uint8_t)c->r[2]);
    {
        uint32_t a = c->r[13];
        c->r[5] = psk_r32(a);
        c->r[7] = psk_r32(a + 4u);
        c->r[13] = a + 8u;
    }
    c->r[15] = psk_bx(c->r[7]);
    return;
}

/*----------------------
 | rrl_func_02004396
 | Description: Register-level C for the Thumb routine at 02004396; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004396(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[5], (uint8_t)c->r[0]);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x00000018u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 4u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[6]);
    c->r[3] = psk_nz(c, 0x0000001Au);
    c->r[3] = psk_nz(c, psk_lsl_s(c, c->r[3], 4u));
    c->r[3] = psk_add_flags(c, c->r[3], c->r[6]);
    c->r[2] = psk_r8(c->r[5] + 4u);
    c->r[0] = psk_r8(c->r[5] + 7u);
    psk_w8(c->r[5] + 5u, (uint8_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[1]);
        c->r[13] = a - 4u;
    }
    psk_call(c, 0x020044EBu, 0x020043BFu);
    psk_call(c, 0x02004533u, 0x020043C3u);
    psk_w16((c->r[5] + 0xCu) & ~1u, (uint16_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[0] = psk_r8(c->r[5] + 5u);
    psk_call(c, 0x02004567u, 0x020043CDu);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[5] + 8u) & ~1u, (uint16_t)c->r[0]);
    psk_w8(c->r[5] + 6u, (uint8_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_0200418E
 | Description: Unverified (a random board event no script rolls). Register-level C for the Thumb routine at 0200418E; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200418E(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[4] = 0x02009F0Cu;
    c->r[0] = psk_r8(c->r[6] + 0x15u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000007u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 2u));
    c->r[4] = psk_ldr(c->r[4] + c->r[0]);
    c->r[0] = psk_nz(c, 0x00000064u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 4u));
    psk_call(c, 0x020042A1u, 0x020041A3u);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[2] = psk_nz(c, 0x000000FFu);
a_020041A6:
    c->r[1] = psk_ldrh(c->r[4]);
    c->r[4] = psk_add_flags(c, c->r[4], 0x00000002u);
    c->r[3] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[3] = psk_nz(c, c->r[3] & c->r[2]);
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[3]);
    if (psk_cond(c, PSK_HS)) {
        goto a_020041A6;
    }
    c->r[1] = psk_nz(c, psk_lsr_s(c, c->r[1], 8u));
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[1]);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[1]);
    return;
}

/*----------------------
 | rrl_func_02004504
 | Description: Register-level C for the Thumb routine at 02004504; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004504(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        psk_w32(a - 8u, c->r[5]);
        psk_w32(a - 0xCu, c->r[1]);
        psk_w32(a - 0x10u, c->r[0]);
        c->r[13] = a - 0x10u;
    }
    c->r[2] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 4u));
    c->r[7] = psk_nz(c, 0x00000008u);
a_0200450C:
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    psk_call(c, 0x020042A1u, 0x02004513u);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    c->r[3] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[0] = psk_add_flags(c, c->r[2], 0x00000000u);
    psk_call(c, 0x020042A1u, 0x0200451Du);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    psk_sub_flags(c, c->r[0], c->r[3]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200452A;
    }
    c->r[4] = psk_r8(c->r[1] + c->r[0]);
    c->r[5] = psk_r8(c->r[1] + c->r[3]);
    psk_w8(c->r[1] + c->r[0], (uint8_t)c->r[5]);
    psk_w8(c->r[1] + c->r[3], (uint8_t)c->r[4]);
a_0200452A:
    c->r[7] = psk_sub_flags(c, c->r[7], 0x00000001u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200450C;
    }
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[1] = psk_r32(a + 4u);
        c->r[5] = psk_r32(a + 8u);
        c->r[7] = psk_r32(a + 0xCu);
        c->r[13] = a + 0x10u;
    }
    c->r[15] = psk_bx(c->r[7]);
    return;
}

/*----------------------
 | rrl_func_020043D6
 | Description: Register-level C for the Thumb routine at 020043D6; it calls 4 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020043D6(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5], (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    c->r[1] = psk_r8(c->r[6] + 0x1Bu);
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_NE)) {
        goto a_020043F0;
    }
    c->r[1] = psk_nz(c, 0x00000000u);
a_020043F0:
    c->r[2] = psk_r8(c->r[5] + 4u);
    c->r[2] = psk_add_flags(c, c->r[2], c->r[1]);
    c->r[1] = psk_nz(c, 0x00000018u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 4u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[6]);
    c->r[3] = psk_nz(c, 0x0000001Au);
    c->r[3] = psk_nz(c, psk_lsl_s(c, c->r[3], 4u));
    c->r[3] = psk_add_flags(c, c->r[3], c->r[6]);
    c->r[0] = psk_r8(c->r[5] + 7u);
    psk_w8(c->r[5] + 5u, (uint8_t)c->r[0]);
    psk_call(c, 0x020044EBu, 0x02004409u);
    psk_call(c, 0x02004533u, 0x0200440Du);
    psk_w16((c->r[5] + 0xCu) & ~1u, (uint16_t)c->r[0]);
    c->r[2] = psk_r8(c->r[6] + 0x1Bu);
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 1u));
    c->r[1] = psk_nz(c, 0x00000019u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 4u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[6]);
    c->r[0] = psk_r8(c->r[6] + 0x19u);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000008u);
    psk_call(c, 0x020049A7u, 0x02004421u);
    psk_w8(c->r[5] + 6u, (uint8_t)c->r[0]);
    psk_call(c, 0x02004505u, 0x02004427u);
    psk_call(c, 0x02004533u, 0x0200442Bu);
    psk_w16((c->r[5] + 0xEu) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[5] + 8u) & ~1u, (uint16_t)c->r[0]);
    psk_w16((c->r[5] + 0xAu) & ~1u, (uint16_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020041FE
 | Description: Register-level C for the Thumb routine at 020041FE.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x3E8
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020041FE(PskCpu *c) {
    c->r[7] = 0x00002710u;
    psk_sub_flags(c, c->r[2], c->r[7]);
    if (psk_cond(c, PSK_HS)) {
        goto a_0200421A;
    }
    c->r[7] = 0x000003E8u;
    psk_sub_flags(c, c->r[2], c->r[7]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004218;
    }
    psk_sub_flags(c, c->r[2], 0x00000064u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004216;
    }
    psk_sub_flags(c, c->r[2], 0x0000000Au);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004214;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000004u);
a_02004214:
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000004u);
a_02004216:
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000004u);
a_02004218:
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000004u);
a_0200421A:
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_02004706
 | Description: Register-level C for the Thumb routine at 02004706; it calls 3 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x12C
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004706(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000040u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004730;
    }
    psk_sub_flags(c, c->r[0], 0x00000001u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200475C;
    }
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004746;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200475C;
    }
    psk_sub_flags(c, c->r[0], 0x00000004u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200476A;
    }
    psk_sub_flags(c, c->r[0], 0x00000005u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004792;
    }
    psk_sub_flags(c, c->r[0], 0x00000006u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200479C;
    }
    psk_sub_flags(c, c->r[0], 0x00000007u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020047B2;
    }
    goto a_020047C8;
a_02004730:
    c->r[1] = psk_ldrh(c->r[7] + 8u);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000004u);
    psk_sub_flags(c, c->r[1], 0x00000080u);
    if (psk_cond(c, PSK_HS)) {
        goto a_0200473C;
    }
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[1]);
    goto a_020047C8;
a_0200473C:
    c->r[1] = psk_nz(c, 0x00000080u);
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[1]);
    c->r[1] = psk_nz(c, 0x0000000Au);
    psk_w16((c->r[6] + 0x12u) & ~1u, (uint16_t)c->r[1]);
    goto a_020047C2;
a_02004746:
    c->r[1] = psk_ldrh(c->r[7] + 0xAu);
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000006u);
    psk_sub_flags(c, c->r[1], 0x00000070u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004752;
    }
    psk_w16((c->r[7] + 0xAu) & ~1u, (uint16_t)c->r[1]);
    goto a_020047C8;
a_02004752:
    c->r[1] = psk_nz(c, 0x00000070u);
    psk_w16((c->r[7] + 0xAu) & ~1u, (uint16_t)c->r[1]);
    c->r[1] = psk_nz(c, 0x0000000Au);
    psk_w16((c->r[6] + 0x12u) & ~1u, (uint16_t)c->r[1]);
    goto a_020047C2;
a_0200475C:
    c->r[1] = psk_ldrh(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w16((c->r[6] + 0x12u) & ~1u, (uint16_t)c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020047C8;
    }
    c->r[1] = 0x0000012Cu;
    psk_w16((c->r[6] + 0x12u) & ~1u, (uint16_t)c->r[1]);
    goto a_020047C2;
a_0200476A:
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200477C;
    }
    c->r[1] = psk_ldrh(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w16((c->r[6] + 0x12u) & ~1u, (uint16_t)c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020047C8;
    }
    goto a_02004782;
a_0200477C:
    c->r[0] = psk_nz(c, 0x00000003u);
    psk_call(c, 0x02003D13u, 0x02004783u);
a_02004782:
    c->r[0] = psk_nz(c, 0x0000001Eu);
    psk_call(c, 0x020042A1u, 0x02004789u);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x0000000Fu);
    if (psk_cond(c, PSK_HI)) {
        goto a_0200478E;
    }
    c->r[0] = psk_nz(c, 0x00000001u);
a_0200478E:
    psk_w8(c->r[7] + 3u, (uint8_t)c->r[0]);
    goto a_020047C2;
a_02004792:
    c->r[1] = psk_r8(c->r[7] + 3u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[7] + 3u, (uint8_t)c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020047C8;
    }
    goto a_020047C2;
a_0200479C:
    c->r[1] = psk_ldrh(c->r[7] + 8u);
    psk_sub_flags(c, c->r[1], 0x00000030u);
    if (psk_cond(c, PSK_LS)) {
        goto a_020047A6;
    }
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000006u);
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[1]);
a_020047A6:
    c->r[1] = psk_ldrh(c->r[7] + 0xAu);
    psk_sub_flags(c, c->r[1], 0x00000030u);
    if (psk_cond(c, PSK_LS)) {
        goto a_020047C8;
    }
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000008u);
    psk_w16((c->r[7] + 0xAu) & ~1u, (uint16_t)c->r[1]);
    goto a_020047C8;
a_020047B2:
    c->r[1] = psk_r8(c->r[6] + 3u);
    c->r[2] = psk_nz(c, 0x00000001u);
    psk_nz(c, c->r[1] & c->r[2]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020047C2;
    }
    c->r[1] = psk_r8(c->r[6] + 0x12u);
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_020047C8;
    }
a_020047C2:
    c->r[0] = psk_r8(c->r[7]);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7], (uint8_t)c->r[0]);
a_020047C8:
    c->r[3] = psk_ldrh(c->r[7] + 8u);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200480A;
    }
    c->r[1] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200480A;
    }
    psk_sub_flags(c, c->r[1], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200480A;
    }
    c->r[0] = psk_r8(c->r[7] + 5u);
    c->r[2] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 3u));
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 1u));
    c->r[2] = psk_add_flags(c, c->r[2], c->r[0]);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 6u));
    psk_sub_flags(c, c->r[1], 0x00000006u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004800;
    }
    psk_sub_flags(c, c->r[3], 0x00000030u);
    if (psk_cond(c, PSK_HI)) {
        goto a_02004800;
    }
    c->r[0] = psk_ldrh(c->r[7] + 0xCu);
    c->r[1] = psk_nz(c, 0x0000000Au);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 6u));
    psk_call(c, 0x02004275u, 0x020047F7u);
    psk_sub_flags(c, c->r[1], c->r[3]);
    if (psk_cond(c, PSK_HI)) {
        goto a_02004800;
    }
    c->r[3] = psk_add_flags(c, c->r[1], 0x00000000u);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[7] + 8u) & ~1u, (uint16_t)c->r[0]);
a_02004800:
    c->r[0] = psk_ldrh(c->r[7] + 0xCu);
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[3]);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004808;
    }
    c->r[0] = psk_add_flags(c, c->r[0], c->r[2]);
a_02004808:
    psk_w16((c->r[7] + 0xCu) & ~1u, (uint16_t)c->r[0]);
a_0200480A:
    c->r[3] = psk_ldrh(c->r[7] + 0xAu);
    psk_sub_flags(c, c->r[3], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004850;
    }
    c->r[1] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004850;
    }
    c->r[0] = psk_r8(c->r[7] + 6u);
    c->r[2] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 3u));
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 1u));
    c->r[2] = psk_add_flags(c, c->r[2], c->r[0]);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 6u));
    psk_sub_flags(c, c->r[1], 0x00000006u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004844;
    }
    psk_sub_flags(c, c->r[3], 0x00000030u);
    if (psk_cond(c, PSK_HI)) {
        goto a_02004844;
    }
    c->r[0] = psk_ldrh(c->r[7] + 0xEu);
    c->r[1] = psk_nz(c, 0x0000000Au);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 6u));
    psk_call(c, 0x02004275u, 0x02004835u);
    c->r[0] = psk_nz(c, 0x0000000Au);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 6u));
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[1]);
    psk_sub_flags(c, c->r[0], c->r[3]);
    if (psk_cond(c, PSK_HI)) {
        goto a_02004844;
    }
    c->r[3] = psk_add_flags(c, c->r[0], 0x00000000u);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[7] + 0xAu) & ~1u, (uint16_t)c->r[0]);
a_02004844:
    c->r[0] = psk_ldrh(c->r[7] + 0xEu);
    c->r[0] = psk_add_flags(c, c->r[0], c->r[3]);
    psk_sub_flags(c, c->r[0], c->r[2]);
    if (psk_cond(c, PSK_LO)) {
        goto a_0200484E;
    }
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[2]);
a_0200484E:
    psk_w16((c->r[7] + 0xEu) & ~1u, (uint16_t)c->r[0]);
a_02004850:
    c->r[0] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[0], 0x00000006u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200486A;
    }
    c->r[0] = psk_ldrh(c->r[7] + 8u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200486A;
    }
    c->r[0] = psk_ldrh(c->r[7] + 0xAu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_0200486A;
    }
    c->r[0] = psk_nz(c, 0x00000007u);
    psk_w8(c->r[7], (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000005Au);
    psk_w8(c->r[6] + 0x12u, (uint8_t)c->r[0]);
a_0200486A:
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_0200421C
 | Description: Register-level C for the Thumb routine at 0200421C; SWI 03.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: WRAM 0x4C4
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200421C(PskCpu *c) {
    c->r[2] = psk_nz(c, 0x000000A0u);
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 20u));
    c->r[4] = 0x00002040u;
    c->r[3] = psk_ldr(c->r[2] + 8u);
    psk_w32(c->r[2] + 0xCu, c->r[4]);
a_02004226:
    c->r[0] = psk_nz(c, 0x00000006u);
    c->r[1] = 0x000004C4u;
    c->r[0] = psk_swi(0x03u, c->r[0], c->r[1], c->r[2]);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004226;
    }
    psk_w32(c->r[2] + 8u, c->r[3]);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_0200448A
 | Description: Unverified (a random board event no script rolls). Register-level C for the Thumb routine at 0200448A; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200448A(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5], (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000004u);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000003u);
    c->r[1] = psk_nz(c, 0x00000018u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 4u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[6]);
    c->r[2] = psk_nz(c, 0x00000000u);
    psk_call(c, 0x020049A7u, 0x020044ABu);
    c->r[0] = psk_sub_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[5] + 5u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000040u);
    psk_w16((c->r[6] + 0x18u) & ~1u, (uint16_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_02004354
 | Description: Register-level C for the Thumb routine at 02004354; it calls 2 routines.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02004354(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[7] + 0xBu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000002u);
    psk_w8(c->r[7] + 0xDu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0xCu, (uint8_t)c->r[0]);
    psk_w8(c->r[5], (uint8_t)c->r[0]);
    psk_w8(c->r[5] + 1u, (uint8_t)c->r[0]);
    psk_w8(c->r[5] + 2u, (uint8_t)c->r[0]);
    c->r[1] = psk_nz(c, 0x00000018u);
    c->r[1] = psk_nz(c, psk_lsl_s(c, c->r[1], 4u));
    c->r[1] = psk_add_flags(c, c->r[1], c->r[6]);
    c->r[3] = psk_nz(c, 0x0000001Au);
    c->r[3] = psk_nz(c, psk_lsl_s(c, c->r[3], 4u));
    c->r[3] = psk_add_flags(c, c->r[3], c->r[6]);
    c->r[2] = psk_r8(c->r[5] + 4u);
    c->r[0] = psk_r8(c->r[5] + 7u);
    psk_w8(c->r[5] + 5u, (uint8_t)c->r[0]);
    psk_call(c, 0x020044EBu, 0x02004387u);
    psk_call(c, 0x02004533u, 0x0200438Bu);
    psk_w16((c->r[5] + 0xCu) & ~1u, (uint16_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w16((c->r[5] + 8u) & ~1u, (uint16_t)c->r[0]);
    psk_w8(c->r[5] + 6u, (uint8_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020041DE
 | Description: Register-level C for the Thumb routine at 020041DE.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020041DE(PskCpu *c) {
    c->r[3] = 0x02009EFCu;
    c->r[2] = psk_nz(c, 0x000000FEu);
    c->r[2] = psk_nz(c, c->r[2] & c->r[0]);
    c->r[2] = psk_ldrh(c->r[3] + c->r[2]);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 1u));
    if (psk_cond(c, PSK_HS)) {
        goto a_020041F0;
    }
    c->r[2] = psk_nz(c, psk_lsl_s(c, c->r[2], 24u));
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 24u));
    goto a_020041F2;
a_020041F0:
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 8u));
a_020041F2:
    psk_sub_flags(c, c->r[2], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_020041FA;
    }
    c->r[0] = psk_nz(c, 0x00000000u);
    c->r[15] = psk_bx(c->r[14]);
    return;
a_020041FA:
    c->r[0] = psk_nz(c, 0x00000001u);
    c->r[15] = psk_bx(c->r[14]);
    return;
}

/*----------------------
 | rrl_func_020041BA
 | Description: Register-level C for the Thumb routine at 020041BA; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020041BA(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[0] = psk_nz(c, 0x00000064u);
    psk_call(c, 0x020042A1u, 0x020041C3u);
    c->r[2] = 0x02009EF8u;
    c->r[2] = psk_ldr(c->r[2]);
    c->r[1] = psk_nz(c, 0x00000000u);
    c->r[3] = psk_nz(c, 0x000000FFu);
a_020041CA:
    c->r[4] = psk_add_flags(c, c->r[2], 0x00000000u);
    c->r[4] = psk_nz(c, c->r[4] & c->r[3]);
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[4]);
    if (psk_cond(c, PSK_LO)) {
        goto a_020041D8;
    }
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    c->r[2] = psk_nz(c, psk_lsr_s(c, c->r[2], 8u));
    goto a_020041CA;
a_020041D8:
    psk_w8(c->r[6] + 0x19u, (uint8_t)c->r[1]);
    {
        uint32_t a = c->r[13];
        c->r[0] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[0]);
    return;
}

/*----------------------
 | rrl_func_020040C0
 | Description: Register-level C for the Thumb routine at 020040C0; it calls 1 routine.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020040C0(PskCpu *c) {
    {
        uint32_t a = c->r[13];
        psk_w32(a - 4u, c->r[14]);
        c->r[13] = a - 4u;
    }
    c->r[7] = psk_nz(c, 0x00000020u);
    c->r[7] = psk_add_flags(c, c->r[7], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0xAu, (uint8_t)c->r[0]);
    c->r[0] = psk_r8(c->r[7] + 0x16u);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004186;
    }
    psk_sub_flags(c, c->r[0], 0x00000003u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200417C;
    }
    c->r[0] = psk_r8(c->r[7] + 6u);
    c->r[1] = psk_nz(c, 0x000000C0u);
    c->r[1] = psk_add_flags(c, c->r[1], c->r[0]);
    c->r[2] = psk_nz(c, 0x000000E0u);
    c->r[2] = psk_add_flags(c, c->r[2], c->r[0]);
    c->r[0] = psk_r8(c->r[6] + c->r[1]);
    c->r[1] = psk_r8(c->r[6] + c->r[2]);
    psk_sub_flags(c, c->r[1], 0x00000002u);
    if (psk_cond(c, PSK_LO)) {
        goto a_020040EA;
    }
    psk_sub_flags(c, c->r[1], 0x00000005u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004174;
    }
a_020040EA:
    psk_sub_flags(c, c->r[1], 0x00000010u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004104;
    }
    c->r[0] = psk_r8(c->r[7] + 0xEu);
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004114;
    }
    c->r[0] = psk_r8(c->r[7]);
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    c->r[0] = psk_nz(c, psk_lsl_s(c, c->r[0], 5u));
    c->r[1] = psk_r8(c->r[7] + 1u);
    c->r[0] = psk_sub_flags(c, c->r[0], c->r[1]);
    psk_sub_flags(c, c->r[0], 0x00000020u);
    if (psk_cond(c, PSK_HS)) {
        goto a_02004114;
    }
    goto a_02004168;
a_02004104:
    psk_sub_flags(c, c->r[1], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004160;
    }
    psk_sub_flags(c, c->r[0], 0x00000000u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_0200413A;
    }
    psk_sub_flags(c, c->r[0], 0x00000007u);
    if (psk_cond(c, PSK_LO)) {
        goto a_02004128;
    }
    psk_sub_flags(c, c->r[0], 0x000000FFu);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004122;
    }
a_02004114:
    c->r[0] = psk_nz(c, 0x000000D0u);
    psk_call(c, 0x020042A1u, 0x0200411Bu);
    c->r[0] = psk_nz(c, psk_lsr_s(c, c->r[0], 4u));
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Bu);
    goto a_02004188;
a_02004122:
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Au);
    goto a_02004188;
a_02004128:
    c->r[0] = psk_r8(c->r[7] + 0x11u);
    c->r[1] = psk_r8(c->r[7] + 0x10u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004186;
    }
    c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);
    psk_w8(c->r[7] + 0x12u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x13u, (uint8_t)c->r[0]);
    goto a_02004186;
a_0200413A:
    c->r[0] = psk_r8(c->r[7] + 0x16u);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004186;
    }
    c->r[0] = psk_r8(c->r[7] + 0x11u);
    c->r[1] = psk_r8(c->r[7] + 0x10u);
    psk_sub_flags(c, c->r[0], c->r[1]);
    if (psk_cond(c, PSK_NE)) {
        goto a_02004150;
    }
    psk_sub_flags(c, c->r[1], 0x00000009u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004150;
    }
    c->r[1] = psk_add_flags(c, c->r[1], 0x00000001u);
    psk_w8(c->r[7] + 0x10u, (uint8_t)c->r[1]);
a_02004150:
    psk_w8(c->r[7] + 0x12u, (uint8_t)c->r[1]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[7] + 0x13u, (uint8_t)c->r[0]);
    c->r[5] = psk_nz(c, 0x00000040u);
    c->r[5] = psk_add_flags(c, c->r[5], c->r[6]);
    c->r[0] = psk_nz(c, 0x00000000u);
    psk_w8(c->r[5] + 4u, (uint8_t)c->r[0]);
    goto a_02004186;
a_02004160:
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000005u);
    psk_w8(c->r[6] + 0x14u, (uint8_t)c->r[1]);
    c->r[0] = psk_nz(c, 0x0000000Fu);
    goto a_02004188;
a_02004168:
    c->r[0] = psk_nz(c, 0x00000001u);
    psk_w8(c->r[7] + 0xEu, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Du);
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[0]);
    c->r[0] = psk_nz(c, 0x0000000Bu);
    goto a_02004188;
a_02004174:
    c->r[1] = psk_sub_flags(c, c->r[1], 0x00000002u);
    psk_w8(c->r[6] + 0x15u, (uint8_t)c->r[1]);
    c->r[0] = psk_nz(c, 0x00000009u);
    goto a_02004188;
a_0200417C:
    c->r[0] = psk_r8(c->r[7]);
    psk_sub_flags(c, c->r[0], 0x00000002u);
    if (psk_cond(c, PSK_EQ)) {
        goto a_02004168;
    }
    c->r[0] = psk_nz(c, 0x00000008u);
    goto a_02004188;
a_02004186:
    c->r[0] = psk_nz(c, 0x00000003u);
a_02004188:
    psk_w8(c->r[6] + 0x10u, (uint8_t)c->r[0]);
    {
        uint32_t a = c->r[13];
        c->r[1] = psk_r32(a);
        c->r[13] = a + 4u;
    }
    c->r[15] = psk_bx(c->r[1]);
    return;
}
