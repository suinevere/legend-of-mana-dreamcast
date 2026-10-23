"""----------------------
 | test_budget.py
 | Description: Tests the instruction budget tool: the VMU-shaped game built for Cortex-M0 runs a script under Unicorn and reports every frame and its costliest functions.
 | Author: suinevere
 | Dependencies: pocketstation/tools/budget.py, runner/budget_main.c, runner/budget.ld, test_game.py, lom-pocket image
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
sys.path.insert(0, HERE)
from test_game import RES, built, entry_state  # noqa: E402,F401

pytestmark = [
    pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available"),
    pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted"),
]


def test_budget_reports_every_frame(built):
    entry_state("boot")
    r = subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket",
                        "python3", "pocketstation/tools/budget.py", "boot"],
                       capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-2000:]
    assert re.search(r"^frames 600$", r.stdout, re.M), r.stdout
    m = re.search(r"^mean (\d+) p90 (\d+) max (\d+) at (\d+)$", r.stdout, re.M)
    assert m and 0 < int(m.group(1)) <= int(m.group(2)) <= int(m.group(3)), r.stdout
    top = r.stdout.split("\ntop\n", 1)[1].strip().splitlines()
    assert len(top) == 15 and all(re.match(r"^\s*\d+ \S+$", line) for line in top), r.stdout
    assert any(line.split()[1].startswith("game_") for line in top)
