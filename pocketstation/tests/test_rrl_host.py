"""----------------------
 | test_rrl_host.py
 | Description: Tests the no-emulator host: LCD, buttons, RTC, a replay of the race's kernel calls, and rrl_host matching every reference trace.
 | Author: suinevere
 | Dependencies: pocketstation/src/psk/psk_host.c, tests/host/host_test.c, lom-pocket image, psk_trace.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import glob
import os
import shutil
import subprocess
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
import psk_file  # noqa: E402
import psk_trace  # noqa: E402

RES = os.path.join(REPO, psk_file.RES_PATH)
REFERENCE = os.path.join(REPO, "pocketstation", "build", "reference", "race.pktr")

pytestmark = [
    pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available"),
    pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted"),
]


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True)


@pytest.fixture(scope="module")
def host():
    r = docker("make", "-C", "pocketstation")
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-2000:]
    r = docker("gcc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-Ipocketstation/src/psk",
               "pocketstation/tests/host/host_test.c", "pocketstation/src/psk/psk_host.c",
               "-o", "pocketstation/build/host_test")
    assert r.returncode == 0, r.stdout + r.stderr
    return lambda *a: docker("pocketstation/build/host_test", *a)


def test_lcd_write_appears_in_vram(host):
    r = host("lcd")
    assert r.returncode == 0, r.stderr
    vram = bytes.fromhex(r.stdout.strip())
    for row in range(32):
        assert int.from_bytes(vram[4 * row:4 * row + 4], "little") == 0x80000001 | (row << 8)


@pytest.mark.parametrize("mask", ["00", "01", "12", "1F"])
def test_buttons_read_back_at_the_input_register(host, mask):
    r = host("buttons", mask)
    assert r.stdout.strip() == mask


def test_rtc_reads_back_in_bcd_and_advances(host):
    r = host("rtc", "00260101", "05125958")
    assert r.stdout.split() == ["05125958", "00260101", "05125959", "00260101", "05130058", "00260101"]


def test_replayed_kernel_calls_match_the_reference(host):
    card = "pocketstation/build/rrl.card"
    with open(RES, "rb") as f:
        with open(os.path.join(REPO, card), "wb") as out:
            out.write(psk_file.reference_card(f.read()))
    r = docker("pocketstation/build/pkrun", "--kernel", "pocketstation/build/openpsk-lom.bin", "--card", card,
               "--script", "pocketstation/tests/scripts/race.pks", "--trace", "pocketstation/build/test/swilog.pktr",
               "--swi-log", "pocketstation/build/test/race.swilog", "--dump-entry-ram", "pocketstation/build/kernel_ram.bin")
    assert r.returncode == 0, r.stdout[-1000:]
    r = host("replay", "pocketstation/build/test/race.swilog", card, "pocketstation/build/kernel_ram.bin")
    words = r.stdout.split()
    assert "mismatch swi" not in r.stdout, r.stdout
    assert int(words[1]) >= 9
    reference = psk_trace.read(os.path.join(REPO, "pocketstation", "build", "test", "swilog.pktr"))
    assert int(words[-1], 16) == reference.card_crc


SCRIPTS = sorted(os.path.basename(p)[:-4] for p in glob.glob(os.path.join(HERE, "scripts", "*.pks")))


@pytest.fixture(scope="module")
def rrl_host():
    r = docker("make", "-C", "pocketstation", "host")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    return "pocketstation/build/rrl_host"


@pytest.mark.parametrize("script", SCRIPTS)
def test_decompiled_c_alone_matches_the_reference(rrl_host, script):
    card = "pocketstation/build/rrl.card"
    entry = "pocketstation/build/test/%s.entry" % script
    pks = "pocketstation/tests/scripts/%s.pks" % script
    r = docker("pocketstation/build/pkrun", "--kernel", "pocketstation/build/openpsk-lom.bin", "--card", card,
               "--script", pks, "--trace", "pocketstation/build/test/entry.pktr", "--idle-time",
               "--dump-entry-state", entry)
    assert r.returncode == 0, r.stdout[-1000:]
    out = "pocketstation/build/test/host-%s.pktr" % script
    r = docker(rrl_host, "--card", card, "--script", pks, "--entry", entry, "--trace", out)
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-2000:]
    d = psk_trace.compare(psk_trace.read(os.path.join(REPO, "pocketstation", "build", "reference", script + ".pktr")),
                          psk_trace.read(os.path.join(REPO, out)))
    assert d is None, d
