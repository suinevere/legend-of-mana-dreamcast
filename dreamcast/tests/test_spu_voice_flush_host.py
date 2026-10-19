#----------------------
# | test_spu_voice_flush_host.py
# | Description: Builds and runs the host C test for the shared SPU voice flush and libspu shim with the host gcc.
# | Author: suinevere
# | Dependencies: tests/host/test_spu_voice_flush.c, ../port/sound/*.c, include/main/audio/spu_registers.h, include/sdk/libspu.h
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
SOURCES = ("spu_voice_flush.c", "libspu_shim.c", "spu_long_voice.c", "spu_sample_map.c", "spu_noise.c", "aica_convert.c", "spu_shadow.c")


def test_spu_voice_flush_host_suite():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_spu_voice_flush_test.exe")
    sound = os.path.join(REPO, "port", "sound")
    cmd = [cc, "-std=gnu11", "-O1", "-Wall", "-Wextra", "-Werror", "-I", sound, "-I", os.path.join(REPO, "include"),
           "-I", os.path.join(REPO, "include", "sdk"), "-o", exe, os.path.join(DC, "tests", "host", "test_spu_voice_flush.c")]
    cmd += [os.path.join(sound, name) for name in SOURCES]
    subprocess.run(cmd, check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
