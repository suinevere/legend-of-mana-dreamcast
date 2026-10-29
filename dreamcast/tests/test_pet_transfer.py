#----------------------
# | test_pet_transfer.py
# | Description: Builds card_prepare_pet_transfer as the Dreamcast build does and checks its Ring Ring Land file against psk_file.pet_transfer, with a hatched pet's kind taken from its species family.
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

RES1506 = next((p for p in (os.path.join(DC, "build-test", "cd-data", "RES", "05", "RES1506.BIN"),
                             os.path.join(DC, "cd", "data", "RES", "05", "RES1506.BIN")) if os.path.exists(p)),
               os.path.join(DC, "cd", "data", "RES", "05", "RES1506.BIN"))

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
# | _run_hook
# | Description: Builds the hook with -DVERSION_US -DLOM_DREAMCAST, runs it on RES1506.BIN with pet, and returns the resource and the hook's output.
# | Dependencies: tests/host/test_pet_transfer.c, src/main/card_callbacks.c
#----------------------
def _run_hook(pet):
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
    pet_path, out_path = os.path.join(work, "pet.bin"), os.path.join(work, "out.bin")
    with open(pet_path, "wb") as f:
        f.write(pet)
    subprocess.run([exe, RES1506, pet_path, out_path], check=True)
    with open(RES1506, "rb") as f:
        res = f.read()
    with open(out_path, "rb") as f:
        out = f.read()
    return res, out


#----------------------
# | _pet
# | Description: REFERENCE_PET with the given species, egg species, level 9, and the egg flag set or clear.
# | Dependencies: psk_file.REFERENCE_PET
#----------------------
def _pet(species, egg_species, egg):
    pet = bytearray(psk_file.REFERENCE_PET)
    pet[0x15] = species
    pet[0x16] = egg_species
    pet[0x18] = 9
    pet[0x47] = (pet[0x47] | 0x80) if egg else (pet[0x47] & 0x7F)
    return bytes(pet)


#----------------------
# | test_egg_keeps_its_egg_species
# | Description: An egg is sent as the JP game sends it: its egg species is the visiting pet's kind.
# | Dependencies: _run_hook, _pet, psk_file.pet_transfer
#----------------------
def test_egg_keeps_its_egg_species():
    pet = _pet(1, 7, egg=True)
    res, out = _run_hook(pet)
    assert out == psk_file.pet_transfer(res, 7, 9, pet, PLACED_LANDS, MAXED_MANA)


#----------------------
# | test_hatched_pet_kind_is_its_species_family
# | Description: A hatched pet's kind is the egg family its species belongs to, whatever egg it came from, and its record is sent unchanged.
# | Dependencies: _run_hook, _pet, psk_file.pet_transfer
#----------------------
@pytest.mark.parametrize("species,family", [(0, 0), (4, 0), (5, 1), (14, 2), (15, 3), (20, 4), (24, 4), (25, 5), (34, 6),
                                            (39, 7), (44, 8), (45, 9), (47, 9), (48, 10), (52, 10), (53, 11), (57, 11)])
def test_hatched_pet_kind_is_its_species_family(species, family):
    pet = _pet(species, 0, egg=False)
    res, out = _run_hook(pet)
    assert out == psk_file.pet_transfer(res, family, 9, pet, PLACED_LANDS, MAXED_MANA)


#----------------------
# | test_species_outside_the_families_keeps_its_egg_species
# | Description: A hatched pet whose species no egg family holds keeps the JP behaviour.
# | Dependencies: _run_hook, _pet, psk_file.pet_transfer
#----------------------
def test_species_outside_the_families_keeps_its_egg_species():
    pet = _pet(60, 3, egg=False)
    res, out = _run_hook(pet)
    assert out == psk_file.pet_transfer(res, 3, 9, pet, PLACED_LANDS, MAXED_MANA)
