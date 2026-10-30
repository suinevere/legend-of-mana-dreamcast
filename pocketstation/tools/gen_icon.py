"""----------------------
 | gen_icon.py
 | Description: Writes the GAME header's icon from RES1506.BIN's PS1 save icon: the ARGB4444 palette and the first frame doubled to 32x32, as dreamcast/src/card/vmu_format.c's vmu_icon_from_ps1 converts save icons.
 | Author: suinevere
 | Dependencies: dreamcast/src/card/vmu_format.c (vmu_icon_from_ps1)
 | Globals: N/A
 | Params: RES1506.BIN OUT.bin
 | Returns: N/A
 ----------------------"""

import struct
import sys

"""----------------------
 | PS1_CLUT_OFFSET / PS1_FRAME_OFFSET / ICON_BYTES
 | Description: Where a PS1 save's 16-colour CLUT and first 16x16 frame sit, and the bytes the output fills from 0x260 to the code at 0x480: 32 of palette and one 512-byte frame.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
PS1_CLUT_OFFSET = 0x60
PS1_FRAME_OFFSET = 0x80
ICON_BYTES = 32 + 512


"""----------------------
 | vmu_icon
 | Description: Converts a PS1 save's icon to the VMS palette and first frame: colour 0x0000 stays transparent, others become opaque ARGB4444, and each pixel doubles with the left pixel in the high nibble.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: data -- the PS1 save's bytes
 | Returns: ICON_BYTES bytes
 ----------------------"""
def vmu_icon(data):
    if data[:2] != b"SC":
        raise ValueError("no SC header")
    palette = []
    for i in range(16):
        c = struct.unpack_from("<H", data, PS1_CLUT_OFFSET + 2 * i)[0]
        r, g, b = (c & 0x1F) >> 1, ((c >> 5) & 0x1F) >> 1, ((c >> 10) & 0x1F) >> 1
        palette.append(0 if c == 0 else 0xF000 | (r << 8) | (g << 4) | b)
    frame = bytearray(512)
    for y in range(32):
        for x in range(0, 32, 2):
            byte = data[PS1_FRAME_OFFSET + (y // 2) * 8 + (x // 2) // 2]
            pixel = byte >> 4 if (x // 2) & 1 else byte & 0x0F
            frame[y * 16 + x // 2] = (pixel << 4) | pixel
    return struct.pack("<16H", *palette) + bytes(frame)


"""----------------------
 | main
 | Description: Reads RES1506.BIN and writes the icon bytes.
 | Author: suinevere
 | Dependencies: vmu_icon
 | Globals: N/A
 | Params: argv -- RES1506.BIN OUT.bin
 | Returns: 0, or 2 on bad arguments
 ----------------------"""
def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 2:
        sys.stderr.write("usage: gen_icon.py RES1506.BIN OUT.bin\n")
        return 2
    with open(argv[0], "rb") as f:
        data = f.read()
    with open(argv[1], "wb") as f:
        f.write(vmu_icon(data))
    return 0


if __name__ == "__main__":
    sys.exit(main())
