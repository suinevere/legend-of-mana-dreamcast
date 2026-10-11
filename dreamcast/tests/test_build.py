#----------------------
# | test_build.py
# | Description: Builds the Dreamcast port in lom-dc and checks lom.elf is a little-endian SuperH ELF and lom.cdi exists; also reproduces the all/test-image/all sequence and checks each image's staged overlay BIN was linked against its own ELF, and that a second unchanged build does not rerun the disc converter.
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
# | Description: The disc converter's first printed line for the committed US image and the dreamcast target (convert_disc.py's "resource %4d kind %d size 0x%X", index 2 being the first overlay resource); its absence from a build's output means pre_build's recipe did not run.
# | Author: suinevere
# | Dependencies: ../port/tools/convert_disc.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CONVERTER_START_LINE = "resource    2 kind 4 size 0x0"

#----------------------
# | SECOND_BUILD_MARKER
# | Description: Echoed between the two `make all` invocations in test_second_build_does_not_rerun_disc_converter, splitting combined stdout into each run's own output.
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
# | Description: Builds twice in a row with nothing changed and checks the disc converter's start-of-run line appears only in the first build's output, so pre_build's stamp skips the recipe on the second `make`.
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
