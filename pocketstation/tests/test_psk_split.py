"""----------------------
 | test_psk_split.py
 | Description: Tests the Capstone function splitter on RES1506 and on a hand-encoded four-function ARM/Thumb blob.
 | Author: suinevere
 | Dependencies: pocketstation/tools/psk_split.py, pocketstation/tools/psk_file.py, capstone, pytest
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import glob
import os
import struct
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
import psk_file  # noqa: E402
import psk_split  # noqa: E402

RES = os.path.join(REPO, psk_file.RES_PATH)
COVERAGE = glob.glob(os.path.join(REPO, "pocketstation", "build", "coverage", "*.txt"))
needs_res = pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted")


def load_coverage():
    cov = set()
    for path in COVERAGE:
        with open(path) as f:
            cov.update(int(x, 16) for x in f.read().split())
    return cov


@pytest.fixture(scope="module")
def real():
    with open(RES, "rb") as f:
        psk = psk_file.parse(f.read())
    return psk, psk_split.split(psk, load_coverage())


def owner(split, addr):
    for f in split.functions.values():
        if f.start <= addr < f.end:
            return f
    return None


@needs_res
def test_entry_is_an_arm_function(real):
    _, s = real
    f = s.functions[0x020027A8]
    assert not f.thumb


@needs_res
def test_bl_targets_are_function_starts(real):
    _, s = real
    for f in s.functions.values():
        for c in f.calls:
            assert (c & ~1) in s.functions, "%08X calls %08X" % (f.start, c)


@needs_res
def test_functions_do_not_overlap(real):
    _, s = real
    spans = sorted((f.start, f.end) for f in s.functions.values())
    for (a0, a1), (b0, _) in zip(spans, spans[1:]):
        assert a0 < a1 <= b0


@needs_res
def test_thumb_functions_are_reached_with_thumb_addresses(real):
    _, s = real
    for f in s.functions.values():
        for c in f.calls:
            assert bool(c & 1) == s.functions[c & ~1].thumb, "%08X -> %08X" % (f.start, c)


@needs_res
def test_body_is_partitioned(real):
    psk, s = real
    spans = [(f.start, f.end) for f in s.functions.values()] + list(s.data) + list(s.unknown)
    spans.sort()
    base = psk_file.FLASH1_BASE
    assert spans[0][0] == base + psk.body_offset
    assert spans[-1][1] == base + len(psk.data)
    for (a0, a1), (b0, _) in zip(spans, spans[1:]):
        assert a1 == b0
    unknown = sum(b - a for a, b in s.unknown)
    assert unknown < 0.02 * (len(psk.data) - psk.body_offset), unknown


@needs_res
@pytest.mark.skipif(not COVERAGE, reason="no coverage files yet")
def test_every_covered_address_is_in_a_function(real):
    _, s = real
    missing = [a for a in sorted(load_coverage()) if owner(s, a & ~1) is None]
    assert not missing, ["%08X" % a for a in missing[:20]]


@needs_res
def test_leaves_first_orders_callees_before_callers(real):
    _, s = real
    order = psk_split.leaves_first(s)
    assert sorted(order) == sorted(s.functions)
    pos = {a: i for i, a in enumerate(order)}
    groups = psk_split.cycle_groups(s)
    for f in s.functions.values():
        for c in f.calls:
            c &= ~1
            if groups[c] != groups[f.start]:
                assert pos[c] < pos[f.start], "%08X before %08X" % (c, f.start)


@needs_res
def test_fallthrough_into_the_next_function_is_a_call(real):
    _, s = real
    f = s.functions[0x02003D42]
    assert f.calls == [0x02003D44 | 1]


@needs_res
@pytest.mark.skipif(not COVERAGE, reason="no coverage files yet")
def test_a_callback_after_a_return_is_its_own_function(real):
    _, s = real
    assert s.functions[0x02003E48].end == 0x02003E70
    assert not s.functions[0x02003E70].thumb


def arm_bl(at, target):
    return 0xEB000000 | (((target - (at + 8)) >> 2) & 0xFFFFFF)


def thumb_bl(at, target):
    off = target - (at + 4)
    return struct.pack("<HH", 0xF000 | ((off >> 12) & 0x7FF), 0xF800 | ((off >> 1) & 0x7FF))


def test_synthetic_arm_thumb_blob():
    body = 0x80
    base = psk_file.FLASH1_BASE + body
    blob = b"".join(struct.pack("<I", w) for w in (
        0xE52DE004, arm_bl(0x04, 0x20), 0xE59FC008, 0xE1A0E00F, 0xE12FFF1C, 0xE49DF004,
        base + 0x28 + 1, 0,
        0xE3A00001, 0xE12FFF1E,
    ))
    blob += struct.pack("<H", 0xB500) + thumb_bl(0x2A, 0x30) + struct.pack("<H", 0xBD00)
    blob += struct.pack("<HH", 0x2001, 0x4770)
    data = bytes(body) + blob
    psk = psk_file.PskFile(blocks=1, title="", ident="MCX0", viewer_frames=0, icon_list=0, func_count=0,
                           entry=base, body_offset=body, data=data)
    s = psk_split.split(psk)
    assert {a - base: f.thumb for a, f in s.functions.items()} == {0x00: False, 0x20: False, 0x28: True, 0x30: True}
    assert s.functions[base].end == base + 0x18
    assert (base + 0x18, base + 0x1C) in s.data
    assert s.functions[base + 0x28].calls == [base + 0x30 + 1]
    assert s.functions[base + 0x30].callers == [base + 0x28]
