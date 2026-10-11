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


def test_dreamcast_never_maps_into_a_saturn_slot():
    assert gp.resolve_aliases(["g_y"], ["g_y = 0x800CBF84;"], set(), gp.OVERLAY_SLOTS["dreamcast"],
                              None, gp.OVERLAY_BUFFER_RANGES["dreamcast"]) == {}


def test_dreamcast_sound_bank_range_covers_the_ps1_bank():
    aliases = gp.resolve_aliases(["g_bank_tail"], ["g_bank_tail = 0x8013F000;"], set(), gp.OVERLAY_SLOTS["dreamcast"],
                                 None, gp.OVERLAY_BUFFER_RANGES["dreamcast"])
    assert aliases == {"g_bank_tail": "_lom_sound_bank + 0x3000"}
    assert gp.resolve_aliases(["g_bank_tail"], ["g_bank_tail = 0x8013F000;"], set()) == {}


def test_saturn_render_is_unchanged():
    c_text, _ = gp.render(["foo"], [])
    assert 'saturn_halt("FIELD function foo not ported yet");' in c_text
    assert '#include "saturn_system.h"\n' in c_text
