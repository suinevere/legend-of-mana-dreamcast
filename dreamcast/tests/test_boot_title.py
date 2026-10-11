#----------------------
# | test_boot_title.py
# | Description: Boots the Dreamcast build in Flycast and checks it reaches the TITLE state and draws primitives there without halting.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, lom.cdi, include/main/game_state.h (GAME_STATE_TITLE)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(DC, "tools"))

import flycast_run


def game_state_title():
    with open(os.path.join(REPO, "include", "main", "game_state.h")) as f:
        match = re.search(r"#define\s+GAME_STATE_TITLE\s+(\d+)", f.read())
    assert match, "GAME_STATE_TITLE not found"
    return int(match.group(1))


def test_reaches_title_and_draws():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = os.path.join(DC, "lom.cdi")
    if not os.path.exists(image):
        pytest.skip("lom.cdi not built")
    code, lines = flycast_run.run_flycast(image, seconds=60, until=None)
    assert not any("LOM HALT:" in line for line in lines), "\n".join(lines[-40:])
    title = game_state_title()
    seen_title = False
    title_frames = []
    for line in lines:
        if line.strip().endswith("LOM: state %d" % title):
            seen_title = True
        match = re.search(r"LOM: frame \d+ prims (\d+)", line)
        if seen_title and match:
            title_frames.append(int(match.group(1)))
    assert seen_title, "\n".join(lines[-40:])
    assert len(title_frames) >= 3 and all(n > 0 for n in title_frames[-3:]), title_frames
