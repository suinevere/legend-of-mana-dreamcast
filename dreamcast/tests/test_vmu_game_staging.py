#----------------------
# | test_vmu_game_staging.py
# | Description: The Dreamcast disc carries Ring Ring Land's GAME as /cd/VMU/RRL.VMS, byte for byte the PocketStation build's rrl.vms, and CARDA has the replace question and the install's block count.
# | Author: suinevere
# | Dependencies: dreamcast/Makefile (cd-data stamp), pocketstation/build/vmu_game/rrl.vms, port/tools/overlay_text.py, port/tools/pocketstation_en.py, tools/overlays/text_table.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(REPO, "port", "tools"))
sys.path.insert(0, os.path.join(REPO, "tools", "overlays"))

import overlay_text  # noqa: E402

GAME = os.path.join(REPO, "pocketstation", "build", "vmu_game", "rrl.vms")
STAGED = os.path.join(DC, "build-test", "cd-data", "VMU", "RRL.VMS")
CARDA_RAW = os.path.join(REPO, "build", "us", "overlays", "carda", "carda.raw")
CARDA_TABLE = 0x8014B038
CARDA_VRAM = 0x80140000


def test_the_test_disc_carries_the_game():
    if not os.path.exists(GAME) or not os.path.exists(os.path.join(DC, "build-test", "cd-data", ".stamp")):
        pytest.skip("needs the PocketStation GAME and the test disc's cd-data")
    with open(GAME, "rb") as a, open(STAGED, "rb") as b:
        assert b.read() == a.read()


def test_carda_has_the_replace_question_and_block_count():
    lines = overlay_text.load_lines()["carda"]
    assert lines[82] == "Replace the other VMU game?"
    assert lines[49] == "126 Blocks to download."


def test_the_replace_question_fills_an_empty_us_entry():
    if not os.path.exists(CARDA_RAW):
        pytest.skip("PS1 overlay not built")
    import text_table
    with open(CARDA_RAW, "rb") as f:
        table = text_table.parse(f.read(), CARDA_TABLE - CARDA_VRAM, text_table.TWO_BYTE_CODES["us"])
    assert not [e for e in table.entries if e.index == 82][0].data
