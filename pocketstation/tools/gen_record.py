"""----------------------
 | gen_record.py
 | Description: Generates the recording copy of the VMU-shaped C: each function renamed NAME__impl with branch probes (conditions, && and || operands, case labels, probed copies of game.h's branching accessors) inserted on the same lines, a wrapper NAME that records the call around it, and vmu_* calls routed through the logging layer.
 | Author: suinevere
 | Dependencies: vmu_callgraph.py, pocketstation/src/vmu/*.c, pocketstation/runner/vmu_record.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vmu_callgraph as vc  # noqa: E402

"""----------------------
 | HW_CALLS
 | Description: The vmu_hw.h functions the game files call that the copy routes through rec_vmu_* to be logged; vmu_r8, vmu_w8, vmu_fb and vmu_rom8 are memory reads and writes the record's RAM and frame buffer already cover.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
HW_CALLS = {"vmu_buttons", "vmu_present", "vmu_idle", "vmu_irq_enable", "vmu_irq_disable", "vmu_irq_ack",
            "vmu_irq_pending", "vmu_irq_levels", "vmu_irq_mask", "vmu_set_handler", "vmu_tick_mode",
            "vmu_sound_play", "vmu_sound_stop", "vmu_sys_flags", "vmu_power", "vmu_save_read", "vmu_save_write",
            "vmu_buzz", "vmu_lcd"}

"""----------------------
 | MAX_BITS
 | Description: Path-key bits a function may use, as vmu_record.h's REC_MAX_BITS.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
MAX_BITS = 512

"""----------------------
 | LAST
 | Description: The span that puts an insert outermost at its offset: the prototypes before a definition, and a probe block number right after an accessor call's '(' before any probe of its first argument.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
LAST = 1 << 30

"""----------------------
 | TOKEN_RE
 | Description: The C tokens these files use: comments, preprocessor lines, strings, characters, numbers, identifiers, punctuators (longest first) and whitespace.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
TOKEN_RE = re.compile(r"""
    (?P<comment>/\*.*?\*/|//[^\n]*)
  | (?P<pp>^[ \t]*\#(?:\\\n|[^\n])*)
  | (?P<str>"(?:\\.|[^"\\\n])*")
  | (?P<chr>'(?:\\.|[^'\\\n])*')
  | (?P<num>\.?\d(?:[\w.]|[eEpP][+-])*)
  | (?P<id>[A-Za-z_]\w*)
  | (?P<punct><<=|>>=|\.\.\.|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|[{}()\[\];,:?~!<>=+\-*/%&|^.])
  | (?P<ws>\s+)
""", re.S | re.M | re.X)


"""----------------------
 | Tok
 | Description: One token: kind, text, start offset in the file and line number.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Tok:
    def __init__(self, kind: str, text: str, start: int, line: int):
        self.kind, self.text, self.start, self.line = kind, text, start, line

    @property
    def end(self) -> int:
        return self.start + len(self.text)


"""----------------------
 | tokenize
 | Description: Splits C source into tokens, failing on anything the pattern does not cover.
 | Author: suinevere
 | Dependencies: TOKEN_RE, Tok
 | Globals: N/A
 | Params: text -- C source
 | Returns: list of Tok, whitespace included
 ----------------------"""
def tokenize(text: str) -> list:
    out, at, line = [], 0, 1
    while at < len(text):
        m = TOKEN_RE.match(text, at)
        if not m or m.end() == at:
            raise ValueError("cannot tokenize at line %d: %r" % (line, text[at:at + 20]))
        out.append(Tok(m.lastgroup, m.group(), at, line))
        line += m.group().count("\n")
        at = m.end()
    return out


"""----------------------
 | significant
 | Description: The tokens that are code: not whitespace, comments or preprocessor lines.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: toks -- all tokens
 | Returns: list of Tok
 ----------------------"""
def significant(toks: list) -> list:
    return [t for t in toks if t.kind not in ("ws", "comment", "pp")]


"""----------------------
 | match
 | Description: The index of the bracket closing the one at i.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: sig -- significant tokens; i -- index of an opening bracket
 | Returns: the closing bracket's index
 ----------------------"""
def match(sig: list, i: int) -> int:
    pairs = {"(": ")", "[": "]", "{": "}"}
    open_, close, depth = sig[i].text, pairs[sig[i].text], 0
    for j in range(i, len(sig)):
        if sig[j].text == open_:
            depth += 1
        elif sig[j].text == close:
            depth -= 1
            if depth == 0:
                return j
    raise ValueError("unbalanced %s at line %d" % (open_, sig[i].line))


"""----------------------
 | TERNARY_STOP
 | Description: Tokens that end a backward scan for a ?: condition: lower-precedence operators, statement and argument boundaries, and keywords that start the expression.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
TERNARY_STOP = {"(", "[", "{", "}", ";", ",", "?", ":", "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
                "<<=", ">>=", "return", "case"}


"""----------------------
 | ternary_start
 | Description: Index of the first token of the condition before a '?', skipping over balanced brackets.
 | Author: suinevere
 | Dependencies: TERNARY_STOP
 | Globals: N/A
 | Params: sig -- significant tokens; q -- index of the '?'
 | Returns: the condition's first index
 ----------------------"""
def ternary_start(sig: list, q: int, stop: set = None) -> int:
    stop = TERNARY_STOP if stop is None else stop
    j = q - 1
    while True:
        t = sig[j].text
        if t in (")", "]"):
            depth, opener = 0, "(" if t == ")" else "["
            while True:
                if sig[j].text == t:
                    depth += 1
                elif sig[j].text == opener:
                    depth -= 1
                    if depth == 0:
                        break
                j -= 1
            j -= 1
            continue
        if t in stop:
            return j + 1
        j -= 1


"""----------------------
 | OPERAND_STOP / OPERAND_END
 | Description: Tokens that bound an operand of && or ||: going back, lower-precedence operators and expression starts; going forward, the same and closing brackets.
 | Author: suinevere
 | Dependencies: TERNARY_STOP
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
OPERAND_STOP = TERNARY_STOP | {"&&", "||"}
OPERAND_END = (TERNARY_STOP - {"(", "[", "{", "return", "case"}) | {"&&", "||", ")", "]"}


"""----------------------
 | operand_end
 | Description: Index of the last token of the operand after a && or ||, skipping over balanced brackets.
 | Author: suinevere
 | Dependencies: match, OPERAND_END
 | Globals: N/A
 | Params: sig -- significant tokens; op -- index of the operator
 | Returns: the operand's last index
 ----------------------"""
def operand_end(sig: list, op: int) -> int:
    j = op + 1
    while True:
        t = sig[j].text
        if t in ("(", "["):
            j = match(sig, j) + 1
            continue
        if t in OPERAND_END:
            return j - 1
        j += 1


"""----------------------
 | Probe
 | Description: One branch probe: its index in the function, source line and kind (if, while, for, ternary, &&, ||, case, default, or ACCESSOR:kind for a probe inside a game.h accessor's copy).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Probe:
    def __init__(self, index: int, line: int, kind: str):
        self.index, self.line, self.kind = index, line, kind


"""----------------------
 | Insert
 | Description: Text to insert at an offset, with the span of source it wraps; at one offset closers go before openers, wider openers first and narrower closers first, so nested probes stay balanced.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Insert:
    def __init__(self, at: int, text: str, opener: bool, span: int):
        self.at, self.text, self.opener, self.span = at, text, opener, span

    def key(self):
        return (self.at, 1 if self.opener else 0, -self.span if self.opener else self.span)


"""----------------------
 | probe_body
 | Description: Adds the probes of one function body: each if, while and for condition, each ?: condition and each operand of && and || wrapped in rec_cond, a rec_bit after each case and default label, and a block of probes passed to each call of a branching game.h accessor's probed copy.
 | Author: suinevere
 | Dependencies: match, ternary_start, operand_end, OPERAND_STOP, Probe, Insert
 | Globals: N/A
 | Params: sig -- significant tokens; lo, hi -- the body's braces; inserts -- list to add to; cond -- rec_cond's opening text with %d for the probe; branchy -- accessor name -> its probes
 | Returns: list of Probe
 ----------------------"""
def probe_body(sig: list, lo: int, hi: int, inserts: list, cond: str = "rec_cond(%du, (", branchy: dict = None) -> list:
    probes, wrapped, branchy = [], set(), branchy or {}

    def wrap(first: int, last: int, kind: str):
        if (first, last) in wrapped:
            return
        wrapped.add((first, last))
        k = len(probes)
        probes.append(Probe(k, sig[first].line, kind))
        span = sig[last].end - sig[first].start
        inserts.append(Insert(sig[first].start, cond % k, True, span))
        inserts.append(Insert(sig[last].end, ") != 0)", False, span))

    for i in range(lo + 1, hi):
        t = sig[i]
        if t.text in ("&&", "||"):
            wrap(ternary_start(sig, i, OPERAND_STOP), i - 1, t.text)
            wrap(i + 1, operand_end(sig, i), t.text)
        elif t.kind == "id" and t.text in branchy and sig[i + 1].text == "(":
            k = len(probes)
            for p in branchy[t.text]:
                probes.append(Probe(len(probes), t.line, "%s:%s" % (t.text, p.kind)))
            inserts.append(Insert(t.start, "rec_", True, 0))
            inserts.append(Insert(sig[i + 1].end, "%du, " % k, True, LAST))
        elif t.kind == "id" and t.text in ("if", "while") and sig[i + 1].text == "(":
            close = match(sig, i + 1)
            if close > i + 2:
                wrap(i + 2, close - 1, t.text)
        elif t.kind == "id" and t.text == "for" and sig[i + 1].text == "(":
            close = match(sig, i + 1)
            semis, depth = [], 0
            for j in range(i + 2, close):
                if sig[j].text in ("(", "[", "{"):
                    depth += 1
                elif sig[j].text in (")", "]", "}"):
                    depth -= 1
                elif sig[j].text == ";" and depth == 0:
                    semis.append(j)
            if len(semis) != 2:
                raise ValueError("odd for at line %d" % t.line)
            if semis[1] > semis[0] + 1:
                wrap(semis[0] + 1, semis[1] - 1, "for")
        elif t.text == "?":
            wrap(ternary_start(sig, i), i - 1, "ternary")
        elif t.kind == "id" and t.text in ("case", "default"):
            j = i + 1
            while sig[j].text != ":":
                j += 1
            k = len(probes)
            probes.append(Probe(k, t.line, t.text))
            through = " __attribute__((fallthrough));" if sig[j + 1].text in ("case", "default") else ""
            inserts.append(Insert(sig[j].end, " rec_bit(%du);%s" % (2 * k, through), True, 0))
    return probes


"""----------------------
 | Param
 | Description: One parameter: its declaration text, name, whether it is a pointer and, for an array parameter, its length.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Param:
    def __init__(self, text: str):
        self.text = " ".join(text.split())
        self.name = re.findall(r"[A-Za-z_]\w*", self.text)[-1]
        m = re.search(r"\[\s*(\w+)\s*\]", self.text)
        self.length = m.group(1) if m else None
        self.pointer = "*" in self.text or self.length is not None

    @property
    def pointee_size(self) -> str:
        if self.length:
            return "(uint8_t)(sizeof %s[0] * %s)" % (self.name, self.length)
        return "(uint8_t)sizeof *%s" % self.name


"""----------------------
 | Def
 | Description: One function definition found in a file: name, id, specifiers and return type as written, parameters, probes and the offset its specifiers start at.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Def:
    def __init__(self, name, fid, spec, params, probes, start):
        self.name, self.fid, self.spec, self.params, self.probes, self.start = name, fid, spec, params, probes, start

    @property
    def nbits(self) -> int:
        return 2 * len(self.probes)

    @property
    def ret(self) -> str:
        return " ".join(w for w in self.spec.split() if w not in ("static", "inline"))


"""----------------------
 | wrapper
 | Description: The C text of a function's wrapper: opens a frame, records each argument (a pointer by its pointee), calls NAME__impl and closes the frame with the result.
 | Author: suinevere
 | Dependencies: Def
 | Globals: N/A
 | Params: d -- the definition
 | Returns: C text
 ----------------------"""
def wrapper(d: Def) -> str:
    plist = ", ".join(p.text for p in d.params) or "void"
    lines = ["%s %s(%s) {" % (d.spec, d.name, plist),
             "    RecFrame rec_f;",
             "    rec_enter(&rec_f, %du, %du);" % (d.fid, d.nbits)]
    for p in d.params:
        if p.pointer:
            lines.append("    rec_arg_ptr(&rec_f, %s, %s);" % (p.name, p.pointee_size))
        else:
            lines.append("    rec_arg(&rec_f, &%s, (uint8_t)sizeof %s);" % (p.name, p.name))
    call = "%s__impl(%s)" % (d.name, ", ".join(p.name for p in d.params))
    if d.ret == "void":
        lines += ["    %s;" % call, "    rec_leave(&rec_f, 0, 0);", "}"]
    else:
        lines += ["    %s rec_r = %s;" % (d.ret, call), "    rec_leave(&rec_f, &rec_r, (uint8_t)sizeof rec_r);",
                  "    return rec_r;", "}"]
    return "\n".join(lines)


"""----------------------
 | FILE_HEADER
 | Description: The header block each generated file starts with.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FILE_HEADER = """/*----------------------
 | %s
 | Description: Generated by pocketstation/tools/gen_record.py from %s for the recording build; do not edit.
 | Author: suinevere
 | Dependencies: vmu_record.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
"""


"""----------------------
 | generate_file
 | Description: The recording copy of one game file: wrapper prototypes on the first definition's line, probes and renames applied in place, vmu_* calls routed through rec_vmu_*, and wrappers plus a registering constructor appended.
 | Author: suinevere
 | Dependencies: tokenize, significant, match, probe_body, Param, Def, wrapper, FILE_HEADER, HW_CALLS
 | Globals: N/A
 | Params: src -- the file's path; ids -- name -> function id; funcs -- discovered functions; rel -- the path #line names; branchy -- game.h accessor name -> its probes
 | Returns: (generated text, list of Def)
 ----------------------"""
def generate_file(src: str, ids: dict, funcs: dict, rel: str, branchy: dict):
    base = os.path.basename(src)
    with open(src) as f:
        text = f.read()
    toks = tokenize(text)
    sig = significant(toks)
    inserts, defs, depth, i = [], [], 0, 0
    while i < len(sig):
        t = sig[i]
        if t.text == "{":
            depth += 1
        elif t.text == "}":
            depth -= 1
        elif (depth == 0 and t.kind == "id" and t.text in ids and funcs[t.text].file == base
              and i + 1 < len(sig) and sig[i + 1].text == "("):
            close = match(sig, i + 1)
            if close + 1 < len(sig) and sig[close + 1].text == "{":
                start = i
                while start > 0 and sig[start - 1].text not in (";", "}") and sig[start - 1].start > _last_break(toks, t.start):
                    start -= 1
                spec = " ".join(s.text for s in sig[start:i])
                ptext = text[sig[i + 1].end:sig[close].start]
                params = [Param(p) for p in vc.split_params(ptext)]
                body_close = match(sig, close + 1)
                probes = probe_body(sig, close + 1, body_close, inserts, branchy=branchy)
                d = Def(t.text, ids[t.text], spec, params, probes, sig[start].start)
                if d.nbits > MAX_BITS:
                    raise ValueError("%s needs %d path bits, more than %d" % (d.name, d.nbits, MAX_BITS))
                defs.append(d)
                inserts.append(Insert(t.end, "__impl", True, 0))
                i = body_close + 1
                continue
        i += 1
    for j, t in enumerate(sig):
        if t.kind == "id" and t.text in HW_CALLS and j + 1 < len(sig) and sig[j + 1].text == "(":
            inserts.append(Insert(t.start, "rec_", True, 0))
    if defs:
        protos = " ".join("%s %s(%s);" % (d.spec, d.name, ", ".join(p.text for p in d.params) or "void") for d in defs)
        inserts.append(Insert(defs[0].start, protos + " ", True, LAST))
    missing = sorted(n for n, f in funcs.items() if f.file == base and n in ids and n not in {d.name for d in defs})
    if missing:
        raise ValueError("%s: definitions not found: %s" % (base, " ".join(missing)))
    out, at = [], 0
    for ins in sorted(inserts, key=Insert.key):
        out.append(text[at:ins.at])
        out.append(ins.text)
        at = ins.at
    out.append(text[at:])
    body = "".join(out)
    if not body.endswith("\n"):
        body += "\n"
    head = FILE_HEADER % (base, rel) + '#include "vmu_record.h"\n#include "record_inline.h"\n#line 1 "%s"\n' % rel
    tail_line = head.count("\n") + body.count("\n") + 2
    tail = ['#line %d "%s"' % (tail_line, "build/record/src/" + base)]
    for d in defs:
        tail.append(wrapper(d))
    tail.append("__attribute__((constructor)) static void rec_register_%s(void) {" % base[:-2])
    for d in defs:
        tail.append("    rec_register(%du, (RecFn)%s);" % (d.fid, d.name))
    tail.append("}")
    return head + body + "\n".join(tail) + "\n", defs


"""----------------------
 | generate_inline
 | Description: Writes record_inline.h: a probed copy rec_NAME(rec_k, ...) of each game.h inline accessor that branches, whose probes start at rec_k in the calling function's key.
 | Author: suinevere
 | Dependencies: tokenize, significant, match, probe_body, FILE_HEADER
 | Globals: N/A
 | Params: header -- game.h; funcs -- discovered functions; out -- the header to write
 | Returns: dict accessor name -> its probes
 ----------------------"""
def generate_inline(header: str, funcs: dict, out: str) -> dict:
    with open(header) as f:
        text = f.read()
    sig = significant(tokenize(text))
    inline = {n for n, f in funcs.items() if f.inline}
    branchy, copies = {}, []
    for i, t in enumerate(sig):
        if t.kind != "id" or t.text not in inline or sig[i + 1].text != "(":
            continue
        close = match(sig, i + 1)
        if sig[close + 1].text != "{":
            continue
        start = i
        while sig[start - 1].text in ("static", "inline") or sig[start - 1].kind == "id" or sig[start - 1].text == "*":
            start -= 1
            if sig[start].text == "static":
                break
        body_close = match(sig, close + 1)
        inserts = []
        probes = probe_body(sig, close + 1, body_close, inserts, cond="rec_cond(rec_k + %du, (")
        if not probes:
            continue
        if any(p.kind in ("case", "default") for p in probes):
            raise ValueError("case labels in accessor %s" % t.text)
        lo, hi = sig[close + 1].start, sig[body_close].end
        body, at = [], lo
        for ins in sorted(inserts, key=Insert.key):
            body.append(text[at:ins.at])
            body.append(ins.text)
            at = ins.at
        body.append(text[at:hi])
        spec = text[sig[start].start:sig[i].start].strip()
        params = text[sig[i + 1].end:sig[close].start]
        copies.append("%s rec_%s(unsigned rec_k, %s) %s" % (spec, t.text, params, "".join(body)))
        branchy[t.text] = probes
    with open(out, "w", newline="\n") as f:
        f.write(FILE_HEADER % (os.path.basename(out), "src/vmu/game.h"))
        f.write('#ifndef RECORD_INLINE_H\n#define RECORD_INLINE_H\n#include "game.h"\n#include "vmu_record.h"\n')
        f.write("\n".join(copies) + "\n#endif\n")
    return branchy


"""----------------------
 | _last_break
 | Description: The offset just past the last comment or preprocessor line before an offset, the earliest a definition's specifiers can start.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: toks -- all tokens; at -- offset
 | Returns: the offset
 ----------------------"""
def _last_break(toks: list, at: int) -> int:
    best = -1
    for t in toks:
        if t.start >= at:
            break
        if t.kind in ("comment", "pp"):
            best = t.end - 1
    return best


"""----------------------
 | function_ids
 | Description: Ids of the functions the recording wraps: every discovered function but game.h's inline accessors, numbered in name order.
 | Author: suinevere
 | Dependencies: vmu_callgraph.discover
 | Globals: N/A
 | Params: funcs -- discovered functions
 | Returns: dict name -> id
 ----------------------"""
def function_ids(funcs: dict) -> dict:
    return {n: i for i, n in enumerate(sorted(n for n, f in funcs.items() if not f.inline))}


"""----------------------
 | generate
 | Description: Writes the recording copy of every game file, record_names.c (names and path bits by id) and probes.txt (NAME PROBE LINE KIND per probe) into out_dir.
 | Author: suinevere
 | Dependencies: generate_file, function_ids, vmu_callgraph.discover, FILE_HEADER
 | Globals: N/A
 | Params: src_dir -- src/vmu; out_dir -- build/record/src
 | Returns: dict name -> Def
 ----------------------"""
def generate(src_dir: str, out_dir: str) -> dict:
    funcs = vc.discover()
    ids = function_ids(funcs)
    os.makedirs(out_dir, exist_ok=True)
    branchy = generate_inline(os.path.join(src_dir, "game.h"), funcs, os.path.join(out_dir, "record_inline.h"))
    defs = {}
    for base in sorted({f.file for n, f in funcs.items() if n in ids}):
        text, found = generate_file(os.path.join(src_dir, base), ids, funcs, "src/vmu/" + base, branchy)
        with open(os.path.join(out_dir, base), "w", newline="\n") as f:
            f.write(text)
        defs.update({d.name: d for d in found})
    names = sorted(ids, key=ids.get)
    with open(os.path.join(out_dir, "record_names.c"), "w", newline="\n") as f:
        f.write(FILE_HEADER % ("record_names.c", "the game files in src/vmu"))
        f.write('#include "vmu_record.h"\n\n')
        f.write("const uint16_t rec_fn_count = %du;\n" % len(names))
        f.write("const char *const rec_fn_names[] = {\n%s\n};\n" % "\n".join('    "%s",' % n for n in names))
        f.write("const uint16_t rec_fn_bits[] = {\n%s\n};\n" % "\n".join("    %du," % defs[n].nbits for n in names))
    with open(os.path.join(os.path.dirname(os.path.normpath(out_dir)), "probes.txt"), "w", newline="\n") as f:
        for n in names:
            for p in defs[n].probes:
                f.write("%s %d %d %s\n" % (n, p.index, p.line, p.kind))
    return defs


"""----------------------
 | main
 | Description: CLI: gen_record.py SRC_DIR OUT_DIR.
 | Author: suinevere
 | Dependencies: generate
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 2:
        print("usage: gen_record.py SRC_DIR OUT_DIR", file=sys.stderr)
        return 2
    defs = generate(argv[0], argv[1])
    print("gen_record: %d functions, %d probes" % (len(defs), sum(len(d.probes) for d in defs.values())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
