#----------------------
# | test_overlay_link_dc.py
# | Description: Checks overlay_link.py's --target and --dry-run: the Dreamcast link profile at each arena region, the region header, the header packing, and Saturn output left unchanged.
# | Author: suinevere
# | Dependencies: tools/overlay_link.py, tools/targets.py, ../saturn/gen
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import struct
import subprocess
import sys
import tempfile

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(PORT)
TOOL = os.path.join(PORT, "tools", "overlay_link.py")
sys.path.insert(0, os.path.join(PORT, "tools"))

import overlay_link  # noqa: E402
import targets  # noqa: E402


def run(*args, env=None):
    result = subprocess.run([sys.executable, TOOL] + list(args), capture_output=True, text=True, env=env)
    assert result.returncode == 0, result.stdout + result.stderr
    return result.stdout


def files_in(stdout):
    out = {}
    name = None
    for line in stdout.splitlines(keepends=True):
        stripped = line.rstrip("\n")
        if stripped.startswith("==> ") and stripped.endswith(" <=="):
            name = stripped[4:-4]
            out[name] = ""
        elif name is not None:
            out[name] += line
    return out


def test_dreamcast_dry_run_prints_each_code_base():
    stdout = run("--target", "dreamcast", "--dry-run")
    for name, region in targets.DREAMCAST.regions.items():
        assert "%s: code 0x%08X" % (name, region.code_base) in stdout


def test_dreamcast_checkps_script_links_little_endian_at_its_code_base():
    script = files_in(run("--target", "dreamcast", "--dry-run"))["checkps.ld"]
    assert "OUTPUT_FORMAT(elf32-shl)" in script
    assert ". = 0x8C800000" in script
    assert "INCLUDE gen/checkps_symbols.ld" in script


def test_dreamcast_scripts_include_their_own_pending_aliases():
    files = files_in(run("--target", "dreamcast", "--dry-run"))
    for name in targets.DREAMCAST.regions:
        assert "INCLUDE gen/%s_pending.ld" % name in files["%s.ld" % name]


def test_dreamcast_scripts_assert_every_region_limit():
    files = files_in(run("--target", "dreamcast", "--dry-run"))
    for name, region in targets.DREAMCAST.regions.items():
        script = files["%s.ld" % name]
        assert ". = 0x%08X" % region.code_base in script
        assert ". = 0x%08X" % region.data_base in script
        assert "ASSERT(_lom_ovl_code_end <= 0x%08X" % (region.code_base + region.code_size) in script
        assert "ASSERT(_lom_ovl_%s_bss_end <= 0x%08X" % (name, region.data_base + region.data_size) in script
        assert "elf32-sh)" not in script


def test_title_data_region_is_the_ruled_size():
    files = files_in(run("--target", "dreamcast", "--dry-run"))
    assert "ASSERT(_lom_ovl_title_bss_end <= 0x8C980000" in files["title.ld"]


def test_dreamcast_entries_point_at_each_code_base():
    entries = files_in(run("--target", "dreamcast", "--dry-run"))["overlay_entries.ld"]
    assert "_run_checkps = 0x8C800000;" in entries
    assert "_run_title = 0x8C880000;" in entries
    assert "_lom_code_floor" not in entries


def test_dreamcast_entries_leave_unlinked_overlays_to_main():
    entries = files_in(run("--target", "dreamcast", "--dry-run"))["overlay_entries.ld"]
    for ovl in overlay_link.OVERLAYS:
        if ovl.name in targets.DREAMCAST_UNLINKED_OVERLAYS:
            for fn in ovl.entries:
                assert "_%s =" % fn not in entries, fn
    assert "_gname_run = 0x8CB00000;" in entries


def test_dreamcast_region_header_matches_targets():
    header = files_in(run("--target", "dreamcast", "--dry-run"))["overlay_regions.h"]
    assert "#define OVERLAY_ARENA_BASE 0x%08Xu" % targets.ARENA_BASE in header
    assert "#define OVERLAY_ARENA_END 0x%08Xu" % targets.ARENA_END in header
    for ovl in overlay_link.OVERLAYS:
        if ovl.name in targets.DREAMCAST.regions:
            r = targets.DREAMCAST.regions[ovl.name]
            assert ("{ %d, \"%s\", 0x%08Xu, 0x%08Xu, 0x%08Xu, 0x%08Xu }"
                    % (ovl.resource, ovl.name.upper(), r.code_base, r.code_size, r.data_base, r.data_size)) in header


def test_dreamcast_dry_run_names_the_linker_from_kos_ld():
    env = dict(os.environ, KOS_LD="/opt/toolchains/dc/sh-elf/bin/sh-elf-ld")
    assert "linker: /opt/toolchains/dc/sh-elf/bin/sh-elf-ld" in run("--target", "dreamcast", "--dry-run", env=env)
    env.pop("KOS_LD")
    assert "linker: sh-elf-ld" in run("--target", "dreamcast", "--dry-run", env=env)


def test_dry_run_writes_nothing():
    out = tempfile.mkdtemp()
    run("--target", "dreamcast", "--dry-run", "gen", "--out", out)
    assert os.listdir(out) == []


def test_dreamcast_gen_writes_every_dry_run_file():
    out = tempfile.mkdtemp()
    run("--target", "dreamcast", "gen", "--out", out)
    expected = files_in(run("--target", "dreamcast", "--dry-run"))
    assert sorted(os.listdir(out)) == sorted(expected)
    for name, text in expected.items():
        with open(os.path.join(out, name), newline="") as f:
            assert f.read() == text


def test_saturn_dry_run_matches_generated_saturn_files():
    gen = os.path.join(REPO, "saturn", "gen")
    files = files_in(run("--target", "saturn", "--dry-run"))
    assert "overlay_entries.ld" in files
    for name, text in files.items():
        path = os.path.join(gen, name)
        if os.path.exists(path):
            with open(path, newline="") as f:
                assert f.read() == text, name


def test_saturn_default_target_is_unchanged():
    assert run("--dry-run") == run("--target", "saturn", "--dry-run")


def test_dreamcast_header_packs_the_region_code_base():
    nm = ("8c800000 T _lom_ovl_code_start\n8c800040 T _lom_ovl_code_end\n0000ac88 A _lom_ovl_checkps_data_size\n"
          "8c84ac88 B _lom_ovl_checkps_bss_start\n8c84ac98 B _lom_ovl_checkps_bss_end\n")
    work = tempfile.mkdtemp()
    nm_path, code_path, out_path = (os.path.join(work, n) for n in ("c.nm", "c.bin", "OVL15C.BIN"))
    with open(nm_path, "w") as f:
        f.write(nm)
    with open(code_path, "wb") as f:
        f.write(b"\x09\x00" * 0x20)
    run("--target", "dreamcast", "header", "checkps", nm_path, out_path, code_path)
    with open(out_path, "rb") as f:
        packed = f.read()
    assert struct.unpack(">4sIIIIIII", packed[:32]) == (b"LOMC", 1, 0x8C800000, 0x40, 0xAC88, 0x8C84AC88, 0x8C84AC98, 0)
    assert packed[32:] == b"\x09\x00" * 0x20


def test_dreamcast_rejects_an_overlay_without_a_region():
    result = subprocess.run([sys.executable, TOOL, "--target", "dreamcast", "header", "nosuchoverlay", "x", "y"],
                            capture_output=True, text=True)
    assert result.returncode != 0
    assert "nosuchoverlay" in result.stdout + result.stderr


def test_dreamcast_menu_entry_is_its_code_base():
    base = targets.DREAMCAST.regions["menu"].code_base
    assert "_func_801405B0 = 0x%08X;" % base in overlay_link.dc_entries_ld()


def test_saturn_leaves_menu_out_of_its_generated_files():
    names = [name for name, _ in overlay_link.generated_files(targets.SATURN)]
    assert "menu.ld" not in names
    assert "_func_801405B0" not in overlay_link.entries_ld()

def test_secondary_aliases_agree_across_overlays():
    seen = {}
    for ovl in overlay_link.dc_overlays():
        for name, address in overlay_link.dc_exported_entries(ovl):
            assert seen.setdefault(name, address) == address, name


def test_gname_entry_is_also_func_80140004():
    gname = overlay_link.overlay_named("gname", targets.DREAMCAST)
    exported = dict(overlay_link.dc_exported_entries(gname))
    assert exported["gname_run"] == exported["func_80140004"] == targets.SECONDARY_REGION.code_base


def test_entries_ld_defines_each_exported_name_once():
    lines = [l for l in overlay_link.dc_entries_ld().splitlines() if l.startswith("_")]
    names = [l.split(" = ")[0] for l in lines]
    assert len(names) == len(set(names))
    assert "_func_80140004 = 0x8CB00000;" in lines
