#----------------------
# | test_render_screens.py
# | Description: Runs the scripted test image once in Flycast and diffs each captured screen against its DuckStation reference, except the shaded world-map screens, which are only checked for being reached.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tools/frame_diff.py, stale_image.py, lom_test.cdi, reference/ps1
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
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import flycast_run
import frame_diff
import stale_image

SCREENS = {"title": "01-title",
           "gname-select-character": "02-gname-select-character",
           "gname-select-weapon": "03-gname-select-weapon",
           "gname-name-entry": "04-gname-name-entry",
           "wsel-room": "05-wsel-room",
           "wsel-prompt": "06-wsel-prompt",
           "wsel-cursor": "07-wsel-cursor",
           "wsel-cursor-confirmed": "08-wsel-cursor-confirmed",
           "wmap-loading": "09-wmap-loading",
           "wmap-mailbox": "10-wmap-mailbox",
           "wmap-mailbox-carried": "11-wmap-mailbox-carried",
           "wmap-place-mailbox-prompt": "12-wmap-place-mailbox-prompt",
           "wmap-land-flash": "13-wmap-land-flash",
           "wmap-home-rising": "14-wmap-home-rising",
           "wmap-home-dark": "15-wmap-home-dark",
           "wmap-board-home": "16-wmap-board-home"}


# The world map's shadow meshes are one multiply pass on the Dreamcast
# (9ba347a0, approved on hardware), which shades the board lighter than the
# PS1's subtraction, so these screens no longer match the DuckStation shots.
SHADED = {"wmap-mailbox", "wmap-mailbox-carried", "wmap-place-mailbox-prompt", "wmap-land-flash",
          "wmap-home-rising", "wmap-home-dark", "wmap-board-home"}


@pytest.fixture(scope="module")
def captures():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = stale_image.require_fresh_test_image(DC)
    out = tempfile.mkdtemp(prefix="lom-render-")
    last = list(SCREENS)[-1]
    code, lines = flycast_run.run_flycast(image, seconds=480, until="LOM: capture " + last, captures=out, no_vmu=True)
    assert code == 0, "\n".join(lines[-60:])
    return out, lines


@pytest.mark.parametrize("name", [n for n in SCREENS if n not in SHADED])
def test_screen_matches_reference(captures, name):
    out, lines = captures
    result = frame_diff.compare(frame_diff.load_capture(os.path.join(out, name + ".png")),
                                frame_diff.load_reference(SCREENS[name]))
    assert frame_diff.matches(result), (name, result, [l for l in lines if "probe" in l or "census" in l][-6:])


@pytest.mark.parametrize("name", sorted(SHADED))
def test_shaded_screen_is_reached(captures, name):
    out, lines = captures
    assert os.path.getsize(os.path.join(out, name + ".png")) > 0
