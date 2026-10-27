#----------------------
# | test_disc_input.py
# | Description: Checks port/kit/disc_input.py resolves cue sheets and bins and reports a wrong dump clearly.
# | Author: suinevere
# | Dependencies: port/kit/disc_input.py
#----------------------
import hashlib
import os
import sys

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", ".."))
from port.kit import disc_input  # noqa: E402


def test_cue_points_at_bin_in_same_folder(tmp_path):
    (tmp_path / "Legend of Mana (USA).bin").write_bytes(b"x" * 2352)
    cue = tmp_path / "Legend of Mana (USA).cue"
    cue.write_text('FILE "Legend of Mana (USA).bin" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n')
    assert disc_input.resolve(str(cue)) == str(tmp_path / "Legend of Mana (USA).bin")


def test_cue_with_uppercase_extension_and_crlf(tmp_path):
    (tmp_path / "lom.BIN").write_bytes(b"x")
    cue = tmp_path / "lom.CUE"
    cue.write_bytes(b'FILE "lom.BIN" BINARY\r\n  TRACK 01 MODE2/2352\r\n')
    assert disc_input.resolve(str(cue)).endswith("lom.BIN")


def test_first_data_track_wins(tmp_path):
    (tmp_path / "a.bin").write_bytes(b"x")
    (tmp_path / "b.bin").write_bytes(b"y")
    cue = tmp_path / "x.cue"
    cue.write_text('FILE "a.bin" BINARY\n  TRACK 01 MODE2/2352\nFILE "b.bin" BINARY\n  TRACK 02 AUDIO\n')
    assert disc_input.resolve(str(cue)).endswith("a.bin")


def test_bin_given_directly(tmp_path):
    path = tmp_path / "x.bin"
    path.write_bytes(b"x")
    assert disc_input.resolve(str(path)) == str(path)


def test_quoted_path_with_spaces(tmp_path):
    path = tmp_path / "my games" / "x.bin"
    path.parent.mkdir()
    path.write_bytes(b"x")
    assert disc_input.resolve('"%s" ' % path) == str(path)


def test_missing_bin_named_in_cue(tmp_path):
    cue = tmp_path / "x.cue"
    cue.write_text('FILE "gone.bin" BINARY\n  TRACK 01 MODE2/2352\n')
    with pytest.raises(disc_input.DiscError, match="gone.bin"):
        disc_input.resolve(str(cue))


def test_wrong_sha1_message(tmp_path):
    path = tmp_path / "x.bin"
    path.write_bytes(b"not the game")
    with pytest.raises(disc_input.DiscError) as err:
        disc_input.check(str(path), "0" * 40)
    text = str(err.value)
    assert "0" * 40 in text and hashlib.sha1(b"not the game").hexdigest() in text and "redump" in text.lower()


def test_right_sha1_passes(tmp_path):
    path = tmp_path / "x.bin"
    path.write_bytes(b"the game")
    disc_input.check(str(path), hashlib.sha1(b"the game").hexdigest())


def test_unsupported_extension(tmp_path):
    path = tmp_path / "x.iso"
    path.write_bytes(b"x")
    with pytest.raises(disc_input.DiscError, match=".cue or .bin"):
        disc_input.resolve(str(path))
