"""----------------------
 | budget.py
 | Description: Instruction budget of the VMU-shaped Ring Ring Land: builds it with vmu_host.c and budget_main.c for Cortex-M0 at -Os, runs a script under Unicorn, and counts the instructions the game's own code runs per frame and per function.
 | Author: suinevere
 | Dependencies: gen_ram.py, runner/budget_main.c, runner/budget.ld, arm-none-eabi-gcc, unicorn, capstone (lom-pocket image)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import bisect
import glob
import os
import struct
import subprocess
import sys

import capstone
from unicorn import UC_ARCH_ARM, UC_HOOK_BLOCK, UC_HOOK_CODE, UC_MODE_MCLASS, UC_MODE_THUMB, Uc
from unicorn.arm_const import UC_ARM_REG_SP

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_ram  # noqa: E402

"""----------------------
 | PS / OUT / CARD
 | Description: The PocketStation tree, where the budget build goes, and the card the tests run from.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
PS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(PS, "build", "budget")
CARD = os.path.join(PS, "build", "rrl.card")

"""----------------------
 | CFLAGS
 | Description: The Cortex-M0 build of the game, its host backend and the driver.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
CFLAGS = ["arm-none-eabi-gcc", "-mcpu=cortex-m0", "-mthumb", "-Os", "-std=c11", "-Wall", "-Wextra", "-Werror",
          "-ffunction-sections", "-I" + os.path.join(PS, "src", "vmu"), "-I" + os.path.join(PS, "build", "vmu", "gen")]

"""----------------------
 | IN_BASE / STACK_TOP / STEP_REBOOT
 | Description: Where the inputs go (budget_main.c's BUDGET_IN), the stack's top, and the reboot step byte.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
IN_BASE = 0x30000000
STACK_TOP = 0x20100000
STEP_REBOOT = 0x80


"""----------------------
 | run
 | Description: Runs a build command, stopping on failure.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: cmd -- argument list
 | Returns: N/A
 ----------------------"""
def run(cmd: list) -> None:
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        raise SystemExit("budget: %s\n%s%s" % (" ".join(cmd), r.stdout, r.stderr))


"""----------------------
 | build
 | Description: Builds build/budget/budget.elf: the game's files into libgame.a, which budget.ld places between __game_start and __game_end, linked with vmu_host.c and budget_main.c.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: the ELF's path
 ----------------------"""
def build() -> str:
    gen = os.path.join(PS, "build", "vmu", "gen")
    os.makedirs(gen, exist_ok=True)
    os.makedirs(OUT, exist_ok=True)
    if gen_ram.main(["vmu", os.path.join(PS, "src", "game", "ram_map.txt"), os.path.join(gen, "rrl_ram.h")]):
        raise SystemExit("budget: gen_ram failed")
    objs = []
    for src in sorted(glob.glob(os.path.join(PS, "src", "vmu", "*.c"))):
        if os.path.basename(src) == "vmu_host.c":
            continue
        obj = os.path.join(OUT, os.path.basename(src)[:-2] + ".o")
        run(CFLAGS + ["-c", src, "-o", obj])
        objs.append(obj)
    lib = os.path.join(OUT, "libgame.a")
    if os.path.exists(lib):
        os.remove(lib)
    run(["arm-none-eabi-ar", "rcs", lib] + objs)
    host = os.path.join(OUT, "vmu_host.o")
    main = os.path.join(OUT, "budget_main.o")
    run(CFLAGS + ["-c", os.path.join(PS, "src", "vmu", "vmu_host.c"), "-o", host])
    run(CFLAGS + ["-c", os.path.join(PS, "runner", "budget_main.c"), "-o", main])
    elf = os.path.join(OUT, "budget.elf")
    run(["arm-none-eabi-gcc", "-mcpu=cortex-m0", "-mthumb", "-nostartfiles", "--specs=nosys.specs",
         "-T", os.path.join(PS, "runner", "budget.ld"), main, host, lib, "-o", elf])
    return elf


"""----------------------
 | read_elf
 | Description: The loadable segments and the symbols of a 32-bit little-endian ELF.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- ELF file
 | Returns: (segments as (address, bytes, memory size), every symbol as (name, value, size, is function), local copies of one name included)
 ----------------------"""
def read_elf(path: str):
    with open(path, "rb") as f:
        e = f.read()
    phoff, shoff = struct.unpack_from("<II", e, 0x1C)
    phentsize, phnum, shentsize, shnum = struct.unpack_from("<HHHH", e, 0x2A)
    segs = []
    for i in range(phnum):
        p_type, p_off, p_vaddr, _, p_filesz, p_memsz = struct.unpack_from("<IIIIII", e, phoff + i * phentsize)
        if p_type == 1:
            segs.append((p_vaddr, e[p_off:p_off + p_filesz], p_memsz))
    secs = [struct.unpack_from("<IIIIIIIIII", e, shoff + i * shentsize) for i in range(shnum)]
    syms = []
    for s in secs:
        if s[1] != 2:
            continue
        strtab = secs[s[6]]
        for k in range(s[5] // 16):
            name, value, size, info = struct.unpack_from("<IIIB", e, s[4] + k * 16)
            end = e.index(b"\0", strtab[4] + name)
            syms.append((e[strtab[4] + name:end].decode(), value, size, (info & 0xF) == 2))
    return segs, syms


"""----------------------
 | card_file
 | Description: The game's file from the card: its 8 KB blocks in directory chain order from block 1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- 128 KB card
 | Returns: file bytes
 ----------------------"""
def card_file(path: str) -> bytes:
    with open(path, "rb") as f:
        card = f.read()
    out, b = b"", 1
    for _ in range(15):
        out += card[b * 0x2000:(b + 1) * 0x2000]
        link = card[b * 0x80 + 8] | (card[b * 0x80 + 9] << 8)
        if link == 0xFFFF:
            break
        b = link + 1
    return out


"""----------------------
 | script_steps
 | Description: A .pks script expanded as pkrun does into step bytes: each sample's button mask, STEP_REBOOT for a reboot; rtc lines drop out.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- script
 | Returns: step bytes
 ----------------------"""
def script_steps(path: str) -> bytes:
    out = bytearray()
    with open(path) as f:
        for line in f:
            w = line.split("#", 1)[0].split()
            if not w:
                continue
            if w[0] == "end":
                break
            if w[0] == "wait":
                out += bytes(int(w[1]))
            elif w[0] == "hold":
                out += bytes([int(w[1], 16)]) * int(w[2])
            elif w[0] == "press":
                out += bytes([int(w[1], 16), 0])
            elif w[0] == "reboot":
                out.append(STEP_REBOOT)
            elif w[0] != "rtc":
                raise SystemExit("budget: bad line in %s: %s" % (path, line))
    return bytes(out)


"""----------------------
 | entry_boots
 | Description: Each app entry of a pkrun --dump-entry-state file as a BudgetBoot: word count, system flags from the kernel words at 0xC0 and 0xD8, 64 device words.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- entry-state file
 | Returns: the records' bytes and their count
 ----------------------"""
def entry_boots(path: str):
    with open(path, "rb") as f:
        data = f.read()
    out, at, n = b"", 0, 0
    while at + 0x804 <= len(data):
        wram = data[at:at + 0x800]
        count = struct.unpack_from("<I", data, at + 0x800)[0]
        words = list(struct.unpack_from("<%dI" % count, data, at + 0x804))
        com, ui = struct.unpack_from("<I", wram, 0xC0)[0], struct.unpack_from("<I", wram, 0xD8)[0]
        sys_flags = (1 if com & 1 else 0) | (2 if com & 0x100 else 0) | (((ui >> 18) & 3) << 2)
        out += struct.pack("<II64I", count, sys_flags, *(words + [0] * (64 - count)))
        at += 0x804 + 4 * count
        n += 1
    return out, n


"""----------------------
 | inputs
 | Description: The input blob at IN_BASE: header, then the file, the steps and the boots, each 4-byte aligned.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: script -- script name
 | Returns: blob bytes
 ----------------------"""
def inputs(script: str) -> bytes:
    file = card_file(CARD)
    steps = script_steps(os.path.join(PS, "tests", "scripts", script + ".pks"))
    boots, nboots = entry_boots(os.path.join(PS, "build", "test", script + ".entry"))
    file_off = 0x100
    steps_off = file_off + len(file)
    boots_off = (steps_off + len(steps) + 3) & ~3
    blob = bytearray(boots_off + len(boots))
    struct.pack_into("<6I", blob, 0, len(file), file_off, len(steps), steps_off, nboots, boots_off)
    blob[file_off:file_off + len(file)] = file
    blob[steps_off:steps_off + len(steps)] = steps
    blob[boots_off:] = boots
    return bytes(blob)


"""----------------------
 | percentile
 | Description: The value at a fraction through a sorted list, nearest rank.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: values -- sorted list; p -- fraction
 | Returns: the value
 ----------------------"""
def percentile(values: list, p: float) -> int:
    return values[min(len(values) - 1, max(0, int(round(p * len(values))) - 1))]


"""----------------------
 | measure
 | Description: Runs the budget build over a script under Unicorn, counting each executed block of game code by its Capstone instruction count, splitting frames at budget_frame_mark.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: elf -- budget.elf; blob -- inputs
 | Returns: (instructions per frame, instructions per function)
 ----------------------"""
def measure(elf: str, blob: bytes):
    segs, all_syms = read_elf(elf)
    syms = {}
    for n, v, size, fn in all_syms:
        syms.setdefault(n, (v, size, fn))
    lo, hi = syms["__game_start"][0], syms["__game_end"][0]
    funcs = sorted((v & ~1, n) for n, v, size, fn in all_syms if fn and lo <= (v & ~1) < hi)
    starts = [a for a, _ in funcs]
    mu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    mu.mem_map(0, 16 << 20)
    mu.mem_map(STACK_TOP - (1 << 20), 1 << 20)
    mu.mem_map(IN_BASE, (len(blob) + 0xFFFF) & ~0xFFFF)
    for addr, data, _ in segs:
        mu.mem_write(addr, data)
    mu.mem_write(IN_BASE, blob)
    mu.reg_write(UC_ARM_REG_SP, STACK_TOP)
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB | capstone.CS_MODE_MCLASS)
    sizes, per_func, frames, state = {}, {}, [], {"now": 0, "end": None}

    def on_block(uc, addr, size, _):
        n = sizes.get(addr)
        if n is None:
            n = sizes[addr] = sum(1 for _ in md.disasm_lite(bytes(uc.mem_read(addr, size)), addr))
        state["now"] += n
        name = funcs[bisect.bisect_right(starts, addr) - 1][1]
        per_func[name] = per_func.get(name, 0) + n

    def on_mark(uc, addr, size, _):
        frames.append(state["now"])
        state["now"] = 0

    def on_stop(uc, addr, size, which):
        state["end"] = which
        uc.emu_stop()

    mu.hook_add(UC_HOOK_BLOCK, on_block, begin=lo, end=hi - 1)
    mark = syms["budget_frame_mark"][0] & ~1
    mu.hook_add(UC_HOOK_CODE, on_mark, begin=mark, end=mark)
    for name in ("budget_done", "_exit"):
        if name in syms:
            a = syms[name][0] & ~1
            mu.hook_add(UC_HOOK_CODE, on_stop, name, begin=a, end=a)
    mu.emu_start(syms["_start"][0] | 1, 0xFFFFFFFF)
    if state["end"] != "budget_done":
        raise SystemExit("budget: the run stopped at %s after %d frames" % (state["end"], len(frames)))
    return frames, per_func


"""----------------------
 | main
 | Description: CLI: budget.py SCRIPT; prints the frame count, mean, p90 and max instructions a frame with the max's frame, and the 15 costliest functions.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 1:
        print("usage: budget.py SCRIPT", file=sys.stderr)
        return 2
    frames, per_func = measure(build(), inputs(argv[0]))
    ordered = sorted(frames)
    peak = max(range(len(frames)), key=lambda i: frames[i])
    print("frames %d" % len(frames))
    print("mean %d p90 %d max %d at %d" % (sum(frames) // len(frames), percentile(ordered, 0.9), frames[peak], peak))
    print("top")
    for name, n in sorted(per_func.items(), key=lambda kv: -kv[1])[:15]:
        print("%12d %s" % (n, name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
