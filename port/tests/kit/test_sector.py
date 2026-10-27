#----------------------
# | test_sector.py
# | Description: Checks port/kit/sector.py's Mode 1 encoder against a scalar ECM-style reference and, when built, against dreamcast/gdi/track03.bin.
# | Author: suinevere
# | Dependencies: port/kit/sector.py, dreamcast/gdi/track03.bin (optional)
#----------------------
import os
import random
import sys

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", ".."))
from port.kit import sector  # noqa: E402

TRACK03 = os.path.join(os.path.dirname(__file__), "..", "..", "..", "dreamcast", "gdi", "track03.bin")


def _ref_tables():
    f, b, e = [0] * 256, [0] * 256, [0] * 256
    for i in range(256):
        j = ((i << 1) ^ (0x11D if i & 0x80 else 0)) & 0xFF
        f[i] = j
        b[i ^ j] = i
        v = i
        for _ in range(8):
            v = (v >> 1) ^ (0xD8018001 if v & 1 else 0)
        e[i] = v
    return f, b, e


def _ref_block(src, major_count, minor_count, major_mult, minor_inc, f, b):
    size = major_count * minor_count
    out = bytearray(major_count * 2)
    for major in range(major_count):
        index = (major >> 1) * major_mult + (major & 1)
        a = c = 0
        for _ in range(minor_count):
            t = src[index]
            index += minor_inc
            if index >= size:
                index -= size
            a ^= t
            c ^= t
            a = f[a]
        a = b[f[a] ^ c]
        out[major] = a
        out[major + major_count] = a ^ c
    return bytes(out)


def _bcd(v):
    return ((v // 10) << 4) | (v % 10)


def _ref_encode(lba, user):
    f, b, e = _ref_tables()
    s = bytearray(2352)
    s[0:12] = b"\x00" + b"\xff" * 10 + b"\x00"
    x = lba + 150
    s[12:16] = bytes([_bcd(x // 4500), _bcd((x // 75) % 60), _bcd(x % 75), 1])
    s[16:2064] = user
    edc = 0
    for byte in s[0:0x810]:
        edc = e[(edc ^ byte) & 0xFF] ^ (edc >> 8)
    s[0x810:0x814] = edc.to_bytes(4, "little")
    s[0x81C:0x8C8] = _ref_block(s[0xC:], 86, 24, 2, 86, f, b)
    s[0x8C8:0x930] = _ref_block(s[0xC:], 52, 43, 86, 88, f, b)
    return bytes(s)


@pytest.mark.parametrize("lba", [0, 45000, 45016, 549149])
def test_matches_reference(lba):
    rng = random.Random(lba)
    user = bytes(rng.randrange(256) for _ in range(2048))
    assert sector.encode(lba, user) == _ref_encode(lba, user)


def test_zero_user_matches_reference():
    assert sector.encode(45000, bytes(2048)) == _ref_encode(45000, bytes(2048))


def test_user_data_slice():
    user = bytes(range(256)) * 8
    assert sector.user_data(sector.encode(45001, user)) == user


def test_rejects_short_user():
    with pytest.raises(ValueError):
        sector.encode(0, b"\x00" * 2047)


@pytest.mark.skipif(not os.path.exists(TRACK03), reason="no built GDI")
def test_round_trips_real_track03():
    with open(TRACK03, "rb") as handle:
        for i in (0, 16, 17, 1000):
            handle.seek(i * 2352)
            raw = handle.read(2352)
            assert sector.encode(45000 + i, sector.user_data(raw)) == raw
