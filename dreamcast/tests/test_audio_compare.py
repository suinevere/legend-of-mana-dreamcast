#----------------------
# | test_audio_compare.py
# | Description: Checks dreamcast/tools/audio_compare.py on synthetic WAVs: alignment lag, envelope correlation, per-channel level difference, loop period ratio, spectral centroid, and note matching across timbre and level.
# | Author: suinevere
# | Dependencies: tools/audio_compare.py, tools/audio_capture.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys
import tempfile

import numpy

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import audio_capture  # noqa: E402
import audio_compare  # noqa: E402

RATE = 44100


def _write(name, left, right=None):
    right = left if right is None else right
    stereo = numpy.stack([left, right], axis=1)
    path = os.path.join(tempfile.mkdtemp(prefix="lom-compare-"), name)
    audio_capture.write_wav(path, numpy.clip(stereo * 32767, -32768, 32767).astype("<i2"))
    return path


def _modulated_noise(seconds, seed=1):
    rng = numpy.random.default_rng(seed)
    t = numpy.arange(int(seconds * RATE)) / RATE
    envelope = 0.3 + 0.25 * numpy.sin(2 * numpy.pi * 4 * t) * numpy.sin(2 * numpy.pi * 0.37 * t)
    return rng.standard_normal(len(t)) * 0.2 * envelope


def _repeating(seconds, period, seed=2):
    rng = numpy.random.default_rng(seed)
    unit = rng.standard_normal(int(period * RATE)) * 0.2
    unit *= 0.2 + numpy.abs(numpy.sin(numpy.linspace(0, 9 * numpy.pi, len(unit))))
    reps = int(numpy.ceil(seconds / period))
    return numpy.tile(unit, reps)[:int(seconds * RATE)]


def test_delay_is_found_and_envelopes_match():
    signal = _modulated_noise(20)
    delayed = numpy.concatenate([numpy.zeros(int(1.2 * RATE)), signal])[:len(signal)]
    result = audio_compare.compare(_write("ps1.wav", signal), _write("dc.wav", delayed))
    assert abs(result["lag_s"] - 1.2) <= 0.05, result
    assert result["envelope_corr"] > 0.95, result


def test_level_difference_per_channel():
    signal = _modulated_noise(10)
    result = audio_compare.compare(_write("ps1.wav", signal), _write("dc.wav", signal * 0.5, signal))
    assert abs(result["level_db_left"] + 6.02) <= 0.3, result
    assert abs(result["level_db_right"]) <= 0.3, result


def test_loop_period_ratio():
    result = audio_compare.compare(_write("ps1.wav", _repeating(40, 7.0)), _write("dc.wav", _repeating(40, 7.07)))
    assert result["loop_period_ratio"] is not None
    assert abs(result["loop_period_ratio"] - 1.01) <= 0.004, result


def test_centroid_ratio_of_identical_audio_is_one():
    signal = _modulated_noise(8)
    result = audio_compare.compare(_write("ps1.wav", signal), _write("dc.wav", signal))
    assert abs(result["centroid_ratio"] - 1.0) <= 0.01, result


def test_locate_finds_a_song_inside_a_longer_capture():
    song = _modulated_noise(12, seed=5)
    rng = numpy.random.default_rng(9)
    capture = numpy.concatenate([rng.standard_normal(30 * RATE) * 0.02, song, rng.standard_normal(10 * RATE) * 0.02])
    ps1 = audio_compare.read_wav(_write("ps1.wav", song))
    dc = audio_compare.read_wav(_write("dc.wav", capture))
    start = audio_compare.locate(ps1.mean(axis=1), dc.mean(axis=1))
    assert abs(start - 30.0) <= 0.05, start


def _melody(notes, seconds_each, harmonics=1, seed=3):
    t = numpy.arange(int(seconds_each * RATE)) / RATE
    parts = []
    for n in notes:
        f = 440.0 * 2 ** (n / 12.0)
        tone = sum(numpy.sin(2 * numpy.pi * f * h * t) / h for h in range(1, harmonics + 1))
        parts.append(0.2 * tone * numpy.minimum(1.0, 20 * t))
    return numpy.concatenate(parts)


TUNE = [0, 4, 7, 12, 7, 4, 2, 5, 9, 5, 2, -1, 0, 3, 7, 3]


def test_notes_are_found_across_timbre_and_level():
    song = _melody(TUNE, 0.5)
    other = _melody([1, 6, 10, 8, 11, 6, 1, 3], 0.5, harmonics=3)
    capture = numpy.concatenate([other, _melody(TUNE, 0.5, harmonics=4) * 0.3, other])
    at, score = audio_compare.locate_notes(song, capture)
    assert abs(at - len(other) / RATE) <= 0.15, (at, score)
    assert score > 0.6, score


def test_different_notes_score_low():
    song = _melody(TUNE, 0.5)
    at, score = audio_compare.locate_notes(song, _melody([1, 6, 10, 8, 11, 6, 1, 3] * 3, 0.5))
    assert score < 0.3, (at, score)
