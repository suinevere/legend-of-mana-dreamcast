#----------------------
# | test_pet_transfer_flycast.py
# | Description: Flycast checks of R2's pet transfer: CARDA mode 2 writes Ring Ring Land with the fixture pet to the VMU as the JP game fills it,
# |              CARDA mode 3 brings the pet and its rewards back and erases the file, and the ranch's ring menu offers Ring Ring Land and opens CARDA.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tools/vmu_image.py, stale_image.py, pocketstation/tools/psk_file.py, lom_pet_send_test.cdi, lom_pet_return_test.cdi, lom_pet_ranch_test.cdi
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
sys.path.insert(0, os.path.join(os.path.dirname(DC), "pocketstation", "tools"))

import flycast_run  # noqa: E402
import psk_file  # noqa: E402
import stale_image  # noqa: E402
import vmu_image  # noqa: E402

#----------------------
# | RRL_NAME / RES1506 / PET / PLACED_LANDS / MAXED_MANA / REWARDS / GROWTH
# | Description: The Ring Ring Land file name, its CD resource, the pet and land state field_watch_unit.c plants, and the rewards and growth the return test writes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RRL_NAME = "BASLUSP01013"
RES1506 = os.path.join(DC, "build-test", "cd-data", "RES", "05", "RES1506.BIN")
PET = b"Rabi" + psk_file.REFERENCE_PET[4:0x16] + bytes([7]) + psk_file.REFERENCE_PET[0x17:0x18] + bytes([9]) + psk_file.REFERENCE_PET[0x19:]
PLACED_LANDS = frozenset({1, 2, 3, 5, 6, 7, 9, 15, 24, 31})
MAXED_MANA = {9: 0x85, 10: 0x02, 31: 0xFF}
REWARDS = (0x10, 0x11)
GROWTH = 50


#----------------------
# | run
# | Description: Runs a scripted disc in Flycast with a VMU image and returns its exit code, log lines and the VMU it leaves.
# | Author: suinevere
# | Dependencies: flycast_run.run_flycast, stale_image.require_fresh_test_image
# | Globals: DC
# | Params: image_name -- disc, vmu_in -- VMU image bytes, until -- log line ending the run, seconds -- time limit, tmp_path_factory, label -- folder name
# | Returns: (exit code, log lines, VMU bytes, capture folder)
#----------------------
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


#----------------------
# | transfer_line
# | Description: The fields of the "transfer result" line field_watch_unit.c logs when CARDA closes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: lines -- log lines
# | Returns: (CARDA result, which FIELD has already reset to 0; pet0 first name byte; pet0 unique id; reward item counts)
#----------------------
def transfer_line(lines):
    words = next(line for line in lines if "transfer result" in line).split()
    at = words.index("result")
    return int(words[at + 1]), int(words[at + 3], 16), int(words[at + 4], 16), (int(words[at + 6]), int(words[at + 7]))


@pytest.fixture(scope="module")
def send_run(tmp_path_factory):
    return run("lom_pet_send_test.cdi", vmu_image.blank(), "LOM: capture send-done", 1200, tmp_path_factory, "send")


@pytest.fixture(scope="module")
def ranch_run(tmp_path_factory):
    return run("lom_pet_ranch_test.cdi", vmu_image.blank(), "LOM: capture ranch-done", 1200, tmp_path_factory, "ranch")


@pytest.fixture(scope="module")
def return_run(send_run, tmp_path_factory):
    sent = vmu_image.ps1_file(send_run[2], RRL_NAME)
    if sent is None:
        pytest.skip("the send run wrote no Ring Ring Land file")
    data = bytearray(sent)
    struct.pack_into("<iI", data, psk_file.TRANSFER_OFFSET, GROWTH, len(REWARDS))
    data[psk_file.TRANSFER_OFFSET + 8:psk_file.TRANSFER_OFFSET + 8 + len(REWARDS)] = bytes(REWARDS)
    vmu_in = vmu_image.set_ps1_file(send_run[2], RRL_NAME, bytes(data))
    return run("lom_pet_return_test.cdi", vmu_in, "LOM: capture return-done", 1200, tmp_path_factory, "return")


def test_send_writes_ring_ring_land_as_the_jp_game_fills_it(send_run):
    code, lines, img, _ = send_run
    assert code == 0, lines[-20:]
    sent = vmu_image.ps1_file(img, RRL_NAME)
    assert sent is not None and len(sent) == 0xC000
    with open(RES1506, "rb") as f:
        res = f.read()
    kept = vmu_image.PET_KEPT_BYTES
    assert sent[:kept] == psk_file.pet_transfer(res, 7, 9, PET, PLACED_LANDS, MAXED_MANA)[:kept]
    assert sent[kept:] == bytes(0xC000 - kept)
    assert len(vmu_image.chain(img, vmu_image.vmu_name_for_ps1(RRL_NAME))) <= 5


def test_send_empties_the_ranch_slot(send_run):
    _, lines, _, _ = send_run
    _, name0, _, _ = transfer_line(lines)
    assert name0 == 0


def test_return_brings_the_pet_and_its_rewards_home_and_erases_the_file(return_run):
    code, lines, img, _ = return_run
    assert code == 0, lines[-20:]
    _, name0, unique_id, counts = transfer_line(lines)
    assert name0 == PET[0] and unique_id == struct.unpack_from("<I", PET, 0x5C)[0]
    assert counts == (1, 1)
    assert vmu_image.ps1_file(img, RRL_NAME) is None


def test_the_ranch_menu_offers_ring_ring_land_and_opens_carda(ranch_run):
    code, lines, _, _ = ranch_run
    assert code == 0, lines[-20:]
    picked = next(n for n, line in enumerate(lines) if "capture ranch-sub-1" in line)
    assert any("field modal 3" in line for line in lines[picked:])
