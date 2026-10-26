"""----------------------
 | test_vmu_game.py
 | Description: Proves the narrow C's call graph (every function in src/vmu/*.c plus game.h's static inline accessors, vmu_host.c and vmu_hw.h excluded, each with a level, leaves at 0, every callee's level below its caller's except within a shared recursion group), that the manifest matches it column for column, that the recording build leaves the game unchanged while recording every function the scripts reach, with synthetic records or a reason for the rest, that the harness's --call mode with call_compare.py checks an assembly routine against its records, catching a stray byte, a wrong hardware call and a register relied on across a call, that the GAME keeps the pages of RES1506.BIN its recorded frames read random-access or unpacked plus the densest picture pages, with every recorded vmu_rom8 read inside a listed table, that R2's pet save carries every table and the rest reads back from it through rt_rom8, that the GAME leaves without a save that carries them, that the GAME and the save fit one VMU with room for the C size of every routine still to port plus four blocks, that the real GAME fits flash bank 0 with 12 KB free, that the sprites stored ready to draw are those a script draws, that every unstored save-page sprite is classified by whether play can reach it, and that they match the C's unpacking and the assembly unpacker, the rest unpacking alike, that the dead hot-set routines are gone, that every ported routine of a level passes all its records, those waiting on a handler not yet ported named in LEVEL_DEFERRED, that every branch outcome of a level no script takes is taken by a passing synthetic record or acknowledged in untaken.txt, and that a race frame projects under the speed gate.
 | Author: suinevere
 | Dependencies: pocketstation/tools/vmu_callgraph.py, pocketstation/tools/call_compare.py, pocketstation/tools/gen_tables.py (tables.txt), pocketstation/src/vmu_game/{manifest.txt,Makefile,game.s,sprite.s}, pocketstation/tests/asm/{stray,wrong_hw,poison}.s, pocketstation/tools/vmu_flash.py, make vmu-record, test_game.py, lom-pocket image
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import glob
import hashlib
import os
import struct
import re
import shutil
import subprocess
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
sys.path.insert(0, HERE)

import vmu_callgraph as vc  # noqa: E402
from test_game import CARD, REFERENCE, RES, SCRIPTS, built, entry_state  # noqa: E402,F401

MANIFEST = os.path.join(REPO, "pocketstation", "src", "vmu_game", "manifest.txt")

# The R3b-3 plan's "Facts" line estimates 208 functions (155 game.h prototypes
# plus the rest static). Parsing src/vmu/*.c (vmu_host.c excluded) finds 221:
# game.h actually declares 164 prototypes, not 155, plus 57 static functions
# (164 + 57 = 221). game.h also defines 9 static inline accessors (rom8,
# rom16, rom32, ref8, rev32, fb_row, set_fb_row, fb_run, set_fb_run) that are
# part of the same call graph but inline at every call site rather than
# becoming a routine; 221 + 9 = 230. See the Task 1 report for the breakdown.
C_FUNCTION_COUNT = 221
INLINE_NAMES = {"rom8", "rom16", "rom32", "ref8", "rev32", "fb_row", "set_fb_row", "fb_run", "set_fb_run"}
FUNCTION_COUNT = C_FUNCTION_COUNT + len(INLINE_NAMES)


def _recursion_groups(funcs):
    comp = vc.sccs(funcs)
    members = {}
    for name, cid in comp.items():
        members.setdefault(cid, []).append(name)
    return {cid: names for cid, names in members.items() if len(names) > 1}


def test_callgraph_covers_every_function():
    funcs = vc.discover()
    assert len(funcs) == FUNCTION_COUNT
    rows = vc.levels(funcs)
    assert len(rows) == FUNCTION_COUNT
    assert {name for _, name, _ in rows} == set(funcs)
    level_of = {name: level for level, name, _ in rows}
    comp = vc.sccs(funcs)
    groups = _recursion_groups(funcs)
    assert groups, "expected at least one recursion group (game_board_gather/board_branch)"
    in_group = {n for names in groups.values() for n in names}
    leaves = [name for level, name, _ in rows if level == 0]
    assert leaves
    assert all(not vc.callees(name, funcs) for name in leaves if name not in in_group)
    for name in funcs:
        for callee in vc.callees(name, funcs):
            if comp[callee] == comp[name]:
                assert level_of[callee] == level_of[name]
            else:
                assert level_of[callee] < level_of[name]


def test_recursion_group_shares_one_level():
    funcs = vc.discover()
    level_of = {name: level for level, name, _ in vc.levels(funcs)}
    groups = _recursion_groups(funcs)
    for names in groups.values():
        assert len({level_of[n] for n in names}) == 1


def test_levels_cli_prints_level_name_file(capsys):
    assert vc.main(["levels"]) == 0
    lines = capsys.readouterr().out.splitlines()
    assert len(lines) == FUNCTION_COUNT
    files = set()
    for line in lines:
        level, name, file = line.split()
        int(level)
        files.add(file)
    assert all(f.endswith(".c") or f == "game.h" for f in files)
    assert "game.h" in files
    assert "vmu_host.c" not in files
    assert "vmu_hw.h" not in files


def test_manifest_matches_the_call_graph():
    assert vc.check(MANIFEST) == 0


def test_manifest_has_eight_columns_per_row():
    rows = vc.parse_manifest(MANIFEST)
    assert len(rows) == FUNCTION_COUNT
    for cols in rows.values():
        assert len(cols) == vc.MANIFEST_COLUMNS


def test_manifest_rows_have_a_status_and_a_destroys_list():
    rows = vc.parse_manifest(MANIFEST)
    regs = {"ACC", "B", "C", "TRH", "TRL", "PSW", "R0", "R1", "R2", "R3", "VRMAD", "FPR"}
    for name, cols in rows.items():
        assert cols[-1] in ("todo", "synthetic", "verified") or re.fullmatch(r"reading:[\w-]+", cols[-1]), cols
        if name in INLINE_NAMES:
            continue
        if cols[3].startswith("inlined:"):
            assert cols[-2] == "-", cols
            continue
        if cols[-1] in ("verified", "synthetic"):
            assert cols[-2] == "-" or set(cols[-2].split(",")) <= regs, cols
        else:
            assert cols[-2] == vc.DESTROYS_DEFAULT, cols


def test_manifest_routine_names_have_no_collisions():
    rows = vc.parse_manifest(MANIFEST)
    routines = [cols[3] for cols in rows.values() if cols[3] != vc.INLINE_ROUTINE]
    assert len(routines) == len(set(routines))


def test_game_h_inline_accessors_are_marked_inlined():
    rows = vc.parse_manifest(MANIFEST)
    assert INLINE_NAMES <= set(rows)
    for name in INLINE_NAMES:
        cols = rows[name]
        assert cols[1] == "game.h"
        assert cols[3] == vc.INLINE_ROUTINE
        assert cols[-2] == "-"
        assert cols[-1] == "todo"


def test_vmu_hw_header_is_excluded():
    funcs = vc.discover()
    assert all(f.file != "vmu_hw.h" for f in funcs.values())


def test_check_reports_a_missing_function(tmp_path):
    lines = [line for line in open(MANIFEST).read().splitlines() if line and not line.startswith("#")]
    bad = tmp_path / "manifest.txt"
    bad.write_text("\n".join(lines[1:]) + "\n")
    assert vc.main(["check", str(bad)]) == 1


def test_check_reports_an_invented_function(tmp_path):
    lines = [line for line in open(MANIFEST).read().splitlines() if line and not line.startswith("#")]
    bad = tmp_path / "manifest.txt"
    bad.write_text("\n".join(lines + ["not_a_real_function fake.c 0 g_fake - - - todo"]) + "\n")
    assert vc.main(["check", str(bad)]) == 1


def test_check_rejects_the_wrong_column_count(tmp_path):
    bad = tmp_path / "manifest.txt"
    bad.write_text("game_entry app.c 12 g_entry - - ACC,B,C,TRH,TRL,PSW\n")
    assert vc.main(["check", str(bad)]) == 1


def test_vmu_host_is_excluded():
    funcs = vc.discover()
    assert all(f.file != "vmu_host.c" for f in funcs.values())


# The recording build: the seven scripts through vmu_runner-record, then
# record_synthetic for what they never call, into build/record/rec. The run
# is skipped while the binaries, card and scripts hash to the stamp left by
# the last one.
needs_game = pytest.mark.skipif(
    shutil.which("docker") is None or not os.path.exists(RES) or not glob.glob(os.path.join(REFERENCE, "*.pktr")),
    reason="needs docker, RES1506.BIN and the reference traces")

REC_DIR = "pocketstation/build/record/rec"
REC_PATH = os.path.join(REPO, REC_DIR)
PROBES = os.path.join(REPO, "pocketstation", "build", "record", "probes.txt")
RAM_H = os.path.join(REPO, "pocketstation", "build", "vmu", "gen", "rrl_ram.h")
RECORD_RUNNER = "pocketstation/build/vmu_runner-record"
SYNTHETIC = "pocketstation/build/record_synthetic"


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))


def _stamp():
    h = hashlib.sha256()
    for path in [RECORD_RUNNER, SYNTHETIC, CARD] + ["pocketstation/tests/scripts/%s.pks" % s for s in SCRIPTS]:
        with open(os.path.join(REPO, path), "rb") as f:
            h.update(f.read())
    return h.hexdigest()


@pytest.fixture(scope="module")
def recordings(built):
    r = docker("make", "-C", "pocketstation", "vmu-host", "vmu-record")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    entries = {s: entry_state(s) for s in SCRIPTS}
    stamp, stamp_path = _stamp(), os.path.join(REC_PATH, "stamp")
    if os.path.exists(stamp_path) and open(stamp_path).read() == stamp:
        return REC_PATH
    shutil.rmtree(REC_PATH, ignore_errors=True)
    runs = ["%s --card %s --script pocketstation/tests/scripts/%s.pks --entry %s --record %s"
            % (RECORD_RUNNER, CARD, s, entries[s], REC_DIR) for s in SCRIPTS]
    runs.append("%s --card %s --entry %s --dir %s" % (SYNTHETIC, CARD, entries["boot"], REC_DIR))
    r = docker("sh", "-c", "set -e; " + "; ".join(runs))
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    with open(stamp_path, "w") as f:
        f.write(stamp)
    return REC_PATH


def _cc():
    import call_compare
    return call_compare


def _non_inlined():
    return {n for n, f in vc.discover().items() if not f.inline}


def _records(rec_dir, name):
    path = os.path.join(rec_dir, name + ".rec")
    return _cc().read_records(path) if os.path.exists(path) else []


@needs_game
@pytest.mark.parametrize("script", ["boot", "race"])
def test_recording_does_not_change_the_game(recordings, tmp_path, script):
    pks = "pocketstation/tests/scripts/%s.pks" % script
    plain, recorded = "pocketstation/build/test/plain-%s.state" % script, "pocketstation/build/test/record-%s.state" % script
    scratch = "pocketstation/build/record/scratch-%s" % script
    shutil.rmtree(os.path.join(REPO, scratch), ignore_errors=True)
    r = docker("sh", "-c", "pocketstation/build/vmu_runner --card %s --script %s --entry %s --state %s && "
               "%s --card %s --script %s --entry %s --state %s --record %s"
               % (CARD, pks, entry_state(script), plain, RECORD_RUNNER, CARD, pks, entry_state(script), recorded, scratch))
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-2000:]
    assert glob.glob(os.path.join(REPO, scratch, "*.rec"))
    with open(os.path.join(REPO, plain), "rb") as a, open(os.path.join(REPO, recorded), "rb") as b:
        want, got = a.read(), b.read()
    assert want and got == want
    shutil.rmtree(os.path.join(REPO, scratch), ignore_errors=True)


@needs_game
def test_every_reached_function_has_records(recordings):
    counts = _cc().call_counts(recordings)
    rows = vc.parse_manifest(MANIFEST)
    names = _non_inlined()
    assert set(counts) <= names
    assert len(counts) > 150
    for name, (entered, returned) in counts.items():
        assert entered >= returned
        if returned:
            assert any(not rec.synthetic for rec in _records(recordings, name)), name
        else:
            assert rows[name][-1] == "reading:never-returns", name


@needs_game
def test_records_round_trip(recordings):
    cc = _cc()
    probes = {}
    with open(PROBES) as f:
        for line in f:
            probes[line.split()[0]] = probes.get(line.split()[0], 0) + 1
    files = glob.glob(os.path.join(recordings, "*.rec"))
    assert files
    total = 0
    for path in files:
        name = os.path.basename(path)[:-4]
        with open(path, "rb") as f:
            raw = f.read()
        recs = cc.read_records(path)
        assert b"".join(cc.encode(rec) for rec in recs) == raw
        for rec in recs:
            assert rec.name == name and rec.nbits == 2 * probes.get(name, 0)
            assert len(rec.ram_before) == len(rec.ram_after) == cc.RAM_BYTES
            assert len(rec.fb_before) == len(rec.fb_after) == cc.FB_BYTES
            assert all(0 <= c.id < len(cc.HW_NAMES) for c in rec.hw)
            assert rec.synthetic == (rec.script == "synthetic")
        total += len(recs)
    seed = int(re.search(r"#define RAM_SEED (0x[0-9A-F]+)u", open(RAM_H).read()).group(1), 16)
    for rec in _records(recordings, "game_random"):
        before = rec.ram_before[seed] | rec.ram_before[seed + 1] << 8
        after = rec.ram_after[seed] | rec.ram_after[seed + 1] << 8
        assert after == ((before + 1) * 0x3D09) & 0xFFFF
        assert int.from_bytes(rec.result, "little") == (rec.args[0].value * after) >> 16
    waits = _records(recordings, "game_wait_ticks")
    assert waits and all(rec.hw[0].name == "vmu_present" and rec.hw[0].depth == rec.depth for rec in waits)
    idled = [rec for rec in waits if len(rec.hw) > 2 and rec.hw[1].name == "vmu_idle"]
    assert idled and all(rec.hw[2].depth > rec.depth for rec in idled)
    assert total > 1000


def _probes():
    out = {}
    with open(PROBES) as f:
        for line in f:
            name, k, line_no, kind = line.split()
            out.setdefault(name, []).append((int(k), int(line_no), kind))
    return out


@needs_game
def test_short_circuit_operands_and_accessor_branches_have_their_own_bits(recordings):
    probes = _probes()
    kinds = [kind for ps in probes.values() for _, _, kind in ps]
    assert kinds.count("&&") + kinds.count("||") > 100
    scene, roll = [k for k, line_no, kind in probes["game_reel_pair_step"] if line_no == 785 and kind == "&&"]
    keys = {frozenset(rec.key_bits) for rec in _records(recordings, "game_reel_pair_step")}
    assert any(2 * scene in key and 2 * roll in key for key in keys)
    assert any(2 * scene in key and 2 * roll + 1 in key for key in keys)
    assert [kind for _, line_no, kind in probes["bag_reveal_scroll"] if line_no == 178].count("||") == 2
    for name, accessor in (("game_add_capped", "ref8:ternary"), ("game_reel_window_draw", "set_fb_run:if"),
                           ("game_reel_window_draw", "fb_run:ternary")):
        ks = [k for k, _, kind in probes[name] if kind == accessor]
        assert ks, (name, accessor)
        assert any(rec.key_bits & {2 * ks[0], 2 * ks[0] + 1} for rec in _records(recordings, name)), (name, accessor)


@needs_game
def test_unreached_functions_have_synthetic_records_or_a_reason(recordings):
    counts = _cc().call_counts(recordings)
    rows = vc.parse_manifest(MANIFEST)
    unreached = sorted(_non_inlined() - set(counts))
    assert unreached
    for name in unreached:
        status = rows[name][-1]
        synthetic = [rec for rec in _records(recordings, name) if rec.synthetic]
        if status.startswith("reading:"):
            assert re.fullmatch(r"reading:[\w-]+", status), name
        else:
            assert status == "synthetic" and synthetic, name
    for name, cols in rows.items():
        if cols[-1] == "synthetic":
            assert name in unreached and any(rec.synthetic for rec in _records(recordings, name)), name


# The --call mode: the harness runs one assembly routine from a recorded
# state and call_compare.py checks every record of a function against it.
CALL_BUILD = "pocketstation/build/vmu_game"
CALL_COMPARE = os.path.join(REPO, "pocketstation", "tools", "call_compare.py")
HARNESS = "pocketstation/build/vmu_harness"


@pytest.fixture(scope="module")
def call_tools(recordings):
    r = docker("make", "-C", "pocketstation/harness")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    r = docker("make", "-C", "pocketstation/src/vmu_game", "call", "fixtures")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    return recordings


def compare(*args):
    return subprocess.run([sys.executable, CALL_COMPARE, *args], capture_output=True, text=True, cwd=REPO)


def fixture_image(name):
    return ["--image", "%s/test/%s.vms" % (CALL_BUILD, name), "--map", "%s/test/%s.map" % (CALL_BUILD, name),
            "--work", "pocketstation/build/call/test-%s" % name]


@needs_game
def test_call_mode_runs_a_routine_and_reports_its_state(call_tools):
    cc = _cc()
    recs = _records(call_tools, "game_random")
    assert recs
    r = compare("game_random")
    assert r.returncode == 0, r.stdout + r.stderr
    assert r.stdout.splitlines()[0] == "ok %d" % len(recs), r.stdout
    assert "untested" not in r.stdout
    seed = cc.hw_placement(os.path.join(REPO, CALL_BUILD, "hw.json"))["seed"]
    for k, rec in enumerate(recs):
        out = cc.read_out(os.path.join(REPO, "pocketstation", "build", "call", "game_random", "%d.out" % k))
        assert out["status"] == "returned"
        assert 0 < out["cycles"] < 200
        assert out["stack_depth"] == 4
        assert out["hw"] == []
        assert out["bank1"][seed.addr:seed.addr + 2] == rec.ram_after[seed.offset:seed.offset + 2]
        assert bytes([out["reg"]["TRL"], out["reg"]["TRH"]]) == rec.result
    row = vc.parse_manifest(MANIFEST)["game_random"]
    assert row[5] == "TR" and row[6] == "ACC,B,C,PSW" and row[7] == "verified"


@needs_game
def test_cycles_report_can_filter_by_script_and_frame(call_tools):
    recs = _records(call_tools, "game_nibble")
    assert recs
    r = compare("game_nibble", "--cycles")
    assert r.returncode == 0, r.stdout + r.stderr
    assert r.stdout.splitlines()[0] == "ok %d" % len(recs), r.stdout
    assert re.search(r"^cycles mean \d+ min \d+ max \d+$", r.stdout, re.M), r.stdout
    race = [rec for rec in recs if rec.script == "race"]
    assert race and len(race) < len(recs)
    r = compare("game_nibble", "--cycles", "--script", "race")
    assert r.returncode == 0, r.stdout + r.stderr
    assert r.stdout.splitlines()[0] == "ok %d" % len(race), r.stdout
    narrow = [rec for rec in race if rec.frame <= 1000]
    assert narrow and len(narrow) < len(race)
    r = compare("game_nibble", "--cycles", "--script", "race", "--frame-max", "1000")
    assert r.returncode == 0, r.stdout + r.stderr
    assert r.stdout.splitlines()[0] == "ok %d" % len(narrow), r.stdout
    assert re.search(r"^cycles mean \d+ min \d+ max \d+$", r.stdout, re.M), r.stdout


@needs_game
def test_cycles_report_can_filter_by_depth(call_tools):
    recs = _records(call_tools, "game_board_gather")
    race = [rec for rec in recs if rec.script == "race"]
    assert race
    min_depth = min(rec.depth for rec in race)
    top_level = [rec for rec in race if rec.depth == min_depth]
    nested = [rec for rec in race if rec.depth != min_depth]
    assert top_level and nested
    r = compare("game_board_gather", "--cycles", "--script", "race", "--depth", str(min_depth))
    assert r.returncode == 0, r.stdout + r.stderr
    assert r.stdout.splitlines()[0] == "ok %d" % len(top_level), r.stdout


@needs_game
def test_compare_catches_a_stray_byte(call_tools):
    r = compare("game_random", *fixture_image("stray"))
    assert r.returncode == 1, r.stdout + r.stderr
    assert re.search(r"^call 0: field points\b", r.stdout, re.M), r.stdout
    assert "ok" not in r.stdout.split()


@needs_game
def test_compare_catches_a_wrong_hardware_call(call_tools):
    r = compare("game_sound_on", *fixture_image("wrong_hw"))
    assert r.returncode == 0, r.stdout + r.stderr
    assert r.stdout.startswith("ok %d" % len(_records(call_tools, "game_sound_on")))
    r = compare("game_sound_off", *fixture_image("wrong_hw"))
    assert r.returncode == 1, r.stdout + r.stderr
    assert re.search(r"^call 0: hw call 0\b.*vmu_buzz", r.stdout, re.M), r.stdout


def _run_state(tmp_name, image, lines, routine):
    rel = "pocketstation/build/call/poison-%s" % tmp_name
    os.makedirs(os.path.join(REPO, rel), exist_ok=True)
    with open(os.path.join(REPO, rel, "in.state"), "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    out = os.path.join(REPO, rel, "out")
    if os.path.exists(out):
        os.remove(out)
    r = docker(HARNESS, "--flash", "%s/%s.vms" % (CALL_BUILD, image), "--map", "%s/%s.map" % (CALL_BUILD, image),
               "--hw", "%s/hw.json" % CALL_BUILD, "--call", routine, "--state", rel + "/in.state", "--out", rel + "/out")
    assert r.returncode == 0, r.stdout + r.stderr
    return _cc().read_out(out)


@needs_game
def test_call_mode_poisons_declared_registers(call_tools):
    seed = _cc().hw_placement(os.path.join(REPO, CALL_BUILD, "hw.json"))["seed"]
    ram = bytearray(0x400)
    ram[seed.offset:seed.offset + 2] = (0x1234).to_bytes(2, "little")
    base = ["fill 5A", "ram " + ram.hex(), "fb " + "00" * 192, "reg ACC 00", "reg TRL 64", "reg TRH 00"]
    s = ((0x1234 + 1) * 0x3D09) & 0xFFFF
    want = (100 * s) >> 16
    out = _run_state("top", "callimg", base + ["poison g_random ACC,B,C,PSW"], "g_random")
    assert out["status"] == "returned"
    assert (out["reg"]["ACC"], out["reg"]["B"], out["reg"]["C"]) == (0xA5, 0xA5, 0xA5)
    assert out["reg"]["PSW"] & 0xE5 == 0xA5 and out["reg"]["PSW"] & 0x1A == 0x02
    assert out["reg"]["TRH"] << 8 | out["reg"]["TRL"] == want
    assert out["bank1"][seed.addr] | out["bank1"][seed.addr + 1] << 8 == s
    out = _run_state("game", "test/poison", base + ["poison g_random ACC,B,C,PSW", "poison p_after_game -"], "p_after_game")
    assert out["status"] == "returned"
    assert out["reg"]["C"] == 0xA5
    assert out["reg"]["TRH"] << 8 | out["reg"]["TRL"] == want
    out = _run_state("rt", "test/poison", base + ["poison p_after_rt -", "ret 12"], "p_after_rt")
    assert out["status"] == "returned"
    assert out["reg"]["TRL"] == 0x12 and out["reg"]["TRH"] == 0xA5
    assert out["hw"] == [("vmu_buttons", "rt_buttons", {"ACC": 0x00, "B": 0x33, "C": 0x44, "TRH": 0x00, "TRL": 0x64})]


def test_struct_results_compare_by_field_not_padding():
    cc = _cc()
    layouts = cc.struct_layouts(os.path.join(REPO, "pocketstation", "src", "vmu", "game.h"))
    size, fields = layouts["SpriteInfo"]
    assert size == 12 and [(n, at) for n, at, _ in fields] == [("x", 0), ("y", 2), ("width", 4), ("height", 5), ("flags", 6), ("bitmap", 8)]
    assert layouts["FrameInfo"][0] == 16 and layouts["FrameInfo"][1][2] == ("bitmap", 4, 4)

    class Stub:
        def __init__(self, **kw):
            self.__dict__.update(kw)

    want = bytes(range(1, 13))
    slot = cc.Slot("result", 0x1DB, 12, False, layouts["SpriteInfo"])
    m, setup, rec = Stub(result="mem:S"), Stub(result_slot=slot), Stub(result=want)
    wram = bytearray(0x200)
    wram[0x1DB:0x1E7] = want
    wram[0x1DB + 7] = 0xEE
    assert cc.result_difference(m, rec, setup, {"wram": bytes(wram)}) is None
    wram[0x1DB + 5] ^= 1
    assert cc.result_difference(m, rec, setup, {"wram": bytes(wram)}).startswith("result field height")


def test_struct_pointer_arguments_compare_by_field_not_padding():
    cc = _cc()
    layout = cc.struct_layouts(os.path.join(REPO, "pocketstation", "src", "vmu", "game.h"))["SpriteInfo"]

    class Stub:
        def __init__(self, **kw):
            self.__dict__.update(kw)

    before, after = bytes(12), bytes(range(1, 13))
    ctx = Stub(block=0x1DB, block_size=13, fields={}, maps={}, labels={}, owner={})
    ctx.name_of = lambda space, addr: "unused " + space
    m = Stub(args=[("s", "mem:S")], arg_layouts=[layout], result=None, destroys=[])
    rec = Stub(args=[cc.Arg(1, before, after)], result=b"", ram_before=bytes(0x400), ram_after=bytes(0x400),
               fb_before=bytes(192), fb_after=bytes(192), hw=[])
    setup = cc.Setup(ctx, m, rec)
    assert ("wram", 0x1DB + 7) in setup.free
    assert [setup.want["wram"][0x1DB + k] for k in (0, 6, 8, 11)] == [1, 7, 9, 12]
    regs = {r: 0xA5 for r in ("ACC", "B", "C", "TRL", "TRH", "VRMAD1", "VRMAD2", "FPR")}
    regs["PSW"] = 0xA7
    out = {"status": "returned", "reg": dict(regs), "entry": dict(regs), "hw": [], "bank1": bytes(setup.want["bank1"]),
           "xram": bytes(setup.want["xram"])}
    wram = bytearray(setup.want["wram"])
    wram[0x1DB + 7] = 0xEE
    out["wram"] = bytes(wram)
    assert cc.first_difference(ctx, m, rec, setup, out) is None
    wram[0x1DB + 4] ^= 1
    out["wram"] = bytes(wram)
    assert cc.first_difference(ctx, m, rec, setup, out).startswith("field arg s")
    out["wram"] = bytes(setup.want["wram"])
    out["reg"]["PSW"] = 0xA5
    assert cc.first_difference(ctx, m, rec, setup, out).startswith("register PSW bank bits")


@needs_game
def test_compare_poisons_registers_at_entry(call_tools):
    r = compare("game_random", *fixture_image("entry_cy"))
    assert r.returncode == 1, r.stdout + r.stderr
    assert re.search(r"^call 0: (result|field seed)\b", r.stdout, re.M), r.stdout


@needs_game
def test_compare_catches_an_undeclared_register(call_tools):
    r = compare("game_random", *fixture_image("undeclared"))
    assert r.returncode == 1, r.stdout + r.stderr
    assert re.search(r"^call 0: register VRMAD1 \(not in DESTROYS", r.stdout, re.M), r.stdout
    r = compare("game_sound_on", *fixture_image("undeclared"))
    assert r.returncode == 1, r.stdout + r.stderr
    assert re.search(r"^call 0: register PSW bank bits", r.stdout, re.M), r.stdout


# Task 4: the tables built into the image from RES1506.BIN at build time, and
# the GAME skeleton laid out around the runtime; Task 7b keeps only the span of
# the file that holds every table the C reads, listed in tables.txt.
needs_res_docker = pytest.mark.skipif(
    shutil.which("docker") is None or not os.path.exists(RES),
    reason="needs docker and RES1506.BIN")

TABLE_LISTING = os.path.join(REPO, CALL_BUILD, "tables.txt")
FLASH_FREE_MIN = 12 * 1024


@pytest.fixture(scope="module")
def table_fixtures():
    r = docker("make", "-C", "pocketstation/harness")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    r = docker("make", "-C", "pocketstation/src/vmu_game", "call", "game", "fixtures")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    return None


def _table_listing():
    kept, tables = [], []
    with open(TABLE_LISTING) as f:
        for line in f:
            parts = line.split()
            if parts and parts[0] == "kept":
                kept.append((int(parts[1], 16), int(parts[2], 16)))
            elif parts and parts[0] == "table":
                tables.append((int(parts[1], 16), int(parts[2], 16), " ".join(parts[3:])))
    return kept, tables


def _recorded_rom_reads(rec_dir):
    paths = sorted(glob.glob(os.path.join(rec_dir, "rom-*.txt")))
    assert {os.path.basename(p) for p in paths} >= {"rom-%s.txt" % s for s in SCRIPTS} | {"rom-synthetic.txt"}, paths
    runs = []
    for path in paths:
        with open(path) as f:
            runs += [(int(a, 16), int(b, 16)) for a, b in (line.split() for line in f if line.strip())]
    return runs


@needs_game
def test_tables_match_the_file(table_fixtures, recordings):
    import gen_tables
    with open(RES, "rb") as f:
        data = f.read()
    kept, tables = _table_listing()
    assert kept and tables
    assert all(0 <= a < b <= len(data) for a, b in kept), kept
    assert all(any(ka <= a and b <= kb for ka, kb in kept) for a, b, _ in tables), "a table outside the kept span"
    runs = _recorded_rom_reads(recordings)
    assert runs
    outside = ["%04X-%04X" % (a, b) for a, b in runs if not any(ka <= a and b <= kb for ka, kb in kept)]
    assert not outside, "recorded reads outside the kept span: %s" % outside
    for image in ("callimg", "rrl"):
        labels, flash = _image(image)
        at = labels["app_tb_pages"]
        assert at % 256 == 0, image
        for k, page in enumerate(gen_tables.GAME_PAGES):
            assert flash[at + 256 * k:at + 256 * (k + 1)] == data[page << 8:(page + 1) << 8], "%s: page %02X" % (image, page)
        for i, page in enumerate(range(gen_tables.MAP_FIRST, gen_tables.MAP_END)):
            want = (at >> 8) + gen_tables.GAME_PAGES.index(page) if page in gen_tables.GAME_PAGES else 0xFF
            assert flash[labels["app_tb_map"] + i] == want, "%s: map of page %02X" % (image, page)
    covered = set()
    for a, b, _ in tables:
        covered.update(range(a, b))
    unlisted = ["%04X-%04X" % (a, b) for a, b in runs if not set(range(a, b)) <= covered]
    assert not unlisted, "recorded reads no listed table explains: %s" % unlisted
    for image in ("callimg", "rrl"):
        labels, flash = _image(image)
        with open(os.path.join(REPO, CALL_BUILD, "tables.s")) as f:
            text = f.read()
        bases = {m.group(1): labels["app_tb_pages"] + int(m.group(2), 16) - int(m.group(3), 16)
                 for m in re.finditer(r"^app_tb_([0-9A-F]{2})\tEQU\tapp_tb_pages\+\$([0-9A-F]+)-\$([0-9A-F]+)$", text, re.M)}
        assert sorted(bases) == ["%02X" % p for p in gen_tables.GAME_PAGES], bases
        assert re.search(r"^app_tables\tEQU\tapp_tb_pages-\$%04X$" % (gen_tables.MAP_FIRST << 8), text, re.M)
        for page, base in bases.items():
            first = int(page, 16) << 8
            for off in range(first, first + 256):
                assert flash[(base + off) & 0xFFFF] == data[off], "%s: app_tb_%s + %04X" % (image, page, off)


@needs_res_docker
def test_game_image_fits_flash_bank_0():
    r = docker("make", "-C", "pocketstation/src/vmu_game", "game")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    path = os.path.join(REPO, "pocketstation", "build", "vmu_game", "rrl.vms")
    size = os.path.getsize(path)
    assert size <= 65536
    blocks = (size + 511) // 512
    assert blocks <= 128
    free = (65536 - 1152) - (size - 1152)
    assert free >= FLASH_FREE_MIN, "rrl.vms is %d bytes, %d free past the header" % (size, free)


# Task 8.3a: the pet save, BASLUSP01013, keeps only the PS1 file's first 0x400
# bytes uncompressed, so two compressed Legend of Mana saves fit beside it.
LOM_SAVE_BLOCKS = 14
GAME_BLOCKS_MAX = 128


def _vmu_flash():
    import vmu_flash
    return vmu_flash


def _res():
    with open(RES, "rb") as f:
        return f.read()


@pytest.mark.skipif(not os.path.exists(RES), reason="needs RES1506.BIN")
def test_pet_save_is_slim():
    vf, data = _vmu_flash(), _res()
    payload = vf.r2_pet(data)
    package = vf.package(vf.PET, payload)
    at = vf.payload_offset(payload)
    kept = vf.vmu_image.PET_KEPT_BYTES
    assert struct.unpack_from("<II", package, 0x48) == (kept, len(payload))
    assert package[at:at + kept] == payload[:kept]
    assert len(package) == at + kept and (len(package) + 511) // 512 <= 5
    assert vf.vmu_image.package_payload(package) == payload[:kept] + bytes(len(payload) - kept)


# Task 8.2b: the spare user blocks must hold every routine still to port at its
# Cortex-M0 size (assembly bytes about equal to C bytes, as level 1 showed)
# plus a margin, so no later level stops on space.
BUDGET_ELF = os.path.join(REPO, "pocketstation", "build", "budget", "budget.elf")
NEED_MARGIN_BLOCKS = 4
NEED_INLINED = {"walk_facing", "draw", "sprite_put", "symbols_draw", "pair_number_draw"}


def _worst_case_need():
    import budget
    r = docker("python3", "-c", "import sys; sys.path.insert(0, 'pocketstation/tools'); import budget; budget.build()")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    sizes = {}
    for name, _, size, is_func in budget.read_elf(BUDGET_ELF)[1]:
        if is_func:
            sizes[name] = max(sizes.get(name, 0), size)
    with open(os.path.join(REPO, CALL_BUILD, "callimg.map")) as f:
        labels = {line.split()[1].lower() for line in f if line.strip()}
    left = [cols[0] for cols in vc.parse_manifest(MANIFEST).values()
            if cols[7] != "verified" and not cols[3].startswith("inlined:")
            and not (cols[7] == "synthetic" and cols[3].lower() in labels)]
    assert {n for n in left if not sizes.get(n)} == NEED_INLINED
    need = sum(sizes[n] for n in left if n not in NEED_INLINED)
    return len(left), need, (need + 511) // 512


@needs_res_docker
def test_game_and_save_fit_one_vmu():
    r = docker("make", "-C", "pocketstation/src/vmu_game", "call", "game")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    vf = _vmu_flash()
    with open(os.path.join(REPO, CALL_BUILD, "rrl.vms"), "rb") as f:
        game = f.read()
    package = vf.package(vf.PET, vf.r2_pet(_res()))
    game_blocks, save_blocks = (len(game) + 511) // 512, (len(package) + 511) // 512
    assert save_blocks <= 5
    routines, need, need_blocks = _worst_case_need()
    finished = game_blocks + need_blocks
    spare = vf.vmu_image.USER_BLOCKS - finished - save_blocks - 2 * LOM_SAVE_BLOCKS
    print("GAME %d + %d still to port = %d blocks (of %d), pet %d, two saves %d: %d of %d spare"
          % (game_blocks, need_blocks, finished, GAME_BLOCKS_MAX, save_blocks, 2 * LOM_SAVE_BLOCKS, spare, vf.vmu_image.USER_BLOCKS))
    print("worst-case need: %d routines left, %d C bytes, %d blocks" % (routines, need, need_blocks))
    assert finished + NEED_MARGIN_BLOCKS <= GAME_BLOCKS_MAX
    assert spare >= NEED_MARGIN_BLOCKS
    vf.build(game, "RRL", vf.r2_pet(_res()), scatter=True)


def _flash_with_pet(name, payload, label):
    vf = _vmu_flash()
    with open(os.path.join(REPO, CALL_BUILD, name + ".vms"), "rb") as f:
        game = f.read()
    img = vf.build(game, "RRL", payload, scatter=label == "scatter")
    rel = "%s/%s-%s.bin" % (CALL_BUILD, name, label)
    with open(os.path.join(REPO, rel), "wb") as f:
        f.write(img)
    return rel, img


@needs_res_docker
def test_game_without_a_save(table_fixtures):
    vf, data = _vmu_flash(), _res()
    outcomes = {}
    for label, payload in (("nopet", None), ("pet", vf.r2_pet(data)), ("scatter", vf.r2_pet(data))):
        rel, _ = _flash_with_pet("rrl", payload, label)
        outcomes[label] = docker(HARNESS, "--flash", rel, "--max-cycles", "3000000")
    r = outcomes["nopet"]
    assert r.returncode == 0 and re.search(r"^exit 02$", r.stdout, re.M), (r.stdout, r.stderr)
    for label in ("pet", "scatter"):
        r = outcomes[label]
        assert not re.search(r"^exit ", r.stdout, re.M), (label, r.stdout)


# Sprite rows stored ready to draw: g_sprite_draw finds a bitmap's rows by its
# address and size and copies them into blit_a and blit_b instead of unpacking.
# Drawn at x = 0x40, a sprite is clipped away after its rows are in place.
SPRITE_PARK_X = 0x40


def _sprite_listing():
    out = []
    with open(TABLE_LISTING) as f:
        for line in f:
            parts = line.split()
            if parts and parts[0] == "sprite":
                out.append((int(parts[1], 16), int(parts[2]), int(parts[3]), int(parts[4])))
    return out


def _image(name):
    with open(os.path.join(REPO, CALL_BUILD, name + ".map")) as f:
        labels = {n.lower(): int(addr, 16) for addr, n in (line.split() for line in f)}
    with open(os.path.join(REPO, CALL_BUILD, name + ".vms"), "rb") as f:
        return labels, f.read()


def _find_stored(labels, flash, p, w, h, masked):
    q_lo, q_hi = (p >> 2) & 0x3F, (p >> 8) & 0xFF
    idx = (flash[labels["g_spr_hash"] + ((q_hi + 8 * w) & 0x3F)] + q_lo) & 0x3F
    if flash[labels["g_spr_kb"] + idx] != q_hi or flash[labels["g_spr_kw"] + idx] != w:
        return None
    kh = flash[labels["g_spr_kh"] + idx]
    if (kh ^ 0x80 if masked else kh & 0x7F) != h:
        return None
    return flash[labels["g_spr_lo"] + idx] | flash[labels["g_spr_hi"] + idx] << 8


def _harness_calls(cc, ctx, name, work, calls):
    template = _records(REC_PATH, name)[0]
    m = ctx.maps[name]
    path = os.path.join(REPO, work)
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path)
    for k, values in enumerate(calls):
        rec = cc.Record(**template.__dict__)
        args = list(rec.args)
        for i, v in values.items():
            a = args[i]
            args[i] = cc.Arg(a.kind, (v & ((1 << (8 * len(a.before))) - 1)).to_bytes(len(a.before), "little"), a.after)
        rec.args = args
        with open(os.path.join(path, "%d.state" % k), "w", newline="\n") as f:
            f.write(cc.Setup(ctx, m, rec).text)
    cc.run_harness(ctx, m.routine, work, len(calls))
    outs = [cc.read_out(os.path.join(path, "%d.out" % k)) for k in range(len(calls))]
    bad = [k for k, o in enumerate(outs) if o.get("status") != "returned"]
    assert not bad, (name, bad[:10])
    return outs


@needs_game
def test_prestored_sprites_match_the_unpacker(call_tools):
    import gen_tables as gt
    cc = _cc()
    ctx = cc.Context()
    with open(RES, "rb") as f:
        data = f.read()
    blit_a, blit_b = ctx.fields["blit_a"], ctx.fields["blit_b"]
    for rec in _records(call_tools, "unpack_rows"):
        src, width, height, mirror, to_mask = (a.value for a in rec.args)
        rows, _ = gt.unpack_rows(data, src - gt.CARD_BASE, width, height, mirror)
        field = blit_b if to_mask else blit_a
        got = b"".join(r.to_bytes(4, "little") for r in rows)
        assert got == rec.ram_after[field.offset:field.offset + len(got)], (hex(src), width, height, mirror, to_mask)
    listed = _sprite_listing()
    assert listed
    stored = {(p, w, h): mask for p, w, h, mask in listed}
    assert len(stored) == len(listed)
    assert all(w % 8 and w < 32 for _, w, _, _ in listed)
    for rec in _records(call_tools, "game_sprite_draw"):
        x, y, width, height, bitmap, flags = (a.value for a in rec.args)
        if width % 8 and width < 32:
            key = (bitmap - gt.CARD_BASE, width, height)
            assert stored.get(key, -1) >= flags & 1, "recorded draw %04X %dx%d not stored" % key
    for image in ("callimg", "rrl"):
        labels, flash = _image(image)
        for p, w, h, mask in listed:
            at = _find_stored(labels, flash, p, w, h, mask)
            assert at is not None, (image, hex(p), w, h, mask)
            rows, nxt = gt.unpack_rows(data, p, w, h, 0)
            if mask:
                rows += gt.unpack_rows(data, nxt, w, h, 0)[0]
            nb = (w + 7) // 8
            want = b"".join(r.to_bytes(4, "little")[:nb] for r in rows)
            assert flash[at:at + len(want)] == want, (image, hex(p), w, h, mask)
            assert _find_stored(labels, flash, p, w, h + 1, mask) is None
            if not mask:
                assert _find_stored(labels, flash, p, w, h, 1) is None
    cases = [(p, w, h, mask, mirror) for p, w, h, mask in listed for mirror in (0, 1)]
    draws = _harness_calls(cc, ctx, "game_sprite_draw", "pocketstation/build/call/prestored_draw",
                           [{0: SPRITE_PARK_X, 1: 0, 2: w, 3: h, 4: gt.CARD_BASE + p, 5: mask | (4 if mirror else 0)}
                            for p, w, h, mask, mirror in cases])
    unpack_calls, plan = [], []
    for p, w, h, mask, mirror in cases:
        plan.append(len(unpack_calls))
        unpack_calls.append({0: gt.CARD_BASE + p, 1: w, 2: h, 3: mirror, 4: 0})
        if mask:
            unpack_calls.append({0: gt.CARD_BASE + gt.unpack_rows(data, p, w, h, 0)[1], 1: w, 2: h, 3: mirror, 4: 1})
    unpacks = _harness_calls(cc, ctx, "unpack_rows", "pocketstation/build/call/prestored_unpack", unpack_calls)
    for (p, w, h, mask, mirror), draw, at in zip(cases, draws, plan):
        n, image = 4 * h, unpacks[at]
        case = (hex(p), w, h, mask, mirror)
        assert draw["bank1"][blit_a.addr:blit_a.addr + n] == image["bank1"][blit_a.addr:blit_a.addr + n], ("blit_a",) + case
        spent = image["cycles"]
        if mask:
            m = unpacks[at + 1]
            assert draw["wram"][blit_b.addr:blit_b.addr + n] == m["wram"][blit_b.addr:blit_b.addr + n], ("blit_b",) + case
            spent += m["cycles"]
        assert draw["cycles"] < spent, ("no faster than unpacking", draw["cycles"], spent) + case
    labels, flash = _image("callimg")
    misses = set()
    for p, w, h, mask in listed:
        for key in ((p, w, h + 1, mask), (p + 4, w, h, mask), (p, w, h, 1)):
            if key[2] <= 32 and _find_stored(labels, flash, *key) is None:
                misses |= {key + (0,), key + (1,)}
    assert len(misses) > 2 * len(listed)
    dropped = [s for s in gt.drawable_sprites(data) if s not in listed]
    assert dropped and all(_find_stored(labels, flash, *s) is None for s in dropped)
    misses = sorted(misses | {s + (mirror,) for s in dropped for mirror in (0, 1)})
    template = _records(REC_PATH, "game_sprite_draw")[0]
    draws = _harness_calls(cc, ctx, "game_sprite_draw", "pocketstation/build/call/unstored_draw",
                           [{0: SPRITE_PARK_X, 1: 0, 2: w, 3: h, 4: gt.CARD_BASE + p, 5: mask | (4 if mirror else 0)}
                            for p, w, h, mask, mirror in misses])
    unpack_calls, plan = [], []
    for p, w, h, mask, mirror in misses:
        plan.append(len(unpack_calls))
        unpack_calls.append({0: gt.CARD_BASE + p, 1: w, 2: h, 3: mirror, 4: 0})
        if mask:
            unpack_calls.append({0: gt.CARD_BASE + gt.unpack_rows(data, p, w, h, 0)[1], 1: w, 2: h, 3: mirror, 4: 1})
    unpacks = _harness_calls(cc, ctx, "unpack_rows", "pocketstation/build/call/unstored_unpack", unpack_calls)
    for (p, w, h, mask, mirror), draw, at in zip(misses, draws, plan):
        n, case = 4 * h, ("unstored", hex(p), w, h, mask, mirror)
        rows, nxt = gt.unpack_rows(data, p, w, h, mirror)
        want = b"".join(r.to_bytes(4, "little") for r in rows)
        assert draw["bank1"][blit_a.addr:blit_a.addr + n] == want, ("blit_a",) + case
        assert unpacks[at]["bank1"][blit_a.addr:blit_a.addr + n] == want, ("unpack blit_a",) + case
        if mask:
            want = b"".join(r.to_bytes(4, "little") for r in gt.unpack_rows(data, nxt, w, h, mirror)[0])
            assert draw["wram"][blit_b.addr:blit_b.addr + n] == want, ("blit_b",) + case
            assert unpacks[at + 1]["wram"][blit_b.addr:blit_b.addr + n] == want, ("unpack blit_b",) + case
        assert draw["xram"][:len(template.fb_before)] == template.fb_before, ("frame buffer",) + case


# Task 8.3b: a drawable sprite keeps stored rows only when race, menu or
# buttons, the scripts whose frames must fit the tick, draw it.
TICK_SCRIPTS = ("race", "menu", "buttons")


@needs_game
def test_stored_sprites_are_the_drawn_ones(call_tools):
    import gen_tables as gt
    import project_frame as pf
    with open(RES, "rb") as f:
        drawable = {s[:3] for s in gt.drawable_sprites(f.read())}
    drawn, tick = set(), set()
    for script in SCRIPTS:
        frames, draws = pf.sprite_draws(script)
        keys = {k for k in draws if k in drawable}
        drawn |= keys
        if script in TICK_SCRIPTS:
            tick |= keys
        print("%s: %d frames draw %d drawable sprites" % (script, frames, len(keys)))
    stored = {s[:3] for s in _sprite_listing()}
    assert stored == tick, (sorted(stored - tick), sorted(tick - stored))
    print("stored %d of %d drawable sprites, %d drawn by race, menu or buttons" % (len(stored), len(drawable), len(tick)))


# A frame drawing one unstored sprite must still fit the tick: the frozen-ratio
# mean race frame (20,441 cycles, scenario 2) plus one such draw stays under 28,700.
MISS_LIMIT = 8000


@needs_game
def test_unstored_sprites_draw_within_a_tick(call_tools):
    import gen_tables as gt
    cc = _cc()
    ctx = cc.Context()
    stored = {s[:3] for s in _sprite_listing()}
    with open(RES, "rb") as f:
        unstored = sorted(s for s in gt.drawable_sprites(f.read()) if s[:3] not in stored)
    assert unstored
    cases = [(p, w, h, mask, mirror) for p, w, h, mask in unstored for mirror in (0, 1)]
    draws = _harness_calls(cc, ctx, "game_sprite_draw", "pocketstation/build/call/reach_draw",
                           [{0: 0, 1: 0, 2: w, 3: h, 4: gt.CARD_BASE + p, 5: mask | (4 if mirror else 0)}
                            for p, w, h, mask, mirror in cases])
    worst = max(zip((d["cycles"] for d in draws), cases))
    print("worst unstored draw: %d cycles for %04X %dx%d mask %d mirror %d" % ((worst[0],) + worst[1]))
    assert worst[0] <= MISS_LIMIT, worst


DEAD_HOT_SET = {"shift_left": "g_shift_left", "shift_right": "g_shift_right"}


def test_dead_hot_set_routines_are_gone():
    rows = vc.parse_manifest(MANIFEST)
    with open(os.path.join(REPO, "pocketstation", "src", "vmu_game", "sprite.s")) as f:
        sprite = f.read()
    for name, routine in DEAD_HOT_SET.items():
        assert rows[name][3] == vc.INLINE_ROUTINE, rows[name]
        assert rows[name][-2] == "-", rows[name]
        assert not re.search(r"^%s:" % routine, sprite, re.M), routine
    assert not re.search(r"^%macro MSKIP\b", sprite, re.M)
    assert not re.search(r"^\tMNEXT \S+, \d+,", sprite, re.M)


# R3b-3 Tasks 5-9: every routine of a level passes all its records. A routine
# whose records run the game's interrupt handler inside vmu_idle waits in
# LEVEL_DEFERRED until --call's rt_idle runs that handler's assembly, which the
# level named in its reason ports.
LEVEL_DEFERRED = {
    0: {"game_wait_ticks": "vmu_idle runs game_irq_callback (level 6) for the recorded ticks"},
    3: {"game_dock_screen": "calls game_wait_ticks, deferred to level 6",
        "game_frame_info": "its 16-byte FrameInfo does not fit ARG_BLOCK; ported with its callers game_marker_start and game_marker_frame (level 4)"},
}


def _level_functions(level):
    rows = vc.parse_manifest(MANIFEST)
    return sorted(n for n, c in rows.items() if int(c[2]) == level and not c[3].startswith("inlined:")
                  and not c[-1].startswith("reading:"))


@needs_game
@pytest.mark.parametrize("level", [0, 1, 2, 3])
def test_level_passes(call_tools, level):
    rows = vc.parse_manifest(MANIFEST)
    deferred = LEVEL_DEFERRED.get(level, {})
    failures = []
    for name in _level_functions(level):
        if name in deferred:
            assert rows[name][-1] == "todo", name
            continue
        r = compare(name)
        lines = (r.stdout + r.stderr).strip().splitlines()
        if r.returncode != 0 or not lines or lines[0] != "ok %d" % len(_records(call_tools, name)) or len(lines) != 1:
            failures.append("%s: %s" % (name, " / ".join(lines[:3])))
        elif rows[name][-1] not in ("verified", "synthetic"):
            failures.append("%s: passes but is marked %s" % (name, rows[name][-1]))
    assert not failures, "\n".join(failures)


# R3b-3 Ruling 15: a branch outcome no record takes has no path key, so the
# untested-key report cannot see it. Every outcome of a level's routines that no
# scripted record takes is taken by a synthetic record that passes, or named
# with a reason in untaken.txt; no line there may be stale.
@needs_game
@pytest.mark.parametrize("level", [0, 1, 2, 3])
def test_level_outcomes_covered(call_tools, level):
    import untaken
    probes = os.path.join(REPO, untaken.PROBES)
    acked = untaken.acknowledged(os.path.join(REPO, untaken.ACKNOWLEDGED))
    deferred = LEVEL_DEFERRED.get(level, {})
    names = [n for n in _level_functions(level) if n not in deferred]
    failures, synth_names, seen = [], set(), set()
    for name in names:
        for o, synth in untaken.untaken(name, call_tools, probes):
            if synth:
                synth_names.add(name)
            elif o.key() in acked:
                seen.add(o.key())
            else:
                failures.append("%s probe %d (line %d %s) %s: no record takes it" % (name, o.probe, o.line, o.kind, o.which))
    for key in acked:
        if key[0] in names and key not in seen:
            failures.append("%s probe %d %s: acknowledged but taken" % key)
    for name in sorted(synth_names):
        r = compare(name)
        lines = (r.stdout + r.stderr).strip().splitlines()
        if r.returncode != 0 or lines != ["ok %d" % len(_records(call_tools, name))]:
            failures.append("%s: %s" % (name, " / ".join(lines[:3])))
    assert not failures, "\n".join(failures)


# The hot drawing set, about three quarters of a race frame in the C: the
# sprite blitter, row unpacking, every fb.c function and the board gathering
# (a recursion group with board_branch), with everything they call.
HOT_ROOTS = ("game_sprite_draw", "unpack_rows", "game_board_gather", "board_branch")


def _hot_set():
    funcs = vc.discover()
    rows = vc.parse_manifest(MANIFEST)
    todo = list(HOT_ROOTS) + sorted(n for n, f in funcs.items() if f.file == "fb.c")
    seen = set()
    while todo:
        name = todo.pop()
        if name not in seen:
            seen.add(name)
            todo.extend(vc.callees(name, funcs))
    return sorted(n for n in seen if not rows[n][3].startswith("inlined:"))


@needs_game
def test_hot_set_passes(call_tools):
    rows = vc.parse_manifest(MANIFEST)
    hot = _hot_set()
    assert {"game_sprite_draw", "plot_rows", "unpack_rows", "mirror_row", "game_fb_clear", "game_fb_put_rows",
            "game_board_gather", "board_branch", "game_nibble"} <= set(hot), hot
    failures = []
    for name in hot:
        r = compare(name)
        lines = (r.stdout + r.stderr).strip().splitlines()
        if r.returncode != 0 or not lines or lines[0] != "ok %d" % len(_records(call_tools, name)) or len(lines) != 1:
            failures.append("%s: %s" % (name, " / ".join(lines[:3])))
        elif rows[name][-1] not in ("verified", "synthetic"):
            failures.append("%s: passes but is marked %s" % (name, rows[name][-1]))
    assert not failures, "\n".join(failures)


# Task 7's speed gate: projects a race frame's assembly cycles from the hot
# set before anything more is ported.
PROJECT_FRAME = os.path.join(REPO, "pocketstation", "tools", "project_frame.py")


def project(*args):
    return subprocess.run([sys.executable, PROJECT_FRAME, *args], capture_output=True, text=True, cwd=REPO)


@pytest.fixture(scope="module")
def projection(call_tools):
    r = project()
    assert r.returncode == 0, r.stdout + r.stderr
    return r


@needs_game
def test_projection_prints_a_frame_estimate(projection):
    r = projection
    assert re.search(r"^hot set assembly cycles/frame [\d.]+$", r.stdout, re.M), r.stdout
    assert re.search(r"^game_sprite_draw beyond plot_rows: [\d.]+ cycles/frame$", r.stdout, re.M), r.stdout
    for label in ("game_sprite_draw", "plot_rows"):
        m = re.search(r"^%s true-frequency weighted: (\d+) distinct tuples, (\d+)/(\d+) calls matched "
                      r"\(status returned\), weighted mean [\d.]+ cycles/call, [\d.]+ cycles/frame$"
                      % re.escape(label), r.stdout, re.M)
        assert m, r.stdout
        assert int(m.group(1)) > 0
        assert int(m.group(2)) == int(m.group(3))  # full coverage: every true call matched a synthesized price
    assert re.search(r"^rom32 hot share: \d+ of \d+ calls \([\d.]+%\), [\d.]+ of its C instructions credited as hot$",
                      r.stdout, re.M), r.stdout
    for scenario in (1, 2):
        assert re.search(r"^C-to-assembly ratio \(scenario %d.*\) [\d.]+ cycles per hot C instruction$" % scenario,
                          r.stdout, re.M), r.stdout
        m = re.search(r"^projected mean \(scenario %d\) (\d+) cycles \(([\d.]+)%% of 28700\)$" % scenario,
                       r.stdout, re.M)
        p = re.search(r"^projected p90 \(scenario %d\) (\d+) cycles \(([\d.]+)%% of 28700\)$" % scenario,
                       r.stdout, re.M)
        assert m and p, r.stdout
        assert 0 < int(m.group(1)) <= int(p.group(1))
        assert re.search(r"^gate \(scenario %d\) (TRIPPED|CLEARED) \(mean [\d.]+, limit 22960\.0\)$" % scenario,
                          r.stdout, re.M), r.stdout
    for name in ("plot_rows", "board_branch"):
        line = re.search(r"^routine %s .*$" % re.escape(name), r.stdout, re.M)
        assert line and "dead" in line.group(0), r.stdout
    gather_line = re.search(r"^routine game_board_gather .*$", r.stdout, re.M)
    assert gather_line, r.stdout
    assert "top-level" in gather_line.group(0), gather_line.group(0)
    assert "dead" not in gather_line.group(0), gather_line.group(0)
    cpf = re.search(r"calls/frame ([\d.]+) \(naive ([\d.]+)\)", gather_line.group(0))
    assert cpf and float(cpf.group(1)) < float(cpf.group(2)), gather_line.group(0)
    sprite_line = re.search(r"^routine game_sprite_draw .*$", r.stdout, re.M)
    assert sprite_line and "generic-cross-check" in sprite_line.group(0), r.stdout


@needs_game
def test_projection_clears_the_gate(projection):
    for scenario in (1, 2):
        m = re.search(r"^projected mean \(scenario %d\) (\d+) cycles" % scenario, projection.stdout, re.M)
        assert m, projection.stdout
        assert int(m.group(1)) <= 22960, projection.stdout
        f = re.search(r"^frozen-ratio mean \(scenario %d, rest at [\d.]+\) (\d+) cycles .*, p90 (\d+) cycles, "
                      r"gate CLEARED$" % scenario, projection.stdout, re.M)
        assert f and int(f.group(1)) <= 22960, projection.stdout
        assert int(f.group(2)) <= 28700, projection.stdout
    assert re.search(r"^break-even rest-of-frame ratio [\d.]+ cycles per C instruction", projection.stdout, re.M)


def test_compute_projection_arithmetic():
    import project_frame as pf
    r = pf.compute_projection(hot_asm=100.0, hot_c=1000.0, hot_c_no_rev32=800.0, sum_instr=10000.0, mean_c=500.0,
                               p90_c=800.0, gate_fraction=0.5, frame_budget=1000.0)
    assert r["share"] == 0.1
    assert r["ratio"] == 2.0
    assert r["proj_mean"] == 1000.0
    assert r["proj_p90"] == 1600.0
    assert r["gate_cycles"] == 500.0
    assert r["tripped"] is True
    assert r["share2"] == 0.08
    assert r["ratio2"] == 2.5
    assert r["proj_mean2"] == 1225.0
    assert r["proj_p90_2"] == 1960.0
    assert r["tripped2"] is True
    assert r["rest_c"] == 450.0
    assert r["frozen_mean"] == 100.0 + pf.FROZEN_RATIO1 * 450.0
    assert r["frozen_p90_2"] == pytest.approx((100.0 + pf.FROZEN_RATIO2 * 450.0) * 800.0 / 500.0)
    assert r["break_even"] == 400.0 / 450.0
    low = pf.compute_projection(hot_asm=100.0, hot_c=1000.0, hot_c_no_rev32=800.0, sum_instr=10000.0, mean_c=500.0,
                                 p90_c=800.0, gate_fraction=10.0, frame_budget=1000.0)
    assert low["tripped"] is False and low["tripped2"] is False
