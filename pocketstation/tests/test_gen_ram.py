"""----------------------
 | test_gen_ram.py
 | Description: Tests the RAM map generator (accessors at both address spaces, overlap checks, field list), the VMU packer and the call graph tool.
 | Author: suinevere
 | Dependencies: pocketstation/tools/gen_ram.py, pocketstation/tools/pack_ram.py, pocketstation/tools/callgraph.py
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
import pack_ram  # noqa: E402

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
    assert fields[0] == {"name": "seed", "type": "u16", "count": 1, "ps": 0x3C4, "vmu": 0x010, "vmu_type": "u16",
                         "conv": None}


VMU_MAP = """
0x3C4 u16 seed 0x010 random seed
0x3C6 u8 btn - kernel-only
0x3CC u8 idle_lo 0x020 low byte
0x3CD u8 repeat_up + high byte
0x208 u32[13] irq_handler 0x030:u8:handler slots
0x5C4 u32 snd_start 0x040:u16:file sample start
0x464 u8[32] board_list 0x100 board
- u8[32] stack 0x3E0 C stack
"""


def test_vmu_accessors_use_vmu_offsets_through_vmu_r8(tmp_path):
    out = gen(tmp_path, "vmu", VMU_MAP)
    assert '#include "vmu_hw.h"' in out and "psk_" not in out
    assert "static inline uint16_t ram_seed(void) { return vmu_r16(0x010u); }" in out
    assert "static inline void set_seed(uint16_t v) { vmu_w16(0x010u, v); }" in out
    assert "static inline uint8_t ram_board_list(uint8_t i) { return vmu_r8(0x100u + VMU_IX(\"board_list\", i, 32u)); }" in out
    assert "#define RAM_BOARD_LIST 0x100u" in out


def test_vmu_glue_places_a_field_right_after_the_one_before(tmp_path):
    out = gen(tmp_path, "vmu", VMU_MAP)
    assert "static inline uint8_t ram_repeat_up(void) { return vmu_r8(0x021u); }" in out


def test_vmu_type_override_and_dropped_fields(tmp_path):
    out = gen(tmp_path, "vmu", VMU_MAP)
    assert "static inline uint8_t ram_irq_handler(uint8_t i) { return vmu_r8(0x030u + VMU_IX(\"irq_handler\", i, 13u)); }" in out
    assert "static inline uint16_t ram_snd_start(void) { return vmu_r16(0x040u); }" in out
    assert "ram_btn(" not in out and "ram_stack" not in out


def test_vmu_fields_json_carries_type_and_conversion(tmp_path):
    fields = {f["name"]: f for f in json.loads(gen(tmp_path, "fields", VMU_MAP))}
    assert fields["irq_handler"]["vmu_type"] == "u8" and fields["irq_handler"]["conv"] == "handler"
    assert fields["repeat_up"]["vmu"] == 0x21 and fields["btn"]["vmu"] is None


@pytest.mark.parametrize("text", [
    "0x200 u16 a 0x010 x\n0x300 u8 b 0x011 y\n",
    "0x200 u8[8] a 0x3FC x\n",
    "0x200 u8 a + x\n",
    "0x200 u8 a - x\n0x201 u8 b + y\n",
])
def test_vmu_overlap_overflow_and_dangling_glue_are_rejected(tmp_path, text):
    with pytest.raises(SystemExit):
        gen(tmp_path, "vmu", text)


PACK_MAP = """0x200 u8[2] arr 0x000 a
0x202 u8 cold 0x002 x
0x203 u16 warm 0x003 y
0x205 u8 warm_hi + y high
0x206 u8 gone - z
- u8[4] stack 0x006 s
"""


def test_pack_keeps_glued_runs_whole_and_puts_the_hottest_scalars_first():
    fields = {f.name: f for f in gen_ram.parse(pack_ram.pack(PACK_MAP, None))}
    assert [fields[n].vmu for n in ("cold", "warm", "warm_hi", "arr", "stack")] == [0, 1, 3, 4, 6]
    counts = [0] * 0x400
    counts[0] = counts[1] = 900
    counts[3] = counts[4] = counts[5] = 50
    fields = {f.name: f for f in gen_ram.parse(pack_ram.pack(PACK_MAP, counts))}
    assert [fields[n].vmu for n in ("warm", "warm_hi", "cold", "arr", "stack")] == [0, 2, 3, 4, 6]
    assert fields["gone"].vmu is None


def test_pack_keeps_the_runtime_block_first_and_gives_it_no_accessors(tmp_path):
    text = PACK_MAP + "- u8[32] rt_reserved 0x000 r\n"
    counts = [1000] * 0x400
    fields = {f.name: f for f in gen_ram.parse(pack_ram.pack(text, counts))}
    assert fields["rt_reserved"].vmu == 0
    assert min(f.vmu for n, f in fields.items() if n != "rt_reserved" and f.vmu is not None) == 32
    assert "rt_reserved" not in gen(tmp_path, "vmu", pack_ram.pack(text, None))


def test_pack_pins_the_runtime_input_block_at_0f9(tmp_path):
    text = ("0x200 u8[200] big 0x000 b\n0x2C8 u8[20] tail 0x0C8 t\n0x2DC u8 last 0x0DC l\n"
            "- u8[32] rt_reserved 0x000 r\n- u8[7] rt_input 0x000 i\n- u8[4] stack 0x200 s\n")
    fields = {f.name: f for f in gen_ram.parse(pack_ram.pack(text, [1000] * 0x400))}
    assert fields["rt_input"].vmu == 0x0F9
    assert [fields[n].vmu for n in ("last", "big", "tail")] == [0x020, 0x021, 0x100]
    assert all(f.vmu + f.vmu_size <= 0x0F9 or f.vmu >= 0x100 for n, f in fields.items() if n != "rt_input")
    assert "rt_input" not in gen(tmp_path, "vmu", pack_ram.pack(text, None))


def test_pack_pins_the_save_block_list_at_the_end_of_wram(tmp_path):
    text = ("0x200 u8[200] big 0x000 b\n0x2C8 u8[464] wide 0x100 w\n0x498 u8[32] last 0x2D0 l\n"
            "- u8[32] rt_reserved 0x000 r\n- u8[24] rt_save 0x000 s\n- u8[32] stack 0x300 k\n")
    fields = {f.name: f for f in gen_ram.parse(pack_ram.pack(text, None))}
    assert (fields["rt_save"].vmu, fields["rt_save"].vmu_size) == (0x2E8, 24)
    assert fields["last"].vmu >= 0x300 and fields["stack"].vmu >= 0x300
    assert all(f.vmu + f.vmu_size <= 0x2E8 or f.vmu >= 0x300 for n, f in fields.items() if n != "rt_save")
    assert "rt_save" not in gen(tmp_path, "vmu", pack_ram.pack(text, None))


def test_the_repository_map_reserves_the_runtime_block():
    with open(os.path.join(REPO, "pocketstation", "src", "game", "ram_map.txt")) as f:
        fields = gen_ram.parse(f.read())
    rt = next(f for f in fields if f.name == "rt_reserved")
    assert (rt.ps, rt.vmu, rt.vmu_size) == (None, 0, 32)
    assert all(f.vmu >= 32 for f in fields if f.vmu is not None and f is not rt)
    inp = next(f for f in fields if f.name == "rt_input")
    assert (inp.ps, inp.vmu, inp.vmu_size) == (None, 0x0F9, 3)
    skip = next(f for f in fields if f.name == "draw_skip")
    assert (skip.ps, skip.vmu, skip.vmu_size) == (None, 0x0FC, 1)
    assert all(f.vmu + f.vmu_size <= 0x0F9 or f.vmu >= 0x100 for f in fields if f.vmu is not None and f not in (inp, skip))
    save = next(f for f in fields if f.name == "rt_save")
    assert (save.ps, save.vmu, save.vmu_size) == (None, 0x2E8, 24)
    assert all(f.vmu + f.vmu_size <= 0x2E8 or f.vmu >= 0x300 for f in fields if f.vmu is not None and f is not save)


def test_the_repository_map_generates():
    assert gen_ram.main(["ps", os.path.join(REPO, "pocketstation", "src", "game", "ram_map.txt"), os.devnull]) == 0


def test_vmu_ram_fits():
    path = os.path.join(REPO, "pocketstation", "src", "game", "ram_map.txt")
    assert gen_ram.main(["vmu", path, os.devnull]) == 0
    with open(path) as f:
        fields = gen_ram.parse(f.read())
    placed = [f for f in fields if f.vmu is not None]
    assert max(f.vmu + f.vmu_size for f in placed) <= 0x400
    assert all(f.vmu is not None for f in fields if f.name not in ("free", "kernel_calls", "com_flags", "fb"))


def test_vmu_ram_fits_hardware(tmp_path):
    path = os.path.join(REPO, "pocketstation", "src", "game", "ram_map.txt")
    with open(path) as f:
        fields = gen_ram.parse(f.read())
    placed = [f for f in fields if f.vmu is not None and f.name != "stack"]
    assert max(f.vmu + f.vmu_size for f in placed) <= 0x300
    inc, js = tmp_path / "hw.inc", tmp_path / "hw.json"
    assert gen_ram.main(["hw", path, str(inc), str(js)]) == 0
    hw = json.loads(js.read_text())
    assert sorted(h["name"] for h in hw) == sorted(f.name for f in placed if f.name not in gen_ram.NO_ACCESS)
    text = inc.read_text()
    for h in hw:
        f = next(f for f in placed if f.name == h["name"])
        assert h["offset"] == f.vmu and h["size"] == f.vmu_size
        if f.vmu < 0x100:
            assert h["space"] == "bank1" and h["addr"] == f.vmu and f.vmu + f.vmu_size <= 0x100
            assert text.count("HW_%s equ 0x%03X\n" % (f.name.upper(), f.vmu)) == 1
        else:
            assert h["space"] == "wram" and h["addr"] == f.vmu - 0x100
            assert text.count("HWW_%s equ 0x%03X\n" % (f.name.upper(), f.vmu - 0x100)) == 1
    assert "STACK" not in text


def test_hw_rejects_a_field_across_the_bank_1_end(tmp_path):
    src = tmp_path / "map.txt"
    src.write_text("0x200 u16 a 0x0FF x\n")
    assert gen_ram.main(["hw", str(src), str(tmp_path / "hw.inc"), str(tmp_path / "hw.json")]) == 1


def test_pack_keeps_runs_out_of_the_bank_1_end():
    text = "".join("0x%03X u8 s%d 0x%03X x\n" % (0x200 + i, i, i) for i in range(0xFF)) + "0x300 u16 w 0x0FF y\n0x302 u8[4] a 0x101 z\n"
    fields = {f.name: f for f in gen_ram.parse(pack_ram.pack(text, None))}
    assert fields["w"].vmu + 2 <= 0x100 or fields["w"].vmu >= 0x100
    assert not (fields["a"].vmu < 0x100 < fields["a"].vmu + 4)


def test_fb_is_not_in_vmu_ram(tmp_path):
    out = gen(tmp_path, "vmu", open(os.path.join(REPO, "pocketstation", "src", "game", "ram_map.txt")).read())
    assert "ram_fb(" not in out and "set_fb(" not in out and "RAM_FB " not in out
    assert "ram_blit_a(" in out and "ram_blit_b(" in out


@pytest.mark.skipif(not os.path.exists(callgraph.CARD), reason="no reference card")
def test_call_levels_follow_calls_continuations_and_tables():
    levels = collections.Counter(level for level, _, _, _ in callgraph.levels())
    assert [levels[n] for n in range(14)] == [49, 48, 37, 21, 17, 10, 9, 7, 2, 3, 2, 4, 1, 1]


def doc_table_of(lang):
    with open(os.path.join(REPO, "docs", lang, "technical", "reference", "ring-ring-land-ram.md"), encoding="utf-8") as f:
        text = f.read()
    body = text.split("-->\n", 1)[1].split("<!-- end of ram table -->", 1)[0]
    return body.splitlines()


def test_reference_docs_carry_the_generated_ram_table():
    with open(os.path.join(REPO, "pocketstation", "src", "game", "ram_map.txt")) as f:
        table = gen_ram.doc_table(gen_ram.parse(f.read())).splitlines()
    assert doc_table_of("en") == table
    jp = doc_table_of("jp")
    assert len(jp) == len(table)
    assert [r.replace("バンク1", "bank 1").split(" | ")[:4] for r in jp[2:]] == [r.split(" | ")[:4] for r in table[2:]]
