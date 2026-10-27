#----------------------
# | test_cli.py
# | Description: Checks the lom-dc-kit command line: usage, the drag-and-drop prompt, and errors shown as one line with a non-zero exit.
# | Author: suinevere
# | Dependencies: port/kit/lom_dc_kit.py
#----------------------
import io
import os
import sys

sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", "..")))
from port.kit import fill, lom_dc_kit  # noqa: E402


def test_prompts_for_path_without_args(monkeypatch, capsys):
    seen = {}

    def make_gdi(kit, image, out, overwrite=False, progress=None):
        seen["image"] = image
        seen["out"] = out
        return out
    monkeypatch.setattr(lom_dc_kit.fill, "make_gdi", make_gdi)
    monkeypatch.setattr(lom_dc_kit.disc_input, "resolve", lambda p: p)
    stdin = io.StringIO('"C:\\My Games\\Legend of Mana.cue"\n\n')
    code = lom_dc_kit.main([], stdin=stdin, kit_dir="kit", interactive=True)
    assert code == 0
    assert seen["image"] == "C:\\My Games\\Legend of Mana.cue"
    assert seen["out"].endswith("Legend of Mana (Dreamcast)")
    printed = capsys.readouterr().out
    assert "Drag" in printed and "Press Enter" in printed
    assert stdin.read() == ""


def test_error_is_one_line(monkeypatch, capsys):
    def boom(*a, **k):
        raise fill.KitError("bad thing")
    monkeypatch.setattr(lom_dc_kit.fill, "make_gdi", boom)
    monkeypatch.setattr(lom_dc_kit.disc_input, "resolve", lambda p: p)
    assert lom_dc_kit.main(["x.cue"], kit_dir="kit", interactive=False) == 1
    assert "lom-dc-kit: bad thing" in capsys.readouterr().err


def test_no_args_without_terminal_prints_usage(capsys):
    assert lom_dc_kit.main([], stdin=io.StringIO(""), kit_dir="kit", interactive=False) == 2
    assert "usage" in capsys.readouterr().out.lower()


def test_version_flag(capsys):
    assert lom_dc_kit.main(["--version"], kit_dir="kit") == 0
    assert lom_dc_kit.VERSION in capsys.readouterr().out
