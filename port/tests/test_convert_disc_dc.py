#----------------------
# | test_convert_disc_dc.py
# | Description: Checks the Dreamcast target exports TITLE's sound effect set and title song raw instead of skipping them, while the Saturn still skips every sound resource.
# | Author: suinevere
# | Dependencies: tools/convert_disc.py, tools/targets.py, include/main/cd_resources.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(PORT, "tools"))

import convert_disc as cd  # noqa: E402
import targets  # noqa: E402


def test_dreamcast_reads_title_sound_resources_raw():
    assert cd.DREAMCAST_RAW_SOUND_RESOURCES == (21, 23)
    skipped = cd.skipped_resources(targets.DREAMCAST)
    assert 21 not in skipped and 23 not in skipped
    assert 22 in skipped and 24 in skipped


def test_saturn_still_skips_every_sound_resource():
    skipped = cd.skipped_resources(targets.SATURN)
    assert set(cd.SKIPPED_SOUND_RESOURCES) <= set(skipped)
    assert skipped == cd.SKIPPED_RESOURCES + cd.SKIPPED_SOUND_RESOURCES + cd.wmap_resources.SKIPPED
