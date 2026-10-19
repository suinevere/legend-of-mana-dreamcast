/*----------------------
 | psk_cpu.h
 | Description: NZCV helpers for register-level decompiled functions, matching the ARM data-processing flag rules.
 | Author: suinevere
 | Dependencies: psk_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef PSK_CPU_H
#define PSK_CPU_H

#include "psk_hw.h"

/*----------------------
 | PSK_N
 | Description: Negative flag bit in PskCpu.flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PSK_N 0x80000000u

/*----------------------
 | PSK_Z
 | Description: Zero flag bit in PskCpu.flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PSK_Z 0x40000000u

/*----------------------
 | PSK_C
 | Description: Carry flag bit in PskCpu.flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PSK_C 0x20000000u

/*----------------------
 | PSK_V
 | Description: Overflow flag bit in PskCpu.flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PSK_V 0x10000000u

/*----------------------
 | psk_nz
 | Description: Sets N and Z from a result and leaves C and V, as MOVS, ANDS and logical ops without a shifter carry do.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file; v -- result
 | Returns: v
 ----------------------*/
static inline uint32_t psk_nz(PskCpu *c, uint32_t v) {
    c->flags = (c->flags & (PSK_C | PSK_V)) | (v & PSK_N) | (v ? 0u : PSK_Z);
    return v;
}

/*----------------------
 | psk_sub_flags
 | Description: Sets NZCV for a - b as CMP and SUBS do (C = no borrow).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file; a, b -- operands
 | Returns: a - b
 ----------------------*/
static inline uint32_t psk_sub_flags(PskCpu *c, uint32_t a, uint32_t b) {
    uint32_t r = a - b;
    c->flags = (r & PSK_N) | (r ? 0u : PSK_Z) | (a >= b ? PSK_C : 0u) | ((((a ^ b) & (a ^ r)) >> 31) ? PSK_V : 0u);
    return r;
}

/*----------------------
 | psk_add_flags
 | Description: Sets NZCV for a + b as CMN and ADDS do.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file; a, b -- operands
 | Returns: a + b
 ----------------------*/
static inline uint32_t psk_add_flags(PskCpu *c, uint32_t a, uint32_t b) {
    uint32_t r = a + b;
    c->flags = (r & PSK_N) | (r ? 0u : PSK_Z) | (r < a ? PSK_C : 0u) | (((~(a ^ b) & (a ^ r)) >> 31) ? PSK_V : 0u);
    return r;
}

/*----------------------
 | psk_adc_flags
 | Description: Sets NZCV for a + b + C as ADCS does.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file; a, b -- operands
 | Returns: the sum
 ----------------------*/
static inline uint32_t psk_adc_flags(PskCpu *c, uint32_t a, uint32_t b) {
    uint64_t s = (uint64_t)a + b + ((c->flags & PSK_C) ? 1u : 0u);
    uint32_t r = (uint32_t)s;
    c->flags = (r & PSK_N) | (r ? 0u : PSK_Z) | ((s >> 32) ? PSK_C : 0u) | ((((a ^ r) & (b ^ r)) >> 31) ? PSK_V : 0u);
    return r;
}

/*----------------------
 | psk_sbc_flags
 | Description: Sets NZCV for a - b - !C as SBCS does.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file; a, b -- operands
 | Returns: the difference
 ----------------------*/
static inline uint32_t psk_sbc_flags(PskCpu *c, uint32_t a, uint32_t b) {
    uint64_t sub = (uint64_t)b - ((c->flags & PSK_C) ? 1u : 0u) + 1u;
    uint32_t r = (uint32_t)((uint64_t)a - sub);
    c->flags = (r & PSK_N) | (r ? 0u : PSK_Z) | (sub <= a ? PSK_C : 0u) | ((((a ^ r) & (~b ^ r)) >> 31) ? PSK_V : 0u);
    return r;
}

/*----------------------
 | psk_carry
 | Description: The C flag as 0 or 1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file
 | Returns: 0 or 1
 ----------------------*/
static inline uint32_t psk_carry(const PskCpu *c) { return (c->flags & PSK_C) ? 1u : 0u; }

/*----------------------
 | psk_set_carry
 | Description: Sets or clears C.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file; on -- new carry
 | Returns: N/A
 ----------------------*/
static inline void psk_set_carry(PskCpu *c, uint32_t on) { c->flags = (c->flags & ~PSK_C) | (on ? PSK_C : 0u); }

/*----------------------
 | psk_ror
 | Description: Rotates right by n modulo 32.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value; n -- amount
 | Returns: rotated value
 ----------------------*/
static inline uint32_t psk_ror(uint32_t v, uint32_t n) {
    n &= 31u;
    return n ? (v >> n) | (v << (32u - n)) : v;
}

/*----------------------
 | psk_lsl / psk_lsr / psk_asr
 | Description: Shifts with the ARM rules for amounts of 32 and more, no flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value; n -- amount
 | Returns: shifted value
 ----------------------*/
static inline uint32_t psk_lsl(uint32_t v, uint32_t n) { return n < 32u ? v << n : 0u; }
static inline uint32_t psk_lsr(uint32_t v, uint32_t n) { return n < 32u ? v >> n : 0u; }
static inline uint32_t psk_asr(uint32_t v, uint32_t n) {
    return (uint32_t)(n < 32u ? ((int32_t)v >> n) : ((int32_t)v >> 31));
}

/*----------------------
 | psk_lsl_s / psk_lsr_s / psk_asr_s / psk_ror_s / psk_rrx_s
 | Description: Shifts that also set C from the last bit out, as the barrel shifter does for flag-setting logical ops.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file; v -- value; n -- amount
 | Returns: shifted value
 ----------------------*/
static inline uint32_t psk_lsl_s(PskCpu *c, uint32_t v, uint32_t n) {
    if (n == 0) return v;
    if (n < 32u) {
        psk_set_carry(c, (v >> (32u - n)) & 1u);
        return v << n;
    }
    psk_set_carry(c, n == 32u ? (v & 1u) : 0u);
    return 0;
}
static inline uint32_t psk_lsr_s(PskCpu *c, uint32_t v, uint32_t n) {
    if (n == 0) return v;
    if (n < 32u) {
        psk_set_carry(c, (v >> (n - 1u)) & 1u);
        return v >> n;
    }
    psk_set_carry(c, n == 32u ? (v >> 31) : 0u);
    return 0;
}
static inline uint32_t psk_asr_s(PskCpu *c, uint32_t v, uint32_t n) {
    if (n == 0) return v;
    if (n < 32u) {
        psk_set_carry(c, (v >> (n - 1u)) & 1u);
        return (uint32_t)((int32_t)v >> n);
    }
    psk_set_carry(c, v >> 31);
    return (uint32_t)((int32_t)v >> 31);
}
static inline uint32_t psk_ror_s(PskCpu *c, uint32_t v, uint32_t n) {
    uint32_t r = psk_ror(v, n);
    if (n) psk_set_carry(c, r >> 31);
    return r;
}
static inline uint32_t psk_rrx(const PskCpu *c, uint32_t v) { return (v >> 1) | (psk_carry(c) << 31); }
static inline uint32_t psk_rrx_s(PskCpu *c, uint32_t v) {
    uint32_t r = psk_rrx(c, v);
    psk_set_carry(c, v & 1u);
    return r;
}

/*----------------------
 | PskCond
 | Description: ARM condition codes by their assembler names, for psk_cond.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum PskCond {
    PSK_EQ, PSK_NE, PSK_HS, PSK_LO, PSK_MI, PSK_PL, PSK_VS, PSK_VC,
    PSK_HI, PSK_LS, PSK_GE, PSK_LT, PSK_GT, PSK_LE, PSK_AL
};

/*----------------------
 | psk_cond
 | Description: Whether an ARM condition code (0 EQ .. 14 AL) holds for the flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: c -- register file; cond -- condition
 | Returns: nonzero when it holds
 ----------------------*/
static inline int psk_cond(const PskCpu *c, int cond) {
    uint32_t f = c->flags;
    int n = !!(f & PSK_N), z = !!(f & PSK_Z), cy = !!(f & PSK_C), v = !!(f & PSK_V);
    switch (cond) {
        case 0: return z;
        case 1: return !z;
        case 2: return cy;
        case 3: return !cy;
        case 4: return n;
        case 5: return !n;
        case 6: return v;
        case 7: return !v;
        case 8: return cy && !z;
        case 9: return !cy || z;
        case 10: return n == v;
        case 11: return n != v;
        case 12: return !z && n == v;
        case 13: return z || n != v;
        default: return 1;
    }
}

/*----------------------
 | psk_ldr / psk_ldrh / psk_ldrsh / psk_ldrsb
 | Description: Loads as the ARM7 does them: misaligned words and halfwords rotated, odd signed halfwords read as a signed byte.
 | Author: suinevere
 | Dependencies: psk_hw.h
 | Globals: N/A
 | Params: a -- address
 | Returns: loaded value
 ----------------------*/
static inline uint32_t psk_ldr(uint32_t a) { return psk_ror(psk_r32(a & ~3u), (a & 3u) * 8u); }
static inline uint32_t psk_ldrh(uint32_t a) { return psk_ror(psk_r16(a & ~1u), (a & 1u) * 8u); }
static inline uint32_t psk_ldrsh(uint32_t a) {
    return (a & 1u) ? (uint32_t)(int32_t)(int8_t)psk_r8(a) : (uint32_t)(int32_t)(int16_t)psk_r16(a);
}
static inline uint32_t psk_ldrsb(uint32_t a) { return (uint32_t)(int32_t)(int8_t)psk_r8(a); }

/*----------------------
 | psk_bx
 | Description: The continuation of BX: bit 0 set stays Thumb (bit kept), clear goes ARM word-aligned.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- target register value
 | Returns: continuation, bit 0 = Thumb
 ----------------------*/
static inline uint32_t psk_bx(uint32_t v) { return (v & 1u) ? v : (v & ~3u); }

#endif
