#----------------------
# | test_watch_host.py
# | Description: Builds and runs the host C test for the change-printing watcher, and compile-checks the test-image-only field_watch_unit.c against FIELD's real headers without linking it.
# | Author: suinevere
# | Dependencies: tests/host/test_watch.c, src/system/watch.c, src/overlays/field_watch_unit.c
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


def test_watch_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_watch_test.exe")
    system_dir = os.path.join(DC, "src", "system")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wno-builtin-declaration-mismatch", "-Wall", "-Wextra",
                    "-I", system_dir,
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_watch.c"),
                    os.path.join(system_dir, "watch.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout


def test_field_watch_unit_host_compiles():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    out_dir = tempfile.mkdtemp(prefix="lom-field-watch-")
    obj = os.path.join(out_dir, "field_watch_unit.o")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wno-builtin-declaration-mismatch", "-w",
                    "-DLOM_PAD_SCRIPT",
                    "-I", os.path.join(DC, "src", "system"), "-I", os.path.join(DC, "src", "card"),
                    "-I", os.path.join(REPO, "include"), "-I", os.path.join(REPO, "include", "sdk"),
                    "-I", os.path.join(REPO, "src"),
                    "-c", "-o", obj,
                    os.path.join(DC, "src", "overlays", "field_watch_unit.c")], check=True)
    assert os.path.exists(obj)
