#----------------------
# | targets.py
# | Description: Converter targets: the Saturn profile with its byte-swap and colour rules, and the little-endian Dreamcast profile with fixed per-overlay arena regions.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: SATURN, DREAMCAST, ARENA_BASE, ARENA_END, TARGETS
# | Params: N/A
# | Returns: N/A
#----------------------
import collections

#----------------------
# | Region
# | Description: One overlay's fixed Dreamcast addresses: where its code and its packed data blob load, and how many bytes each may take.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Region = collections.namedtuple("Region", "code_base code_size data_base data_size")

#----------------------
# | Target
# | Description: A converter profile: name, whether data is byte-swapped to big-endian, per-overlay regions (None keeps the Saturn slot logic) and whether the convert rules run.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Target = collections.namedtuple("Target", "name big_endian regions convert_rules")

#----------------------
# | ARENA_BASE / ARENA_END
# | Description: Dreamcast overlay arena bounds in main RAM; must match the Dreamcast linker script's NOLOAD arena section.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ARENA_BASE = 0x8C800000
ARENA_END = 0x8CE00000

#----------------------
# | SATURN
# | Description: Saturn profile: big-endian output through the per-overlay convert rules, packed into the LWRAM slot overlay_rules.SLOT_BASE describes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SATURN = Target("saturn", True, None, True)

#----------------------
# | DREAMCAST
# | Description: Dreamcast profile: little-endian PS1 bytes kept as they are, each ported overlay given its own code and data region in the arena.
# |              TITLE's data region is 0xC0000 because its packed blob is 0xB03E4 bytes.
# | Author: suinevere
# | Dependencies: Region, ARENA_BASE
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST = Target("dreamcast", False, {
    "checkps": Region(0x8C800000, 0x40000, 0x8C840000, 0x40000),
    "title": Region(0x8C880000, 0x40000, 0x8C8C0000, 0xC0000),
}, False)

#----------------------
# | TARGETS
# | Description: Every converter target keyed by its command-line name.
# | Author: suinevere
# | Dependencies: SATURN, DREAMCAST
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TARGETS = {target.name: target for target in (SATURN, DREAMCAST)}


#----------------------
# | by_name
# | Description: Looks up a converter target by its command-line name.
# | Author: suinevere
# | Dependencies: TARGETS
# | Globals: N/A
# | Params: name -- "saturn" or "dreamcast"
# | Returns: Target; raises ValueError for an unknown name
#----------------------
def by_name(name):
    if name not in TARGETS:
        raise ValueError("unknown target %r, expected one of %s" % (name, ", ".join(sorted(TARGETS))))
    return TARGETS[name]
