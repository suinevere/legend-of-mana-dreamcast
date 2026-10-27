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

    def make_gdi(kit, image, out, overwrite=False, progress=None, program_version=None):
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


def test_pauses_when_launched_alone_with_a_dragged_file(monkeypatch, capsys):
    monkeypatch.setattr(lom_dc_kit.fill, "make_gdi", lambda *a, **k: "out")
    monkeypatch.setattr(lom_dc_kit.disc_input, "resolve", lambda p: p)
    monkeypatch.setattr(lom_dc_kit, "_launched_alone", lambda: True)
    stdin = io.StringIO("\n")
    assert lom_dc_kit.main(["C:\\games\\lom.cue"], stdin=stdin, kit_dir="kit") == 0
    assert "Press Enter" in capsys.readouterr().out
    assert stdin.read() == ""


def test_no_pause_from_a_terminal_with_args(monkeypatch, capsys):
    monkeypatch.setattr(lom_dc_kit.fill, "make_gdi", lambda *a, **k: "out")
    monkeypatch.setattr(lom_dc_kit.disc_input, "resolve", lambda p: p)
    monkeypatch.setattr(lom_dc_kit, "_launched_alone", lambda: False)
    assert lom_dc_kit.main(["lom.cue"], stdin=io.StringIO(""), kit_dir="kit") == 0
    assert "Press Enter" not in capsys.readouterr().out


def test_unexpected_error_is_one_line(monkeypatch, capsys):
    import zlib

    def corrupt(*a, **k):
        raise zlib.error("invalid stored block lengths")
    monkeypatch.setattr(lom_dc_kit.fill, "make_gdi", corrupt)
    monkeypatch.setattr(lom_dc_kit.disc_input, "resolve", lambda p: p)
    assert lom_dc_kit.main(["x.cue"], kit_dir="kit", interactive=False) == 1
    err = capsys.readouterr().err
    assert "lom-dc-kit:" in err and "invalid stored block lengths" in err and "Traceback" not in err


def test_passes_program_version_to_fill(monkeypatch):
    seen = {}
    monkeypatch.setattr(lom_dc_kit.fill, "make_gdi", lambda *a, **k: seen.update(k) or "out")
    monkeypatch.setattr(lom_dc_kit.disc_input, "resolve", lambda p: p)
    lom_dc_kit.main(["x.cue"], kit_dir="kit", interactive=False)
    assert seen["program_version"] == lom_dc_kit.VERSION
