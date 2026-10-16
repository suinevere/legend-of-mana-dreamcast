#----------------------
# | test_field.py
# | Description: Boots the scripted test image in Flycast and checks the new-game path reaches FIELD scene 857 from WMAP; later tasks add their gates here.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, stale_image.py, lom_test.cdi, tests/pad_scripts/new_game.pad
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import flycast_run
import stale_image


#----------------------
# | field_run
# | Description: Runs lom_test.cdi once until "LOM: capture field-battle-end" and hands every test in this module the same (code, lines) result.
# | Author: suinevere
# | Dependencies: flycast_run.run_flycast, stale_image.require_fresh_test_image
# | Globals: N/A
# | Params: N/A
# | Returns: (exit code, captured stdout lines)
#----------------------
@pytest.fixture(scope="module")
def field_run():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = stale_image.require_fresh_test_image(DC)
    return flycast_run.run_flycast(image, seconds=900, until="LOM: capture field-battle-end")


def test_field_loads_scene_857_from_wmap(field_run):
    code, lines = field_run
    assert not any("LOM HALT" in l for l in lines), [l for l in lines if "HALT" in l]
    assert any(l.strip().endswith("LOM: field scene 857") for l in lines), lines[-40:]
