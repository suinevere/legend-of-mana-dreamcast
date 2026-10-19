/*----------------------
 | rrl_page8.c
 | Description: Ring Ring Land routines at 02008000-02008FFF as register-level C, until they are named and grouped by concern.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "rrl.h"

/*----------------------
 | rrl_func_02008294
 | Description: Register-level C for the ARM routine at 02008294.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008294(PskCpu *c) {
    c->r[12] = 0x02004311u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020082B8
 | Description: Unverified (standby, the runner cannot sample a stopped clock). Register-level C for the ARM routine at 020082B8.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020082B8(PskCpu *c) {
    c->r[12] = 0x02003D85u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020082C4
 | Description: Register-level C for the ARM routine at 020082C4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020082C4(PskCpu *c) {
    c->r[12] = 0x020042FFu;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020082A0
 | Description: Register-level C for the ARM routine at 020082A0.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020082A0(PskCpu *c) {
    c->r[12] = 0x0200433Bu;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020082AC
 | Description: Register-level C for the ARM routine at 020082AC.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020082AC(PskCpu *c) {
    c->r[12] = 0x02003305u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_0200845C
 | Description: Register-level C for the ARM routine at 0200845C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200845C(PskCpu *c) {
    c->r[12] = 0x020042EDu;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008468
 | Description: Register-level C for the ARM routine at 02008468.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008468(PskCpu *c) {
    c->r[12] = 0x02004323u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020082D0
 | Description: Register-level C for the ARM routine at 020082D0.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020082D0(PskCpu *c) {
    c->r[12] = 0x02001AE1u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020083FC
 | Description: Register-level C for the ARM routine at 020083FC.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020083FC(PskCpu *c) {
    c->r[12] = 0x02004275u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020082DC
 | Description: Register-level C for the ARM routine at 020082DC.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020082DC(PskCpu *c) {
    c->r[12] = 0x02003313u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020082F4
 | Description: Register-level C for the ARM routine at 020082F4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020082F4(PskCpu *c) {
    c->r[12] = 0x02003317u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020082E8
 | Description: Register-level C for the ARM routine at 020082E8.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020082E8(PskCpu *c) {
    c->r[12] = 0x02001BCDu;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008300
 | Description: Register-level C for the ARM routine at 02008300.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008300(PskCpu *c) {
    c->r[12] = 0x02001C3Fu;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_0200830C
 | Description: Register-level C for the ARM routine at 0200830C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200830C(PskCpu *c) {
    c->r[12] = 0x02001C85u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008384
 | Description: Register-level C for the ARM routine at 02008384.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008384(PskCpu *c) {
    c->r[12] = 0x020039C3u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020083E4
 | Description: Register-level C for the ARM routine at 020083E4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020083E4(PskCpu *c) {
    c->r[12] = 0x02004235u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020083F0
 | Description: Register-level C for the ARM routine at 020083F0.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020083F0(PskCpu *c) {
    c->r[12] = 0x0200424Du;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008318
 | Description: Register-level C for the ARM routine at 02008318.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008318(PskCpu *c) {
    c->r[12] = 0x02001CA5u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008324
 | Description: Register-level C for the ARM routine at 02008324.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008324(PskCpu *c) {
    c->r[12] = 0x020045F1u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008330
 | Description: Register-level C for the ARM routine at 02008330.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008330(PskCpu *c) {
    c->r[12] = 0x02002695u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_0200833C
 | Description: Register-level C for the ARM routine at 0200833C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200833C(PskCpu *c) {
    c->r[12] = 0x02001D05u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008348
 | Description: Register-level C for the ARM routine at 02008348.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008348(PskCpu *c) {
    c->r[12] = 0x02001F29u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020083D8
 | Description: Register-level C for the ARM routine at 020083D8.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020083D8(PskCpu *c) {
    c->r[12] = 0x02003A9Fu;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008354
 | Description: Register-level C for the ARM routine at 02008354.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008354(PskCpu *c) {
    c->r[12] = 0x02002DE1u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008360
 | Description: Register-level C for the ARM routine at 02008360.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008360(PskCpu *c) {
    c->r[12] = 0x02001FD9u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_0200836C
 | Description: Register-level C for the ARM routine at 0200836C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200836C(PskCpu *c) {
    c->r[12] = 0x020045F1u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008378
 | Description: Register-level C for the ARM routine at 02008378.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008378(PskCpu *c) {
    c->r[12] = 0x02002103u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_0200839C
 | Description: Register-level C for the ARM routine at 0200839C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200839C(PskCpu *c) {
    c->r[12] = 0x020041FFu;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008390
 | Description: Register-level C for the ARM routine at 02008390.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008390(PskCpu *c) {
    c->r[12] = 0x02004707u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020083A8
 | Description: Register-level C for the ARM routine at 020083A8.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020083A8(PskCpu *c) {
    c->r[12] = 0x02002417u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020083B4
 | Description: Register-level C for the ARM routine at 020083B4.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020083B4(PskCpu *c) {
    c->r[12] = 0x02002465u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_020083CC
 | Description: Register-level C for the ARM routine at 020083CC.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_020083CC(PskCpu *c) {
    c->r[12] = 0x02001E43u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008450
 | Description: Register-level C for the ARM routine at 02008450.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008450(PskCpu *c) {
    c->r[12] = 0x02003DB5u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008408
 | Description: Register-level C for the ARM routine at 02008408.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008408(PskCpu *c) {
    c->r[12] = 0x02002815u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008414
 | Description: Register-level C for the ARM routine at 02008414.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008414(PskCpu *c) {
    c->r[12] = 0x02003CA9u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008420
 | Description: Register-level C for the ARM routine at 02008420.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008420(PskCpu *c) {
    c->r[12] = 0x020042A1u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008444
 | Description: Unverified (quit dialog, needs a 96-pass action hold). Register-level C for the ARM routine at 02008444.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008444(PskCpu *c) {
    c->r[12] = 0x02003C5Du;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_02008438
 | Description: Register-level C for the ARM routine at 02008438.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_02008438(PskCpu *c) {
    c->r[12] = 0x02003D13u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}

/*----------------------
 | rrl_func_0200842C
 | Description: Register-level C for the ARM routine at 0200842C.
 | Author: suinevere
 | Dependencies: rrl.h
 | Globals: N/A
 | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)
 | Returns: N/A
 ----------------------*/
void rrl_func_0200842C(PskCpu *c) {
    c->r[12] = 0x020040C1u;
    c->r[15] = psk_bx(c->r[12]);
    return;
}
