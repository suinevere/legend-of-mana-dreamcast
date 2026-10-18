#----------------------
# | test_cd_stream_host.py
# | Description: Builds and runs the host C test for the paced movie sector stream.
# | Author: suinevere
# | Dependencies: tests/host/test_cd_stream.c, src/cd/cd_stream.c
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


def test_cd_stream_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_cd_stream_test.exe")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-O1",
                    "-I", os.path.join(DC, "src", "cd"),
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_cd_stream.c"),
                    os.path.join(DC, "src", "cd", "cd_stream.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "ok" in result.stdout
