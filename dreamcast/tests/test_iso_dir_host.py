#----------------------
# | test_iso_dir_host.py
# | Description: Builds and runs the host C test for the ISO9660 volume descriptor and directory record parser with the host gcc.
# | Author: suinevere
# | Dependencies: tests/host/test_iso_dir.c, src/cd/iso_dir.c
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


def test_iso_dir_host_suite():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_iso_dir_test.exe")
    cd = os.path.join(DC, "src", "cd")
    cmd = [cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-I", cd, "-o", exe,
           os.path.join(DC, "tests", "host", "test_iso_dir.c"), os.path.join(cd, "iso_dir.c")]
    subprocess.run(cmd, check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
