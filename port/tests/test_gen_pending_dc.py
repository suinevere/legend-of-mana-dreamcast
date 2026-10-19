#----------------------
# | test_gen_pending_dc.py
# | Description: Checks gen_pending.py's Dreamcast run: dc_pending.c halts through dc_halt("pending: <name>"), data aliases the trap buffer, nothing maps to a Saturn slot and the sound bank range is the full PS1 bank.
# | Author: suinevere
# | Dependencies: tools/gen_pending.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(PORT, "tools"))

import gen_pending as gp  # noqa: E402


def test_dreamcast_render_halts_with_pending_and_the_name():
    c_text, ld_text = gp.render(["foo"], ["g_x"], overlay="dreamcast")
    assert " | dc_pending.c\n" in c_text
    assert '#include "dc_system.h"\n' in c_text
    assert 'dc_halt("pending: foo");' in c_text
    assert "saturn" not in c_text
    assert " | dc_pending.ld\n" in ld_text
    assert "_g_x = _lom_trap_buffer;\n" in ld_text


def dc_field_address(addr):
    spec = gp.overlay_rules.target_spec(gp.overlay_rules.OVERLAYS[2], gp.targets.DREAMCAST)
    return gp.targets.DREAMCAST.regions["field"].data_base + gp.overlay_rules.packed_offset(addr, spec)


def test_dreamcast_never_maps_into_a_saturn_slot():
    assert gp.resolve_aliases(["g_y"], ["g_y = 0x800CBF84;"], set(), gp.OVERLAY_SLOTS["dreamcast"],
                              None, gp.OVERLAY_BUFFER_RANGES["dreamcast"]) == {}


def test_field_slots_map_field_data_into_fields_dreamcast_region():
    lines = ["g_text_choice_glyph_offsets = 0x800EC3FA;", "g_lom_save_filename_prefix = 0x800ECF7C;"]
    aliases = gp.resolve_aliases(["g_text_choice_glyph_offsets", "g_lom_save_filename_prefix", "D_800ECF9C"], lines, set(),
                                 gp.DREAMCAST_FIELD_SLOTS, None, gp.OVERLAY_BUFFER_RANGES["dreamcast"])
    assert aliases == {"g_text_choice_glyph_offsets": "0x%08X" % dc_field_address(0x800EC3FA),
                       "g_lom_save_filename_prefix": "0x%08X" % dc_field_address(0x800ECF7C),
                       "D_800ECF9C": "0x%08X" % dc_field_address(0x800ECF9C)}
    assert aliases["D_800ECF9C"] != "0x%08X" % gp.slot_address(0x800ECF9C)


def test_field_slots_leave_names_outside_fields_data_unresolved():
    assert gp.resolve_aliases(["g_marker"], ["g_marker = 0x80051500;"], set(), gp.DREAMCAST_FIELD_SLOTS,
                              None, gp.OVERLAY_BUFFER_RANGES["dreamcast"]) == {}


def test_a_secondary_linked_to_field_reads_fields_text_tables_through_field(monkeypatch, tmp_path):
    nm = {"main.o": [], "field.o": ["8c470000 D _g_field_pixel_lookup_tables"],
          "carda.o": ["         U _g_text_choice_glyph_offsets"], "gname.o": ["         U _g_frame"]}
    monkeypatch.setattr(gp, "run_nm", lambda tool, objects: [line for o in objects for line in nm[os.path.basename(o)]])
    monkeypatch.setattr(gp, "code_range", lambda path: (0x80010000, 0x80020000))
    symbols = tmp_path / "s.txt"
    symbols.write_text("g_text_choice_glyph_offsets = 0x800EC3FA;\ng_frame = 0x800D1000;\n")
    for name in ("field", "carda", "gname"):
        (tmp_path / ("%s_symbols.ld" % name)).write_text("_g_field_pixel_lookup_tables = 0x8C470000;\n" if name == "field" else "")
    out_c, out_ld = tmp_path / "p.c", tmp_path / "p.ld"
    groups = []
    for name in ("field", "carda", "gname"):
        groups += ["--overlay-group", str(tmp_path / ("%s.o" % name)), str(tmp_path / ("%s_symbols.ld" % name))]
    assert gp.main(["--overlay", "dreamcast", "--objects", str(tmp_path / "main.o"), "--symbols", str(symbols),
                    "--out-c", str(out_c), "--out-ld", str(out_ld), "--yamls", "x.yaml", "--group-out-dir", str(tmp_path)]
                   + groups + ["--group-links", "carda:field"]) == 0
    field_ld = (tmp_path / "field_pending.ld").read_text()
    assert "_g_text_choice_glyph_offsets = 0x%08X;" % dc_field_address(0x800EC3FA) in field_ld
    assert "g_text_choice_glyph_offsets" not in out_ld.read_text()
    assert "_g_frame = _lom_trap_buffer;" in out_ld.read_text()


def test_dreamcast_sound_bank_range_covers_the_ps1_bank():
    aliases = gp.resolve_aliases(["g_bank_tail"], ["g_bank_tail = 0x8013F000;"], set(), gp.OVERLAY_SLOTS["dreamcast"],
                                 None, gp.OVERLAY_BUFFER_RANGES["dreamcast"])
    assert aliases == {"g_bank_tail": "_lom_sound_bank + 0x3000"}
    assert gp.resolve_aliases(["g_bank_tail"], ["g_bank_tail = 0x8013F000;"], set()) == {}


def test_saturn_render_is_unchanged():
    c_text, _ = gp.render(["foo"], [])
    assert 'saturn_halt("FIELD function foo not ported yet");' in c_text
    assert '#include "saturn_system.h"\n' in c_text


def test_a_function_inside_another_overlays_sized_data_stays_a_stub():
    lines = ["g_table = 0x800C3000; // size:0x1000 allow_addend:True",
             "field_rebuild = 0x800C3BB0;",
             "g_inside = 0x800C3104;"]
    aliases = gp.resolve_aliases(["field_rebuild", "g_inside"], lines, {"g_table"}, gp.OVERLAY_SLOTS["dreamcast"],
                                 None, gp.OVERLAY_BUFFER_RANGES["dreamcast"], code_ranges=[(0x800C3800, 0x800C3C00)])
    assert aliases == {"g_inside": "_g_table + 0x104"}


def run_groups(monkeypatch, tmp_path, nm, symbol_text, scripts, groups):
    monkeypatch.setattr(gp, "run_nm", lambda tool, objects: [line for o in objects for line in nm[os.path.basename(o)]])
    monkeypatch.setattr(gp, "code_range", lambda path: (0x80010000, 0x80020000))
    symbols = tmp_path / "s.txt"
    symbols.write_text(symbol_text)
    for name, text in scripts.items():
        (tmp_path / name).write_text(text)
    out_c, out_ld = tmp_path / "p.c", tmp_path / "p.ld"
    argv = ["--overlay", "dreamcast", "--objects", str(tmp_path / "main.o"), "--symbols", str(symbols),
            "--out-c", str(out_c), "--out-ld", str(out_ld), "--yamls", "x.yaml"]
    for group in groups:
        argv += ["--overlay-group"] + [str(tmp_path / p) for p in group]
    assert gp.main(argv) == 0
    return out_c.read_text(), out_ld.read_text()


def test_main_never_aliases_to_another_overlays_equal_address_code(monkeypatch, tmp_path):
    nm = {"main.o": ["         U _field_x"], "wmap.o": ["8c480010 T _wmap_y"]}
    symbol_text = "field_x = 0x800CB0D8; // type:func\nwmap_y = 0x800CB0D8; // type:func\n"
    c_text, ld_text = run_groups(monkeypatch, tmp_path, nm, symbol_text, {}, [["wmap.o"]])
    assert "_wmap_y" not in ld_text
    assert 'dc_halt("pending: field_x");' in c_text


def test_a_name_only_another_overlay_defines_still_gets_main_storage(monkeypatch, tmp_path):
    nm = {"main.o": ["         U _g_pending", "         U _field_text_y"],
          "wmap.o": ["8c480010 T _field_text_y", "         U _g_wmap_data"],
          "gname.o": ["         U _g_frame"]}
    symbol_text = ("g_pending = 0x8010D018;\nfield_text_y = 0x80012000; // type:func\n"
                   "g_wmap_data = 0x800D0000;\ng_frame = 0x800D1000;\n")
    scripts = {"wmap_symbols.ld": "_g_pending = 0x8C5D0000;\n_g_wmap_data = 0x8C4F0000;\n_g_frame = 0x8C4F1000;\n"}
    c_text, ld_text = run_groups(monkeypatch, tmp_path, nm, symbol_text, scripts,
                                 [["wmap.o", "wmap_symbols.ld"], ["gname.o"]])
    assert "_g_pending = _lom_trap_buffer;" in ld_text
    assert "_g_frame = _lom_trap_buffer;" in ld_text
    assert "g_wmap_data" not in ld_text
    assert 'dc_halt("pending: field_text_y");' in c_text


def test_an_overlays_own_unnamed_data_resolves_inside_its_data_region(monkeypatch, tmp_path):
    nm = {"main.o": [], "wmap.o": ["         U _D_8004FD04", "         U _D_800D0000"]}
    scripts = {"wmap_symbols.ld": "_g_other = 0x8C4E2000;\n"}
    argv_extra = ["--group-out-dir", str(tmp_path)]
    monkeypatch.setattr(gp, "run_nm", lambda tool, objects: [line for o in objects for line in nm[os.path.basename(o)]])
    monkeypatch.setattr(gp, "code_range", lambda path: (0x80010000, 0x80020000))
    symbols = tmp_path / "s.txt"
    symbols.write_text("")
    for name, text in scripts.items():
        (tmp_path / name).write_text(text)
    out_c, out_ld = tmp_path / "p.c", tmp_path / "p.ld"
    assert gp.main(["--overlay", "dreamcast", "--objects", str(tmp_path / "main.o"), "--symbols", str(symbols),
                    "--out-c", str(out_c), "--out-ld", str(out_ld), "--yamls", "x.yaml",
                    "--overlay-group", str(tmp_path / "wmap.o"), str(tmp_path / "wmap_symbols.ld")] + argv_extra) == 0
    spec = gp.overlay_rules.target_spec(gp.overlay_rules.OVERLAYS[3], gp.targets.DREAMCAST)
    base = gp.targets.DREAMCAST.regions["wmap"].data_base
    local = (tmp_path / "wmap_pending.ld").read_text()
    assert "_D_8004FD04 = 0x%08X;" % (base + gp.overlay_rules.packed_offset(0x8004FD04, spec)) in local
    assert "_D_800D0000 = 0x%08X;" % (base + gp.overlay_rules.packed_offset(0x800D0000, spec)) in local
    assert "D_8004FD04" not in out_ld.read_text()
