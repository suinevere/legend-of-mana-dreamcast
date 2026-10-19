"""----------------------
 | psk_lift.py
 | Description: Lifts Ring Ring Land functions to register-level C by decoding raw ARM and Thumb bits as the pinned emulator core does.
 | Author: suinevere
 | Dependencies: psk_file.py, psk_split.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import argparse
import glob
import os
import struct
import sys
from dataclasses import dataclass, field

import psk_file

"""----------------------
 | COND
 | Description: C names of the ARM condition codes 0..13, as psk_cpu.h defines them.
 | Author: suinevere
 | Dependencies: psk_cpu.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
COND = ["PSK_EQ", "PSK_NE", "PSK_HS", "PSK_LO", "PSK_MI", "PSK_PL", "PSK_VS", "PSK_VC",
        "PSK_HI", "PSK_LS", "PSK_GE", "PSK_LT", "PSK_GT", "PSK_LE"]

"""----------------------
 | MOV_LR_PC
 | Description: The ARM word `mov lr, pc` without its condition field; a jump right after it is a call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
MOV_LR_PC = 0x01A0E00F

"""----------------------
 | WRAM_END
 | Description: One past the last work RAM address; literals below it are reported as globals.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
WRAM_END = 0x800

"""----------------------
 | IDLE_EDGES
 | Description: Back edges of loops that wait for an interrupt to change memory: the frame wait, the sound wait, the INT latch wait.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
IDLE_EDGES = frozenset({0x02003E64, 0x02003D00, 0x020038D0})

"""----------------------
 | FRAME_POINT
 | Description: The frame wait's entry, where traces take their samples; its C marks it with psk_frame().
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FRAME_POINT = 0x02003E48


"""----------------------
 | LiftError
 | Description: Raised for an encoding the core panics on or this lifter does not model.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class LiftError(ValueError):
    pass


"""----------------------
 | Step
 | Description: One decoded instruction: its C statements, condition, and where control goes next.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class Step:
    addr: int
    size: int
    lines: list = field(default_factory=list)
    cond: str = None
    flow: str = "next"
    target: int = None
    globals_: set = field(default_factory=set)


"""----------------------
 | hx / off
 | Description: C literals: hx for data values (eight hex digits), off for address offsets (decimal below ten).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value
 | Returns: C text
 ----------------------"""
def hx(v):
    return "0x%08Xu" % (v & 0xFFFFFFFF)


def off(v):
    return "%du" % v if v < 10 else "0x%Xu" % v


"""----------------------
 | par
 | Description: Parenthesises a C expression unless it is a single token.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: e -- expression
 | Returns: C text
 ----------------------"""
def par(e):
    return e if " " not in e else "(%s)" % e


"""----------------------
 | plus
 | Description: base + k or base - k as C text, dropping a zero offset.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: base -- C expression; k -- signed byte offset
 | Returns: C text
 ----------------------"""
def plus(base, k):
    if k == 0:
        return base
    return "%s + %s" % (base, off(k)) if k > 0 else "%s - %s" % (base, off(-k))


"""----------------------
 | sx
 | Description: Sign-extends the low bits of a value.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value; bits -- width
 | Returns: signed int
 ----------------------"""
def sx(v, bits):
    v &= (1 << bits) - 1
    return v - (1 << bits) if v >> (bits - 1) else v


"""----------------------
 | ror32
 | Description: Rotates a 32-bit value right.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: v -- value; n -- amount
 | Returns: rotated value
 ----------------------"""
def ror32(v, n):
    n &= 31
    return ((v >> n) | (v << (32 - n))) & 0xFFFFFFFF if n else v


"""----------------------
 | Lifter
 | Description: Decodes instructions of the file mapped at base and turns them into Steps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Lifter:
    def __init__(self, data, base):
        self.data = data
        self.base = base

    def u16(self, a):
        o = a - self.base
        if o < 0 or o + 2 > len(self.data):
            return None
        return struct.unpack_from("<H", self.data, o)[0]

    def u32(self, a):
        o = a - self.base
        if o < 0 or o + 4 > len(self.data):
            return None
        return struct.unpack_from("<I", self.data, o)[0]

    def u8(self, a):
        o = a - self.base
        return self.data[o] if 0 <= o < len(self.data) else None

    def literal(self, a, size, step):
        if size == 1:
            v = self.u8(a)
        else:
            w = self.u32(a & ~3)
            v = None if w is None else ror32(w, (a & 3) * 8)
        if v is None:
            raise LiftError("literal outside the file at %08X" % a)
        if v < WRAM_END and size == 4:
            step.globals_.add(v)
        return v

    def decode(self, a, thumb):
        if thumb:
            return self.thumb(a)
        return self.arm(a)

    def reg(self, n, pc):
        return hx(pc) if n == 15 else "c->r[%d]" % n

    def shifted(self, w, pc, carry):
        rm = self.reg(w & 15, pc)
        kind = (w >> 5) & 3
        if (w >> 4) & 1:
            amt = "(%s & 0xFFu)" % self.reg((w >> 8) & 15, pc - 4)
            if carry:
                return "%s(c, %s, %s)" % (("psk_lsl_s", "psk_lsr_s", "psk_asr_s", "psk_ror_s")[kind], rm, amt)
            return "%s(%s, %s)" % (("psk_lsl", "psk_lsr", "psk_asr", "psk_ror")[kind], rm, amt)
        n = (w >> 7) & 31
        if kind == 0:
            if n == 0:
                return rm
            return "psk_lsl_s(c, %s, %du)" % (rm, n) if carry else "(%s << %du)" % (rm, n)
        if kind == 1:
            if carry:
                return "psk_lsr_s(c, %s, %du)" % (rm, n or 32)
            return hx(0) if n == 0 else "(%s >> %du)" % (rm, n)
        if kind == 2:
            return ("psk_asr_s(c, %s, %du)" if carry else "psk_asr(%s, %du)") % (rm, n or 32)
        if n == 0:
            return ("psk_rrx_s(c, %s)" if carry else "psk_rrx(c, %s)") % rm
        return ("psk_ror_s(c, %s, %du)" if carry else "psk_ror(%s, %du)") % (rm, n)

    def arm(self, a):
        w = self.u32(a)
        if w is None:
            raise LiftError("no code at %08X" % a)
        s = Step(a, 4)
        cc = w >> 28
        if cc == 15:
            raise LiftError("unconditional-space encoding %08X at %08X" % (w, a))
        if cc != 14:
            s.cond = COND[cc]
        idx = ((w >> 16) & 0xFF0) | ((w >> 4) & 0xF)
        if (idx & 0xF00) == 0xF00:
            s.lines = ["c->r[0] = psk_swi(0x%02Xu, c->r[0], c->r[1], c->r[2]);" % (w & 0xFF)]
        elif ((idx >> 7) == 0b00010 and (idx & 0xF) == 0 and not (idx >> 4) & 1) or ((idx >> 7) == 0b00110 and not (idx >> 4) & 1):
            raise LiftError("PSR transfer at %08X" % a)
        elif (idx & 0xF8F) == 0x89:
            self.arm_mull(w, s)
        elif (idx & 0xFCF) == 9:
            rd, rn, rs, rm = (w >> 16) & 15, (w >> 12) & 15, (w >> 8) & 15, w & 15
            e = "c->r[%d] * c->r[%d]" % (rm, rs)
            if (w >> 21) & 1:
                e += " + c->r[%d]" % rn
            s.lines = ["c->r[%d] = %s;" % (rd, "psk_nz(c, %s)" % e if (w >> 20) & 1 else e)]
        elif (idx & 0xFBF) == 0x109:
            self.arm_swap(w, s)
        elif idx == 0x121:
            self.arm_bx(w, s, a)
        elif (idx >> 9) == 0b101:
            t = a + 8 + (sx(w, 24) << 2)
            if (w >> 24) & 1:
                s.lines = ["psk_call(c, %s, %s);" % (hx(t), hx(a + 4))]
            else:
                s.flow, s.target = "branch", t
        elif (idx >> 10) == 1:
            self.arm_ldst(w, s, a)
        elif (idx & 0b1001) == 0b1001 and (idx >> 9) == 0:
            self.arm_misc(w, s, a)
        elif (idx >> 9) == 0b100:
            self.arm_block(w, s, a)
        elif (idx >> 9) in (0, 1):
            self.arm_dp(w, s, a)
        else:
            raise LiftError("undefined ARM %08X at %08X" % (w, a))
        return s

    def arm_mull(self, w, s):
        lo, hi, rs, rm = (w >> 12) & 15, (w >> 16) & 15, (w >> 8) & 15, w & 15
        if (w >> 22) & 1:
            m = "(uint64_t)((int64_t)(int32_t)c->r[%d] * (int32_t)c->r[%d])" % (rm, rs)
        else:
            m = "(uint64_t)c->r[%d] * c->r[%d]" % (rm, rs)
        if (w >> 21) & 1:
            m += " + (((uint64_t)c->r[%d] << 32) | c->r[%d])" % (hi, lo)
        s.lines = ["{", "uint64_t m = %s;" % m]
        if (w >> 20) & 1:
            s.lines.append("c->flags = (c->flags & (PSK_C | PSK_V)) | ((uint32_t)(m >> 32) & PSK_N) | (m ? 0u : PSK_Z);")
        s.lines += ["c->r[%d] = (uint32_t)m;" % lo, "c->r[%d] = (uint32_t)(m >> 32);" % hi, "}"]

    def arm_swap(self, w, s):
        rn, rd, rm = (w >> 16) & 15, (w >> 12) & 15, w & 15
        if (w >> 22) & 1:
            s.lines = ["{", "uint32_t a = c->r[%d];" % rn, "uint32_t v = c->r[%d];" % rm,
                       "c->r[%d] = psk_r8(a);" % rd, "psk_w8(a, (uint8_t)v);", "}"]
        else:
            s.lines = ["{", "uint32_t a = c->r[%d];" % rn, "uint32_t v = c->r[%d];" % rm,
                       "c->r[%d] = psk_ldr(a);" % rd, "psk_w32(a & ~3u, v);", "}"]

    def arm_bx(self, w, s, a):
        t = hx((a + 8) & ~3) if (w & 15) == 15 else "psk_bx(c->r[%d])" % (w & 15)
        prev = self.u32(a - 4)
        if prev is not None and (prev & 0x0FFFFFFF) == MOV_LR_PC and (prev >> 28) == (w >> 28):
            s.lines = ["psk_call(c, %s, %s);" % (t, hx(a + 4))]
            return
        s.lines = ["c->r[15] = %s;" % t, "return;"]
        s.flow = "exit"

    def jump(self, s, a, expr, mask):
        prev = self.u32(a - 4)
        if prev is not None and (prev & 0x0FFFFFFF) == MOV_LR_PC and (prev >> 28) == (self.u32(a) >> 28):
            s.lines.append("psk_call(c, %s & %s, %s);" % (par(expr), mask, hx(a + 4)))
            return
        s.lines += ["c->r[15] = %s & %s;" % (par(expr), mask), "return;"]
        s.flow = "exit"

    def arm_dp(self, w, s, a):
        imm, op, sf = (w >> 25) & 1, (w >> 21) & 15, (w >> 20) & 1
        rn, rd = (w >> 16) & 15, (w >> 12) & 15
        logical = op in (0, 1, 8, 9, 12, 13, 14, 15)
        carry = bool(sf) and logical
        pc = a + 12 if (not imm and (w >> 4) & 1) else a + 8
        if imm:
            rot = ((w >> 8) & 15) * 2
            v = ror32(w & 0xFF, rot)
            b = hx(v)
            if carry and rot:
                s.lines.append("psk_set_carry(c, %du);" % (v >> 31))
        else:
            b = self.shifted(w, pc, carry)
        x = self.reg(rn, pc)
        if op in (8, 9, 10, 11):
            e = {8: "psk_nz(c, %s & %s)" % (x, par(b)), 9: "psk_nz(c, %s ^ %s)" % (x, par(b)),
                 10: "psk_sub_flags(c, %s, %s)" % (x, b), 11: "psk_add_flags(c, %s, %s)" % (x, b)}[op]
            s.lines.append(e + ";")
            return
        if sf and not logical:
            e = {2: "psk_sub_flags(c, %s, %s)" % (x, b), 3: "psk_sub_flags(c, %s, %s)" % (b, x),
                 4: "psk_add_flags(c, %s, %s)" % (x, b), 5: "psk_adc_flags(c, %s, %s)" % (x, b),
                 6: "psk_sbc_flags(c, %s, %s)" % (x, b), 7: "psk_sbc_flags(c, %s, %s)" % (b, x)}[op]
        elif not logical:
            e = {2: "%s - %s" % (x, par(b)), 3: "%s - %s" % (b, par(x)), 4: "%s + %s" % (x, b),
                 5: "%s + %s + psk_carry(c)" % (x, b), 6: "%s - %s - 1u + psk_carry(c)" % (x, par(b)),
                 7: "%s - %s - 1u + psk_carry(c)" % (b, par(x))}[op]
        else:
            e = {0: "%s & %s" % (x, par(b)), 1: "%s ^ %s" % (x, par(b)), 12: "%s | %s" % (x, par(b)),
                 13: b, 14: "%s & ~%s" % (x, par(b)), 15: "~%s" % par(b)}[op]
            if sf:
                e = "psk_nz(c, %s)" % e
        if rd == 15:
            if sf:
                raise LiftError("DP with rd=pc and S at %08X" % a)
            self.jump(s, a, e, "~3u")
            return
        s.lines.append("c->r[%d] = %s;" % (rd, e))

    def mem_ops(self, s, a, base, newbase, pre, wb_reg, op_lines_fn):
        if wb_reg is None:
            s.lines += op_lines_fn(newbase if pre else base)
            return
        s.lines.append("{")
        if pre:
            s.lines.append("uint32_t b = %s;" % newbase)
            s.lines += op_lines_fn("b")
        else:
            s.lines.append("uint32_t a = %s;" % base)
            s.lines.append("uint32_t b = %s;" % newbase.replace(base, "a", 1))
            s.lines += op_lines_fn("a")
        s.lines.append("c->r[%d] = b;" % wb_reg)
        s.lines.append("}")

    def arm_ldst(self, w, s, a):
        regoff, pre, up, byte, wbit, load = [(w >> k) & 1 for k in (25, 24, 23, 22, 21, 20)]
        rn, rd = (w >> 16) & 15, (w >> 12) & 15
        if wbit and not pre:
            raise LiftError("LDRT/STRT at %08X" % a)
        if rn == 15 and not regoff and pre and not wbit and load:
            addr = a + 8 + ((w & 0xFFF) if up else -(w & 0xFFF))
            v = self.literal(addr, 1 if byte else 4, s)
            if rd == 15:
                self.jump(s, a, hx(v), "~3u")
                return
            s.lines.append("c->r[%d] = %s;" % (rd, hx(v)))
            return
        base = self.reg(rn, a + 8)
        if regoff:
            o = self.shifted(w & ~0x10, a + 8, False)
            newbase = "%s %s %s" % (base, "+" if up else "-", par(o))
        else:
            newbase = plus(base, (w & 0xFFF) if up else -(w & 0xFFF))
        wb = not (pre and not wbit)
        wb_reg = rn if wb and not (load and rn == rd) else None
        exit_after = load and not byte and rd == 15

        def ops(addr):
            if load and byte:
                return ["c->r[%d] = psk_r8(%s);" % (rd, addr)]
            if load:
                if rd == 15:
                    return ["c->r[15] = psk_ldr(%s) & ~3u;" % addr]
                return ["c->r[%d] = psk_ldr(%s);" % (rd, addr)]
            if byte:
                if rd == 15:
                    raise LiftError("STRB pc at %08X" % a)
                return ["psk_w8(%s, (uint8_t)c->r[%d]);" % (addr, rd)]
            return ["psk_w32(%s, %s);" % (addr, self.reg(rd, a + 12))]

        self.mem_ops(s, a, base, newbase, pre, wb_reg, ops)
        if exit_after:
            if (self.u32(a - 4) or 0) & 0x0FFFFFFF == MOV_LR_PC:
                raise LiftError("call through ldr pc at %08X" % a)
            s.lines.append("return;")
            s.flow = "exit"

    def arm_misc(self, w, s, a):
        pre, up, immf, wbit, load = [(w >> k) & 1 for k in (24, 23, 22, 21, 20)]
        sgn, half = (w >> 6) & 1, (w >> 5) & 1
        rn, rd = (w >> 16) & 15, (w >> 12) & 15
        if rd == 15:
            raise LiftError("halfword transfer with rd=pc at %08X" % a)
        base = self.reg(rn, a + 8)
        if immf:
            newbase = plus(base, ((w & 15) | ((w >> 4) & 0xF0)) * (1 if up else -1))
        else:
            newbase = "%s %s %s" % (base, "+" if up else "-", self.reg(w & 15, a + 8))
        wb = not (pre and not wbit)
        wb_reg = rn if wb and not (load and rn == rd) else None

        def ops(addr):
            if load:
                fn = {(1, 0): "psk_ldrh", (1, 1): "psk_ldrsh", (0, 1): "psk_ldrsb"}.get((half, sgn))
                if fn is None:
                    raise LiftError("misc load at %08X" % a)
                return ["c->r[%d] = %s(%s);" % (rd, fn, addr)]
            if half and not sgn:
                return ["psk_w16(%s, (uint16_t)c->r[%d]);" % (addr, rd)]
            raise LiftError("LDRD/STRD at %08X" % a)

        self.mem_ops(s, a, base, newbase, pre, wb_reg, ops)

    def block(self, s, rb, regs, up, pre, load, wb, pcval, pc_mask, store_val=None):
        n = len(regs)
        order = sorted(regs) if up else sorted(regs, reverse=True)
        s.lines += ["{", "uint32_t a = c->r[%d];" % rb]
        pc_line = None
        for k, i in enumerate(order):
            o = (4 * (k + 1) if pre else 4 * k) if up else (-4 * (k + 1) if pre else -4 * k)
            addr = plus("a", o)
            if load:
                if i == 15:
                    pc_line = pc_mask % ("psk_r32(%s)" % addr)
                    s.lines.append(pc_line)
                else:
                    s.lines.append("c->r[%d] = psk_r32(%s);" % (i, addr))
            else:
                v = store_val(i) if store_val else self.reg(i, pcval)
                s.lines.append("psk_w32(%s, %s);" % (addr, v))
        if wb:
            s.lines.append("c->r[%d] = %s;" % (rb, plus("a", 4 * n if up else -4 * n)))
        if pc_line:
            s.lines.append("return;")
            s.flow = "exit"
        s.lines.append("}")

    def arm_block(self, w, s, a):
        pre, up, user, wbit, load = [(w >> k) & 1 for k in (24, 23, 22, 21, 20)]
        rb = (w >> 16) & 15
        regs = [i for i in range(16) if (w >> i) & 1]
        if user or not regs or rb == 15:
            raise LiftError("LDM/STM form at %08X" % a)
        wb = bool(wbit) and not (load and rb in regs)
        self.block(s, rb, regs, up, pre, load, wb, a + 8, "c->r[15] = %s & ~3u;")

    def thumb(self, a):
        w = self.u16(a)
        if w is None:
            raise LiftError("no code at %08X" % a)
        s = Step(a, 2)
        i = w >> 8
        rd3, rs3 = w & 7, (w >> 3) & 7
        r = lambda n: self.reg(n, a + 4)
        if (i >> 5) == 1:
            d, v, op = (w >> 8) & 7, w & 0xFF, (w >> 11) & 3
            s.lines = [["c->r[%d] = psk_nz(c, %s);" % (d, hx(v)),
                        "psk_sub_flags(c, c->r[%d], %s);" % (d, hx(v)),
                        "c->r[%d] = psk_add_flags(c, c->r[%d], %s);" % (d, d, hx(v)),
                        "c->r[%d] = psk_sub_flags(c, c->r[%d], %s);" % (d, d, hx(v))][op]]
        elif (i >> 4) == 0xF:
            nxt = self.u16(a + 2)
            if (w & 0xF800) != 0xF000 or nxt is None or (nxt & 0xF800) != 0xF800:
                raise LiftError("lone Thumb BL half at %08X" % a)
            t = a + 4 + (sx(w, 11) << 12) + ((nxt & 0x7FF) << 1)
            s.size = 4
            s.lines = ["psk_call(c, %s, %s);" % (hx(t | 1), hx((a + 4) | 1))]
        elif (i >> 3) == 0b11000 or (i >> 3) == 0b11001:
            rb, load = (w >> 8) & 7, (i >> 3) == 0b11001
            regs = [k for k in range(8) if (w >> k) & 1]
            if not regs:
                raise LiftError("empty Thumb LDM/STM at %08X" % a)
            if load:
                self.block(s, rb, regs, True, False, True, rb not in regs, a + 4, None)
            else:
                first = all(k >= rb for k in regs)
                final = plus("a", 4 * len(regs))
                self.block(s, rb, regs, True, False, False, True, a + 4, None,
                           lambda k: final if (k == rb and not first) else "c->r[%d]" % k)
        elif (i >> 1) == 0b1011010:
            regs = [k for k in range(8) if (w >> k) & 1] + ([14] if w & 0x100 else [])
            if not regs:
                raise LiftError("empty push at %08X" % a)
            self.block(s, 13, regs, False, True, False, True, a + 4, None)
        elif (i >> 1) == 0b1011110:
            regs = [k for k in range(8) if (w >> k) & 1] + ([15] if w & 0x100 else [])
            if not regs:
                raise LiftError("empty pop at %08X" % a)
            self.block(s, 13, regs, True, False, True, True, a + 4, "c->r[15] = (%s & ~1u) | 1u;")
        elif (i >> 3) == 0b1001:
            addr = ((a + 4) & ~2) + ((w & 0xFF) << 2)
            s.lines = ["c->r[%d] = %s;" % ((w >> 8) & 7, hx(self.literal(addr, 4, s)))]
        elif (i >> 4) == 0x8:
            ea = plus(r(rs3), ((w >> 6) & 31) << 1)
            if (w >> 11) & 1:
                s.lines = ["c->r[%d] = psk_ldrh(%s);" % (rd3, ea)]
            else:
                s.lines = ["psk_w16((%s) & ~1u, (uint16_t)c->r[%d]);" % (ea, rd3) if " " in ea else
                           "psk_w16(%s & ~1u, (uint16_t)c->r[%d]);" % (ea, rd3)]
        elif (i >> 2) == 0b010110:
            ea = "%s + %s" % (r(rs3), r((w >> 6) & 7))
            s.lines = ["c->r[%d] = %s(%s);" % (rd3, "psk_ldrh" if (i >> 1) & 1 else "psk_ldr", ea)]
        elif (i >> 3) in (1, 2):
            n = ((w >> 6) & 31) or 32
            fn = "psk_lsr_s" if (i >> 3) == 1 else "psk_asr_s"
            s.lines = ["c->r[%d] = psk_nz(c, %s(c, %s, %du));" % (rd3, fn, r(rs3), n)]
        elif (i >> 4) == 0b1101:
            cc = (w >> 8) & 15
            if cc == 15:
                s.lines = ["c->r[0] = psk_swi(0x%02Xu, c->r[0], c->r[1], c->r[2]);" % (w & 0xFF)]
            else:
                s.flow, s.target = "branch", a + 4 + (sx(w & 0xFF, 8) << 1)
                if cc != 14:
                    s.cond = COND[cc]
        elif (i >> 2) == 0b010001:
            op = i & 3
            d, m = (w & 7) | ((w >> 4) & 8), (w >> 3) & 15
            if op == 0:
                e = "%s + %s" % (r(m), r(d))
            elif op == 1:
                s.lines = ["psk_sub_flags(c, %s, %s);" % (r(d), r(m))]
                return s
            elif op == 2:
                e = r(m)
            else:
                t = hx((a + 4) & ~3) if m == 15 else "psk_bx(c->r[%d])" % m
                s.lines, s.flow = ["c->r[15] = %s;" % t, "return;"], "exit"
                return s
            if d == 15:
                s.lines, s.flow = ["c->r[15] = (%s & ~1u) | 1u;" % par(e), "return;"], "exit"
            else:
                s.lines = ["c->r[%d] = %s;" % (d, e)]
        elif (i >> 4) == 0b0110:
            ea = plus(r(rs3), ((w >> 6) & 31) << 2)
            if (w >> 11) & 1:
                s.lines = ["c->r[%d] = psk_ldr(%s);" % (rd3, ea)]
            else:
                s.lines = ["psk_w32(%s, c->r[%d]);" % (ea, rd3)]
        elif (i >> 3) == 0:
            s.lines = ["c->r[%d] = psk_nz(c, psk_lsl_s(c, %s, %du));" % (rd3, r(rs3), (w >> 6) & 31)]
        elif (i >> 2) == 0b010000:
            self.thumb_alu(w, s)
        elif (i >> 2) == 0b000110:
            fn = "psk_sub_flags" if (i >> 1) & 1 else "psk_add_flags"
            s.lines = ["c->r[%d] = %s(c, %s, %s);" % (rd3, fn, r(rs3), r((w >> 6) & 7))]
        elif (i >> 1) in (0b0101110, 0b0101010, 0b0101000, 0b0101011, 0b0101111, 0b0101001):
            ea = "%s + %s" % (r(rs3), r((w >> 6) & 7))
            k = i >> 1
            s.lines = [{0b0101001: "psk_w16((%s) & ~1u, (uint16_t)c->r[%d]);" % (ea, rd3),
                        0b0101110: "c->r[%d] = psk_r8(%s);" % (rd3, ea),
                        0b0101010: "psk_w8(%s, (uint8_t)c->r[%d]);" % (ea, rd3),
                        0b0101000: "psk_w32(%s, c->r[%d]);" % (ea, rd3),
                        0b0101011: "c->r[%d] = psk_ldrsb(%s);" % (rd3, ea),
                        0b0101111: "c->r[%d] = psk_ldrsh(%s);" % (rd3, ea)}[k]]
        elif (i >> 4) == 0b0111:
            ea = plus(r(rs3), (w >> 6) & 31)
            if (w >> 11) & 1:
                s.lines = ["c->r[%d] = psk_r8(%s);" % (rd3, ea)]
            else:
                s.lines = ["psk_w8(%s, (uint8_t)c->r[%d]);" % (ea, rd3)]
        elif (i >> 2) == 0b000111:
            fn = "psk_sub_flags" if (i >> 1) & 1 else "psk_add_flags"
            s.lines = ["c->r[%d] = %s(c, %s, %s);" % (rd3, fn, r(rs3), hx((w >> 6) & 7))]
        elif (i >> 3) == 0b11100:
            s.flow, s.target = "branch", a + 4 + (sx(w & 0x7FF, 11) << 1)
        elif (i >> 4) == 0b1010:
            d, k = (w >> 8) & 7, (w & 0xFF) << 2
            if (w >> 11) & 1:
                s.lines = ["c->r[%d] = %s;" % (d, plus("c->r[13]", k))]
            else:
                s.lines = ["c->r[%d] = %s;" % (d, hx(((a + 4) & ~3) + k))]
        elif i == 0xB0:
            k = (w & 0x7F) << 2
            s.lines = ["c->r[13] = %s;" % plus("c->r[13]", -k if w & 0x80 else k)]
        elif (i >> 3) == 0b10010:
            s.lines = ["psk_w32(%s, c->r[%d]);" % (plus("c->r[13]", (w & 0xFF) << 2), (w >> 8) & 7)]
        elif (i >> 3) == 0b10011:
            s.lines = ["c->r[%d] = psk_ldr(%s);" % ((w >> 8) & 7, plus("c->r[13]", (w & 0xFF) << 2))]
        else:
            raise LiftError("undefined Thumb %04X at %08X" % (w, a))
        return s

    def thumb_alu(self, w, s):
        d, m, op = w & 7, (w >> 3) & 7, (w >> 6) & 15
        x, y = "c->r[%d]" % d, "c->r[%d]" % m
        e = {0: "psk_nz(c, %s & %s)" % (x, y), 1: "psk_nz(c, %s ^ %s)" % (x, y),
             2: "psk_nz(c, psk_lsl_s(c, %s, %s))" % (x, y), 3: "psk_nz(c, psk_lsr_s(c, %s, %s))" % (x, y),
             4: "psk_nz(c, psk_asr_s(c, %s, %s))" % (x, y), 5: "psk_adc_flags(c, %s, %s)" % (x, y),
             6: "psk_sbc_flags(c, %s, %s)" % (x, y), 7: "psk_nz(c, psk_ror_s(c, %s, %s))" % (x, y),
             9: "psk_sub_flags(c, 0u, %s)" % y, 12: "psk_nz(c, %s | %s)" % (x, y),
             13: "psk_nz(c, %s * %s)" % (x, y), 14: "psk_nz(c, %s & ~%s)" % (x, y), 15: "psk_nz(c, ~%s)" % y}
        tests = {8: "psk_nz(c, %s & %s);" % (x, y), 10: "psk_sub_flags(c, %s, %s);" % (x, y),
                 11: "psk_add_flags(c, %s, %s);" % (x, y)}
        s.lines = [tests[op]] if op in tests else ["c->r[%d] = %s;" % (d, e[op])]


"""----------------------
 | lift_function
 | Description: Lifts one function to a C definition with its header block: gotos for in-function branches, continuation exits otherwise.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: data -- file bytes; base -- address of data[0]; start, end -- function span; thumb -- mode; idle -- branch addresses whose back edge idles
 | Returns: C text
 ----------------------"""
def lift_function(data, base, start, end, thumb, idle=IDLE_EDGES):
    lf = Lifter(data, base)
    steps = {}
    work = [start]
    while work:
        a = work.pop()
        if a in steps or not start <= a < end:
            continue
        st = lf.decode(a, thumb)
        steps[a] = st
        if st.flow == "branch" and start <= st.target < end:
            work.append(st.target)
        if st.flow == "next" or st.cond:
            work.append(a + st.size)
    key = (lambda t: t | 1) if thumb else (lambda t: t)
    starts = set(steps)
    labels = {st.target for st in steps.values() if st.flow == "branch" and st.target in starts}
    out, calls, swis, gl = [], set(), set(), set()
    if start == FRAME_POINT:
        out.append(("code", "psk_frame();"))
    for a in sorted(steps):
        st = steps[a]
        gl |= st.globals_
        for line in st.lines:
            if line.startswith("psk_call(c, 0x"):
                calls.add(line.split(",")[1].strip())
            if "psk_swi(" in line:
                swis.add(line.split("psk_swi(")[1][2:4])
        if a in labels:
            out.append(("label", "a_%08X:" % a))
        lines = list(st.lines)
        if st.flow == "branch":
            if st.target in starts:
                lines = (["psk_idle();"] if a in idle else []) + ["goto a_%08X;" % st.target]
            else:
                lines = ["c->r[15] = %s;" % hx(key(st.target)), "return;"]
        if st.cond:
            lines = ["if (psk_cond(c, %s)) {" % st.cond] + lines + ["}"]
        out += [("code", l) for l in lines]
        nxt = a + st.size
        if (st.flow == "next" or st.cond) and nxt not in starts:
            out += [("code", "c->r[15] = %s;" % hx(key(nxt))), ("code", "return;")]
    name = "rrl_func_%08X" % start
    desc = "Register-level C for the %s routine at %08X" % ("Thumb" if thumb else "ARM", start)
    if calls:
        desc += "; it calls %d routine%s" % (len(calls), "" if len(calls) == 1 else "s")
    if swis:
        desc += "%s SWI %s" % (" and" if calls else ";", ", ".join(sorted(swis)))
    text = ["/*----------------------",
            " | %s" % name,
            " | Description: %s." % desc,
            " | Author: suinevere",
            " | Dependencies: rrl.h",
            " | Globals: %s" % (", ".join("WRAM 0x%03X" % g for g in sorted(gl)) if gl else "N/A"),
            " | Params: c -- ARM register file; on return r15 holds the continuation (bit 0 = Thumb)",
            " | Returns: N/A",
            " ----------------------*/",
            "void %s(PskCpu *c) {" % name]
    depth = 1
    for kind, l in out:
        if kind == "label":
            text.append(l)
            continue
        if l.startswith("}"):
            depth -= 1
        text.append("    " * depth + l)
        if l.endswith("{"):
            depth += 1
    text.append("}")
    return "\n".join(text) + "\n"


"""----------------------
 | read_functions
 | Description: Reads functions.txt into (start, end, thumb) tuples.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- functions.txt
 | Returns: list of tuples
 ----------------------"""
def read_functions(path):
    out = []
    with open(path) as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            p = line.split()
            out.append((int(p[0], 16), int(p[1], 16), p[2] == "thumb"))
    return out


"""----------------------
 | hooked
 | Description: Addresses (Thumb bit dropped) already listed in a hooks file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- hooks.txt
 | Returns: set of addresses
 ----------------------"""
def hooked(path):
    out = set()
    if os.path.exists(path):
        with open(path) as f:
            for line in f:
                p = line.split("#", 1)[0].split()
                if p:
                    out.add(int(p[0], 16) & ~1)
    return out


"""----------------------
 | page_file
 | Description: The C file a lifted function goes to until it is grouped by concern: one per 4 KB of FLASH1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: src -- pocketstation/src/rrl; start -- function address
 | Returns: path
 ----------------------"""
def page_file(src, start):
    return os.path.join(src, "rrl_page%X.c" % ((start >> 12) & 0xF))


"""----------------------
 | page_header
 | Description: File header block for a new page file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- page file; start -- a function address in it
 | Returns: C text
 ----------------------"""
def page_header(path, start):
    lo = start & ~0xFFF
    return "\n".join(["/*----------------------",
                      " | %s" % os.path.basename(path),
                      " | Description: Ring Ring Land routines at %08X-%08X as register-level C, until they are named and grouped by concern." % (lo, lo + 0xFFF),
                      " | Author: suinevere",
                      " | Dependencies: rrl.h",
                      " | Globals: N/A",
                      " | Params: N/A",
                      " | Returns: N/A",
                      " ----------------------*/",
                      '#include "rrl.h"', ""])


"""----------------------
 | main
 | Description: CLI: lifts the next N unhooked functions in leaves-first order, or the listed addresses (a mid-function entry lifts to its function's end), into page files and hooks.txt.
 | Author: suinevere
 | Dependencies: psk_split.py
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--res", default=psk_file.RES_PATH)
    ap.add_argument("--build", default="pocketstation/build")
    ap.add_argument("--src", default="pocketstation/src/rrl")
    ap.add_argument("--next", type=int, default=0)
    ap.add_argument("--only", default="")
    ap.add_argument("--idle", default="")
    args = ap.parse_args(argv)
    import psk_split
    with open(args.res, "rb") as f:
        psk = psk_file.parse(f.read())
    cov = set()
    for path in glob.glob(os.path.join(args.build, "coverage", "*.txt")):
        with open(path) as f:
            cov.update(int(x, 16) for x in f.read().split())
    sp = psk_split.split(psk, cov)
    hooks_path = os.path.join(args.src, "hooks.txt")
    done = hooked(hooks_path)
    if args.only:
        todo = [int(x, 16) & ~1 for x in args.only.split(",")]
    else:
        groups = psk_split.cycle_groups(sp)
        todo = []
        for a in psk_split.leaves_first(sp):
            if a in done:
                continue
            if len(todo) >= args.next and groups[a] != groups[todo[-1]]:
                break
            todo.append(a)
    idle = IDLE_EDGES | {int(x, 16) for x in args.idle.split(",") if x}
    for a in todo:
        f = sp.functions.get(a) or next(g for g in sp.functions.values() if g.start < a < g.end)
        text = lift_function(psk.data, psk_file.FLASH1_BASE, a, f.end, f.thumb, idle)
        path = page_file(args.src, a)
        fresh = not os.path.exists(path)
        with open(path, "a", newline="\n") as out:
            if fresh:
                out.write(page_header(path, a))
            out.write("\n" + text)
        with open(hooks_path, "a", newline="\n") as out:
            out.write("%08X rrl_func_%08X regs\n" % (a | (1 if f.thumb else 0), a))
        print("lifted %08X %s" % (a, "thumb" if f.thumb else "arm"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
