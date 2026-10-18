#----------------------
# | test_mdec_host.py
# | Description: Builds and runs the host C test for the Dreamcast MDEC emulator.
# | Author: suinevere
# | Dependencies: tests/host/test_mdec.c, src/movie/mdec.c
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


def test_mdec_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_mdec_test.exe")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-O1",
                    "-I", os.path.join(DC, "src", "movie"),
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_mdec.c"),
                    os.path.join(DC, "src", "movie", "mdec.c"), "-lm"], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "ok" in result.stdout
