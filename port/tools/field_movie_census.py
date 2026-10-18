#----------------------
# | field_movie_census.py
# | Description: Lists the FIELD scenes whose map holds a movie animation (kind 4), with the movie number (FIELD_MOVIE_STREAM_RESOURCE's head.b.unk1) and the definition's flags, straight from the PS1 image.
# | Author: suinevere
# | Dependencies: disc_image.py, convert_disc.py, field_map.py, tools/splat_ext/decompress.py
# | Globals: SCENE_RESOURCE_BASE, MAP_RESOURCE_BASE, MAP_QUEUED_READ_LIMIT, SCENE_COUNT, SCENE_ACTORS_ENTRY, MAP_ID_OFFSET, MAP_ID_MASK, MOVIE_KIND
# | Params: N/A
# | Returns: N/A
#----------------------
import collections
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tools", "splat_ext"))

import convert_disc  # noqa: E402
import disc_image  # noqa: E402
import field_map  # noqa: E402
from decompress import decompress  # noqa: E402

#----------------------
# | SCENE_RESOURCE_BASE / MAP_RESOURCE_BASE / MAP_QUEUED_READ_LIMIT / SCENE_COUNT
# | Description: Resource index of scene 0 and of map 0 (CD_RES_FIELD_MAP_BASE), the first map id streamed compressed rather than read raw, and the scene ids scanned.
# | Author: suinevere
# | Dependencies: include/main/cd_resources.h, src/overlays/field/scene/field_scene_load.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SCENE_RESOURCE_BASE = 1548
MAP_RESOURCE_BASE = 180
MAP_QUEUED_READ_LIMIT = 15
SCENE_COUNT = 0x400

#----------------------
# | SCENE_ACTORS_ENTRY / MAP_ID_OFFSET / MAP_ID_MASK / MOVIE_KIND
# | Description: Header entry of a scene's actors section, the offset of its map-id halfword inside it, that halfword's map-id bits (field_update_scene), and FIELD_TILE_ANIM_MOVIE.
# | Author: suinevere
# | Dependencies: src/overlays/field/scene/field_scene_transition.c, src/overlays/field/internal/field_scene_internal.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SCENE_ACTORS_ENTRY = 0x14
MAP_ID_OFFSET = 4
MAP_ID_MASK = 0x7FFF
MOVIE_KIND = 4

#----------------------
# | MovieRow
# | Description: One scene whose map plays a movie: scene id, map id, movie number and definition flags.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MovieRow = collections.namedtuple("MovieRow", "scene map movie flags")


#----------------------
# | default_image
# | Description: The PS1 image the disc converter uses.
# | Author: suinevere
# | Dependencies: convert_disc.default_image
# | Globals: N/A
# | Params: N/A
# | Returns: image path
#----------------------
def default_image():
    return convert_disc.default_image()


#----------------------
# | CensusDisc
# | Description: Reads scene and map resources from the PS1 image, decompressing streamed maps.
# | Author: suinevere
# | Dependencies: disc_image, convert_disc.TABLE_LBA, convert_disc.TABLE_SIZE, decompress
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class CensusDisc:
    #----------------------
    # | __init__
    # | Description: Opens the image and reads its resource table.
    # | Author: suinevere
    # | Dependencies: disc_image.DiscImage, disc_image.parse_resource_table
    # | Globals: N/A
    # | Params: path -- PS1 image
    # | Returns: N/A
    #----------------------
    def __init__(self, path):
        self.image = disc_image.DiscImage(path)
        blob = self.image.read_sectors(convert_disc.TABLE_LBA, (convert_disc.TABLE_SIZE + 2047) // 2048)[:convert_disc.TABLE_SIZE]
        self.table = disc_image.parse_resource_table(blob)

    #----------------------
    # | resource
    # | Description: One live resource's bytes, or None.
    # | Author: suinevere
    # | Dependencies: disc_image.read_resource
    # | Globals: N/A
    # | Params: index -- resource index
    # | Returns: bytes or None
    #----------------------
    def resource(self, index):
        if index >= len(self.table) or not self.table[index].live or self.table[index].size <= 0:
            return None
        return disc_image.read_resource(self.image, self.table[index])

    #----------------------
    # | scene
    # | Description: A scene's raw bytes.
    # | Author: suinevere
    # | Dependencies: resource
    # | Globals: SCENE_RESOURCE_BASE
    # | Params: scene_id -- scene number
    # | Returns: bytes or None
    #----------------------
    def scene(self, scene_id):
        return self.resource(SCENE_RESOURCE_BASE + scene_id)

    #----------------------
    # | map
    # | Description: A map's PS1 bytes as FIELD loads them: read raw below MAP_QUEUED_READ_LIMIT, decompressed above it.
    # | Author: suinevere
    # | Dependencies: resource, decompress
    # | Globals: MAP_RESOURCE_BASE, MAP_QUEUED_READ_LIMIT
    # | Params: map_id -- map number
    # | Returns: bytes or None
    #----------------------
    def map(self, map_id):
        raw = self.resource(MAP_RESOURCE_BASE + map_id)
        if raw is None or map_id < MAP_QUEUED_READ_LIMIT:
            return raw
        return bytes(decompress(raw[1:]))


#----------------------
# | scene_map_id
# | Description: The map id field_update_scene reads from a scene's actors section.
# | Author: suinevere
# | Dependencies: SCENE_ACTORS_ENTRY, MAP_ID_OFFSET, MAP_ID_MASK
# | Globals: N/A
# | Params: data -- scene bytes
# | Returns: map id
#----------------------
def scene_map_id(data):
    actors = struct.unpack_from("<I", data, SCENE_ACTORS_ENTRY)[0]
    return struct.unpack_from("<H", data, actors + MAP_ID_OFFSET)[0] & MAP_ID_MASK


#----------------------
# | map_movies
# | Description: The (movie number, flags) of each movie definition in a map, or none for a map the walk cannot read.
# | Author: suinevere
# | Dependencies: CensusDisc.map, field_map.anim_defs
# | Globals: MOVIE_KIND
# | Params: disc -- CensusDisc, map_id -- map number
# | Returns: list of (movie, flags)
#----------------------
def map_movies(disc, map_id):
    data = disc.map(map_id)
    if data is None:
        return []
    try:
        return [(head1, flags) for _, _, kind, head1, flags in field_map.anim_defs(data) if kind == MOVIE_KIND]
    except (field_map.MapFormatError, IndexError, struct.error):
        return []


#----------------------
# | census
# | Description: Every scene whose map holds a movie animation, one row per movie definition.
# | Author: suinevere
# | Dependencies: CensusDisc.scene, scene_map_id, map_movies
# | Globals: SCENE_COUNT
# | Params: disc -- CensusDisc
# | Returns: list of MovieRow
#----------------------
def census(disc):
    rows = []
    movies_by_map = {}
    for scene_id in range(SCENE_COUNT):
        scene = disc.scene(scene_id)
        if scene is None or len(scene) < 0x28:
            continue
        try:
            map_id = scene_map_id(scene)
        except struct.error:
            continue
        if map_id not in movies_by_map:
            movies_by_map[map_id] = map_movies(disc, map_id)
        for movie, flags in movies_by_map[map_id]:
            rows.append(MovieRow(scene_id, map_id, movie, flags))
    return rows


#----------------------
# | main
# | Description: Prints "scene <id> map <map> movie <n> flags 0x<flags>" for every row of the census.
# | Author: suinevere
# | Dependencies: census, CensusDisc, default_image
# | Globals: N/A
# | Params: argv -- [PS1 image]
# | Returns: process exit code
#----------------------
def main(argv):
    disc = CensusDisc(argv[0] if argv else default_image())
    for row in census(disc):
        print("scene %d map %d movie %d flags 0x%08X" % (row.scene, row.map, row.movie, row.flags))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
