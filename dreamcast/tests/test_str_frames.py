#----------------------
# | test_str_frames.py
# | Description: Checks str_frames.py against the PS1 image: one frame's v2 bitstream header and raw 2352-byte sectors for ffmpeg.
# | Author: suinevere
# | Dependencies: tools/str_frames.py, the PS1 disc image
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import struct
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import str_frames  # noqa: E402


#----------------------
# | disc_image_or_skip
# | Description: The PS1 image path, skipping the test when it is absent.
# | Author: suinevere
# | Dependencies: str_frames.default_image
# | Globals: N/A
# | Params: N/A
# | Returns: image path
#----------------------
def disc_image_or_skip():
    image = str_frames.default_image()
    if not os.path.exists(image):
        pytest.skip("no PS1 image")
    return image


def test_first_intro_frame_has_v2_header():
    image = disc_image_or_skip()
    data, width, height = str_frames.frame_bitstream(image, 0x16A0, 0)
    words, magic, qscale, version = struct.unpack_from("<4H", data)
    assert (magic, version, width, height) == (0x3800, 2, 320, 240)
    assert 0 < words * 4 <= 81920 - 4
    assert 1 <= qscale <= 63


def test_field_movie_frame_has_its_size():
    image = disc_image_or_skip()
    _, width, height = str_frames.frame_bitstream(image, 0x16A8, 0)
    assert (width, height) == (176, 176)


def test_raw_sectors_are_2352_bytes_with_sync():
    image = disc_image_or_skip()
    raw = str_frames.raw_sectors(image, 0x16A0, 0, 3)
    assert len(raw) == 3 * 2352
    assert raw[:12] == bytes([0] + [0xFF] * 10 + [0])
