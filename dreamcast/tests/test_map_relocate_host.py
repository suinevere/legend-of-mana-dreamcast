#----------------------
# | test_map_relocate_host.py
# | Description: Builds and runs the host C test for map_relocate.c with the host gcc.
# | Author: suinevere
# | Dependencies: tests/host/test_map_relocate.c, src/cd/map_relocate.c
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


def test_map_relocate_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_map_relocate_test.exe")
    cd_dir = os.path.join(DC, "src", "cd")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-I", cd_dir, "-o", exe,
                    os.path.join(DC, "tests", "host", "test_map_relocate.c"),
                    os.path.join(cd_dir, "map_relocate.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
