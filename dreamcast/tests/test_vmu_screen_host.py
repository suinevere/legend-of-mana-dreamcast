#----------------------
# | test_vmu_screen_host.py
# | Description: Builds and runs tests/host/test_vmu_screen.c against src/card/vmu_screen.c and vmu_format.c with the host compiler.
# | Author: suinevere
# | Dependencies: tests/host/test_vmu_screen.c, src/card/vmu_screen.c, src/card/vmu_format.c
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


def test_vmu_screen_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_vmu_screen_test.exe")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror",
                    "-I", os.path.join(REPO, "include", "sdk"), "-I", os.path.join(DC, "src", "card"),
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_vmu_screen.c"),
                    os.path.join(DC, "src", "card", "vmu_screen.c"),
                    os.path.join(DC, "src", "card", "vmu_format.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
