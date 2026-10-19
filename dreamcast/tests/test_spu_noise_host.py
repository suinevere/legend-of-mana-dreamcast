#----------------------
# | test_spu_noise_host.py
# | Description: Builds and runs the host C test for spu_noise.c's LFSR sequence, rendered table statistics, pitch formula and voice span with the host gcc.
# | Author: suinevere
# | Dependencies: tests/host/test_spu_noise.c, ../port/sound/spu_noise.c
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


def test_spu_noise_host_suite():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_spu_noise_test.exe")
    sound = os.path.join(REPO, "port", "sound")
    cmd = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", sound, "-o", exe,
           os.path.join(DC, "tests", "host", "test_spu_noise.c"),
           os.path.join(sound, "spu_noise.c"), "-lm"]
    subprocess.run(cmd, check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
