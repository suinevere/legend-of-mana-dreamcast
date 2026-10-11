#----------------------
# | test_build.py
# | Description: Builds the Dreamcast port in lom-dc and checks lom.elf is a little-endian SuperH ELF and lom.cdi exists.
# | Author: suinevere
# | Dependencies: compile.sh, makefile
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import shutil
import subprocess

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)


def test_build_produces_sh4_little_endian_elf_and_cdi():
    if shutil.which("docker") is None:
        pytest.skip("no docker")
    if subprocess.run(["docker", "image", "inspect", "lom-dc"], capture_output=True).returncode != 0:
        pytest.skip("lom-dc image not built")
    build = subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-dc",
                            "bash", "-lc", "make -C dreamcast all"], capture_output=True, text=True)
    assert build.returncode == 0, build.stdout[-4000:] + build.stderr[-4000:]
    with open(os.path.join(DC, "lom.elf"), "rb") as f:
        header = f.read(20)
    assert header[:4] == b"\x7fELF"
    assert header[5] == 1
    assert int.from_bytes(header[18:20], "little") == 42
    assert os.path.getsize(os.path.join(DC, "lom.cdi")) > 0
