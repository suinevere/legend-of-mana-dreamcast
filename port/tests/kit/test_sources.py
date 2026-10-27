#----------------------
# | test_sources.py
# | Description: Checks port/kit/sources.py yields exactly the disc-derived bytes the Docker build linked and packed (needs the US disc and a pre-built dreamcast/gen).
# | Author: suinevere
# | Dependencies: port/kit/sources.py, disc/us/LegendOfMana.bin, dreamcast/gen, dreamcast/cd/data
#----------------------
import os
import re
import sys

import pytest

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
sys.path.insert(0, ROOT)
from port.kit import sources  # noqa: E402

IMAGE = os.path.join(ROOT, "disc", "us", "LegendOfMana.bin")
GEN = os.path.join(ROOT, "dreamcast", "gen")
CD = os.path.join(ROOT, "dreamcast", "cd", "data")
pytestmark = pytest.mark.skipif(not (os.path.exists(IMAGE) and os.path.exists(os.path.join(GEN, "libgte_tables.bin"))),
                                reason="needs the US disc and a pre-built dreamcast/gen")


def _c_bytes(path, name):
    text = open(path, encoding="utf-8").read()
    body = text[text.index(name + "["):]
    body = body[body.index("{"):body.index("};")]
    return bytes(int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{2})\b", body))


@pytest.fixture(scope="module")
def built(tmp_path_factory):
    return sources.build(IMAGE, str(tmp_path_factory.mktemp("kit")))


def test_libgte_matches(built):
    assert built["gen/libgte_tables"] == open(os.path.join(GEN, "libgte_tables.bin"), "rb").read()


def test_main_data_matches(built):
    assert built["gen/main_data"] == _c_bytes(os.path.join(GEN, "main_data.c"), "lom_main_data")


def test_vmu_screens_match(built):
    assert built["gen/vmu_pet_screens"] == _c_bytes(os.path.join(GEN, "vmu_pet_screens.c"), "lom_vmu_pet_screens")
    assert len(built["gen/vmu_pet_screens"]) == 0x39C0


def test_akao_block_size_and_pointers(built):
    assert len(built["gen/akao_data"]) == 0x1EA0
    assert sources.akao_pointer_words(IMAGE)


def test_every_cd_file_matches_build(built):
    on_disk = set()
    for folder, _, names in os.walk(CD):
        for name in names:
            rel = os.path.relpath(os.path.join(folder, name), CD).replace(os.sep, "/").upper()
            if rel.startswith("OVL/") and rel.endswith("C.BIN"):
                continue
            on_disk.add("cd/" + rel)
            assert built["cd/" + rel] == open(os.path.join(folder, name), "rb").read(), rel
    assert on_disk == {k for k in built if k.startswith("cd/")}


def test_rrl_pages_are_whole_pages(built):
    assert built["vmu/rrl_pages"] and len(built["vmu/rrl_pages"]) % 256 == 0
