"""----------------------
 | test_vmu_relax.py
 | Description: Tests vmu_relax.py: far calls and jumps shrink to CALL, JMP or BR where they reach, while JMPF tables, numeric targets, macro bodies and calls across a 4 KB block stay far.
 | Author: suinevere
 | Dependencies: pocketstation/tools/vmu_relax.py, pocketstation/tools/vmu_map.py, pocketstation/tests/asm/relax.s, lom-pocket image
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import os
import shutil
import subprocess

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = "pocketstation/build/relax_test"

pytestmark = pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available")


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))


def test_relax_shrinks_only_what_reaches():
    r = docker("sh", "-c",
               "rm -rf %s && python3 pocketstation/tools/vmu_relax.py pocketstation/tests/asm/relax.s %s/src %s/relax.vms"
               " && python3 pocketstation/tools/vmu_map.py %s/src/pocketstation/tests/asm/relax.s %s/relax.vms %s/relax.map %s/probe"
               % ((OUT,) * 7))
    assert r.returncode == 0, r.stdout + r.stderr
    with open(os.path.join(REPO, OUT, "relax.map")) as f:
        labels = {name: int(addr, 16) for addr, name in (line.split() for line in f)}
    assert labels["r_start"] == 0x0200
    assert labels["r_close"] == 0x0204
    assert labels["r_table_end"] == 0x0210
    assert labels["r_numeric_end"] == 0x0213
    assert labels["r_macro_end"] == 0x0216
    with open(os.path.join(REPO, OUT, "relax.vms"), "rb") as f:
        code = f.read()
    assert code[0x0204] == 0x20 and code[0x0207:0x0210:3] == bytes([0x21] * 3)
    assert code[0x0210] == 0x20 and code[0x0213] == 0x20
    assert code[0x0202] == 0x01
