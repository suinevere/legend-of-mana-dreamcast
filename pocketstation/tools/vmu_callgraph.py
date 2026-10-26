"""----------------------
 | vmu_callgraph.py
 | Description: Call levels of the VMU-shaped C's functions (src/vmu/*.c plus game.h's static inline accessors, vmu_host.c and vmu_hw.h excluded) and a checker for the assembly porting manifest.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/*.c, pocketstation/src/vmu/game.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import glob
import os
import re
import sys

"""----------------------
 | VMU_DIR
 | Description: The VMU-shaped C's source directory.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
VMU_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "vmu")

"""----------------------
 | EXCLUDE
 | Description: C files in VMU_DIR that are not the game's own code: the host model of the VMU interface.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
EXCLUDE = {"vmu_host.c"}

"""----------------------
 | INLINE_HEADER
 | Description: The header whose static inline accessors (rom8, rom16, rom32, ref8, rev32, fb_row, set_fb_row, fb_run, set_fb_run) are part of the narrow C's call graph too; they inline at every call site rather than becoming a routine. vmu_hw.h is not scanned: it only declares the runtime's own externs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
INLINE_HEADER = "game.h"

"""----------------------
 | SIG_RE
 | Description: A top-level function definition: an optional 'static' and/or 'inline', a return type, the name, its parameter list and the opening brace of its body.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
SIG_RE = re.compile(r"^(?:static\s+)?(?:inline\s+)?([A-Za-z_]\w*\s*\**)\s+(\w+)\s*\(([^;{]*)\)\s*\{", re.M)

"""----------------------
 | IDENT_RE
 | Description: A C identifier token.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
IDENT_RE = re.compile(r"[A-Za-z_]\w*")

"""----------------------
 | strip_comments
 | Description: Blanks out /* ... */ comments, keeping line counts, so a header block's prose cannot be mistaken for code.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: text -- C source
 | Returns: text with block comments blanked
 ----------------------"""
def strip_comments(text: str) -> str:
    out, i, n = [], 0, len(text)
    while i < n:
        if text[i:i + 2] == "/*":
            end = text.find("*/", i + 2)
            end = n - 2 if end == -1 else end
            out.append("\n" * text.count("\n", i, end + 2))
            i = end + 2
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


"""----------------------
 | body_end
 | Description: Finds the index just past a function body's closing brace, given the index of its opening brace.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: text -- C source; open -- index of the body's '{'
 | Returns: index just past the matching '}'
 ----------------------"""
def body_end(text: str, open_: int) -> int:
    depth = 0
    for i in range(open_, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i + 1
    raise ValueError("unbalanced braces")


"""----------------------
 | Func
 | Description: One discovered function: its file, return type, raw parameter text, body and whether it is one of game.h's inline accessors rather than a routine the C file defines.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Func:
    def __init__(self, file: str, ret: str, params: str, body: str, inline: bool = False):
        self.file = file
        self.ret = ret.strip()
        self.params = params.strip()
        self.body = body
        self.inline = inline


"""----------------------
 | parse_file
 | Description: Parses every top-level function defined in one C or header file.
 | Author: suinevere
 | Dependencies: strip_comments, body_end, SIG_RE, Func
 | Globals: N/A
 | Params: path -- source file; inline -- whether to mark every function found as game.h's inline accessors
 | Returns: dict of name -> Func
 ----------------------"""
def parse_file(path: str, inline: bool = False) -> dict:
    funcs = {}
    name_of_file = os.path.basename(path)
    with open(path) as f:
        text = strip_comments(f.read())
    for m in SIG_RE.finditer(text):
        name = m.group(2)
        end = body_end(text, m.end() - 1)
        funcs[name] = Func(name_of_file, m.group(1), m.group(3), text[m.end():end - 1], inline)
    return funcs


"""----------------------
 | discover
 | Description: Parses every function defined in src/vmu/*.c (vmu_host.c excluded) plus game.h's static inline accessors (vmu_hw.h excluded: it only declares the runtime's externs).
 | Author: suinevere
 | Dependencies: parse_file
 | Globals: VMU_DIR, EXCLUDE, INLINE_HEADER
 | Params: N/A
 | Returns: dict of name -> Func
 ----------------------"""
def discover() -> dict:
    funcs = {}
    for path in sorted(glob.glob(os.path.join(VMU_DIR, "*.c"))):
        if os.path.basename(path) in EXCLUDE:
            continue
        funcs.update(parse_file(path))
    funcs.update(parse_file(os.path.join(VMU_DIR, INLINE_HEADER), inline=True))
    return funcs


"""----------------------
 | callees
 | Description: The functions a function's body names, direct calls and names appearing in a handler table alike, found by matching identifiers against the known function set.
 | Author: suinevere
 | Dependencies: IDENT_RE
 | Globals: N/A
 | Params: name -- the function; funcs -- dict of name -> Func
 | Returns: set of callee names
 ----------------------"""
def callees(name: str, funcs: dict) -> set:
    return {t for t in IDENT_RE.findall(funcs[name].body) if t in funcs and t != name}


"""----------------------
 | sccs
 | Description: Tarjan's algorithm over the call graph: every function's strongly connected component (a lone function with no self-loop is its own singleton component).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: funcs -- dict of name -> Func
 | Returns: dict of name -> component id (an integer, components numbered in reverse topological order: 0 is a sink)
 ----------------------"""
def sccs(funcs: dict) -> dict:
    graph = {name: callees(name, funcs) for name in funcs}
    index, low, on_stack, stack, comp, counter, next_comp = {}, {}, set(), [], {}, [0], [0]

    def strongconnect(v):
        index[v] = low[v] = counter[0]
        counter[0] += 1
        stack.append(v)
        on_stack.add(v)
        for w in graph[v]:
            if w not in index:
                strongconnect(w)
                low[v] = min(low[v], low[w])
            elif w in on_stack:
                low[v] = min(low[v], index[w])
        if low[v] == index[v]:
            cid = next_comp[0]
            next_comp[0] += 1
            while True:
                w = stack.pop()
                on_stack.discard(w)
                comp[w] = cid
                if w == v:
                    break

    sys.setrecursionlimit(max(10000, len(funcs) * 4))
    for v in funcs:
        if v not in index:
            strongconnect(v)
    return comp


"""----------------------
 | levels
 | Description: Call level of every function: leaves (and leaf recursion groups) at 0, every level one more than the highest level any callee reaches, members of a recursion group sharing their component's level.
 | Author: suinevere
 | Dependencies: discover, sccs, callees
 | Globals: N/A
 | Params: funcs -- dict of name -> Func, discovered if omitted
 | Returns: list of (level, name, file) sorted by level then name
 ----------------------"""
def levels(funcs: dict = None) -> list:
    funcs = funcs or discover()
    comp = sccs(funcs)
    members = {}
    for name, cid in comp.items():
        members.setdefault(cid, []).append(name)
    comp_level = {}

    def level_of(cid):
        if cid in comp_level:
            return comp_level[cid]
        comp_level[cid] = 0
        best = -1
        for name in members[cid]:
            for callee in callees(name, funcs):
                ccid = comp[callee]
                if ccid != cid:
                    best = max(best, level_of(ccid))
        comp_level[cid] = best + 1
        return comp_level[cid]

    for cid in members:
        level_of(cid)
    return sorted(((comp_level[comp[n]], n, funcs[n].file) for n in funcs), key=lambda t: (t[0], t[1]))


"""----------------------
 | PARAM_REG
 | Description: Scalar C base types this ABI can carry in a register, mapped to the register category ('u8' for ACC/B/C, 'u16' for TRH:TRL).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
PARAM_REG = {"uint8_t": "u8", "int8_t": "u8", "uint16_t": "u16", "int16_t": "u16"}

"""----------------------
 | classify_param
 | Description: One parameter's register category: 'u8'/'u16' for a scalar byte or halfword the ABI can carry in a register, 'wide' for everything else (u32, a pointer, an array parameter or a struct passed by value).
 | Author: suinevere
 | Dependencies: PARAM_REG
 | Globals: N/A
 | Params: ptext -- one parameter's declaration text
 | Returns: (name, category)
 ----------------------"""
def classify_param(ptext: str) -> tuple:
    ptext = ptext.strip()
    is_wide = "*" in ptext or "[" in ptext
    m = re.match(r"(?:const\s+)?(\w+)\s*\**\s*(\w+)", ptext)
    base, name = (m.group(1), m.group(2)) if m else ("", ptext.split()[-1] if ptext.split() else ptext)
    name = name.split("[", 1)[0].strip()
    return name, ("wide" if is_wide else PARAM_REG.get(base, "wide"))


"""----------------------
 | split_params
 | Description: Splits a parameter list into its parameters, treating an empty list or 'void' as none.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: params -- raw parameter list text
 | Returns: list of parameter declaration strings
 ----------------------"""
def split_params(params: str) -> list:
    params = params.strip()
    if params == "" or params == "void":
        return []
    return [p.strip() for p in params.split(",")]


"""----------------------
 | args_mapping
 | Description: A function's ARGS column: each parameter, in order, assigned the next free register of its category (ACC, B, C for a byte; the one TRH:TRL pair for a halfword), the rest falling back to a named memory slot, as a conservative default to be tightened once the routine is written.
 | Author: suinevere
 | Dependencies: classify_param, split_params
 | Globals: N/A
 | Params: params -- raw parameter list text
 | Returns: the ARGS column text ('-' for none)
 ----------------------"""
def args_mapping(params: str) -> str:
    parts = split_params(params)
    if not parts:
        return "-"
    acc_pool, tr_free, out = ["ACC", "B", "C"], True, []
    for ptext in parts:
        name, kind = classify_param(ptext)
        if kind == "u8" and acc_pool:
            reg = acc_pool.pop(0)
        elif kind == "u16" and tr_free:
            reg, tr_free = "TR", False
        else:
            reg = "mem:%s" % name.upper()
        out.append("%s=%s" % (name, reg))
    return ",".join(out)


"""----------------------
 | RESULT_REG
 | Description: Return types this ABI carries back in a register, mapped to the register ('-' for void and for anything wider than a halfword, which the routine must write to memory instead).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
RESULT_REG = {"void": "-", "uint8_t": "ACC", "int8_t": "ACC", "uint16_t": "TR", "int16_t": "TR"}

"""----------------------
 | result_mapping
 | Description: A function's RESULT column from its return type.
 | Author: suinevere
 | Dependencies: RESULT_REG
 | Globals: N/A
 | Params: ret -- the return type text
 | Returns: the RESULT column text
 ----------------------"""
def result_mapping(ret: str) -> str:
    base = ret.strip().rstrip("*").strip()
    return RESULT_REG.get(base, "-")


"""----------------------
 | routine_name
 | Description: The assembly routine name the manifest proposes for a function: 'g_' plus the C name with a leading 'game_' stripped.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: name -- the C function name
 | Returns: the proposed routine name
 ----------------------"""
def routine_name(name: str) -> str:
    return "g_" + (name[len("game_"):] if name.startswith("game_") else name)


"""----------------------
 | INLINE_ROUTINE
 | Description: The ROUTINE column for one of game.h's accessors: inlined everywhere, so the caller column is '*' rather than one name.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
INLINE_ROUTINE = "inlined:*"


"""----------------------
 | DESTROYS_DEFAULT
 | Description: The conservative default DESTROYS column for a routine not yet written: every register the calling convention and PSW cover, from the runtime ABI (docs/en/technical/reference/vmu-runtime.md).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
DESTROYS_DEFAULT = "ACC,B,C,TRH,TRL,PSW"

"""----------------------
 | MANIFEST_COLUMNS
 | Description: The manifest line's column count: NAME FILE LEVEL ROUTINE ARGS RESULT DESTROYS STATUS.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
MANIFEST_COLUMNS = 8

"""----------------------
 | manifest_lines
 | Description: Builds one manifest line per function discovered, every routine at 'todo' with its proposed name, register mapping and the conservative DESTROYS default; game.h's inline accessors get 'inlined:*' instead of a routine name and '-' for DESTROYS, since they are never CALLF'd on their own.
 | Author: suinevere
 | Dependencies: routine_name, INLINE_ROUTINE, args_mapping, result_mapping, DESTROYS_DEFAULT, levels
 | Globals: N/A
 | Params: funcs -- dict of name -> Func
 | Returns: list of manifest line strings, ordered as levels() orders them
 ----------------------"""
def manifest_lines(funcs: dict) -> list:
    out = []
    for level, name, file in levels(funcs):
        f = funcs[name]
        routine = INLINE_ROUTINE if f.inline else routine_name(name)
        destroys = "-" if f.inline else DESTROYS_DEFAULT
        out.append(" ".join((name, file, str(level), routine, args_mapping(f.params),
                             result_mapping(f.ret), destroys, "todo")))
    return out


"""----------------------
 | parse_manifest
 | Description: Reads a manifest, rejecting any line whose column count is wrong.
 | Author: suinevere
 | Dependencies: MANIFEST_COLUMNS
 | Globals: N/A
 | Params: path -- manifest file path
 | Returns: dict of name -> column list
 ----------------------"""
def parse_manifest(path: str) -> dict:
    rows = {}
    with open(path) as f:
        for n, raw in enumerate(f, 1):
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            cols = line.split()
            if len(cols) != MANIFEST_COLUMNS:
                raise ValueError("%s:%d: expected %d columns, got %d" % (path, n, MANIFEST_COLUMNS, len(cols)))
            rows[cols[0]] = cols
    return rows


"""----------------------
 | check
 | Description: Reports a manifest that misses a function the call graph finds or invents one it does not, or whose column count is wrong.
 | Author: suinevere
 | Dependencies: discover, parse_manifest
 | Globals: N/A
 | Params: path -- manifest file path
 | Returns: 0 if the manifest matches the call graph, 1 otherwise
 ----------------------"""
def check(path: str) -> int:
    funcs = discover()
    try:
        rows = parse_manifest(path)
    except ValueError as e:
        print("vmu_callgraph: %s" % e, file=sys.stderr)
        return 1
    missing = sorted(set(funcs) - set(rows))
    invented = sorted(set(rows) - set(funcs))
    if missing:
        print("vmu_callgraph: %s: missing %s" % (path, " ".join(missing)), file=sys.stderr)
    if invented:
        print("vmu_callgraph: %s: invented %s" % (path, " ".join(invented)), file=sys.stderr)
    return 1 if missing or invented else 0


"""----------------------
 | main
 | Description: CLI: vmu_callgraph.py levels | manifest | check MANIFEST.
 | Author: suinevere
 | Dependencies: discover, levels, manifest_lines, check
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if argv[:1] == ["levels"]:
        for level, name, file in levels():
            print("%d %s %s" % (level, name, file))
        return 0
    if argv[:1] == ["manifest"]:
        for line in manifest_lines(discover()):
            print(line)
        return 0
    if len(argv) == 2 and argv[0] == "check":
        return check(argv[1])
    print("usage: vmu_callgraph.py levels | manifest | check MANIFEST", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
