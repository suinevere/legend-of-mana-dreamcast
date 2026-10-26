#----------------------
# | vmu_portraits.py
# | Description: Turns GOSUB's 48x48 pet and egg portraits from the user's disc into 48x32 one-bit VMU screens and writes them to gen/vmu_pet_screens.c, so the Dreamcast port can show the party pet on every VMU.
# | Author: suinevere
# | Dependencies: convert_disc.py (TABLE_LBA, TABLE_SIZE, default_image), disc_image.py, decompress.py, overlay_rules.py (OVERLAYS)
# | Globals: GOSUB_RESOURCE, ARCHIVE_ADDRESS, PET_SCREENS, EGG_SCREENS, EGG_FIRST, SCREEN_BYTES, DARK, MID
# | Params: --image PS1 disc image, --out project directory (gen/ goes under it)
# | Returns: process exit code
#----------------------
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import convert_disc  # noqa: E402
import disc_image  # noqa: E402
import overlay_rules  # noqa: E402
from decompress import decompress  # noqa: E402

#----------------------
# | GOSUB_RESOURCE / ARCHIVE_ADDRESS
# | Description: GOSUB's CD resource and the count word of its portrait archive (four bytes before g_gosub_portrait_archive).
# | Author: suinevere
# | Dependencies: overlay_rules.OVERLAYS, config/us/symbols/gosub_symbol_addrs.txt
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
GOSUB_RESOURCE = 13
ARCHIVE_ADDRESS = 0x80152DF0

#----------------------
# | PET_SCREENS / EGG_SCREENS / EGG_FIRST
# | Description: Screens 0-64 are the pet portraits by species and 65-76 the egg portraits by egg species, which the archive keeps from index 72 (gosub_lists.c's GOSUB_EGG_PORTRAIT_FIRST).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PET_SCREENS = 65
EGG_SCREENS = 12
EGG_FIRST = 72

#----------------------
# | SCREEN_BYTES / DARK / MID
# | Description: A VMU screen's bytes (48x32, one bit a pixel, bit 7 leftmost); colours darker than DARK are set, those darker than MID set on one pixel in four, and lighter ones clear, which keeps a 1-bit picture to outlines and features instead of speckle.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SCREEN_BYTES = 192
DARK = 0.4
MID = 0.6


#----------------------
# | luminance
# | Description: A BGR555 colour's luminance from 0 (black) to 1 (white).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- the colour word
# | Returns: float
#----------------------
def luminance(value):
    return (0.299 * (value & 31) + 0.587 * ((value >> 5) & 31) + 0.114 * ((value >> 10) & 31)) / 31


#----------------------
# | screen
# | Description: One portrait as a VMU screen: nearest-sampled to 32x32 in columns 8-39; a pixel is set on the silhouette's edge (next to transparent or the square's side), where DARK, or on even rows and columns where MID; palette value 0 (transparent) is never set.
# | Author: suinevere
# | Dependencies: luminance, SCREEN_BYTES, DARK, MID
# | Globals: N/A
# | Params: palette -- 16 BGR555 words; pixels -- 48x48 4-bit pixels, low nibble first
# | Returns: SCREEN_BYTES bytes
#----------------------
def screen(palette, pixels):
    lum = [[None] * 32 for _ in range(32)]
    for y in range(32):
        for x in range(32):
            sx, sy = x * 3 // 2, y * 3 // 2
            colour = palette[pixels[sy * 24 + sx // 2] >> (4 * (sx & 1)) & 15]
            lum[y][x] = luminance(colour) if colour else None
    out = bytearray(SCREEN_BYTES)
    for y in range(32):
        for x in range(32):
            value = lum[y][x]
            if value is None:
                continue
            edge = any(not (0 <= x + dx < 32 and 0 <= y + dy < 32) or lum[y + dy][x + dx] is None
                       for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if edge or value < DARK or (value < MID and not (x & 1) and not (y & 1)):
                vx = x + 8
                out[y * 6 + vx // 8] |= 0x80 >> (vx & 7)
    return bytes(out)


#----------------------
# | archive_screens
# | Description: The PET_SCREENS pet screens then the EGG_SCREENS egg screens from a portrait archive whose count word starts data.
# | Author: suinevere
# | Dependencies: screen, PET_SCREENS, EGG_SCREENS, EGG_FIRST
# | Globals: N/A
# | Params: data -- bytes from the archive's count word on
# | Returns: list of screens
#----------------------
def archive_screens(data):
    count = struct.unpack_from("<I", data, 0)[0]
    if count < EGG_FIRST + EGG_SCREENS:
        raise ValueError("portrait archive holds %d portraits" % count)
    picks = list(range(PET_SCREENS)) + list(range(EGG_FIRST, EGG_FIRST + EGG_SCREENS))
    out = []
    for index in picks:
        at = struct.unpack_from("<I", data, 4 + index * 4)[0]
        palette = struct.unpack_from("<16H", data, at)
        out.append(screen(palette, data[at + 32:at + 32 + 48 * 24]))
    return out


#----------------------
# | disc_screens
# | Description: Reads GOSUB from the user's disc, decompresses it and converts its portrait archive.
# | Author: suinevere
# | Dependencies: archive_screens, disc_image, decompress, convert_disc (TABLE_LBA, TABLE_SIZE), overlay_rules.OVERLAYS
# | Globals: N/A
# | Params: path -- the PS1 disc image
# | Returns: list of screens
#----------------------
def disc_screens(path):
    image = disc_image.DiscImage(path)
    blob = image.read_sectors(convert_disc.TABLE_LBA, (convert_disc.TABLE_SIZE + 2047) // 2048)[:convert_disc.TABLE_SIZE]
    table = disc_image.parse_resource_table(blob)
    overlay = decompress(disc_image.read_resource(image, table[GOSUB_RESOURCE])[1:])
    return archive_screens(overlay[ARCHIVE_ADDRESS - overlay_rules.OVERLAYS[GOSUB_RESOURCE].vram:])


#----------------------
# | c_source
# | Description: gen/vmu_pet_screens.c: the screens as lom_vmu_pet_screens, 4-byte aligned for vmu_draw_lcd_rotated, and their count.
# | Author: suinevere
# | Dependencies: SCREEN_BYTES
# | Globals: N/A
# | Params: screens -- list of screens
# | Returns: the source text
#----------------------
def c_source(screens):
    lines = ["/* vmu_pet_screens.c -- generated by port/tools/vmu_portraits.py from the user's disc; do not edit or commit. */",
             "const unsigned int lom_vmu_pet_screen_count = %d;" % len(screens),
             "const unsigned char lom_vmu_pet_screens[%d][%d] __attribute__((aligned(4))) = {" % (len(screens), SCREEN_BYTES)]
    for s in screens:
        lines.append("    {" + ", ".join("0x%02X" % b for b in s) + "},")
    lines.append("};")
    return "\n".join(lines) + "\n"


#----------------------
# | main
# | Description: Writes gen/vmu_pet_screens.c under --out from --image.
# | Author: suinevere
# | Dependencies: disc_screens, c_source, convert_disc.default_image
# | Globals: N/A
# | Params: argv -- command-line arguments
# | Returns: 0
#----------------------
def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", default=convert_disc.default_image())
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    path = os.path.join(args.out, "gen", "vmu_pet_screens.c")
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(c_source(disc_screens(args.image)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
