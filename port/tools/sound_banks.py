
#----------------------
# | sound_banks.py
# | Description: Lists Legend of Mana's sound resources and reads their AKAO instrument banks from the PS1 disc, for the Dreamcast sound tests and the sweep script.
# | Author: suinevere
# | Dependencies: akao.py, disc_image.py, convert_disc.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import functools

import akao
import convert_disc
import disc_image

#----------------------
# | SOUND_RESOURCES
# | Description: (resource, kind) for every sound resource the game loads, from cd_resources.h and field_audio_runtime.c.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SOUND_RESOURCES = sorted(
    [(21, "effect_set"), (22, "bank"), (146, "music")]
    + [(i, "music") for i in range(23, 74)]
    + [(i, "sfx_set") for i in range(81, 119)]
    + [(i, "weapon_sfx") for i in range(131, 142)]
    + [(i, "program") for i in (142, 143)]
    + [(i, "music") for i in range(147, 167)])

#----------------------
# | song_entry
# | Description: Manifest entry for one song header.
# | Author: suinevere
# | Dependencies: akao.parse_song
# | Globals: N/A
# | Params: data -- resource bytes, offset -- song position, resource -- index, section -- section index
# | Returns: dict
#----------------------
def song_entry(data, offset, resource, section):
    song = akao.parse_song(data, offset)
    return {"resource": resource, "section": section, "id": song.header.id, "length": song.header.length,
            "reverb_type": song.header.reverb_type, "channel_mask": song.channel_mask & 0xFFFFFFFF,
            "channels": len(song.channel_offsets)}


#----------------------
# | resource_banks
# | Description: Splits one sound resource by kind into (section, bank offset, effect count) triples, recording songs and programs.
# | Author: suinevere
# | Dependencies: akao.parse_container, akao.parse_effect_table, akao.parse_header, song_entry
# | Globals: N/A
# | Params: data -- resource bytes, resource -- index, kind -- resource kind, report -- manifest being built
# | Returns: list of (section, bank offset, effect count or None)
#----------------------
def resource_banks(data, resource, kind, report):
    if kind == "program":
        report["programs"].append({"resource": resource, "section": 0, "id": akao.parse_header(data).id})
        return []
    if kind == "bank":
        return [(0, 0, None)]
    if kind == "weapon_sfx":
        entries, end = akao.parse_effect_table(data, 0)
        return [(0, end, len(entries))]
    sections = akao.parse_container(data)
    if kind == "sfx_set":
        out = []
        for number, offset in enumerate(sections):
            entries, end = akao.parse_effect_table(data, offset)
            out.append((number, end, len(entries)))
        return out
    if kind == "effect_set":
        report["programs"].append({"resource": resource, "section": 0, "id": akao.parse_header(data, sections[0]).id})
    else:
        for number, offset in enumerate(sections[:-1]):
            report["songs"].append(song_entry(data, offset, resource, number))
    return [(len(sections) - 1, sections[-1], None)]


#----------------------
# | read_live_resource
# | Description: Returns a live resource's bytes from the disc image, or None for an index that is out of range, dead or empty.
# | Author: suinevere
# | Dependencies: disc_image.read_resource
# | Globals: N/A
# | Params: image -- disc_image.DiscImage, table -- parsed resource table, index -- resource index
# | Returns: bytes or None
#----------------------
def read_live_resource(image, table, index):
    if index >= len(table) or not table[index].live or table[index].size <= 0:
        return None
    return disc_image.read_resource(image, table[index])


#----------------------
# | disc_reader
# | Description: Builds a resource reader over a disc image using the SKCDPOSE table location from convert_disc.
# | Author: suinevere
# | Dependencies: disc_image, read_live_resource, convert_disc.TABLE_LBA, convert_disc.TABLE_SIZE
# | Globals: N/A
# | Params: path -- disc image path
# | Returns: callable index -> bytes or None
#----------------------
def disc_reader(path):
    image = disc_image.DiscImage(path)
    blob = image.read_sectors(convert_disc.TABLE_LBA, (convert_disc.TABLE_SIZE + 2047) // 2048)[:convert_disc.TABLE_SIZE]
    return functools.partial(read_live_resource, image, disc_image.parse_resource_table(blob))

