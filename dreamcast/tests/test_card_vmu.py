#----------------------
# | test_card_vmu.py
# | Description: Flycast checks of D5's VMU saves: CARDA writes a save as a VMS package, CLOAD loads it back into Home, a nearly full VMU takes the card-full path and a corrupt save is refused, never halting.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tools/vmu_image.py, stale_image.py, lom_card_save_test.cdi, lom_card_load_test.cdi, tests/pad_scripts/card_save.pad, card_load.pad
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import struct
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import flycast_run
import stale_image
import vmu_image


def package(data):
    icons, _, _, _, data_len = struct.unpack_from("<HHHHI", data, 0x40)
    start = 128 + 512 * icons
    return {"desc_long": data[16:48].rstrip(b" \0").decode("ascii", "replace"), "icons": icons,
            "data": data[start:start + data_len]}


def lom_files(img):
    return [(name, data) for name, data in vmu_image.files(img) if name.startswith("LOM")]


def run(image_name, vmu_in, until, seconds, tmp_path_factory, label):
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    stale_image.require_fresh_test_image(DC)
    out_dir = tmp_path_factory.mktemp(label)
    vmu_out = os.path.join(str(out_dir), "vmu.bin")
    code, lines = flycast_run.run_flycast(os.path.join(DC, image_name), seconds=seconds, until=until,
                                          captures=str(out_dir), vmu_in=vmu_in, vmu_out=vmu_out)
    with open(os.path.join(str(out_dir), "log.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    with open(vmu_out, "rb") as f:
        return code, lines, f.read(), str(out_dir)


def between(lines, first, last):
    i = next(n for n, line in enumerate(lines) if first in line)
    j = next(n for n, line in enumerate(lines) if last in line)
    return lines[i:j + 1]


@pytest.fixture(scope="module")
def save_run(tmp_path_factory):
    reuse = os.environ.get("LOM_CARD_SAVE_VMU")
    if reuse:
        with open(reuse, "rb") as f:
            return 0, ["LOM: capture carda-saved"], f.read(), ""
    return run("lom_card_save_test.cdi", vmu_image.blank(), "LOM: capture carda-saved", 900, tmp_path_factory, "save")


@pytest.fixture(scope="module")
def load_run(save_run, tmp_path_factory):
    return run("lom_card_load_test.cdi", save_run[2], "LOM: capture carda-second-saved", 600, tmp_path_factory, "load")


@pytest.fixture(scope="module")
def full_run(save_run, tmp_path_factory):
    img = vmu_image.add(save_run[2], "OTHERGAME", b"\x5A" * (vmu_image.free_blocks(save_run[2]) - 5) * 512)
    return img, run("lom_card_load_test.cdi", img, "LOM: capture carda-second-saved", 600, tmp_path_factory, "full")


@pytest.fixture(scope="module")
def broken_run(tmp_path_factory):
    img = vmu_image.add(vmu_image.blank(), "LOM0-1", b"LEGEND OF MANA  " + b"\xA5" * (35 * 512 - 16))
    return img, run("lom_card_load_test.cdi", img, "LOM: capture cload-after", 300, tmp_path_factory, "broken")


def test_carda_saves_a_ps1_save_as_one_vms_package(save_run):
    code, lines, img, _ = save_run
    assert code == 0, lines[-20:]
    saves = lom_files(img)
    assert len(saves) == 1, saves
    pkg = package(saves[0][1])
    assert pkg["desc_long"].startswith("BASLUS-01013")
    assert len(pkg["data"]) == 0x4000 and pkg["data"][:2] == b"SC"
    assert pkg["icons"] >= 1


def test_cload_loads_that_save_back_into_home(load_run):
    code, lines, _, _ = load_run
    assert code == 0, lines[-20:]
    assert any("field scene 19" in line for line in between(lines, "capture cload-prompt-yes", "capture carda-second-list"))


def test_a_second_save_adds_a_second_file_beside_the_first(save_run, load_run):
    first = {name for name, _ in lom_files(save_run[2])}
    after = lom_files(load_run[2])
    assert len(after) == 2 and first < {name for name, _ in after}
    assert all(package(data)["data"][:2] == b"SC" for _, data in after)


def test_a_full_vmu_takes_the_card_full_path_and_writes_nothing(full_run):
    before, (code, lines, img, _) = full_run
    assert code == 0, lines[-20:]
    assert any("field scene 19" in line for line in lines)
    assert vmu_image.files(img) == vmu_image.files(before)


def test_a_corrupt_save_is_refused_without_a_halt(broken_run):
    before, (code, lines, img, _) = broken_run
    assert code == 0, lines[-20:]
    assert not any("field scene 19" in line for line in between(lines, "capture cload-list", "capture cload-after"))
    assert vmu_image.files(img) == vmu_image.files(before)
