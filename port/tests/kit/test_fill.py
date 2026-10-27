#----------------------
# | test_fill.py
# | Description: Checks port/kit/fill.py rebuilds tracks from a synthetic kit and refuses unsafe runs; with the disc and a built GDI, that the real kit round-trips byte for byte.
# | Author: suinevere
# | Dependencies: port/kit/fill.py, port/kit/make_template.py, disc/us/LegendOfMana.bin (optional), dreamcast/gdi (optional)
#----------------------
import hashlib
import os
import sys

import pytest

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
sys.path.insert(0, ROOT)
from port.kit import fill, make_template, manifest, sector  # noqa: E402

IMAGE = os.path.join(ROOT, "disc", "us", "LegendOfMana.bin")
GDI = os.path.join(ROOT, "dreamcast", "gdi")
ELF = os.path.join(ROOT, "dreamcast", "lom.elf")


def _sha(data):
    return hashlib.sha1(data).hexdigest()


def _kit(tmp_path, wrong_sha=False, raw_sector=None):
    kit = tmp_path / "kit"
    kit.mkdir()
    head = b"IP" + bytes(2046)
    sectors = [sector.encode(45000 + i, {0: head, 1: b"0123456789" + bytes(2038)}.get(i, bytes(2048))) for i in range(4)]
    runs = [(0, head)]
    if raw_sector is not None:
        sectors[raw_sector] = b"\x5a" * 2352
        runs.append((-1 - raw_sector, sectors[raw_sector]))
    full = b"".join(sectors)
    for name, body in (("track01.bin", b"a" * 2352), ("track02.raw", b"b" * 2352), ("disc.gdi", b"3\n")):
        (kit / name).write_bytes(body)
    manifest.write_literals(str(kit / "track03.lit"), runs)
    manifest.write(str(kit), {
        "version": manifest.VERSION, "kit_tag": "v-test", "git_sha": "0", "ps1_sha1": "f" * 40,
        "tracks": [{"name": "track01.bin", "lba": 0, "sectors": 1, "sha1": _sha(b"a" * 2352)},
                   {"name": "track02.raw", "lba": 600, "sectors": 1, "sha1": _sha(b"b" * 2352)},
                   {"name": "track03.bin", "lba": 45000, "sectors": 4, "sha1": "0" * 40 if wrong_sha else _sha(full)}],
        "holes": [{"offset": 2048, "length": 10, "source": "cd/X.BIN", "source_offset": 0}],
        "raw_sectors": [] if raw_sector is None else [raw_sector]})
    return kit, full


def _quiet(*args):
    pass


@pytest.fixture
def stub(monkeypatch):
    monkeypatch.setattr(fill.sources, "build", lambda image, work: {"cd/X.BIN": b"0123456789"})
    monkeypatch.setattr(fill.disc_input, "check", lambda path, expected, progress=None: None)


def test_rebuilds_synthetic_track(tmp_path, stub):
    kit, full = _kit(tmp_path)
    out = fill.make_gdi(str(kit), "x.bin", str(tmp_path / "out"), progress=_quiet)
    assert open(os.path.join(out, "track03.bin"), "rb").read() == full
    assert open(os.path.join(out, "disc.gdi"), "rb").read() == b"3\n"


def test_raw_sector_written_verbatim(tmp_path, stub):
    kit, full = _kit(tmp_path, raw_sector=3)
    out = fill.make_gdi(str(kit), "x.bin", str(tmp_path / "out"), progress=_quiet)
    assert open(os.path.join(out, "track03.bin"), "rb").read() == full


def test_refuses_existing_output(tmp_path, stub):
    kit, _ = _kit(tmp_path)
    (tmp_path / "out").mkdir()
    with pytest.raises(fill.KitError, match="already exists"):
        fill.make_gdi(str(kit), "x.bin", str(tmp_path / "out"), progress=_quiet)


def test_overwrite_replaces_existing_output(tmp_path, stub):
    kit, full = _kit(tmp_path)
    (tmp_path / "out").mkdir()
    (tmp_path / "out" / "stale.txt").write_text("old")
    out = fill.make_gdi(str(kit), "x.bin", str(tmp_path / "out"), overwrite=True, progress=_quiet)
    assert not os.path.exists(os.path.join(out, "stale.txt"))
    assert open(os.path.join(out, "track03.bin"), "rb").read() == full


def test_failed_check_leaves_no_output(tmp_path, stub):
    kit, _ = _kit(tmp_path, wrong_sha=True)
    with pytest.raises(fill.KitError, match="track03.bin"):
        fill.make_gdi(str(kit), "x.bin", str(tmp_path / "out"), progress=_quiet)
    assert not (tmp_path / "out").exists() and not (tmp_path / "out.tmp").exists()


def test_manifest_version_mismatch(tmp_path, stub):
    kit, _ = _kit(tmp_path)
    data = manifest.read(str(kit))
    data["version"] = 99
    manifest.write(str(kit), data)
    with pytest.raises(fill.KitError, match="99"):
        fill.make_gdi(str(kit), "x.bin", str(tmp_path / "out"), progress=_quiet)


def test_missing_kit_folder(tmp_path, stub):
    with pytest.raises(fill.KitError, match="kit"):
        fill.make_gdi(str(tmp_path / "nowhere"), "x.bin", str(tmp_path / "out"), progress=_quiet)


def test_low_disk_space_message(tmp_path, stub, monkeypatch):
    kit, _ = _kit(tmp_path)
    monkeypatch.setattr(fill.shutil, "disk_usage", lambda p: (0, 0, 1024))
    with pytest.raises(fill.KitError, match="MB free"):
        fill.make_gdi(str(kit), "x.bin", str(tmp_path / "out"), progress=_quiet)


@pytest.mark.skipif(not (os.path.exists(IMAGE) and os.path.exists(os.path.join(GDI, "track03.bin")) and os.path.exists(ELF)),
                    reason="needs the US disc and a built GDI")
def test_real_round_trip(tmp_path):
    kit = tmp_path / "kit"
    make_template.build_kit(GDI, ELF, IMAGE, str(kit), "v-test", "0")
    out = fill.make_gdi(str(kit), IMAGE, str(tmp_path / "gdi"), progress=_quiet)
    for name in ("track01.bin", "track02.raw", "track03.bin", "disc.gdi"):
        assert _sha(open(os.path.join(out, name), "rb").read()) == _sha(open(os.path.join(GDI, name), "rb").read()), name
