#----------------------
# | test_gname_pacing.py
# | Description: Checks the frame probe reports GNAME running at two vblanks per frame or better in Flycast.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, lom_test.cdi
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import flycast_run


def test_gname_runs_at_two_vblanks():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = os.path.join(DC, "lom_test.cdi")
    if not os.path.exists(image):
        pytest.skip("lom_test.cdi not built")
    code, lines = flycast_run.run_flycast(image, seconds=240, until="LOM: capture gname-name-entry")
    assert code == 0, "\n".join(lines[-60:])
    in_gname = False
    rates = []
    for line in lines:
        if line.strip().endswith("LOM: state 3"):
            in_gname = True
        match = re.search(r"LOM: probe vbl (\d+\.\d+)", line)
        if in_gname and match:
            rates.append(float(match.group(1)))
    assert len(rates) >= 3, lines[-40:]
    assert max(rates[1:]) <= 2.05, rates
