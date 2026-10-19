#----------------------
# | test_main_data.py
# | Description: Checks the 20 symbols once aliased to the trap buffer now have storage: FIELD's two rodata bins packed into its Dreamcast data region, and the main executable's five symbols in gen/main_data.c copied from the SLUS (or zeroed past its data).
# | Author: suinevere
# | Dependencies: port/tools/main_data.py, port/tools/overlay_rules.py, port/tools/akao_data.py, dreamcast/gen/dc_pending.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re
import sys

import pytest

TOOLS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
REPO = os.path.normpath(os.path.join(TOOLS, "..", ".."))
sys.path.insert(0, TOOLS)

import akao_data  # noqa: E402
import main_data  # noqa: E402
import overlay_rules  # noqa: E402
import targets  # noqa: E402

FORMER_TRAP_SYMBOLS = ("D_80051888", "D_80051C50", "D_80051CBC", "D_80051CE4", "D_80051EB4", "D_80051EC0", "D_80051ECC",
                       "D_80051ED8", "D_80051EF8", "D_80051F04", "g_field_marker_overlay_enabled", "g_field_marker_u_offsets",
                       "g_field_marker_widths", "g_field_marker_x_offsets", "g_field_small_icon_frames", "g_game_diagnostic_arg0",
                       "g_game_diagnostic_arg1", "g_game_diagnostic_code", "g_game_diagnostic_status", "g_last_error_status")


def _address(name, by_name):
    return by_name[name] if name in by_name else int(name[-8:], 16)


def _field_dc_spec():
    return overlay_rules.target_spec(overlay_rules.OVERLAYS[2], targets.DREAMCAST)


def test_every_former_trap_symbol_has_storage():
    by_name, _ = akao_data.load_symbols()
    spans = {name: (addr, size) for name, addr, size in main_data.symbol_spans()}
    field = _field_dc_spec()
    for name in FORMER_TRAP_SYMBOLS:
        addr = _address(name, by_name)
        in_main = name in spans
        in_field = overlay_rules.packed_offset(addr, field) is not None
        assert in_main != in_field, name


def test_main_spans_are_sized_and_disjoint():
    spans = sorted((addr, size) for _, addr, size in main_data.symbol_spans())
    assert all(size > 0 for _, size in spans)
    assert all(a + s <= b for (a, s), (b, _) in zip(spans, spans[1:]))


def test_saturn_field_ranges_are_unchanged():
    saturn = overlay_rules.target_spec(overlay_rules.OVERLAYS[2], targets.SATURN)
    assert saturn.ranges == [(0x800CBF44, 0x80123FD8)]


def test_dreamcast_field_packs_both_rodata_bins_before_its_data():
    ranges = _field_dc_spec().ranges
    assert ranges[-1] == (0x800CBF44, 0x80123FD8)
    assert (0x800513E8, 0x8005146C) in ranges and (0x80051888, 0x80051F28) in ranges


def test_generated_block_copies_exe_data_and_zeroes_bss():
    exe_path = os.path.join(REPO, "disc", "us", "SLUS_010.13")
    if not os.path.isfile(exe_path):
        pytest.skip("no SLUS_010.13")
    with open(exe_path, "rb") as f:
        exe = f.read()
    text = main_data.generate_from_exe(exe)
    by_name, _ = akao_data.load_symbols()
    body = text[text.index("lom_main_data["):]
    data = bytes(int(v, 16) for v in re.findall(r"0x([0-9A-F]{2})\b", body[body.index("{"):body.index("};")]))
    for name, addr, size in main_data.symbol_spans():
        offset = main_data.block_offset(addr)
        got = data[offset:offset + size]
        if addr < main_data.exe_end(exe):
            want = exe[addr - 0x80010000 + 0x800:addr - 0x80010000 + 0x800 + size]
        else:
            want = bytes(size)
        assert got == want, name


def test_dc_pending_has_no_trap_buffer_alias():
    path = os.path.join(REPO, "dreamcast", "gen", "dc_pending.ld")
    if not os.path.isfile(path):
        pytest.skip("dreamcast/gen not generated")
    with open(path) as f:
        lines = [line for line in f if "= _lom_trap_buffer;" in line]
    assert lines == []
