#----------------------
# | test_ranch_patch.py
# | Description: The Dreamcast copy of the ranch scene (resource 1573, MHM_ALC2.IMG) offers the Ring Ring Land option and has its four transfer lines.
# | Author: suinevere
# | Dependencies: port/tools/scene_patches.py, port/tools/convert_disc.py, port/tools/disc_image.py, tools/scenes
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
sys.path.insert(0, REPO)

import convert_disc  # noqa: E402
import disc_image  # noqa: E402
import overlay_text  # noqa: E402
import scene_patches  # noqa: E402
from tools.scenes.event_scripts import read_event_scripts  # noqa: E402
from tools.scenes.scene_format import SceneHeader  # noqa: E402
from tools.scenes.strings import read_strings  # noqa: E402


#----------------------
# | ranch
# | Description: The ranch scene bytes from the user's disc image, or a skip without one.
# | Author: suinevere
# | Dependencies: convert_disc.default_image, disc_image
# | Globals: N/A
# | Params: N/A
# | Returns: bytes
#----------------------
@pytest.fixture(scope="module")
def ranch():
    path = convert_disc.default_image()
    if not os.path.exists(path):
        pytest.skip("no PS1 disc image (set LOM_PS1_IMAGE)")
    image = disc_image.DiscImage(path)
    blob = image.read_sectors(convert_disc.TABLE_LBA, (convert_disc.TABLE_SIZE + 2047) // 2048)[:convert_disc.TABLE_SIZE]
    table = disc_image.parse_resource_table(blob)
    return disc_image.read_resource(image, table[scene_patches.RANCH_RESOURCE])


#----------------------
# | test_ranch_offers_ring_ring_land_and_says_its_lines
# | Description: Both ring menus include option 10, only their mask bytes change in the event scripts, the four lines decode as written,
# |              every other string and every later section are unchanged, and later sections move by a multiple of four.
# | Author: suinevere
# | Dependencies: scene_patches.patch, read_event_scripts, read_strings, overlay_text.load_lines
# | Globals: N/A
# | Params: ranch -- original scene bytes
# | Returns: N/A
#----------------------
def test_ranch_offers_ring_ring_land_and_says_its_lines(ranch):
    out = scene_patches.patch(scene_patches.RANCH_RESOURCE, ranch)
    before, after = SceneHeader.parse(ranch), SceneHeader.parse(out)
    old_events = ranch[before.event_scripts:before.strings]
    new_events = out[after.event_scripts:after.strings]
    assert len(old_events) == len(new_events)
    changed = {i + before.event_scripts for i in range(len(old_events)) if old_events[i] != new_events[i]}
    assert changed == {0x2D2 + 4, 0x2DB + 4}
    masks = {i["offset"]: i["operands"]["excluded_mask"]["value"] for i in read_event_scripts(out, after)["instructions"]
             if i["offset"] in (0x2D2, 0x2DB)}
    assert masks == {0x2D2: 0x1C5F & ~0x400, 0x2DB: 0x1C3F & ~0x400}
    shift = after.actor_scripts - before.actor_scripts
    assert shift > 0 and shift % 4 == 0
    assert all(a - b == shift for a, b in zip(tuple(after)[3:], tuple(before)[3:]))
    assert out[after.actor_scripts:] == ranch[before.actor_scripts:]
    lines = overlay_text.load_lines()["scene_mhm_alc2"]
    old_strings = {e["index"]: e for e in read_strings(ranch, before)["strings"]}
    for entry in read_strings(out, after)["strings"]:
        if entry["index"] in lines:
            assert bytes.fromhex(entry["bytes"]) == overlay_text.encode_scene(lines[entry["index"]])
        else:
            assert entry["bytes"] == old_strings[entry["index"]]["bytes"]


#----------------------
# | test_other_resources_pass_through
# | Description: A resource with no patch comes back unchanged.
# | Author: suinevere
# | Dependencies: scene_patches.patch
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
def test_other_resources_pass_through():
    assert scene_patches.patch(1564, b"abc") == b"abc"
