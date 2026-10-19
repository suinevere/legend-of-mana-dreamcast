#----------------------
# | test_scene_sweep_host.py
# | Description: Builds and runs the host C test for the scene sweep's state machine and range parser.
# | Author: suinevere
# | Dependencies: tests/host/test_scene_sweep.c, src/system/scene_sweep.c
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


def test_scene_sweep_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_scene_sweep_test.exe")
    system_dir = os.path.join(DC, "src", "system")
    subprocess.run([cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-I", system_dir, "-o", exe,
                    os.path.join(DC, "tests", "host", "test_scene_sweep.c"),
                    os.path.join(system_dir, "scene_sweep.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
