#----------------------
# | test_render_screens.py
# | Description: Runs the scripted test image once in Flycast and diffs each captured screen against its DuckStation reference.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tools/frame_diff.py, lom_test.cdi, reference/ps1
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import flycast_run
import frame_diff

SCREENS = {"title": "01-title",
           "gname-select-character": "02-gname-select-character",
           "gname-select-weapon": "03-gname-select-weapon",
           "gname-name-entry": "04-gname-name-entry",
           "wsel-room": "05-wsel-room",
           "wsel-prompt": "06-wsel-prompt",
           "wsel-cursor": "07-wsel-cursor",
           "wsel-cursor-confirmed": "08-wsel-cursor-confirmed"}


@pytest.fixture(scope="module")
def captures():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = os.path.join(DC, "lom_test.cdi")
    if not os.path.exists(image):
        pytest.skip("lom_test.cdi not built")
    out = tempfile.mkdtemp(prefix="lom-render-")
    last = list(SCREENS)[-1]
    code, lines = flycast_run.run_flycast(image, seconds=240, until="LOM: capture " + last, captures=out)
    assert code == 0, "\n".join(lines[-60:])
    return out, lines


@pytest.mark.parametrize("name", list(SCREENS))
def test_screen_matches_reference(captures, name):
    out, lines = captures
    result = frame_diff.compare(frame_diff.load_capture(os.path.join(out, name + ".png")),
                                frame_diff.load_reference(SCREENS[name]))
    assert frame_diff.matches(result), (name, result, [l for l in lines if "probe" in l or "census" in l][-6:])
