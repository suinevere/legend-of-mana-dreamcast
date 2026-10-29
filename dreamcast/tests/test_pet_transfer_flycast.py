#----------------------
# | test_pet_transfer_flycast.py
# | Description: Flycast checks of R2's pet transfer: CARDA mode 2 writes Ring Ring Land with the fixture pet to the VMU as the JP game fills it,
# |              CARDA mode 3 brings the pet and its rewards back and erases the file, and the ranch's ring menu offers Ring Ring Land and opens CARDA;
# |              a send also installs Ring Ring Land's VMU GAME from block 0, rewriting an older one and replacing another minigame after Yes, and changes nothing when declined or full; the VMU screen shows the sent pet, then the wordmark once it is home.
# | Author: suinevere
# | Dependencies: tools/flycast_run.py, tools/vmu_image.py, stale_image.py, pocketstation/tools/psk_file.py, pocketstation/build/vmu_game/rrl.vms, lom_pet_send_test.cdi, lom_pet_send_decline_test.cdi, lom_pet_return_test.cdi, lom_pet_ranch_test.cdi
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
import frame_diff  # noqa: E402
import psk_file  # noqa: E402
import stale_image  # noqa: E402
import vmu_image  # noqa: E402

#----------------------
# | RRL_NAME / RES1506 / PET / PLACED_LANDS / MAXED_MANA / PET_KIND / REWARDS / GROWTH
# | Description: The Ring Ring Land file name, its CD resource, the pet and land state field_watch_unit.c plants, the kind sent for that pet (species 1 is in the Beast family 0, whatever its egg species 7), and the rewards and growth the return test writes.
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
PET_KIND = 0
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


#----------------------
# | vmu_screen_before
# | Description: The last "vmu screen" choice field_watch_unit.c logged before a capture: a pet or egg screen, or -1 for the wordmark.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: lines -- log lines; capture -- the capture's name
# | Returns: the screen number
#----------------------
def vmu_screen_before(lines, capture):
    end = next(i for i, line in enumerate(lines) if "capture " + capture in line)
    return int([line for line in lines[:end] if "vmu screen" in line][-1].split()[-1])


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
    assert sent[:kept] == psk_file.pet_transfer(res, PET_KIND, 9, PET, PLACED_LANDS, MAXED_MANA)[:kept]
    assert sent[kept:] == bytes(0xC000 - kept)
    assert len(vmu_image.chain(img, vmu_image.vmu_name_for_ps1(RRL_NAME))) <= 5


def test_send_empties_the_ranch_slot(send_run):
    _, lines, _, _ = send_run
    _, name0, _, _ = transfer_line(lines)
    assert name0 == 0


def test_the_vmu_screen_shows_the_pet_sent_to_ring_ring_land(send_run):
    _, lines, _, _ = send_run
    assert vmu_screen_before(lines, "send-done") == PET[0x15]


def test_return_brings_the_pet_and_its_rewards_home_and_erases_the_file(return_run):
    code, lines, img, _ = return_run
    assert code == 0, lines[-20:]
    _, name0, unique_id, counts = transfer_line(lines)
    assert name0 == PET[0] and unique_id == struct.unpack_from("<I", PET, 0x5C)[0]
    assert counts == (1, 1)
    assert vmu_image.ps1_file(img, RRL_NAME) is None
    assert vmu_screen_before(lines, "return-done") == -1


#----------------------
# | GAME_FILE / GAME_IMAGE / OTHER_GAME
# | Description: Ring Ring Land's VMU GAME name and the build it comes from, and the stand-in minigame's name.
# | Author: suinevere
# | Dependencies: dreamcast/src/card/vmu_game.h (VMU_GAME_FILE), pocketstation/src/vmu_game/Makefile (rrl.vms)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
GAME_FILE = "RINGRINGLAND"
GAME_IMAGE = os.path.join(os.path.dirname(DC), "pocketstation", "build", "vmu_game", "rrl.vms")
OTHER_GAME = "MINIGAME"


def _game():
    with open(GAME_IMAGE, "rb") as f:
        return f.read()


def _entry(img, name):
    return [data for n, data in vmu_image.files(img) if n == name]


def _sent_package(send_run):
    found = _entry(send_run[2], vmu_image.vmu_name_for_ps1(RRL_NAME))
    if not found:
        pytest.skip("the send run wrote no Ring Ring Land file")
    return found[0]


@pytest.fixture(scope="module")
def stale_run(tmp_path_factory):
    vmu_in = vmu_image.add_game(vmu_image.blank(), GAME_FILE, bytes(512 * 3))
    return vmu_in, run("lom_pet_send_test.cdi", vmu_in, "LOM: capture send-done", 1500, tmp_path_factory, "stale")


@pytest.fixture(scope="module")
def other_run(tmp_path_factory):
    vmu_in = vmu_image.add_game(vmu_image.blank(), OTHER_GAME, bytes(512 * 40))
    return vmu_in, run("lom_pet_send_test.cdi", vmu_in, "LOM: capture send-done", 1500, tmp_path_factory, "other")


def _with_foreign_pet(send_run, vmu_in):
    package = bytes(_sent_package(send_run))
    uid = struct.pack("<I", struct.unpack_from("<I", PET, 0x5C)[0])
    vmu_in = vmu_image.add(vmu_in, vmu_image.vmu_name_for_ps1(RRL_NAME), package)
    payload = bytearray(vmu_image.ps1_file(vmu_in, RRL_NAME))
    at = payload.find(uid)
    if at < 0:
        pytest.skip("the sent pet's unique id is not in its payload")
    payload[at] ^= 0xFF
    return vmu_image.set_ps1_file(vmu_in, RRL_NAME, bytes(payload))


def _low_save(img):
    return vmu_image.add(img, "FILLER", bytes(512 * 10), blocks=list(range(69, 59, -1)))


@pytest.fixture(scope="module")
def swap_other_run(send_run, tmp_path_factory):
    vmu_in = _with_foreign_pet(send_run, vmu_image.add_game(vmu_image.blank(), OTHER_GAME, bytes(512 * 40)))
    return vmu_in, run("lom_pet_send_test.cdi", vmu_in, "LOM: capture send-done", 1500, tmp_path_factory, "swap-other")


@pytest.fixture(scope="module")
def swap_noroom_run(send_run, tmp_path_factory):
    vmu_in = _with_foreign_pet(send_run, _low_save(vmu_image.blank()))
    return vmu_in, run("lom_pet_send_decline_test.cdi", vmu_in, "LOM: capture send-done", 1500, tmp_path_factory, "swap-noroom")


@pytest.fixture(scope="module")
def decline_run(tmp_path_factory):
    vmu_in = vmu_image.add_game(vmu_image.blank(), OTHER_GAME, bytes(512 * 40))
    return vmu_in, run("lom_pet_send_decline_test.cdi", vmu_in, "LOM: capture send-done", 1500, tmp_path_factory, "decline")


@pytest.fixture(scope="module")
def noroom_run(tmp_path_factory):
    vmu_in = _low_save(vmu_image.blank())
    return vmu_in, run("lom_pet_send_decline_test.cdi", vmu_in, "LOM: capture send-done", 1500, tmp_path_factory, "noroom")


def _assert_game_installed(img):
    game = _game()
    blocks = (len(game) + 511) // 512
    games = [n for n, _ in vmu_image.files(img) if n in (GAME_FILE, OTHER_GAME)]
    assert games == [GAME_FILE], games
    assert vmu_image.chain(img, GAME_FILE) == list(range(blocks))
    assert _entry(img, GAME_FILE)[0][:len(game)] == game


def test_send_installs_the_game_from_block_0(send_run):
    code, lines, img, _ = send_run
    assert code == 0, lines[-20:]
    _assert_game_installed(img)


def test_send_rewrites_an_older_ring_ring_land(stale_run):
    _, (code, lines, img, _) = stale_run
    assert code == 0, lines[-20:]
    _assert_game_installed(img)
    assert vmu_image.ps1_file(img, RRL_NAME) is not None


def test_send_replaces_another_minigame_after_yes(other_run):
    _, (code, lines, img, _) = other_run
    assert code == 0, lines[-20:]
    _assert_game_installed(img)
    assert vmu_image.ps1_file(img, RRL_NAME) is not None


def test_a_swap_replaces_another_minigame_after_yes(swap_other_run):
    _, (code, lines, img, _) = swap_other_run
    assert code == 0, lines[-20:]
    _assert_game_installed(img)


def test_the_replace_question_shows_on_another_minigame(send_run, other_run):
    with open(os.path.join(send_run[3], "send-prompt.png"), "rb") as a, open(os.path.join(other_run[1][3], "send-prompt.png"), "rb") as b:
        assert a.read() != b.read()
    with open(os.path.join(send_run[3], "send-slot.png"), "rb") as a, open(os.path.join(other_run[1][3], "send-slot.png"), "rb") as b:
        assert a.read() == b.read()


def test_declining_leaves_the_vmu_and_ranch_alone(decline_run):
    vmu_in, (code, lines, img, _) = decline_run
    assert code == 0, lines[-20:]
    assert img == vmu_in
    _, name0, _, _ = transfer_line(lines)
    assert name0 == PET[0]


#----------------------
# | SAME_SCREEN_MEAN
# | Description: The largest mean pixel difference (frame_diff.compare, 320x240 blurred) between two captures of the same CARDA screen; the animated room behind it moves a few pixels, while a different message moves several units.
# | Author: suinevere
# | Dependencies: tools/frame_diff.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SAME_SCREEN_MEAN = 0.5


def _screen_gap(a, b, name):
    return frame_diff.compare(frame_diff.load_capture(os.path.join(a[1][3], name)),
                              frame_diff.load_capture(os.path.join(b[1][3], name)))["mean"]


def test_no_room_refuses_and_keeps_the_pet(noroom_run, decline_run):
    vmu_in, (code, lines, img, _) = noroom_run
    assert code == 0, lines[-20:]
    assert img == vmu_in
    _, name0, _, _ = transfer_line(lines)
    assert name0 == PET[0]
    assert _screen_gap(noroom_run, decline_run, "send-prompt.png") > SAME_SCREEN_MEAN


def test_a_swap_with_no_room_shows_the_not_enough_blocks_screen(swap_noroom_run, noroom_run):
    vmu_in, (code, lines, img, _) = swap_noroom_run
    assert code == 0, lines[-20:]
    assert img == vmu_in
    assert _screen_gap(swap_noroom_run, noroom_run, "send-prompt.png") <= SAME_SCREEN_MEAN


def test_the_ranch_menu_offers_ring_ring_land_and_opens_carda(ranch_run):
    code, lines, _, _ = ranch_run
    assert code == 0, lines[-20:]
    picked = next(n for n, line in enumerate(lines) if "capture ranch-sub-1" in line)
    assert any("field modal 3" in line for line in lines[picked:])
