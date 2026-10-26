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
import random
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


def test_ps1_file_finds_a_package_by_its_ps1_name_and_returns_the_payload():
    payload = bytes((i * 7) & 0xFF for i in range(0xC000))
    name = vmu_image.vmu_name_for_ps1("BASLUSP01013")
    assert name.startswith("LOMX") and len(name) == 12
    assert vmu_image.vmu_name_for_ps1("BASLUS-01013LOM00") == "LOMLOM00"
    img = vmu_image.add(vmu_image.blank(), name, vmu_image.build_package("BASLUSP01013", payload, b"\x01" * 1024))
    assert vmu_image.ps1_file(img, "BASLUSP01013") == payload[:0x400] + bytes(0xC000 - 0x400)
    assert vmu_image.ps1_file(img, "BASLUS-01013LOM00") is None


def test_the_pet_save_keeps_only_its_prefix_uncompressed():
    payload = bytes((i * 7) & 0xFF for i in range(0xC000))
    package = vmu_image.build_package("BASLUSP01013", payload, b"\x01" * 1024)
    data_len, raw_len = struct.unpack_from("<II", package, 0x48)
    assert (data_len, raw_len) == (0x400, 0xC000)
    assert package[128 + 1024:128 + 1024 + 0x400] == payload[:0x400]
    assert (len(package) + 511) // 512 <= 5
    name = vmu_image.vmu_name_for_ps1("BASLUSP01013")
    img = vmu_image.add(vmu_image.blank(), name, package)
    changed = bytearray(payload)
    changed[0x300:0x3B8] = bytes(0xB8)
    out = vmu_image.set_ps1_file(img, "BASLUSP01013", bytes(changed))
    assert vmu_image.ps1_file(out, "BASLUSP01013")[:0x400] == bytes(changed[:0x400])
    assert vmu_image.free_blocks(out) == vmu_image.free_blocks(img)


def test_build_package_compresses_and_keeps_incompressible_data_raw():
    save = bytearray(0x4000)
    save[:4] = b"SC\x11\x01"
    save[0x600:0x700] = bytes(range(256))
    package = vmu_image.build_package("BASLUS-01013LOM00", bytes(save))
    data_len, raw_len = struct.unpack_from("<II", package, 0x48)
    assert raw_len == 0x4000 and data_len < 0x400
    assert vmu_image.package_payload(package) == bytes(save)
    noise = random.Random(7).randbytes(0x2000)
    package = vmu_image.build_package("BASLUS-01013LOM01", noise)
    assert struct.unpack_from("<II", package, 0x48) == (0x2000, 0x2000)
    assert vmu_image.package_payload(package) == noise


def test_save_unpack_refuses_truncated_and_out_of_range_streams():
    packed = vmu_image.save_pack(b"abcabcabcabc" * 50)
    assert vmu_image.save_unpack(packed, 600) == b"abcabcabcabc" * 50
    with pytest.raises(ValueError):
        vmu_image.save_unpack(packed[:-1], 600)
    with pytest.raises(ValueError):
        vmu_image.save_unpack(b"\x00\x00\x00", 4)


def test_vms_crc_is_crc16_xmodem():
    assert vmu_image.vms_crc(b"123456789") == 0x31C3


def test_set_ps1_file_recompresses_into_a_resized_chain_with_a_fresh_crc():
    payload = bytes(0x4000)
    name = vmu_image.vmu_name_for_ps1("BASLUS-01013LOM00")
    img = vmu_image.add(vmu_image.blank(), name, vmu_image.build_package("BASLUS-01013LOM00", payload))
    new = random.Random(9).randbytes(0x4000)
    out = vmu_image.set_ps1_file(img, "BASLUS-01013LOM00", new)
    assert vmu_image.ps1_file(out, "BASLUS-01013LOM00") == new
    data = dict(vmu_image.files(out))[name]
    icons, _, _, crc, data_len = struct.unpack_from("<HHHHI", data, 0x40)
    size = 128 + 512 * icons + data_len
    assert crc == vmu_image.vms_crc(data[:0x46] + b"\0\0" + data[0x48:size])
    assert vmu_image.free_blocks(out) == vmu_image.free_blocks(img) - (len(vmu_image.chain(out, name)) - len(vmu_image.chain(img, name)))
    back = vmu_image.set_ps1_file(out, "BASLUS-01013LOM00", payload)
    assert vmu_image.ps1_file(back, "BASLUS-01013LOM00") == payload
    assert vmu_image.free_blocks(back) == vmu_image.free_blocks(img)


def test_add_game_takes_blocks_from_zero_up_with_its_header_in_block_one():
    game = bytes(range(256)) * 6
    img = vmu_image.add(vmu_image.blank(), "SAVE", b"s" * 512)
    img = vmu_image.add_game(img, "RRLAND", game)
    assert vmu_image.files(img) == [("SAVE", b"s" * 512), ("RRLAND", game)]
    assert vmu_image.chain(img, "RRLAND") == [0, 1, 2]
    assert img[:len(game)] == game
    entry = img[253 * 512 + 32:253 * 512 + 64]
    assert entry[0] == 0xCC and struct.unpack_from("<HHH", entry, 0x18)[:2] == (3, 1)
    assert vmu_image.free_blocks(img) == 196
    with pytest.raises(ValueError):
        vmu_image.add_game(img, "SECOND", b"g")


def test_add_into_given_blocks_chains_them_in_that_order():
    img = vmu_image.add(vmu_image.blank(), "GAPS", b"g" * 1024, blocks=[199, 197])
    assert vmu_image.chain(img, "GAPS") == [199, 197]
    assert fat(img)[198] == 0xFFFC
    with pytest.raises(ValueError):
        vmu_image.add(img, "TAKEN", b"t", blocks=[199])
