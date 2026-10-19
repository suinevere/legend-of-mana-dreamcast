#----------------------
# | test_sound.py
# | Description: Flycast checks of the Dreamcast sound path: AKAO's tick rate; from one audio capture of the test image's boot, the stream channel's test tone and a streamed long voice; from the sound-effects disc, run with the title song stopped, the effects step's reverb tail, noise voice and pitch modulation; the new-game songs' notes against PS1 references; and the sweep of every song and effect bank. The sweep's post-stop gap check only needs to catch a stuck voice (around -20 dBFS), since Studio Large's reverb now rings for about 3 s after a stop and the check's 0.5-0.7 s window cannot move later inside sound_sweep.pad's 45-vblank pause.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tools/audio_capture.py, stale_image.py, lom.cdi, lom_test.cdi, src/sound/akao_tick_dc.c, src/sound/aica_stream.c, src/sound/sound_test_dc.c, src/boot/state_report_dc.c, tests/pad_scripts/new_game.pad, tests/pad_scripts/sound_sweep.pad, tests/pad_scripts/sound_effects.pad, lom_sound_effects_test.cdi, tools/sweep_check.py, tools/gen_pad_script.py, tools/aica_reverb.py, lom_sound_sweep_test.cdi
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re
import sys
import tempfile

import numpy
import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import aica_reverb  # noqa: E402
import audio_capture  # noqa: E402
import audio_compare  # noqa: E402
import flycast_run  # noqa: E402
import gen_pad_script  # noqa: E402
import stale_image  # noqa: E402
import sweep_check  # noqa: E402

TICKS_PER_SECOND = 240.0
TICK_TOLERANCE = 4.0
TONE_HZ = 1000.0
LONG_VOICE_HZ = 1050.0


def test_akao_ticks_at_240_hz():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = os.path.join(DC, "lom.cdi")
    if not os.path.exists(image):
        pytest.skip("lom.cdi not built")
    code, lines = flycast_run.run_flycast(image, seconds=60, until=None)
    text = "\n".join(lines[-60:])
    assert code == 0, text
    assert not any("LOM HALT" in line for line in lines), text
    assert any(line.strip().endswith(("LOM: state 2", "LOM: state 8")) for line in lines), text
    pairs = []
    frame = None
    for line in lines:
        m = re.search(r"LOM: frame (\d+) prims", line)
        if m:
            frame = int(m.group(1))
        m = re.search(r"LOM: akao ticks (\d+)", line)
        if m and frame is not None:
            pairs.append((frame, int(m.group(1))))
    assert len(pairs) >= 10, text
    rates = [(t1 - t0) * 60.0 / (f1 - f0) for (f0, t0), (f1, t1) in zip(pairs[2:], pairs[3:])]
    assert all(abs(rate - TICKS_PER_SECOND) <= TICK_TOLERANCE for rate in rates), rates


@pytest.fixture(scope="module")
def boot_capture():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = stale_image.require_fresh_test_image(DC)
    raw = os.path.join(tempfile.mkdtemp(prefix="lom-sound-"), "boot.raw")
    code, lines = flycast_run.run_flycast(image, seconds=90, until="LOM: tone end", audio_out=raw)
    assert code == 0, "\n".join(lines[-40:])
    return lines, audio_capture.read_raw(raw)


def _longest_run(flags):
    best = run = 0
    for flag in flags:
        run = run + 1 if flag else 0
        best = max(best, run)
    return best


def _first_run(flags, length):
    run = 0
    for index, flag in enumerate(flags):
        run = run + 1 if flag else 0
        if run == length:
            return index - length + 1
    return None


def test_stream_tone(boot_capture):
    lines, samples = boot_capture
    assert any("LOM: tone start 1000" in line for line in lines)
    for side in (0, 1):
        level = audio_capture.tone_db(samples[:, side], TONE_HZ, 0.1)
        assert _longest_run(level > -12.0) >= 25, (side, _longest_run(level > -12.0))
        start = _first_run(level > -12.0, 25)
        fine = audio_capture.levels_db(samples[:, side], 0.01)
        inside = fine[(start + 1) * 10:(start + 25) * 10]
        assert (inside > -40.0).all(), (side, float(inside.min()))


def test_long_voice_is_seamless(boot_capture):
    lines, samples = boot_capture
    assert any("LOM: longvoice start" in line for line in lines)
    assert not any("spu: key-on at unwritten" in line for line in lines)
    for side in (0, 1):
        level = audio_capture.tone_db(samples[:, side], LONG_VOICE_HZ, 0.1)
        start = _first_run(level > -20.0, 35)
        assert start is not None, (side, _longest_run(level > -20.0))
        body = level[start + 1:start + 35]
        assert body.min() >= numpy.median(body) - 3.0, (side, float(body.min()), float(numpy.median(body)))
        fine = audio_capture.levels_db(samples[:, side], 0.01)[(start + 1) * 10:(start + 35) * 10]
        assert (fine > -40.0).all(), (side, float(fine.min()))
        high = audio_capture.band_db(samples[:, side], 5000.0, 0.01)[(start + 1) * 10:(start + 35) * 10]
        assert high.max() <= numpy.median(high) + 6.0, (side, float(high.max()), float(numpy.median(high)))


RATE = audio_capture.RATE
EFFECT_PHASES = ("reverb-on", "reverb-off", "noise", "pitchmod", "end")
CLICK_LEVEL = 0.1 * 32768
NOISE_LEVEL = 0.05 * 32768
PITCHMOD_LEVEL = 0.1 * 32768
REVERB_HOLD = 3.0
NOISE_HOLD = 1.3
PITCHMOD_HOLD = 1.5
UNMOD_HOLD = 1.0
ONSET_SLACK = 0.2
QUIET_LEVEL = 0.01 * 32768
QUIET_BEFORE_CLICK = 0.5
NORMALIZE_FROM = int(0.05 * RATE)
NORMALIZE_TO = int(0.1 * RATE)


@pytest.fixture(scope="module")
def effects_capture():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    stale_image.require_fresh_test_image(DC)
    image = os.path.join(DC, "lom_sound_effects_test.cdi")
    raw = os.path.join(tempfile.mkdtemp(prefix="lom-effects-"), "effects.raw")
    code, lines = flycast_run.run_flycast(image, seconds=150, until="LOM: effects end", audio_out=raw)
    assert code == 0, "\n".join(lines[-40:])
    order = [phase for line in lines for phase in EFFECT_PHASES if line.strip().endswith("LOM: effects " + phase)]
    assert order == list(EFFECT_PHASES), order
    samples = audio_capture.read_raw(raw)
    return lines, samples, _effect_onsets(samples)


def _first_above(samples, level, start):
    loud = numpy.nonzero(numpy.abs(samples[start:].astype(numpy.int32)).max(axis=1) > level)[0]
    assert len(loud), (start, level)
    return start + int(loud[0])


def _after_quiet(samples, level, quiet):
    peak = numpy.abs(samples.astype(numpy.int32)).max(axis=1)
    loud = numpy.nonzero(peak > level)[0]
    still = numpy.r_[0, numpy.cumsum(peak > QUIET_LEVEL)]
    for at in loud:
        if at >= quiet and still[at] == still[at - quiet]:
            return int(at)
    raise AssertionError("no click after %d quiet samples" % quiet)


def _effect_onsets(samples):
    first = _after_quiet(samples, CLICK_LEVEL, int(QUIET_BEFORE_CLICK * RATE))
    second = _first_above(samples, CLICK_LEVEL, first + int((REVERB_HOLD - ONSET_SLACK) * RATE))
    noise = _first_above(samples, NOISE_LEVEL, second + int((REVERB_HOLD - ONSET_SLACK) * RATE))
    pitchmod = _first_above(samples, PITCHMOD_LEVEL, noise + int((NOISE_HOLD - ONSET_SLACK) * RATE))
    return first, second, noise, pitchmod


def _rms_db(channel):
    rms = numpy.sqrt(numpy.mean((channel.astype(numpy.float64) / 32768.0) ** 2))
    return 20.0 * numpy.log10(max(rms, 1e-9))


def test_reverb_tail(effects_capture):
    _lines, samples, (first, second, _noise, _pitchmod) = effects_capture
    near, far = int(0.03 * RATE), int(0.4 * RATE)
    reference = aica_reverb.impulse_response(0.4, bus=2)
    for side in (0, 1):
        wet = _rms_db(samples[first + near:first + far, side])
        dry = _rms_db(samples[second + near:second + far, side])
        assert wet - dry >= 30.0, (side, wet, dry)
        ref_side = numpy.asarray(reference[side], dtype=numpy.float64)
        tail = audio_capture.levels_db(samples[first:first + far, side], 0.01)
        ref = audio_capture.levels_db(ref_side, 0.01)
        keep = numpy.arange(3, min(len(tail), len(ref)))
        keep = keep[(ref[keep] > audio_capture.SILENCE_DB) & (tail[keep] > audio_capture.SILENCE_DB)]
        tail = tail - _rms_db(samples[first + NORMALIZE_FROM:first + NORMALIZE_TO, side])
        ref = ref - _rms_db(ref_side[NORMALIZE_FROM:NORMALIZE_TO])
        error = float(numpy.median(numpy.abs(tail[keep] - ref[keep])))
        assert error <= 3.0, (side, error, numpy.round(tail[keep], 1).tolist(), numpy.round(ref[keep], 1).tolist())


def test_noise_voice(effects_capture):
    _lines, samples, (_first, _second, noise, _pitchmod) = effects_capture
    body = samples[noise + int(0.2 * RATE):noise + int(0.8 * RATE)]
    for side in (0, 1):
        level = _rms_db(body[:, side])
        assert level >= -30.0, (side, level)
        bands = [float(numpy.median(audio_capture.band_db(body[:, side], hz, 0.01))) for hz in (1000.0, 4000.0, 10000.0)]
        assert max(bands) - min(bands) <= 10.0, (side, bands)


def _peak_track(mono, start, end, hop=441, size=2048, pad=16384):
    window = numpy.hanning(size)
    freqs = numpy.fft.rfftfreq(pad, 1.0 / RATE)
    band = numpy.nonzero((freqs > 700.0) & (freqs < 1500.0))[0]
    track = []
    for at in range(start, end - size + 1, hop):
        spectrum = numpy.abs(numpy.fft.rfft(mono[at:at + size] * window, pad))
        k = band[int(numpy.argmax(spectrum[band]))]
        a, b, c = numpy.log(spectrum[k - 1:k + 2] + 1e-12)
        track.append(freqs[k] + 0.5 * (a - c) / (a - 2.0 * b + c) * (freqs[1] - freqs[0]))
    return numpy.asarray(track)


def test_pitch_modulation(effects_capture):
    _lines, samples, (_first, _second, _noise, pitchmod) = effects_capture
    mono = samples.astype(numpy.float64).mean(axis=1)
    modulated = _peak_track(mono, pitchmod, pitchmod + int(PITCHMOD_HOLD * RATE))
    plain = _peak_track(mono, pitchmod + int((PITCHMOD_HOLD + 0.1) * RATE), pitchmod + int((PITCHMOD_HOLD + UNMOD_HOLD - 0.05) * RATE))
    ratio = float(modulated.max() / modulated.min())
    assert 1.15 <= ratio <= 1.45, (ratio, float(modulated.min()), float(modulated.max()))
    variation = numpy.abs(numpy.fft.rfft(modulated - modulated.mean(), 1024))
    rate = numpy.fft.rfftfreq(1024, 0.01)[1 + int(numpy.argmax(variation[1:]))]
    assert 4.0 <= rate <= 6.0, rate
    still = float(plain.max() / plain.min())
    assert still < 1.02, (still, float(plain.min()), float(plain.max()))


REFERENCES = os.path.join(DC, "..", "reference", "ps1", "audio")
SONGS = ("title", "wmap")

#----------------------
# | NOTES_MIN_SCORE
# | Description: Lowest note-match score a reference may have in the capture; right songs score about 0.88 and wrong ones at most 0.54.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
NOTES_MIN_SCORE = 0.8


def test_new_game_songs():
    present = [name for name in SONGS if os.path.isfile(os.path.join(REFERENCES, name + ".wav"))]
    if not present:
        pytest.skip("no references in reference/ps1/audio (see its README)")
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = stale_image.require_fresh_test_image(DC)
    work = tempfile.mkdtemp(prefix="lom-songs-")
    raw = os.path.join(work, "new_game.raw")
    code, lines = flycast_run.run_flycast(image, seconds=900, until="LOM: field scene 857", audio_out=raw)
    assert code == 0, "\n".join(lines[-40:])
    capture = audio_capture.read_raw(raw)
    capture_float = capture.astype(numpy.float64).mean(axis=1) / 32768.0
    failures = []
    for name in present:
        song = audio_compare.read_wav(os.path.join(REFERENCES, name + ".wav"))
        at, score = audio_compare.locate_notes(song.mean(axis=1), capture_float)
        if score < NOTES_MIN_SCORE:
            failures.append((name, at, score))
    assert not failures, failures


KNOWN_SPU_LINES = ("spu: SpuRead unsupported", "spu: reverb on", "spu: noise clock", "spu: volume sweep",
                   "spu: pitch modulation", "spu: noise", "spu: reverb")


def test_sweep_plays_everything():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    stale_image.require_fresh_test_image(DC)
    raw = os.path.join(tempfile.mkdtemp(prefix="lom-sweep-"), "sweep.raw")
    code, lines = flycast_run.run_flycast(os.path.join(DC, "lom_sound_sweep_test.cdi"), seconds=900,
                                          until="LOM: capture sweep-done", audio_out=raw)
    text = "\n".join(lines[-40:])
    assert code == 0, text
    assert not any("LOM HALT" in line for line in lines), text
    assert any("LOM: capture sweep-done" in line for line in lines), text
    unknown = sorted({line.split("LOM: ", 1)[1] for line in lines
                      if "LOM: spu: " in line and not line.split("LOM: ", 1)[1].strip() in KNOWN_SPU_LINES})
    assert not unknown, unknown
    assert not [line for line in lines if re.search(r"LOM: music \d+ bad song", line)]
    uploaded = {int(m.group(1)) for m in (re.search(r"LOM: sfxset (\d+) banks", line) for line in lines) if m}
    assert uploaded == set(gen_pad_script.SFXSET_RESOURCES), sorted(set(gen_pad_script.SFXSET_RESOURCES) - uploaded)
    report = sweep_check.check(lines, audio_capture.read_raw(raw))
    assert sorted(report["songs"]) == sorted(gen_pad_script.MUSIC_RESOURCES), report["songs"]
    assert report["gap_db"] < -40.0, report
    assert not report["silent"], report
