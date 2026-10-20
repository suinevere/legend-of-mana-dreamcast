"""----------------------
 | callgraph.py
 | Description: Call levels and callers of R1's register-level Ring Ring Land functions, from psk_call sites, r15 continuations and dispatch tables.
 | Author: suinevere
 | Dependencies: pocketstation/src/rrl/hooks.txt, pocketstation/src/rrl/rrl_page*.c, pocketstation/build/rrl.card
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import glob
import os
import re
import struct
import sys

"""----------------------
 | RRL
 | Description: R1's decompiled C directory.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
RRL = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "rrl")

"""----------------------
 | FUNC_RE
 | Description: A register-level function definition and its body.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FUNC_RE = re.compile(r"\nvoid (\w+)\(PskCpu \*c\) \{(.*?)\n\}\n", re.S)

"""----------------------
 | CALL_RE
 | Description: A psk_call target, a continuation, or a routine address loaded into any register (veneers and handler pointers).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
CALL_RE = re.compile(r"(?:psk_call\(c, |c->r\[\d+\] = )0x([0-9A-F]+)u")

"""----------------------
 | INDIRECT_RE
 | Description: An indirect call or jump through a register other than lr.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
INDIRECT_RE = re.compile(r"psk_call\(c, psk_bx\(|c->r\[15\] = psk_bx\(c->r\[(?!14)")

"""----------------------
 | CONST_RE
 | Description: A constant in the app's address range, a candidate dispatch table base.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
CONST_RE = re.compile(r"= 0x(020[0-9A-F]{5})u;")

"""----------------------
 | CARD
 | Description: The reference card image; app address 0x02000000 is card offset 0x2000.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
CARD = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build", "rrl.card")

"""----------------------
 | CODE_START
 | Description: First address of Ring Ring Land's code.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
CODE_START = 0x02000000

"""----------------------
 | CODE_END
 | Description: End of Ring Ring Land's code: the dispatch tables start at 0x02008470.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
CODE_END = 0x02008470


"""----------------------
 | table
 | Description: The hooked functions listed in a dispatch table, read from the card while words point into the code; unhooked code entries are skipped.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: card -- card image; base -- table address; by_addr -- name by address without bit 0
 | Returns: list of names
 ----------------------"""
def table(card, base, by_addr):
    out, i = [], base - 0x02000000 + 0x2000
    while 0 <= i and i + 4 <= len(card):
        v = struct.unpack_from("<I", card, i)[0]
        if not CODE_START <= (v & ~1) < CODE_END:
            break
        if (v & ~1) in by_addr:
            out.append(by_addr[v & ~1])
        i += 4
    return out


"""----------------------
 | graph
 | Description: Reads R1's hooks, pages and dispatch tables into per-function address, line count and callee names.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: (addr by name, lines by name, callees by name)
 ----------------------"""
def graph():
    by_addr, addr = {}, {}
    with open(os.path.join(RRL, "hooks.txt")) as f:
        for line in f:
            p = line.split("#", 1)[0].split()
            if len(p) >= 3:
                by_addr[int(p[0], 16) & ~1] = p[1]
                addr[p[1]] = int(p[0], 16)
    with open(CARD, "rb") as f:
        card = f.read()
    lines, calls = {}, {}
    for path in sorted(glob.glob(os.path.join(RRL, "rrl_page*.c"))):
        with open(path) as f:
            src = f.read()
        for m in FUNC_RE.finditer(src):
            name, body = m.group(1), m.group(2)
            lines[name] = body.count("\n")
            calls[name] = {by_addr[int(t, 16) & ~1] for t in CALL_RE.findall(body) if int(t, 16) & ~1 in by_addr}
            if INDIRECT_RE.search(body):
                for t in CONST_RE.findall(body):
                    if int(t, 16) & ~1 not in by_addr:
                        calls[name].update(table(card, int(t, 16), by_addr))
            calls[name].discard(name)
    return addr, lines, calls


"""----------------------
 | levels
 | Description: Call level of every function (leaves 0); a cycle member counts as a leaf where it is first reached.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: list of (level, name, address, lines) sorted by level then address
 ----------------------"""
def levels():
    addr, lines, calls = graph()
    level = {}

    def walk(name, stack):
        if name in level:
            return level[name]
        if name in stack:
            return 0
        level[name] = 1 + max((walk(c, stack | {name}) for c in calls[name] if c != name), default=-1)
        return level[name]

    for name in lines:
        walk(name, frozenset())
    return sorted(((level[n], n, addr.get(n, 0), lines[n]) for n in lines), key=lambda t: (t[0], t[2]))


"""----------------------
 | main
 | Description: CLI: callgraph.py levels | callers NAME
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if argv[:1] == ["levels"]:
        for level, name, a, n in levels():
            print("%d %s %08X %d" % (level, name, a, n))
        return 0
    if len(argv) == 2 and argv[0] == "callers":
        _, _, calls = graph()
        for name in sorted(n for n, cs in calls.items() if argv[1] in cs):
            print(name)
        return 0
    print("usage: callgraph.py levels | callers NAME", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
