#----------------------
# | button_art.py
# | Description: Dreamcast buttons in place of the PS1's in the art the Dreamcast disc carries, by the map pad_dc_map.c plays: ✕ A, ○ B, □ X, △ Y; the field font's glyphs, the world-map help pages' baked-in discs, MENU's shoulder labels and pad icon, and the text that names ✕ as X.
# | Author: suinevere
# | Dependencies: struct, button_icons.py
# | Globals: FONT_RESOURCE, FONT_GLYPHS, FONT_GLYPH_BYTES, STREAM_PATCHES, HELP_GLYPHS, HELP_HEADER, RAW_PATCHES, MENU_RESOURCE, MENU_SHEET, MENU_EDITS, OVERLAY_TEXT
# | Params: N/A
# | Returns: N/A
#----------------------
import math
import struct

import button_icons

#----------------------
# | FONT_RESOURCE / FONT_GLYPH_BYTES
# | Description: The field text font's stream resource, whose glyph for text code c is twelve little-endian 16-bit rows at (c - 0x20) * 24, bit 15 the leftmost pixel.
# | Author: suinevere
# | Dependencies: src/overlays/field/ui/field_text.c (field_draw_glyph_row's FIELD_TEXT_FONT index)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FONT_RESOURCE = 178
FONT_GLYPH_BYTES = 24

#----------------------
# | FONT_GLYPHS
# | Description: The four face-button glyphs, text code to rows: a filled 9x9 disc with its letter cut out, drawn in the text colour; the PS1 had ○ at 0xE0, ✕ at 0xE1, △ at 0xE2 and □ at 0xE3.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FONT_GLYPHS = {
    0xE0: ("...###...",
           ".#######.",
           ".##..####",
           "###.#.###",
           "###..####",
           "###.#.###",
           ".##..###.",
           ".#######.",
           "...###..."),
    0xE1: ("...###...",
           ".#######.",
           ".###.###.",
           "###.#.###",
           "###...###",
           "###.#.###",
           ".##.#.##.",
           ".#######.",
           "...###..."),
    0xE2: ("...###...",
           ".#######.",
           ".##.#.##.",
           "###.#.###",
           "####.####",
           "####.####",
           ".###.###.",
           ".#######.",
           "...###..."),
    0xE3: ("...###...",
           ".#######.",
           ".##.#.##.",
           "###.#.###",
           "####.####",
           "###.#.###",
           ".##.#.##.",
           ".#######.",
           "...###..."),
}


#----------------------
# | patch_font
# | Description: Draws FONT_GLYPHS over their glyphs in the decompressed field font, clearing the rest of each glyph.
# | Author: suinevere
# | Dependencies: FONT_GLYPHS, FONT_GLYPH_BYTES, struct
# | Globals: N/A
# | Params: data -- the decompressed font
# | Returns: patched bytes; raises ValueError when the font is too short to hold the glyphs
#----------------------
def patch_font(data):
    end = (max(FONT_GLYPHS) - 0x20 + 1) * FONT_GLYPH_BYTES
    if len(data) < end:
        raise ValueError("field font is 0x%X bytes, shorter than its button glyphs" % len(data))
    buf = bytearray(data)
    for code, grid in FONT_GLYPHS.items():
        at = (code - 0x20) * FONT_GLYPH_BYTES
        for r in range(FONT_GLYPH_BYTES // 2):
            row = grid[r] if r < len(grid) else ""
            bits = sum(0x8000 >> b for b, c in enumerate(row) if c == "#")
            struct.pack_into("<H", buf, at + r * 2, bits)
    return bytes(buf)


#----------------------
# | HELP_HEADER / HELP_WIDTH / HELP_HEIGHT
# | Description: The world-map help pages' TIM as the US disc has them: 8-bit with a 256-entry CLUT at (704, 504), a 256x240 image at (320, 256).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HELP_HEADER = ((0x10, 0x09), (704, 504, 256, 1), (320, 256, 128, 240))
HELP_WIDTH = 256
HELP_HEIGHT = 240

#----------------------
# | HELP_GLYPHS
# | Description: The PS1 buttons baked into the world-map help pages, as page resource to (x, y, Dreamcast letter, width, height) boxes: the dark 13x12 discs inline in the text and the 33x33 one on the artifact prompt.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HELP_GLYPHS = {
    0x114C: ((203, 50, "A", 13, 12), (216, 78, "A", 13, 12), (154, 133, "A", 13, 12), (41, 173, "A", 13, 12),
             (42, 146, "B", 13, 12), (170, 200, "B", 13, 12), (33, 107, "X", 13, 12)),
    0x114D: ((47, 164, "Y", 13, 12),),
    0x1152: ((145, 82, "X", 33, 33),),
    0x1153: ((74, 118, "A", 13, 12),),
}


#----------------------
# | ps1_colour
# | Description: A 24-bit colour as an opaque 15-bit CLUT entry, pure black nudged to the darkest grey since 0 is transparent.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: rgb -- (r, g, b), 0-255
# | Returns: the CLUT entry
#----------------------
def ps1_colour(rgb):
    value = (rgb[0] >> 3) | (rgb[1] >> 3) << 5 | (rgb[2] >> 3) << 10
    return value or 0x0421


#----------------------
# | is_ps1_disc
# | Description: Whether a CLUT entry belongs to a PS1 button disc: a dark grey or a bright saturated symbol colour, as against the page behind it.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- CLUT entry
# | Returns: True or False
#----------------------
def is_ps1_disc(value):
    rgb = (value & 31, (value >> 5) & 31, (value >> 10) & 31)
    return max(rgb) - min(rgb) > 8 and max(rgb) > 17 or max(rgb) < 12 and max(rgb) - min(rgb) < 6


#----------------------
# | patch_help_page
# | Description: Draws each HELP_GLYPHS box's Dreamcast icon over the PS1 button, its colours in CLUT entries the page's pixels never use, and gives the PS1 disc's uncovered pixels inside the box's ellipse the page colour most common on the box's edge.
# | Author: suinevere
# | Dependencies: HELP_HEADER, HELP_GLYPHS, HELP_RIM_MIN_SIZE, button_icons.ICONS, ps1_colour, is_ps1_disc, clear_help_rim, struct
# | Globals: N/A
# | Params: index -- page resource, data -- the page's raw TIM bytes
# | Returns: patched bytes; raises ValueError when the TIM is not laid out as HELP_HEADER says or the page runs out of free CLUT entries
#----------------------
def patch_help_page(index, data):
    if len(data) < 20 or struct.unpack_from("<II", data, 0) != HELP_HEADER[0] or struct.unpack_from("<HHHH", data, 12) != HELP_HEADER[1]:
        raise ValueError("help page 0x%X is not an 8-bit TIM with the US CLUT" % index)
    image = 8 + struct.unpack_from("<I", data, 8)[0]
    if struct.unpack_from("<HHHH", data, image + 4) != HELP_HEADER[2]:
        raise ValueError("help page 0x%X image is not the US one" % index)
    buf = bytearray(data)
    clut = list(struct.unpack_from("<256H", data, 20))
    pixels = image + 12
    free = [i for i in range(256) if i not in set(data[pixels:pixels + HELP_WIDTH * HELP_HEIGHT])]
    entries = {}
    for x0, y0, letter, w, h in HELP_GLYPHS[index]:
        palette, rows = button_icons.ICONS[(letter, w, h)]
        edge = [buf[pixels + y * HELP_WIDTH + x] for y in (y0, y0 + h - 1) for x in range(x0, x0 + w)]
        edge += [buf[pixels + y * HELP_WIDTH + x] for x in (x0, x0 + w - 1) for y in range(y0, y0 + h)]
        background = [i for i in edge if not is_ps1_disc(clut[i])] or edge
        fill = max(set(background), key=background.count)
        for dy, row in enumerate(rows):
            for dx, digit in enumerate(row):
                at = pixels + (y0 + dy) * HELP_WIDTH + x0 + dx
                if digit == "0":
                    inside = ((dx + 0.5 - w / 2) / (w / 2)) ** 2 + ((dy + 0.5 - h / 2) / (h / 2)) ** 2 <= 1
                    if inside and is_ps1_disc(clut[buf[at]]):
                        buf[at] = fill
                    continue
                colour = ps1_colour(palette[int(digit, 16) - 1])
                if colour not in entries:
                    if not free:
                        raise ValueError("help page 0x%X has no free CLUT entry left" % index)
                    entries[colour] = free.pop(0)
                    clut[entries[colour]] = colour
                buf[at] = entries[colour]
    for x0, y0, letter, w, h in HELP_GLYPHS[index]:
        if w >= HELP_RIM_MIN_SIZE:
            clear_help_rim(buf, clut, pixels, x0, y0, w, h, button_icons.ICONS[(letter, w, h)][1])
    struct.pack_into("<256H", buf, 20, *clut)
    return bytes(buf)


#----------------------
# | HELP_RIM_MIN_SIZE / HELP_RIM_WIDTH / HELP_RIM_SAMPLE / HELP_RIM_TOLERANCE / HELP_RIM_INSIDE
# | Description: Icons at least this wide have the PS1 disc's wider light rim cleared: pixels the icon leaves transparent from HELP_RIM_INSIDE inside its circle to HELP_RIM_WIDTH beyond it whose colour is more than HELP_RIM_TOLERANCE (summed 5-bit channels) from the median of the ring further out take the pixel HELP_RIM_SAMPLE further out along the same radius when that is background too, else the page colour nearest that median, so nearby text is never copied in.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HELP_RIM_MIN_SIZE = 20
HELP_RIM_WIDTH = 2
HELP_RIM_SAMPLE = 5
HELP_RIM_TOLERANCE = 6
HELP_RIM_INSIDE = 2


#----------------------
# | clut_rgb
# | Description: A CLUT entry's three 5-bit channels.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- CLUT entry
# | Returns: (r, g, b), 0-31
#----------------------
def clut_rgb(value):
    return (value & 31, (value >> 5) & 31, (value >> 10) & 31)


#----------------------
# | clear_help_rim
# | Description: Replaces what is left of a PS1 disc's rim around a large help-page icon with the page texture just outside it, so the icon reads perfectly round.
# | Author: suinevere
# | Dependencies: HELP_WIDTH, HELP_HEIGHT, HELP_RIM_WIDTH, HELP_RIM_SAMPLE, HELP_RIM_TOLERANCE, HELP_RIM_INSIDE, clut_rgb, math
# | Globals: N/A
# | Params: buf -- page bytearray, patched in place; clut -- its CLUT entries; pixels -- offset of its pixel data; x0, y0, w, h -- the icon's box; rows -- the icon's digit rows, 0 transparent
# | Returns: N/A
#----------------------
def clear_help_rim(buf, clut, pixels, x0, y0, w, h, rows):
    cx, cy, r = x0 + w / 2, y0 + h / 2, w / 2
    reach = int(r + HELP_RIM_WIDTH + HELP_RIM_SAMPLE + 3)
    around = [(x, y, math.hypot(x + 0.5 - cx, y + 0.5 - cy)) for y in range(int(cy) - reach, int(cy) + reach + 1)
              for x in range(int(cx) - reach, int(cx) + reach + 1) if 0 <= x < HELP_WIDTH and 0 <= y < HELP_HEIGHT]
    ring = sorted(clut_rgb(clut[buf[pixels + y * HELP_WIDTH + x]]) for x, y, d in around
                  if r + HELP_RIM_WIDTH + 1 <= d <= r + HELP_RIM_WIDTH + HELP_RIM_SAMPLE + 2)
    typical = tuple(sorted(c[i] for c in ring)[len(ring) // 2] for i in range(3))
    far = lambda index: sum(abs(a - b) for a, b in zip(clut_rgb(clut[index]), typical))
    plain = min({buf[pixels + y * HELP_WIDTH + x] for x, y, d in around}, key=far)
    for x, y, d in around:
        at = pixels + y * HELP_WIDTH + x
        drawn = 0 <= x - x0 < w and 0 <= y - y0 < h and rows[y - y0][x - x0] != "0"
        if not drawn and r - HELP_RIM_INSIDE <= d <= r + HELP_RIM_WIDTH and far(buf[at]) > HELP_RIM_TOLERANCE:
            k = (d + HELP_RIM_SAMPLE) / d
            sx = min(max(int(cx + (x + 0.5 - cx) * k), 0), HELP_WIDTH - 1)
            sy = min(max(int(cy + (y + 0.5 - cy) * k), 0), HELP_HEIGHT - 1)
            sample = buf[pixels + sy * HELP_WIDTH + sx]
            buf[at] = sample if far(sample) <= HELP_RIM_TOLERANCE else plain


#----------------------
# | MENU_RESOURCE / MENU_SHEET / MENU_SHEET_HEADER
# | Description: The MENU overlay, its 4-bit icon sheet's TIM offset in the decompressed overlay, and that TIM's header: 16 CLUT rows at (0, 480), a 64x256 image at (0, 0).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MENU_RESOURCE = 6
MENU_SHEET = 0x2026C
MENU_SHEET_HEADER = ((0x10, 0x08), (0, 480, 16, 16), (0, 0, 64, 256))

#----------------------
# | MENU_EDITS
# | Description: (x, y, rows) redrawn on the menu sheet, one hex digit a pixel indexing the CLUT row a menu picks and "." transparent: the big L1 and R1 labels as LT and RT, the small L1 and R1 tiles as L and R, L2 and R2 as the D-pad's left and right arrows, the same four small tiles, and the BUTTON icon's PS1 pad as a Dreamcast pad, and the key configuration's ○, △, □ and ✕ discs as B, Y, X and A round 11x11 discs, in the empty cells MENU_ICON_SPRITES gives them, drawn in MENU_DISC_PALETTES' entries: 1 outline, 2 highlight, 3 face, 4 shade, 5 letter.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MENU_EDITS = (
    (128, 48,
     ('01111100111111100',
      '01fff1111fffff100',
      '018ff11111ff11100',
      '018ff11111ff10000',
      '018ff8ff11ff10000',
      '01ffffff11ff10000')),
    (112, 80,
     ('1111111111111110',
      '1fffff811fffff10',
      '1ff11ff111ff1110',
      '1fffff1111ff1000',
      '1ff8ff8111ff1000',
      '1ff11ff811ff1000')),
    (129, 64,
     ('11111111111111',
      '11111111ff1111',
      '111111ffff1111',
      '1111ffffff1111',
      '111111ffff1111',
      '11111111ff1111')),
    (129, 80,
     ('11111111111111',
      '1111ff11111111',
      '1111ffff111111',
      '1111ffffff1111',
      '1111ffff111111',
      '1111ff11111111')),
    (128, 112,
     ('fff2222222222240',
      '2f444fff14444411',
      '224441f114444411',
      '234444f144444411',
      '244444f1f1444411',
      '24444ffff1444441',
      '4144411111444441',
      '0111111111111111')),
    (128, 128,
     ('fff2222222222240',
      '2f444ffff1444411',
      '22444f11f1444411',
      '23444ffff1444411',
      '24444f1f11444411',
      '24444f11f1444441',
      '4144411411444441',
      '0111111111111111')),
    (128, 120,
     ('fff2222222222240',
      '2f444444ff144411',
      '224444ffff144411',
      '23444fffff144411',
      '244441ffff144411',
      '24444411ff144441',
      '4114444114444141',
      '0111111111111111')),
    (128, 136,
     ('fff2222222222240',
      '2f444ff144444411',
      '22444ffff1444411',
      '23444fffff144411',
      '24444ffff1444411',
      '24444ff114444441',
      '4144114114444141',
      '0111111111111111')),
    (101, 64,
     ('...222222222222222...',
      '.2244444111114444422.',
      '244433441111144444442',
      '2444444411111444b4442',
      '244474444444444b4c442',
      '2447774444444444c4442',
      '233373322222223333332',
      '2333332.......2333332',
      '2333332.......2333332',
      '.22222.........22222.')),
    (64, 88,
     ('...1111.........',
      '..155331........',
      '.12535341.......',
      '.12553341.......',
      '.13535341.......',
      '.13553441.......',
      '..144441........',
      '...1111.........')),
    (80, 88,
     ('...1111.........',
      '..152531........',
      '.12535341.......',
      '.12353341.......',
      '.13353341.......',
      '.13353441.......',
      '..144441........',
      '...1111.........')),
    (128, 96,
     ('...1111.........',
      '..152531........',
      '.12535341.......',
      '.12353341.......',
      '.13535341.......',
      '.13535441.......',
      '..144441........',
      '...1111.........')),
    (128, 104,
     ('...1111.........',
      '..125331........',
      '.12535341.......',
      '.12555341.......',
      '.13535341.......',
      '.13535441.......',
      '..144441........',
      '...1111.........')),
    (112, 148,
     ('.....111........',
      '...1122211......',
      '..122333331.....',
      '..125555341.....',
      '.12353335341....',
      '.13355553341....',
      '.13353335341....',
      '..135555341.....',
      '..133333441.....',
      '...1144411......',
      '.....111........',
      '................')),
    (128, 148,
     ('.....111........',
      '...1122211......',
      '..122333331.....',
      '..125333541.....',
      '.12335353341....',
      '.13333533341....',
      '.13333533341....',
      '..133353341.....',
      '..133333441.....',
      '...1144411......',
      '.....111........',
      '................')),
    (144, 148,
     ('.....111........',
      '...1122211......',
      '..122333331.....',
      '..125333541.....',
      '.12335353341....',
      '.13333533341....',
      '.13335353341....',
      '..135333541.....',
      '..133333441.....',
      '...1144411......',
      '.....111........',
      '................')),
    (32, 164,
     ('.....111........',
      '...1122211......',
      '..122333331.....',
      '..123353341.....',
      '.12335353341....',
      '.13353335341....',
      '.13355555341....',
      '..135333541.....',
      '..133333441.....',
      '...1144411......',
      '.....111........',
      '................')),
)

#----------------------
# | MENU_SECOND_CLUT_SIZE / MENU_DISC_PALETTES / MENU_ICON_CLUT_CODES / MENU_ICON_CLUT_CODE_TABLE
# | Description: The menu's second 256-colour CLUT, stored after the sheet image and uploaded to row 499, whose palettes 12-15 the US disc leaves empty; there go the X, Y, B and A discs' colours (transparent, outline, highlight, face, shade, letter), and icons 35, 36, 37 and 34 (□, △, ○, ✕) are pointed at them in the overlay's icon CLUT code table.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MENU_SECOND_CLUT_SIZE = 512
MENU_DISC_PALETTES = {
    12: (0x0000, 0x0421, 0x3BDF, 0x133E, 0x0658, 0x00C9),
    13: (0x0000, 0x0421, 0x3BAE, 0x22C6, 0x1603, 0x04C0),
    14: (0x0000, 0x0421, 0x7ED0, 0x6DC8, 0x5525, 0x2441),
    15: (0x0000, 0x0421, 0x3E1F, 0x1CFB, 0x1094, 0x0428),
}
MENU_ICON_CLUT_CODE_TABLE = 0xFDB8
MENU_ICON_CLUT_CODES = {35: (0x1B, 0x1C), 36: (0x1B, 0x1D), 37: (0x06, 0x1E), 34: (0x1B, 0x1F)}

#----------------------
# | MENU_SPRITE_DEFS / MENU_ICON_SPRITES
# | Description: The overlay's icon sprite table, (u, v, w, h) a byte each, and the key configuration discs moved there: their PS1 16x8 tiles leave no room for a round button, so icons 34-37 take 16x12 cells the US sheet leaves empty.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MENU_SPRITE_DEFS = 0xFBF4
MENU_ICON_SPRITES = {37: ((64, 88, 16, 8), (112, 148, 16, 12)), 36: ((80, 88, 16, 8), (128, 148, 16, 12)),
                     35: ((128, 96, 16, 8), (144, 148, 16, 12)), 34: ((128, 104, 16, 8), (32, 164, 16, 12))}

#----------------------
# | OVERLAY_TEXT
# | Description: Overlay to its text swaps, each the same length: the PS1's "X button" meant ✕, which the Dreamcast's A plays, X being its □.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAY_TEXT = {5: ((b"X button", b"A button"),), 6: ((b"X button", b"A button"),)}


#----------------------
# | patch_menu_sheet
# | Description: Draws MENU_EDITS onto the MENU overlay's icon sheet, fills MENU_DISC_PALETTES into its second CLUT and points the key configuration discs at them and at their larger cells.
# | Author: suinevere
# | Dependencies: MENU_SHEET, MENU_SHEET_HEADER, MENU_EDITS, MENU_SECOND_CLUT_SIZE, MENU_DISC_PALETTES, MENU_ICON_CLUT_CODE_TABLE, MENU_ICON_CLUT_CODES, MENU_SPRITE_DEFS, MENU_ICON_SPRITES, struct
# | Globals: N/A
# | Params: buf -- the decompressed overlay, a bytearray patched in place
# | Returns: N/A; raises ValueError when the sheet, its second CLUT or the icon CLUT codes are not the US ones
#----------------------
def patch_menu_sheet(buf):
    if struct.unpack_from("<II", buf, MENU_SHEET) != MENU_SHEET_HEADER[0] or struct.unpack_from("<HHHH", buf, MENU_SHEET + 12) != MENU_SHEET_HEADER[1]:
        raise ValueError("MENU's icon sheet is not a 4-bit TIM with the US CLUT at 0x%X" % MENU_SHEET)
    image = MENU_SHEET + 8 + struct.unpack_from("<I", buf, MENU_SHEET + 8)[0]
    if struct.unpack_from("<HHHH", buf, image + 4) != MENU_SHEET_HEADER[2]:
        raise ValueError("MENU's icon sheet image is not the US one")
    row_bytes = MENU_SHEET_HEADER[2][2] * 2
    for x0, y0, rows in MENU_EDITS:
        for dy, row in enumerate(rows):
            for dx, digit in enumerate(row):
                x, at = x0 + dx, image + 12 + (y0 + dy) * row_bytes + (x0 + dx) // 2
                value = 0 if digit == "." else int(digit, 16)
                shift = 4 * (x & 1)
                buf[at] = (buf[at] & ~(15 << shift) & 0xFF) | value << shift
    second = image + struct.unpack_from("<I", buf, image)[0]
    if any(buf[second + 12 * 32:second + MENU_SECOND_CLUT_SIZE]):
        raise ValueError("MENU's second CLUT palettes 12-15 are not empty")
    for palette, colours in MENU_DISC_PALETTES.items():
        struct.pack_into("<%dH" % len(colours), buf, second + palette * 32, *colours)
    for icon, (old, new) in MENU_ICON_SPRITES.items():
        if tuple(buf[MENU_SPRITE_DEFS + icon * 4:MENU_SPRITE_DEFS + icon * 4 + 4]) != old:
            raise ValueError("MENU icon %d's sprite is not the US one" % icon)
        buf[MENU_SPRITE_DEFS + icon * 4:MENU_SPRITE_DEFS + icon * 4 + 4] = bytes(new)
    for icon, (old, new) in MENU_ICON_CLUT_CODES.items():
        if buf[MENU_ICON_CLUT_CODE_TABLE + icon] != old:
            raise ValueError("MENU icon %d's CLUT code is not the US one" % icon)
        buf[MENU_ICON_CLUT_CODE_TABLE + icon] = new


#----------------------
# | patch_overlay
# | Description: A decompressed overlay as the Dreamcast disc carries it: OVERLAY_TEXT's swaps made, on MENU the icon sheet redrawn, and on WMAP the legend tables pointed at the moved lines.
# | Author: suinevere
# | Dependencies: OVERLAY_TEXT, MENU_RESOURCE, patch_menu_sheet, WMAP_RESOURCE, patch_legend_tables
# | Globals: N/A
# | Params: index -- overlay resource, data -- decompressed bytes
# | Returns: bytes
#----------------------
def patch_overlay(index, data):
    buf = bytearray(data)
    for old, new in OVERLAY_TEXT.get(index, ()):
        buf = bytearray(bytes(buf).replace(old, new))
    if index == MENU_RESOURCE:
        patch_menu_sheet(buf)
    if index == WMAP_RESOURCE:
        patch_legend_tables(buf)
    return bytes(buf)


#----------------------
# | LEGEND_PAGE / LEGEND_PALETTE_PAGE / LEGEND_PAGE_HEADERS / LEGEND_PAGE_WIDTH
# | Description: The world map's interface VRAM page holding the corner legend "SELECT:Help, □:Artifacts, △:Mana", the resident page holding its palette at (736, 508), each a 16bpp TIM header (x, y, w, h), and their width in VRAM words.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LEGEND_PAGE = 0x10C4
LEGEND_PALETTE_PAGE = 0x10C6
LEGEND_PAGE_HEADERS = {LEGEND_PAGE: (704, 0, 320, 256), LEGEND_PALETTE_PAGE: (704, 256, 320, 256)}
LEGEND_PAGE_WIDTH = 320

#----------------------
# | LEGEND_PALETTE_EDITS
# | Description: (entry, PS1 colour, Dreamcast colour) in the legend's palette at (736, 508): the □'s pink becomes the X button's yellow, and the SELECT label's two greys the dark yellow and dark green that shade the X and Y discs.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LEGEND_PALETTE_X, LEGEND_PALETTE_Y = 736, 508
LEGEND_PALETTE_EDITS = ((0xA, 0x613F, 0x173E), (0xC, 0x358B, 0x012E), (0xD, 0x5650, 0x0941))

#----------------------
# | LEGEND_LINES / LEGEND_LINE_SIZE / LEGEND_MOVED_TO / LEGEND_LINE_ROWS / LEGEND_LINE_COUNT
# | Description: The legend's four 96x8 lines start at texel (0, 224), packed 8 rows apart with no room for a round button; they are copied, text one row down, into 10-row slots at the free texels from (128, 216), still inside the legend's texture page.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LEGEND_LINES = (0, 224)
LEGEND_LINE_SIZE = (96, 8)
LEGEND_MOVED_TO = (128, 216)
LEGEND_LINE_ROWS = 10
LEGEND_LINE_COUNT = 4

#----------------------
# | LEGEND_CLEAR / LEGEND_EDITS
# | Description: Texel boxes (x0, y0, x1, y1) blanked in the moved legend (the SELECT label with its T's last outline texel, and the □ and △ discs), then (x, y, rows) drawn there, one hex digit a texel and "." left blank: the D-pad with its down arm dark as if pressed, down opening Help here, and round 10x10 yellow X and green Y discs as wide as the D-pad, their letters 6 wide so they centre, lit from the top left like the PS1's discs: a thin black outline, a light grey glint top left, a dark band down the right and along the bottom, and a dark letter.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LEGEND_CLEAR = ((131, 216, 155, 226), (155, 218, 156, 219), (143, 226, 155, 246))
LEGEND_EDITS = (
    (145, 216,
     ('...eeee...',
      '...e76e...',
      '...e66e...',
      'eeee66eeee',
      'e76666666e',
      'e66666666e',
      'eeee33eeee',
      '...e33e...',
      '...e22e...',
      '...eeee...')),
    (145, 226,
     ('...eeee...',
      '.ee8aaaee.',
      'e8caaaacce',
      'e8acaacace',
      'eaaaccaace',
      'eaacaacace',
      'eacaaaacce',
      'ecaaaaacce',
      '.eeccccee.',
      '...eeee...')),
    (145, 236,
     ('...eeee...',
      '.ee8fffee.',
      'e8dffffdde',
      'e8fdffdfde',
      'efffddffde',
      'efffddffde',
      'efffddffde',
      'edfffffdde',
      '.eeddddee.',
      '...eeee...')),
)

#----------------------
# | WMAP_RESOURCE / LEGEND_GLYPHS / LEGEND_CHARS / LEGEND_CHAR_COUNT / LEGEND_GLYPH_V_BIAS
# | Description: The WMAP overlay, its legend line glyphs, (u, pad, v + 0x20, pad, w as u16, h as u16) each, and its eight placed lines, (glyph, pad, x as u16, y, pad) each, at their offsets in the decompressed overlay; the moved lines get v, h 10 and a y one higher so the text stays put.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WMAP_RESOURCE = 3
LEGEND_GLYPHS = 0x8077C
LEGEND_CHARS = 0x8079C
LEGEND_CHAR_COUNT = 8
LEGEND_GLYPH_V_BIAS = 0x20


#----------------------
# | check_legend_page
# | Description: Raises unless a world-map VRAM page has the 16bpp TIM header LEGEND_PAGE_HEADERS gives it.
# | Author: suinevere
# | Dependencies: LEGEND_PAGE_HEADERS, struct
# | Globals: N/A
# | Params: index -- page resource, data -- decompressed page bytes
# | Returns: N/A; raises ValueError on a mismatch
#----------------------
def check_legend_page(index, data):
    if len(data) < 20 or data[0] != 0x10 or data[4] != 2 or struct.unpack_from("<HHHH", data, 12) != LEGEND_PAGE_HEADERS[index]:
        raise ValueError("world-map page 0x%X is not the US VRAM page" % index)


#----------------------
# | get_legend_texel
# | Description: Reads one 4-bit texel from the interface page.
# | Author: suinevere
# | Dependencies: LEGEND_PAGE_WIDTH
# | Globals: N/A
# | Params: data -- page bytes; x, y -- texel
# | Returns: palette entry 0-15
#----------------------
def get_legend_texel(data, x, y):
    return (data[20 + y * LEGEND_PAGE_WIDTH * 2 + x // 2] >> (4 * (x & 1))) & 15


#----------------------
# | put_legend_texel
# | Description: Sets one 4-bit texel on the interface page, the even texel in each byte's low nibble.
# | Author: suinevere
# | Dependencies: LEGEND_PAGE_WIDTH
# | Globals: N/A
# | Params: buf -- page bytearray; x, y -- texel; value -- palette entry 0-15
# | Returns: N/A
#----------------------
def put_legend_texel(buf, x, y, value):
    at = 20 + y * LEGEND_PAGE_WIDTH * 2 + x // 2
    shift = 4 * (x & 1)
    buf[at] = (buf[at] & ~(15 << shift) & 0xFF) | value << shift


#----------------------
# | patch_legend
# | Description: Copies the world map's corner legend into 10-row lines at LEGEND_MOVED_TO and redraws its buttons there with LEGEND_CLEAR and LEGEND_EDITS.
# | Author: suinevere
# | Dependencies: check_legend_page, get_legend_texel, put_legend_texel, LEGEND_PAGE, LEGEND_LINES, LEGEND_LINE_SIZE, LEGEND_MOVED_TO, LEGEND_LINE_ROWS, LEGEND_LINE_COUNT, LEGEND_CLEAR, LEGEND_EDITS
# | Globals: N/A
# | Params: data -- decompressed interface page bytes
# | Returns: patched bytes
#----------------------
def patch_legend(data):
    check_legend_page(LEGEND_PAGE, data)
    buf = bytearray(data)
    for line in range(LEGEND_LINE_COUNT):
        for y in range(LEGEND_LINE_ROWS):
            for x in range(LEGEND_LINE_SIZE[0]):
                value = 0
                if 1 <= y <= LEGEND_LINE_SIZE[1]:
                    value = get_legend_texel(data, LEGEND_LINES[0] + x, LEGEND_LINES[1] + line * LEGEND_LINE_SIZE[1] + y - 1)
                put_legend_texel(buf, LEGEND_MOVED_TO[0] + x, LEGEND_MOVED_TO[1] + line * LEGEND_LINE_ROWS + y, value)
    for x0, y0, x1, y1 in LEGEND_CLEAR:
        for y in range(y0, y1):
            for x in range(x0, x1):
                put_legend_texel(buf, x, y, 0)
    for x0, y0, rows in LEGEND_EDITS:
        for dy, row in enumerate(rows):
            for dx, digit in enumerate(row):
                if digit != ".":
                    put_legend_texel(buf, x0 + dx, y0 + dy, int(digit, 16))
    return bytes(buf)


#----------------------
# | patch_legend_tables
# | Description: Points WMAP's four legend glyphs at the moved 10-row lines and raises each placed line a row, checking the US values first.
# | Author: suinevere
# | Dependencies: LEGEND_GLYPHS, LEGEND_CHARS, LEGEND_CHAR_COUNT, LEGEND_GLYPH_V_BIAS, LEGEND_LINES, LEGEND_LINE_SIZE, LEGEND_MOVED_TO, LEGEND_LINE_ROWS, LEGEND_LINE_COUNT, struct
# | Globals: N/A
# | Params: buf -- the decompressed WMAP overlay, a bytearray patched in place
# | Returns: N/A; raises ValueError when the tables are not the US ones
#----------------------
def patch_legend_tables(buf):
    for line in range(LEGEND_LINE_COUNT):
        at = LEGEND_GLYPHS + line * 8
        old_v = (LEGEND_LINES[1] + line * LEGEND_LINE_SIZE[1] + LEGEND_GLYPH_V_BIAS) & 0xFF
        if (buf[at], buf[at + 2], struct.unpack_from("<HH", buf, at + 4)) != (LEGEND_LINES[0], old_v, LEGEND_LINE_SIZE):
            raise ValueError("WMAP's legend glyph %d is not the US one" % line)
        buf[at] = LEGEND_MOVED_TO[0]
        buf[at + 2] = (LEGEND_MOVED_TO[1] + line * LEGEND_LINE_ROWS + LEGEND_GLYPH_V_BIAS) & 0xFF
        struct.pack_into("<HH", buf, at + 4, LEGEND_LINE_SIZE[0], LEGEND_LINE_ROWS)
    for char in range(LEGEND_CHAR_COUNT):
        at = LEGEND_CHARS + char * 6
        if buf[at] >= LEGEND_LINE_COUNT or buf[at + 4] < 1:
            raise ValueError("WMAP's legend line %d is not the US one" % char)
        buf[at + 4] -= 1


#----------------------
# | patch_legend_palette
# | Description: Recolours LEGEND_PALETTE_EDITS in the legend's palette on the resident page.
# | Author: suinevere
# | Dependencies: check_legend_page, LEGEND_PALETTE_PAGE, LEGEND_PAGE_HEADERS, LEGEND_PAGE_WIDTH, LEGEND_PALETTE_X, LEGEND_PALETTE_Y, LEGEND_PALETTE_EDITS
# | Globals: N/A
# | Params: data -- decompressed resident page bytes
# | Returns: patched bytes; raises ValueError when an entry is not the PS1 colour
#----------------------
def patch_legend_palette(data):
    check_legend_page(LEGEND_PALETTE_PAGE, data)
    buf = bytearray(data)
    x0, y0 = LEGEND_PAGE_HEADERS[LEGEND_PALETTE_PAGE][:2]
    for entry, old, new in LEGEND_PALETTE_EDITS:
        at = 20 + ((LEGEND_PALETTE_Y - y0) * LEGEND_PAGE_WIDTH + LEGEND_PALETTE_X - x0 + entry) * 2
        if struct.unpack_from("<H", buf, at)[0] != old:
            raise ValueError("the world-map legend palette entry 0x%X is not the US colour" % entry)
        struct.pack_into("<H", buf, at, new)
    return bytes(buf)


#----------------------
# | RAW_PATCHES
# | Description: World-map resource index to the Dreamcast patch applied to its bytes, decompressed for the VRAM pages.
# | Author: suinevere
# | Dependencies: patch_help_page, HELP_GLYPHS, patch_legend, patch_legend_palette
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RAW_PATCHES = {index: (lambda data, index=index: patch_help_page(index, data)) for index in HELP_GLYPHS}
RAW_PATCHES.update({LEGEND_PAGE: patch_legend, LEGEND_PALETTE_PAGE: patch_legend_palette})


#----------------------
# | patch_raw
# | Description: A raw resource as the Dreamcast disc carries it: patched if RAW_PATCHES names it, else unchanged.
# | Author: suinevere
# | Dependencies: RAW_PATCHES
# | Globals: N/A
# | Params: index -- resource index, data -- raw bytes
# | Returns: bytes
#----------------------
def patch_raw(index, data):
    rule = RAW_PATCHES.get(index)
    return data if rule is None else rule(data)


#----------------------
# | STREAM_PATCHES
# | Description: Stream resource index to the Dreamcast patch applied to its decompressed bytes.
# | Author: suinevere
# | Dependencies: patch_font
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STREAM_PATCHES = {FONT_RESOURCE: patch_font}


#----------------------
# | patch_stream
# | Description: A decompressed stream resource as the Dreamcast disc carries it: patched if STREAM_PATCHES names it, else unchanged.
# | Author: suinevere
# | Dependencies: STREAM_PATCHES
# | Globals: N/A
# | Params: index -- resource index, data -- decompressed bytes
# | Returns: bytes
#----------------------
def patch_stream(index, data):
    rule = STREAM_PATCHES.get(index)
    return data if rule is None else rule(data)
