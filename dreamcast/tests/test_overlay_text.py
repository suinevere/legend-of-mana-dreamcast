#----------------------
# | test_overlay_text.py
# | Description: The Dreamcast converter gives CARDA, CLOAD, NIKI and ADDHERO a message table whose empty PocketStation entries hold the English lines.
# | Author: suinevere
# | Dependencies: port/tools/overlay_rules.py, port/tools/overlay_text.py, port/tools/targets.py, tools/overlays/text_table.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(REPO, "port", "tools"))
sys.path.insert(0, REPO)

import convert_disc  # noqa: E402
import overlay_rules  # noqa: E402
import overlay_text  # noqa: E402
import targets  # noqa: E402
from tools.overlays import text_table  # noqa: E402

US = text_table.TWO_BYTE_CODES["us"]


#----------------------
# | test_encode_overlay_and_scene_text
# | Description: Overlay lines are ASCII plus a terminator; scene lines turn brace controls into their codes and take no extra terminator.
# | Author: suinevere
# | Dependencies: overlay_text.encode_overlay, overlay_text.encode_scene
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
def test_encode_overlay_and_scene_text():
    assert overlay_text.encode_overlay("No VMU.") == b"No VMU.\x00"
    assert overlay_text.encode_scene("{clear}Feed{finish}") == bytes.fromhex("04 46 65 65 64 06")
    assert overlay_text.encode_scene("{color:1}A!{default_color}{newline}B.{end}") == bytes.fromhex("10 01 41 21 11 01 42 2e 00")
    with pytest.raises(ValueError):
        overlay_text.encode_overlay("café")
    with pytest.raises(ValueError):
        overlay_text.encode_scene("{bogus}")


#----------------------
# | test_scene_controls_match_the_scene_reader
# | Description: overlay_text's copy of the control table equals tools/scenes/strings.py's.
# | Author: suinevere
# | Dependencies: overlay_text.CONTROLS, tools.scenes.strings
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
def test_scene_controls_match_the_scene_reader():
    strings = pytest.importorskip("tools.scenes.strings")
    assert overlay_text.CONTROLS == strings.CONTROLS
    assert overlay_text.ARGUMENT_CONTROLS == strings.ARGUMENT_CONTROLS


#----------------------
# | test_dreamcast_overlay_message_tables_hold_the_english_lines
# | Description: Each converted overlay's text symbols point into a rebuilt table: the filled entries decode to the English, the rest are unchanged.
# | Author: suinevere
# | Dependencies: overlay_rules.build_overlay, overlay_text.load_lines, text_table.parse
# | Globals: US
# | Params: name, index -- overlay name and CD resource index
# | Returns: N/A
#----------------------
def test_swap_text_replaces_one_same_length_string():
    swaps = ((b"\x10\x02Vibration\x11", b"\x10\x02Use Stick\x11"),)
    buf = bytearray(b"\x00\x10\x02Vibration\x11\x00ON\x00")
    overlay_rules.swap_text(buf, swaps)
    assert buf == bytearray(b"\x00\x10\x02Use Stick\x11\x00ON\x00")
    with pytest.raises(ValueError):
        overlay_rules.swap_text(bytearray(b"\x10\x02Vibration\x11" * 2), swaps)
    with pytest.raises(ValueError):
        overlay_rules.swap_text(bytearray(b"abc"), ((b"abc", b"abcd"),))


def test_dreamcast_menu_keeps_its_vibration_option():
    raw = os.path.join(REPO, "build", "us", "overlays", "menu", "menu.raw")
    if not os.path.exists(raw):
        pytest.skip("PS1 overlay not built")
    with open(raw, "rb") as f:
        overlay = f.read()
    spec = overlay_rules.OVERLAYS[6]
    symbols = convert_disc.load_spec_symbols(spec, os.path.join(REPO, "config", "us", "symbols"))
    buf, _ = overlay_rules.build_overlay(spec, overlay, symbols, targets.DREAMCAST)
    assert b"\x10\x02Vibration\x11\x00ON\x00OFF\x00" in buf


@pytest.mark.parametrize("name,index", [("carda", 12), ("cload", 16), ("niki", 17), ("addhero", 18)])
def test_dreamcast_overlay_message_tables_hold_the_english_lines(name, index):
    raw = os.path.join(REPO, "build", "us", "overlays", name, name + ".raw")
    if not os.path.exists(raw):
        pytest.skip("PS1 overlay not built")
    with open(raw, "rb") as f:
        overlay = f.read()
    spec = overlay_rules.OVERLAYS[index]
    symbols = convert_disc.load_spec_symbols(spec, os.path.join(REPO, "config", "us", "symbols"))
    table_addr = overlay_rules.TEXT_TABLES[name]
    old = text_table.parse(overlay, table_addr - spec.vram, US)
    buf, script = overlay_rules.build_overlay(spec, overlay, symbols, targets.DREAMCAST)
    base = overlay_rules.data_base(overlay_rules.target_spec(spec, targets.DREAMCAST), targets.DREAMCAST)
    placed = {m.group(1): int(m.group(2), 16) for m in re.finditer(r"^_(\w+) = 0x([0-9A-F]+);", script, re.M)}
    inside = [s for s, a in symbols.items() if table_addr <= a < table_addr + old.size]
    assert inside
    first = min(inside, key=lambda s: symbols[s])
    new_off = placed[first] - base - (symbols[first] - table_addr)
    assert all(placed[s] - base - new_off == symbols[s] - table_addr for s in inside)
    new = text_table.parse(bytes(buf), new_off, US)
    lines = overlay_text.load_lines()[name]
    assert len(new.entries) == len(old.entries)
    for before, after in zip(old.entries, new.entries):
        if before.index in overlay_rules.TEXT_REPLACEMENTS:
            expected, text = overlay_rules.TEXT_REPLACEMENTS[before.index]
            assert before.data == expected and after.text == text
        elif before.index in lines:
            assert before.data == b"" and after.text == lines[before.index]
        else:
            assert after.data == before.data
