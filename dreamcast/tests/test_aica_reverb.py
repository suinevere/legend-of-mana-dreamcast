#----------------------
# | test_aica_reverb.py
# | Description: Checks dreamcast/tools/aica_reverb.py's Studio Large DSP program: its build assertions, the committed header, its impulse response against the PS1 reverb formula, its first arrival, bus routing and depth.
# | Author: suinevere
# | Dependencies: tools/aica_reverb.py, tools/aica_dsp.py, ../../port/tools/ps1_reverb.py, src/sound/aica_reverb_program.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import functools
import math
import os
import sys

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.normpath(os.path.join(DC, ".."))
sys.path.insert(0, os.path.join(DC, "tools"))
sys.path.insert(0, os.path.join(REPO, "port", "tools"))

import aica_dsp  # noqa: E402
import aica_reverb  # noqa: E402
import ps1_reverb  # noqa: E402

HEADER = os.path.join(DC, "src", "sound", "aica_reverb_program.h")


@functools.lru_cache(maxsize=None)
def _dc(seconds, bus, depth=0x7FFF, rate=2):
    return aica_reverb.impulse_response(seconds, bus, depth=depth, rate=rate)


@functools.lru_cache(maxsize=None)
def _ps1(seconds):
    ticks = int(round(seconds * 22050))
    left = [0] * ticks
    left[0] = 8192
    return ps1_reverb.render(ps1_reverb.STUDIO_LARGE, left, [0] * ticks)


def _bin_power(samples, rate, start_ms, end_ms, width_ms=10):
    bins = []
    for lo in range(start_ms, end_ms, width_ms):
        a = int(round(lo * rate / 1000.0))
        b = int(round((lo + width_ms) * rate / 1000.0))
        bins.append(sum(v * v for v in samples[a:b]) / float(b - a))
    return bins


def _db(power):
    return 10.0 * math.log10(power) if power > 0 else -300.0


def _percentile(values, fraction):
    ordered = sorted(values)
    index = min(len(ordered) - 1, int(math.ceil(fraction * len(ordered))) - 1)
    return ordered[max(0, index)]


def _compare(dc, ps1):
    result = []
    for side in (0, 1):
        ref = _bin_power(ps1[side], 22050, 20, 400)
        got = _bin_power(dc[side], 44100, 20, 400)
        peak = _db(max(ref))
        diffs = [abs(_db(g) - _db(r)) for g, r in zip(got, ref) if r > 1.0 and _db(r) >= peak - 60.0]
        assert len(diffs) > 10
        result.append((_percentile(diffs, 0.5), _percentile(diffs, 0.9)))
    return result


def _energy(samples):
    return sum(v * v for v in samples)


def test_build_passes_its_assertions_and_fills_66_steps():
    steps, madrs = aica_reverb.build()
    assert len(steps) == 128
    assert len(madrs) == 64
    words = aica_reverb.program_words(steps)
    assert len(words) == 512
    for i in range(128):
        live = any(words[i * 4:i * 4 + 4])
        assert live == (i < 66), i


def test_header_is_current(tmp_path):
    out = tmp_path / "aica_reverb_program.h"
    aica_reverb.write_header(str(out))
    with open(HEADER, "r", newline="") as f:
        committed = f.read()
    with open(str(out), "r", newline="") as f:
        fresh = f.read()
    assert fresh == committed


def test_impulse_response_matches_ps1_formula():
    measured = _compare(_dc(0.5, 0), _ps1(0.5))
    print("rate 2 (median dB, p90 dB) per side:", measured)
    for median, p90 in measured:
        assert median <= 1.5
        assert p90 <= 4.0


def test_undoubled_delays_fail_the_comparison():
    measured = _compare(_dc(0.5, 0, rate=1), _ps1(0.5))
    print("rate 1 (median dB, p90 dB) per side:", measured)
    for median, _ in measured:
        assert median > 3.0


def test_first_arrival():
    left, _ = _dc(0.1, 0)
    first = next(i for i, v in enumerate(left) if v != 0)
    assert 1896 <= first <= 1898, first


def test_bus_routing():
    left, right = _dc(0.5, 2)
    ratio = 10.0 * math.log10(_energy(left) / float(_energy(right)))
    assert abs(ratio) <= 1.0, ratio
    _, right0 = _dc(0.5, 0)
    assert _energy(right0) > 0


def test_depth_zero_is_silent_and_depth_coefs():
    left, right = _dc(0.1, 0, depth=0)
    assert not any(left) and not any(right)
    assert aica_reverb.depth_coefs(0x4000, 0x4000) == (0x4000, 0x2960, 0x4000, 0x2960)
