#----------------------
# | test_soft_reset.py
# | Description: Flycast check that A+B+X+Y+Start restarts the game: the scripted chord during the intro movie logs the reset and the game boots and reaches the intro again, from this CD-R image's scrambled 1ST_READ.BIN.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tests/stale_image.py, lom_soft_reset_test.cdi, tests/pad_scripts/soft_reset.pad, src/system/soft_reset.c, src/system/boot_descramble.c
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


def test_chord_restarts_the_game():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    stale_image.require_fresh_test_image(DC)
    image = os.path.join(DC, "lom_soft_reset_test.cdi")
    elf = os.path.join(DC, "lom_test.elf")
    assert os.path.exists(image) and os.path.getmtime(image) >= os.path.getmtime(elf), \
        "%s missing or older than lom_test.elf; rebuild the test image" % image
    code, lines = flycast_run.run_flycast(image, seconds=90, until=None)
    assert not any(flycast_run.HALT_MARKER in line for line in lines), "\n".join(lines[-60:])
    resets = [i for i, line in enumerate(lines) if "LOM: soft reset" in line]
    assert resets, "\n".join(lines[-60:])
    after = lines[resets[0] + 1:]
    assert any("LOM: boot" in line for line in after), "\n".join(lines[-60:])
    assert any("LOM: pad script soft_reset" in line for line in after), "\n".join(lines[-60:])
