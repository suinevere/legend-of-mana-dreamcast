#----------------------
# | test_exit_to_map.py
# | Description: Flycast check that answering Yes to Home's Exit to World Map? prompt reaches the world map, which stayed on FIELD's black fade while FIELD wrote its own copy of g_pending_game_state.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tests/stale_image.py, the PADSEL image lom_exit_map_test.cdi
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

import flycast_run  # noqa: E402
import stale_image  # noqa: E402


def test_exit_to_world_map_from_home():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    stale_image.require_fresh_test_image(DC)
    image = os.path.join(DC, "lom_exit_map_test.cdi")
    assert os.path.getmtime(image) >= os.path.getmtime(os.path.join(DC, "lom_test.elf")), "rebuild lom_exit_map_test.cdi"
    code, lines = flycast_run.run_flycast(image, seconds=900, until="LOM: capture wmap-from-field")
    door = [i for i, line in enumerate(lines) if "LOM: field door 65535" in line]
    assert door, "never reached the exit prompt's world-map door:\n" + "\n".join(lines[-40:])
    assert any("LOM: state 1" in line for line in lines[door[-1]:]), "\n".join(lines[-40:])
    assert any("LOM: capture wmap-from-field" in line for line in lines)
