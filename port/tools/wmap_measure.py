#----------------------
# | wmap_measure.py
# | Description: Cross-compiles every WMAP decomp file with the Saturn build's own compiler line and prints per-file and total SH-2 section sizes.
# | Author: suinevere
# | Dependencies: saturn/makefile, saturn/compile.bat toolchain, sh2eb-elf-size
# | Globals: SATURN, REPO, CDIR
# | Params: N/A
# | Returns: N/A
#----------------------
import glob
import os
import re
import subprocess
import sys
import tempfile

SATURN = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(SATURN)
CDIR = os.path.join(REPO, "SaturnRingLib", "Compiler")

#----------------------
# | SECTION_GROUPS
# | Description: Output section prefixes folded into the four totals the memory table needs.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECTION_GROUPS = (("text", ".text"), ("rodata", ".rodata"), ("data", ".data"), ("bss", ".bss"))


#----------------------
# | toolchain_env
# | Description: Copies the environment with compile.bat's PATH additions so make, sh and sh2eb-elf tools resolve.
# | Author: suinevere
# | Dependencies: CDIR
# | Globals: N/A
# | Params: N/A
# | Returns: environment dict
#----------------------
def toolchain_env():
    env = dict(os.environ)
    extra = [os.path.join(CDIR, "sh2eb-elf", "bin"), os.path.join(CDIR, "msys2", "usr", "bin"), os.path.join(CDIR, "Other Utilities")]
    env["PATH"] = os.pathsep.join(extra + [env.get("PATH", "")])
    env["SRL_INSTALL_ROOT"] = "../SaturnRingLib"
    return env


#----------------------
# | compile_line
# | Description: Asks make for the exact command that builds src/wsel/wsel_unit.o, the template every WMAP compile reuses.
# | Author: suinevere
# | Dependencies: toolchain_env
# | Globals: SATURN
# | Params: env -- toolchain environment
# | Returns: the compiler command line as one string
#----------------------
def compile_line(env):
    out = subprocess.run(["make", "-n", "-B", "DEBUG=1", "src/wsel/wsel_unit.o"], cwd=SATURN, env=env,
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        if "wsel_unit.c" in line and "gcc" in line:
            return line
    raise SystemExit("make -n printed no compile line for wsel_unit.c")


#----------------------
# | parse_size_output
# | Description: Folds `size -A` output into text, rodata, data and bss totals, counting every subsection under its prefix.
# | Author: suinevere
# | Dependencies: SECTION_GROUPS
# | Globals: N/A
# | Params: text -- size -A output for one object
# | Returns: dict of the four totals in bytes
#----------------------
def parse_size_output(text):
    totals = {key: 0 for key, _ in SECTION_GROUPS}
    for line in text.splitlines():
        m = re.match(r"\s*(\.\S+)\s+(\d+)\s+\d+", line)
        if not m:
            continue
        for key, prefix in SECTION_GROUPS:
            if m.group(1) == prefix or m.group(1).startswith(prefix + "."):
                totals[key] += int(m.group(2))
    return totals


#----------------------
# | measure
# | Description: Compiles each WMAP decomp file through a temporary unit with the template line, runs size -A on it and prints the table and totals.
# | Author: suinevere
# | Dependencies: toolchain_env, compile_line, parse_size_output
# | Globals: SATURN, REPO
# | Params: N/A
# | Returns: process exit code
#----------------------
def measure():
    env = toolchain_env()
    template = compile_line(env)
    sh = os.path.join(CDIR, "msys2", "usr", "bin", "sh.exe")
    size_bin = os.path.join(CDIR, "sh2eb-elf", "bin", "sh2eb-elf-size.exe")
    work = tempfile.mkdtemp(prefix="lom_wmap_measure_")
    total = {key: 0 for key, _ in SECTION_GROUPS}
    sources = sorted(glob.glob(os.path.join(REPO, "src", "overlays", "wmap", "**", "*.c"), recursive=True))
    for path in sources:
        rel = os.path.relpath(path, os.path.join(REPO, "src")).replace(os.sep, "/")
        unit = os.path.join(work, "unit.c")
        with open(unit, "w") as handle:
            handle.write('#include "%s"\n' % rel)
        obj = os.path.join(work, "unit.o")
        line = template.replace("src/wsel/wsel_unit.c", unit.replace(os.sep, "/")).replace("src/wsel/wsel_unit.o", obj.replace(os.sep, "/"))
        subprocess.run([sh, "-c", line], cwd=SATURN, env=env, check=True)
        sizes = parse_size_output(subprocess.run([size_bin, "-A", obj], env=env, capture_output=True, text=True, check=True).stdout)
        print("%-50s text %7d rodata %6d data %6d bss %6d" % (rel, sizes["text"], sizes["rodata"], sizes["data"], sizes["bss"]))
        for key in total:
            total[key] += sizes[key]
    print("TOTAL text %d rodata %d data %d bss %d" % (total["text"], total["rodata"], total["data"], total["bss"]))
    return 0


if __name__ == "__main__":
    sys.exit(measure())
