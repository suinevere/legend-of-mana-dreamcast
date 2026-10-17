#----------------------
# | test_card_none_host.py
# | Description: Builds and runs the host C test for the Dreamcast's no-memory-card backend.
# | Author: suinevere
# | Dependencies: tests/host/test_card_none.c, src/card/card_none.c, include/sdk
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


def test_card_none_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_card_none_test.exe")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wno-builtin-declaration-mismatch", "-Wall", "-Wextra",
                    "-DEnterCriticalSection=lom_EnterCriticalSection", "-DExitCriticalSection=lom_ExitCriticalSection",
                    "-I", os.path.join(REPO, "include", "sdk"),
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_card_none.c"),
                    os.path.join(DC, "src", "card", "card_none.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
