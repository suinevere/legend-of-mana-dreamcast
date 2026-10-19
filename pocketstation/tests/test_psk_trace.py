"""----------------------
 | test_psk_trace.py
 | Description: Tests trace comparison on synthetic traces: equal, a frame change, a length change and a card change.
 | Author: suinevere
 | Dependencies: pocketstation/tools/psk_trace.py, pytest
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "tools"))
import psk_trace  # noqa: E402


def make(n=12, card=0x1234):
    t = psk_trace.Trace(card_crc=card)
    for i in range(n):
        t.samples.append(psk_trace.Sample(i, 0, bytes([i]) * 128, 0x1000 + i))
    return t


def test_equal_traces_have_no_divergence():
    assert psk_trace.compare(make(), make()) is None


def test_vram_change_reported_at_its_sample():
    b = make()
    b.samples[7].vram = b"\xaa" * 128
    d = psk_trace.compare(make(), b)
    assert (d.index, d.kind) == (7, "vram")


def test_ram_change_reported():
    b = make()
    b.samples[3].ram_crc ^= 1
    d = psk_trace.compare(make(), b)
    assert (d.index, d.kind) == (3, "ram")


def test_buttons_change_reported():
    b = make()
    b.samples[5].buttons = 1
    d = psk_trace.compare(make(), b)
    assert (d.index, d.kind) == (5, "buttons")


def test_length_change_reported():
    d = psk_trace.compare(make(12), make(10))
    assert (d.index, d.kind) == (10, "length")


def test_card_change_reported():
    d = psk_trace.compare(make(), make(card=0x9999))
    assert d.kind == "card"


def test_roundtrip_and_diff_cli(tmp_path, capsys):
    a, b = make(), make()
    b.samples[7].vram = b"\xff" * 128
    pa, pb = str(tmp_path / "a.pktr"), str(tmp_path / "b.pktr")
    psk_trace.write(a, pa)
    psk_trace.write(b, pb)
    assert psk_trace.read(pa) == a
    assert psk_trace.main(["diff", pa, pb]) == 1
    out = capsys.readouterr().out
    assert "sample 7: vram" in out
    assert "#" * 32 in out
    assert psk_trace.main(["diff", pa, pa]) == 0
