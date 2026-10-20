#----------------------
# | scene_patches.py
# | Description: Dreamcast-only edits to raw scene resources: the ranch (MHM_ALC2.IMG) gets back its Ring Ring Land option and the four lines its transfer menu shows.
# | Author: suinevere
# | Dependencies: overlay_text.py, struct
# | Globals: RANCH_RESOURCE, RANCH_RING_MENUS, RANCH_RING_RING_LAND_OPTION, SCENE_STRINGS_WORD, SCENE_HEADER_WORDS, PATCHES
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

import overlay_text

#----------------------
# | RANCH_RESOURCE
# | Description: CD resource index of the ranch scene, MHM_ALC2.IMG.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RANCH_RESOURCE = 1573

#----------------------
# | RANCH_RING_MENUS / RANCH_RING_RING_LAND_OPTION
# | Description: File offsets and US excluded masks of the ranch's two open_ring_menu commands, and the mask bit of option 10, which opens the transfer submenu.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RANCH_RING_MENUS = ((0x2D2, 0x1C5F), (0x2DB, 0x1C3F))
RANCH_RING_RING_LAND_OPTION = 0x400

#----------------------
# | SCENE_STRINGS_WORD / SCENE_HEADER_WORDS
# | Description: Header word holding the strings section offset, and the header's word count; the words after it are the sections that follow.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SCENE_STRINGS_WORD = 2
SCENE_HEADER_WORDS = 10


#----------------------
# | add_scene_strings
# | Description: Appends lines at the end of a scene's strings section, points their empty entries at them, and moves every later section by the padded length.
# | Author: suinevere
# | Dependencies: overlay_text.encode_scene, struct
# | Globals: SCENE_STRINGS_WORD, SCENE_HEADER_WORDS
# | Params: data -- scene bytes, lines -- index to line
# | Returns: new scene bytes; raises ValueError when an entry is not empty or an offset passes a u16
#----------------------
def add_scene_strings(data, lines):
    header = list(struct.unpack_from("<%dI" % SCENE_HEADER_WORDS, data))
    strings, end = header[SCENE_STRINGS_WORD], header[SCENE_STRINGS_WORD + 1]
    buf = bytearray(data[:end])
    added = bytearray()
    for index, text in sorted(lines.items()):
        slot = strings + 2 * index
        current = struct.unpack_from("<H", buf, slot)[0]
        if buf[strings + current] not in (0x00, 0x06):
            raise ValueError("scene string %d is not empty" % index)
        offset = end - strings + len(added)
        if offset > 0xFFFF:
            raise ValueError("scene string %d lands past a u16 offset" % index)
        struct.pack_into("<H", buf, slot, offset)
        added += overlay_text.encode_scene(text)
    added += bytes(-len(added) % 4)
    for word in range(SCENE_STRINGS_WORD + 1, SCENE_HEADER_WORDS):
        header[word] += len(added)
    struct.pack_into("<%dI" % SCENE_HEADER_WORDS, buf, 0, *header)
    return bytes(buf + added + data[end:])


#----------------------
# | patch_ranch
# | Description: Clears option 10 from both ranch ring-menu masks and fills strings 33, 61, 62 and 63.
# | Author: suinevere
# | Dependencies: add_scene_strings, overlay_text.load_lines, struct
# | Globals: RANCH_RING_MENUS, RANCH_RING_RING_LAND_OPTION
# | Params: data -- original ranch scene bytes
# | Returns: patched bytes; raises ValueError when the commands are not where the US disc has them
#----------------------
def patch_ranch(data):
    buf = bytearray(data)
    for offset, mask in RANCH_RING_MENUS:
        if buf[offset:offset + 3] != b"\x28\x64\x06" or struct.unpack_from("<H", buf, offset + 3)[0] != mask:
            raise ValueError("ranch ring menu at 0x%X is not the US one" % offset)
        struct.pack_into("<H", buf, offset + 3, mask & ~RANCH_RING_RING_LAND_OPTION)
    return add_scene_strings(bytes(buf), overlay_text.load_lines()["scene_mhm_alc2"])


#----------------------
# | PATCHES
# | Description: Resource index to the Dreamcast patch applied to its raw bytes.
# | Author: suinevere
# | Dependencies: patch_ranch
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PATCHES = {RANCH_RESOURCE: patch_ranch}


#----------------------
# | patch
# | Description: A raw resource as the Dreamcast disc carries it: patched if PATCHES names it, else unchanged.
# | Author: suinevere
# | Dependencies: PATCHES
# | Globals: N/A
# | Params: index -- resource index, data -- raw bytes
# | Returns: bytes
#----------------------
def patch(index, data):
    rule = PATCHES.get(index)
    return data if rule is None else rule(data)
