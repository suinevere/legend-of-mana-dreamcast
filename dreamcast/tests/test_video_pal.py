#----------------------
# | test_video_pal.py
# | Description: Flycast checks that a PAL console boots at 60 Hz by default, at 50 Hz with the boot-hold script, and keeps the title song matching the PS1 reference at both rates; a genuine European reading needs Flycast's Dreamcast.Broadcast = PAL alongside Dreamcast.Region, since flashrom_get_region() reads a code built from both (core/hw/flashrom/nvmem.cpp fixUpDCFlash).
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tools/audio_capture.py, tools/audio_compare.py, stale_image.py, lom.cdi, lom_pal_50hz_test.cdi, src/system/dc_system.c, src/video/dc_video_mode.c, src/boot/state_report_dc.c, tests/pad_scripts/pal_50hz.pad, reference/ps1/audio/title.wav
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

import audio_capture  # noqa: E402
import audio_compare  # noqa: E402
import flycast_run  # noqa: E402
import stale_image  # noqa: E402

EUROPE = 2
US = 1
RGB = 2
PAL = 1
TICK_TOLERANCE = 0.1
REFERENCE_TITLE = os.path.join(DC, "..", "reference", "ps1", "audio", "title.wav")
NOTES_MIN_SCORE = 0.8


def _ticks_per_vblank(lines):
    pairs = []
    frame = None
    for line in lines:
        m = re.search(r"LOM: frame (\d+) prims", line)
        if m:
            frame = int(m.group(1))
        m = re.search(r"LOM: akao ticks (\d+)", line)
        if m and frame is not None:
            pairs.append((frame, int(m.group(1))))
    assert len(pairs) >= 10, pairs
    return [(t1 - t0) / float(f1 - f0) for (f0, t0), (f1, t1) in zip(pairs[2:], pairs[3:])]


def test_pal_console_boots_60hz_by_default():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    image = os.path.join(DC, "lom.cdi")
    if not os.path.exists(image):
        pytest.skip("lom.cdi not built")
    code, lines = flycast_run.run_flycast(image, seconds=60, until=None, region=EUROPE, cable=RGB, broadcast=PAL)
    text = "\n".join(lines[-40:])
    assert code == 0, text
    assert not any("LOM HALT" in line for line in lines), text
    assert any(line.strip().endswith("LOM: video 60 Hz") for line in lines), text
    rates = _ticks_per_vblank(lines)
    assert all(abs(rate - 4.0) <= TICK_TOLERANCE for rate in rates), rates


def test_pal_console_boots_50hz_held_with_title_music():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    stale_image.require_fresh_test_image(DC)
    image = os.path.join(DC, "lom_pal_50hz_test.cdi")
    if not os.path.exists(image):
        pytest.skip("lom_pal_50hz_test.cdi not built")
    raw = os.path.join(tempfile.mkdtemp(prefix="lom-pal50-"), "pal50.raw")
    code, lines = flycast_run.run_flycast(image, seconds=60, until=None,
                                           region=EUROPE, cable=RGB, broadcast=PAL, audio_out=raw)
    text = "\n".join(lines[-40:])
    assert code == 0, text
    assert not any("LOM HALT" in line for line in lines), text
    assert any(line.strip().endswith("LOM: video 50 Hz") for line in lines), text
    assert any(line.strip().endswith("LOM: state 2") for line in lines), text
    assert any(line.strip().endswith("LOM: capture title") for line in lines), text
    rates = _ticks_per_vblank(lines)
    assert all(abs(rate - 4.8) <= TICK_TOLERANCE for rate in rates), rates
    if not os.path.isfile(REFERENCE_TITLE):
        pytest.skip("no reference/ps1/audio/title.wav (see its README)")
    capture = audio_capture.read_raw(raw)
    capture_float = capture.astype(numpy.float64).mean(axis=1) / 32768.0
    song = audio_compare.read_wav(REFERENCE_TITLE)
    _at, score = audio_compare.locate_notes(song.mean(axis=1), capture_float)
    assert score >= NOTES_MIN_SCORE, score


def test_pal_disc_us_region_stays_60hz():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    stale_image.require_fresh_test_image(DC)
    image = os.path.join(DC, "lom_pal_50hz_test.cdi")
    if not os.path.exists(image):
        pytest.skip("lom_pal_50hz_test.cdi not built")
    code, lines = flycast_run.run_flycast(image, seconds=30, until="LOM: video 60 Hz", region=US, cable=RGB)
    text = "\n".join(lines[-40:])
    assert code == 0, text
    assert not any("LOM HALT" in line for line in lines), text
