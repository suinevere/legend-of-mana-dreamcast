"""----------------------
 | test_psk_lift.py
 | Description: Tests the raw-bit ARM/Thumb lifter on hand-encoded instructions and on every RES1506 function.
 | Author: suinevere
 | Dependencies: pocketstation/tools/psk_lift.py, pocketstation/tools/psk_file.py, pocketstation/tools/psk_split.py, pytest
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import os
import shutil
import struct
import subprocess
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
import psk_file  # noqa: E402
import psk_lift  # noqa: E402
import psk_split  # noqa: E402

RES = os.path.join(REPO, psk_file.RES_PATH)
BASE = 0x02000000


def arm(*words):
    return struct.pack("<%dI" % len(words), *words)


def thumb(*halves):
    return struct.pack("<%dH" % len(halves), *halves)


def lift(blob, is_thumb, start=0, end=None):
    return psk_lift.lift_function(blob, BASE, BASE + start, BASE + (len(blob) if end is None else end), is_thumb)


def body(text):
    return [l.strip() for l in text.split("{", 1)[1].splitlines()]


def test_arm_bx_lr_returns_through_lr():
    out = body(lift(arm(0xE12FFF1E), False))
    assert "c->r[15] = psk_bx(c->r[14]);" in out
    assert "return;" in out


def test_arm_pop_pc_loads_continuation_without_interworking():
    out = body(lift(arm(0xE8BD8010), False))
    assert "uint32_t a = c->r[13];" in out
    assert "c->r[4] = psk_r32(a);" in out
    assert "c->r[15] = psk_r32(a + 4u) & ~3u;" in out
    assert "c->r[13] = a + 8u;" in out
    assert "return;" in out


def test_thumb_pop_pc_stays_thumb():
    out = body(lift(thumb(0xBD10), True))
    assert "c->r[4] = psk_r32(a);" in out
    assert "c->r[15] = (psk_r32(a + 4u) & ~1u) | 1u;" in out
    assert "return;" in out


def test_arm_bl_is_a_call_returning_after_it():
    blob = arm(0xEB000001, 0xE12FFF1E, 0, 0)
    out = body(lift(blob, False, 0, 8))
    assert "psk_call(c, 0x0200000Cu, 0x02000004u);" in out


def test_thumb_bl_pair_is_one_call_with_thumb_bits():
    blob = thumb(0xF000, 0xF802, 0x4770, 0, 0, 0)
    out = body(lift(blob, True, 0, 6))
    assert "psk_call(c, 0x02000009u, 0x02000005u);" in out


def test_arm_literal_load_becomes_constant():
    blob = arm(0xE59F0000, 0xE12FFF1E, 0x12345678)
    out = body(lift(blob, False, 0, 8))
    assert "c->r[0] = 0x12345678u;" in out


def test_arm_condition_wraps_statement():
    out = "\n".join(body(lift(arm(0x13A00001, 0xE12FFF1E), False)))
    assert "if (psk_cond(c, PSK_NE)) {" in out
    assert "c->r[0] = 0x00000001u;" in out


def test_local_loop_gets_label_and_goto_and_outside_branch_exits():
    blob = arm(0xE2500001, 0x1AFFFFFD, 0xEA000010, 0)
    out = "\n".join(body(lift(blob, False, 0, 12)))
    assert "a_02000000:" in out
    assert "goto a_02000000;" in out
    assert "c->r[15] = 0x02000050u;" in out


def test_mov_lr_pc_then_bx_is_a_call():
    blob = arm(0xE1A0E00F, 0xE12FFF13, 0xE12FFF1E)
    out = body(lift(blob, False))
    assert "psk_call(c, psk_bx(c->r[3]), 0x02000008u);" in out


def test_thumb_flag_setting_add_and_compare():
    out = "\n".join(body(lift(thumb(0x3001, 0x2805, 0x4770), True)))
    assert "c->r[0] = psk_add_flags(c, c->r[0], 0x00000001u);" in out
    assert "psk_sub_flags(c, c->r[0], 0x00000005u);" in out


def test_header_block_and_signature():
    text = lift(arm(0xE12FFF1E), False)
    assert " | rrl_func_02000000\n" in text
    assert " | Author: suinevere\n" in text
    assert "void rrl_func_02000000(PskCpu *c) {" in text


@pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted")
def test_every_res1506_function_lifts_and_compiles(tmp_path):
    with open(RES, "rb") as f:
        psk = psk_file.parse(f.read())
    build = os.path.join(REPO, "pocketstation", "build")
    assert psk_split.main(["--res", RES, "--build", build]) == 0
    funcs = psk_lift.read_functions(os.path.join(build, "functions.txt"))
    assert len(funcs) == 210
    text = ['#include "rrl.h"']
    for start, end, is_thumb in funcs:
        text.append(psk_lift.lift_function(psk.data, BASE, start, end, is_thumb))
    if shutil.which("docker") is None:
        pytest.skip("docker not available")
    rel = "pocketstation/build/test/lift_all.c"
    with open(os.path.join(REPO, rel), "w", newline="\n") as f:
        f.write("\n".join(text))
    r = subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", "gcc", "-std=c11",
                        "-Wall", "-Wextra", "-Werror", "-fsyntax-only", "-Ipocketstation/src/psk",
                        "-Ipocketstation/src/rrl", rel], capture_output=True, text=True)
    assert r.returncode == 0, r.stderr[-3000:]


def test_idle_edge_idles_before_the_back_edge():
    blob = arm(0xE2500001, 0x1AFFFFFD, 0xE12FFF1E)
    text = psk_lift.lift_function(blob, BASE, BASE, BASE + 12, False, idle={BASE + 4})
    out = body(text)
    assert out.index("psk_idle();") + 1 == out.index("goto a_02000000;")


def test_frame_point_marks_the_frame():
    blob = bytes(psk_lift.FRAME_POINT - BASE) + arm(0xE12FFF1E)
    out = body(psk_lift.lift_function(blob, BASE, psk_lift.FRAME_POINT, psk_lift.FRAME_POINT + 4, False))
    assert [l for l in out if l][0] == "psk_frame();"
