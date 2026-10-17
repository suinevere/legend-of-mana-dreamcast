#----------------------
# | test_convert_disc_dc.py
# | Description: Checks the Dreamcast target exports TITLE's sound effect set and title song raw instead of skipping them, and exports every other live resource raw with a fanned-out RES/<XX> path.
# |              Saturn keeps its allowlist, flat paths and index/folder caps byte-for-byte unchanged.
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
    skipped = cd.skipped_resources(targets.DREAMCAST)
    assert 21 not in skipped and 23 not in skipped
    assert 22 in skipped and 24 in skipped


def test_saturn_still_skips_every_sound_resource():
    skipped = cd.skipped_resources(targets.SATURN)
    assert set(cd.SKIPPED_SOUND_RESOURCES) <= set(skipped)
    assert skipped == cd.SKIPPED_RESOURCES + cd.SKIPPED_SOUND_RESOURCES + cd.wmap_resources.SKIPPED


def test_dreamcast_exports_every_live_resource_no_step_claimed():
    Resource = cd.disc_image.Resource
    table = [Resource(i, 1000 + i, 2048, True) for i in range(8)]
    table[5] = Resource(5, 1005, 2048, False)
    entries = [(1, cd.KIND_OVERLAY, 16), (3, cd.KIND_STREAM, 16)]
    overlays = {1, 2}
    assert cd.dreamcast_raw_resources(table, entries, overlays) == [0, 4, 6, 7]


def test_saturn_keeps_its_allowlist():
    assert cd.targets.SATURN.convert_rules


def test_dreamcast_res_subfolder_is_index_shifted_by_eight_bits_in_hex():
    assert cd.dreamcast_res_subfolder(0x123) == "01"
    assert cd.dreamcast_res_subfolder(0) == "00"
    assert cd.dreamcast_res_subfolder(0x1FF) == "01"


def test_dreamcast_resource_path_fans_out_into_res_subfolders():
    data = os.path.join("o", "cd", "data")
    assert cd.resource_path("o", 0x123, cd.KIND_RAW, targets.DREAMCAST) == os.path.join(data, "RES", "01", "RES0291.BIN")
    assert cd.resource_path("o", 5, cd.KIND_STREAM, targets.DREAMCAST) == os.path.join(data, "RES", "00", "RES0005.BIN")
    assert cd.resource_path("o", 14, cd.KIND_OVERLAY, targets.DREAMCAST) == os.path.join(data, "OVL", "OVL14.BIN")


def test_dreamcast_overlay_exported_skips_unlinked_overlays_with_regions(monkeypatch):
    assert "field" in targets.DREAMCAST.regions
    monkeypatch.setattr(targets, "DREAMCAST_UNLINKED_OVERLAYS", ("field",))
    assert not cd.dreamcast_overlay_exported(targets.DREAMCAST, "field")


def test_dreamcast_exports_wmap_now_that_it_links():
    assert "wmap" not in targets.DREAMCAST_UNLINKED_OVERLAYS
    assert cd.dreamcast_overlay_exported(targets.DREAMCAST, "wmap")


def test_dreamcast_exports_field_now_that_it_links():
    assert "field" not in targets.DREAMCAST_UNLINKED_OVERLAYS
    assert cd.dreamcast_overlay_exported(targets.DREAMCAST, "field")


def test_dreamcast_overlay_exported_is_true_for_a_linked_region():
    assert cd.dreamcast_overlay_exported(targets.DREAMCAST, "checkps")


def test_dreamcast_overlay_exported_is_true_with_no_region_table():
    assert targets.SATURN.regions is None
    assert cd.dreamcast_overlay_exported(targets.SATURN, "field")


def test_saturn_resource_path_is_unchanged_by_the_target_default():
    data = os.path.join("o", "cd", "data")
    assert cd.resource_path("o", 178, cd.KIND_STREAM) == os.path.join(data, "RES", "RES0178.BIN")
    assert cd.resource_path("o", 178, cd.KIND_STREAM, targets.SATURN) == os.path.join(data, "RES", "RES0178.BIN")


def test_dreamcast_index_capacity_is_6144_and_covers_every_live_resource():
    assert cd.DREAMCAST_RESOURCE_INDEX_MAX == 6144
    entries = [(i, cd.KIND_RAW, 1) for i in range(5056)]
    cd.check_index_capacity(entries, targets.DREAMCAST)
    with pytest.raises(ValueError):
        cd.check_index_capacity(entries, targets.SATURN)


def test_dreamcast_index_capacity_still_rejects_an_oversize_export():
    entries = [(i, cd.KIND_RAW, 1) for i in range(cd.DREAMCAST_RESOURCE_INDEX_MAX)]
    cd.check_index_capacity(entries, targets.DREAMCAST)
    with pytest.raises(ValueError):
        cd.check_index_capacity(entries + [(99999, cd.KIND_RAW, 1)], targets.DREAMCAST)


def test_saturn_index_capacity_is_unchanged_by_the_target_default():
    entries = [(i, cd.KIND_RAW, 1) for i in range(cd.RESOURCE_INDEX_MAX)]
    cd.check_index_capacity(entries)
    cd.check_index_capacity(entries, targets.SATURN)
    with pytest.raises(ValueError):
        cd.check_index_capacity(entries + [(9999, cd.KIND_RAW, 1)], targets.SATURN)


def test_dreamcast_folder_capacity_fans_res_across_many_256_file_subfolders():
    entries = [(i, cd.KIND_RAW, 1) for i in range(5056)]
    cd.check_folder_capacity(entries, targets.DREAMCAST)


def test_dreamcast_folder_capacity_rejects_a_subfolder_over_256_files():
    entries = [(i % 256, cd.KIND_RAW, 1) for i in range(257)]
    with pytest.raises(ValueError, match="RES/00"):
        cd.check_folder_capacity(entries, targets.DREAMCAST)


def test_saturn_folder_capacity_is_unchanged_by_the_target_default():
    overlays = [(i, cd.KIND_OVERLAY, 1) for i in range((cd.FOLDER_CAPACITY["OVL"] - 2) // 2)]
    resources = [(1000 + i, cd.KIND_RAW, 1) for i in range(cd.FOLDER_CAPACITY["RES"] - 2)]
    cd.check_folder_capacity(overlays + resources)
    cd.check_folder_capacity(overlays + resources, targets.SATURN)
    with pytest.raises(ValueError, match="RES"):
        cd.check_folder_capacity(resources + [(8999, cd.KIND_STREAM, 1)], targets.SATURN)


def test_dreamcast_exports_menu_now_that_it_links():
    assert cd.dreamcast_overlay_exported(targets.DREAMCAST, "menu")


def test_saturn_skips_menu_until_it_has_byte_order_rules():
    assert not cd.dreamcast_overlay_exported(targets.SATURN, "menu")
