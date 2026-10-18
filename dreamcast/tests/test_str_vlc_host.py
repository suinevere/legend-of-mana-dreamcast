#----------------------
# | test_str_vlc_host.py
# | Description: Builds the str_vlc.c host test and runs it on real intro and FIELD movie frames from the PS1 image.
# | Author: suinevere
# | Dependencies: tests/host/test_str_vlc.c, src/movie/str_vlc.c, tools/str_frames.py, the PS1 disc image
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import shutil
import subprocess
import sys
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import str_frames  # noqa: E402

FRAMES = [(0x16A0, 0), (0x16A0, 1), (0x16A0, 200), (0x16A8, 0)]


#----------------------
# | build
# | Description: Compiles the host test, skipping without a host C compiler.
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
    exe = os.path.join(tempfile.gettempdir(), "lom_str_vlc_test.exe")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-O1",
                    "-I", os.path.join(DC, "src", "movie"),
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_str_vlc.c"),
                    os.path.join(DC, "src", "movie", "str_vlc.c")], check=True)
    return exe


def test_str_vlc_synthetic():
    result = subprocess.run([build()], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "ok" in result.stdout


@pytest.mark.parametrize("resource,frame", FRAMES)
def test_str_vlc_real_frame(resource, frame, tmp_path):
    image = str_frames.default_image()
    if not os.path.exists(image):
        pytest.skip("no PS1 image")
    exe = build()
    data, width, height = str_frames.frame_bitstream(image, resource, frame)
    path = str(tmp_path / "frame.bin")
    with open(path, "wb") as f:
        f.write(data)
    result = subprocess.run([exe, path, str(width), str(height)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
