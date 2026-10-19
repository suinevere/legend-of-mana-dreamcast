#----------------------
# | test_aica_regs_host.py
# | Description: Builds and runs the host C test for the AICA register-word packing and reverb COEF table with the host gcc.
# | Author: suinevere
# | Dependencies: tests/host/test_aica_regs.c, src/sound/aica_regs.c, src/sound/aica_reverb.c, ../port/sound/spu_backend.h
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


def test_aica_regs_host_suite():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_aica_regs_test.exe")
    sound = os.path.join(DC, "src", "sound")
    cmd = [cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-I", sound, "-I", os.path.join(REPO, "port", "sound"),
           "-o", exe, os.path.join(DC, "tests", "host", "test_aica_regs.c"), os.path.join(sound, "aica_regs.c"),
           os.path.join(sound, "aica_reverb.c")]
    subprocess.run(cmd, check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
