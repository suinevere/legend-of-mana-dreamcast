#----------------------
# | test_overlay_link_build.py
# | Description: In lom-dc, checks lom.elf's overlay arena, heap start, GNAME and WSEL regions and WSEL's entry, then links a stand-in CHECKPS overlay against lom.elf with overlay_link.py and checks its packed OVL15C.BIN.
# | Author: suinevere
# | Dependencies: makefile, dreamcast.ld, tests/overlay/checkps_probe.c, ../port/tools/overlay_link.py, ../port/tools/targets.py, gen/checkps_symbols.ld, cd/data/OVL/OVL15.BIN
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re
import shutil
import struct
import subprocess
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
WORK = os.path.join(DC, "build", "test_overlay")
sys.path.insert(0, os.path.join(REPO, "port", "tools"))

import targets

STEPS = " && ".join([
    "make all",
    "mkdir -p build/test_overlay",
    "kos-cc -Isrc/system -c tests/overlay/checkps_probe.c -o build/test_overlay/checkps_probe.o",
    "kos-cc -c gen/checkps_vectors.s -o build/test_overlay/checkps_vectors.o",
    "python3 ../port/tools/overlay_link.py --target dreamcast link checkps lom.elf build/test_overlay"
    " build/test_overlay/OVL15C.BIN build/test_overlay/checkps_vectors.o build/test_overlay/checkps_probe.o",
    "sh-elf-nm lom.elf > build/test_overlay/lom.nm",
])


def nm_map(path):
    out = {}
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) == 3:
                out[parts[2]] = int(parts[0], 16)
    return out


def nm_types(path):
    out = {}
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) == 3:
                out[parts[2]] = parts[1]
    return out


@pytest.fixture(scope="module")
def linked():
    if shutil.which("docker") is None:
        pytest.skip("no docker")
    if subprocess.run(["docker", "image", "inspect", "lom-dc"], capture_output=True).returncode != 0:
        pytest.skip("lom-dc image not built")
    build = subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work/dreamcast", "lom-dc",
                            "bash", "-lc", STEPS], capture_output=True, text=True)
    assert build.returncode == 0, build.stdout[-4000:] + build.stderr[-4000:]
    with open(os.path.join(WORK, "OVL15C.BIN"), "rb") as f:
        packed = f.read()
    with open(os.path.join(DC, "gen", "checkps_symbols.ld")) as f:
        data_symbols = {m.group(1): int(m.group(2), 16) for m in re.finditer(r"^(\w+) = (0x[0-9A-Fa-f]+);", f.read(), re.M)}
    return packed, nm_map(os.path.join(WORK, "lom.nm")), nm_map(os.path.join(WORK, "checkps.nm")), data_symbols


def test_lom_elf_places_the_arena_and_starts_the_heap_above_it(linked):
    _, main, _, _ = linked
    assert main["_lom_overlay_arena"] == 0x8C800000
    assert main["_lom_program_end"] <= 0x8C800000
    assert main["_end"] == 0x8CE00000
    assert main["_run_checkps"] == 0x8C800000
    assert main["_run_title"] == 0x8C880000
    assert main["_gname_run"] == 0x8C980000


def region_sits_in_the_arena_clear_of(name, others):
    regions = targets.DREAMCAST.regions
    own = regions[name]
    spans = [(own.code_base, own.code_base + own.code_size), (own.data_base, own.data_base + own.data_size)]
    for start, end in spans:
        assert targets.ARENA_BASE <= start < end <= targets.ARENA_END
        for other in others:
            r = regions[other]
            for o_start, o_end in ((r.code_base, r.code_base + r.code_size), (r.data_base, r.data_base + r.data_size)):
                assert end <= o_start or o_end <= start, (other, hex(start), hex(o_start))


def test_gname_region_sits_in_the_arena_clear_of_checkps_and_title():
    region_sits_in_the_arena_clear_of("gname", ("checkps", "title"))


def test_wsel_region_sits_in_the_arena_clear_of_checkps_title_and_gname():
    region_sits_in_the_arena_clear_of("wsel", ("checkps", "title", "gname"))


def test_wsel_main_is_wsel_code_not_a_trap_buffer_alias(linked):
    _, main, _, _ = linked
    types = nm_types(os.path.join(WORK, "lom.nm"))
    assert main["_wsel_main"] == targets.DREAMCAST.regions["wsel"].code_base
    assert main["_wsel_main"] != main["_lom_trap_buffer"]
    assert types["_wsel_main"] not in "BbDd"


def test_probe_header_matches_the_checkps_regions(linked):
    packed, _, _, data_symbols = linked
    magic, version, code_base, code_size, data_size, bss_start, bss_end, _ = struct.unpack(">4sIIIIIII", packed[:32])
    assert (magic, version, code_base) == (b"LOMC", 1, 0x8C800000)
    assert code_size == len(packed) - 32 and 0 < code_size <= 0x40000
    assert data_size == data_symbols["_lom_ovl_checkps_data_size"]
    assert data_size == os.path.getsize(os.path.join(DC, "cd", "data", "OVL", "OVL15.BIN"))
    assert bss_start == 0x8C840000 + data_size
    assert bss_start < bss_end <= 0x8C880000


def test_probe_vector_jumps_to_its_entry(linked):
    packed, _, probe, _ = linked
    code = packed[32:]
    assert 0x8C800010 <= probe["_run_checkps"] < 0x8C840000
    assert struct.unpack_from("<I", code, 8)[0] == probe["_run_checkps"]


def test_probe_code_resolves_lom_elf_and_checkps_data_symbols(linked):
    packed, main, _, data_symbols = linked
    code = packed[32:]
    assert struct.pack("<I", main["_dc_log"]) in code
    assert struct.pack("<I", data_symbols["_g_checkps_state"]) in code
