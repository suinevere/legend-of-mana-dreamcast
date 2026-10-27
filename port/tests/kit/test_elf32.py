#----------------------
# | test_elf32.py
# | Description: Checks port/kit/elf32.py against a synthetic ELF and, when built, the symbols and segments of dreamcast/lom.elf.
# | Author: suinevere
# | Dependencies: port/kit/elf32.py, dreamcast/lom.elf (optional)
#----------------------
import os
import struct
import sys

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", ".."))
from port.kit import elf32  # noqa: E402

LOM_ELF = os.path.join(os.path.dirname(__file__), "..", "..", "..", "dreamcast", "lom.elf")


def _elf(path):
    strtab = b"\x00_lom_main_data\x00"
    shstr = b"\x00.symtab\x00.strtab\x00.shstrtab\x00"
    sym = bytes(16) + struct.pack("<IIIBBH", 1, 0x8C0656A0, 0x4C, 0x11, 0, 2)
    seg = b"\xAA" * 0x40
    ehsize, phsize, shsize = 52, 32, 40
    off_seg = ehsize + phsize
    off_sym = off_seg + len(seg)
    off_str = off_sym + len(sym)
    off_shs = off_str + len(strtab)
    off_sh = off_shs + len(shstr)
    eh = b"\x7fELF\x01\x01\x01" + bytes(9) + struct.pack("<HHIIIIIHHHHHH", 2, 42, 1, 0x8C010000, ehsize, off_sh, 0, ehsize, phsize, 1, shsize, 4, 3)
    ph = struct.pack("<IIIIIIII", 1, off_seg, 0x8C010000, 0x8C010000, len(seg), 0x80, 5, 0x80)
    sh = bytes(40)
    sh += struct.pack("<IIIIIIIIII", 1, 2, 0, 0, off_sym, len(sym), 2, 1, 4, 16)
    sh += struct.pack("<IIIIIIIIII", 9, 3, 0, 0, off_str, len(strtab), 0, 0, 1, 0)
    sh += struct.pack("<IIIIIIIIII", 17, 3, 0, 0, off_shs, len(shstr), 0, 0, 1, 0)
    with open(path, "wb") as handle:
        handle.write(eh + ph + seg + sym + strtab + shstr + sh)


def test_synthetic_symbols_and_segments(tmp_path):
    path = tmp_path / "t.elf"
    _elf(path)
    assert elf32.symbols(str(path))["lom_main_data"] == (0x8C0656A0, 0x4C)
    assert elf32.load_segments(str(path)) == [(0x8C010000, 0x80, b"\xAA" * 0x40)]


def test_rejects_non_elf(tmp_path):
    path = tmp_path / "x.bin"
    path.write_bytes(b"not an elf at all")
    with pytest.raises(ValueError):
        elf32.symbols(str(path))


@pytest.mark.skipif(not os.path.exists(LOM_ELF), reason="no built lom.elf")
def test_real_elf_hole_symbols():
    syms = elf32.symbols(LOM_ELF)
    assert syms["lom_akao_data"][1] == 0x1EA0
    assert syms["lom_main_data"][1] == 0x4C
    assert syms["lom_vmu_pet_screens"][1] == 0x39C0
    assert "lom_libgte_tables" in syms
    assert elf32.load_segments(LOM_ELF)[0][0] == 0x8C010000
