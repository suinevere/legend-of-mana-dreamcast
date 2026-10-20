#----------------------
# | test_pet_transfer.py
# | Description: Builds card_prepare_pet_transfer as the Dreamcast build does and checks its Ring Ring Land file against psk_file.pet_transfer.
# | Author: suinevere
# | Dependencies: tests/host/test_pet_transfer.c, src/main/card_callbacks.c, pocketstation/tools/psk_file.py
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
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))

import psk_file  # noqa: E402

RES1506 = os.path.join(DC, "build-test", "cd-data", "RES", "05", "RES1506.BIN")

#----------------------
# | PLACED_LANDS / MAXED_MANA
# | Description: The land state test_pet_transfer.c sets, as psk_file.pet_transfer takes it; land 10 is maxed but not placed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PLACED_LANDS = frozenset({1, 2, 3, 5, 6, 7, 9, 15, 24, 31})
MAXED_MANA = {9: 0x85, 10: 0x02, 31: 0xFF}


#----------------------
# | test_dreamcast_build_prepares_the_jp_transfer
# | Description: The hook built with -DVERSION_US -DLOM_DREAMCAST writes the same file psk_file.pet_transfer does.
# | Author: suinevere
# | Dependencies: psk_file.pet_transfer
# | Globals: RES1506, PLACED_LANDS, MAXED_MANA
# | Params: N/A
# | Returns: N/A
#----------------------
def test_dreamcast_build_prepares_the_jp_transfer():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    if not os.path.exists(RES1506):
        pytest.skip("RES1506.BIN not converted")
    work = tempfile.mkdtemp()
    exe = os.path.join(work, "pet_transfer.exe")
    subprocess.run([cc, "-std=gnu11", "-w", "-mno-ms-bitfields", "-DVERSION_US", "-DLOM_DREAMCAST", "-I", os.path.join(REPO, "include"),
                    "-o", exe, os.path.join(DC, "tests", "host", "test_pet_transfer.c"),
                    os.path.join(REPO, "src", "main", "card_callbacks.c")], check=True)
    pet = psk_file.REFERENCE_PET[:0x16] + bytes([7]) + psk_file.REFERENCE_PET[0x17:0x18] + bytes([9]) + psk_file.REFERENCE_PET[0x19:]
    pet_path, out_path = os.path.join(work, "pet.bin"), os.path.join(work, "out.bin")
    with open(pet_path, "wb") as f:
        f.write(pet)
    subprocess.run([exe, RES1506, pet_path, out_path], check=True)
    with open(RES1506, "rb") as f:
        res = f.read()
    with open(out_path, "rb") as f:
        out = f.read()
    assert out == psk_file.pet_transfer(res, 7, 9, pet, PLACED_LANDS, MAXED_MANA)
