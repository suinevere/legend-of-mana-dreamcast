#----------------------
# | test_field_battle_census.py
# | Description: Checks field_battle_census against the converted Dreamcast disc: scene 71 lists monster groups and scene 857 lists none; skipped when the disc is not built.
# | Author: suinevere
# | Dependencies: tools/field_battle_census.py, dreamcast/cd/data/RES
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
RES = os.path.join(os.path.dirname(PORT), "dreamcast", "cd", "data", "RES")
sys.path.insert(0, os.path.join(PORT, "tools"))

import field_battle_census  # noqa: E402


def scene_bytes(scene_id):
    path = field_battle_census.scene_path(RES, scene_id)
    if not os.path.exists(path):
        pytest.skip("Dreamcast disc data not built")
    with open(path, "rb") as f:
        return f.read()


def test_scene_71_has_monster_groups():
    assert field_battle_census.monster_groups(scene_bytes(71)) == [1, 2]


def test_scene_857_has_no_monster_groups():
    assert field_battle_census.monster_groups(scene_bytes(857)) == []


def test_script_triggers_are_not_monster_groups():
    data = bytearray(0x60)
    words = [0x28] + [0x30] * 4 + [0x50] * 5
    for i, word in enumerate(words):
        data[i * 4:i * 4 + 4] = word.to_bytes(4, "little")
    data[0x30:0x34] = (4).to_bytes(4, "little")
    data[0x34:0x38] = (6).to_bytes(2, "little") + (2).to_bytes(2, "little")
    data[0x38 + 8:0x38 + 10] = (0x8003).to_bytes(2, "little")
    data[0x44 + 8:0x44 + 10] = (5).to_bytes(2, "little")
    assert field_battle_census.monster_groups(bytes(data)) == [5]
