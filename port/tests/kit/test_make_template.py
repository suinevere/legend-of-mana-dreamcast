#----------------------
# | test_make_template.py
# | Description: Checks the release kit's hole planning on synthetic inputs and the literal-run file format.
# | Author: suinevere
# | Dependencies: port/kit/make_template.py, port/kit/manifest.py
#----------------------
import os
import sys

import pytest

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
sys.path.insert(0, ROOT)
from port.kit import make_template, manifest  # noqa: E402

BOOT = make_template.BOOT_VADDR


def test_plan_holes_cd_elf_akao_rrl():
    files = {"1ST_READ.BIN": (100, 0x1000), "RES/A.BIN": (200, 5), "VMU/RRL.VMS": (300, 600)}
    srcs = {"cd/RES/A.BIN": b"hello", "gen/main_data": b"\x01" * 8, "gen/libgte_tables": b"\x02" * 4,
            "gen/vmu_pet_screens": b"\x03" * 4, "gen/akao_data": b"\x04" * 16, "vmu/rrl_pages": b"P" * 256}
    syms = {"lom_main_data": (BOOT + 0x10, 8), "lom_libgte_tables": (BOOT + 0x20, 0),
            "lom_vmu_pet_screens": (BOOT + 0x30, 4), "lom_akao_data": (BOOT + 0x40, 16)}
    rrl = b"\x00" * 100 + b"P" * 256 + b"\x00" * 244
    holes = make_template.plan_holes(files, srcs, 100 * 2048, syms, [1, 2], rrl)
    by = {(h["source"], h["source_offset"]): h for h in holes}
    assert by[("cd/RES/A.BIN", 0)] == {"offset": 200 * 2048, "length": 5, "source": "cd/RES/A.BIN", "source_offset": 0}
    assert by[("gen/main_data", 0)]["offset"] == 100 * 2048 + 0x10
    assert by[("gen/libgte_tables", 0)]["length"] == 4
    assert by[("gen/akao_data", 0)]["length"] == 4
    assert by[("gen/akao_data", 12)] == {"offset": 100 * 2048 + 0x40 + 12, "length": 4, "source": "gen/akao_data", "source_offset": 12}
    assert ("gen/akao_data", 4) not in by
    assert by[("vmu/rrl_pages", 0)]["offset"] == 300 * 2048 + 100
    assert [h["offset"] for h in holes] == sorted(h["offset"] for h in holes)


def test_cd_source_missing_from_iso_is_error():
    with pytest.raises(ValueError, match="RES/B.BIN"):
        make_template.plan_holes({"1ST_READ.BIN": (1, 1)}, {"cd/RES/B.BIN": b"x"}, 2048, {}, [], b"")


def test_cd_size_mismatch_is_error():
    with pytest.raises(ValueError, match="RES/B.BIN"):
        make_template.plan_holes({"RES/B.BIN": (5, 3)}, {"cd/RES/B.BIN": b"xy"}, 2048, {}, [], b"")


def test_ambiguous_rrl_pages_is_error():
    files = {"1ST_READ.BIN": (1, 1), "VMU/RRL.VMS": (2, 512)}
    with pytest.raises(ValueError, match="RRL"):
        make_template.plan_holes(files, {"vmu/rrl_pages": b"P" * 256}, 2048, {}, [], b"P" * 512)


def test_literals_round_trip_with_raw_sector_markers(tmp_path):
    runs = [(0, b"a" * 2048), (-1 - 3, b"r" * 2352), (8192, b"b" * 5)]
    path = str(tmp_path / "t.lit")
    manifest.write_literals(path, runs)
    assert list(manifest.read_literals(path)) == runs


def test_literals_round_trip_large(tmp_path):
    runs = [(i * 2048, bytes([i % 251]) * 2048) for i in range(1500)]
    path = str(tmp_path / "t.lit")
    manifest.write_literals(path, runs)
    assert list(manifest.read_literals(path)) == runs
