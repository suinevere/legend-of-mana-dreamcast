#----------------------
# | wmap_layout.py
# | Description: Converts WMAP's 1208-byte land layout resources (0x12EA + world_map_cell) to Saturn byte order.
# | Author: suinevere
# | Dependencies: struct
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

#----------------------
# | LAYOUT_FIRST
# | Description: CD resource index of layout 0; layout n is LAYOUT_FIRST + n, as in wmap_load_land_layout.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LAYOUT_FIRST = 0x12EA

#----------------------
# | LAYOUT_COUNT
# | Description: Number of layouts, one per WSEL grid cell (19 by 19).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LAYOUT_COUNT = 361

#----------------------
# | LAYOUT_SIZE
# | Description: Byte size of one WmapLayout: header 8, 36 cells of 12, scenes 512, entry flags 256.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LAYOUT_SIZE = 1208

#----------------------
# | CELL_COUNT
# | Description: Terrain cells in a layout's six-by-six grid.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CELL_COUNT = 36

#----------------------
# | SCENES_OFFSET
# | Description: Byte offset of the u16 scenes[256] table.
# | Author: suinevere
# | Dependencies: CELL_COUNT
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SCENES_OFFSET = 8 + CELL_COUNT * 12

#----------------------
# | ENTRY_FLAGS_OFFSET
# | Description: Byte offset of the u8 entry_flags[256] table.
# | Author: suinevere
# | Dependencies: SCENES_OFFSET
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ENTRY_FLAGS_OFFSET = SCENES_OFFSET + 256 * 2

#----------------------
# | MAX_LANDS
# | Description: Largest land count a layout may hold; scenes[] has four variants per land.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAX_LANDS = 64


#----------------------
# | SECTOR
# | Description: CD sector size each packed layout is padded to, so one layout is one sector-offset read.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECTOR = 2048


#----------------------
# | LayoutFormatError
# | Description: Raised when a layout resource does not match the WmapLayout shape.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class LayoutFormatError(ValueError):
    pass


#----------------------
# | convert_layout
# | Description: Returns a copy of a layout with the header, each cell's two u16 words and the scene table byte-swapped; spirit and entry-flag bytes stay raw.
# | Author: suinevere
# | Dependencies: LAYOUT_SIZE, CELL_COUNT, SCENES_OFFSET, MAX_LANDS, LayoutFormatError
# | Globals: N/A
# | Params: data -- raw resource bytes
# | Returns: converted bytes
#----------------------
def convert_layout(data):
    if len(data) != LAYOUT_SIZE:
        raise LayoutFormatError("layout is 0x%X bytes, expected 0x%X" % (len(data), LAYOUT_SIZE))
    buf = bytearray(data)
    start_x, start_y, land_count = struct.unpack_from("<HHi", buf, 0)
    if not 0 <= land_count <= MAX_LANDS:
        raise LayoutFormatError("layout land count %d is outside 0..%d" % (land_count, MAX_LANDS))
    struct.pack_into(">HHi", buf, 0, start_x, start_y, land_count)
    for i in range(CELL_COUNT):
        pos = 8 + i * 12 + 8
        struct.pack_into(">HH", buf, pos, *struct.unpack_from("<HH", buf, pos))
    struct.pack_into(">256H", buf, SCENES_OFFSET, *struct.unpack_from("<256H", buf, SCENES_OFFSET))
    return bytes(buf)


#----------------------
# | pack_layouts
# | Description: Concatenates every WMAP layout cell 0..LAYOUT_COUNT-1, each run through convert_layout and padded to one sector, in cell order.
# | Author: suinevere
# | Dependencies: convert_layout, LAYOUT_COUNT, LAYOUT_SIZE, SECTOR, LayoutFormatError
# | Globals: N/A
# | Params: read_layout -- callable(cell) -> raw PS1 layout bytes for that cell, or None if the cell's resource is missing
# | Returns: packed bytes, LAYOUT_COUNT * SECTOR long, layout n at n * SECTOR
#----------------------
def pack_layouts(read_layout):
    out = bytearray()
    for cell in range(LAYOUT_COUNT):
        raw = read_layout(cell)
        if raw is None:
            raise LayoutFormatError("layout %d is missing" % cell)
        out += convert_layout(raw) + bytes(SECTOR - LAYOUT_SIZE)
    return bytes(out)
