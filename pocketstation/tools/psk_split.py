"""----------------------
 | psk_split.py
 | Description: Splits a PocketStation program body into ARM and Thumb functions, literal pools, data and unknown bytes by recursive descent.
 | Author: suinevere
 | Dependencies: capstone, psk_file.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import argparse
import bisect
import glob
import os
import struct
import sys
from dataclasses import dataclass, field

import capstone
from capstone import arm as carm

import psk_file

"""----------------------
 | IO_LOW
 | Description: Lowest address counted as I/O (interrupt controller and up).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
IO_LOW = 0x0A000000

"""----------------------
 | IO_HIGH
 | Description: One past the highest I/O address.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
IO_HIGH = 0x10000000


"""----------------------
 | Function
 | Description: One function: span, mode, outgoing calls (bit 0 = Thumb), callers, SWI numbers and I/O addresses used.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class Function:
    start: int
    end: int
    thumb: bool
    calls: list = field(default_factory=list)
    callers: list = field(default_factory=list)
    swis: list = field(default_factory=list)
    io: list = field(default_factory=list)


"""----------------------
 | Split
 | Description: Result of a split: functions by start address, data spans and unknown spans.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class Split:
    functions: dict
    data: list
    unknown: list


"""----------------------
 | _Decoder
 | Description: Capstone ARM and Thumb decoders over the file mapped at FLASH1.
 | Author: suinevere
 | Dependencies: capstone
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class _Decoder:
    def __init__(self, data: bytes):
        self.data = data
        self.arm = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_ARM)
        self.thumb = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
        for cs in (self.arm, self.thumb):
            cs.detail = True
        self.cache = {}

    def word(self, addr: int):
        off = addr - psk_file.FLASH1_BASE
        if off < 0 or off + 4 > len(self.data):
            return None
        return struct.unpack_from("<I", self.data, off)[0]

    def insn(self, addr: int, thumb: bool):
        key = (addr, thumb)
        if key not in self.cache:
            off = addr - psk_file.FLASH1_BASE
            cs = self.thumb if thumb else self.arm
            got = next(cs.disasm(self.data[off:off + 4], addr, 1), None) if 0 <= off < len(self.data) else None
            self.cache[key] = got
        return self.cache[key]


"""----------------------
 | _literal
 | Description: For a PC-relative load, the literal address and its size; None for anything else.
 | Author: suinevere
 | Dependencies: capstone
 | Globals: N/A
 | Params: i -- decoded instruction; thumb -- mode
 | Returns: (address, size) or None
 ----------------------"""
def _literal(i, thumb):
    if not i.mnemonic.startswith("ldr") or len(i.operands) < 2:
        return None
    op = i.operands[1]
    if op.type != carm.ARM_OP_MEM or op.mem.base != carm.ARM_REG_PC or op.mem.index != 0:
        return None
    disp = op.mem.disp
    if getattr(op, "subtracted", False) and disp > 0:
        disp = -disp
    pc = ((i.address + 4) & ~3) if thumb else i.address + 8
    size = 1 if i.mnemonic.startswith(("ldrb", "ldrsb")) else 2 if i.mnemonic.startswith(("ldrh", "ldrsh")) else 4
    return pc + disp, size


"""----------------------
 | _writes
 | Description: Registers an instruction writes.
 | Author: suinevere
 | Dependencies: capstone
 | Globals: N/A
 | Params: i -- decoded instruction
 | Returns: set of capstone register ids
 ----------------------"""
def _writes(i):
    try:
        return set(i.regs_access()[1])
    except capstone.CsError:
        return set()


"""----------------------
 | _decode_function
 | Description: Recursive descent over one function region; collects instructions, branch targets, calls, literals and uses.
 | Author: suinevere
 | Dependencies: capstone
 | Globals: N/A
 | Params: dec -- decoder; start, thumb -- function; limit -- end of its region; pools -- known literal bytes; roots -- extra entry points
 | Returns: dict of results
 ----------------------"""
def _decode_function(dec, start, thumb, limit, pools, roots):
    seen = {}
    work = [start] + sorted(roots)
    seeds = []
    calls = []
    lits = {}
    swis = []
    io = []
    while work:
        a = work.pop()
        first = True
        while True:
            if a == limit and not first and a not in pools and dec.word(a & ~3) is not None:
                seeds.append((a, thumb))
                calls.append(a | (1 if thumb else 0))
            first = False
            if a in seen or a < start or a >= limit or a in pools:
                break
            i = dec.insn(a, thumb)
            if i is None:
                break
            seen[a] = i.size
            nxt = a + i.size
            m = i.mnemonic
            ids = i.id
            lit = _literal(i, thumb)
            if lit is not None:
                lits[lit[0]] = lit[1]
                v = dec.word(lit[0]) if lit[1] == 4 else None
                if v is not None and IO_LOW <= v < IO_HIGH:
                    io.append(v)
            for op in i.operands:
                if op.type == carm.ARM_OP_IMM and IO_LOW <= (op.imm & 0xFFFFFFFF) < IO_HIGH and m not in ("b", "bl"):
                    io.append(op.imm & 0xFFFFFFFF)
            if ids == carm.ARM_INS_SVC:
                swis.append(i.operands[0].imm & 0xFF)
            if ids == carm.ARM_INS_BL:
                t = i.operands[0].imm
                seeds.append((t, thumb))
                calls.append(t | (1 if thumb else 0))
                a = nxt
                continue
            if ids == carm.ARM_INS_B:
                t = i.operands[0].imm
                cond = i.cc not in (carm.ARM_CC_AL, carm.ARM_CC_INVALID)
                if start <= t < limit:
                    work.append(t)
                else:
                    seeds.append((t, thumb))
                    calls.append(t | (1 if thumb else 0))
                if cond:
                    a = nxt
                    continue
                break
            if ids == carm.ARM_INS_BX:
                reg = i.operands[0].reg
                cond = i.cc not in (carm.ARM_CC_AL, carm.ARM_CC_INVALID)
                linked = _linked(dec, a, thumb)
                if reg == carm.ARM_REG_PC and thumb:
                    t = (a + 4) & ~3
                    seeds.append((t, False))
                    calls.append(t)
                    break
                v = _loaded(dec, a, thumb, reg, start)
                if v is not None and psk_file.FLASH1_BASE <= v < psk_file.FLASH1_BASE + len(dec.data):
                    seeds.append((v & ~1, bool(v & 1)))
                    calls.append(v)
                if linked or cond:
                    a = nxt
                    continue
                break
            if _ends_path(i, thumb):
                if i.cc not in (carm.ARM_CC_AL, carm.ARM_CC_INVALID) or _linked(dec, a, thumb):
                    a = nxt
                    continue
                break
            a = nxt
    return {"seen": seen, "seeds": seeds, "calls": calls, "lits": lits, "swis": swis, "io": io}


"""----------------------
 | _linked
 | Description: Whether the instruction before addr is `mov lr, pc`, making the jump at addr a call that returns.
 | Author: suinevere
 | Dependencies: capstone
 | Globals: N/A
 | Params: dec -- decoder; addr -- jump address; thumb -- mode
 | Returns: bool
 ----------------------"""
def _linked(dec, addr, thumb):
    p = dec.insn(addr - (2 if thumb else 4), thumb)
    return p is not None and p.mnemonic == "mov" and p.op_str.replace(" ", "") == "lr,pc"


"""----------------------
 | _loaded
 | Description: The literal value a register holds at a BX, if a PC-relative load set it in the few instructions before.
 | Author: suinevere
 | Dependencies: capstone
 | Globals: N/A
 | Params: dec -- decoder; addr -- BX address; thumb -- mode; reg -- capstone register; start -- function start
 | Returns: the value or None
 ----------------------"""
def _loaded(dec, addr, thumb, reg, start):
    step = 2 if thumb else 4
    for back in range(1, 5):
        p_addr = addr - back * step
        if p_addr < start:
            return None
        p = dec.insn(p_addr, thumb)
        if p is None:
            return None
        if reg in _writes(p):
            lit = _literal(p, thumb)
            if lit is None or lit[1] != 4:
                return None
            return dec.word(lit[0])
    return None


"""----------------------
 | _ends_path
 | Description: Whether an instruction leaves the function: a return, or an indirect jump through PC.
 | Author: suinevere
 | Dependencies: capstone
 | Globals: N/A
 | Params: i -- decoded instruction; thumb -- mode
 | Returns: bool
 ----------------------"""
def _ends_path(i, thumb):
    if i.id in (carm.ARM_INS_POP, carm.ARM_INS_LDM, carm.ARM_INS_LDMIB, carm.ARM_INS_LDMDA, carm.ARM_INS_LDMDB):
        return any(op.type == carm.ARM_OP_REG and op.reg == carm.ARM_REG_PC for op in i.operands)
    return carm.ARM_REG_PC in _writes(i)


"""----------------------
 | _spans
 | Description: Merges a set of byte addresses into sorted half-open spans.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: addrs -- byte addresses
 | Returns: list of (start, end)
 ----------------------"""
def _spans(addrs):
    out = []
    for a in sorted(addrs):
        if out and out[-1][1] == a:
            out[-1][1] = a + 1
        else:
            out.append([a, a + 1])
    return [tuple(s) for s in out]


"""----------------------
 | split
 | Description: Finds every function reachable from the entry, calls, interworking literals and covered addresses, then partitions the body.
 | Author: suinevere
 | Dependencies: capstone, psk_file.py
 | Globals: N/A
 | Params: psk -- parsed file; coverage -- executed addresses (bit 0 = Thumb)
 | Returns: Split; raises ValueError when one address is needed as both ARM and Thumb
 ----------------------"""
def split(psk, coverage=frozenset()) -> Split:
    dec = _Decoder(psk.data)
    base = psk_file.FLASH1_BASE
    body0, body1 = base + psk.body_offset, base + len(psk.data)
    seeds = {psk.entry & ~1: bool(psk.entry & 1)}
    pools = set()
    while True:
        starts = sorted(seeds)
        pool_list = sorted(pools)
        limits = {}
        for k, s in enumerate(starts):
            lim = starts[k + 1] if k + 1 < len(starts) else body1
            j = bisect.bisect_right(pool_list, s)
            if j < len(pool_list) and pool_list[j] < lim:
                lim = pool_list[j]
            limits[s] = lim
        results = {s: _decode_function(dec, s, seeds[s], limits[s], pools, ()) for s in starts}
        grew = False
        new = {}
        decoded = set()
        for r in results.values():
            for a, n in r["seen"].items():
                decoded.update(range(a, a + n))
        for c in sorted(coverage):
            a, th = c & ~1, bool(c & 1)
            if not body0 <= a < body1 or a in pools or a in decoded or a in seeds:
                continue
            k = bisect.bisect_right(starts, a) - 1
            lim = starts[k + 1] if k + 1 < len(starts) else body1
            j = bisect.bisect_right(pool_list, a)
            if j < len(pool_list) and pool_list[j] < lim:
                lim = pool_list[j]
            new[a] = th
            probe = _decode_function(dec, a, th, lim, pools, ())
            for b, n in probe["seen"].items():
                decoded.update(range(b, b + n))
        for s, r in results.items():
            for t, th in r["seeds"]:
                if not body0 <= t < body1:
                    continue
                if t in seeds and seeds[t] != th:
                    raise ValueError("%08X needed as both ARM and Thumb" % t)
                if t not in seeds and new.get(t, th) == th:
                    new[t] = th
            for lit, size in r["lits"].items():
                for b in range(lit, lit + size):
                    if body0 <= b < body1 and b not in pools:
                        pools.add(b)
                        grew = True
        for t, th in new.items():
            if t not in seeds:
                seeds[t] = th
                grew = True
        if not grew:
            break
    functions = {}
    code = set()
    for s in starts:
        r = results[s]
        if not r["seen"]:
            continue
        end = max(a + n for a, n in r["seen"].items())
        functions[s] = Function(s, end, seeds[s], list(dict.fromkeys(r["calls"])), [],
                                sorted(set(r["swis"])), sorted(set(r["io"])))
        code.update(range(s, end))
    for f in functions.values():
        for c in f.calls:
            g = functions.get(c & ~1)
            if g is not None and f.start not in g.callers:
                g.callers.append(f.start)
    for f in functions.values():
        f.callers.sort()
    data = set(pools) - code
    gaps = _spans(set(range(body0, body1)) - code - data)
    refs = set()
    for b in sorted(pools):
        v = dec.word(b)
        if v is not None:
            refs.add(v)
    changed = True
    while changed:
        changed = False
        for g0, g1 in list(gaps):
            if any(g0 <= v < g1 for v in refs):
                gaps.remove((g0, g1))
                data.update(range(g0, g1))
                for w in range(g0 & ~3, g1, 4):
                    v = dec.word(w)
                    if v is not None:
                        refs.add(v)
                changed = True
    return Split(functions, _spans(data), gaps)


"""----------------------
 | _sccs
 | Description: Tarjan's strongly connected components of the call graph, emitted callees first.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: s -- Split
 | Returns: list of components, each a sorted list of function starts
 ----------------------"""
def _sccs(s: Split):
    index, low, on, stack, out = {}, {}, set(), [], []
    counter = [0]
    edges = {a: [c & ~1 for c in f.calls if (c & ~1) in s.functions] for a, f in s.functions.items()}
    for root in sorted(s.functions):
        if root in index:
            continue
        work = [(root, 0)]
        while work:
            v, k = work.pop()
            if k == 0:
                index[v] = low[v] = counter[0]
                counter[0] += 1
                stack.append(v)
                on.add(v)
            recurse = False
            for j in range(k, len(edges[v])):
                w = edges[v][j]
                if w not in index:
                    work.append((v, j + 1))
                    work.append((w, 0))
                    recurse = True
                    break
                if w in on:
                    low[v] = min(low[v], index[w])
            if recurse:
                continue
            if low[v] == index[v]:
                comp = []
                while True:
                    w = stack.pop()
                    on.discard(w)
                    comp.append(w)
                    if w == v:
                        break
                out.append(sorted(comp))
            if work:
                u = work[-1][0]
                low[u] = min(low[u], low[v])
    return out


"""----------------------
 | cycle_groups
 | Description: Maps each function start to the index of its call-graph cycle group.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: s -- Split
 | Returns: dict start -> group index
 ----------------------"""
def cycle_groups(s: Split) -> dict:
    return {a: k for k, comp in enumerate(_sccs(s)) for a in comp}


"""----------------------
 | leaves_first
 | Description: Function starts ordered so every callee precedes its callers, members of a cycle kept together.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: s -- Split
 | Returns: list of starts
 ----------------------"""
def leaves_first(s: Split) -> list:
    return [a for comp in _sccs(s) for a in comp]


"""----------------------
 | write_listings
 | Description: Writes one func_XXXXXXXX.s per function with bytes, disassembly and literal values.
 | Author: suinevere
 | Dependencies: capstone
 | Globals: N/A
 | Params: s -- Split; psk -- parsed file; out_dir -- output directory
 | Returns: N/A
 ----------------------"""
def write_listings(s: Split, psk, out_dir: str) -> None:
    os.makedirs(out_dir, exist_ok=True)
    dec = _Decoder(psk.data)
    for f in s.functions.values():
        lines = ["@ func_%08X %s %08X-%08X" % (f.start, "thumb" if f.thumb else "arm", f.start, f.end),
                 "@ calls " + " ".join("%08X" % c for c in f.calls),
                 "@ callers " + " ".join("%08X" % c for c in f.callers),
                 "@ swis " + " ".join("%02X" % n for n in f.swis),
                 "@ io " + " ".join("%08X" % v for v in f.io)]
        a = f.start
        while a < f.end:
            i = dec.insn(a, f.thumb)
            if i is None:
                lines.append("%08X: %s  .byte" % (a, psk.data[a - psk_file.FLASH1_BASE:a - psk_file.FLASH1_BASE + 2].hex()))
                a += 2 if f.thumb else 4
                continue
            note = ""
            lit = _literal(i, f.thumb)
            if lit is not None and lit[1] == 4:
                v = dec.word(lit[0])
                note = "  @ [%08X] = %08X" % (lit[0], v if v is not None else 0)
            lines.append("%08X: %-9s %-7s %s%s" % (a, i.bytes.hex(), i.mnemonic, i.op_str, note))
            a += i.size
        with open(os.path.join(out_dir, "func_%08X.s" % f.start), "w") as out:
            out.write("\n".join(lines) + "\n")


"""----------------------
 | write_functions
 | Description: Writes functions.txt: start, end, mode, calls, SWIs and I/O per line.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: s -- Split; path -- output file
 | Returns: N/A
 ----------------------"""
def write_functions(s: Split, path: str) -> None:
    with open(path, "w") as out:
        out.write("# start end mode calls=... swis=... io=...\n")
        for a in sorted(s.functions):
            f = s.functions[a]
            out.write("%08X %08X %s calls=%s swis=%s io=%s\n" % (
                f.start, f.end, "thumb" if f.thumb else "arm",
                ",".join("%08X" % c for c in f.calls), ",".join("%02X" % n for n in f.swis),
                ",".join("%08X" % v for v in f.io)))


"""----------------------
 | main
 | Description: CLI: splits RES1506 with every coverage file, writes functions.txt and listings, prints a summary.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--res", default=psk_file.RES_PATH)
    ap.add_argument("--build", default="pocketstation/build")
    args = ap.parse_args(argv)
    with open(args.res, "rb") as f:
        psk = psk_file.parse(f.read())
    cov = set()
    for path in glob.glob(os.path.join(args.build, "coverage", "*.txt")):
        with open(path) as f:
            cov.update(int(x, 16) for x in f.read().split())
    s = split(psk, cov)
    write_functions(s, os.path.join(args.build, "functions.txt"))
    write_listings(s, psk, os.path.join(args.build, "asm"))
    fs = list(s.functions.values())
    swis = sorted({n for f in fs for n in f.swis})
    io = sorted({v for f in fs for v in f.io})
    print("functions %d (arm %d, thumb %d)" % (len(fs), sum(not f.thumb for f in fs), sum(f.thumb for f in fs)))
    print("code %d data %d unknown %d bytes" % (sum(f.end - f.start for f in fs), sum(b - a for a, b in s.data),
                                                 sum(b - a for a, b in s.unknown)))
    print("swis " + " ".join("%02X" % n for n in swis))
    print("io " + " ".join("%08X" % v for v in io))
    return 0


if __name__ == "__main__":
    sys.exit(main())
