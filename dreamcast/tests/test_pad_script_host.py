#----------------------
# | test_pad_script_host.py
# | Description: Builds and runs the host C test for the pad script interpreter, checks gen_pad_script.py's error paths, and checks mash state/marker lines compile to the right step literal.
# | Author: suinevere
# | Dependencies: tests/host/test_pad_script.c, src/input/pad_script.c, src/system/watch.c, tools/gen_pad_script.py
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
    system_dir = os.path.join(DC, "src", "system")
    subprocess.run([cc, "-std=gnu11", "-fno-builtin", "-Wno-builtin-declaration-mismatch", "-Wall", "-Wextra",
                    "-DLOM_HOST_TEST",
                    "-I", input_dir, "-I", system_dir,
                    "-o", exe,
                    os.path.join(DC, "tests", "host", "test_pad_script.c"),
                    os.path.join(input_dir, "pad_script.c"),
                    os.path.join(system_dir, "watch.c")], check=True)
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


def test_gen_pad_script_rejects_short_mash_period():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "short_mash.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("wait state 2\nmash CROSS 7 state 7\n")
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                             capture_output=True, text=True)
    assert result.returncode == 1
    assert "line 2" in result.stderr
    assert not os.path.exists(header_path)


def test_gen_pad_script_mash_state_compiles_to_the_right_literal():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "mash_state.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("mash CROSS 30 state 7\n")
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                             capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_MASH, 30, PAD_SCRIPT_BTN_CROSS, "", 7 },' in header


def test_gen_pad_script_mash_marker_keeps_the_full_watcher_text():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "mash_marker.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("mash CROSS 30 marker field scene 857\n")
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                             capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_MASH, 30, PAD_SCRIPT_BTN_CROSS, "field scene 857", 0 },' in header

def test_gen_pad_script_scene_and_battle_compile_to_their_ops():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "jump.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("scene 71 2"+chr(10)+"battle"+chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                             capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_SCENE, 71, 0, "", 2 },' in header
    assert '{ PAD_OP_BATTLE, 0, 0, "", 0 },' in header
