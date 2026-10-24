#----------------------
# | test_vsync_pacer_host.py
# | Description: Builds and runs the host C test for the shared VSync pacer.
# | Author: suinevere
# | Dependencies: tests/host/test_vsync_pacer.c, ../port/system/vsync_pacer.c
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
PORT_SYSTEM = os.path.join(DC, "..", "port", "system")


def test_vsync_pacer_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_vsync_pacer_test.exe")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-I", PORT_SYSTEM, "-o", exe,
                    os.path.join(DC, "tests", "host", "test_vsync_pacer.c"),
                    os.path.join(PORT_SYSTEM, "vsync_pacer.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
