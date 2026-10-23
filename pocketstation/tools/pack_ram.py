"""----------------------
 | pack_ram.py
 | Description: Assigns the VMU offsets in Ring Ring Land's RAM map: each run of a field and the '+' fields glued after it is placed whole; runs of scalars come first, hottest first by accesses per byte, since only they gain from bank 0 direct addresses, then runs holding arrays, then the stack reserve.
 | Author: suinevere
 | Dependencies: gen_ram.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_ram  # noqa: E402

"""----------------------
 | HEAD_RE
 | Description: The address part of a VMU column that pack_ram may rewrite, with the type and conversion after it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
HEAD_RE = re.compile(r"^0x[0-9A-Fa-f]+((?::\w+)*)$")


"""----------------------
 | groups
 | Description: Splits the placed fields into runs: a field with an offset and the '+' fields after it, in file order.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: fields -- parsed map; tokens -- each line's VMU column, by field index
 | Returns: list of lists of field indexes
 ----------------------"""
def groups(fields: list, tokens: list) -> list:
    out = []
    for i, f in enumerate(fields):
        if f.vmu is None:
            continue
        if tokens[i].startswith("+"):
            out[-1].append(i)
        else:
            out.append([i])
    return out


"""----------------------
 | heat
 | Description: Accesses per byte of a run, from per-offset counts taken under the map's current layout.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: fields -- parsed map; run -- field indexes; counts -- accesses per vmu_ram offset, or None
 | Returns: the run's heat (0 without counts)
 ----------------------"""
def heat(fields: list, run: list, counts) -> float:
    if counts is None:
        return 0.0
    size = sum(fields[i].vmu_size for i in run)
    return sum(sum(counts[fields[i].vmu:fields[i].vmu + fields[i].vmu_size]) for i in run) / size


"""----------------------
 | pack
 | Description: Rewrites the map text with new offsets for every run's head: scalar runs, then runs holding arrays, each by falling heat and then file order, with stack last.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: text -- map contents; counts -- accesses per offset, or None
 | Returns: new map contents
 ----------------------"""
def pack(text: str, counts) -> str:
    fields = gen_ram.parse(text, check_vmu=False)
    lines = text.splitlines()
    where, tokens = [], []
    for n, raw in enumerate(lines):
        parts = raw.split("#", 1)[0].split(None, 4)
        if parts:
            where.append(n)
            tokens.append(parts[3])
    runs = groups(fields, tokens)
    order = sorted(range(len(runs)), key=lambda k: (fields[runs[k][0]].name == "stack", any(fields[i].count > 1 for i in runs[k]),
                                                    -heat(fields, runs[k], counts), k))
    at = 0
    for k in order:
        head = runs[k][0]
        m = HEAD_RE.match(tokens[head])
        if not m:
            raise ValueError("%s: a run head needs an offset, not %s" % (fields[head].name, tokens[head]))
        parts = lines[where[head]].split(None, 4)
        parts[3] = "0x%03X%s" % (at, m.group(1))
        lines[where[head]] = " ".join(parts)
        at += sum(fields[i].vmu_size for i in runs[k])
    out = "\n".join(lines) + "\n"
    gen_ram.parse(out)
    return out


"""----------------------
 | main
 | Description: CLI: pack_ram.py MAP [COUNTS], COUNTS being 0x400 little-endian 32-bit access counts from vmu_runner --counts.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) not in (1, 2):
        print("usage: pack_ram.py MAP [COUNTS]", file=sys.stderr)
        return 2
    counts = None
    if len(argv) == 2:
        with open(argv[1], "rb") as f:
            counts = list(struct.unpack("<%dI" % gen_ram.VMU_RAM, f.read()))
    with open(argv[0]) as f:
        text = f.read()
    text = pack(text, counts)
    with open(argv[0], "w", newline="\n") as f:
        f.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
