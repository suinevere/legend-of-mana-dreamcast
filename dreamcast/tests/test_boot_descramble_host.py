#----------------------
# | test_boot_descramble_host.py
# | Description: Builds and runs the host C test for the boot binary descrambler the soft reset uses on CD-R discs.
# | Author: suinevere
# | Dependencies: tests/host/test_boot_descramble.c, src/system/boot_descramble.c
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


def test_boot_descramble_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_dc_boot_descramble_test.exe")
    system_dir = os.path.join(DC, "src", "system")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-I", system_dir, "-o", exe,
                    os.path.join(DC, "tests", "host", "test_boot_descramble.c"),
                    os.path.join(system_dir, "boot_descramble.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
