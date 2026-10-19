"""----------------------
 | test_pkrun.py
 | Description: Builds the lom-pocket runner, boots Ring Ring Land under OpenPSK and checks the trace is live and deterministic.
 | Author: suinevere
 | Dependencies: pocketstation/Makefile, lom-pocket image, pocketstation/tools/psk_file.py, pocketstation/tools/psk_trace.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import os
import re
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
BUILD = os.path.join(REPO, "pocketstation", "build")
CARD = "pocketstation/build/rrl.card"
KERNEL = "pocketstation/build/openpsk-lom.bin"

pytestmark = [
    pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available"),
    pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted"),
]


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True)


def pkrun(script, trace):
    return docker("pocketstation/build/pkrun", "--kernel", KERNEL, "--card", CARD,
                  "--script", script, "--trace", trace, "--max-seconds", "300")


@pytest.fixture(scope="module")
def built():
    r = docker("make", "-C", "pocketstation")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    with open(RES, "rb") as f:
        res = f.read()
    with open(os.path.join(REPO, CARD), "wb") as f:
        f.write(psk_file.reference_card(res))
    os.makedirs(os.path.join(BUILD, "test"), exist_ok=True)
    runs = []
    for i in (1, 2):
        trace = "pocketstation/build/test/boot-%d.pktr" % i
        runs.append((pkrun("pocketstation/tests/scripts/boot.pks", trace), trace))
    return runs


def test_boot_runs_to_the_end(built):
    r, _ = built[0]
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    assert "PSK: done samples 600" in r.stdout


def test_boot_draws_something(built):
    _, trace = built[0]
    t = psk_trace.read(os.path.join(REPO, trace))
    assert len(t.samples) == 600
    assert any(s.vram not in (bytes(128), b"\xff" * 128) for s in t.samples)


def test_boot_is_deterministic(built):
    with open(os.path.join(REPO, built[0][1]), "rb") as a, open(os.path.join(REPO, built[1][1]), "rb") as b:
        assert a.read() == b.read()


def test_swi_census(built):
    r, _ = built[0]
    census = [line for line in r.stdout.splitlines() if re.match(r"PSK: swi \d+ ", line)]
    assert census
    with open(os.path.join(BUILD, "swi_census.txt"), "w") as f:
        f.write("\n".join(census) + "\n")
