#----------------------
# | test_targets.py
# | Description: Checks the Dreamcast converter target: regions inside the arena, pointer relocation and pass-through, and no byte swapping.
# | Author: suinevere
# | Dependencies: tools/targets.py, tools/overlay_rules.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(PORT, "tools"))

import overlay_rules
import targets


def spec_named(name):
    return next(s for s in overlay_rules.OVERLAYS.values() if s.name == name)


def test_regions_fit_arena_and_do_not_overlap():
    spans = []
    for region in targets.DREAMCAST.regions.values():
        spans.append((region.code_base, region.code_base + region.code_size))
        spans.append((region.data_base, region.data_base + region.data_size))
    spans.sort()
    for start, end in spans:
        assert targets.ARENA_BASE <= start < end <= targets.ARENA_END
    for (_, end), (start, _) in zip(spans, spans[1:]):
        assert end <= start


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
