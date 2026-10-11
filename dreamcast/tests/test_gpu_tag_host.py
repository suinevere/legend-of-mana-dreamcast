#----------------------
# | test_gpu_tag_host.py
# | Description: Builds and runs the host C test for Dreamcast packet tag pointers with the shared prefix force-included under LOM_DREAMCAST, linked against the Dreamcast fixed buffers.
# | Author: suinevere
# | Dependencies: tests/host/test_dc_gpu_tag.c, src/boot/main_globals_dc.c, ../port/psyq/lom_prefix.h, ../port/psyq/gpu_tag.h, src/boot/lom_platform_memory.h
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


def test_gpu_tag_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_dc_gpu_tag_test.exe")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wno-builtin-declaration-mismatch", "-Wall", "-Wextra", "-Werror",
                    "-I", PORT_PSYQ, "-I", os.path.join(DC, "src", "boot"),
                    "-isystem", os.path.join(REPO, "include"), "-isystem", os.path.join(REPO, "include", "sdk"),
                    "-include", os.path.join(PORT_PSYQ, "lom_prefix.h"), "-DVERSION_US", "-DLOM_DREAMCAST",
                    "-o", exe, os.path.join(DC, "tests", "host", "test_dc_gpu_tag.c"),
                    os.path.join(DC, "src", "boot", "main_globals_dc.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
