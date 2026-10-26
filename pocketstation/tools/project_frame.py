"""----------------------
 | project_frame.py
 | Description: Projects a race frame's LC86K assembly cycles from the hot set's assembly, scaling the rest of the C frame by the hot set's own cycles-per-C-instruction ratio, and reports the mean and p90 race frame against 28,700, under two assumptions for the non-hot share.
 | Author: suinevere
 | Dependencies: call_compare.py, vmu_callgraph.py, pocketstation/src/vmu_game/manifest.txt, pocketstation/build/record/rec/calls-race.txt, pocketstation/tools/budget.py (run in the lom-pocket image), make vmu-record, make -C pocketstation/harness, make -C pocketstation/src/vmu_game call
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------
 Method.

 The hot set is HOT_ROOTS plus fb.c, walked downward through vmu_callgraph.callees, minus inlined
 rows. A member every one of whose C-level callers is itself in the hot set is dead (0 cycles), its
 work already inside the caller that bypasses it (game_sprite_draw calls plot_core/unpack_core
 directly; g_gather_core calls g_branch_core directly).

 Per-call cycles come from vmu_harness --call via call_compare.check(), over choose_tier()'s
 narrowest record tier (race frames 1000-1100, then all of race, then every script) with at least
 MIN_SAMPLE passing calls. game_board_gather is recursive, so price_top_level() prices it from its
 shallowest-depth (top-level) race records only. game_sprite_draw varies strongly with its
 arguments, the bitmap included now that stored rows are found by it, so weighted_price() prices each
 distinct argument tuple the real race run produced (read by a Unicorn pass over budget.py's build) by
 synthesizing it against an existing record's background state and running it once,
 true-count-weighted. plot_rows is priced the same way to show what game_sprite_draw spends beyond it.

 Calls a race frame come from calls-race.txt's entered count, corrected for recursion
 (game_board_gather, game_nibble) by the same Unicorn pass's LR attribution: a call's caller is the
 reverse-callgraph owner of its return address, and only callers outside the hot set count.

 rom32's hot-attributable C instructions are its whole budget.py bucket times the LR-attributed
 fraction of its calls whose caller is hot (rom32 is not itself a hot-set member, by manifest);
 rev32's are its whole bucket, both its callers being hot.

 Two scenarios credit the hot set's C instructions differently: scenario 1 credits rev32's and
 rom32's hot share; scenario 2 credits only rom32's, showing the projection's sensitivity to crediting
 rev32. compute_projection() has the arithmetic.
 ----------------------"""

import argparse
import copy
import importlib.util
import os
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import call_compare as cc  # noqa: E402
import vmu_callgraph as vc  # noqa: E402

"""----------------------
 | HOT_ROOTS
 | Description: The hot set's roots, as test_vmu_game.py's _hot_set() builds it: the sprite blitter, row unpacking and the board's recursion group, plus every fb.c function.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
HOT_ROOTS = ("game_sprite_draw", "unpack_rows", "game_board_gather", "board_branch")

"""----------------------
 | SPRITE_DRAW / PLOT_ROWS / REV32_NAME / ROM32_NAME / BOARD_GATHER
 | Description: Names handled specially: game_sprite_draw is priced over its true argument tuples, plot_rows beside it; rev32 and rom32 are inlined accessors (manifest "inlined:*", not hot-set members) credited to the hot set's C instructions.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
SPRITE_DRAW = "game_sprite_draw"
PLOT_ROWS = "plot_rows"
REV32_NAME = "rev32"
ROM32_NAME = "rom32"
BOARD_GATHER = "game_board_gather"

"""----------------------
 | PLOT_ARG_INDICES / SPRITE_ARG_INDICES
 | Description: Record.args positions holding each weighted-pricing key's fields, in the manifest's ARGS order: plot_rows's x,y,width,height,flags,unpacked (bitmap left at its template value), and game_sprite_draw's x,y,width,height,bitmap,flags.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
PLOT_ARG_INDICES = (0, 1, 2, 3, 5, 6)
SPRITE_ARG_INDICES = (0, 1, 2, 3, 4, 5)

"""----------------------
 | ATTRIB_NAMES
 | Description: Hot-set members whose true calls-a-frame needs the LR attribution pass rather than calls-race.txt's entered count, because a hot-set caller as well as an outside one can reach them (game_board_gather's recursion; game_nibble's internal calls from inside it) or because confirming every call is already from outside the hot set is cheap insurance (game_sprite_draw, game_fb_clear, game_fb_present, game_state_reset). game_fb_put_rows is left out: it has 0 instructions in budget.py's race build (inlined into its one caller) and 0 entered calls in race, so no attribution is needed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
ATTRIB_NAMES = ("game_sprite_draw", "game_board_gather", "game_fb_clear", "game_fb_present",
                "game_nibble", "game_state_reset")

"""----------------------
 | FRAME_BUDGET / GATE_FRACTION / RACE_SCRIPT / RACE_FRAME_MIN / RACE_FRAME_MAX
 | Description: The game's cycle budget a frame, the stop gate's fraction of it, the script the projection covers and the record frame window Task 7's ruling names.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FRAME_BUDGET = 28700
GATE_FRACTION = 0.80
RACE_SCRIPT = "race"
RACE_FRAME_MIN = 1000
RACE_FRAME_MAX = 1100

"""----------------------
 | MIN_SAMPLE
 | Description: The fewest passing calls a tier needs before choose_tier() accepts it over a wider one. A single sampled call is not a mean, so two is the floor.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
MIN_SAMPLE = 2

"""----------------------
 | FROZEN_RATIO1 / FROZEN_RATIO2
 | Description: Task 7's C-to-assembly ratios (scenarios 1 and 2), before sprite work moved to build time; pricing the rest of the frame at them keeps that move from cheapening unchanged C.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FROZEN_RATIO1 = 0.7159
FROZEN_RATIO2 = 0.8227


"""----------------------
 | hot_set
 | Description: The hot set's C function names: HOT_ROOTS plus every fb.c function, walked downward through vmu_callgraph.callees, minus inlined manifest rows.
 | Author: suinevere
 | Dependencies: vmu_callgraph
 | Globals: HOT_ROOTS
 | Params: funcs -- vmu_callgraph.discover()'s result; rows -- vmu_callgraph.parse_manifest()'s result
 | Returns: sorted list of C function names
 ----------------------"""
def hot_set(funcs: dict, rows: dict) -> list:
    todo = list(HOT_ROOTS) + sorted(n for n, f in funcs.items() if f.file == "fb.c")
    seen = set()
    while todo:
        name = todo.pop()
        if name not in seen:
            seen.add(name)
            todo.extend(vc.callees(name, funcs))
    return sorted(n for n in seen if not rows[n][3].startswith("inlined:"))


"""----------------------
 | reverse_callers
 | Description: Every function's direct C-level callers, the reverse of vmu_callgraph.callees over the whole call graph.
 | Author: suinevere
 | Dependencies: vmu_callgraph.callees
 | Globals: N/A
 | Params: funcs -- vmu_callgraph.discover()'s result
 | Returns: dict callee name -> set of caller names
 ----------------------"""
def reverse_callers(funcs: dict) -> dict:
    rev = {}
    for name in funcs:
        for callee in vc.callees(name, funcs):
            rev.setdefault(callee, set()).add(name)
    return rev


"""----------------------
 | has_outside_caller
 | Description: Whether a hot-set member has a C-level caller that is not itself in the hot set. A member with none is reached only from inside the hot set, so its work is already inside whichever hot-set caller's measured cycles include it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: name -- the C function name; rev -- reverse_callers()'s result; hot -- the hot set, as a set
 | Returns: True when some caller outside the hot set exists
 ----------------------"""
def has_outside_caller(name: str, rev: dict, hot: set) -> bool:
    return any(caller not in hot for caller in rev.get(name, ()))


"""----------------------
 | race_entered
 | Description: Reads pocketstation/build/record/rec/calls-SCRIPT.txt: every call entered, by function name, nested calls included (not corrected for recursion -- see outside_calls for the corrected count).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: script -- the script name
 | Returns: dict name -> entered count
 ----------------------"""
def race_entered(script: str) -> dict:
    path = os.path.join(cc.REPO, "pocketstation", "build", "record", "rec", "calls-%s.txt" % script)
    out = {}
    with open(path) as f:
        for line in f:
            name, entered, returned = line.split()
            out[name] = int(entered)
    return out


"""----------------------
 | outside_calls
 | Description: A routine's calls from outside the hot set, from the LR attribution pass; None when the routine was not attributed (race_entered's naive count is the fallback).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: name -- the C function name; attrib_callers -- run_budget_and_attribution()'s per-caller counts; hot -- the hot set, as a set
 | Returns: int, or None if name is not a key of attrib_callers
 ----------------------"""
def outside_calls(name: str, attrib_callers: dict, hot: set):
    if name not in attrib_callers:
        return None
    return sum(n for caller, n in attrib_callers[name].items() if caller not in hot)


"""----------------------
 | budget_available_locally
 | Description: Whether this process can run budget.py's build itself: the Cortex-M0 cross compiler on PATH and unicorn and capstone importable.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: True when it can
 ----------------------"""
def budget_available_locally() -> bool:
    return (shutil.which("arm-none-eabi-gcc") is not None and importlib.util.find_spec("unicorn") is not None
            and importlib.util.find_spec("capstone") is not None)


"""----------------------
 | ATTRIBUTION_SCRIPT
 | Description: The program run through python3 (directly or in the lom-pocket container) to get budget.py race's frame/sum/p90, the named functions' instruction buckets, and an LR-read attribution of every caller of each name in ATTRIB and rom32, over every one of a name's compiled instances (a static or file-local routine used from more than one file compiles to more than one address, such as rom32's three copies).
 | Author: suinevere
 | Dependencies: pocketstation/tools/budget.py, unicorn, capstone
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
ATTRIBUTION_SCRIPT = """
import sys
sys.path.insert(0, 'pocketstation/tools')
import bisect
import struct
import budget
from unicorn import UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_SP
from unicorn import UC_ARCH_ARM, UC_MODE_MCLASS, UC_MODE_THUMB, Uc

SCRIPT = %(script)r
NAMES = %(names)r
ATTRIB = %(attrib)r
SPRITE_MIRROR = 4
SPRITE_DRAW_NAME = 'game_sprite_draw'

elf = budget.build()
blob = budget.inputs(SCRIPT)
frames, per_func = budget.measure(elf, blob)
print('frames', len(frames))
print('sum', sum(frames))
print('p90', budget.percentile(sorted(frames), 0.9))
for n in NAMES:
    print('bucket', n, per_func.get(n, 0))

segs, all_syms = budget.read_elf(elf)
all_by_name = {}
for n, v, size, fn in all_syms:
    all_by_name.setdefault(n, []).append(v)
lo, hi = all_by_name['__game_start'][0], all_by_name['__game_end'][0]
funcs = sorted((v & ~1, n) for n, v, size, fn in all_syms if fn and lo <= (v & ~1) < hi)
starts = [a for a, _ in funcs]


def owner(addr):
    i = bisect.bisect_right(starts, addr) - 1
    return funcs[i][1] if 0 <= i < len(funcs) else '?'


by_name = {}
for a, n in funcs:
    by_name.setdefault(n, []).append(a)

mu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
mu.mem_map(0, 16 << 20)
mu.mem_map(budget.STACK_TOP - (1 << 20), 1 << 20)
mu.mem_map(budget.IN_BASE, (len(blob) + 0xFFFF) & ~0xFFFF)
for addr, data, _ in segs:
    mu.mem_write(addr, data)
mu.mem_write(budget.IN_BASE, blob)
mu.reg_write(UC_ARM_REG_SP, budget.STACK_TOP)

counts = {}
plot_tuples = {}
sprite_tuples = {}


def s16(v):
    v &= 0xFFFF
    return v - 0x10000 if v >= 0x8000 else v


def make_hook(label):
    def hook(uc, addr, size, _):
        lr = uc.reg_read(UC_ARM_REG_LR) & ~1
        c = counts.setdefault(label, {})
        name = owner(lr)
        c[name] = c.get(name, 0) + 1
    return hook


def hook_sprite_draw(uc, addr, size, _):
    lr = uc.reg_read(UC_ARM_REG_LR) & ~1
    c = counts.setdefault(SPRITE_DRAW_NAME, {})
    name = owner(lr)
    c[name] = c.get(name, 0) + 1
    x = s16(uc.reg_read(UC_ARM_REG_R0))
    y = s16(uc.reg_read(UC_ARM_REG_R1))
    width = uc.reg_read(UC_ARM_REG_R2) & 0xFF
    height = uc.reg_read(UC_ARM_REG_R3) & 0xFF
    sp = uc.reg_read(UC_ARM_REG_SP)
    bitmap = struct.unpack('<I', uc.mem_read(sp, 4))[0]
    flags = struct.unpack('<I', uc.mem_read(sp + 4, 4))[0] & 0xFF
    unpacked = 1 if (width != 32 or (flags & SPRITE_MIRROR)) else 0
    key = (x, y, width, height, flags, unpacked)
    plot_tuples[key] = plot_tuples.get(key, 0) + 1
    key = (x, y, width, height, bitmap, flags)
    sprite_tuples[key] = sprite_tuples.get(key, 0) + 1


for name in ATTRIB:
    hook = hook_sprite_draw if name == SPRITE_DRAW_NAME else make_hook(name)
    for a in by_name.get(name, []):
        mu.hook_add(UC_HOOK_CODE, hook, begin=a, end=a)

state = {'end': None}


def on_stop(uc, addr, size, which):
    state['end'] = which
    uc.emu_stop()


for name in ('budget_done', '_exit'):
    for a in all_by_name.get(name, []):
        mu.hook_add(UC_HOOK_CODE, on_stop, name, begin=a & ~1, end=a & ~1)

mu.emu_start(all_by_name['_start'][0] | 1, 0xFFFFFFFF)
if state['end'] != 'budget_done':
    raise SystemExit('attribution run stopped at %%s' %% state['end'])

for label in ATTRIB:
    c = counts.get(label, {})
    print('attrib_total', label, sum(c.values()))
    for caller, n in sorted(c.items(), key=lambda kv: -kv[1]):
        print('attrib_caller', label, caller, n)

for key, n in sorted(plot_tuples.items()):
    print('plot_tuple', key[0], key[1], key[2], key[3], key[4], key[5], n)
for key, n in sorted(sprite_tuples.items()):
    print('sprite_tuple', key[0], key[1], key[2], key[3], key[4], key[5], n)
"""


"""----------------------
 | run_budget_and_attribution
 | Description: Runs budget.py's build and measure() for a script plus the LR attribution pass (ATTRIBUTION_SCRIPT), directly when this process can run budget.py's build itself, else in the lom-pocket container.
 | Author: suinevere
 | Dependencies: budget_available_locally, ATTRIBUTION_SCRIPT
 | Globals: N/A
 | Params: script -- the script name; names -- C function names to read a bucket for; attrib -- names to run the LR attribution pass on; docker -- True, False or None to decide
 | Returns: (frames, sum_instr, mean, p90, per_func dict name -> instructions, attrib_totals dict name -> int, attrib_callers dict name -> {caller: int}, plot_tuples dict (x,y,width,height,flags,unpacked) -> true count, sprite_tuples dict (x,y,width,height,bitmap,flags) -> true count)
 ----------------------"""
def run_budget_and_attribution(script: str, names: tuple, attrib: tuple, docker=None):
    if docker is None:
        docker = not budget_available_locally()
    code = ATTRIBUTION_SCRIPT % dict(script=script, names=tuple(names), attrib=tuple(attrib))
    cmd = ["python3", "-c", code]
    if docker:
        cmd = ["docker", "run", "--rm", "-v", cc.REPO + ":/work", "-w", "/work", "lom-pocket"] + cmd
    r = subprocess.run(cmd, cwd=cc.REPO, capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))
    if r.returncode != 0:
        raise RuntimeError("budget/attribution run failed: " + r.stdout[-3000:] + r.stderr[-3000:])
    frames = sum_instr = p90 = None
    per_func, attrib_totals, attrib_callers = {}, {}, {}
    plot_tuples, sprite_tuples = {}, {}
    for line in r.stdout.splitlines():
        parts = line.split()
        if not parts:
            continue
        if parts[0] == "frames":
            frames = int(parts[1])
        elif parts[0] == "sum":
            sum_instr = int(parts[1])
        elif parts[0] == "p90":
            p90 = int(parts[1])
        elif parts[0] == "bucket":
            per_func[parts[1]] = int(parts[2])
        elif parts[0] == "attrib_total":
            attrib_totals[parts[1]] = int(parts[2])
        elif parts[0] == "attrib_caller":
            attrib_callers.setdefault(parts[1], {})[parts[2]] = int(parts[3])
        elif parts[0] == "plot_tuple":
            x, y, width, height, flags, unpacked, n = (int(v) for v in parts[1:])
            plot_tuples[(x, y, width, height, flags, unpacked)] = n
        elif parts[0] == "sprite_tuple":
            x, y, width, height, bitmap, flags, n = (int(v) for v in parts[1:])
            sprite_tuples[(x, y, width, height, bitmap, flags)] = n
    if frames is None or sum_instr is None or p90 is None:
        raise RuntimeError("budget run gave no usable output: " + r.stdout)
    return frames, sum_instr, sum_instr / frames, p90, per_func, attrib_totals, attrib_callers, plot_tuples, sprite_tuples


"""----------------------
 | sprite_draws
 | Description: How often the C draws each bitmap over one script: game_sprite_draw's calls from the attribution pass, counted by table offset, width and height.
 | Author: suinevere
 | Dependencies: run_budget_and_attribution
 | Globals: SPRITE_DRAW
 | Params: script -- the script name; docker -- passed to run_budget_and_attribution
 | Returns: (frames, dict (offset, width, height) -> calls)
 ----------------------"""
def sprite_draws(script: str, docker=None):
    result = run_budget_and_attribution(script, (), (SPRITE_DRAW,), docker)
    draws = {}
    for (_, _, width, height, bitmap, _), n in result[8].items():
        key = (bitmap & 0xFFFFFF, width, height)
        draws[key] = draws.get(key, 0) + n
    return result[0], draws


"""----------------------
 | choose_tier
 | Description: The narrowest of three record filters for a function's .rec file that still has MIN_SAMPLE calls: race frames RACE_FRAME_MIN-RACE_FRAME_MAX, else all of race, else every script; when none reaches MIN_SAMPLE, the richest non-empty one of the three (every script is a superset of the other two, so it is never emptier than they are).
 | Author: suinevere
 | Dependencies: call_compare.read_records
 | Globals: RACE_SCRIPT, RACE_FRAME_MIN, RACE_FRAME_MAX, MIN_SAMPLE
 | Params: path -- the .rec file
 | Returns: (tier label, dict of check()'s script/frame_min/frame_max kwargs, filtered record count)
 ----------------------"""
def choose_tier(path: str):
    recs = cc.read_records(path)
    tiers = (
        ("race %d-%d" % (RACE_FRAME_MIN, RACE_FRAME_MAX),
         dict(script=RACE_SCRIPT, frame_min=RACE_FRAME_MIN, frame_max=RACE_FRAME_MAX)),
        ("race", dict(script=RACE_SCRIPT, frame_min=None, frame_max=None)),
        ("every script", dict(script=None, frame_min=None, frame_max=None)),
    )
    found = []
    for label, filt in tiers:
        rows = recs
        if filt["script"] is not None:
            rows = [r for r in rows if r.script == filt["script"]]
        if filt["frame_min"] is not None:
            rows = [r for r in rows if r.frame >= filt["frame_min"]]
        if filt["frame_max"] is not None:
            rows = [r for r in rows if r.frame <= filt["frame_max"]]
        found.append((label, filt, rows))
    for label, filt, rows in found:
        if len(rows) >= MIN_SAMPLE:
            return label, filt, len(rows)
    for label, filt, rows in reversed(found):
        if rows:
            return label, filt, len(rows)
    return "none", dict(script=None, frame_min=None, frame_max=None), 0


"""----------------------
 | price
 | Description: One function's per-call cycles over its chosen record tier: the tier label, record count, min/mean/max cycles.
 | Author: suinevere
 | Dependencies: call_compare.check, choose_tier
 | Globals: N/A
 | Params: ctx -- call_compare.Context; name -- the C function name; work -- repo-relative work directory
 | Returns: dict tier, n, cmin, cmean, cmax
 ----------------------"""
def price(ctx: cc.Context, name: str, work: str) -> dict:
    path = os.path.join(cc.REPO, ctx.paths["rec"], name + ".rec")
    tier, filt, _ = choose_tier(path)
    result = cc.check(name, ctx, work=work, **filt)
    c = result["cycles"]
    cmean = (sum(c) / len(c)) if c else 0.0
    return dict(tier=tier, n=len(c), cmin=min(c) if c else 0, cmean=cmean, cmax=max(c) if c else 0)


"""----------------------
 | price_top_level
 | Description: A recursive routine's price from its shallowest-depth records only (the top-level calls into it from outside the hot set), not a mean over records at every recursion depth -- a deeper record is already inside the top-level call that recursed into it, so averaging all depths together understates the top-level price (game_board_gather: 399 over every depth, 792.7 at the shallowest). Falls back to price() if the script has no records for this routine at all.
 | Author: suinevere
 | Dependencies: call_compare.read_records, call_compare.check, price
 | Globals: N/A
 | Params: ctx -- call_compare.Context; name -- the C function name; script -- the script to take the shallowest depth from
 | Returns: dict tier, n, cmin, cmean, cmax (same shape as price())
 ----------------------"""
def price_top_level(ctx: cc.Context, name: str, script: str) -> dict:
    path = os.path.join(cc.REPO, ctx.paths["rec"], name + ".rec")
    scoped = [r for r in cc.read_records(path) if r.script == script]
    if not scoped:
        return price(ctx, name, "pocketstation/build/call/_project_frame/" + name)
    min_depth = min(r.depth for r in scoped)
    work = "pocketstation/build/call/_project_frame/" + name + "_top_level"
    result = cc.check(name, ctx, work=work, script=script, depth=min_depth)
    c = result["cycles"]
    cmean = (sum(c) / len(c)) if c else 0.0
    return dict(tier="%s top-level (depth=%d)" % (script, min_depth), n=len(c), cmin=min(c) if c else 0,
                cmean=cmean, cmax=max(c) if c else 0)


"""----------------------
 | weighted_price
 | Description: A routine's true frequency-weighted price over every distinct argument tuple the LR attribution pass saw in the real race run (not a mean over a reservoir-sampled subset, which stratifies by path key, not true frequency): for each distinct tuple, an existing record is cloned with those fields overridden at the given argument positions (every other field, including the call's background RAM and frame buffer, stays the template's -- the routine's cycles depend only on its arguments, confirmed by rerunning an existing record's own arguments through a different record's background and matching its real cycles exactly) and run once through vmu_harness --call, batched into one harness invocation; the weighted mean is the true-count-weighted average of those results.
 | Author: suinevere
 | Dependencies: call_compare.Context, call_compare.Setup, call_compare.run_harness, call_compare.read_out
 | Globals: N/A
 | Params: ctx -- call_compare.Context; name -- the C function name; arg_indices -- Record.args positions the tuple's fields go at, in order; tuples -- dict of tuple(values) -> true count, from the LR attribution pass; frames -- race frame count
 | Returns: dict n_distinct, total_calls, matched_calls (status "returned" only), weighted_mean, per_frame
 ----------------------"""
def weighted_price(ctx: cc.Context, name: str, arg_indices: tuple, tuples: dict, frames: int) -> dict:
    path = os.path.join(cc.REPO, ctx.paths["rec"], name + ".rec")
    recs = cc.read_records(path)
    if not recs:
        raise ValueError("no records for %s to use as a synthesis template" % name)
    template = recs[0]
    m = ctx.maps[name]
    work = "pocketstation/build/call/_project_frame/" + name + "_weighted"
    os.makedirs(os.path.join(cc.REPO, work), exist_ok=True)
    for fname in os.listdir(os.path.join(cc.REPO, work)):
        if re.fullmatch(r"\d+\.(state|out|log)|run\.sh", fname):
            os.remove(os.path.join(cc.REPO, work, fname))
    keys = sorted(tuples)
    for k, key in enumerate(keys):
        rec = copy.copy(template)
        args = list(rec.args)
        for idx, value in zip(arg_indices, key):
            a = args[idx]
            v = value & ((1 << (8 * len(a.before))) - 1)
            args[idx] = cc.Arg(a.kind, v.to_bytes(len(a.before), "little"), a.after)
        rec.args = args
        setup = cc.Setup(ctx, m, rec)
        with open(os.path.join(cc.REPO, work, "%d.state" % k), "w", newline="\n") as f:
            f.write(setup.text)
    if keys:
        cc.run_harness(ctx, m.routine, work, len(keys), None)
    weighted_total, matched_calls, total_calls = 0.0, 0, 0
    for k, key in enumerate(keys):
        n = tuples[key]
        total_calls += n
        out = cc.read_out(os.path.join(cc.REPO, work, "%d.out" % k))
        if out["status"] == "returned":
            weighted_total += out["cycles"] * n
            matched_calls += n
    return dict(n_distinct=len(keys), total_calls=total_calls, matched_calls=matched_calls,
                weighted_mean=(weighted_total / matched_calls) if matched_calls else 0.0,
                per_frame=weighted_total / frames)


"""----------------------
 | RoutineRow
 | Description: One hot-set routine's measured and projected numbers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class RoutineRow:
    def __init__(self, name, routine, reachable, tier, n, cmin, cmean, cmax, calls_per_frame, naive_cpf,
                 contribution):
        self.name, self.routine, self.reachable, self.tier = name, routine, reachable, tier
        self.n, self.cmin, self.cmean, self.cmax = n, cmin, cmean, cmax
        self.calls_per_frame, self.naive_cpf, self.contribution = calls_per_frame, naive_cpf, contribution


"""----------------------
 | measure_routines
 | Description: For each hot-set function, whether it has a caller outside the hot set, its per-call cycles from the richest race sample available (game_board_gather's from its top-level records only, price_top_level(), since it recurses; every other routine's from choose_tier()'s richest tier), its calls a race frame (the LR attribution's outside-caller count when attributed, else calls-race.txt's entered count) alongside the naive entered-count for comparison, and its cycles contribution a frame (0 for a member with no outside caller). game_sprite_draw's row uses this generic pricing too, for the cross-check printed alongside its weighted price; project() does not sum this row's contribution into the hot set total.
 | Author: suinevere
 | Dependencies: call_compare.Context, call_compare.check, has_outside_caller, choose_tier, price, price_top_level, race_entered, outside_calls
 | Globals: BOARD_GATHER, RACE_SCRIPT
 | Params: ctx -- call_compare.Context; names -- hot_set()'s result; rev -- reverse_callers()'s result; entered -- race_entered()'s result; attrib_callers -- run_budget_and_attribution()'s result; frames -- race frame count
 | Returns: (list of RoutineRow, hot set assembly cycles a frame, summed over every row including game_sprite_draw's generic price)
 ----------------------"""
def measure_routines(ctx: cc.Context, names: list, rev: dict, entered: dict, attrib_callers: dict, frames: int):
    hot = set(names)
    rows, total = [], 0.0
    for name in names:
        routine = ctx.maps[name].routine
        reachable = has_outside_caller(name, rev, hot)
        if name == BOARD_GATHER:
            p = price_top_level(ctx, name, RACE_SCRIPT)
        else:
            work = "pocketstation/build/call/_project_frame/" + name
            p = price(ctx, name, work)
        corrected = outside_calls(name, attrib_callers, hot)
        naive_cpf = entered.get(name, 0) / frames
        calls_per_frame = (corrected / frames) if corrected is not None else naive_cpf
        contribution = calls_per_frame * p["cmean"] if reachable else 0.0
        total += contribution
        rows.append(RoutineRow(name, routine, reachable, p["tier"], p["n"], p["cmin"], p["cmean"], p["cmax"],
                                calls_per_frame, naive_cpf, contribution))
    return rows, total


"""----------------------
 | price_sprite_draw
 | Description: game_sprite_draw's price a race frame, weighted by true frequency over every argument tuple the race made, bitmap included. Plot_rows's weighted price sits beside it.
 | Author: suinevere
 | Dependencies: call_compare.Context, weighted_price
 | Globals: SPRITE_DRAW, PLOT_ROWS, PLOT_ARG_INDICES, SPRITE_ARG_INDICES
 | Params: ctx -- call_compare.Context; plot_tuples -- run_budget_and_attribution()'s plot_tuples; sprite_tuples -- its sprite_tuples; frames -- race frame count
 | Returns: dict sprite, plot (weighted_price()'s result each), beyond_plot (cycles a frame spent beyond plot_rows), contribution (the sprite's cycles a frame)
 ----------------------"""
def price_sprite_draw(ctx: cc.Context, plot_tuples: dict, sprite_tuples: dict, frames: int) -> dict:
    sprite = weighted_price(ctx, SPRITE_DRAW, SPRITE_ARG_INDICES, sprite_tuples, frames)
    plot = weighted_price(ctx, PLOT_ROWS, PLOT_ARG_INDICES, plot_tuples, frames)
    return dict(sprite=sprite, plot=plot, beyond_plot=sprite["per_frame"] - plot["per_frame"],
                contribution=sprite["per_frame"])


"""----------------------
 | rom32_hot_share
 | Description: rom32's hot-attributable instructions: its whole budget.py bucket times the fraction of its LR-attributed calls whose caller is in the hot set, assuming roughly even cost a call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: ROM32_NAME
 | Params: per_func -- run_budget_and_attribution()'s per_func; attrib_callers -- its attrib_callers; hot -- the hot set, as a set
 | Returns: dict hot_calls, total_calls, fraction, hot_instructions
 ----------------------"""
def rom32_hot_share(per_func: dict, attrib_callers: dict, hot: set) -> dict:
    callers = attrib_callers.get(ROM32_NAME, {})
    total_calls = sum(callers.values())
    hot_calls = sum(n for caller, n in callers.items() if caller in hot)
    fraction = (hot_calls / total_calls) if total_calls else 0.0
    return dict(hot_calls=hot_calls, total_calls=total_calls, fraction=fraction,
                hot_instructions=fraction * per_func.get(ROM32_NAME, 0))


"""----------------------
 | compute_projection
 | Description: The pure arithmetic of the projection under two C-instruction-crediting scenarios: share = hot_c / sum_instr, ratio = hot_asm / (share * mean_c), and scenario 1 projects ratio times a frame's C instructions. Scenario 2 prices only the C instructions beyond scenario 1's hot share at its own ratio2, then scales the whole result like scenario 1 does, since hot_asm is real, already-measured work that must not be priced again as non-hot.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: GATE_FRACTION, FRAME_BUDGET
 | Params: hot_asm -- hot set assembly cycles a frame; hot_c -- hot set C instructions a race run, scenario 1; hot_c_no_rev32 -- scenario 2; sum_instr -- all C instructions a race run; mean_c, p90_c -- the mean and p90 C frame; gate_fraction, frame_budget -- the stop gate's fraction and the frame's cycle budget; frozen1, frozen2 -- the rest of the frame's frozen ratios
 | Returns: dict share, ratio, proj_mean, proj_p90, tripped (scenario 1); share2, ratio2, proj_mean2, proj_p90_2, tripped2 (scenario 2); frozen_mean, frozen_p90, frozen_mean2, frozen_p90_2 (the rest priced at frozen ratios); rest_c, break_even; gate_cycles
 ----------------------"""
def compute_projection(hot_asm: float, hot_c: float, hot_c_no_rev32: float, sum_instr: float, mean_c: float,
                        p90_c: float, gate_fraction: float = GATE_FRACTION, frame_budget: float = FRAME_BUDGET,
                        frozen1: float = FROZEN_RATIO1, frozen2: float = FROZEN_RATIO2) -> dict:
    gate_cycles = gate_fraction * frame_budget
    share = hot_c / sum_instr
    hot_c_mean = share * mean_c
    ratio = hot_asm / hot_c_mean if hot_c_mean else 0.0
    proj_mean = ratio * mean_c
    proj_p90 = ratio * p90_c
    share2 = hot_c_no_rev32 / sum_instr
    hot_c_mean2 = share2 * mean_c
    ratio2 = hot_asm / hot_c_mean2 if hot_c_mean2 else 0.0
    proj_mean2 = hot_asm + ratio2 * (mean_c - hot_c_mean)
    proj_p90_2 = proj_mean2 * p90_c / mean_c if mean_c else 0.0
    rest_c = mean_c - hot_c_mean
    frozen_mean = hot_asm + frozen1 * rest_c
    frozen_mean2 = hot_asm + frozen2 * rest_c
    scale = p90_c / mean_c if mean_c else 0.0
    break_even = (gate_cycles - hot_asm) / rest_c if rest_c else 0.0
    return dict(share=share, ratio=ratio, proj_mean=proj_mean, proj_p90=proj_p90, tripped=proj_mean > gate_cycles,
                share2=share2, ratio2=ratio2, proj_mean2=proj_mean2, proj_p90_2=proj_p90_2,
                tripped2=proj_mean2 > gate_cycles, frozen_mean=frozen_mean, frozen_p90=frozen_mean * scale,
                frozen_mean2=frozen_mean2, frozen_p90_2=frozen_mean2 * scale, rest_c=rest_c, break_even=break_even,
                gate_cycles=gate_cycles)


"""----------------------
 | project
 | Description: Projects the whole race frame's assembly cycles under both rev32-credit scenarios: the hot set's own cycles (game_sprite_draw weighted over its true argument tuples, every other counted routine priced directly, calls a frame corrected for recursion by the LR attribution pass) plus the rest of the frame scaled by the hot set's cycles-per-C-instruction ratio, at the mean and p90 C frame from budget.py.
 | Author: suinevere
 | Dependencies: measure_routines, price_sprite_draw, rom32_hot_share, compute_projection, run_budget_and_attribution, reverse_callers
 | Globals: SPRITE_DRAW, REV32_NAME, ROM32_NAME, ATTRIB_NAMES, RACE_SCRIPT
 | Params: ctx -- call_compare.Context; funcs -- vmu_callgraph.discover()'s result; names -- hot_set()'s result; docker -- passed to run_budget_and_attribution
 | Returns: dict with rows, sprite_decomp, rom32, hot_asm, frames and compute_projection()'s keys
 ----------------------"""
def project(ctx: cc.Context, funcs: dict, names: list, docker=None) -> dict:
    entered = race_entered(RACE_SCRIPT)
    per_func_names = tuple(names) + (REV32_NAME, ROM32_NAME)
    (frames, sum_instr, mean_c, p90_c, per_func, attrib_totals, attrib_callers, plot_tuples,
     sprite_tuples) = run_budget_and_attribution(RACE_SCRIPT, per_func_names, ATTRIB_NAMES + (ROM32_NAME,), docker)
    rev = reverse_callers(funcs)
    hot = set(names)
    rows, generic_total = measure_routines(ctx, names, rev, entered, attrib_callers, frames)
    sprite_row = next(r for r in rows if r.name == SPRITE_DRAW)
    sprite_decomp = price_sprite_draw(ctx, plot_tuples, sprite_tuples, frames)
    hot_asm = generic_total - sprite_row.contribution + sprite_decomp["contribution"]
    rom32 = rom32_hot_share(per_func, attrib_callers, hot)
    hot_c = sum(per_func.get(n, 0) for n in names) + per_func.get(REV32_NAME, 0) + rom32["hot_instructions"]
    hot_c_no_rev32 = hot_c - per_func.get(REV32_NAME, 0)
    proj = compute_projection(hot_asm, hot_c, hot_c_no_rev32, sum_instr, mean_c, p90_c)
    out = dict(rows=rows, sprite_decomp=sprite_decomp, rom32=rom32, hot_asm=hot_asm, hot_c=hot_c,
               hot_c_no_rev32=hot_c_no_rev32, mean_c=mean_c, p90_c=p90_c, sum_instr=sum_instr, frames=frames)
    out.update(proj)
    return out


"""----------------------
 | main
 | Description: CLI. project_frame.py [--local|--docker] prints each hot-set routine's reachability, record tier, per-call cycles and corrected/naive calls a race frame, game_sprite_draw's and plot_rows's weighted prices, rom32's hot share, the hot set's assembly cycles a frame, both scenarios' C-to-assembly ratio, and the projected mean and p90 race frame with their percentage of 28,700 and the gate's verdict, for each scenario, also with the rest of the frame at Task 7's frozen ratios, and the break-even ratio.
 | Author: suinevere
 | Dependencies: call_compare.Context, hot_set, project, vmu_callgraph
 | Globals: FRAME_BUDGET
 | Params: argv -- command-line arguments
 | Returns: 0
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    ap = argparse.ArgumentParser(prog="project_frame.py")
    ap.add_argument("--docker", dest="docker", action="store_true", default=None)
    ap.add_argument("--local", dest="docker", action="store_false")
    a = ap.parse_args(argv)
    ctx = cc.Context()
    funcs = vc.discover()
    manifest_rows = vc.parse_manifest(os.path.join(cc.REPO, ctx.paths["manifest"]))
    names = hot_set(funcs, manifest_rows)
    result = project(ctx, funcs, names, a.docker)
    for row in result["rows"]:
        tag = "generic-cross-check" if row.name == SPRITE_DRAW else ("reachable" if row.reachable else "dead")
        print("routine %s %s %s tier=%s n=%d cycles %d/%d/%d calls/frame %.3f (naive %.3f) contribution %.1f"
              % (row.name, row.routine, tag, row.tier, row.n, row.cmin, row.cmean, row.cmax,
                 row.calls_per_frame, row.naive_cpf, row.contribution))
    d = result["sprite_decomp"]
    for label, w in (("game_sprite_draw", d["sprite"]), ("plot_rows", d["plot"])):
        print("%s true-frequency weighted: %d distinct tuples, %d/%d calls matched (status returned), "
              "weighted mean %.1f cycles/call, %.1f cycles/frame"
              % (label, w["n_distinct"], w["matched_calls"], w["total_calls"], w["weighted_mean"], w["per_frame"]))
    print("game_sprite_draw beyond plot_rows: %.1f cycles/frame" % d["beyond_plot"])
    r32 = result["rom32"]
    print("rom32 hot share: %d of %d calls (%.1f%%), %.1f of its C instructions credited as hot"
          % (r32["hot_calls"], r32["total_calls"], 100.0 * r32["fraction"], r32["hot_instructions"]))
    print("hot set assembly cycles/frame %.1f" % result["hot_asm"])
    print("hot set C instructions (scenario 1, with rev32 credit): %.1f of %.1f (share %.4f)"
          % (result["hot_c"], result["sum_instr"], result["share"]))
    print("hot set C instructions (scenario 2, no rev32 credit): %.1f of %.1f (share %.4f)"
          % (result["hot_c_no_rev32"], result["sum_instr"], result["share2"]))
    print("race frame C instructions mean %.1f p90 %.1f" % (result["mean_c"], result["p90_c"]))
    print("C-to-assembly ratio (scenario 1) %.4f cycles per hot C instruction" % result["ratio"])
    print("projected mean (scenario 1) %d cycles (%.1f%% of %d)"
          % (round(result["proj_mean"]), 100.0 * result["proj_mean"] / FRAME_BUDGET, FRAME_BUDGET))
    print("projected p90 (scenario 1) %d cycles (%.1f%% of %d)"
          % (round(result["proj_p90"]), 100.0 * result["proj_p90"] / FRAME_BUDGET, FRAME_BUDGET))
    print("gate (scenario 1) %s (mean %.1f, limit %.1f)" % ("TRIPPED" if result["tripped"] else "CLEARED",
                                                              result["proj_mean"], result["gate_cycles"]))
    print("C-to-assembly ratio (scenario 2, no rev32 credit) %.4f cycles per hot C instruction" % result["ratio2"])
    print("projected mean (scenario 2) %d cycles (%.1f%% of %d)"
          % (round(result["proj_mean2"]), 100.0 * result["proj_mean2"] / FRAME_BUDGET, FRAME_BUDGET))
    print("projected p90 (scenario 2) %d cycles (%.1f%% of %d)"
          % (round(result["proj_p90_2"]), 100.0 * result["proj_p90_2"] / FRAME_BUDGET, FRAME_BUDGET))
    print("gate (scenario 2) %s (mean %.1f, limit %.1f)" % ("TRIPPED" if result["tripped2"] else "CLEARED",
                                                              result["proj_mean2"], result["gate_cycles"]))
    for scenario, ratio, mean, p90 in ((1, FROZEN_RATIO1, "frozen_mean", "frozen_p90"),
                                       (2, FROZEN_RATIO2, "frozen_mean2", "frozen_p90_2")):
        print("frozen-ratio mean (scenario %d, rest at %.4f) %d cycles (%.1f%% of %d), p90 %d cycles, gate %s"
              % (scenario, ratio, round(result[mean]), 100.0 * result[mean] / FRAME_BUDGET, FRAME_BUDGET,
                 round(result[p90]), "TRIPPED" if result[mean] > result["gate_cycles"] else "CLEARED"))
    print("break-even rest-of-frame ratio %.4f cycles per C instruction (%.1f rest C instructions a frame)"
          % (result["break_even"], result["rest_c"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
