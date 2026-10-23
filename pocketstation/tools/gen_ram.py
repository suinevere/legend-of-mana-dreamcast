"""----------------------
 | gen_ram.py
 | Description: Turns Ring Ring Land's RAM map into named C accessors at the PocketStation addresses or at the VMU offsets, or into a JSON field list.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import json
import re
import sys
from dataclasses import dataclass

"""----------------------
 | TYPES
 | Description: Map type names to their C type, size in bytes and bus width.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
TYPES = {"u8": ("uint8_t", 1, 8), "s8": ("int8_t", 1, 8), "u16": ("uint16_t", 2, 16), "s16": ("int16_t", 2, 16),
         "u32": ("uint32_t", 4, 32)}

"""----------------------
 | NO_ACCESS
 | Description: Field names that reserve bytes without accessors.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
NO_ACCESS = ("stack", "free")

"""----------------------
 | VMU_RAM
 | Description: Bytes of vmu_ram.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
VMU_RAM = 0x400

"""----------------------
 | TYPE_RE
 | Description: A field type: base type with an optional element count.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
TYPE_RE = re.compile(r"^(u8|s8|u16|s16|u32)(?:\[(\d+)\])?$")

"""----------------------
 | VMU_RE
 | Description: The VMU column: '-' (not in vmu_ram), '+' (right after the line before) or an offset, then an optional VMU type and value conversion (handler: code address to handler id; file: card address to file offset).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
VMU_RE = re.compile(r"^(?:-|(\+|0x[0-9A-Fa-f]+)(?::(u8|s8|u16|s16|u32))?(?::(handler|file))?)$")


"""----------------------
 | Field
 | Description: One RAM map line: PocketStation address (None for VMU-only fields), type, count, name, VMU offset, VMU type and value conversion.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class Field:
    ps: object
    type: str
    count: int
    name: str
    vmu: object
    text: str
    vmu_type: str = ""
    conv: object = None

    @property
    def size(self) -> int:
        return TYPES[self.type][1] * self.count

    @property
    def vmu_size(self) -> int:
        return TYPES[self.vmu_type][1] * self.count


"""----------------------
 | claim
 | Description: Marks a field's bytes in an owner map, rejecting overlaps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: owner -- address to name; start, size -- range; name -- field; n -- line number; space -- which address space
 | Returns: N/A
 ----------------------"""
def claim(owner: dict, start: int, size: int, name: str, n: int, space: str) -> None:
    for a in range(start, start + size):
        if a in owner:
            raise ValueError("line %d: %s overlaps %s at %s 0x%X" % (n, name, owner[a], space, a))
        owner[a] = name


"""----------------------
 | parse
 | Description: Parses the map, resolving '+' VMU offsets and rejecting bad lines, duplicate names, overlaps in either address space and fields past vmu_ram's end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: text -- map file contents; check_vmu -- False skips the VMU overlap and range checks, for pack_ram's unplaced input
 | Returns: list of Field in file order
 ----------------------"""
def parse(text: str, check_vmu: bool = True) -> list:
    fields, names, owner, vmu_owner = [], set(), {}, {}
    prev = None
    for n, raw in enumerate(text.splitlines(), 1):
        line = raw.split("#", 1)[0].split(None, 4)
        if not line:
            continue
        m = TYPE_RE.match(line[1]) if len(line) >= 4 else None
        v = VMU_RE.match(line[3]) if m else None
        if not v:
            raise ValueError("line %d: expected '<ps addr|-> <type> <name> <vmu addr|+|->[:type][:conv] [description]'" % n)
        name = line[2]
        if name in names and name not in NO_ACCESS:
            raise ValueError("line %d: %s defined twice" % (n, name))
        names.add(name)
        if v.group(1) == "+":
            if prev is None or prev.vmu is None:
                raise ValueError("line %d: %s follows a field that is not in vmu_ram" % (n, name))
            vmu = prev.vmu + prev.vmu_size
        else:
            vmu = None if line[3] == "-" else int(v.group(1), 16)
        f = Field(None if line[0] == "-" else int(line[0], 16), m.group(1), int(m.group(2) or 1), name, vmu,
                  line[4] if len(line) > 4 else "", v.group(2) or m.group(1), v.group(3))
        if f.ps is not None:
            claim(owner, f.ps, f.size, name, n, "ps")
        if f.vmu is not None and check_vmu:
            if f.vmu + f.vmu_size > VMU_RAM:
                raise ValueError("line %d: %s ends past vmu_ram at 0x%X" % (n, name, f.vmu + f.vmu_size))
            claim(vmu_owner, f.vmu, f.vmu_size, name, n, "vmu")
        fields.append(f)
        prev = f
    return fields


"""----------------------
 | accessors
 | Description: The ram_<name>/set_<name> pair for one field over a bus's read and write calls.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: name -- field; type -- map type; count -- elements; addr -- base address; rd, wr -- call prefixes; checked -- index through VMU_IX
 | Returns: two lines of C
 ----------------------"""
def accessors(name: str, type: str, count: int, addr: int, rd: str, wr: str, checked: bool = False) -> list:
    ctype, size, bits = TYPES[type]
    i = 'VMU_IX("%s", i, %du)' % (name, count) if checked else "i"
    at = "0x%0*Xu" % (3, addr) if count == 1 else "0x%0*Xu + %s" % (3, addr, i if size == 1 else "%s * %du" % (i, size))
    idx, idx_set = ("void", "") if count == 1 else ("uint8_t i", "uint8_t i, ")
    return ["static inline %s ram_%s(%s) { return %s%s%d(%s); }" % (
                ctype, name, idx, "" if type.startswith("u") else "(%s)" % ctype, rd, bits, at),
            "static inline void set_%s(%s%s v) { %s%d(%s, %sv); }" % (
                name, idx_set, ctype, wr, bits, at, "" if type.startswith("u") else "(uint%d_t)" % bits)]


"""----------------------
 | ps_header
 | Description: Emits ram_<name>/set_<name> accessors that go through psk_r*/psk_w* at the PocketStation address.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: fields -- parsed map
 | Returns: header text
 ----------------------"""
def ps_header(fields: list) -> str:
    out = ["/* rrl_ram.h -- generated by pocketstation/tools/gen_ram.py; do not edit. */", "#ifndef RRL_RAM_H",
           "#define RRL_RAM_H", "#include <stdint.h>", '#include "psk_hw.h"', ""]
    for f in fields:
        if f.name in NO_ACCESS or f.ps is None:
            continue
        out += accessors(f.name, f.type, f.count, f.ps, "psk_r", "psk_w")
    out.append("#endif")
    return "\n".join(out) + "\n"


"""----------------------
 | VMU_WIDE
 | Description: Little-endian halfword and word access to vmu_ram, and VMU_IX, which under VMU_CHECK_BOUNDS reports an array index past its field, emitted into the VMU header.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
VMU_WIDE = [
    "static inline uint16_t vmu_r16(uint16_t o) { return (uint16_t)(vmu_r8(o) | (vmu_r8((uint16_t)(o + 1u)) << 8)); }",
    "static inline void vmu_w16(uint16_t o, uint16_t v) { vmu_w8(o, (uint8_t)v); vmu_w8((uint16_t)(o + 1u), (uint8_t)(v >> 8)); }",
    "static inline uint32_t vmu_r32(uint16_t o) { return vmu_r16(o) | ((uint32_t)vmu_r16((uint16_t)(o + 2u)) << 16); }",
    "static inline void vmu_w32(uint16_t o, uint32_t v) { vmu_w16(o, (uint16_t)v); vmu_w16((uint16_t)(o + 2u), (uint16_t)(v >> 16)); }",
    "#ifdef VMU_CHECK_BOUNDS",
    "void vmu_index_fault(const char *field, unsigned i);",
    "#define VMU_IX(f, i, n) ((i) < (n) ? (i) : (vmu_index_fault(f, i), (i)))",
    "#else",
    "#define VMU_IX(f, i, n) (i)",
    "#endif",
]


"""----------------------
 | vmu_header
 | Description: Emits RAM_<NAME> offsets and ram_<name>/set_<name> accessors over vmu_r*/vmu_w* at the VMU offset and type, for every field in vmu_ram.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: fields -- parsed map
 | Returns: header text
 ----------------------"""
def vmu_header(fields: list) -> str:
    out = ["/* rrl_ram.h -- generated by pocketstation/tools/gen_ram.py; do not edit. */", "#ifndef RRL_RAM_H",
           "#define RRL_RAM_H", "#include <stdint.h>", '#include "vmu_hw.h"', ""] + VMU_WIDE + [""]
    for f in fields:
        if f.name in NO_ACCESS or f.vmu is None:
            continue
        out.append("#define RAM_%s 0x%03Xu" % (f.name.upper(), f.vmu))
        out += accessors(f.name, f.vmu_type, f.count, f.vmu, "vmu_r", "vmu_w", True)
    out.append("#endif")
    return "\n".join(out) + "\n"


"""----------------------
 | fields_json
 | Description: Emits the field list both runners' state dumps are compared through.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: fields -- parsed map
 | Returns: JSON text
 ----------------------"""
def fields_json(fields: list) -> str:
    return json.dumps([{"name": f.name, "type": f.type, "count": f.count, "ps": f.ps, "vmu": f.vmu,
                        "vmu_type": f.vmu_type, "conv": f.conv}
                       for f in fields if f.name not in NO_ACCESS], indent=1)


"""----------------------
 | doc_table
 | Description: Emits the RAM map as a Markdown table for the reference docs: every named field by VMU offset, then the fields the VMU drops; free bytes are left out.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: fields -- parsed map
 | Returns: Markdown text
 ----------------------"""
def doc_table(fields: list) -> str:
    rows = [f for f in fields if f.name != "free"]
    rows.sort(key=lambda f: (f.vmu is None, f.vmu if f.vmu is not None else f.ps))
    out = ["| VMU | PocketStation | Name | Type | Meaning |", "|---|---|---|---|---|"]
    for f in rows:
        count = "[%d]" % f.count if f.count > 1 else ""
        kind = f.type + count if f.vmu is None or f.vmu_type == f.type else "%s%s, VMU %s%s" % (f.type, count, f.vmu_type, count)
        out.append("| %s | %s | `%s` | %s | %s |" % ("`0x%03X`" % f.vmu if f.vmu is not None else "-",
                                                    "`0x%03X`" % f.ps if f.ps is not None else "-", f.name, kind,
                                                    f.text.replace("|", "\\|")))
    return "\n".join(out) + "\n"


"""----------------------
 | main
 | Description: CLI: gen_ram.py ps|vmu|fields|doc MAP OUT
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 3 or argv[0] not in ("ps", "vmu", "fields", "doc"):
        print("usage: gen_ram.py ps|vmu|fields|doc MAP OUT", file=sys.stderr)
        return 2
    try:
        with open(argv[1]) as f:
            fields = parse(f.read())
    except ValueError as e:
        print("gen_ram: %s: %s" % (argv[1], e), file=sys.stderr)
        return 1
    text = {"ps": ps_header, "vmu": vmu_header, "fields": fields_json, "doc": doc_table}[argv[0]](fields)
    with open(argv[2], "w", newline="\n") as f:
        f.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
