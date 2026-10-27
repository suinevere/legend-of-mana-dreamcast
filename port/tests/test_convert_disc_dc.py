#----------------------
# | test_convert_disc_dc.py
# | Description: Checks the Dreamcast target exports TITLE's sound effect set and title song raw instead of skipping them, and exports every other live resource raw with a fanned-out RES/<XX> path.
# | Author: suinevere
# | Dependencies: tools/convert_disc.py, tools/targets.py, include/main/cd_resources.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(PORT, "tools"))

import convert_disc as cd  # noqa: E402
import targets  # noqa: E402


def test_dreamcast_reads_title_sound_resources_raw():
    assert cd.DREAMCAST_RAW_SOUND_RESOURCES == (21, 23)
    skipped = cd.skipped_resources()
    assert 21 not in skipped and 23 not in skipped
    assert 22 in skipped and 24 in skipped


def test_dreamcast_exports_every_live_resource_no_step_claimed():
    Resource = cd.disc_image.Resource
    table = [Resource(i, 1000 + i, 2048, True) for i in range(8)]
    table[5] = Resource(5, 1005, 2048, False)
    entries = [(1, cd.KIND_OVERLAY, 16), (3, cd.KIND_STREAM, 16)]
    overlays = {1, 2}
    assert cd.dreamcast_raw_resources(table, entries, overlays) == [0, 4, 6, 7]


def test_dreamcast_res_subfolder_is_index_shifted_by_eight_bits_in_hex():
    assert cd.dreamcast_res_subfolder(0x123) == "01"
    assert cd.dreamcast_res_subfolder(0) == "00"
    assert cd.dreamcast_res_subfolder(0x1FF) == "01"


def test_dreamcast_resource_path_fans_out_into_res_subfolders():
    data = os.path.join("o", "cd", "data")
    assert cd.resource_path("o", 0x123, cd.KIND_RAW) == os.path.join(data, "RES", "01", "RES0291.BIN")
    assert cd.resource_path("o", 5, cd.KIND_STREAM) == os.path.join(data, "RES", "00", "RES0005.BIN")
    assert cd.resource_path("o", 14, cd.KIND_OVERLAY) == os.path.join(data, "OVL", "OVL14.BIN")


def test_dreamcast_overlay_exported_skips_unlinked_overlays_with_regions(monkeypatch):
    assert "field" in targets.DREAMCAST.regions
    monkeypatch.setattr(targets, "DREAMCAST_UNLINKED_OVERLAYS", ("field",))
    assert not cd.dreamcast_overlay_exported("field")


def test_dreamcast_exports_wmap_now_that_it_links():
    assert "wmap" not in targets.DREAMCAST_UNLINKED_OVERLAYS
    assert cd.dreamcast_overlay_exported("wmap")


def test_dreamcast_exports_field_now_that_it_links():
    assert "field" not in targets.DREAMCAST_UNLINKED_OVERLAYS
    assert cd.dreamcast_overlay_exported("field")


def test_dreamcast_overlay_exported_is_true_for_a_linked_region():
    assert cd.dreamcast_overlay_exported("checkps")


def test_dreamcast_index_capacity_is_6144_and_covers_every_live_resource():
    assert cd.DREAMCAST_RESOURCE_INDEX_MAX == 6144
    entries = [(i, cd.KIND_RAW, 1) for i in range(5056)]
    cd.check_index_capacity(entries)


def test_dreamcast_index_capacity_still_rejects_an_oversize_export():
    entries = [(i, cd.KIND_RAW, 1) for i in range(cd.DREAMCAST_RESOURCE_INDEX_MAX)]
    cd.check_index_capacity(entries)
    with pytest.raises(ValueError):
        cd.check_index_capacity(entries + [(99999, cd.KIND_RAW, 1)])


def test_dreamcast_folder_capacity_fans_res_across_many_256_file_subfolders():
    entries = [(i, cd.KIND_RAW, 1) for i in range(5056)]
    cd.check_folder_capacity(entries)


def test_dreamcast_folder_capacity_rejects_a_subfolder_over_256_files():
    entries = [(i % 256, cd.KIND_RAW, 1) for i in range(257)]
    with pytest.raises(ValueError, match="RES/00"):
        cd.check_folder_capacity(entries)


def test_dreamcast_exports_menu_now_that_it_links():
    assert cd.dreamcast_overlay_exported("menu")
