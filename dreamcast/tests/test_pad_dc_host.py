#----------------------
# | test_pad_dc_host.py
# | Description: Builds and runs the host C test for the Dreamcast Maple-to-PS1 pad mapping with the host gcc.
# | Author: suinevere
# | Dependencies: tests/host/test_pad_dc_map.c, src/input/pad_dc_map.c
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


def test_pad_dc_map_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_dc_pad_map_test.exe")
    input_dir = os.path.join(DC, "src", "input")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wno-builtin-declaration-mismatch", "-Wall", "-Wextra",
                    "-I", input_dir, "-isystem", os.path.join(REPO, "include"),
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_pad_dc_map.c"),
                    os.path.join(input_dir, "pad_dc_map.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
