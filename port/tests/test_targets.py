#----------------------
# | test_targets.py
# | Description: Checks the Dreamcast converter target: regions inside the arena, pointer relocation and pass-through, and no byte swapping.
# | Author: suinevere
# | Dependencies: tools/targets.py, tools/overlay_rules.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import functools
import os
import struct
import sys

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(PORT, "tools"))

import overlay_rules
import targets


def spec_named(name):
    return next(s for s in overlay_rules.OVERLAYS.values() if s.name == name)


def test_regions_fit_arena_and_do_not_overlap():
    spans = []
    for region in set(targets.DREAMCAST.regions.values()):
        spans.append((region.code_base, region.code_base + region.code_size))
        spans.append((region.data_base, region.data_base + region.data_size))
    spans.sort()
    for start, end in spans:
        assert targets.ARENA_BASE <= start < end <= targets.ARENA_END
    for (_, end), (start, _) in zip(spans, spans[1:]):
        assert end <= start


def test_dreamcast_regions_fit_the_arena_without_overlap():
    spans = []
    for r, name in {r: name for name, r in targets.DREAMCAST.regions.items()}.items():
        spans.append((r.code_base, r.code_base + r.code_size, name))
        spans.append((r.data_base, r.data_base + r.data_size, name))
    spans.sort()
    for lo, hi, name in spans:
        assert targets.ARENA_BASE <= lo < hi <= targets.ARENA_END, name
    for (alo, ahi, a), (blo, bhi, b) in zip(spans, spans[1:]):
        assert ahi <= blo, (a, b)
    assert {"wmap", "field"} <= set(targets.DREAMCAST.regions)
    assert targets.ARENA_BASE == 0x8C480000


def test_secondary_overlays_share_one_region():
    for name in targets.SECONDARY_OVERLAYS:
        assert targets.DREAMCAST.regions[name] is targets.SECONDARY_REGION, name
    assert {"gname", "menu"} <= set(targets.SECONDARY_OVERLAYS)


def test_pointer_inside_ranges_relocates_to_data_region():
    spec = spec_named("title")
    first_start = spec.ranges[0][0]
    value = overlay_rules.relocate_pointer(first_start + 8, spec, targets.DREAMCAST)
    region = targets.DREAMCAST.regions["title"]
    assert value == region.data_base + overlay_rules.packed_offset(first_start + 8, spec)


def test_pointer_outside_ranges_passes_through():
    spec = spec_named("title")
    assert overlay_rules.relocate_pointer(0x80010000, spec, targets.DREAMCAST) == 0x80010000


def test_pointer_one_past_last_range_passes_through():
    spec = spec_named("title")
    end = spec.ranges[-1][1]
    assert overlay_rules.relocate_pointer(end, spec, targets.DREAMCAST) == end


def test_saturn_relocation_unchanged_by_default():
    spec = spec_named("title")
    first_start = spec.ranges[0][0]
    assert overlay_rules.relocate_pointer(first_start, spec) == overlay_rules.relocate_pointer(first_start, spec, targets.SATURN)


def test_dreamcast_skips_convert_rules():
    assert targets.DREAMCAST.convert_rules is False
    assert targets.DREAMCAST.big_endian is False


SYNTH_VRAM = 0x80100000
SYNTH_START = SYNTH_VRAM + 0x10
SYNTH_END = SYNTH_START + 0x40
SYNTH_SYMBOLS = {"synth_ptrs": SYNTH_START, "synth_tim": SYNTH_START + 0x10}


def synth_target(data_size):
    return targets.Target("synth_dc", False, {"synth": targets.Region(0x8C800000, 0x1000, 0x8C801000, data_size)}, False)


def synth_convert(buf, off):
    overlay_rules.swap_words(buf, 0, len(buf) // 4, 4)


def synth_overlay():
    spec = overlay_rules.OverlaySpec("synth", SYNTH_VRAM, [(SYNTH_START, SYNTH_END)], "synth.txt", ["synth_tim"], synth_convert)
    overlay = bytearray((i * 7 + 1) & 0xFF for i in range(SYNTH_END - SYNTH_VRAM))
    ptrs = SYNTH_START - SYNTH_VRAM
    overlay[ptrs:ptrs + 8] = (SYNTH_START + 0x30).to_bytes(4, "little") + (0x80010000).to_bytes(4, "little")
    tim = ptrs + 0x10
    overlay[tim:tim + 20] = bytes([0x10, 0, 0, 0, 2, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0])
    return spec, bytes(overlay)


def with_synth_pointer_rule(monkeypatch, spec):
    def rule(buf, off, target):
        overlay_rules.relocate_words(buf, [off("synth_ptrs"), off("synth_ptrs") + 4], spec, target)
    monkeypatch.setitem(overlay_rules.POINTER_RULES, "synth", rule)


def test_dreamcast_build_relocates_only_inside_pointer_and_skips_rules(monkeypatch):
    spec, overlay = synth_overlay()
    with_synth_pointer_rule(monkeypatch, spec)
    target = synth_target(0x40)
    raw = overlay[SYNTH_START - SYNTH_VRAM:SYNTH_END - SYNTH_VRAM]
    buf, script = overlay_rules.build_overlay(spec, overlay, SYNTH_SYMBOLS, target)
    changed = [i for i in range(len(raw)) if raw[i] != buf[i]]
    assert changed and all(0 <= i < 4 for i in changed)
    assert int.from_bytes(buf[0:4], "little") == 0x8C801000 + overlay_rules.packed_offset(SYNTH_START + 0x30, spec)
    assert buf[4:8] == raw[4:8]
    assert buf[0x10:] == raw[0x10:]
    assert "_synth_tim = 0x8C801010;" in script


def test_dreamcast_build_rejects_blob_over_data_size(monkeypatch):
    spec, overlay = synth_overlay()
    with_synth_pointer_rule(monkeypatch, spec)
    try:
        overlay_rules.build_overlay(spec, overlay, SYNTH_SYMBOLS, synth_target(0x3F))
    except ValueError as exc:
        assert "region holds 0x3F" in str(exc)
    else:
        raise AssertionError("expected an overflow ValueError")


def test_dreamcast_packs_wmap_whole_with_no_placed_buffers():
    spec = overlay_rules.target_spec(overlay_rules.OVERLAYS[3], targets.DREAMCAST)
    assert spec.ranges == [(0x8004FC74, 0x80054A2C), (0x800C4588, 0x801B32D8)]
    assert spec.placed == ()


def test_saturn_keeps_wmap_split_and_placed():
    spec = overlay_rules.target_spec(overlay_rules.OVERLAYS[3], targets.SATURN)
    assert spec is overlay_rules.OVERLAYS[3]
    assert spec.placed


def test_dreamcast_build_places_a_cut_buffer_at_its_ps1_offset():
    spec, overlay = synth_overlay()
    split = spec._replace(ranges=overlay_rules.split_ranges(spec.ranges, [(SYNTH_START + 0x20, SYNTH_START + 0x30)]),
                          placed=(("synth_buf", 0x002F0000),), unsplit_ranges=spec.ranges)
    symbols = dict(SYNTH_SYMBOLS, synth_buf=SYNTH_START + 0x20, synth_after=SYNTH_START + 0x30)
    buf, script = overlay_rules.build_overlay(split, overlay, symbols, synth_target(0x40))
    assert len(buf) == 0x40
    assert "_synth_buf = 0x8C801020;" in script
    assert "_synth_after = 0x8C801030;" in script
    assert "0x002F0000" not in script


def test_dreamcast_wmap_relocates_its_input_script_pointers():
    import convert_disc
    spec = overlay_rules.target_spec(overlay_rules.OVERLAYS[3], targets.DREAMCAST)
    symbols = convert_disc.load_spec_symbols(spec, os.path.join(PORT, "..", "config", "us", "symbols"))
    off = functools.partial(overlay_rules.symbol_offset, symbols, spec)
    buf = bytearray(sum(end - start for start, end in spec.ranges))
    scripts = off("g_wmap_input_scripts")
    struct.pack_into("<3I", buf, scripts, symbols["D_800D0460"], symbols["D_800D0460"] + 4, 0)
    overlay_rules.POINTER_RULES["wmap"](buf, off, targets.DREAMCAST)
    base = targets.DREAMCAST.regions["wmap"].data_base
    assert struct.unpack_from("<3I", buf, scripts) == (base + off("D_800D0460"), base + off("D_800D0460") + 4, 0)


def test_menu_pointer_rule_relocates_both_tables_and_keeps_the_script_count():
    spec = spec_named("menu")
    symbols = {"g_menu_content_table": 0x801686F8, "g_script_table": 0x80168778}
    overlay = bytearray(0x2B9DC)
    content = 0x801686F8 - spec.vram
    script = 0x80168778 - spec.vram
    struct.pack_into("<3I", overlay, content, 0x8014FF7C, 0, 0x80151A54)
    struct.pack_into("<3I", overlay, script, 1, 0x80150594, 0x80151104)
    buf, _ = overlay_rules.build_overlay(spec, overlay, symbols, targets.DREAMCAST)
    off = functools.partial(overlay_rules.symbol_offset, symbols, spec)
    base = targets.DREAMCAST.regions["menu"].data_base
    at = lambda addr: base + overlay_rules.packed_offset(addr, spec)
    assert struct.unpack_from("<3I", buf, off("g_menu_content_table")) == (at(0x8014FF7C), 0, at(0x80151A54))
    assert struct.unpack_from("<3I", buf, off("g_script_table")) == (1, at(0x80150594), at(0x80151104))


def test_movie_is_a_secondary_overlay_saturn_leaves_unlinked():
    assert "movie" in targets.SECONDARY_OVERLAYS
    assert targets.DREAMCAST.regions["movie"] is targets.SECONDARY_REGION
    assert "movie" in targets.SATURN_UNLINKED_OVERLAYS
