#----------------------
# | vmu_art.py
# | Description: Our own VMU drawing for the ranch ring menu, which the Dreamcast copy of its icon sheet (resource 0xBEE) shows in place of the PocketStation.
# | Author: suinevere
# | Dependencies: struct
# | Globals: RING_SHEET_RESOURCE, ICON, CELL_X, CELL_Y, PALETTE_INDICES, SHEET_HEADER, VMU_ICON
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

#----------------------
# | RING_SHEET_RESOURCE
# | Description: CD resource of ring menu 6's icon sheet (FIELD_RING_ICON_RESOURCE_BASE 0xBE8 + 6), the ranch's.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RING_SHEET_RESOURCE = 0xBEE

#----------------------
# | ICON / CELL_X / CELL_Y
# | Description: The PocketStation's icon number in the sheet and its 32x32 cell's top-left pixel; the icon's palette is CLUT row ICON.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ICON = 10
CELL_X = 64
CELL_Y = 32

#----------------------
# | PALETTE_INDICES
# | Description: The colours of CLUT row ICON the drawing uses: 1, 2, 3, 4, 5, 7 and 9 a grey ramp, 6 and 10 near black, 8 white; 0 is transparent.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PALETTE_INDICES = (1, 2, 3, 4, 5, 6, 7, 8, 9, 10)

#----------------------
# | SHEET_HEADER
# | Description: The sheet's TIM as the US disc has it: 4-bit with a CLUT, 16x16 CLUT entries at (0, 480), a 64x256 image at (768, 0).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SHEET_HEADER = ((0x10, 0x08), (0, 480, 16, 16), (768, 0, 64, 256))

#----------------------
# | VMU_ICON
# | Description: The VMU, one hex digit a pixel indexing CLUT row ICON: a white body with a grey outline, the dark screen in its frame, the D-pad, A and B, and the Mode and Sleep buttons.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
VMU_ICON = (
    "00000000000000000000000000000000",
    "00000000000055555555000000000000",
    "00000000555553333335555500000000",
    "00000005999999999999999250000000",
    "00000059888888888888888725000000",
    "00000598333333333333333372500000",
    "00000598344444444444444372500000",
    "0000059834AAAAAAAAAAAAA372500000",
    "0000059834AAAAAAAAA44AA372500000",
    "0000059834AAAAAAAAA44AA372500000",
    "0000059834AAAAAAAAAAAAA372500000",
    "0000059834AAAAAAAAAAAAA372500000",
    "0000059834AAAAAAAAAAAAA372500000",
    "0000059834AAAAAAAAAAAAA372500000",
    "00000598333333333333333372500000",
    "00000598888888888888888872500000",
    "00000598888888888888888872500000",
    "00000598888888888888866872500000",
    "00000598884668888888646672500000",
    "00000598884668888888666672500000",
    "00000598446664488888866872500000",
    "00000598666A66688866888872500000",
    "00000598666666688646688872500000",
    "00000598886668888666688872500000",
    "00000598886668888866888872500000",
    "00000527788888888888888772500000",
    "00000052277888888888877225000000",
    "00000005522774484487722550000000",
    "00000000055227777772255000000000",
    "00000000000552222225500000000000",
    "00000000000005555550000000000000",
    "00000000000000000000000000000000",
)


#----------------------
# | patch_ring_sheet
# | Description: Draws VMU_ICON over the PocketStation's cell of the ranch ring sheet, leaving every other byte as it was.
# | Author: suinevere
# | Dependencies: SHEET_HEADER, VMU_ICON, CELL_X, CELL_Y, struct
# | Globals: N/A
# | Params: data -- the sheet's raw TIM bytes
# | Returns: patched bytes; raises ValueError when the TIM is not laid out as SHEET_HEADER says
#----------------------
def patch_ring_sheet(data):
    if len(data) < 20 or struct.unpack_from("<II", data, 0) != SHEET_HEADER[0]:
        raise ValueError("ring sheet is not a 4-bit TIM with a CLUT")
    clut_len = struct.unpack_from("<I", data, 8)[0]
    if struct.unpack_from("<HHHH", data, 12) != SHEET_HEADER[1] or len(data) < 8 + clut_len + 12:
        raise ValueError("ring sheet CLUT is not the US one")
    image = 8 + clut_len
    if struct.unpack_from("<HHHH", data, image + 4) != SHEET_HEADER[2]:
        raise ValueError("ring sheet image is not the US one")
    buf = bytearray(data)
    row_bytes = SHEET_HEADER[2][2] * 2
    for y, row in enumerate(VMU_ICON):
        at = image + 12 + (CELL_Y + y) * row_bytes + CELL_X // 2
        for x in range(0, 32, 2):
            buf[at + x // 2] = int(row[x], 16) | int(row[x + 1], 16) << 4
    return bytes(buf)
