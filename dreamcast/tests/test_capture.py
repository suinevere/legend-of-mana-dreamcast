#----------------------
# | test_capture.py
# | Description: Boots the scripted test image in Flycast and checks the harness saves a 640x480 grab at the title capture marker.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, lom_test.cdi, lom.elf, tests/pad_scripts/new_game.pad
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys
import tempfile

import pytest
from PIL import Image

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import flycast_run


def test_title_capture_is_saved():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = os.path.join(DC, "lom_test.cdi")
    if not os.path.exists(image):
        pytest.skip("lom_test.cdi not built")
    out = tempfile.mkdtemp(prefix="lom-cap-")
    code, lines = flycast_run.run_flycast(image, seconds=90, until="LOM: capture title", captures=out)
    assert code == 0, "\n".join(lines[-40:])
    with Image.open(os.path.join(out, "title.png")) as grab:
        assert grab.size == (640, 480)


def test_shipping_image_has_no_pad_script():
    elf = os.path.join(DC, "lom.elf")
    if not os.path.exists(elf):
        pytest.skip("lom.elf not built")
    with open(elf, "rb") as f:
        data = f.read()
    assert b"pad_script_steps" not in data
    assert b"pad_script_poll" not in data
    assert b"pad_script_start" not in data
