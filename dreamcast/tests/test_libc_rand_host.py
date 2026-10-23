#----------------------
# | test_libc_rand_host.py
# | Description: Builds and runs tests/host/test_libc_rand.c against port/psyq/libc_rand.c with the host compiler.
# | Author: suinevere
# | Dependencies: gcc or cc, port/psyq/libc_rand.c, tests/host/test_libc_rand.c
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


def test_libc_rand_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_dc_libc_rand_test.exe")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wall", "-Wextra", "-Werror", "-o", exe,
                    os.path.join(DC, "tests", "host", "test_libc_rand.c"),
                    os.path.join(REPO, "port", "psyq", "libc_rand.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "ok" in result.stdout
