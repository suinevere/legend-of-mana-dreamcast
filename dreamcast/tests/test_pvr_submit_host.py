#----------------------
# | test_pvr_submit_host.py
# | Description: Builds and runs the host C test for the PowerVR submitter's scene policy through libgpu with a fake backend.
# | Author: suinevere
# | Dependencies: tests/host/test_pvr_submit.c, src/video/*.c (pure units), ../port/psyq/lom_prefix.h
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
UNITS = ["pvr_submit.c", "libgpu_dc.c", "gpu_decode.c", "gpu_census.c", "clip.c", "blend.c", "palette_bank.c",
         "texture_cache.c", "vram_mirror.c"]


def test_pvr_submit_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_dc_pvr_submit_test.exe")
    src = os.path.join(DC, "src")
    video = os.path.join(src, "video")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wno-builtin-declaration-mismatch", "-Wall", "-Wextra",
                    "-I", PORT_PSYQ, "-I", PORT_SYSTEM,
                    "-I", video, "-I", os.path.join(src, "system"), "-I", os.path.join(src, "boot"),
                    "-isystem", os.path.join(REPO, "include"), "-isystem", os.path.join(REPO, "include", "sdk"),
                    "-include", os.path.join(PORT_PSYQ, "lom_prefix.h"), "-DVERSION_US", "-DLOM_DREAMCAST",
                    "-DLOM_HOST_TEST", "-o", exe,
                    os.path.join(DC, "tests", "host", "test_pvr_submit.c")]
                   + [os.path.join(video, u) for u in UNITS], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
