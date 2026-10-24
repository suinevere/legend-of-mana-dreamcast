#----------------------
# | test_hang_watch_host.py
# | Description: Builds and runs the host C test for the hang check's decision and halt text.
# | Author: suinevere
# | Dependencies: tests/host/test_hang_watch.c, src/system/hang_watch_check.c
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


def test_hang_watch_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_hang_watch_test.exe")
    system_dir = os.path.join(DC, "src", "system")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-I", system_dir, "-o", exe,
                    os.path.join(DC, "tests", "host", "test_hang_watch.c"),
                    os.path.join(system_dir, "hang_watch_check.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
