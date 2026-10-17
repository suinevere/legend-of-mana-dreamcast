#----------------------
# | test_spu_shadow_host.py
# | Description: Builds and runs the host C test for the shared SPU register shadow with the host gcc.
# | Author: suinevere
# | Dependencies: tests/host/test_spu_shadow.c, ../port/sound/spu_shadow.c, include/main/audio/spu_registers.h
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


def test_spu_shadow_host_suite():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_spu_shadow_test.exe")
    sound = os.path.join(REPO, "port", "sound")
    cmd = [cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-I", sound, "-I", os.path.join(REPO, "include"),
           "-o", exe, os.path.join(DC, "tests", "host", "test_spu_shadow.c"), os.path.join(sound, "spu_shadow.c")]
    subprocess.run(cmd, check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
