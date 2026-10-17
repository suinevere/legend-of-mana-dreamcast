#----------------------
# | test_sweep_check.py
# | Description: Host checks of sweep_check on a synthetic capture: marker alignment at an off-nominal vblank rate, a silent song found by name, and quiet gaps.
# | Author: suinevere
# | Dependencies: tools/sweep_check.py, tools/audio_capture.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import numpy

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import audio_capture  # noqa: E402
import sweep_check  # noqa: E402

RATE = audio_capture.RATE
VBLANK_SECONDS = 1.0 / 59.94
LEAD_SECONDS = 2.3


def _at(vblank):
    return int((LEAD_SECONDS + (vblank - 1000) * VBLANK_SECONDS) * RATE)


def _synthetic(silent_resource):
    lines = ["LOM: tone start 1000 at 1000"]
    total = _at(1000 + 120 + 3 * 225 + 200)
    mono = numpy.zeros(total)
    t = numpy.arange(int(RATE)) / RATE
    for vblank in (1000, 1000 + 120 + 3 * 225):
        mono[_at(vblank):_at(vblank) + len(t)] = 0.9 * numpy.sin(2 * numpy.pi * 1000.0 * t)
    for index, resource in enumerate((23, 24, 25)):
        start = 1000 + 120 + index * 225
        lines.append("LOM: music %d at %d" % (resource, start))
        lines.append("LOM: musicoff at %d" % (start + 180))
        if resource != silent_resource:
            noise = numpy.random.default_rng(resource).normal(0.0, 0.05, _at(start + 180) - _at(start))
            mono[_at(start):_at(start + 180)] = noise
    lines.append("LOM: tone start 1000 at %d" % (1000 + 120 + 3 * 225))
    samples = (numpy.stack([mono, mono], axis=1) * 32767).astype("<i2")
    return lines, samples


def test_sweep_check_passes_when_every_song_sounds():
    lines, samples = _synthetic(None)
    report = sweep_check.check(lines, samples)
    assert report["songs"] == [23, 24, 25]
    assert report["silent"] == []
    assert report["gap_db"] < -50.0


def test_sweep_check_names_a_silent_song():
    lines, samples = _synthetic(24)
    report = sweep_check.check(lines, samples)
    assert report["silent"] == [24]


def test_sweep_check_allows_a_slow_intro():
    lines, samples = _synthetic(None)
    late = [i for i, line in enumerate(lines) if line.startswith("LOM: music 23 ")][0]
    start = int(lines[late].split()[-1])
    samples[_at(start):_at(start) + int(3.0 * RATE)] = 0
    assert sweep_check.check(lines, samples)["silent"] == []
    lines[late] = "LOM: music 24 at %d" % start
    lines = [line for line in lines if not line.startswith("LOM: music 24 at %d" % (start + 225))]
    assert 24 in sweep_check.check(lines, samples)["silent"]
