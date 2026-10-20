"""----------------------
 | test_gen_ram.py
 | Description: Tests the RAM map generator (accessors, overlap check, field list) and the call graph tool.
 | Author: suinevere
 | Dependencies: pocketstation/tools/gen_ram.py, pocketstation/tools/callgraph.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import collections
import json
import os
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
import callgraph  # noqa: E402
import gen_ram  # noqa: E402

MAP = """
# test map
0x3C4 u16 seed 0x010 random seed stepped by game_random
0x484 u8[72] board 0x100 board squares
0x72C u8[212] stack - ARM stack
"""


def gen(tmp_path, mode, text):
    src = tmp_path / "map.txt"
    out = tmp_path / "out"
    src.write_text(text)
    rc = gen_ram.main([mode, str(src), str(out)])
    if rc:
        raise SystemExit(rc)
    return out.read_text()


def test_scalar_accessors_use_the_pocketstation_address(tmp_path):
    out = gen(tmp_path, "ps", MAP)
    assert "static inline uint16_t ram_seed(void) { return psk_r16(0x3C4u); }" in out
    assert "static inline void set_seed(uint16_t v) { psk_w16(0x3C4u, v); }" in out


def test_array_accessors_index_by_element_size(tmp_path):
    out = gen(tmp_path, "ps", MAP)
    assert "static inline uint8_t ram_board(uint8_t i) { return psk_r8(0x484u + i); }" in out


def test_stack_and_free_get_no_accessors(tmp_path):
    assert "ram_stack" not in gen(tmp_path, "ps", MAP)


def test_overlapping_fields_are_rejected(tmp_path):
    with pytest.raises(SystemExit):
        gen(tmp_path, "ps", "0x200 u16 a - x\n0x201 u8 b - y\n")


def test_fields_json_lists_both_addresses(tmp_path):
    fields = json.loads(gen(tmp_path, "fields", MAP))
    assert fields[0] == {"name": "seed", "type": "u16", "count": 1, "ps": 0x3C4, "vmu": 0x010}


def test_the_repository_map_generates():
    assert gen_ram.main(["ps", os.path.join(REPO, "pocketstation", "src", "game", "ram_map.txt"), os.devnull]) == 0


@pytest.mark.skipif(not os.path.exists(callgraph.CARD), reason="no reference card")
def test_call_levels_follow_calls_continuations_and_tables():
    levels = collections.Counter(level for level, _, _, _ in callgraph.levels())
    assert [levels[n] for n in range(14)] == [49, 48, 37, 21, 17, 10, 9, 7, 2, 3, 2, 4, 1, 1]
