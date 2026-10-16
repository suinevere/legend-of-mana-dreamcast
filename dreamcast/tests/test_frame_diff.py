#----------------------
# | test_frame_diff.py
# | Description: Pins the reference crops and the capture overscan crop, and proves the frame diff accepts a screen against itself and rejects different screens.
# | Author: suinevere
# | Dependencies: tools/frame_diff.py, reference/ps1/*.png, reference/ps1/crops.json
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import json
import os
import sys

import numpy as np
from PIL import Image

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(DC, "tools"))

import frame_diff

D2_SCREENS = ["01-title", "02-gname-select-character", "03-gname-select-weapon", "04-gname-name-entry",
              "05-wsel-room", "06-wsel-prompt", "07-wsel-cursor", "08-wsel-cursor-confirmed"]

D3_WMAP_SCREENS = ["09-wmap-loading", "10-wmap-mailbox", "11-wmap-mailbox-carried",
                    "12-wmap-place-mailbox-prompt", "13-wmap-land-flash", "14-wmap-home-rising",
                    "15-wmap-home-dark", "16-wmap-board-home"]


def test_crops_cover_every_d2_screen_at_four_by_three():
    with open(os.path.join(REPO, "reference", "ps1", "crops.json")) as f:
        crops = json.load(f)
    for name in D2_SCREENS:
        left, top, right, bottom = crops[name]
        assert abs((right - left) * 3 - (bottom - top) * 4) <= 4, name
        assert bottom - top >= 1000, name


def test_crops_json_matches_detection():
    with open(os.path.join(REPO, "reference", "ps1", "crops.json")) as f:
        crops = json.load(f)
    for name in D2_SCREENS:
        assert tuple(crops[name]) == frame_diff.reference_crop(name), name


def test_name_entry_shares_the_session_pillarbox():
    with open(os.path.join(REPO, "reference", "ps1", "crops.json")) as f:
        crops = json.load(f)
    assert crops["04-gname-name-entry"] == crops["03-gname-select-weapon"]


def test_wmap_screens_share_the_session_pillarbox():
    with open(os.path.join(REPO, "reference", "ps1", "crops.json")) as f:
        crops = json.load(f)
    for name in D3_WMAP_SCREENS:
        assert crops[name] == crops["01-title"], name


def test_wmap_crops_json_matches_detection():
    with open(os.path.join(REPO, "reference", "ps1", "crops.json")) as f:
        crops = json.load(f)
    for name in D3_WMAP_SCREENS:
        assert tuple(crops[name]) == frame_diff.reference_crop(name), name


def test_reference_matches_itself_shifted_one_pixel():
    a = frame_diff.load_reference("01-title")
    b = np.roll(a, 1, axis=1)
    assert frame_diff.matches(frame_diff.compare(a, b))


def test_different_screens_differ():
    pairs = [("01-title", "02-gname-select-character"), ("02-gname-select-character", "03-gname-select-weapon"),
             ("04-gname-name-entry", "05-wsel-room"), ("05-wsel-room", "06-wsel-prompt")]
    for a, b in pairs:
        result = frame_diff.compare(frame_diff.load_reference(a), frame_diff.load_reference(b))
        assert not frame_diff.matches(result), (a, b, result)


def test_black_frame_differs_from_title():
    black = np.zeros((240, 320, 3), np.uint8)
    assert not frame_diff.matches(frame_diff.compare(black, frame_diff.load_reference("01-title")))


def test_capture_drops_duckstation_overscan_rows(tmp_path):
    grab = np.full((480, 640, 3), (255, 0, 255), np.uint8)
    grab[16:464] = (0, 200, 0)
    path = os.path.join(str(tmp_path), "grab.png")
    Image.fromarray(grab).save(path)
    frame = frame_diff.load_capture(path)
    assert frame.shape == (240, 320, 3)
    assert not np.any(frame[:, :, 0] > 0)
    assert np.all(frame[:, :, 1] == 200)
