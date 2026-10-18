#----------------------
# | test_libpress_host.py
# | Description: Decodes real STR frames through libpress_dc.c on the host and compares them with ffmpeg's own MDEC decode of the same frames.
# | Author: suinevere
# | Dependencies: tests/host/test_libpress_dc.c, src/movie/*.c, tools/str_frames.py, ffmpeg, the PS1 disc image
# | Globals: FRAMES, MEAN_LIMIT, OUTLIER_DIFF, OUTLIER_LIMIT
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import shutil
import struct
import subprocess
import sys
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(DC, "tools"))

import str_frames  # noqa: E402

FRAMES = [(0x16A0, 200, 10), (0x16AA, 40, 10)]
MEAN_LIMIT = 3.0
OUTLIER_DIFF = 24
OUTLIER_LIMIT = 0.005


#----------------------
# | build
# | Description: Compiles the host test against the SDK headers, skipping without a host C compiler.
# | Author: suinevere
# | Dependencies: host gcc
# | Globals: N/A
# | Params: N/A
# | Returns: executable path
#----------------------
def build():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    movie = os.path.join(DC, "src", "movie")
    exe = os.path.join(tempfile.gettempdir(), "lom_libpress_test.exe")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-O2",
                    "-I", movie, "-I", os.path.join(DC, "src", "system"), "-I", os.path.join(REPO, "include"),
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_libpress_dc.c"),
                    os.path.join(movie, "libpress_dc.c"), os.path.join(movie, "mdec.c"), os.path.join(movie, "str_vlc.c"),
                    "-lm"], check=True)
    return exe


#----------------------
# | decode_ours
# | Description: Runs the host decoder on one frame and returns its RGB24 bytes.
# | Author: suinevere
# | Dependencies: build, str_frames.frame_bitstream
# | Globals: N/A
# | Params: exe -- host test, image -- PS1 image, resource, frame -- what to decode, depth -- 24 or 15, tmp -- directory
# | Returns: (bytes, width, height)
#----------------------
def decode_ours(exe, image, resource, frame, depth, tmp):
    data, width, height = str_frames.frame_bitstream(image, resource, frame)
    src = os.path.join(tmp, "frame.bin")
    out = os.path.join(tmp, "ours%d.rgb" % depth)
    with open(src, "wb") as f:
        f.write(data)
    result = subprocess.run([exe, src, str(width), str(height), out, str(depth)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    with open(out, "rb") as f:
        return f.read(), width, height


#----------------------
# | decode_ffmpeg
# | Description: Decodes one frame with ffmpeg's psxstr demuxer and mdec decoder, nearest-neighbour chroma, to RGB24; only video sectors go in, since the demuxer sizes the picture from the first sector.
# | Author: suinevere
# | Dependencies: ffmpeg, str_frames.raw_sectors
# | Globals: N/A
# | Params: image -- PS1 image, resource, frame -- what to decode, sectors_per_frame -- stream interleave, tmp -- directory
# | Returns: bytes
#----------------------
def decode_ffmpeg(image, resource, frame, sectors_per_frame, tmp):
    raw = os.path.join(tmp, "movie.str")
    ref = os.path.join(tmp, "ref.rgb")
    data = str_frames.raw_sectors(image, resource, 0, (frame + 2) * sectors_per_frame)
    with open(raw, "wb") as f:
        for at in range(0, len(data), 2352):
            if struct.unpack_from("<H", data, at + 24 + 2)[0] == str_frames.VIDEO_SECTOR_TYPE:
                f.write(data[at:at + 2352])
    subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "psxstr", "-i", raw,
                    "-vf", "select=eq(n\\,%d)" % frame, "-frames:v", "1",
                    "-sws_flags", "neighbor+full_chroma_int+accurate_rnd+bitexact",
                    "-pix_fmt", "rgb24", "-f", "rawvideo", ref], check=True)
    with open(ref, "rb") as f:
        return f.read()


@pytest.mark.parametrize("resource,frame,sectors_per_frame", FRAMES)
def test_frame_matches_ffmpeg(resource, frame, sectors_per_frame, tmp_path):
    image = str_frames.default_image()
    if not os.path.exists(image):
        pytest.skip("no PS1 image")
    if shutil.which("ffmpeg") is None:
        pytest.skip("no ffmpeg")
    exe = build()
    ours, width, height = decode_ours(exe, image, resource, frame, 24, str(tmp_path))
    ref = decode_ffmpeg(image, resource, frame, sectors_per_frame, str(tmp_path))
    assert len(ref) == len(ours) == width * height * 3
    diffs = [abs(a - b) for a, b in zip(ours, ref)]
    mean = sum(diffs) / len(diffs)
    outliers = sum(1 for d in diffs if d > OUTLIER_DIFF) / len(diffs)
    print("0x%X frame %d: mean %.2f outliers %.4f" % (resource, frame, mean, outliers))
    assert mean <= MEAN_LIMIT
    assert outliers <= OUTLIER_LIMIT


def test_15_bit_matches_24_bit(tmp_path):
    image = str_frames.default_image()
    if not os.path.exists(image):
        pytest.skip("no PS1 image")
    exe = build()
    full, _, _ = decode_ours(exe, image, 0x16AA, 40, 24, str(tmp_path))
    short, _, _ = decode_ours(exe, image, 0x16AA, 40, 15, str(tmp_path))
    assert all(abs((a >> 3) - (b >> 3)) <= 1 for a, b in zip(full, short))
