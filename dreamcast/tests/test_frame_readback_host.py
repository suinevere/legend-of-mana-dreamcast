#----------------------
# | test_frame_readback_host.py
# | Description: Builds and runs the host C test for frame_readback_copy's 2x point-sampled RGB565 to PS1 15-bit conversion.
# | Author: suinevere
# | Dependencies: tests/host/test_frame_readback.c, src/video/frame_readback.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import shutil
import subprocess
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
PORT_PSYQ = os.path.join(REPO, "port", "psyq")
PORT_SYSTEM = os.path.join(REPO, "port", "system")


def test_frame_readback_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_dc_frame_readback_test.exe")
    src = os.path.join(DC, "src")
    video = os.path.join(src, "video")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wno-builtin-declaration-mismatch", "-Wall", "-Wextra",
                    "-I", PORT_PSYQ, "-I", PORT_SYSTEM,
                    "-I", video, "-I", os.path.join(src, "system"), "-I", os.path.join(src, "boot"),
                    "-isystem", os.path.join(REPO, "include"), "-isystem", os.path.join(REPO, "include", "sdk"),
                    "-include", os.path.join(PORT_PSYQ, "lom_prefix.h"), "-DVERSION_US", "-DLOM_DREAMCAST",
                    "-DLOM_HOST_TEST", "-o", exe,
                    os.path.join(DC, "tests", "host", "test_frame_readback.c"),
                    os.path.join(video, "frame_readback.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
