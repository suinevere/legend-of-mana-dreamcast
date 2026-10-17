#----------------------
# | test_field.py
# | Description: Boots the scripted test image in Flycast and checks the new-game path reaches FIELD scene 857 from WMAP, then Home walking at two vblanks, then one battle in scene 71 started, fought and ended.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, stale_image.py, lom_test.cdi, tests/pad_scripts/new_game.pad
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


def test_home_runs_at_two_vblanks(field_run):
    code, lines = field_run
    i = next((i for i, l in enumerate(lines) if l.strip().endswith("LOM: field map 18")), None)
    assert i is not None, lines[-40:]
    rates = [float(m.group(1)) for m in (re.search(r"LOM: probe vbl (\d+\.\d+)", l) for l in lines[i:]) if m]
    assert len(rates) >= 6, lines[i:i + 40]
    assert max(rates[2:6]) <= 2.05, rates[:8]


def test_one_battle_starts_resolves_and_returns(field_run):
    code, lines = field_run
    text = [l.strip() for l in lines]
    assert not any("LOM HALT" in l for l in text), [l for l in text if "HALT" in l]
    start = next((i for i, l in enumerate(text) if l.endswith("LOM: field battle 1")), None)
    assert start is not None, text[-40:]
    end = next((i for i, l in enumerate(text) if i > start and l.endswith("LOM: field battle 0")), None)
    assert end is not None, text[start:start + 40]
    assert any("LOM: capture field-battle-end" in l for l in text[end:])
