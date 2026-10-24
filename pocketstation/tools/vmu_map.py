#!/usr/bin/env python3
#----------------------
# | vmu_map.py
# | Description: Writes a Waterbear program's link map, one "ADDR NAME" line per label in address order: assembles a copy of the source with a .word table of every label appended, in a mirror of the files it includes so their relative paths still resolve, and reads the addresses back from the output's tail.
# | Author: suinevere
# | Dependencies: os, re, shutil, struct, subprocess, sys, waterbear on PATH
# | Globals: N/A
# | Params: SRC.s BUILT.vms OUT.map WORKDIR
# | Returns: process exit code
#----------------------
import os
import re
import shutil
import struct
import subprocess
import sys

#----------------------
# | INCLUDE / LABEL
# | Description: A source include line (bytes or text) and a label definition at the start of a line.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
INCLUDE = re.compile(r'^\s*\.include\s+(bytes\s+)?"([^"]+)"', re.IGNORECASE)
LABEL = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*):")


#----------------------
# | scan
# | Description: Follows a source's text includes, resolved from the program's own directory as Waterbear resolves them, collecting every file read and every label defined, in source order.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: INCLUDE, LABEL
# | Params: path -- a source file; base -- the program's directory; files -- set of files read; labels -- list of labels
# | Returns: N/A
#----------------------
def scan(path, base, files, labels):
    files.add(os.path.abspath(path))
    with open(path) as f:
        for line in f:
            inc = INCLUDE.match(line)
            if inc:
                target = os.path.join(base, inc.group(2))
                if inc.group(1):
                    files.add(os.path.abspath(target))
                else:
                    scan(target, base, files, labels)
                continue
            label = LABEL.match(line)
            if label:
                labels.append(label.group(1))


#----------------------
# | link_map
# | Description: Assembles the probe in WORKDIR, checks its code matches BUILT byte for byte so no address moved, and returns each label with its address.
# | Author: suinevere
# | Dependencies: scan, waterbear
# | Globals: N/A
# | Params: src -- the program's source; built -- its assembled .vms; work -- a scratch directory
# | Returns: list of (address, label) sorted by address; raises RuntimeError when the probe fails or differs
#----------------------
def link_map(src, built, work):
    base = os.path.dirname(os.path.abspath(src))
    files, labels = set(), []
    scan(src, base, files, labels)
    shutil.rmtree(work, ignore_errors=True)
    for path in files:
        copy = os.path.join(work, os.path.abspath(path).lstrip("/\\").replace(":", ""))
        os.makedirs(os.path.dirname(copy), exist_ok=True)
        shutil.copyfile(path, copy)
    probe = os.path.join(work, base.lstrip("/\\").replace(":", ""), "vmu_map_probe.s")
    with open(src) as f:
        text = f.read()
    with open(probe, "w", newline="\n") as f:
        f.write(text + "\n" + "".join("\t.word %s\n" % name for name in labels))
    out = probe[:-2] + ".vms"
    r = subprocess.run(["waterbear", "assemble", probe, "-o", out], capture_output=True, text=True)
    if not os.path.exists(out):
        raise RuntimeError("probe failed to assemble: " + r.stdout + r.stderr)
    with open(out, "rb") as f:
        data = f.read()
    with open(built, "rb") as f:
        code = f.read()
    if len(data) != len(code) + 2 * len(labels) or data[:len(code)] != code:
        raise RuntimeError("probe does not match %s" % built)
    words = struct.unpack("<%dH" % len(labels), data[len(code):])
    return sorted(zip(words, labels))


#----------------------
# | main
# | Description: CLI: vmu_map.py SRC.s BUILT.vms OUT.map WORKDIR.
# | Author: suinevere
# | Dependencies: link_map
# | Globals: N/A
# | Params: argv -- arguments, or None for sys.argv
# | Returns: 0, 1 when the probe fails, 2 on a usage error
#----------------------
def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 4:
        sys.stderr.write("usage: vmu_map.py SRC.s BUILT.vms OUT.map WORKDIR\n")
        return 2
    try:
        entries = link_map(argv[0], argv[1], argv[3])
    except RuntimeError as e:
        sys.stderr.write("vmu_map: %s\n" % e)
        return 1
    with open(argv[2], "w", newline="\n") as f:
        f.write("".join("%04X %s\n" % (addr, name) for addr, name in entries))
    return 0


if __name__ == "__main__":
    sys.exit(main())
