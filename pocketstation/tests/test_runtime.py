"""----------------------
 | test_runtime.py
 | Description: Tests the VMU runtime against vmu_host: each shared scenario runs as scenario_host on vmu_host.c and as the rt_test GAME under the harness, and the two event logs must match line for line; the sleep scenario's idle marks show the schedule stopped while asleep, every base timer interrupt reaches the BIOS clock, the frame buffer reaches XRAM, pet save writes land only on their pages and the package CRC, which stays valid, and the buzzer and LCD calls match the host's log, and on the real schedule a frame loop's runtime cost stays under a tenth of the frame, frames that outlast the tick are reported as missed ticks, and the runtime's counts stay within a period of Timer 0's, also with an overflow pending at a sync.
 | Author: suinevere
 | Dependencies: pocketstation/tools/scenario.py, pocketstation/tests/host/scenario_host.c, pocketstation/src/vmu_rt/*, pocketstation/harness/*, pocketstation/tools/vmu_flash.py, lom-pocket image
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import os
import re
import shutil
import struct
import subprocess
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
BUILD = os.path.join(REPO, "pocketstation", "build", "vmu_rt")
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))

import scenario  # noqa: E402

sys.path.insert(0, os.path.join(REPO, "dreamcast", "tools"))
import vmu_image  # noqa: E402

pytestmark = pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available")


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))


@pytest.fixture(scope="module")
def tools():
    r = docker("make", "-C", "pocketstation/harness")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    r = docker("make", "-C", "pocketstation/src/vmu_rt", "../../build/scenario_host")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]


def host_log(name, files=False, code=0):
    r = docker("make", "-C", "pocketstation/src/vmu_rt", "../../build/vmu_rt/%s.bin" % name,
               "../../build/vmu_rt/tables.bin", "../../build/vmu_rt/pet.bin")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    extra = ["pocketstation/build/vmu_rt/tables.bin", "pocketstation/build/vmu_rt/pet.bin"] if files else []
    r = docker("pocketstation/build/scenario_host", "pocketstation/build/vmu_rt/%s.bin" % name, *extra)
    assert r.returncode == code, r.stdout + r.stderr
    return r.stdout.splitlines()


RUNS = {}


def runtime_log(name, *extra, pet=False, stop="halted"):
    r = docker("make", "-C", "pocketstation/src/vmu_rt", "SCN=%s" % name)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    flash = os.path.join(BUILD, "%s.flash" % name)
    more = ["--pet", os.path.join(BUILD, "pet.bin"), "--scatter"] if pet else []
    r = subprocess.run([sys.executable, os.path.join(REPO, "pocketstation", "tools", "vmu_flash.py"), flash,
                        os.path.join(BUILD, "%s.vms" % name), *more], capture_output=True, text=True)
    assert r.returncode == 0, r.stdout + r.stderr
    rel = "pocketstation/build/vmu_rt/%s" % name
    r = docker("pocketstation/build/vmu_harness", "--flash", rel + ".flash", "--inputs", rel + ".inputs",
               "--log", rel + ".log", "--marks", rel + ".marks", "--max-cycles", "2000000000", *extra)
    assert r.returncode == 0, r.stdout + r.stderr
    assert stop in r.stdout, r.stdout
    RUNS[name] = r.stdout
    return scenario.decode(open(os.path.join(BUILD, "%s.log" % name), "rb").read())


def test_scenario_tool_round_trips_ops_inputs_and_log(tmp_path):
    src = tmp_path / "t.scn"
    src.write_text("# comment\nram_write 0x150 0xA5\nidle 2\npress 0x01 1\nrelease 0x01 2\ndock 1 3\nlog_ram 0x150\n")
    ops = scenario.parse(src.read_text())
    assert scenario.compile_ops(ops) == bytes([0x14, 0x01, 0x50, 0xA5, 0x07, 0x00, 0x02, 0x08, 0x01, 0x00, 0x01,
                                               0x09, 0x01, 0x00, 0x02, 0x0A, 0x01, 0x00, 0x03, 0x13, 0x01, 0x50, 0x00])
    assert scenario.inputs(ops) == "press 0x10 1\nrelease 0x10 2\ndock 1 3\n"
    log = bytes([0x6D, 0x01, 0x50, 0xA5, 0x68, 0x01, 0x00, 0x69, 0x00, 0x02, 0x72, 0x5A,
                 0x77, 0x00, 0x03, 0x00, 0x78, 0x02, 0x45, 0x01])
    assert scenario.decode(log) == ["m 150 A5", "h 0100", "i 0002", "r 5A", "w 0003 00", "x 02", "error 01"]
    assert scenario.compile_ops(scenario.parse("log_buttons\nlog_sys\nlog_levels\n")) == bytes([0x15, 0x16, 0x17, 0x00])
    assert scenario.decode(bytes([0x62, 0x10, 0x73, 0x02, 0x6C, 0x04, 0x00])) == ["b 10", "s 02", "l 0400"]
    with pytest.raises(ValueError):
        scenario.parse("ram_write 0x300 1\n")
    with pytest.raises(ValueError):
        scenario.parse("log_ram 0x01F\n")
    with pytest.raises(ValueError):
        scenario.parse("frobnicate 1\n")
    assert scenario.parse("repeat 2\nidle 1\nrepeat 2\nlog_buttons\nend_repeat\nend_repeat\n") == [
        ("idle", [1]), ("log_buttons", []), ("log_buttons", []), ("idle", [1]), ("log_buttons", []), ("log_buttons", [])]
    for bad in ("repeat 2\nidle 1\n", "end_repeat\n", "repeat 0\nend_repeat\n", "repeat 2\npress 1 1\nend_repeat\n"):
        with pytest.raises(ValueError):
            scenario.parse(bad)


def test_runtime_scaffold_runs_ram_ops(tools):
    expect = host_log("ticks")
    got = runtime_log("ticks")
    assert expect[:2] == ["m 020 5A", "m 150 A5"]
    assert {"m 050 11", "m 0FF 96", "m 100 69", "m 200 C3", "m 021 00"} <= set(expect)
    assert got == expect


def handler_calls(log):
    return [line for line in log if line.startswith("h ")]


def check_ticks(log):
    assert handler_calls(log) == ["h 0100"] * 40
    assert [line for line in log if line.startswith("i ")] == ["i %04X" % n for n in range(1, 41)]


def check_sounds(log):
    calls = handler_calls(log)
    assert calls[:5] == ["h 2000", "h 0100", "h 2000", "h 2000", "h 0100"]
    assert calls.count("h 2100") == 1
    assert log[log.index("h 2100") + 1] == "i 0024"


def check_modes(log):
    calls = handler_calls(log)
    assert "h 0000" in calls and "h 2000" in calls
    assert calls.count("h 0100") > 40


def check_masked(log):
    assert handler_calls(log) == ["h 0100", "h 0000"]
    assert log[-3:] == ["i 0001", "h 0000", "i 0002"]


def check_between(log):
    assert handler_calls(log) == ["h 2000", "h 2000", "h 0100", "h 2000", "h 2000", "h 0100"]


CHECKS = {"ticks": check_ticks, "sounds": check_sounds, "modes": check_modes, "masked": check_masked,
          "between": check_between}


@pytest.mark.parametrize("name", sorted(CHECKS))
def test_runtime_events_match_host(tools, name):
    expect = host_log(name)
    assert not [line for line in expect if line.startswith("error")], expect
    CHECKS[name](expect)
    got = runtime_log(name)
    assert got == expect


def runtime_marks(name):
    with open(os.path.join(BUILD, "%s.marks" % name)) as f:
        return [int(x) for x in f.read().split()]


INPUT_LOGS = {
    "buttons": ["h 0100", "h 0100", "h 0110", "b 10", "l 0010", "h 0100", "h 0109", "b 09", "l 0009", "h 0100",
                "h 0100", "l 0001", "h 0100", "s 00", "h 0102", "s 02", "s 00", "b 03", "h 0100", "b 00", "h 0110"],
    "sleep": ["h 0100", "h 0100", "h 0110", "h 0100", "h 0100", "b 00", "l 0000", "h 0100", "h 0110", "b 10",
              "h 0100", "h 0100"],
    "dock": ["l 0400", "h 0100", "h 0100", "l 0000", "h 0500", "l 0400", "h 0100", "l 0400", "h 0100", "h 0100",
             "l 0400", "h 0100"],
    "repress": ["h 0010", "h 0010", "h 0000", "b 10"],
}


@pytest.mark.parametrize("name", sorted(INPUT_LOGS))
def test_runtime_input_matches_host(tools, name):
    expect = host_log(name)
    assert [line for line in expect if not line.startswith("i ")] == INPUT_LOGS[name]
    got = runtime_log(name)
    assert got == expect
    if name == "sleep":
        marks = runtime_marks(name)
        gaps = [b - a for a, b in zip(marks, marks[1:])]
        assert len(gaps) == 8
        assert all(25000 < g < 40000 for g in gaps[:3] + gaps[4:]), gaps
        assert gaps[3] > 1000000, gaps


def timer_counts(name):
    words = RUNS[name].split()
    at = words.index("timer_ex")
    return int(words[at + 1]), int(words[at + 3]), int(words[at + 5])


@pytest.mark.parametrize("name", ["sleep", "modes"])
def test_runtime_forwards_every_base_timer_interrupt_to_the_bios_clock(tools, name):
    runtime_log(name)
    calls, overflows, seconds = timer_counts(name)
    assert calls == overflows >= 3
    assert seconds == calls // 2
    if name == "sleep":
        marks = runtime_marks(name)
        assert marks[3] < 500000 and marks[4] > 500000 * overflows


def frames(name):
    data = open(os.path.join(BUILD, "%s.dump" % name), "rb").read()
    size = 0x400 + 192
    assert len(data) % size == 0
    return [data[i + 0x400:i + size] for i in range(0, len(data), size)]


def test_runtime_screen_matches_host(tools):
    expect = host_log("screen")
    shown = [bytes.fromhex(line[2:]) for line in expect if line.startswith("f ")]
    assert len(shown) == 2 and all(len(fb) == 192 for fb in shown)
    assert shown[0][0] == 0x80 and shown[0][5] == 0x01 and shown[0][6] == 0xC3 and shown[0][15 * 6 + 2] == 0xFF
    assert shown[0][16 * 6 + 4] == 0x7E and shown[0][31 * 6 + 5] == 0xA5 and shown[1][16 * 6 + 4] == 0
    with open(os.path.join(BUILD, "empty_hw.json"), "w") as f:
        f.write("[]\n")
    rel = "pocketstation/build/vmu_rt/screen"
    got = runtime_log("screen", "--dump", rel + ".dump", "--hw", "pocketstation/build/vmu_rt/empty_hw.json")
    assert got == [line for line in expect if not line.startswith("f ")]
    assert frames("screen") == shown


def sector_page(img, sector):
    chain = vmu_image.chain(img, vmu_image.vmu_name_for_ps1("BASLUSP01013"))
    page = 1 + 4 * 2 + sector
    return chain[page // 4] * vmu_image.BLOCK + (page % 4) * 128


def test_runtime_files_match_host(tools):
    expect = host_log("files", files=True)
    table = scenario.test_table()
    assert [line for line in expect if line[0] in "rwv"] == ["r %02X" % table[n] for n in (0, 1, 0xFF, 0x100, 0x1FF)] + [
        "v 0000 00", "w 0006 00", "w 0007 00", "v 0006 00", "v 0007 00", "w 0040 01", "v 0040 01", "w 0100 01",
        "w 0006 01", "v 0006 01", "v 0006 01", "v 0006 00"]
    first = expect.index("v 0000 00") + 1
    assert expect[first:first + 4] == ["m 100 53", "m 101 43", "m 102 12", "m 103 %02X" % scenario.test_pet()[3]]
    fields = [line for line in expect if line[:2] == "m " and int(line[2:5], 16) < 0x100]
    assert fields == ["m 07C 5C", "m 080 81", "m 0C6 C7", "m 0F8 9F"] * 12
    assert [line for line in expect if line[0] in "bs"] == ["b 10", "b 10", "s 02", "b 10"]
    rel = "pocketstation/build/vmu_rt/files"
    got = runtime_log("files", "--out-flash", rel + ".out", pet=True)
    assert got == expect
    before = open(os.path.join(BUILD, "files.flash"), "rb").read()
    after = open(os.path.join(BUILD, "files.out"), "rb").read()
    chain = vmu_image.chain(before, vmu_image.vmu_name_for_ps1("BASLUSP01013"))
    assert all(a - b != 1 for a, b in zip(chain, chain[1:])) and min(chain) >= 128
    assert sector_page(before, 6) // vmu_image.BLOCK != sector_page(before, 7) // vmu_image.BLOCK
    want = bytearray(before)
    payload = bytearray(scenario.test_pet())
    for sector, start in ((6, 0x40), (7, 0xC0)):
        at = sector_page(before, sector)
        want[at:at + 128] = bytes((start + i) & 0xFF for i in range(128))
        payload[sector * 128:sector * 128 + 128] = want[at:at + 128]
    package = bytearray(b"".join(want[b * vmu_image.BLOCK:(b + 1) * vmu_image.BLOCK] for b in chain))
    package[0x46:0x48] = b"\0\0"
    head = chain[0] * vmu_image.BLOCK + 0x46
    want[head:head + 2] = struct.pack("<H", vmu_image.vms_crc(bytes(package[:128 + 512 * 2 + len(payload)])))
    assert want[head:head + 2] != before[head:head + 2]
    assert after == bytes(want)
    assert vmu_image.ps1_file(after, "BASLUSP01013") == bytes(payload)
    stored, = struct.unpack_from("<H", after, head)
    package[0x46:0x48] = b"\0\0"
    assert stored == vmu_image.vms_crc(bytes(package[:128 + 512 * 2 + len(payload)]))


def test_runtime_power_matches_host(tools):
    expect = host_log("power", code=5)
    assert [line for line in expect if not line.startswith("i ")] == ["l 0200", "h 0100", "b 10", "h 0010", "x 02"]
    assert runtime_log("power", stop="exit 02") == expect
    expect = host_log("exit_docked", code=5)
    assert expect == ["h 0000", "i 0001", "x 03"]
    assert runtime_log("exit_docked", stop="exit 03") == expect


def test_runtime_buzz_logs_tones(tools):
    expect = host_log("buzz")
    tones = [line for line in expect if line.startswith("z ")]
    assert tones == ["z 0000 0 60", "z 0002 0 C0", "z 0002 1 00", "z 0003 0 00", "z 0003 1 01", "z 0003 0 01",
                     "z 0003 0 FF", "z 0003 0 FF", "z 0004 0 00", "z 0004 0 00", "z 0004 1 00"]
    rel = "pocketstation/build/vmu_rt/buzz"
    got = runtime_log("buzz", "--buzz", rel + ".buzz")
    assert got == [line for line in expect if not line.startswith("z ")]
    with open(os.path.join(BUILD, "buzz.buzz")) as f:
        assert f.read().splitlines() == tones


FRAME_BUDGET = 30760
PERIOD_COUNTS = 32


def report(name):
    out = RUNS[name]
    lines = ((r"^frames (\d+) period (\d+)$", ("frames", "period")),
             (r"^cycles/frame mean (\d+) p90 (\d+) max (\d+) frame \d+$", ("mean", "p90", "max")),
             (r"^runtime cycles/frame mean (\d+) max (\d+)$", ("rt_mean", "rt_max")),
             (r"^ticks (\d+) late max (\d+) cycles$", ("ticks", "late_max")),
             (r"^counts drift min (-?\d+) max (-?\d+)$", ("drift_min", "drift_max")),
             (r"^missed ticks (\d+) \(overrun (\d+), late (\d+)\)$", ("missed", "overrun", "late")))
    found = {}
    for pattern, keys in lines:
        m = re.search(pattern, out, re.M)
        if m:
            found.update(zip(keys, (int(v) for v in m.groups())))
    return found


def report_run(name):
    expect = host_log(name)
    rel = "pocketstation/build/vmu_rt/" + name
    got = runtime_log(name, "--schedule", "real", "--report", "--map", rel + ".map")
    assert got == [line for line in expect if not line.startswith("f ")]
    return expect, report(name)


def test_real_schedule_reports_runtime_cost(tools):
    expect, r = report_run("frame_loop")
    assert expect.count("h 0100") >= 280 and expect.count("b 00") == 6000
    assert r["frames"] == 299 and r["ticks"] >= 280
    assert abs(r["period"] - FRAME_BUDGET) < 50, r
    assert r["missed"] == r["overrun"] == r["late"] == 0, r
    assert -PERIOD_COUNTS < r["drift_min"] <= r["drift_max"] < PERIOD_COUNTS, r
    assert 0 < r["rt_mean"] < r["mean"] <= r["p90"] <= r["max"] < FRAME_BUDGET
    assert r["rt_mean"] <= r["rt_max"] < FRAME_BUDGET // 10, r


def test_real_schedule_reports_frames_that_outlast_the_tick(tools):
    expect, r = report_run("frame_overrun")
    assert expect.count("h 0100") == 8
    assert r["frames"] == 7 and r["ticks"] == 8
    assert r["overrun"] == 7 and r["late"] == 0 and r["missed"] == 7, r


@pytest.mark.parametrize("name", sorted(CHECKS) + sorted(INPUT_LOGS))
def test_runtime_matches_host_on_the_real_schedule(tools, name):
    expect = host_log(name)
    got = runtime_log(name, "--schedule", "real")
    assert got == expect


def test_real_schedule_credits_an_overflow_pending_at_a_sync_once(tools):
    expect, r = report_run("sync_overflow")
    assert expect.count("h 2000") == 0 and expect.count("i 0001") == 200
    assert -PERIOD_COUNTS < r["drift_min"] <= r["drift_max"] < PERIOD_COUNTS, r
    assert runtime_log("sync_overflow") == expect
