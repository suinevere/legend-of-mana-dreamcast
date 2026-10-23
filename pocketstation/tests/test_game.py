"""----------------------
 | test_game.py
 | Description: Checks the structured Ring Ring Land: each override against its ARM call by call (idle-time and cycle-timed), the game driven by it, and run alone with no R1 code, against every reference trace, that every R1 function has an override, and that the RAM map names every byte the game reads.
 | Author: suinevere
 | Dependencies: pocketstation/Makefile game targets, lom-pocket image, psk_file.py, psk_trace.py, test_reference_traces.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import glob
import os
import re
import shutil
import subprocess
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
GAME = os.path.join(REPO, "pocketstation", "src", "game")
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
sys.path.insert(0, HERE)
import psk_file  # noqa: E402
import psk_trace  # noqa: E402
from test_reference_traces import CARD, KERNEL, NO_RETURN  # noqa: E402

RES = os.path.join(REPO, psk_file.RES_PATH)
SCRIPTS = sorted(os.path.basename(p)[:-4] for p in glob.glob(os.path.join(HERE, "scripts", "*.pks")))
REFERENCE = os.path.join(REPO, "pocketstation", "build", "reference")

pytestmark = [
    pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available"),
    pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted"),
    pytest.mark.skipif(not glob.glob(os.path.join(REFERENCE, "*.pktr")), reason="no reference traces; run test_reference_traces.py --regen"),
]


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))


@pytest.fixture(scope="module")
def built():
    r = docker("make", "-C", "pocketstation", "all", "game", "game-host")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    with open(RES, "rb") as f:
        res = f.read()
    with open(os.path.join(REPO, CARD), "wb") as f:
        f.write(psk_file.reference_card(res))
    os.makedirs(os.path.join(REPO, "pocketstation", "build", "test"), exist_ok=True)


def entry_state(script):
    entry = "pocketstation/build/test/%s.entry" % script
    if not os.path.exists(os.path.join(REPO, entry)):
        r = docker("pocketstation/build/pkrun", "--kernel", KERNEL, "--card", CARD,
                   "--script", "pocketstation/tests/scripts/%s.pks" % script,
                   "--trace", "pocketstation/build/test/entry-%s.pktr" % script, "--idle-time", "--dump-entry-state", entry)
        assert r.returncode == 0, r.stdout[-1000:]
    return entry


def run_game(script, mode):
    trace = "pocketstation/build/test/game-%s-%s.pktr" % (mode, script)
    pks = "pocketstation/tests/scripts/%s.pks" % script
    if mode == "host":
        r = docker("pocketstation/build/rrl_host-game", "--card", CARD, "--script", pks, "--entry", entry_state(script),
                   "--trace", trace)
    else:
        extra = {"verify": ("--idle-time", "--verify", "--hooks-exclude", NO_RETURN),
                 "verify-cycles": ("--verify", "--hooks-exclude", NO_RETURN),
                 "drive": ("--idle-time", "--hooks")}[mode]
        r = docker("pocketstation/build/pkrun-game", "--kernel", KERNEL, "--card", CARD, "--script", pks,
                   "--trace", trace, "--max-seconds", "600", *extra)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    if mode == "drive":
        assert "drive ran ARM app code from 0 entries" in r.stdout, r.stdout[-500:]
    if mode == "verify-cycles":
        return None
    return psk_trace.compare(psk_trace.read(os.path.join(REFERENCE, script + ".pktr")),
                             psk_trace.read(os.path.join(REPO, trace)))


@pytest.mark.parametrize("script", SCRIPTS)
def test_game_verify_idle(built, script):
    assert run_game(script, "verify") is None


@pytest.mark.parametrize("script", SCRIPTS)
def test_game_verify_cycles(built, script):
    assert run_game(script, "verify-cycles") is None


@pytest.mark.parametrize("script", SCRIPTS)
def test_game_drive(built, script):
    assert run_game(script, "drive") is None


@pytest.mark.parametrize("script", SCRIPTS)
def test_game_host(built, script):
    assert run_game(script, "host") is None


def test_game_functions_are_structured():
    names = set()
    with open(os.path.join(GAME, "hooks.txt")) as f:
        for line in f:
            parts = line.split("#", 1)[0].split()
            if len(parts) >= 3 and not parts[2].startswith("regs"):
                names.add(parts[1])
    for path in glob.glob(os.path.join(GAME, "*.c")):
        with open(path) as f:
            text = f.read()
        for name in names:
            assert not re.search(r"\b%s\s*\(\s*PskCpu" % re.escape(name), text), "%s takes PskCpu in %s" % (name, path)


def hook_addresses(path):
    out = set()
    with open(path) as f:
        for line in f:
            parts = line.split("#", 1)[0].split()
            if parts:
                out.add(int(parts[0], 16))
    return out


def test_every_r1_function_is_overridden():
    missing = hook_addresses(os.path.join(GAME, "..", "rrl", "hooks.txt")) - hook_addresses(os.path.join(GAME, "hooks.txt"))
    assert not missing, "R1 functions with no structured override: " + ", ".join("%08X" % a for a in sorted(missing))


@pytest.fixture(scope="module")
def alone(built):
    r = docker("make", "-C", "pocketstation", "game-alone", "game-reads")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]


def run_alone(script, binary="game_host", extra=()):
    trace = "pocketstation/build/test/alone-%s.pktr" % script
    r = docker("pocketstation/build/%s" % binary, "--card", CARD, "--script", "pocketstation/tests/scripts/%s.pks" % script,
               "--entry", entry_state(script), "--trace", trace, *extra)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    return psk_trace.compare(psk_trace.read(os.path.join(REFERENCE, script + ".pktr")),
                             psk_trace.read(os.path.join(REPO, trace)))


@pytest.mark.parametrize("script", SCRIPTS)
def test_game_alone_matches_reference(alone, script):
    assert run_alone(script) is None


def test_ram_map_covers_every_read_byte(alone):
    import gen_ram
    with open(os.path.join(GAME, "ram_map.txt")) as f:
        fields = gen_ram.parse(f.read())
    unnamed = [f.name for f in fields if f.name.startswith("unk_")]
    assert not unnamed, "unnamed fields: " + ", ".join(unnamed)
    owner = {}
    for f in fields:
        if f.ps is None:
            continue
        for a in range(f.ps, f.ps + f.size):
            owner[a] = f.name
    missing = set()
    for script in SCRIPTS:
        reads = "pocketstation/build/test/reads-%s.bin" % script
        assert run_alone(script, "game_host-reads", ("--reads", reads)) is None
        with open(os.path.join(REPO, reads), "rb") as f:
            seen = f.read()
        missing |= {a for a in range(0x200, 0x800) if seen[a] and owner.get(a, "free") == "free"}
    assert not missing, "read bytes outside named fields: " + ", ".join("0x%03X" % a for a in sorted(missing))
