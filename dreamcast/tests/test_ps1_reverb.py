#----------------------
# | test_ps1_reverb.py
# | Description: Checks ps1_reverb.py's Studio Large preset and work-area tables against SLUS_010.13, and its renderer's first arrival, linearity and silence handling.
# | Author: suinevere
# | Dependencies: ../../port/tools/ps1_reverb.py, ../../disc/us/SLUS_010.13
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.normpath(os.path.join(DC, ".."))
sys.path.insert(0, os.path.join(REPO, "port", "tools"))

import ps1_reverb  # noqa: E402

DISC = os.path.join(REPO, "disc", "us", "SLUS_010.13")


def _require_disc():
    if not os.path.exists(DISC):
        pytest.skip("no PS1 disc image")


def test_studio_large_preset_and_work_area():
    _require_disc()
    with open(DISC, "rb") as handle:
        exe = handle.read()
    presets = ps1_reverb.read_presets(exe)
    assert presets[4] == ps1_reverb.STUDIO_LARGE
    assert all(value == 0 for value in presets[0].values())
    starts = ps1_reverb.read_starts(exe)
    assert starts[4] == ps1_reverb.STUDIO_LARGE_START
    assert 0x80000 - ps1_reverb.STUDIO_LARGE_START * 8 == 0x6FE0


def test_first_arrival_is_the_shortest_write_to_comb_path():
    left = [8192] + [0] * 2000
    right = [0] * 2001
    out_l, _ = ps1_reverb.render(ps1_reverb.STUDIO_LARGE, left, right)
    first = next(i for i, v in enumerate(out_l) if v != 0)
    assert first == 948


def test_render_is_linear_below_the_clamp():
    ticks = 1000
    left_a = [8000] + [0] * (ticks - 1)
    left_b = [16000] + [0] * (ticks - 1)
    right = [0] * ticks
    out_a, _ = ps1_reverb.render(ps1_reverb.STUDIO_LARGE, left_a, right)
    out_b, _ = ps1_reverb.render(ps1_reverb.STUDIO_LARGE, left_b, right)
    for a, b in zip(out_a, out_b):
        assert abs(b - 2 * a) <= 4


def test_silence_in_is_silence_out():
    ticks = 200
    silence = [0] * ticks
    out_l, out_r = ps1_reverb.render(ps1_reverb.STUDIO_LARGE, silence, silence)
    assert all(v == 0 for v in out_l)
    assert all(v == 0 for v in out_r)


def test_zero_output_volume_is_silence():
    ticks = 200
    loud = [8192] * ticks
    out_l, out_r = ps1_reverb.render(ps1_reverb.STUDIO_LARGE, loud, loud, vlout=0, vrout=0)
    assert all(v == 0 for v in out_l)
    assert all(v == 0 for v in out_r)
