#----------------------
# | test_field_movie_census.py
# | Description: Checks field_movie_census.py against the PS1 image: scene map ids for two known scenes, and the scenes whose maps play a FIELD movie.
# | Author: suinevere
# | Dependencies: tools/field_movie_census.py, the PS1 disc image
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(PORT, "tools"))

import field_movie_census  # noqa: E402


#----------------------
# | disc_or_skip
# | Description: An opened census disc, skipping the test when the PS1 image is absent.
# | Author: suinevere
# | Dependencies: field_movie_census.CensusDisc
# | Globals: N/A
# | Params: N/A
# | Returns: CensusDisc
#----------------------
def disc_or_skip():
    path = field_movie_census.default_image()
    if not os.path.exists(path):
        pytest.skip("no PS1 image")
    return field_movie_census.CensusDisc(path)


def test_known_scene_maps():
    disc = disc_or_skip()
    assert field_movie_census.scene_map_id(disc.scene(857)) + field_movie_census.MAP_RESOURCE_BASE == 578
    assert field_movie_census.scene_map_id(disc.scene(19)) == 18


def test_movie_ids_in_range():
    rows = field_movie_census.census(disc_or_skip())
    assert rows, "no scene plays a FIELD movie"
    assert {row.movie for row in rows} <= set(range(7))
