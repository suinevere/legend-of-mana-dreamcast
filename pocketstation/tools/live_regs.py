"""----------------------
 | live_regs.py
 | Description: Interprocedural register and flag liveness over R1's register-level C, giving each routine's inputs and the outputs its callers read.
 | Author: suinevere
 | Dependencies: callgraph.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import glob
import os
import re
import sys
from dataclasses import dataclass, field

"""----------------------
 | FLAGS_NZ
 | Description: Bit for the N and Z flags in a liveness mask; bits 0-15 are r0-r15.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FLAGS_NZ = 1 << 16

"""----------------------
 | FLAGS_C
 | Description: Bit for the C flag in a liveness mask.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FLAGS_C = 1 << 17

"""----------------------
 | FLAGS_V
 | Description: Bit for the V flag in a liveness mask.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FLAGS_V = 1 << 18

"""----------------------
 | FLAGS
 | Description: All flag bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FLAGS = FLAGS_NZ | FLAGS_C | FLAGS_V

"""----------------------
 | SP
 | Description: The stack pointer bit, always live.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
SP = 1 << 13

"""----------------------
 | ALL
 | Description: Every tracked register (r0-r13) and the flags; lr and pc are not tracked.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
ALL = 0x3FFF | FLAGS

"""----------------------
 | COND_READS
 | Description: Flags each ARM condition reads.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
COND_READS = {"EQ": FLAGS_NZ, "NE": FLAGS_NZ, "MI": FLAGS_NZ, "PL": FLAGS_NZ, "HS": FLAGS_C, "CS": FLAGS_C,
              "LO": FLAGS_C, "CC": FLAGS_C, "VS": FLAGS_V, "VC": FLAGS_V, "HI": FLAGS_NZ | FLAGS_C,
              "LS": FLAGS_NZ | FLAGS_C, "GE": FLAGS_NZ | FLAGS_V, "LT": FLAGS_NZ | FLAGS_V, "GT": FLAGS_NZ | FLAGS_V,
              "LE": FLAGS_NZ | FLAGS_V}

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
 | REG_RE
 | Description: A register reference.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
REG_RE = re.compile(r"c->r\[(\d+)\]")

"""----------------------
 | ROOTS
 | Description: Routines the kernel enters (callbacks and the app entry); nothing reads their registers afterwards.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
ROOTS = {"rrl_entry", "rrl_irq_callback", "rrl_fiq_callback", "rrl_timer_callback", "rrl_sound_fiq"}


"""----------------------
 | Node
 | Description: One statement: what it reads, kills and may write, where control goes next, and the routines it calls or continues into (None when unknown).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class Node:
    reads: int = 0
    kills: int = 0
    writes: int = 0
    succ: list = field(default_factory=list)
    calls: object = ()
    exit: bool = False
    dest: object = ()


"""----------------------
 | Result
 | Description: Live inputs and outputs by routine: live_in/live_out with passthrough, inputs read before written, writes, and outputs (live_out written).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class Result:
    live_in: dict
    live_out: dict
    inputs: dict
    writes: dict
    outputs: dict


"""----------------------
 | mask_of
 | Description: Mask of the tracked registers referenced in a piece of text.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: text -- C text
 | Returns: bitmask
 ----------------------"""
def mask_of(text: str) -> int:
    m = 0
    for r in REG_RE.findall(text):
        if int(r) < 14:
            m |= 1 << int(r)
    return m


"""----------------------
 | names
 | Description: Register names in a mask, registers ascending then flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: mask -- bitmask
 | Returns: list of names
 ----------------------"""
def names(mask: int) -> list:
    out = ["r%d" % i for i in range(16) if mask >> i & 1]
    return out + (["flags"] if mask & FLAGS else [])


"""----------------------
 | resolve
 | Description: Where a branch through register k goes, from the last assignment to k: a routine, a dispatch table, or a popped return address.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: k -- register; last -- last assignment text by register; hooks -- name by address; tables -- base to names
 | Returns: list of names, "return", or None when unknown
 ----------------------"""
def resolve(k: int, last: dict, hooks: dict, tables):
    rhs = last.get(k, "")
    m = re.fullmatch(r"0x([0-9A-F]+)u", rhs)
    if m and int(m.group(1), 16) & ~1 in hooks:
        return [hooks[int(m.group(1), 16) & ~1]]
    if re.search(r"psk_r32\(a\b", rhs):
        return "return"
    t = re.match(r"psk_ldr\(c->r\[(\d+)\]", rhs)
    if t:
        base = re.fullmatch(r"0x([0-9A-F]+)u", last.get(int(t.group(1)), ""))
        if base and int(base.group(1), 16) & ~1 not in hooks:
            found = tables(int(base.group(1), 16))
            return found or None
    return None


"""----------------------
 | statement
 | Description: Reads, kills and may-writes of one plain statement, and its call targets.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: line -- statement text; hooks -- name by address; last -- last assignments; tables -- base to names
 | Returns: Node
 ----------------------"""
def statement(line: str, hooks: dict, last: dict, tables) -> Node:
    n = Node()
    m = re.match(r"^c->r\[(\d+)\] = (.*);$", line)
    rhs = m.group(2) if m else line
    n.reads = mask_of(rhs)
    if m and int(m.group(1)) < 14:
        n.kills = 1 << int(m.group(1))
    for cond in re.findall(r"psk_cond\(c, PSK_(\w+)\)", line):
        n.reads |= COND_READS.get(cond, FLAGS)
    if re.search(r"psk_carry\(|psk_(adc|sbc)_flags\(", line):
        n.reads |= FLAGS_C
    if re.search(r"psk_(sub|add|adc|sbc)_flags\(", line):
        n.kills |= FLAGS
    if re.search(r"psk_nz\(", line):
        n.kills |= FLAGS_NZ
    if re.search(r"psk_set_carry\(", line) or re.search(
            r"psk_(lsl|lsr|asr|ror)_s\(c, [^;]*?, (0x0*[1-9A-F][0-9A-F]*|[1-9][0-9]*)u\)", line):
        n.kills |= FLAGS_C
    n.writes = n.kills
    if re.search(r"psk_(lsl|lsr|asr|ror)_s\(", line):
        n.writes |= FLAGS_C
    call = re.search(r"psk_call\(c, (?:0x([0-9A-F]+)u|psk_bx\(c->r\[(\d+)\]\))", line)
    if call and call.group(1):
        t = hooks.get(int(call.group(1), 16) & ~1)
        n.calls = [t] if t else None
    elif call:
        r = resolve(int(call.group(2)), last, hooks, tables)
        n.calls = None if r in (None, "return") else r
    return n


"""----------------------
 | saved_regs
 | Description: Registers a routine pushes and pops on the stack; they leave with the values they came in with.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: body -- function body text
 | Returns: bitmask
 ----------------------"""
def saved_regs(body: str) -> int:
    blocks = "\n".join(re.findall(r"uint32_t a = c->r\[13\];(.*?)\n\s*\}", body, re.S))
    pushed = {int(r) for r in re.findall(r"psk_w32\(a [^,]*, c->r\[(\d+)\]\)", blocks)}
    popped = {int(r) for r in re.findall(r"c->r\[(\d+)\] = psk_r32\(a\b", blocks)}
    m = 0
    for r in pushed & popped:
        if r < 13:
            m |= 1 << r
    return m


"""----------------------
 | build
 | Description: Turns a routine body into a statement graph.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: body -- function body text; hooks -- name by address; tables -- base to names
 | Returns: list of Node
 ----------------------"""
def build(body: str, hooks: dict, tables) -> list:
    nodes, labels, gotos, ifs, stack, last = [], {}, [], [], [], {}
    transfer = None
    sp_block = False
    for raw in body.split("\n"):
        line = raw.strip()
        if not line:
            continue
        if line == "uint32_t a = c->r[13];":
            sp_block = True
        elif line == "}" or line.startswith("uint32_t a = "):
            sp_block = False
        pop = re.match(r"^c->r\[(\d+)\] = (psk_r32\(a\b[^;]*\));$", line)
        if sp_block and (re.match(r"^psk_w32\(a\b", line) or pop):
            n = Node(reads=SP)
            if pop:
                last[int(pop.group(1))] = pop.group(2)
                if int(pop.group(1)) < 13:
                    n.kills = 1 << int(pop.group(1))
            nodes.append(n)
            continue
        if re.match(r"^a_[0-9A-F]+:$", line):
            labels[line[:-1]] = len(nodes)
        elif line.startswith("if (") and line.endswith("{"):
            stack.append(("if", len(nodes)))
            nodes.append(statement(line, hooks, last, tables))
        elif line == "{":
            stack.append(("block", None))
        elif line == "}":
            kind, at = stack.pop()
            if kind == "if":
                ifs.append((at, len(nodes)))
        elif line.startswith("goto "):
            gotos.append((len(nodes), line[5:-1]))
            nodes.append(Node())
        elif line == "return;":
            n = Node(exit=True)
            j = re.fullmatch(r"psk_bx\(c->r\[(\d+)\]\)", transfer or "")
            k = re.fullmatch(r"0x([0-9A-F]+)u", transfer or "")
            if k:
                t = hooks.get(int(k.group(1), 16) & ~1)
                n.dest = [t] if t else None
            elif j and j.group(1) != "14":
                r = resolve(int(j.group(1)), last, hooks, tables)
                n.dest = () if r == "return" else r
            nodes.append(n)
        else:
            if line.startswith("c->r[15] = "):
                transfer = line[len("c->r[15] = "):-1]
            a = re.match(r"^c->r\[(\d+)\] = (.*);$", line)
            if a:
                last[int(a.group(1))] = a.group(2)
            nodes.append(statement(line, hooks, last, tables))
    goto_at = {g[0] for g in gotos}
    for i, n in enumerate(nodes):
        if not n.exit and i not in goto_at:
            n.succ.append(i + 1)
    for i, label in gotos:
        nodes[i].succ.append(labels[label])
    for at, end in ifs:
        nodes[at].succ.append(end)
    return nodes


"""----------------------
 | flow
 | Description: Backward dataflow over one routine; out_at_return gives what is live at a return, into(t) what a call or continuation target needs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: nodes -- statement graph; out_at_return -- mask; into -- name to mask
 | Returns: list of per-node live-before masks, plus the live-after mask of every node
 ----------------------"""
def flow(nodes, out_at_return, into):
    live = [0] * (len(nodes) + 1)
    after_of = [0] * len(nodes)
    again = True
    while again:
        again = False
        for i in range(len(nodes) - 1, -1, -1):
            n = nodes[i]
            if n.exit:
                if n.dest is None:
                    after = ALL
                elif n.dest:
                    after = out_at_return
                    for t in n.dest:
                        after |= into(t)
                else:
                    after = out_at_return
            else:
                after = 0
                for s in n.succ:
                    after |= live[s] if s < len(nodes) else out_at_return
            if n.calls is None:
                before = ALL
            else:
                before = after
                for t in n.calls:
                    before |= into(t)
            v = (before & ~n.kills) | n.reads
            after_of[i] = after
            if v != live[i]:
                live[i] = v
                again = True
    return live, after_of


"""----------------------
 | analyse
 | Description: Runs the liveness fixpoints over every routine in src.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: src -- register-level C; hooks -- name by address (bit 0 ignored); tables -- base address to listed names; roots -- routines the kernel enters
 | Returns: Result
 ----------------------"""
def analyse(src: str, hooks: dict, tables, roots: set) -> Result:
    hooks = {a & ~1: n for a, n in hooks.items()}
    graphs, saved = {}, {}
    for m in FUNC_RE.finditer("\n" + src):
        graphs[m.group(1)] = build(m.group(2), hooks, tables)
        saved[m.group(1)] = saved_regs(m.group(2))
        for n in graphs[m.group(1)]:
            n.writes &= ~saved[m.group(1)]
    reached = set()
    for nodes in graphs.values():
        for n in nodes:
            reached.update(n.calls or ())
            reached.update(n.dest or ())
    inputs = {n: 0 for n in graphs}
    changed = True
    while changed:
        changed = False
        for name, nodes in graphs.items():
            live, _ = flow(nodes, SP, lambda t: inputs.get(t, ALL))
            if live[0] != inputs[name]:
                inputs[name] = live[0]
                changed = True
    live_in = {n: 0 for n in graphs}
    live_out = {n: (0 if n in roots or n in reached else ALL) for n in graphs}
    changed = True
    while changed:
        changed = False
        for name, nodes in graphs.items():
            live, after_of = flow(nodes, (live_out[name] & ~saved[name]) | SP, lambda t: inputs.get(t, ALL))
            for i, n in enumerate(nodes):
                for t in (n.calls or ()):
                    if t in live_out and live_out[t] | after_of[i] != live_out[t]:
                        live_out[t] |= after_of[i]
                        changed = True
                if n.exit:
                    for t in (n.dest or ()):
                        if t in live_out and live_out[t] | live_out[name] != live_out[t]:
                            live_out[t] |= live_out[name]
                            changed = True
            entry = live[0] | (live_out[name] & saved[name])
            if entry != live_in[name]:
                live_in[name] = entry
                changed = True
    writes = {}
    for name, nodes in graphs.items():
        writes[name] = 0
        for n in nodes:
            writes[name] |= n.writes
            if n.calls is None or (n.exit and n.dest is None):
                writes[name] |= ALL
    changed = True
    while changed:
        changed = False
        for name, nodes in graphs.items():
            for n in nodes:
                for t in list(n.calls or ()) + list(n.dest or ()):
                    v = writes[name] | (writes.get(t, ALL) & ~saved[name])
                    if v != writes[name]:
                        writes[name] = v
                        changed = True
    live_out = {n: v | SP for n, v in live_out.items()}
    outputs = {n: live_out[n] & writes[n] & ~SP for n in graphs}
    return Result(live_in, live_out, inputs, {n: v & ~SP for n, v in writes.items()}, outputs)


"""----------------------
 | repository
 | Description: Runs the analysis over R1's pages with the card's dispatch tables.
 | Author: suinevere
 | Dependencies: callgraph.py
 | Globals: N/A
 | Params: N/A
 | Returns: Result
 ----------------------"""
def repository() -> Result:
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import callgraph
    hooks = {}
    with open(os.path.join(callgraph.RRL, "hooks.txt")) as f:
        for line in f:
            p = line.split("#", 1)[0].split()
            if len(p) >= 3:
                hooks[int(p[0], 16)] = p[1]
    by_addr = {a & ~1: n for a, n in hooks.items()}
    with open(callgraph.CARD, "rb") as f:
        card = f.read()
    src = ""
    for path in sorted(glob.glob(os.path.join(callgraph.RRL, "rrl_page*.c"))):
        with open(path) as f:
            src += f.read()
    return analyse(src, hooks, lambda base: callgraph.table(card, base, by_addr), ROOTS)


"""----------------------
 | main
 | Description: CLI: live_regs.py NAME... prints each routine's inputs and its hooks.txt live-output field.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    r = repository()
    for name in argv:
        full = name if name in r.outputs else "rrl_func_" + name
        print("%s in=%s regs:%s" % (full, ",".join(names(r.inputs[full])), ",".join(names(r.outputs[full]))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
