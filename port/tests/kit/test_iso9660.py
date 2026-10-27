#----------------------
# | test_iso9660.py
# | Description: Checks port/kit/iso9660.py against a synthetic ISO and, when built, dreamcast/gdi/track03.bin.
# | Author: suinevere
# | Dependencies: port/kit/iso9660.py, dreamcast/gdi/track03.bin (optional)
#----------------------
import os
import struct
import sys

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", ".."))
from port.kit import iso9660  # noqa: E402

BASE = 45000
TRACK03 = os.path.join(os.path.dirname(__file__), "..", "..", "..", "dreamcast", "gdi", "track03.bin")


def _record(name, extent, size, is_dir):
    ident = name.encode("ascii")
    length = 33 + len(ident) + (1 - len(ident) % 2)
    rec = bytearray(length)
    rec[0] = length
    rec[2:10] = struct.pack("<I", extent) + struct.pack(">I", extent)
    rec[10:18] = struct.pack("<I", size) + struct.pack(">I", size)
    rec[25] = 2 if is_dir else 0
    rec[32] = len(ident)
    rec[33:33 + len(ident)] = ident
    return bytes(rec)


def _image():
    sectors = {}
    root = _record("\x00", BASE + 18, 2048, True) + _record("\x01", BASE + 18, 2048, True)
    root += _record("1ST_READ.BIN;1", BASE + 20, 5000, False) + _record("RES", BASE + 19, 2048, True)
    res = _record("\x00", BASE + 19, 2048, True) + _record("\x01", BASE + 18, 2048, True) + _record("A.BIN;1", BASE + 23, 10, False)
    pvd = bytearray(2048)
    pvd[0:6] = b"\x01CD001"
    pvd[156:156 + 34] = _record("\x00", BASE + 18, 2048, True)
    sectors[16] = bytes(pvd)
    sectors[18] = root.ljust(2048, b"\x00")
    sectors[19] = res.ljust(2048, b"\x00")
    return lambda i: sectors.get(i, bytes(2048))


def test_lists_nested_files():
    files = iso9660.list_files(_image(), BASE)
    assert files == {"1ST_READ.BIN": (20, 5000), "RES/A.BIN": (23, 10)}


def test_rejects_missing_pvd():
    with pytest.raises(ValueError):
        iso9660.list_files(lambda i: bytes(2048), BASE)


@pytest.mark.skipif(not os.path.exists(TRACK03), reason="no built GDI")
def test_real_track_has_boot_and_index():
    files = iso9660.list_files(iso9660.raw_reader(TRACK03), BASE)
    assert "1ST_READ.BIN" in files and "RESINDEX.BIN" in files
