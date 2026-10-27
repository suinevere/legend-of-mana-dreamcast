
#----------------------
# | targets.py
# | Description: The converter's Dreamcast target: little-endian PS1 bytes with fixed per-overlay arena regions.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: DREAMCAST, SECONDARY_REGION, SECONDARY_OVERLAYS, DREAMCAST_UNLINKED_OVERLAYS, ARENA_BASE, ARENA_END, TARGETS
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
# | Description: A converter profile: name and per-overlay regions.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Target = collections.namedtuple("Target", "name regions")

#----------------------
# | ARENA_BASE / ARENA_END
# | Description: Dreamcast overlay arena bounds in main RAM; must match the Dreamcast linker script's NOLOAD arena section.
# |              ARENA_BASE sits at 0x8C480000, below the original four regions with room for WMAP and FIELD, after D6's SPU RAM image and D7's movie buffers grew main past 0x8C400000.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ARENA_BASE = 0x8C480000
ARENA_END = 0x8CE00000

#----------------------
# | SECONDARY_REGION
# | Description: The one arena region every secondary overlay links at: code holds MENU's 0xF644 bytes of MIPS text with room to spare, data the largest PS1 data plus bss (GOSUB, 0x2DA04 bytes).
# | Author: suinevere
# | Dependencies: Region
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECONDARY_REGION = Region(0x8CB00000, 0x40000, 0x8CB40000, 0x40000)

#----------------------
# | SECONDARY_OVERLAYS
# | Description: The overlays sharing SECONDARY_REGION; FIELD reaches their entries through overlay_link.DC_ENTRY_ALIASES.
# | Author: suinevere
# | Dependencies: SECONDARY_REGION
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECONDARY_OVERLAYS = ("gname", "menu", "shop", "golem", "zukan", "gosub", "carda", "cload", "niki", "addhero", "gover", "movie")

#----------------------
# | DREAMCAST
# | Description: Dreamcast profile: little-endian PS1 bytes kept as they are, each ported overlay given its own code and data region in the arena.
# |              TITLE's data region is 0xC0000 because its packed blob is 0xB03E4 bytes.
# |              WSEL's data region holds its 0x782DC-byte blob; its render buffers live in main.
# |              FIELD's region sits below the other four, sized from its measured SH-4 text (0x532B7 bytes, grown once the
# |              8 FIELD files missing from DECOMP_FIELD were added) and its whole data span, bss included (0x5815B bytes),
# |              each plus 25% rounded up to 64 KB (Task 6, once FIELD got its own overlay link). WMAP's code and data bases
# |              shift up by FIELD's growth (0x20000) to stay clear of it; WMAP's own sizing follows the same rule from its
# |              measured SH-4 text (0x493AC bytes) and its whole PS1 data span, buffers included (0xF03D0 bytes plus a
# |              0x3738-byte tail).
# |              Every secondary overlay (GNAME, MENU and D4's nine) shares SECONDARY_REGION, since the PS1 loads them all at 0x80140000 and only one is resident.
# | Author: suinevere
# | Dependencies: Region, ARENA_BASE, SECONDARY_REGION
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST = Target("dreamcast", {
    "field": Region(0x8C480000, 0x70000, 0x8C4F0000, 0x70000),
    "wmap": Region(0x8C560000, 0x60000, 0x8C5C0000, 0x140000),
    "checkps": Region(0x8C800000, 0x40000, 0x8C840000, 0x40000),
    "title": Region(0x8C880000, 0x40000, 0x8C8C0000, 0xC0000),
    "gname": SECONDARY_REGION,
    "wsel": Region(0x8CA00000, 0x40000, 0x8CA40000, 0xC0000),
    "menu": SECONDARY_REGION,
    "shop": SECONDARY_REGION,
    "golem": SECONDARY_REGION,
    "zukan": SECONDARY_REGION,
    "gover": SECONDARY_REGION,
    "movie": SECONDARY_REGION,
    "addhero": SECONDARY_REGION,
    "niki": SECONDARY_REGION,
    "cload": SECONDARY_REGION,
    "carda": SECONDARY_REGION,
    "gosub": SECONDARY_REGION,
})

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
# | TARGETS
# | Description: Every converter target keyed by its command-line name.
# | Author: suinevere
# | Dependencies: DREAMCAST
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TARGETS = {target.name: target for target in (DREAMCAST,)}


#----------------------
# | by_name
# | Description: Looks up a converter target by its command-line name.
# | Author: suinevere
# | Dependencies: TARGETS
# | Globals: N/A
# | Params: name -- "dreamcast"
# | Returns: Target; raises ValueError for an unknown name
#----------------------
def by_name(name):
    if name not in TARGETS:
        raise ValueError("unknown target %r, expected one of %s" % (name, ", ".join(sorted(TARGETS))))
    return TARGETS[name]
