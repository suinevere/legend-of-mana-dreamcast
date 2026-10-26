"""----------------------
 | test_harness.py
 | Description: Tests the LC86K harness: halting, flash images with a GAME and R2's pet package, ROM entry interception with the BIOS page write and its ROM return, the registers the BIOS routines destroy, the debug log and present ports, .pks scripts, input events, per-frame dumps, the idle schedule and the report's late ticks.
 | Author: suinevere
 | Dependencies: pocketstation/harness/*, pocketstation/tests/asm/*.s, pocketstation/tools/vmu_flash.py, dreamcast/tools/vmu_image.py, lom-pocket image
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import json
import os
import re
import shutil
import struct
import subprocess
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
BUILD = os.path.join(REPO, "pocketstation", "build", "asm")
sys.path.insert(0, os.path.join(REPO, "dreamcast", "tools"))

import vmu_image

pytestmark = pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available")


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))


def assemble(name):
    os.makedirs(BUILD, exist_ok=True)
    out = os.path.join(BUILD, "%s.vms" % name)
    if os.path.exists(out):
        os.remove(out)
    r = docker("waterbear", "assemble", "pocketstation/tests/asm/%s.s" % name, "-o", "pocketstation/build/asm/%s.vms" % name)
    assert r.returncode == 0 and os.path.getsize(out) > 0, r.stdout + r.stderr
    return "pocketstation/build/asm/%s.vms" % name


def vmu_flash(out, game, *extra):
    return subprocess.run([sys.executable, os.path.join(REPO, "pocketstation", "tools", "vmu_flash.py"), out, game, *extra],
                          capture_output=True, text=True)


def flash(name, *extra):
    game = assemble(name)
    out = "pocketstation/build/asm/%s.flash" % name
    r = vmu_flash(os.path.join(REPO, out), os.path.join(REPO, game), *extra)
    assert r.returncode == 0, r.stdout + r.stderr
    return out


def counts(stdout):
    m = re.search(r"cycles (\d+) steps (\d+)", stdout)
    assert m, stdout
    return int(m.group(1)), int(m.group(2))


@pytest.fixture(scope="module")
def harness():
    r = docker("make", "-C", "pocketstation/harness")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    return "pocketstation/build/vmu_harness"


def test_halt_program_halts(harness):
    game = assemble("halt")
    r = docker(harness, "--game", game, "--max-cycles", "1000")
    assert r.returncode == 0, r.stdout + r.stderr
    assert "halted" in r.stdout
    assert "acc 5A" in r.stdout


def test_flash_image_lists_game_and_pet():
    game = os.path.join(REPO, assemble("halt"))
    pet = os.path.join(BUILD, "pet.bin")
    payload = bytes((i * 13 + 5) & 0xFF for i in range(0x2000))
    with open(pet, "wb") as f:
        f.write(payload)
    for extra in ([], ["--scatter"]):
        out = os.path.join(BUILD, "listing.flash")
        r = vmu_flash(out, game, "--pet", pet, *extra)
        assert r.returncode == 0, r.stdout + r.stderr
        img = open(out, "rb").read()
        code = open(game, "rb").read()
        assert len(img) == 131072
        assert vmu_image.files(img)[0] == ("HALT", code + b"\0" * (-len(code) % 512))
        assert vmu_image.chain(img, "HALT") == list(range((len(code) + 511) // 512))
        assert vmu_image.ps1_file(img, "BASLUSP01013") == payload[:vmu_image.PET_KEPT_BYTES] + bytes(len(payload) - vmu_image.PET_KEPT_BYTES)
        blocks = vmu_image.chain(img, vmu_image.vmu_name_for_ps1("BASLUSP01013"))
        contiguous = all(a - b == 1 for a, b in zip(blocks, blocks[1:]))
        assert contiguous == (extra == [])


def test_unknown_rom_entry_stops_the_run(harness):
    img = flash("rom_call")
    r = docker(harness, "--flash", img, "--max-cycles", "100000")
    assert r.returncode == 3, r.stdout + r.stderr
    assert "rom entry 0140 not modelled" in r.stdout


def test_flash_entries_write_read_and_exit(harness):
    img = flash("flash_call")
    out = "pocketstation/build/asm/flash_call.out"
    r = docker(harness, "--flash", img, "--out-flash", out, "--max-cycles", "100000")
    assert r.returncode == 0, r.stdout + r.stderr
    assert "exit FC" in r.stdout
    data = open(os.path.join(REPO, out), "rb").read()
    before = open(os.path.join(REPO, img), "rb").read()
    assert data[0x300:0x380] == bytes(range(0x80, 0x100))
    assert data[:0x300] == before[:0x300] and data[0x380:] == before[0x380:]


def test_bios_page_write_programs_a_bank_1_page_and_returns_through_rom_0103(harness):
    img = flash("page_call")
    out = "pocketstation/build/asm/page_call.out"
    r = docker(harness, "--flash", img, "--out-flash", out, "--max-cycles", "100000")
    assert r.returncode == 0, r.stdout + r.stderr
    assert "exit DB" in r.stdout
    data = open(os.path.join(REPO, out), "rb").read()
    before = open(os.path.join(REPO, img), "rb").read()
    assert data[0x1C800:0x1C880] == bytes(b ^ 0x5A for b in range(0x80, 0x100))
    assert data[:0x1C800] == before[:0x1C800] and data[0x1C880:] == before[0x1C880:]


def test_bios_flash_routines_poison_the_registers_they_destroy_and_timer_ex_keeps_all(harness):
    img = flash("clobber")
    r = docker(harness, "--flash", img, "--log", "pocketstation/build/asm/clobber.log", "--max-cycles", "100000")
    assert r.returncode == 0, r.stdout + r.stderr
    log = open(os.path.join(BUILD, "clobber.log"), "rb").read()
    p = 0xA5
    assert log == bytes([p, 0x22, 0x33, p, p, p, p,
                         0x00, 0x22, 0x33, p, p, p, p,
                         p, p, p, p, p, p, p,
                         0x11, 0x22, 0x33, 0x44, 0x55, 0x01, 0x66])


def test_unmodelled_rom_return_stops_the_run(harness):
    img = flash("page_return")
    r = docker(harness, "--flash", img, "--max-cycles", "100000")
    assert r.returncode == 3, r.stdout + r.stderr
    assert "rom return 1234 not modelled" in r.stdout


def test_bios_page_write_with_interrupts_on_stops_the_run(harness):
    img = flash("page_ie")
    r = docker(harness, "--flash", img, "--max-cycles", "100000")
    assert r.returncode == 3, r.stdout + r.stderr
    assert "rom entry E024 entered with interrupts enabled" in r.stdout


def test_log_port_bytes_reach_the_log(harness):
    img = flash("log")
    r = docker(harness, "--flash", img, "--log", "pocketstation/build/asm/log.log", "--max-cycles", "100000")
    assert r.returncode == 0, r.stdout + r.stderr
    assert open(os.path.join(BUILD, "log.log"), "rb").read() == b"\x01\x02\x03"


def test_present_port_dumps_vmu_ram_and_the_frame_buffer(harness):
    img = flash("frame")
    hw = [{"name": "wram", "offset": 0x010, "space": "bank1", "addr": 0x010, "size": 2},
          {"name": "b", "offset": 0x105, "space": "wram", "addr": 0x005, "size": 1}]
    with open(os.path.join(BUILD, "frame.json"), "w") as f:
        json.dump(hw, f, indent=1)
    r = docker(harness, "--flash", img, "--hw", "pocketstation/build/asm/frame.json", "--log", "pocketstation/build/asm/frame.log",
               "--dump", "pocketstation/build/asm/frame.state", "--max-cycles", "100000")
    assert r.returncode == 0, r.stdout + r.stderr
    assert open(os.path.join(BUILD, "frame.log"), "rb").read() == b"\xFF"
    state = open(os.path.join(BUILD, "frame.state"), "rb").read()
    assert len(state) == 2 * (0x400 + 192)
    first, second = state[:0x4C0], state[0x4C0:]
    assert first[0x010:0x012] == b"\xAB\x00" and first[0x105] == 0xCD and first[0x400] == 0xEF
    assert sum(first) == 0xAB + 0xCD + 0xEF
    assert second[0x011] == 0x12 and second[0x400 + 6 * 16 + 5] == 0x34


def test_script_samples_drive_port3_at_frame_points(harness):
    img = flash("sample")
    with open(os.path.join(BUILD, "empty.json"), "w") as f:
        f.write("[]\n")
    r = docker(harness, "--flash", img, "--script", "pocketstation/tests/asm/sample.pks", "--hw", "pocketstation/build/asm/empty.json",
               "--log", "pocketstation/build/asm/sample.log", "--dump", "pocketstation/build/asm/sample.state", "--max-cycles", "100000")
    assert r.returncode == 0, r.stdout + r.stderr
    assert "end samples 7" in r.stdout
    assert open(os.path.join(BUILD, "sample.log"), "rb").read() == bytes([0xFF, 0xFF, 0xEF, 0xFF, 0xF6, 0xF6, 0xFF])
    assert os.path.getsize(os.path.join(BUILD, "sample.state")) == 7 * (0x400 + 192)


def test_input_press_wakes_a_halted_program(harness):
    img = flash("press")
    with open(os.path.join(BUILD, "press.in"), "w") as f:
        f.write("press 0x10 1\n")
    r = docker(harness, "--flash", img, "--inputs", "pocketstation/build/asm/press.in", "--max-cycles", "1000000")
    assert r.returncode == 0, r.stdout + r.stderr
    assert "acc EF" in r.stdout


def test_input_idle_indices_count_rt_idle_writes_not_halts(harness):
    img = flash("press")
    with open(os.path.join(BUILD, "press2.in"), "w") as f:
        f.write("press 0x10 2\n")
    r = docker(harness, "--flash", img, "--inputs", "pocketstation/build/asm/press2.in", "--max-cycles", "1000000")
    assert r.returncode == 0, r.stdout + r.stderr
    assert "halted" in r.stdout and "acc EF" not in r.stdout


def test_pet_package_matches_r2s_kos_write(harness):
    sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
    import vmu_flash
    kos = open(os.path.join(REPO, "dreamcast", "src", "card", "card_store_kos.c")).read()
    short = re.search(r'#define KOS_VMU_SHORT_DESC "([^"]*)"', kos).group(1)
    app = re.search(r'#define KOS_VMU_APP_ID "([^"]*)"', kos).group(1)
    assert "pkg.icon_anim_speed = 8;" in kos and "pkg.eyecatch_type = VMUPKG_EC_NONE;" in kos
    payload = bytearray((i * 7 + 3) & 0xFF for i in range(0x2000))
    payload[0:4] = b"SC\x12\x01"
    with open(os.path.join(BUILD, "icon.ps1"), "wb") as f:
        f.write(payload)
    r = docker("sh", "-c", "gcc -std=c11 -Wall -Idreamcast/src/card -o pocketstation/build/asm/vmu_icon_dump "
               "pocketstation/tests/host/vmu_icon_dump.c dreamcast/src/card/vmu_format.c && "
               "pocketstation/build/asm/vmu_icon_dump pocketstation/build/asm/icon.ps1 pocketstation/build/asm/icon.out")
    assert r.returncode == 0, r.stdout + r.stderr
    icon = open(os.path.join(BUILD, "icon.out"), "rb").read()
    count = icon[0]
    pkg = vmu_flash.package("BASLUSP01013", bytes(payload))
    assert count == 2
    assert pkg[0:16] == short.encode().ljust(16, b" ")
    assert pkg[16:48] == b"BASLUSP01013".ljust(32, b" ")
    assert pkg[48:64] == app.encode().ljust(16, b"\0")
    assert struct.unpack_from("<HHH", pkg, 0x40) == (count, 8, 0)
    assert struct.unpack_from("<II", pkg, 0x48) == (vmu_image.PET_KEPT_BYTES, len(payload))
    assert pkg[0x50:0x60] == bytes(16)
    assert pkg[0x60:0x80] == icon[1:33]
    assert pkg[0x80:0x80 + 512 * count] == icon[33:]
    assert pkg[0x80 + 512 * count:] == bytes(payload[:vmu_image.PET_KEPT_BYTES])
    assert struct.unpack_from("<H", pkg, 0x46)[0] == vmu_image.vms_crc(pkg[:0x46] + b"\0\0" + pkg[0x48:])
    plain = vmu_flash.package("BASLUSP01013", bytes(0x100))
    assert struct.unpack_from("<H", plain, 0x40)[0] == 1 and plain[0x80:0x280] == bytes(512)


def test_idle_schedule_skips_to_the_timer(harness):
    img = flash("timer")
    idle = docker(harness, "--flash", img, "--schedule", "idle", "--max-cycles", "10000000")
    assert idle.returncode == 0, idle.stdout + idle.stderr
    assert "acc 77" in idle.stdout
    cycles, steps = counts(idle.stdout)
    assert cycles >= 0x10000
    assert steps * 100 < cycles
    real = docker(harness, "--flash", img, "--schedule", "real", "--max-cycles", "10000000")
    assert real.returncode == 0, real.stdout + real.stderr
    assert "acc 77" in real.stdout
    real_cycles, real_steps = counts(real.stdout)
    assert real_cycles == cycles
    assert real_steps > cycles // 2


def test_idle_schedule_matches_real_with_base_and_timer1_interrupts(harness):
    img = flash("base")
    runs = [docker(harness, "--flash", img, "--schedule", s, "--max-cycles", "10000000") for s in ("idle", "real")]
    for r in runs:
        assert r.returncode == 0, r.stdout + r.stderr
    (idle_cycles, idle_steps), (real_cycles, real_steps) = counts(runs[0].stdout), counts(runs[1].stdout)
    assert runs[0].stdout.split("\n")[1] == runs[1].stdout.split("\n")[1]
    assert idle_cycles == real_cycles and idle_cycles >= 3 * 32768 * 5461 // 65536
    assert idle_steps * 5 < real_steps


def test_report_counts_a_tick_delivered_after_its_due_time_as_late(harness):
    img = flash("late_tick")
    r = docker("python3", "pocketstation/tools/vmu_map.py", "pocketstation/tests/asm/late_tick.s",
               "pocketstation/build/asm/late_tick.vms", "pocketstation/build/asm/late_tick.map",
               "pocketstation/build/asm/map_probe")
    assert r.returncode == 0, r.stdout + r.stderr
    r = docker(harness, "--flash", img, "--schedule", "real", "--report", "--map", "pocketstation/build/asm/late_tick.map",
               "--max-cycles", "10000000")
    assert r.returncode == 0, r.stdout + r.stderr
    late = re.search(r"^ticks (\d+) late max (\d+) cycles$", r.stdout, re.M)
    missed = re.search(r"^missed ticks (\d+) \(overrun (\d+), late (\d+)\)$", r.stdout, re.M)
    assert late and missed, r.stdout
    assert int(late.group(1)) == 4 and int(late.group(2)) > 2 * 63, r.stdout
    assert missed.groups() == ("3", "0", "3"), r.stdout
