#----------------------
# | test_cdrom_dc_host.py
# | Description: Writes a temp-directory disc fixture (RESINDEX.BIN plus RES/<XX> subfolders), then builds and runs the host C test for cdrom_dc.c against it with the host gcc.
# | Author: suinevere
# | Dependencies: tests/host/test_cdrom_dc.c, src/cd/cdrom_dc.c, src/cd/map_relocate.c, ../port/boot/resource_index.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import shutil
import struct
import subprocess
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.normpath(os.path.join(DC, ".."))
PORT_BOOT = os.path.join(REPO, "port", "boot")

RESOURCE_KIND_STREAM = 2
RESOURCE_KIND_RAW = 3
RESOURCE_KIND_SKIP = 4


def _write_fixture(root):
    os.makedirs(os.path.join(root, "RES", "00"), exist_ok=True)
    os.makedirs(os.path.join(root, "RES", "01"), exist_ok=True)
    entries = [
        (9, RESOURCE_KIND_SKIP, 0),
        (10, RESOURCE_KIND_STREAM, 4),
        (11, RESOURCE_KIND_RAW, 6),
        (12, RESOURCE_KIND_SKIP, 0),
        (13, RESOURCE_KIND_RAW, 8),
        (0x123, RESOURCE_KIND_RAW, 4),
        (0x12EB, RESOURCE_KIND_RAW, 4),
        (0x14DF, RESOURCE_KIND_RAW, 5),
    ]
    blob = struct.pack(">I", len(entries))
    for index, kind, size in entries:
        blob += struct.pack(">HBxI", index, kind, size)
    with open(os.path.join(root, "RESINDEX.BIN"), "wb") as f:
        f.write(blob)
    with open(os.path.join(root, "RES", "00", "RES0010.BIN"), "wb") as f:
        f.write(b"ABCD")
    with open(os.path.join(root, "RES", "00", "RES0011.BIN"), "wb") as f:
        f.write(b"FEDCBA")
    with open(os.path.join(root, "RES", "00", "RES0013.BIN"), "wb") as f:
        f.write(b"WXYZ")
    with open(os.path.join(root, "RES", "01", "RES0291.BIN"), "wb") as f:
        f.write(b"HIJK")
    os.makedirs(os.path.join(root, "RES", "12"), exist_ok=True)
    os.makedirs(os.path.join(root, "RES", "14"), exist_ok=True)
    with open(os.path.join(root, "RES", "12", "RES4843.BIN"), "wb") as f:
        f.write(b"LAYO")
    with open(os.path.join(root, "RES", "14", "RES5343.BIN"), "wb") as f:
        f.write(b"ZIMAG")


def test_cdrom_dc_messages_host_suite():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")

    root = tempfile.mkdtemp(prefix="lom_dc_cd_")
    _write_fixture(root)
    cd_root = root.replace("\\", "/")

    exe = os.path.join(tempfile.gettempdir(), "lom_dc_cdrom_test.exe")
    boot = os.path.join(DC, "src", "boot")
    cd = os.path.join(DC, "src", "cd")
    system = os.path.join(DC, "src", "system")
    cmd = [cc, "-std=gnu11", "-Wall", "-Wextra",
           "-DLOM_CD_ROOT=\"%s\"" % cd_root,
           "-I", os.path.join(REPO, "include"),
           "-I", boot,
           "-I", cd,
           "-I", system,
           "-I", PORT_BOOT,
           "-o", exe,
           os.path.join(DC, "tests", "host", "test_cdrom_dc.c"),
           os.path.join(cd, "cdrom_dc.c"),
           os.path.join(cd, "map_relocate.c"),
           os.path.join(PORT_BOOT, "resource_index.c")]
    subprocess.run(cmd, check=True, cwd=REPO)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout
