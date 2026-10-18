#----------------------
# | test_movie.py
# | Description: Flycast checks for movie playback: the software decode-speed probe, and the intro playing at 15 fps with frames that match ffmpeg's decode before Start skips it to the title, its audio matching the disc's, and FIELD's embedded and full-screen movies reaching their still images.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tools/str_frames.py, tests/stale_image.py, ffmpeg, the PADSEL images lom_movie_probe_test.cdi, lom_movie_intro_test.cdi and lom_field_movie_test.cdi
# | Globals: AUDIO_BLOCKS, AUDIO_MATCH_LIMIT, PROBE_LINE, VBL_LINE, FRAMES_LINE, SECTORS_LINE, FIELD_MOVIE_LINE, MOVIE_END_STILL, RATE_MEAN_LIMIT, RATE_WINDOW_LIMIT, FIELD_RATE_MEAN_LIMIT, FIELD_RATE_WINDOW_LIMIT, MATCH_MEAN_LIMIT, MATCH_WINDOW
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(DC, "..", "port", "tools"))

import audio_capture  # noqa: E402
import flycast_run  # noqa: E402
import stale_image  # noqa: E402
import spu_adpcm  # noqa: E402
import str_frames  # noqa: E402

PROBE_LINE = re.compile(r"LOM: movie probe frames (\d+) us_avg (\d+) us_max (\d+)")
VBL_LINE = re.compile(r"LOM: probe vbl (\d+\.\d+)")
FRAMES_LINE = re.compile(r"LOM: movie frames (\d+) ")
SECTORS_LINE = re.compile(r"LOM: cd stream sectors (\d+)")
FIELD_MOVIE_LINE = re.compile(r"LOM: fieldmovie anim (\d+) movie (\d+)")
MOVIE_END_STILL = "LOM: field movie_end 4"
RATE_MEAN_LIMIT = 4.05
RATE_WINDOW_LIMIT = 4.25
FIELD_RATE_MEAN_LIMIT = 2.15
FIELD_RATE_WINDOW_LIMIT = 2.20
MATCH_MEAN_LIMIT = 6.0
MATCH_WINDOW = (300, 600)
AUDIO_BLOCKS = (60, 120)
AUDIO_MATCH_LIMIT = 0.9


#----------------------
# | padsel_image
# | Description: A fresh PADSEL disc for one pad script, skipping without Flycast and failing when it is older than lom_test.elf.
# | Author: suinevere
# | Dependencies: flycast_run.find_flycast, stale_image.require_fresh_test_image
# | Globals: N/A
# | Params: script -- pad script name
# | Returns: image path
#----------------------
def padsel_image(script):
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    stale_image.require_fresh_test_image(DC)
    image = os.path.join(DC, "lom_%s_test.cdi" % script)
    elf = os.path.join(DC, "lom_test.elf")
    assert os.path.exists(image) and os.path.getmtime(image) >= os.path.getmtime(elf), \
        "%s missing or older than lom_test.elf; rebuild the test image" % image
    return image


def test_decode_probe():
    image = padsel_image("movie_probe")
    code, lines = flycast_run.run_flycast(image, seconds=300, until="LOM: movie probe")
    assert code == 0, "\n".join(lines[-60:])
    found = [m for m in (PROBE_LINE.search(line) for line in lines) if m]
    assert found, "\n".join(lines[-60:])
    frames, us_avg, us_max = (int(v) for v in found[-1].groups())
    print("movie probe: frames %d us_avg %d us_max %d" % (frames, us_avg, us_max))
    assert frames == 150


#----------------------
# | ffmpeg_frames
# | Description: Decodes the intro's frames first..last with ffmpeg from its video sectors only, as RGB24 320x240 arrays.
# | Author: suinevere
# | Dependencies: ffmpeg, str_frames.raw_sectors, numpy
# | Globals: N/A
# | Params: first, last -- 0-based frame range, inclusive
# | Returns: list of (frame index, array)
#----------------------
def ffmpeg_frames(first, last):
    import numpy as np

    image = str_frames.default_image()
    data = str_frames.raw_sectors(image, 0x16A0, 0, (last + 2) * 10)
    work = tempfile.mkdtemp(prefix="lom-movie-")
    raw = os.path.join(work, "intro.str")
    out = os.path.join(work, "frames.rgb")
    with open(raw, "wb") as f:
        for at in range(0, len(data), 2352):
            if struct.unpack_from("<H", data, at + 24 + 2)[0] == str_frames.VIDEO_SECTOR_TYPE:
                f.write(data[at:at + 2352])
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "psxstr", "-i", raw,
                    "-vf", "select=between(n\\,%d\\,%d)" % (first, last), "-fps_mode", "passthrough",
                    "-sws_flags", "neighbor+full_chroma_int+accurate_rnd+bitexact",
                    "-pix_fmt", "rgb24", "-f", "rawvideo", out], check=True)
    blob = np.fromfile(out, dtype=np.uint8).reshape(-1, 240, 320, 3)
    shutil.rmtree(work, ignore_errors=True)
    return [(first + i, frame) for i, frame in enumerate(blob)]


#----------------------
# | best_match
# | Description: The ffmpeg frame in MATCH_WINDOW closest to a 640x480 capture halved by nearest sampling, by mean absolute difference.
# | Author: suinevere
# | Dependencies: ffmpeg_frames, PIL.Image, numpy
# | Globals: MATCH_WINDOW
# | Params: capture -- PNG path
# | Returns: (mean difference, frame index)
#----------------------
def best_match(capture):
    import numpy as np
    from PIL import Image

    shot = np.asarray(Image.open(capture).convert("RGB").resize((320, 240), Image.NEAREST), dtype=np.int16)
    scores = [(float(np.abs(shot - frame.astype(np.int16)).mean()), index) for index, frame in ffmpeg_frames(*MATCH_WINDOW)]
    return min(scores)


#----------------------
# | intro_audio
# | Description: The intro's audio from the disc as movie_audio_dc.c plays it: ring blocks of two audio sectors, left and right SPU ADPCM halves decoded up to their loop-end blocks with running history.
# | Author: suinevere
# | Dependencies: str_frames.raw_sectors, spu_adpcm.decode_block, numpy
# | Globals: N/A
# | Params: first, last -- ring block range, last exclusive
# | Returns: (frames, 2) int16 array
#----------------------
def intro_audio(first, last):
    import numpy as np

    data = str_frames.raw_sectors(str_frames.default_image(), 0x16A0, 0, (last + 2) * 10)
    audio = [data[at + 24:at + 24 + 2048] for at in range(0, len(data), 2352)
             if struct.unpack_from("<H", data, at + 24 + 2)[0] == 1]
    hist = [(0, 0), (0, 0)]
    out = []
    for index in range(last):
        block = audio[2 * index] + audio[2 * index + 1]
        halves = []
        for side, start, count in ((0, 0xD0, 128), (1, 0x8D0, 115)):
            pcm = []
            for b in range(count):
                chunk = block[start + b * 16:start + b * 16 + 16]
                samples, hist[side] = spu_adpcm.decode_block(chunk, hist[side])
                pcm.extend(samples)
                if chunk[1] & 1:
                    break
            halves.append(pcm)
        if index >= first:
            frames = min(len(halves[0]), len(halves[1]))
            out.extend(zip(halves[0][:frames], halves[1][:frames]))
    return np.array(out, dtype=np.int16)


#----------------------
# | best_audio_match
# | Description: Finds the reference in a capture by cross-correlation and returns the normalised correlation of each channel at that lag.
# | Author: suinevere
# | Dependencies: numpy
# | Globals: N/A
# | Params: capture, reference -- (frames, 2) int16 arrays
# | Returns: (left correlation, right correlation, lag in frames)
#----------------------
def best_audio_match(capture, reference):
    import numpy as np

    ref = reference[:, 0].astype(np.float64)
    cap = capture[:, 0].astype(np.float64)
    size = 1
    while size < len(cap) + len(ref):
        size *= 2
    corr = np.fft.irfft(np.fft.rfft(cap, size) * np.conj(np.fft.rfft(ref, size)), size)[:len(cap) - len(ref)]
    lag = int(np.argmax(corr))
    scores = []
    for side in (0, 1):
        a = capture[lag:lag + len(reference), side].astype(np.float64)
        b = reference[:, side].astype(np.float64)
        scores.append(float(np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b) + 1e-9)))
    return scores[0], scores[1], lag


def test_intro_plays_at_movie_rate(tmp_path):
    image = padsel_image("movie_intro")
    if shutil.which("ffmpeg") is None or not os.path.exists(str_frames.default_image()):
        pytest.skip("needs ffmpeg and the PS1 image")
    out = str(tmp_path)
    raw = os.path.join(out, "intro.raw")
    code, lines = flycast_run.run_flycast(image, seconds=240, until="LOM: capture title-after-movie", captures=out, audio_out=raw)
    assert code == 0, "\n".join(lines[-60:])
    assert not any(flycast_run.HALT_MARKER in line for line in lines), "\n".join(lines[-60:])
    in_intro = False
    rates = []
    for line in lines:
        if line.strip().endswith("LOM: state 8"):
            in_intro = True
        elif re.search(r"LOM: state \d+$", line.strip()):
            in_intro = False
        match = VBL_LINE.search(line)
        if in_intro and match:
            rates.append(float(match.group(1)))
    steady = rates[2:-1]
    assert len(steady) >= 20, rates
    print("intro rates: mean %.3f max %.2f over %d windows" % (sum(steady) / len(steady), max(steady), len(steady)))
    assert sum(steady) / len(steady) <= RATE_MEAN_LIMIT, steady
    assert max(steady) <= RATE_WINDOW_LIMIT, steady
    decoded = sum(int(m.group(1)) for m in (FRAMES_LINE.search(line) for line in lines) if m)
    sectors = [int(m.group(1)) for m in (SECTORS_LINE.search(line) for line in lines) if m]
    assert sectors, "no cd stream sectors line"
    print("decoded %d frames from %d sectors" % (decoded, sectors[0]))
    assert decoded >= 0.95 * (sectors[0] // 10) - 60
    mean, index = best_match(os.path.join(out, "movie-intro.png"))
    print("movie-intro matches ffmpeg frame %d at mean %.2f" % (index, mean))
    assert mean <= MATCH_MEAN_LIMIT
    assert os.path.exists(os.path.join(out, "title-after-movie.png"))
    left, right, lag = best_audio_match(audio_capture.read_raw(raw), intro_audio(*AUDIO_BLOCKS))
    print("intro audio matches the disc at %.3f / %.3f, %d frames in" % (left, right, lag))
    assert left >= AUDIO_MATCH_LIMIT and right >= AUDIO_MATCH_LIMIT


def test_field_movie_reaches_still(tmp_path):
    image = padsel_image("field_movie")
    out = str(tmp_path)
    code, lines = flycast_run.run_flycast(image, seconds=480, until="LOM: capture field-movie-full-screen", captures=out)
    assert code == 0, "\n".join(lines[-60:])
    assert not any(flycast_run.HALT_MARKER in line for line in lines), "\n".join(lines[-60:])
    starts = [i for i, line in enumerate(lines) if FIELD_MOVIE_LINE.search(line)]
    movies = [int(FIELD_MOVIE_LINE.search(lines[i]).group(2)) for i in starts]
    assert movies == [0, 2], "\n".join(lines[-60:])
    stills = [i for i, line in enumerate(lines) if line.strip().endswith(MOVIE_END_STILL)]
    assert any(starts[0] < i < starts[1] for i in stills), "embedded movie never showed its still"
    assert any(i > starts[1] for i in stills), "full-screen movie never showed its still"
    first_still = min(i for i in stills if i > starts[0])
    rates = [float(m.group(1)) for m in (VBL_LINE.search(line) for line in lines[starts[0]:first_still]) if m][2:]
    print("field movie 0 rates: %s" % rates)
    assert len(rates) >= 3, rates
    assert sum(rates) / len(rates) <= FIELD_RATE_MEAN_LIMIT, rates
    assert max(rates) <= FIELD_RATE_WINDOW_LIMIT, rates
    assert os.path.exists(os.path.join(out, "field-movie-playing.png"))
    assert os.path.exists(os.path.join(out, "field-movie-still.png"))
    assert os.path.exists(os.path.join(out, "field-movie-playing-full-screen.png"))
    assert os.path.exists(os.path.join(out, "field-movie-full-screen.png"))
