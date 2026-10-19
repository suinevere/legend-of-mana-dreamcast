"""----------------------
 | test_reference_traces.py
 | Description: Runs every .pks script on the reference runner and checks its trace against the stored reference; --regen rewrites them.
 | Author: suinevere
 | Dependencies: pocketstation/Makefile, lom-pocket image, pocketstation/tools/psk_file.py, pocketstation/tools/psk_trace.py
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
SCRIPTS = sorted(os.path.basename(p)[:-4] for p in glob.glob(os.path.join(HERE, "scripts", "*.pks")))
CARD = "pocketstation/build/rrl.card"
NO_RETURN = "020027A8,02003348"
KERNEL = "pocketstation/build/openpsk-lom.bin"

pytestmark = [
    pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available"),
    pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted"),
]


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True)


def run(script, trace, extra=()):
    r = docker("pocketstation/build/pkrun", "--kernel", KERNEL, "--card", CARD,
               "--script", "pocketstation/tests/scripts/%s.pks" % script, "--trace", trace,
               "--max-seconds", "600", "--idle-time", *extra)
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    if "--hooks" in extra:
        assert "drive ran ARM app code from 0 entries" in r.stdout, r.stdout[-500:]
    return psk_trace.read(os.path.join(REPO, trace))


@pytest.fixture(scope="module")
def built():
    r = docker("make", "-C", "pocketstation")
    assert r.returncode == 0, r.stdout[-3000:] + r.stderr[-3000:]
    with open(RES, "rb") as f:
        res = f.read()
    with open(os.path.join(REPO, CARD), "wb") as f:
        f.write(psk_file.reference_card(res))
    for d in ("test", "reference", "coverage"):
        os.makedirs(os.path.join(REPO, "pocketstation", "build", d), exist_ok=True)


def test_scripts_exist():
    assert {"boot", "menu", "race", "save", "laps", "buttons"} <= set(SCRIPTS)


@pytest.mark.parametrize("script", SCRIPTS)
def test_reference_trace(built, request, script):
    ref = "pocketstation/build/reference/%s.pktr" % script
    cov = "pocketstation/build/coverage/%s.txt" % script
    extra = ("--coverage", cov)
    if request.config.getoption("--drive"):
        extra += ("--hooks",)
    if request.config.getoption("--verify"):
        exclude = ",".join(x for x in (NO_RETURN, request.config.getoption("--hooks-exclude")) if x)
        extra += ("--verify", "--hooks-exclude", exclude)
    got = run(script, "pocketstation/build/test/%s.pktr" % script, extra)
    if request.config.getoption("--regen"):
        again = run(script, "pocketstation/build/test/%s-again.pktr" % script)
        assert psk_trace.compare(got, again) is None
        shutil.copyfile(os.path.join(REPO, "pocketstation/build/test/%s.pktr" % script), os.path.join(REPO, ref))
    assert os.path.exists(os.path.join(REPO, ref)), "no reference; run with --regen"
    d = psk_trace.compare(psk_trace.read(os.path.join(REPO, ref)), got)
    assert d is None, d
    with open(os.path.join(REPO, cov)) as f:
        assert f.read().split()
