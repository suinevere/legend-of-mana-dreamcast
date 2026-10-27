#----------------------
# | psx_oracle.py
# | Description: Test oracle that interprets the original MIPS libgte routines of SLUS_010.13, with a psx-spx GTE subset, to produce the PS1's exact results.
# | Author: suinevere
# | Dependencies: port/tools/libgte_tables.py, port/tools/disc_image.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "port", "tools"))

import disc_image  # noqa: E402
import libgte_tables  # noqa: E402

ENTRY = {
    "rsin": 0x8001C6AC,
    "rcos": 0x8001C77C,
    "ccos": 0x8001C81C,
    "csin": 0x8001CAAC,
    "SquareRoot0": 0x8001CC7C,
    "VectorNormalS": 0x8001CD98,
    "VectorNormal": 0x8001CDAC,
    "CompMatrix": 0x8001CFBC,
    "ApplyMatrixLV": 0x8001D11C,
    "ScaleMatrix": 0x8001D3FC,
    "RotMatrix": 0x8001D5EC,
    "RotMatrixX": 0x8001D87C,
    "RotMatrixY": 0x8001DA1C,
    "RotMatrixZ": 0x8001DBBC,
    "RotMatrix_gte": 0x8001DD5C,
    "ratan2": 0x8001DF6C,
}

RAM_BASE = 0x80000000
RAM_SIZE = 0x200000
RETURN_ADDRESS = 0x80000100
STACK_TOP = 0x801FFF00
ARG_BASE = 0x80180000
MAX_STEPS = 100000
MASK = 0xFFFFFFFF


class Trap(Exception):
    pass


class PS1Exception(Trap):
    pass


def s32(value):
    value &= MASK
    return value - 0x100000000 if value & 0x80000000 else value


def s16(value):
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def default_image():
    return os.environ.get("LOM_PS1_IMAGE", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..",
                                                        "disc", "us", "LegendOfMana.bin"))


def load_exe(image_path):
    return libgte_tables.read_iso_file(disc_image.DiscImage(image_path), libgte_tables.EXE_NAME)


class Machine:
    def __init__(self, exe):
        self.ram = bytearray(RAM_SIZE)
        load = struct.unpack_from("<I", exe, 0x18)[0]
        body = exe[libgte_tables.EXE_HEADER_SIZE:]
        self.ram[load - RAM_BASE:load - RAM_BASE + len(body)] = body
        self.reset_gte()

    def reset_gte(self):
        self.v = [[0, 0, 0] for _ in range(3)]
        self.ir = [0, 0, 0, 0]
        self.mac = [0, 0, 0, 0]
        self.lzcs = 0
        self.lzcr = 0
        self.ctrl = [0] * 32

    def read(self, address, size, signed=False):
        offset = (address & 0x1FFFFFFF) - (RAM_BASE & 0x1FFFFFFF)
        value = int.from_bytes(self.ram[offset:offset + size], "little")
        if signed and value & (1 << (size * 8 - 1)):
            value -= 1 << (size * 8)
        return value

    def write(self, address, size, value):
        offset = (address & 0x1FFFFFFF) - (RAM_BASE & 0x1FFFFFFF)
        self.ram[offset:offset + size] = (value & ((1 << (size * 8)) - 1)).to_bytes(size, "little")

    def rt(self):
        c = self.ctrl
        return [[s16(c[0]), s16(c[0] >> 16), s16(c[1])],
                [s16(c[1] >> 16), s16(c[2]), s16(c[2] >> 16)],
                [s16(c[3]), s16(c[3] >> 16), s16(c[4])]]

    def mfc2(self, reg):
        if reg in (0, 2, 4):
            vec = self.v[reg // 2]
            return ((vec[0] & 0xFFFF) | ((vec[1] & 0xFFFF) << 16)) & MASK
        if reg in (1, 3, 5):
            return self.v[reg // 2][2] & MASK
        if 8 <= reg <= 11:
            return self.ir[reg - 8] & MASK
        if 24 <= reg <= 27:
            return self.mac[reg - 24] & MASK
        if reg == 30:
            return self.lzcs & MASK
        if reg == 31:
            return self.lzcr
        raise Trap("mfc2 $%d" % reg)

    def mtc2(self, reg, value):
        if reg in (0, 2, 4):
            self.v[reg // 2][0] = s16(value)
            self.v[reg // 2][1] = s16(value >> 16)
        elif reg in (1, 3, 5):
            self.v[reg // 2][2] = s16(value)
        elif 8 <= reg <= 11:
            self.ir[reg - 8] = s16(value)
        elif 24 <= reg <= 27:
            self.mac[reg - 24] = s32(value)
        elif reg == 30:
            self.lzcs = s32(value)
            bits = value & MASK
            sign = bits >> 31
            count = 0
            while count < 32 and ((bits >> (31 - count)) & 1) == sign:
                count += 1
            self.lzcr = count
        else:
            raise Trap("mtc2 $%d" % reg)

    def mac_step(self, value):
        value &= (1 << 44) - 1
        return value - (1 << 44) if value & (1 << 43) else value

    def store_mac_ir(self, sums, sf, lm):
        lo = 0 if lm else -0x8000
        for i in range(3):
            shifted = sums[i] >> (12 * sf)
            self.mac[i + 1] = s32(shifted)
            self.ir[i + 1] = max(lo, min(0x7FFF, shifted))

    def cop2(self, word):
        cmd = word & 0x3F
        sf = (word >> 19) & 1
        lm = (word >> 10) & 1
        if cmd == 0x12:
            mx, vsel, cv = (word >> 17) & 3, (word >> 15) & 3, (word >> 13) & 3
            if mx != 0 or cv != 3:
                raise Trap("MVMVA mx=%d cv=%d" % (mx, cv))
            vec = self.ir[1:4] if vsel == 3 else self.v[vsel]
            matrix = self.rt()
            sums = []
            for row in matrix:
                acc = 0
                for j in range(3):
                    acc = self.mac_step(acc + row[j] * vec[j])
                sums.append(acc)
            self.store_mac_ir(sums, sf, lm)
        elif cmd == 0x28:
            self.store_mac_ir([self.ir[i] * self.ir[i] for i in (1, 2, 3)], sf, lm)
        elif cmd == 0x3D:
            self.store_mac_ir([self.mac_step(self.ir[0] * self.ir[i]) for i in (1, 2, 3)], sf, lm)
        else:
            raise Trap("cop2 0x%07X" % word)

    def call(self, name, *args):
        regs = [0] * 32
        for i, value in enumerate(args):
            regs[4 + i] = value & MASK
        regs[29] = STACK_TOP
        regs[31] = RETURN_ADDRESS
        hi = lo = 0
        pc = ENTRY[name]
        npc = pc + 4
        for _ in range(MAX_STEPS):
            if pc == RETURN_ADDRESS:
                return s32(regs[2])
            w = self.read(pc, 4)
            pc, npc = npc, npc + 4
            op, rs, rt, rd = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
            sa, fn, imm = (w >> 6) & 31, w & 63, w & 0xFFFF
            simm = s16(imm)
            a, b = regs[rs], regs[rt]
            out = None
            target = None
            if op == 0:
                if fn == 0:
                    out = b << sa
                elif fn == 2:
                    out = b >> sa
                elif fn == 3:
                    out = s32(b) >> sa
                elif fn == 4:
                    out = b << (a & 31)
                elif fn == 6:
                    out = b >> (a & 31)
                elif fn == 7:
                    out = s32(b) >> (a & 31)
                elif fn == 8:
                    target = a
                elif fn == 9:
                    target = a
                    out = pc + 4
                elif fn == 13:
                    raise PS1Exception("break at 0x%08X" % (pc - 4))
                elif fn == 16:
                    out = hi
                elif fn == 18:
                    out = lo
                elif fn in (24, 25):
                    product = (s32(a) * s32(b)) if fn == 24 else (a * b)
                    lo, hi = product & MASK, (product >> 32) & MASK
                elif fn in (26, 27):
                    if b == 0:
                        raise PS1Exception("divide by zero")
                    if fn == 26:
                        q = abs(s32(a)) // abs(s32(b))
                        q = q if (s32(a) < 0) == (s32(b) < 0) else -q
                        lo, hi = q & MASK, (s32(a) - q * s32(b)) & MASK
                    else:
                        lo, hi = a // b, a % b
                elif fn in (32, 34):
                    full = s32(a) + s32(b) if fn == 32 else s32(a) - s32(b)
                    if full != s32(full):
                        raise PS1Exception("integer overflow at 0x%08X" % (pc - 4))
                    out = full
                elif fn == 33:
                    out = a + b
                elif fn == 35:
                    out = a - b
                elif fn == 36:
                    out = a & b
                elif fn == 37:
                    out = a | b
                elif fn == 38:
                    out = a ^ b
                elif fn == 39:
                    out = ~(a | b)
                elif fn == 42:
                    out = int(s32(a) < s32(b))
                elif fn == 43:
                    out = int(a < b)
                else:
                    raise Trap("special %d" % fn)
                if out is not None:
                    regs[rd] = out & MASK
            elif op == 1:
                taken = s32(a) < 0 if rt in (0, 16) else s32(a) >= 0
                if taken:
                    target = (pc + (simm << 2)) & MASK
            elif op in (2, 3):
                target = (pc & 0xF0000000) | ((w & 0x3FFFFFF) << 2)
                if op == 3:
                    regs[31] = (pc + 4) & MASK
            elif op in (4, 5, 6, 7):
                taken = {4: a == b, 5: a != b, 6: s32(a) <= 0, 7: s32(a) > 0}[op]
                if taken:
                    target = (pc + (simm << 2)) & MASK
            elif op == 8:
                full = s32(a) + simm
                if full != s32(full):
                    raise PS1Exception("integer overflow at 0x%08X" % (pc - 4))
                regs[rt] = full & MASK
            elif op == 9:
                regs[rt] = (a + simm) & MASK
            elif op == 10:
                regs[rt] = int(s32(a) < simm)
            elif op == 11:
                regs[rt] = int(a < (simm & MASK))
            elif op == 12:
                regs[rt] = a & imm
            elif op == 13:
                regs[rt] = a | imm
            elif op == 14:
                regs[rt] = a ^ imm
            elif op == 15:
                regs[rt] = (imm << 16) & MASK
            elif op in (32, 33, 35, 36, 37):
                size = {32: 1, 33: 2, 35: 4, 36: 1, 37: 2}[op]
                regs[rt] = self.read((a + simm) & MASK, size, op in (32, 33)) & MASK
            elif op in (40, 41, 43):
                self.write((a + simm) & MASK, {40: 1, 41: 2, 43: 4}[op], b)
            elif op == 50:
                self.mtc2(rt, self.read((a + simm) & MASK, 4))
            elif op == 58:
                self.write((a + simm) & MASK, 4, self.mfc2(rt))
            elif op == 18:
                if rs & 0x10:
                    self.cop2(w & 0x1FFFFFF)
                elif rs == 0:
                    regs[rt] = self.mfc2(rd)
                elif rs == 2:
                    regs[rt] = self.ctrl[rd] if rd != 4 else s16(self.ctrl[4]) & MASK
                elif rs == 4:
                    self.mtc2(rd, b)
                elif rs == 6:
                    self.ctrl[rd] = b
                else:
                    raise Trap("cop2 rs %d" % rs)
            else:
                raise Trap("opcode %d at 0x%08X" % (op, pc - 4))
            regs[0] = 0
            if target is not None:
                npc = target
        raise Trap("no return from %s" % name)

    def put_matrix(self, address, values):
        for i in range(9):
            self.write(address + 2 * i, 2, values[i])
        self.write(address + 18, 2, values[9])
        for i in range(3):
            self.write(address + 20 + 4 * i, 4, values[10 + i])

    def get_matrix(self, address):
        return ([self.read(address + 2 * i, 2, True) for i in range(9)] + [self.read(address + 18, 2, True)]
                + [self.read(address + 20 + 4 * i, 4, True) for i in range(3)])

    def put_vector(self, address, values):
        for i in range(3):
            self.write(address + 4 * i, 4, values[i])
        self.write(address + 12, 4, 0)

    def fill(self, address, size, value):
        for i in range(size):
            self.write(address + i, 1, value)

    def gte_state(self):
        return self.ir[:] + self.mac[1:4] + [self.lzcs, self.lzcr]

    def run_case(self, case):
        name, args = case[0], [int(x) for x in case[1:]]
        self.reset_gte()
        m0, m1, m2, v0, v1 = ARG_BASE, ARG_BASE + 0x40, ARG_BASE + 0x80, ARG_BASE + 0xC0, ARG_BASE + 0x100
        if name in ("rsin", "rcos", "csin", "ccos", "SquareRoot0"):
            out = [self.call(name, args[0])]
        elif name == "ratan2":
            out = [self.call(name, args[0], args[1])]
        elif name in ("VectorNormal", "VectorNormalS"):
            self.put_vector(v0, args[0:3])
            self.fill(v1, 16, 0x11)
            out = [self.call(name, v0, v1)]
            if name == "VectorNormal":
                out += [self.read(v1 + 4 * i, 4, True) for i in range(4)]
            else:
                out += [self.read(v1 + 2 * i, 2, True) for i in range(4)]
        elif name in ("RotMatrix", "RotMatrix_gte"):
            for i in range(3):
                self.write(v0 + 2 * i, 2, args[i])
            self.put_matrix(m0, args[3:16])
            self.call(name, v0, m0)
            out = self.get_matrix(m0)
        elif name in ("RotMatrixX", "RotMatrixY", "RotMatrixZ"):
            self.put_matrix(m0, args[1:14])
            self.call(name, args[0], m0)
            out = self.get_matrix(m0)
        elif name == "ApplyMatrixLV":
            self.put_matrix(m0, args[0:13])
            self.put_vector(v0, args[13:16])
            self.fill(v1, 16, 0x11)
            self.call(name, m0, v0, v1)
            out = [self.read(v1 + 4 * i, 4, True) for i in range(4)]
        elif name == "CompMatrix":
            self.put_matrix(m0, args[0:13])
            self.put_matrix(m1, args[13:26])
            self.put_matrix(m2, args[26:39])
            self.call(name, m0, m1, m2)
            out = self.get_matrix(m2)
        elif name == "ScaleMatrix":
            self.put_matrix(m0, args[0:13])
            self.put_vector(v0, args[13:16])
            self.call(name, m0, v0)
            out = self.get_matrix(m0)
        else:
            raise KeyError(name)
        return out + self.gte_state()
