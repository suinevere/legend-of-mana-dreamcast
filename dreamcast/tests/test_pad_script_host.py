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


def test_gen_pad_script_open_compiles_to_its_op_and_target_index():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "open.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("open zukan" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_OPEN, 2, 0, "", 0 },' in header


def test_gen_pad_script_rejects_unknown_open_target():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "open.pad")
    with open(script_path, "w") as f:
        f.write("open movie" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path,
                             os.path.join(out_dir, "out.h")], capture_output=True, text=True)
    assert result.returncode == 1
    assert "movie" in result.stderr


def test_gen_pad_script_gameover_compiles_to_its_op():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "gameover.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("gameover" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_GAMEOVER, 0, 0, "", 0 },' in header


def test_gen_pad_script_tables_every_script_by_name():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    first = os.path.join(out_dir, "new_game.pad")
    second = os.path.join(out_dir, "card_save.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(first, "w") as f:
        f.write("wait 1" + chr(10))
    with open(second, "w") as f:
        f.write("wait 2" + chr(10) + "open carda" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), first, second, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert "static const PadScriptStep pad_script_steps_new_game[] = {" in header
    assert "static const PadScriptStep pad_script_steps_card_save[] = {" in header
    assert "static const PadScriptChoice pad_script_scripts[] = {" in header
    assert '    { "new_game", pad_script_steps_new_game, 1 },' in header
    assert '    { "card_save", pad_script_steps_card_save, 2 },' in header
    assert "static const unsigned pad_script_script_count = 2;" in header


def test_gen_pad_script_probe_movie_compiles_to_its_op():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "probe.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("probe movie" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_PROBE_MOVIE, 0, 0, "", 0 },' in header


def test_gen_pad_script_probe_rejects_unknown_target():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "probe.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("wait 1" + chr(10) + "probe nothing" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode != 0
    assert "line 2" in result.stderr


def test_gen_pad_script_movies_compiles_to_its_op():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "movies.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("movies" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_MOVIES, 0, 0, "", 0 },' in header


def test_gen_pad_script_fieldmovie_compiles_to_its_op():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "fieldmovie.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("fieldmovie" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_FIELD_MOVIE, 0, 0, "", 0 },' in header


def test_gen_pad_script_effects_compiles_to_its_op():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "effects.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("effects" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_EFFECTS, 0, 0, "", 0 },' in header


def test_gen_pad_script_sweep_compiles_to_its_op():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "sweep.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("sweep" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_SWEEP, 0, 0, "", 0 },' in header


def test_gen_pad_script_sweep_rejects_arguments():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "sweep.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("sweep 3" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 1
    assert "line 1" in result.stderr


def test_gen_pad_script_tone_compiles_to_its_op():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "tone.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("tone 1000 3" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_TONE, 1000, 0, "", 3 },' in header


def test_gen_pad_script_tone_rejects_a_bad_rate():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "tone.pad")
    with open(script_path, "w") as f:
        f.write("tone 30000 3" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path,
                             os.path.join(out_dir, "out.h")], capture_output=True, text=True)
    assert result.returncode == 1
    assert "tone" in result.stderr


def test_gen_pad_script_longvoice_compiles_to_its_op():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "longvoice.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("longvoice 3" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_LONGVOICE, 3, 0, "", 0 },' in header


def _compile(text):
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "sound.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write(text)
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    header = open(header_path).read() if result.returncode == 0 else ""
    return result, header


def test_gen_pad_script_music_ops_compile():
    result, header = _compile("music 23" + chr(10) + "musicoff" + chr(10) + "sfxset 81" + chr(10))
    assert result.returncode == 0, result.stderr
    assert '{ PAD_OP_MUSIC, 23, 0, "", 0 },' in header
    assert '{ PAD_OP_MUSICOFF, 0, 0, "", 0 },' in header
    assert '{ PAD_OP_SFXSET, 81, 0, "", 0 },' in header


def test_gen_pad_script_music_rejects_a_non_music_resource():
    result, _ = _compile("music 81" + chr(10))
    assert result.returncode == 1
    assert "music" in result.stderr


def test_gen_pad_script_sfxset_rejects_a_music_resource():
    result, _ = _compile("sfxset 23" + chr(10))
    assert result.returncode == 1
    assert "sfxset" in result.stderr


def test_gen_pad_script_boothold_compiles_to_its_op():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "boothold.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("boothold" + chr(10) + "wait 1" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    with open(header_path) as f:
        header = f.read()
    assert '{ PAD_OP_BOOTHOLD, 0, 0, "", 0 },' in header


def test_gen_pad_script_boothold_rejects_when_not_first():
    out_dir = tempfile.mkdtemp(prefix="lom-pad-script-")
    script_path = os.path.join(out_dir, "boothold.pad")
    header_path = os.path.join(out_dir, "pad_script_table.h")
    with open(script_path, "w") as f:
        f.write("wait 1" + chr(10) + "wait 1" + chr(10) + "boothold" + chr(10))
    result = subprocess.run([sys.executable, os.path.join(DC, "tools", "gen_pad_script.py"), script_path, header_path],
                            capture_output=True, text=True)
    assert result.returncode == 1
    assert "line 3" in result.stderr
    assert not os.path.exists(header_path)


def test_gen_pad_script_sound_sets_match_sound_banks():
    sys.path.insert(0, os.path.join(DC, "tools"))
    sys.path.insert(0, os.path.join(DC, "..", "port", "tools"))
    sound_banks = pytest.importorskip("sound_banks")
    import gen_pad_script
    kinds = dict(sound_banks.SOUND_RESOURCES)
    assert gen_pad_script.MUSIC_RESOURCES == {r for r, k in kinds.items() if k == "music"}
    assert gen_pad_script.SFXSET_RESOURCES == {r for r, k in kinds.items() if k in ("effect_set", "bank", "sfx_set", "weapon_sfx")}


def test_sound_sweep_script_is_generated():
    sys.path.insert(0, os.path.join(DC, "tools"))
    sys.path.insert(0, os.path.join(DC, "..", "port", "tools"))
    pytest.importorskip("sound_banks")
    import gen_sound_sweep
    with open(os.path.join(DC, "tests", "pad_scripts", "sound_sweep.pad")) as f:
        assert f.read() == gen_sound_sweep.render(), "rerun dreamcast/tools/gen_sound_sweep.py"
