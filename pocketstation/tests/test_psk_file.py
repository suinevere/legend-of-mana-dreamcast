"""----------------------
 | test_psk_file.py
 | Description: Tests the PocketStation file parser and memory-card image builder against RES1506.BIN.
 | Author: suinevere
 | Dependencies: pocketstation/tools/psk_file.py, pytest
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import os
import sys
import unicodedata

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tools"))
import psk_file  # noqa: E402

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
RES = os.path.join(REPO, psk_file.RES_PATH)

needs_res = pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted")


@pytest.fixture
def res():
    with open(RES, "rb") as f:
        return f.read()


@needs_res
def test_parse_res1506(res):
    p = psk_file.parse(res)
    assert len(res) == 47236
    assert p.blocks == 6
    assert p.ident == "MCX0"
    assert p.viewer_frames == 0
    assert p.icon_list == 1
    assert p.func_count == 0
    assert p.body_offset == 0x188
    assert p.entry == 0x020027A8
    assert "RING" in unicodedata.normalize("NFKC", p.title)
    assert p.data == res


@needs_res
def test_parse_rejects_bad_magic(res):
    with pytest.raises(ValueError):
        psk_file.parse(b"XX" + res[2:])


@needs_res
def test_parse_rejects_bad_ident(res):
    bad = bytearray(res)
    bad[0x52:0x56] = b"ABCD"
    with pytest.raises(ValueError):
        psk_file.parse(bytes(bad))


@needs_res
def test_parse_rejects_entry_past_end(res):
    bad = bytearray(res)
    bad[0x5C:0x60] = (psk_file.FLASH1_BASE + len(res) + 0x100).to_bytes(4, "little")
    with pytest.raises(ValueError):
        psk_file.parse(bytes(bad))


@needs_res
def test_card_image_roundtrip(res):
    img = psk_file.card_image([(psk_file.DEFAULT_NAME, res)])
    assert len(img) == psk_file.CARD_BYTES
    files = psk_file.card_files(img)
    assert len(files) == 1
    name, data = files[0]
    assert name == psk_file.DEFAULT_NAME
    assert data[: len(res)] == res
    for i in range(16):
        frame = img[i * 0x80 : (i + 1) * 0x80]
        x = 0
        for b in frame[:0x7F]:
            x ^= b
        assert x == frame[0x7F], i
    assert img[0:2] == b"MC"
    assert img[0x80 + 0x0A + 6 : 0x80 + 0x0A + 7] == b"P"
    links = [int.from_bytes(img[i * 0x80 + 8 : i * 0x80 + 10], "little") for i in range(1, 7)]
    assert links == [1, 2, 3, 4, 5, 0xFFFF]
    states = [img[i * 0x80] for i in range(1, 8)]
    assert states == [0x51, 0x52, 0x52, 0x52, 0x52, 0x53, 0xA0]
    assert int.from_bytes(img[0x80 + 4 : 0x80 + 8], "little") == 6 * psk_file.BLOCK_BYTES
    assert img[psk_file.BLOCK_BYTES : psk_file.BLOCK_BYTES + len(res)] == res


def test_card_image_too_many_blocks():
    with pytest.raises(ValueError):
        psk_file.card_image([("BASLUSP0000A", b"\0" * 6 * 0x2000), ("BASLUSP0000B", b"\0" * 10 * 0x2000)])


@needs_res
def test_pet_transfer_layout(res):
    pet = bytes(range(0x60))
    out = psk_file.pet_transfer(res, egg_species=0x21, level=7, pet=pet, placed_lands={1, 2, 6, 7, 8, 14, 31},
                                maxed_mana={8: 0x81})
    assert len(out) == len(res)
    assert out[:0x300] == res[:0x300]
    assert b"SAVE DATA AREA" not in out
    t = 0x300
    assert out[t:t + 8] == bytes(8)
    assert out[t + 8:t + 0x3C] == bytes(0x34)
    assert out[t + 0x3C:t + 0x40] == bytes([0x21, 7, 3, 6])
    assert int.from_bytes(out[t + 0x40:t + 0x44], "little") == (1 << 0) | (1 << 1) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 25)
    assert out[t + 0x44] == 0
    assert out[t + 0x45] == 0x81
    assert out[t + 0x46:t + 0x58] == bytes(18)
    assert out[t + 0x58:t + 0xB8] == pet
    assert out[t + 0xB8:] == res[t + 0xB8:]


@needs_res
def test_reference_card_holds_the_reference_pet(res):
    img = psk_file.reference_card(res)
    name, data = psk_file.card_files(img)[0]
    assert name == psk_file.DEFAULT_NAME
    assert data[0x358:0x3B8] == psk_file.REFERENCE_PET
    assert psk_file.REFERENCE_PET[0] != 0
