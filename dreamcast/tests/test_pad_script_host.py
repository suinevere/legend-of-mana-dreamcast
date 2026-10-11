#----------------------
# | test_pad_script_host.py
# | Description: Builds and runs the host C test for the pad script interpreter, and checks gen_pad_script.py rejects a bad button with the line number on stderr.
# | Author: suinevere
# | Dependencies: tests/host/test_pad_script.c, src/input/pad_script.c, tools/gen_pad_script.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import shutil
import subprocess
import sys
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def test_pad_script_host():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    exe = os.path.join(tempfile.gettempdir(), "lom_pad_script_test.exe")
    input_dir = os.path.join(DC, "src", "input")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wno-builtin-declaration-mismatch", "-Wall", "-Wextra",
                    "-DLOM_HOST_TEST",
                    "-I", input_dir,
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_pad_script.c"),
                    os.path.join(input_dir, "pad_script.c")], check=True)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout


def test_gen_pad_script_rejects_bad_button():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "bad.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("wait state 2\npress XSQUARE 3\n")
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                             capture_output=True, text=True)
    assert result.returncode == 1
    assert "line 2" in result.stderr
    assert not os.path.exists(header_path)
