#----------------------
# | targets.py
# | Description: Converter targets: the Saturn profile with its byte-swap and colour rules, and the little-endian Dreamcast profile with fixed per-overlay arena regions.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: SATURN, DREAMCAST, DREAMCAST_UNLINKED_OVERLAYS, SATURN_UNLINKED_OVERLAYS, ARENA_BASE, ARENA_END, TARGETS
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
# |              ARENA_BASE was lowered to 0x8C400000 to give WMAP and FIELD room below the original four regions.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ARENA_BASE = 0x8C400000
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
# |              GNAME's data region holds its 0xCB18-byte blob and bss with room to spare; its render contexts live in main.
# |              WSEL's data region holds its 0x782DC-byte blob; its render buffers live in main.
# |              FIELD's region sits below the other four, sized from its measured SH-4 text (0x532B7 bytes, grown once the
# |              8 FIELD files missing from DECOMP_FIELD were added) and its whole data span, bss included (0x5815B bytes),
# |              each plus 25% rounded up to 64 KB (Task 6, once FIELD got its own overlay link). WMAP's code and data bases
# |              shift up by FIELD's growth (0x20000) to stay clear of it; WMAP's own sizing follows the same rule from its
# |              measured SH-4 text (0x493AC bytes) and its whole PS1 data span, buffers included (0xF03D0 bytes plus a
# |              0x3738-byte tail).
# |              MENU's code region holds its PS1 0xFBF4 bytes of MIPS text with room to spare, and its data region its 0x1BDE8-byte blob plus bss.
# | Author: suinevere
# | Dependencies: Region, ARENA_BASE
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST = Target("dreamcast", False, {
    "field": Region(0x8C400000, 0x70000, 0x8C470000, 0x70000),
    "wmap": Region(0x8C4E0000, 0x60000, 0x8C540000, 0x140000),
    "checkps": Region(0x8C800000, 0x40000, 0x8C840000, 0x40000),
    "title": Region(0x8C880000, 0x40000, 0x8C8C0000, 0xC0000),
    "gname": Region(0x8C980000, 0x40000, 0x8C9C0000, 0x40000),
    "wsel": Region(0x8CA00000, 0x40000, 0x8CA40000, 0xC0000),
    "menu": Region(0x8CB00000, 0x40000, 0x8CB40000, 0x40000),
}, False)

#----------------------
# | DREAMCAST_UNLINKED_OVERLAYS
# | Description: Overlays that have a Dreamcast region but no overlay link yet, so main keeps their resident code; empty since Task 6 gave FIELD its own overlay.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST_UNLINKED_OVERLAYS = ()

#----------------------
# | SATURN_UNLINKED_OVERLAYS
# | Description: Overlays the Dreamcast links that the Saturn build does not yet, since they have no big-endian data rules; Saturn skips them on the disc and in its link scripts.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SATURN_UNLINKED_OVERLAYS = ("menu",)

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
