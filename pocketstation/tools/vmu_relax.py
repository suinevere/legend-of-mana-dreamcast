#!/usr/bin/env python3
#----------------------
# | vmu_relax.py
# | Description: Assembles a Waterbear program with its 3-byte CALLF and JMPF shrunk to 2-byte CALL, JMP or BR wherever the target reaches, writing the rewritten sources to a mirror of the files it includes (paths relative to the working directory).
# | Author: suinevere
# | Dependencies: os, re, shutil, struct, subprocess, sys, waterbear on PATH
# | Globals: N/A
# | Params: SRC.s MIRROR_DIR OUT.vms
# | Returns: process exit code
#----------------------
import os
import re
import shutil
import struct
import subprocess
import sys

#----------------------
# | INCLUDE / SITE / MACRO_START / MACRO_END / CODE / KEEP_FILES / ERROR_AT / MAX_ROUNDS / CR
# | Description: Source patterns, the files never rewritten (fixed vector layout), Waterbear's error location, the round limit before giving up and the carriage return kept on CRLF lines.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
INCLUDE = re.compile(r'^\s*\.include\s+(bytes\s+)?"([^"]+)"', re.IGNORECASE)
SITE = re.compile(r"^(\s+)(CALLF|JMPF)(\s+)([A-Za-z_][A-Za-z0-9_]*)(\s*(;.*)?)$")
MACRO_START = re.compile(r"^\s*%macro\b", re.IGNORECASE)
MACRO_END = re.compile(r"^\s*%end\b", re.IGNORECASE)
CODE = re.compile(r"^\s+[A-Za-z]")
KEEP_FILES = {"rt_vectors.s"}
ERROR_AT = re.compile(r"^(\S+\.(?:s|inc)):(\d+):\d+", re.M)
MAX_ROUNDS = 400
CR = chr(13)


#----------------------
# | scan
# | Description: Follows text includes from the program's directory as Waterbear does, recording each file's lines and the far sites that may shrink: not in a macro, not a numeric target, not a JMPF next to another JMPF (a table).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: INCLUDE, SITE, MACRO_START, MACRO_END, CODE, KEEP_FILES, CR
# | Params: path -- a source file; base -- the program's directory; files -- dict path -> lines; blobs -- set of byte includes; sites -- list of (path, line index, op, target)
# | Returns: N/A
#----------------------
def scan(path, base, files, blobs, sites):
    path = os.path.abspath(path)
    with open(path, newline="") as f:
        lines = f.read().split("\n")
    files[path] = lines
    code = [i for i, line in enumerate(lines) if CODE.match(line) and not line.lstrip().startswith(".")]
    neighbours = {i: (code[k - 1] if k else None, code[k + 1] if k + 1 < len(code) else None) for k, i in enumerate(code)}
    in_macro = False
    for i, line in enumerate(lines):
        if MACRO_START.match(line):
            in_macro = True
        elif MACRO_END.match(line):
            in_macro = False
        inc = INCLUDE.match(line)
        if inc:
            target = os.path.join(base, inc.group(2))
            if inc.group(1):
                blobs.add(os.path.abspath(target))
            else:
                scan(target, base, files, blobs, sites)
            continue
        m = SITE.match(line.rstrip(CR))
        if not m or in_macro or os.path.basename(path) in KEEP_FILES:
            continue
        if m.group(2) == "JMPF" and any(j is not None and lines[j].split()[0] == "JMPF" for j in neighbours.get(i, ())):
            continue
        sites.append((path, i, m.group(2), m.group(4)))


#----------------------
# | mirror_path
# | Description: Where a source lands in the mirror: its path relative to the working directory under MIRROR_DIR.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: mirror -- the mirror directory; path -- an absolute source path
# | Returns: the absolute mirrored path; raises RuntimeError for a file outside the working directory
#----------------------
def mirror_path(mirror, path):
    rel = os.path.relpath(path, os.getcwd())
    if rel.startswith(".."):
        raise RuntimeError("%s is outside the working directory" % path)
    return os.path.abspath(os.path.join(mirror, rel))


#----------------------
# | write_mirror
# | Description: Writes every source into the mirror with each site in its chosen form, optionally a probe label before each site and, on the program, a .word table of site and target addresses.
# | Author: suinevere
# | Dependencies: mirror_path
# | Globals: SITE, CR
# | Params: mirror; src -- the program; files; blobs; sites; forms -- per site "far", "near" or "br"; probe -- add the labels and table
# | Returns: (the mirrored program path, dict (mirrored path, 1-based line) -> site)
#----------------------
def write_mirror(mirror, src, files, blobs, sites, forms, probe):
    shutil.rmtree(mirror, ignore_errors=True)
    edits = {}
    for k, (path, i, op, _) in enumerate(sites):
        line = files[path][i]
        m = SITE.match(line.rstrip(CR))
        new = {"far": op, "near": "CALL" if op == "CALLF" else "JMP", "br": "BR"}[forms[k]]
        text = m.group(1) + new + m.group(3) + m.group(4) + m.group(5) + (CR if line.endswith(CR) else "")
        edits[(path, i)] = (k, ["__rx_%d:" % k, text] if probe else [text])
    where = {}
    for path, lines in files.items():
        dest = mirror_path(mirror, path)
        out = []
        for i, line in enumerate(lines):
            if (path, i) in edits:
                k, new = edits[(path, i)]
                out += new
                where[(dest, len(out))] = k
            else:
                out.append(line)
        if probe and path == os.path.abspath(src):
            out.append("".join("\t.word __rx_%d, %s\n" % (k, s[3]) for k, s in enumerate(sites)))
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "w", newline="") as f:
            f.write("\n".join(out))
    for path in blobs:
        dest = mirror_path(mirror, path)
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        shutil.copyfile(path, dest)
    return mirror_path(mirror, os.path.abspath(src)), where


#----------------------
# | assemble
# | Description: Runs Waterbear, which exits 0 on failure, and checks the output exists; a failure at a rewritten site's line names that site.
# | Author: suinevere
# | Dependencies: waterbear
# | Globals: ERROR_AT
# | Params: src -- the program; out -- the .vms to write; where -- write_mirror's line map
# | Returns: (output bytes, None), or (None, site) when a 2-byte site failed to reach; raises RuntimeError on any other failure
#----------------------
def assemble(src, out, where):
    if os.path.exists(out):
        os.remove(out)
    r = subprocess.run(["waterbear", "assemble", src, "-o", out], capture_output=True, text=True)
    if not os.path.exists(out):
        for m in ERROR_AT.finditer(r.stdout + r.stderr):
            k = where.get((os.path.abspath(os.path.join(os.path.dirname(src), m.group(1))), int(m.group(2))))
            if k is not None:
                return None, k
        raise RuntimeError("waterbear failed on %s: %s%s" % (src, r.stdout, r.stderr))
    with open(out, "rb") as f:
        return f.read(), None


#----------------------
# | fits
# | Description: Whether a 2-byte form at addr reaches target: BR within -128..127 of the next instruction, CALL and JMP inside the next instruction's 4 KB block.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: form -- "near" or "br"; addr -- the site; target -- its target
# | Returns: True when it reaches
#----------------------
def fits(form, addr, target):
    nxt = addr + 2
    if form == "br":
        return -128 <= target - nxt <= 127
    return (target & 0xF000) == (nxt & 0xF000)


#----------------------
# | relax
# | Description: Chooses each site's form by rounds: probes the layout, sends any 2-byte form that no longer reaches back to far for good, shrinks far sites that now reach, and stops when a round changes nothing.
# | Author: suinevere
# | Dependencies: scan, write_mirror, assemble, fits
# | Globals: MAX_ROUNDS
# | Params: src; mirror; out -- the final .vms
# | Returns: (sites shrunk, sites, rounds); raises RuntimeError when the rounds do not settle
#----------------------
def relax(src, mirror, out):
    files, blobs, sites = {}, set(), []
    scan(src, os.path.dirname(os.path.abspath(src)), files, blobs, sites)
    forms = ["far"] * len(sites)
    pinned = set()
    for rounds in range(1, MAX_ROUNDS + 1):
        program, where = write_mirror(mirror, src, files, blobs, sites, forms, True)
        code, failed = assemble(program, out, where)
        if failed is not None:
            forms[failed] = "far"
            pinned.add(failed)
            continue
        words = struct.unpack("<%dH" % (2 * len(sites)), code[len(code) - 4 * len(sites):]) if sites else ()
        bad = [k for k in range(len(sites)) if forms[k] != "far" and not fits(forms[k], words[2 * k], words[2 * k + 1])]
        for k in bad:
            forms[k] = "far"
            pinned.add(k)
        if bad:
            continue
        grown = False
        for k, (_, _, op, _) in enumerate(sites):
            if forms[k] != "far" or k in pinned:
                continue
            for form in (("br", "near") if op == "JMPF" else ("near",)):
                if fits(form, words[2 * k], words[2 * k + 1]):
                    forms[k] = form
                    grown = True
                    break
        if not grown:
            program, where = write_mirror(mirror, src, files, blobs, sites, forms, False)
            if assemble(program, out, where)[0] is None:
                raise RuntimeError("the settled layout failed to assemble without its probe")
            return sum(f != "far" for f in forms), len(sites), rounds
    raise RuntimeError("relaxation did not settle in %d rounds" % MAX_ROUNDS)


#----------------------
# | main
# | Description: CLI: vmu_relax.py SRC.s MIRROR_DIR OUT.vms.
# | Author: suinevere
# | Dependencies: relax
# | Globals: N/A
# | Params: argv -- arguments, or None for sys.argv
# | Returns: 0, 1 on failure, 2 on a usage error
#----------------------
def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 3:
        sys.stderr.write("usage: vmu_relax.py SRC.s MIRROR_DIR OUT.vms\n")
        return 2
    try:
        shrunk, total, rounds = relax(argv[0], argv[1], argv[2])
    except RuntimeError as e:
        sys.stderr.write("vmu_relax: %s\n" % e)
        return 1
    print("vmu_relax: %d of %d far sites shrunk in %d rounds" % (shrunk, total, rounds))
    return 0


if __name__ == "__main__":
    sys.exit(main())
