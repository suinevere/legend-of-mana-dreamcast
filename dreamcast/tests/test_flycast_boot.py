#----------------------
# | test_flycast_boot.py
# | Description: Boots lom.cdi in Flycast and checks the serial log reports the boot.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, lom.cdi
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import flycast_run


def test_boot_line_in_serial_log():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = os.path.join(DC, "lom.cdi")
    if not os.path.exists(image):
        pytest.skip("lom.cdi not built")
    code, lines = flycast_run.run_flycast(image, seconds=20, until="LOM: boot")
    assert code == 0, "\n".join(lines[-40:])
