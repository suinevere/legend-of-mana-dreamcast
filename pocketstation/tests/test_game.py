"""----------------------
 | test_game.py
 | Description: Checks the structured Ring Ring Land: each override against its ARM call by call (idle-time and cycle-timed), and the game driven by it against every reference trace.
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
