#----------------------
# | wmap_resources.py
# | Description: The PS1 disc resources WMAP's Home path loads, by how the converter reads them, and the WMAP resources it marks skipped.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: STREAM, RAW, RULES, SKIPPED
# | Params: N/A
# | Returns: N/A
#----------------------

#----------------------
# | STREAM / RAW
# | Description: RULES kinds: a stream resource is decompressed from its bytes after the leading flag byte, a raw one is exported as read.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STREAM = "stream"
RAW = "raw"

#----------------------
# | RULES
# | Description: Home-path resource id to kind: the interface, land and resident images, land and party animation banks, party sprites, loading, placement, help and tutorial images, and Home's MHM effect set.
# | Author: suinevere
# | Dependencies: STREAM, RAW
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RULES = {0x10C4: STREAM, 0x10C5: STREAM, 0x10C6: STREAM}
RULES.update({res_id: RAW for res_id in range(0x10C7, 0x10D8)})
RULES.update({res_id: RAW for res_id in range(0x114B, 0x1154)})
RULES.update({res_id: RAW for res_id in range(0x11B0, 0x11BA)})
RULES.update({res_id: RAW for res_id in range(0x1453, 0x145D)})

#----------------------
# | SKIPPED
# | Description: WMAP's sound banks and song (wmap_main.c), exported as skipped so the silent sound path reads nothing instead of halting.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SKIPPED = [0x145E, 0x145F, 0x1460, 0x1461]
