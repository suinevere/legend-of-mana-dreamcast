#----------------------
# | test_overlay_pointer_census.py
# | Description: Checks the secondary overlay pointer census: every word in a ported overlay's packed data that points into its own PS1 image is relocated, named in a function table or explicitly ignored.
# | Author: suinevere
# | Dependencies: tools/overlay_pointer_census.py, tools/overlay_rules.py, build/us/overlays/<name>/<name>.raw
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(PORT, "tools"))

import overlay_pointer_census as census_mod  # noqa: E402

CENSUSED = ["menu", "shop", "golem", "zukan", "gosub", "carda", "cload", "niki", "addhero", "gover"]


def require_raw(name):
    if not os.path.exists(census_mod.raw_path(name)):
        pytest.skip("no PS1 build of %s" % name)


def test_census_kinds_follow_the_packed_ranges():
    require_raw("menu")
    rows = census_mod.census("menu")
    assert rows
    for offset, value, kind in rows:
        assert kind in ("packed", "unpacked")
        assert 0x80140000 <= value < 0x80180000


def test_menu_census_includes_its_content_table():
    require_raw("menu")
    offsets = {o for o, v, k in census_mod.census("menu")}
    assert census_mod.classified("menu") & offsets


@pytest.mark.parametrize("name", CENSUSED)
def test_every_secondary_pointer_is_classified(name):
    require_raw(name)
    missing = {o for o, v, k in census_mod.census(name)} - census_mod.classified(name)
    assert not missing, sorted(hex(o) for o in missing)
