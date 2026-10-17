#----------------------
# | test_vmu_image.py
# | Description: Checks vmu_image.py builds Flycast's blank VMU layout, adds and reads back data files block-rounded with chained FAT entries, and refuses a file the card cannot hold.
# | Author: suinevere
# | Dependencies: tools/vmu_image.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import struct
import sys

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "tools"))

import vmu_image


def fat(img):
    return struct.unpack_from("<256H", img, 254 * 512)


def test_blank_matches_flycasts_formatted_layout():
    img = vmu_image.blank()
    assert len(img) == 131072
    root = img[255 * 512:]
    assert root[:16] == b"\x55" * 16
    assert struct.unpack_from("<6H", root, 0x46) == (254, 1, 253, 13, 5, 200)
    table = fat(img)
    assert all(entry == 0xFFFC for entry in table[:200])
    assert table[254] == 0xFFFA and table[255] == 0xFFFA
    assert table[241] == 0xFFFA and table[253] == 252 and table[242] == 241
    assert vmu_image.files(img) == []
    assert vmu_image.free_blocks(img) == 200


def test_added_file_reads_back_block_rounded_from_the_top_block_down():
    img = vmu_image.add(vmu_image.blank(), "LOMLOM00", b"x" * 1000)
    assert vmu_image.files(img) == [("LOMLOM00", b"x" * 1000 + b"\0" * 24)]
    table = fat(img)
    assert table[199] == 198 and table[198] == 0xFFFA
    assert vmu_image.free_blocks(img) == 198


def test_two_files_keep_separate_chains():
    img = vmu_image.add(vmu_image.blank(), "A", b"a" * 512)
    img = vmu_image.add(img, "OTHERGAME", b"b" * 1500)
    names = [name for name, _ in vmu_image.files(img)]
    assert names == ["A", "OTHERGAME"]
    assert dict(vmu_image.files(img))["OTHERGAME"][:1500] == b"b" * 1500
    assert vmu_image.free_blocks(img) == 196


def test_a_file_bigger_than_the_free_space_is_refused():
    with pytest.raises(ValueError):
        vmu_image.add(vmu_image.blank(), "BIG", b"z" * (201 * 512))
