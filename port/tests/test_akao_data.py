#----------------------
# | test_akao_data.py
# | Description: Checks port/tools/akao_data.py: the AKAO data block comes from the user's SLUS with its pointer tables relocated to Dreamcast symbols, and gen_pending maps AKAO's PS1 addresses onto the data and BSS blocks.
# | Author: suinevere
# | Dependencies: ../tools/akao_data.py, ../tools/gen_pending.py, ../../disc/us/LegendOfMana.bin
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re
import sys

import pytest

TOOLS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
sys.path.insert(0, TOOLS)

import akao_data  # noqa: E402
import gen_pending  # noqa: E402


def _generated():
    image = os.environ.get("LOM_PS1_IMAGE", os.path.join(TOOLS, "..", "..", "disc", "us", "LegendOfMana.bin"))
    if not os.path.isfile(image):
        pytest.skip("no US disc image")
    return akao_data.generate(image)


def _word_at(text, address):
    index = (address - akao_data.DATA_START) // 4
    body = text[text.index("lom_akao_data[") :]
    entries = [e.strip() for e in body[body.index("{") + 1 : body.index("};")].split(",") if e.strip()]
    return entries[index]


def test_blocks_cover_akao_symbols():
    assert akao_data.DATA_START == 0x8003D160 and akao_data.DATA_END == 0x8003F000
    assert akao_data.BSS_START == 0x80049130 and akao_data.BSS_END == 0x8004FC70


def test_pitch_table_keeps_ps1_values():
    text = _generated()
    assert _word_at(text, 0x8003D24C) == "0x00001000"
    assert _word_at(text, 0x8003D250) == "0x000010F3"


def test_handler_tables_name_their_functions():
    text = _generated()
    assert _word_at(text, 0x8003E890) == "(unsigned int)akao_release_channels"
    assert _word_at(text, 0x8003E894) == "(unsigned int)akao_seq_op_set_mapped_articulation"
    assert "extern char akao_seq_op_set_pan[];" in text


def test_lfo_waveforms_point_into_the_block():
    text = _generated()
    assert _word_at(text, 0x8003DD80) == "(unsigned int)lom_akao_data + 0x%X" % (0x8003D57C - akao_data.DATA_START)


def test_bss_block_is_declared_zeroed():
    text = _generated()
    assert re.search(r"unsigned int lom_akao_bss\[0x%X\]" % ((akao_data.BSS_END - akao_data.BSS_START) // 4), text)


def test_gen_pending_maps_akao_addresses_to_the_blocks():
    ranges = gen_pending.DREAMCAST_BUFFER_RANGES
    assert (akao_data.DATA_START, akao_data.DATA_END - akao_data.DATA_START, "_lom_akao_data") in ranges
    assert (akao_data.BSS_START, akao_data.BSS_END - akao_data.BSS_START, "_lom_akao_bss") in ranges
    assert all(r[2] not in ("_lom_akao_data", "_lom_akao_bss") for r in gen_pending.BUFFER_RANGES)


def test_song_globals_outside_the_blocks_are_defined():
    text = _generated()
    assert "unsigned int g_current_song_handle;" in text
    assert re.search(r"unsigned char g_music_sequence\[0x400\]", text)
