#----------------------
# | test_vmu_game_host.py
# | Description: Builds and runs the host C test for the VMU GAME classifier that decides whether a VMU can take Ring Ring Land.
# | Author: suinevere
# | Dependencies: tests/host/test_vmu_game.c, src/card/vmu_game.c
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


def test_vmu_game_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_vmu_game_test.exe")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-I", os.path.join(DC, "src", "card"), "-o", exe,
                    os.path.join(DC, "tests", "host", "test_vmu_game.c"),
                    os.path.join(DC, "src", "card", "vmu_game.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "vmu_game ok" in result.stdout
