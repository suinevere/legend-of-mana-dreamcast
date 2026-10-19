#----------------------
# | test_build.py
# | Description: Builds the Dreamcast port in lom-dc and checks lom.elf is a little-endian SuperH ELF and lom.cdi exists.
# |              Also reproduces the all/test-image/all sequence and checks each image's staged overlay BIN was linked against its own ELF, and that a second unchanged build does not rerun the disc converter.
# | Author: suinevere
# | Dependencies: compile.sh, makefile, pre.makefile, ../port/tools/overlay_link.py, ../port/tools/convert_disc.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import shutil
import subprocess

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)

#----------------------
# | CONVERTER_START_LINE
# | Description: The start of the disc converter's first printed line for the dreamcast target (convert_disc.py's "resource %4d kind %d size 0x%X", index 2 being FIELD, a real KIND_OVERLAY since Task 6 gave it its own Dreamcast link), without the size, which moves whenever FIELD's packed data does.
# |              Its absence from a build's output means pre_build's recipe did not run.
# | Author: suinevere
# | Dependencies: ../port/tools/convert_disc.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CONVERTER_START_LINE = "resource    2 kind 1 size 0x"

#----------------------
# | SECOND_BUILD_MARKER
# | Description: Echoed before the build under test in test_second_build_does_not_rerun_disc_converter and test_touching_splat_ext_module_reruns_disc_converter, splitting combined stdout so only that build's own output is checked.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECOND_BUILD_MARKER = "LOM_TEST_SECOND_BUILD_MARKER"

CHECKPS_LINK_OBJS = [
    "build/ovl/checkps_vectors.o",
    "build/decomp/overlays/checkps/cdrom.o",
    "build/decomp/overlays/checkps/font.o",
    "build/decomp/overlays/checkps/kanji.o",
    "build/decomp/overlays/checkps/pattern.o",
    "build/overlays/checkps_init_unit.o",
]


def test_build_produces_sh4_little_endian_elf_and_cdi():
    if shutil.which("docker") is None:
        pytest.skip("no docker")
    if subprocess.run(["docker", "image", "inspect", "lom-dc"], capture_output=True).returncode != 0:
        pytest.skip("lom-dc image not built")
    build = subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-dc",
                            "bash", "-lc", "make -C dreamcast all"], capture_output=True, text=True)
    assert build.returncode == 0, build.stdout[-4000:] + build.stderr[-4000:]
    with open(os.path.join(DC, "lom.elf"), "rb") as f:
        header = f.read(20)
    assert header[:4] == b"\x7fELF"
    assert header[5] == 1
    assert int.from_bytes(header[18:20], "little") == 42
    assert os.path.getsize(os.path.join(DC, "lom.cdi")) > 0


def test_alternating_builds_each_keep_their_own_overlay_bin():
    if shutil.which("docker") is None:
        pytest.skip("no docker")
    if subprocess.run(["docker", "image", "inspect", "lom-dc"], capture_output=True).returncode != 0:
        pytest.skip("lom-dc image not built")

    def link_checkps(target, outdir):
        return ("mkdir -p %s && python3 ../port/tools/overlay_link.py --target dreamcast link checkps %s %s %s/OVL15C.BIN %s"
                % (outdir, target, outdir, outdir, " ".join(CHECKPS_LINK_OBJS)))

    steps = " && ".join([
        "make all",
        "make test-image",
        "make all",
        link_checkps("lom.elf", ".verify/shipping"),
        link_checkps("lom_test.elf", ".verify/test"),
    ])
    build = subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work/dreamcast", "lom-dc",
                            "bash", "-lc", steps], capture_output=True, text=True)
    assert build.returncode == 0, build.stdout[-4000:] + build.stderr[-4000:]

    def read(*parts):
        with open(os.path.join(DC, *parts), "rb") as f:
            return f.read()

    shipping_staged = read("build", "cd-data", "OVL", "OVL15C.BIN")
    test_staged = read("build-test", "cd-data", "OVL", "OVL15C.BIN")
    shipping_fresh = read(".verify", "shipping", "OVL15C.BIN")
    test_fresh = read(".verify", "test", "OVL15C.BIN")

    assert shipping_staged == shipping_fresh, "lom.cdi's staged overlay must match a fresh link against lom.elf"
    assert test_staged == test_fresh, "lom_test.cdi's staged overlay must match a fresh link against lom_test.elf"
    assert shipping_staged != test_staged, "the two builds' EXTERN addresses must actually differ"


#----------------------
# | test_second_build_does_not_rerun_disc_converter
# | Description: Builds twice in a row with nothing changed and checks the disc converter's start-of-run line appears only in the first build's output.
# |              So pre_build's stamp skips the recipe on the second `make`.
# | Author: suinevere
# | Dependencies: CONVERTER_START_LINE, SECOND_BUILD_MARKER
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
def test_second_build_does_not_rerun_disc_converter():
    if shutil.which("docker") is None:
        pytest.skip("no docker")
    if subprocess.run(["docker", "image", "inspect", "lom-dc"], capture_output=True).returncode != 0:
        pytest.skip("lom-dc image not built")

    steps = " && ".join([
        "make -C dreamcast all",
        "echo %s" % SECOND_BUILD_MARKER,
        "make -C dreamcast all",
    ])
    build = subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-dc",
                            "bash", "-lc", steps], capture_output=True, text=True)
    assert build.returncode == 0, build.stdout[-4000:] + build.stderr[-4000:]

    _, _, second_build_output = build.stdout.partition(SECOND_BUILD_MARKER)
    assert CONVERTER_START_LINE not in second_build_output, second_build_output[-4000:]


#----------------------
# | test_touching_splat_ext_module_reruns_disc_converter
# | Description: Touches a repo-root tools/splat_ext module convert_disc.py reaches through its own sys.path.insert, then checks the next build's output has the disc converter's start-of-run line, so PRE_BUILD_INPUTS tracks that directory too.
# | Author: suinevere
# | Dependencies: CONVERTER_START_LINE, SECOND_BUILD_MARKER, ../tools/splat_ext/decompress.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
def test_touching_splat_ext_module_reruns_disc_converter():
    if shutil.which("docker") is None:
        pytest.skip("no docker")
    if subprocess.run(["docker", "image", "inspect", "lom-dc"], capture_output=True).returncode != 0:
        pytest.skip("lom-dc image not built")

    steps = " && ".join([
        "make -C dreamcast all",
        "touch tools/splat_ext/decompress.py",
        "echo %s" % SECOND_BUILD_MARKER,
        "make -C dreamcast all",
    ])
    build = subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-dc",
                            "bash", "-lc", steps], capture_output=True, text=True)
    assert build.returncode == 0, build.stdout[-4000:] + build.stderr[-4000:]

    _, _, rebuild_output = build.stdout.partition(SECOND_BUILD_MARKER)
    assert CONVERTER_START_LINE in rebuild_output, rebuild_output[-4000:]


MOVIE_LINKED_NAMES = ["movie_play", "movie_init", "movie_update", "movie_service_video_ops", "DecDCTvlc2", "DecDCTin",
                      "DecDCTout", "CdGetSector", "cdrom_reset", "cdrom_queue_command", "DrawSyncCallback", "BreakDraw",
                      "LoadImage2", "akao_start_xa_stream", "g_field_movie_frame_width", "g_field_movie_frame_height"]


def test_movie_names_are_not_pending_trap_aliases():
    pending = os.path.join(DC, "gen", "dc_pending.ld")
    if not os.path.exists(pending):
        pytest.skip("dreamcast/gen not generated")
    with open(pending) as f:
        aliased = {line.split(" = ")[0].strip().lstrip("_") for line in f if "= _lom_trap_buffer" in line}
    assert not aliased & set(MOVIE_LINKED_NAMES), sorted(aliased & set(MOVIE_LINKED_NAMES))
