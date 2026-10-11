#----------------------
# | test_exception_host.py
# | Description: Builds and runs the host C test for the Dreamcast exception halt text.
# | Author: suinevere
# | Dependencies: tests/host/test_dc_exception.c, src/system/dc_exception_format.c
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


def test_exception_format_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_dc_exception_test.exe")
    system = os.path.join(DC, "src", "system")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-I", system, "-o", exe,
                    os.path.join(DC, "tests", "host", "test_dc_exception.c"),
                    os.path.join(system, "dc_exception_format.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
