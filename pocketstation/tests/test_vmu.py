"""----------------------
 | test_vmu.py
 | Description: Tests the VMU interface's host backend against psk_host (tick modes, sound end and RTC delivery, masked expiries, buttons, the buzzer log, the split of tables and pet save, the frame buffer), the state comparison against the original's fields and fb, that the VMU-shaped game uses only the narrow interface, and that it runs every script to its end.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/*, pocketstation/src/psk/psk_host.c, tests/host/vmu_host_test.c, runner/vmu_runner.c, test_game.py, psk_trace.py, lom-pocket image
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import json
import os
import re
import shutil
import subprocess
import sys
import zlib

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
sys.path.insert(0, HERE)
import psk_trace  # noqa: E402
from test_game import CARD, REFERENCE, RES, SCRIPTS, built, entry_state  # noqa: E402,F401

pytestmark = pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available")


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))


@pytest.fixture(scope="module")
def host():
    r = docker("gcc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-Ipocketstation/src/psk", "-Ipocketstation/src/vmu",
               "pocketstation/tests/host/vmu_host_test.c", "pocketstation/src/vmu/vmu_host.c",
               "pocketstation/src/psk/psk_host.c", "-o", "pocketstation/build/vmu_host_test")
    assert r.returncode == 0, r.stdout + r.stderr
    return lambda *a: docker("pocketstation/build/vmu_host_test", *a)


def both(r):
    assert r.returncode == 0, r.stderr
    m = re.match(r"psk (.*)\nvmu (.*)\ncounts (\d+)\n", r.stdout)
    assert m, r.stdout[-500:]
    return m.group(1), m.group(2), int(m.group(3))


@pytest.mark.parametrize("case", ["frame", "slow", "waited", "standby", "same", "fastsame", "rtcsame"])
def test_narrow_timing_matches_psk_host(host, case):
    psk, vmu, counts = both(host("narrow", case))
    assert counts > 1000000 and psk.count("t") > 400 and "r" in psk
    assert psk.count("s") == (0 if case in ("frame", "standby", "rtcsame") else 1)
    if case in ("same", "fastsame"):
        assert re.search(r"(^| )\d+st ", psk)
    if case == "rtcsame":
        assert re.search(r" \d+tr1 ", psk) and not re.search(r"\d[a-z]*[2-9] ", psk)
    assert vmu == psk


def test_masked_expiry_stays_pending_until_unmasked(host):
    psk, vmu, _ = both(host("masked"))
    assert "p100" in psk and psk.count("i100") >= 1
    assert vmu == psk


@pytest.mark.parametrize("mask", ["00", "01", "12", "1F"])
def test_buttons_read_back_and_latch_only_enabled_lines(host, mask):
    r = host("buttons", mask)
    assert r.returncode == 0, r.stderr
    m = int(mask, 16)
    assert r.stdout.split() == ["%02X" % m, "%02X" % m, "%02X" % (m & 5), "00", "00", "%02X" % (m & 5)]


def test_buzz_log_records_each_call_with_its_idle_index(host):
    r = host("buzz")
    assert r.returncode == 0, r.stderr
    assert r.stdout.splitlines() == ["0 b 5", "2 l 0", "2 b 0", "3 l 1"]


def test_save_split_writes_only_the_save(host):
    r = host("save")
    assert r.returncode == 0, r.stderr
    assert r.stdout.split() == ["ok", "0", "bad", "1", "changed", "128", "first", "180", "last", "1FF", "rom", "0", "read", "0",
                                "back", "1", "readbad", "1"]


def test_frame_buffer_is_192_bytes_and_present_calls_the_frame_once(host):
    r = host("frame")
    assert r.returncode == 0, r.stderr
    assert r.stdout.split() == ["bytes", "192", "frames", "1", "fb", "A5"]


WIDE = r"\bvmu_io_|\bVMU_IO_|\bvmu_set_handlers\b|\bvmu_set_clock|\bvmu_clock|\bvmu_set_timer|\bvmu_timer_run|\bVMU_LINE_TIMER|\bVMU_LINE_COM\b|\bVMU_LINE_LINK\b"


def test_vmu_c_uses_only_the_narrow_interface():
    src = os.path.join(REPO, "pocketstation", "src", "vmu")
    found = []
    for name in sorted(n for n in os.listdir(src) if n.endswith(".c") and n != "vmu_host.c"):
        with open(os.path.join(src, name), encoding="utf-8") as f:
            found += ["%s: %s" % (name, m) for m in sorted(set(re.findall(WIDE, f.read())))]
    assert not found, found


def test_vmu_c_has_no_pocketstation_calls():
    src = os.path.join(REPO, "pocketstation", "src", "vmu")
    found = []
    for name in sorted(n for n in os.listdir(src) if not n.startswith("vmu_host")):
        with open(os.path.join(src, name), encoding="utf-8") as f:
            found += ["%s: %s" % (name, m) for m in re.findall(r"\bpsk_\w+|\bPskCpu\b", f.read())]
    assert not found, found[:20]


@pytest.fixture(scope="module")
def vmu_runner(built):
    r = docker("make", "-C", "pocketstation", "vmu-host")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    return "pocketstation/build/vmu_runner"


@pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted")
@pytest.mark.parametrize("script", SCRIPTS)
def test_vmu_runner_reaches_the_end_of_every_script(vmu_runner, script):
    r = docker(vmu_runner, "--card", CARD, "--script", "pocketstation/tests/scripts/%s.pks" % script,
               "--entry", entry_state(script))
    assert r.returncode == 0, r.stdout[-1000:] + r.stderr[-1000:]
    want = len(psk_trace.read(os.path.join(REFERENCE, script + ".pktr")).samples)
    assert re.search(r"samples (\d+)", r.stdout).group(1) == str(want)


def state_frames(ps_frames, vmu_frames):
    return b"".join(ps_frames), b"".join(vmu_frames)


def fields_json(tmp):
    import gen_ram
    out = os.path.join(tmp, "fields.json")
    assert gen_ram.main(["fields", os.path.join(REPO, "pocketstation", "src", "game", "ram_map.txt"), out]) == 0
    with open(out) as f:
        return json.load(f), out


def blank_pair(fields):
    import state_compare
    ps, vmu = bytearray(state_compare.PS_FRAME), bytearray(state_compare.VMU_FRAME)
    seed = next(f for f in fields if f["name"] == "seed")
    ps[seed["ps"] - 0x200:seed["ps"] - 0x200 + 2] = b"\x34\x12"
    vmu[seed["vmu"]:seed["vmu"] + 2] = b"\x34\x12"
    return ps, vmu


def test_state_compare_spots_a_field(tmp_path):
    import state_compare
    fields, _ = fields_json(str(tmp_path))
    ps, vmu = blank_pair(fields)
    assert state_compare.compare(fields, bytes(ps) * 2, bytes(vmu) * 2) == ("same 2", True)
    bad = bytearray(vmu)
    seed = next(f for f in fields if f["name"] == "seed")
    bad[seed["vmu"]] ^= 1
    assert state_compare.compare(fields, bytes(ps) * 2, bytes(vmu) + bytes(bad)) == ("frame 1 field seed ps 4660 vmu 4661", False)


def test_state_compare_maps_handler_addresses_and_file_offsets(tmp_path):
    import state_compare
    fields, _ = fields_json(str(tmp_path))
    ps, vmu = blank_pair(fields)
    slot = next(f for f in fields if f["name"] == "irq_handler")
    snd = next(f for f in fields if f["name"] == "snd_start")
    snd.pop("nocmp")
    ps[slot["ps"] - 0x200 + 8 * 4:slot["ps"] - 0x200 + 9 * 4] = (0x02003E70).to_bytes(4, "little")
    vmu[slot["vmu"] + 8] = 2
    ps[snd["ps"] - 0x200:snd["ps"] - 0x200 + 4] = (0x0200A9F0).to_bytes(4, "little")
    vmu[snd["vmu"]:snd["vmu"] + 2] = (0xA9F0).to_bytes(2, "little")
    assert state_compare.compare(fields, bytes(ps), bytes(vmu)) == ("same 1", True)
    vmu[slot["vmu"] + 8] = 1
    assert state_compare.compare(fields, bytes(ps), bytes(vmu))[0] == "frame 0 field irq_handler[8] ps %d vmu 1" % 0x02003E70


def test_state_compare_skips_nocmp_fields(tmp_path):
    import state_compare
    fields, _ = fields_json(str(tmp_path))
    ps, vmu = blank_pair(fields)
    skipped = [f["name"] for f in fields if f.get("nocmp")]
    assert skipped == ["snd_start", "snd_pos", "snd_len", "draw_skip"]
    pos = next(f for f in fields if f["name"] == "snd_pos")
    vmu[pos["vmu"]] = 7
    assert state_compare.compare(fields, bytes(ps), bytes(vmu)) == ("same 1", True)


def test_state_compare_takes_pixels_from_fb_not_the_lcd(tmp_path):
    import state_compare
    fields, _ = fields_json(str(tmp_path))
    ps, vmu = blank_pair(fields)
    fb = next(f for f in fields if f["name"] == "fb")
    assert fb["vmu"] is None
    ps[0x600] = 0xFF
    assert state_compare.compare(fields, bytes(ps), bytes(vmu)) == ("same 1", True)
    ps[fb["ps"] - 0x200] = 0x01
    assert state_compare.compare(fields, bytes(ps), bytes(vmu)) == ("frame 0 fb x 8 y 0", False)


def test_state_compare_skips_the_pixels_of_undrawn_frames(tmp_path):
    import state_compare
    fields, _ = fields_json(str(tmp_path))
    ps, vmu = blank_pair(fields)
    cycle = next(f for f in fields if f["name"] == "cycle16")
    ps, vmu = ps * 2, vmu * 2
    for k, value in enumerate((16, 15)):
        ps[k * state_compare.PS_FRAME + cycle["ps"] - 0x200] = value
        vmu[k * state_compare.VMU_FRAME + cycle["vmu"]] = value
    vmu[state_compare.VMU_FRAME + 0x400 + 1] = 0x80
    assert state_compare.compare(fields, bytes(ps), bytes(vmu)) == ("same 2", True)
    vmu[0x400 + 1] = 0x80
    assert state_compare.compare(fields, bytes(ps), bytes(vmu)) == ("frame 0 fb x 8 y 0", False)


def test_state_compare_spots_a_pixel(tmp_path):
    import state_compare
    fields, _ = fields_json(str(tmp_path))
    ps, vmu = blank_pair(fields)
    ps[0x40 + 5 * 4] = 0x01
    vmu[0x400 + 5 * 6 + 1] = 0x80
    assert state_compare.compare(fields, bytes(ps), bytes(vmu)) == ("same 1", True)
    vmu[0x400 + 5 * 6 + 1] = 0x40
    assert state_compare.compare(fields, bytes(ps), bytes(vmu)) == ("frame 0 fb x 8 y 5", False)
    vmu[0x400 + 5 * 6 + 1] = 0x80
    vmu[0x400 + 7 * 6] = 0x01
    assert state_compare.compare(fields, bytes(ps), bytes(vmu)) == ("frame 0 fb x 7 y 7", False)


@pytest.fixture(scope="module")
def phase1(built):
    r = docker("make", "-C", "pocketstation", "game-alone")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]


@pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted")
@pytest.mark.parametrize("script", SCRIPTS)
def test_vmu_matches_phase1(phase1, vmu_runner, tmp_path, script):
    import state_compare
    pks = "pocketstation/tests/scripts/%s.pks" % script
    ps_state, vmu_state = "pocketstation/build/test/ps-%s.state" % script, "pocketstation/build/test/vmu-%s.state" % script
    r = docker("pocketstation/build/game_host", "--card", CARD, "--script", pks, "--entry", entry_state(script),
               "--trace", "pocketstation/build/test/ps-%s.pktr" % script, "--state", ps_state)
    assert r.returncode == 0, r.stdout[-1000:] + r.stderr[-1000:]
    r = docker(vmu_runner, "--card", CARD, "--script", pks, "--entry", entry_state(script), "--state", vmu_state)
    assert r.returncode == 0, r.stdout[-1000:] + r.stderr[-1000:]
    fields, _ = fields_json(str(tmp_path))
    with open(os.path.join(REPO, ps_state), "rb") as f:
        ps = f.read()
    with open(os.path.join(REPO, vmu_state), "rb") as f:
        vmu = f.read()
    line, same = state_compare.compare(fields, ps, vmu)
    assert same and ps, line


@pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted")
@pytest.mark.parametrize("script", SCRIPTS)
def test_vmu_pet_save_matches(vmu_runner, script):
    out = "pocketstation/build/test/vmu-%s.file" % script
    r = docker(vmu_runner, "--card", CARD, "--script", "pocketstation/tests/scripts/%s.pks" % script, "--entry", entry_state(script),
               "--file-out", out)
    assert r.returncode == 0, r.stdout[-1000:] + r.stderr[-1000:]
    with open(os.path.join(REPO, CARD), "rb") as f:
        card = bytearray(f.read())
    with open(os.path.join(REPO, out), "rb") as f:
        file = f.read()
    block, at = 1, 0
    while at < len(file):
        card[block * 0x2000:(block + 1) * 0x2000] = file[at:at + 0x2000]
        at += 0x2000
        link = card[block * 0x80 + 8] | (card[block * 0x80 + 9] << 8)
        block = link + 1
    want = psk_trace.read(os.path.join(REFERENCE, script + ".pktr")).card_crc
    assert zlib.crc32(bytes(card)) == want
