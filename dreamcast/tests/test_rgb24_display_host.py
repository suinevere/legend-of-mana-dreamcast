#----------------------
# | test_rgb24_display_host.py
# | Description: Builds and runs the host C test for the RGB24 display conversion.
# | Author: suinevere
# | Dependencies: tests/host/test_rgb24_display.c, src/video/rgb24_display.c
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


def test_rgb24_display_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_rgb24_display_test.exe")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-O1",
                    "-I", os.path.join(DC, "src", "video"),
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_rgb24_display.c"),
                    os.path.join(DC, "src", "video", "rgb24_display.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "ok" in result.stdout
