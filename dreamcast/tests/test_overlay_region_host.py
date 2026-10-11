#----------------------
# | test_overlay_region_host.py
# | Description: Builds and runs the host C test for the Dreamcast overlay region checks with the shared header parser.
# | Author: suinevere
# | Dependencies: tests/host/test_overlay_region.c, src/boot/overlay_region.c, ../port/boot/overlay_header.c
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
PORT_BOOT = os.path.join(os.path.dirname(DC), "port", "boot")


def test_overlay_region_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_dc_overlay_region_test.exe")
    boot = os.path.join(DC, "src", "boot")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-I", boot, "-I", PORT_BOOT, "-o", exe,
                    os.path.join(DC, "tests", "host", "test_overlay_region.c"),
                    os.path.join(boot, "overlay_region.c"),
                    os.path.join(PORT_BOOT, "overlay_header.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
