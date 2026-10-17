#----------------------
# | test_field.py
# | Description: Boots the scripted test image in Flycast and checks the new-game path reaches FIELD scene 857 from WMAP, then Home walking at two vblanks, then each ported secondary overlay opened and closed, then one battle in scene 71 started, fought and ended, then a scripted game over through GOVER back to the world map, as GOVER's pending state 1 sends the player home.
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
# | Description: Runs lom_test.cdi once until "LOM: capture gover-return" and hands every test in this module the same (code, lines) result.
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
    return flycast_run.run_flycast(image, seconds=1500, until="LOM: capture gover-return", no_vmu=True)


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


def overlay_round_trip(lines, name, modal):
    text = [l.strip() for l in lines]
    assert not any("LOM HALT" in l for l in text), [l for l in text if "HALT" in l]
    i = next((i for i, l in enumerate(text) if l.endswith("LOM: open %s" % name)), None)
    assert i is not None, text[-40:]
    closed = "LOM: field modal 0" if modal else "LOM: field open 0"
    j = next((j for j, l in enumerate(text) if j > i and l.endswith(closed)), None)
    assert j is not None, text[i:i + 60]
    if modal:
        assert any(l.endswith("LOM: field modal %d" % modal) for l in text[i:j]), text[i:j + 1]
    assert any("LOM: capture overlay-%s" % name in l for l in text[i:j]), text[i:j + 1]


def test_shop_opens_and_closes(field_run):
    overlay_round_trip(field_run[1], "shop", 1)


def test_golem_opens_and_returns(field_run):
    overlay_round_trip(field_run[1], "golem", 0)


def test_zukan_opens_and_returns(field_run):
    overlay_round_trip(field_run[1], "zukan", 0)


def test_gosub_opens_and_closes(field_run):
    overlay_round_trip(field_run[1], "gosub", 2)


def test_carda_opens_with_no_card_and_closes(field_run):
    overlay_round_trip(field_run[1], "carda", 3)


def test_niki_opens_with_no_card_and_closes(field_run):
    overlay_round_trip(field_run[1], "niki", 4)


def test_addhero_opens_with_no_card_and_closes(field_run):
    overlay_round_trip(field_run[1], "addhero", 5)


def test_a_game_over_shows_gover_then_returns_to_the_world_map(field_run):
    text = [l.strip() for l in field_run[1]]
    end = next((i for i, l in enumerate(text) if "LOM: capture field-battle-end" in l), None)
    assert end is not None, text[-40:]
    i = next((i for i, l in enumerate(text) if i > end and l.endswith("LOM: field gover 1")), None)
    assert i is not None, text[end:end + 60]
    assert any("LOM: capture overlay-gover" in l for l in text[i:])
    j = next((j for j, l in enumerate(text) if j > i and l.endswith("LOM: state 1")), None)
    assert j is not None, text[i:i + 60]
    assert not any("LOM HALT" in l for l in text), [l for l in text if "HALT" in l]


def test_cload_runs_its_no_card_path_from_the_title_and_returns(field_run):
    text = [l.strip() for l in field_run[1]]
    i = next((i for i, l in enumerate(text) if l.endswith("LOM: state 7")), None)
    assert i is not None, text[:80]
    j = next((j for j, l in enumerate(text) if j > i and l.endswith("LOM: state 2")), None)
    assert j is not None, text[i:i + 60]
    assert any("LOM: capture overlay-cload" in l for l in text[i:j]), text[i:j + 1]
    assert not any("LOM HALT" in l for l in text[:j]), [l for l in text if "HALT" in l]


def test_one_battle_starts_resolves_and_returns(field_run):
    code, lines = field_run
    text = [l.strip() for l in lines]
    assert not any("LOM HALT" in l for l in text), [l for l in text if "HALT" in l]
    start = next((i for i, l in enumerate(text) if l.endswith("LOM: field battle 1")), None)
    assert start is not None, text[-40:]
    end = next((i for i, l in enumerate(text) if i > start and l.endswith("LOM: field battle 0")), None)
    assert end is not None, text[start:start + 40]
    assert any("LOM: capture field-battle-end" in l for l in text[end:])
