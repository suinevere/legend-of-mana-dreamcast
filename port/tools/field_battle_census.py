#----------------------
# | field_battle_census.py
# | Description: Lists the FIELD scenes whose trigger table (scene record 6) names monster groups, for picking a scene the test image can jump to and fight in.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: SCENE_RESOURCE_BASE, TRIGGER_RECORD_ID, TRIGGER_SCRIPT, HEADER_LAYOUT_OFFSET
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import struct
import sys

#----------------------
# | SCENE_RESOURCE_BASE
# | Description: CD resource of scene 0 (CD_RES_FIELD_SCENE_BASE); scene n is this plus n.
# | Author: suinevere
# | Dependencies: include/main/cd_resources.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SCENE_RESOURCE_BASE = 1548

#----------------------
# | TRIGGER_RECORD_ID
# | Description: Scene record id of the trigger table (FIELD_RESOURCE_TRIGGERS).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TRIGGER_RECORD_ID = 6

#----------------------
# | TRIGGER_SCRIPT
# | Description: Command bit of a trigger region that starts a script rather than a battle (FIELD_TRIGGER_SCRIPT).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TRIGGER_SCRIPT = 0x8000

#----------------------
# | HEADER_LAYOUT_OFFSET
# | Description: A scene file's first header word, the layout section offset, which always equals the header size.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HEADER_LAYOUT_OFFSET = 0x28


#----------------------
# | scene_path
# | Description: Path of a scene's resource file on the Dreamcast disc, fanned out into RES/<index high byte>.
# | Author: suinevere
# | Dependencies: SCENE_RESOURCE_BASE
# | Globals: N/A
# | Params: res_dir -- the disc's RES folder, scene_id -- scene number
# | Returns: file path
#----------------------
def scene_path(res_dir, scene_id):
    index = SCENE_RESOURCE_BASE + scene_id
    return os.path.join(res_dir, "%02X" % (index >> 8), "RES%04d.BIN" % index)


#----------------------
# | monster_groups
# | Description: Reads a raw little-endian scene's records section and returns the monster group of each trigger region without the script bit, in table order.
# | Author: suinevere
# | Dependencies: TRIGGER_RECORD_ID, TRIGGER_SCRIPT, HEADER_LAYOUT_OFFSET
# | Globals: N/A
# | Params: data -- scene file bytes
# | Returns: list of group numbers, empty when the scene has no monster triggers
#----------------------
def monster_groups(data):
    if len(data) < HEADER_LAYOUT_OFFSET or struct.unpack_from("<I", data, 0)[0] != HEADER_LAYOUT_OFFSET:
        return []
    start, end = struct.unpack_from("<2I", data, 0x10)
    if end - start < 4:
        return []
    groups = []
    for i in range(struct.unpack_from("<I", data, start)[0] >> 2):
        record = start + struct.unpack_from("<I", data, start + i * 4)[0]
        if record + 4 > end:
            continue
        record_id, count = struct.unpack_from("<2H", data, record)
        if record_id != TRIGGER_RECORD_ID:
            continue
        for k in range(count):
            region = record + 4 + k * 12
            if region + 12 <= end:
                command = struct.unpack_from("<H", data, region + 8)[0]
                if not command & TRIGGER_SCRIPT:
                    groups.append(command)
    return groups


#----------------------
# | main
# | Description: Prints "scene <id>: groups ..." for every scene on the disc with monster triggers.
# | Author: suinevere
# | Dependencies: scene_path, monster_groups
# | Globals: N/A
# | Params: argv -- [RES folder]
# | Returns: process exit code
#----------------------
def main(argv):
    if len(argv) != 1:
        sys.stderr.write("usage: field_battle_census.py <dreamcast/cd/data/RES>\n")
        return 2
    for scene_id in range(0x400):
        path = scene_path(argv[0], scene_id)
        if os.path.exists(path):
            with open(path, "rb") as f:
                groups = monster_groups(f.read())
            if groups:
                print("scene %d: groups %s" % (scene_id, " ".join(str(g) for g in groups)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
