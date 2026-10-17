#----------------------
# | test_aica_convert_host.py
# | Description: Builds and runs the host C test for aica_convert.c's pitch, level/pan and envelope conversions with the host gcc.
# | Author: suinevere
# | Dependencies: tests/host/test_aica_convert.c, ../port/sound/aica_convert.c
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


def test_aica_convert_host_suite():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_aica_convert_test.exe")
    sound = os.path.join(REPO, "port", "sound")
    cmd = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", sound, "-o", exe,
           os.path.join(DC, "tests", "host", "test_aica_convert.c"),
           os.path.join(sound, "aica_convert.c")]
    subprocess.run(cmd, check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
