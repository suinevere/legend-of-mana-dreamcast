#----------------------
# | test_button_art.py
# | Description: The Dreamcast copies of the PS1's button art show Dreamcast buttons: the field font's ○ ✕ △ □ glyphs become B, A, Y and X discs and the world-map help pages' baked-in glyphs the sheet's buttons, with every other byte and every used palette entry as the disc has it.
# | Author: suinevere
# | Dependencies: port/tools/button_art.py, port/tools/button_icons.py, port/tools/convert_disc.py, port/tools/disc_image.py, tools/splat_ext/decompress.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import struct
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(REPO, "port", "tools"))

import button_art  # noqa: E402
import convert_disc  # noqa: E402
import disc_image  # noqa: E402
from decompress import decompress  # noqa: E402


@pytest.fixture(scope="module")
def disc():
    path = convert_disc.default_image()
    if not os.path.exists(path):
        pytest.skip("no PS1 disc image (set LOM_PS1_IMAGE)")
    image = disc_image.DiscImage(path)
    blob = image.read_sectors(convert_disc.TABLE_LBA, (convert_disc.TABLE_SIZE + 2047) // 2048)[:convert_disc.TABLE_SIZE]
    return image, disc_image.parse_resource_table(blob)


def _font(disc):
    image, table = disc
    return bytes(decompress(disc_image.read_resource(image, table[button_art.FONT_RESOURCE])[1:]))


def _glyph(font, code):
    at = (code - 0x20) * 24
    return ["".join("#" if struct.unpack_from("<H", font, at + r * 2)[0] & (0x8000 >> b) else "." for b in range(16))
            for r in range(12)]


def test_the_font_shows_dreamcast_discs_for_the_four_face_buttons(disc):
    font = _font(disc)
    assert _glyph(font, 0xE1)[2].startswith("#.....#"), "the US font no longer has ✕ at 0xE1"
    out = button_art.patch_stream(button_art.FONT_RESOURCE, font)
    assert len(out) == len(font)
    for code, grid in button_art.FONT_GLYPHS.items():
        drawn = [row[:9] for row in _glyph(out, code)]
        assert drawn[:len(grid)] == list(grid), hex(code)
        assert all(row == "." * 9 for row in drawn[len(grid):]), hex(code)
    changed = {i for i in range(len(font)) if font[i] != out[i]}
    allowed = {(code - 0x20) * 24 + k for code in button_art.FONT_GLYPHS for k in range(24)}
    assert changed and changed <= allowed


def test_each_disc_is_a_9_by_9_letter_in_a_filled_circle():
    letters = {"A": 0xE1, "B": 0xE0, "Y": 0xE2, "X": 0xE3}
    assert set(button_art.FONT_GLYPHS) == set(letters.values())
    for grid in button_art.FONT_GLYPHS.values():
        assert len(grid) == 9 and all(len(row) == 9 for row in grid)
        assert grid[0] == "...###..." and grid[4][0] == "#" and grid[4][8] == "#"
    assert len({tuple(g) for g in button_art.FONT_GLYPHS.values()}) == 4


def test_other_resources_pass_through():
    assert button_art.patch_stream(177, b"abc") == b"abc"


def _page(data):
    clut = struct.unpack_from("<256H", data, 20)
    at = 8 + struct.unpack_from("<I", data, 8)[0] + 12
    pixels = data[at:at + 256 * 240]
    return clut, pixels


def _rgb(value):
    return ((value & 31) << 3, ((value >> 5) & 31) << 3, ((value >> 10) & 31) << 3)


def _ps_symbols(clut, pixels, box):
    x0, y0, w, h = box
    found = 0
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            r, g, b = _rgb(clut[pixels[y * 256 + x]])
            if max(r, g, b) > 140 and max(r, g, b) - min(r, g, b) > 70 and (b > 180 or (r > 200 and g < 120) or (g > 200 and r < 100)):
                found += 1
    return found


def _in_rim(x, y, bx, by, w, h):
    d = ((x + 0.5 - bx - w / 2) ** 2 + (y + 0.5 - by - h / 2) ** 2) ** 0.5
    return w >= button_art.HELP_RIM_MIN_SIZE and d <= w / 2 + button_art.HELP_RIM_WIDTH


@pytest.mark.parametrize("index", sorted(button_art.HELP_GLYPHS))
def test_help_pages_show_dreamcast_buttons(disc, index):
    image, table = disc
    raw = disc_image.read_resource(image, table[index])
    out = button_art.patch_raw(index, raw)
    assert len(out) == len(raw)
    clut, pixels = _page(raw)
    new_clut, new_pixels = _page(out)
    used = set(pixels)
    for x, y, letter, w, h in button_art.HELP_GLYPHS[index]:
        assert _ps_symbols(clut, pixels, (x, y, w, h)) > 0, (hex(index), x, y)
        palette, rows = button_art.button_icons.ICONS[(letter, w, h)]
        for dy, row in enumerate(rows):
            for dx, digit in enumerate(row):
                if digit != "0":
                    got = _rgb(new_clut[new_pixels[(y + dy) * 256 + x + dx]])
                    want = tuple(c & 0xF8 for c in palette[int(digit, 16) - 1])
                    want = want if any(want) else (8, 8, 8)
                    assert got == want, (hex(index), x + dx, y + dy)
    for i in range(256):
        if i in used:
            assert new_clut[i] == clut[i], (hex(index), i)
    boxes = button_art.HELP_GLYPHS[index]
    for y in range(240):
        for x in range(256):
            if not any(bx <= x < bx + w and by <= y < by + h or _in_rim(x, y, bx, by, w, h) for bx, by, _, w, h in boxes):
                assert new_pixels[y * 256 + x] == pixels[y * 256 + x], (hex(index), x, y)


def test_help_page_glyphs_cover_every_ps_button():
    assert {i: len(g) for i, g in button_art.HELP_GLYPHS.items()} == {0x114C: 7, 0x114D: 1, 0x1152: 1, 0x1153: 1}


def _overlay(disc, index):
    image, table = disc
    return bytes(decompress(disc_image.read_resource(image, table[index])[1:]))


def _sheet_pixel(data, x, y):
    at = button_art.MENU_SHEET + 8 + 524 + 12 + y * 128 + x // 2
    return (data[at] >> (4 * (x & 1))) & 15


def test_the_menu_sheet_shows_dreamcast_shoulder_labels_and_pad(disc):
    menu = _overlay(disc, button_art.MENU_RESOURCE)
    out = button_art.patch_overlay(button_art.MENU_RESOURCE, menu)
    assert len(out) == len(menu)
    covered = set()
    for x0, y0, rows in button_art.MENU_EDITS:
        for dy, row in enumerate(rows):
            for dx, digit in enumerate(row):
                want = 0 if digit == "." else int(digit, 16)
                assert _sheet_pixel(out, x0 + dx, y0 + dy) == want, (x0 + dx, y0 + dy)
                covered.add((x0 + dx, y0 + dy))
    sheet_start = button_art.MENU_SHEET + 8 + 524 + 12
    second = sheet_start + 128 * 256
    codes = {button_art.MENU_ICON_CLUT_CODE_TABLE + icon for icon in button_art.MENU_ICON_CLUT_CODES}
    codes |= {button_art.MENU_SPRITE_DEFS + icon * 4 + k for icon in button_art.MENU_ICON_SPRITES for k in range(4)}
    for i in range(len(menu)):
        if menu[i] != out[i] and not (sheet_start <= i < second + button_art.MENU_SECOND_CLUT_SIZE) and i not in codes:
            assert menu[i:i + 8] == b"X button", hex(i)
    assert _sheet_pixel(menu, 137, 49) != _sheet_pixel(out, 137, 49)


@pytest.mark.parametrize("index,count", [(5, 1), (6, 6)])
def test_x_button_text_becomes_a_button(disc, index, count):
    data = _overlay(disc, index)
    assert data.count(b"X button") == count
    out = button_art.patch_overlay(index, data)
    assert out.count(b"X button") == 0 and out.count(b"A button") == data.count(b"A button") + count


@pytest.mark.parametrize("x,y,symbol,icon", [(64, 88, 7, 37), (80, 88, 0xB, 36), (128, 96, 7, 35), (128, 104, 0xC, 34)])
def test_the_key_configuration_discs_get_dreamcast_palettes(disc, x, y, symbol, icon):
    menu = _overlay(disc, button_art.MENU_RESOURCE)
    out = button_art.patch_overlay(button_art.MENU_RESOURCE, menu)
    assert any(_sheet_pixel(menu, x + dx, y + dy) == symbol for dx in range(11) for dy in range(8))
    old, new = button_art.MENU_ICON_CLUT_CODES[icon]
    table = button_art.MENU_ICON_CLUT_CODE_TABLE
    assert menu[table + icon] == old and out[table + icon] == new
    second = button_art.MENU_SHEET + 8 + 524 + 12 + 128 * 256 + (new & 15) * 32
    colours = button_art.MENU_DISC_PALETTES[new & 15]
    assert not any(menu[second:second + 32])
    assert struct.unpack_from("<%dH" % len(colours), out, second) == colours
    assert {_sheet_pixel(out, x + dx, y + dy) for dx in range(16) for dy in range(8)} <= set(range(len(colours)))


def _legend_texel(data, x, y):
    return (data[20 + y * 640 + x // 2] >> (4 * (x & 1))) & 15


def _legend_pages(disc):
    image, table = disc
    page = bytes(decompress(disc_image.read_resource(image, table[button_art.LEGEND_PAGE])[1:]))
    return page, button_art.patch_raw(button_art.LEGEND_PAGE, page)


def _legend_drawn():
    return {(x0 + dx, y0 + dy) for x0, y0, rows in button_art.LEGEND_EDITS
            for dy, row in enumerate(rows) for dx, c in enumerate(row) if c != "."}


def _in_boxes(x, y, boxes):
    return any(x0 <= x < x1 and y0 <= y < y1 for x0, y0, x1, y1 in boxes)


def test_the_world_map_legend_is_copied_into_ten_row_lines(disc):
    page, out = _legend_pages(disc)
    assert len(out) == len(page)
    assert _legend_texel(page, 19, 236) == 0xA and _legend_texel(page, 19, 244) == 0xF
    mx, my = button_art.LEGEND_MOVED_TO
    rows = button_art.LEGEND_LINE_ROWS
    lines = button_art.LEGEND_LINE_COUNT
    drawn = _legend_drawn()
    for line in range(lines):
        for dy in range(rows):
            for dx in range(96):
                x, y = mx + dx, my + line * rows + dy
                if (x, y) in drawn or _in_boxes(x, y, button_art.LEGEND_CLEAR):
                    continue
                want = _legend_texel(page, dx, 224 + line * 8 + dy - 1) if 1 <= dy <= 8 else 0
                assert _legend_texel(out, x, y) == want, (x, y)
    for x0, y0, rows_ in button_art.LEGEND_EDITS:
        for dy, row in enumerate(rows_):
            for dx, digit in enumerate(row):
                if digit != ".":
                    assert _legend_texel(out, x0 + dx, y0 + dy) == int(digit, 16), (x0 + dx, y0 + dy)
    for y in range(256):
        for x in range(1280):
            if not (mx <= x < mx + 96 and my <= y < my + lines * rows):
                assert _legend_texel(out, x, y) == _legend_texel(page, x, y), (x, y)


def test_the_moved_legend_lands_on_empty_texels(disc):
    page, _ = _legend_pages(disc)
    mx, my = button_art.LEGEND_MOVED_TO
    assert my + button_art.LEGEND_LINE_COUNT * button_art.LEGEND_LINE_ROWS <= 256 and mx + 96 <= 256
    assert all(_legend_texel(page, x, y) == 0 for x in range(mx, mx + 96)
               for y in range(my, my + button_art.LEGEND_LINE_COUNT * button_art.LEGEND_LINE_ROWS))


def test_wmap_draws_the_legend_from_the_moved_lines(disc):
    image, table = disc
    wmap = bytes(decompress(disc_image.read_resource(image, table[button_art.WMAP_RESOURCE])[1:]))
    out = button_art.patch_overlay(button_art.WMAP_RESOURCE, wmap)
    assert len(out) == len(wmap)
    for line in range(4):
        at = button_art.LEGEND_GLYPHS + line * 8
        assert (wmap[at], (wmap[at + 2] - 0x20) & 0xFF, struct.unpack_from("<HH", wmap, at + 4)) == (0, 224 + line * 8, (96, 8))
        assert (out[at], (out[at + 2] - 0x20) & 0xFF, struct.unpack_from("<HH", out, at + 4)) == (128, 216 + line * 10, (96, 10))
    changed = {i for i in range(len(wmap)) if wmap[i] != out[i]}
    allowed = {button_art.LEGEND_GLYPHS + line * 8 + k for line in range(4) for k in (0, 2, 6)}
    allowed |= {button_art.LEGEND_CHARS + char * 6 + 4 for char in range(8)}
    assert changed <= allowed
    for char in range(8):
        at = button_art.LEGEND_CHARS + char * 6 + 4
        assert out[at] == wmap[at] - 1 and wmap[at] in (190, 200, 210)


def test_the_world_map_legend_palette_gets_the_x_and_y_colours(disc):
    image, table = disc
    page = bytes(decompress(disc_image.read_resource(image, table[button_art.LEGEND_PALETTE_PAGE])[1:]))
    out = button_art.patch_raw(button_art.LEGEND_PALETTE_PAGE, page)
    changed = {i for i in range(len(page)) if page[i] != out[i]}
    allowed = set()
    for entry, old, new in button_art.LEGEND_PALETTE_EDITS:
        at = 20 + ((508 - 256) * 320 + 736 - 704 + entry) * 2
        assert struct.unpack_from("<H", page, at)[0] == old and struct.unpack_from("<H", out, at)[0] == new
        allowed |= {at, at + 1}
    assert changed and changed <= allowed


def test_the_legend_shades_are_used_only_by_the_new_discs(disc):
    _, out = _legend_pages(disc)
    mx, my = button_art.LEGEND_MOVED_TO
    found = {(x, y) for y in range(my, my + 40) for x in range(mx, mx + 96) if _legend_texel(out, x, y) in (0xC, 0xD)}
    assert found and all(145 <= x < 155 and 226 <= y < 246 for x, y in found)


def test_nothing_of_the_ps1_legend_is_left_beside_the_colons(disc):
    page, out = _legend_pages(disc)
    drawn = _legend_drawn()
    for y in range(216, 246):
        for x in range(131, 156):
            if (x, y) not in drawn and not (x == 155 and y != 218):
                assert _legend_texel(out, x, y) == 0, (x, y)
    assert _legend_texel(out, 156, 216 + 4) == _legend_texel(page, 28, 227) != 0


def test_the_artifact_prompt_button_has_no_ps1_rim_left(disc):
    image, table = disc
    raw = disc_image.read_resource(image, table[0x1152])
    clut, pixels = _page(button_art.patch_raw(0x1152, raw))
    x0, y0, letter, w, h = button_art.HELP_GLYPHS[0x1152][0]
    rows = button_art.button_icons.ICONS[(letter, w, h)][1]
    cx, cy, r = x0 + w / 2, y0 + h / 2, w / 2
    for y in range(y0 - 3, y0 + h + 3):
        for x in range(x0, x0 + w + 3):
            drawn = 0 <= x - x0 < w and 0 <= y - y0 < h and rows[y - y0][x - x0] != "0"
            if not drawn and r <= ((x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2) ** 0.5 <= r + button_art.HELP_RIM_WIDTH:
                red, green, blue = _rgb(clut[pixels[y * 256 + x]])
                assert red > green + 2 and red > blue + 2, (x, y)


@pytest.mark.parametrize("x,y,outline", [(112, 148, "1"), (128, 148, "1"), (144, 148, "1"), (32, 164, "1"), (145, 226, "e"), (145, 236, "e")])
def test_button_outlines_have_no_corner_pixels(x, y, outline):
    edits = {(x0, y0): rows for x0, y0, rows in button_art.MENU_EDITS + button_art.LEGEND_EDITS}
    rows = edits[(x, y)]
    at = lambda c, r: 0 <= r < len(rows) and 0 <= c < len(rows[r]) and rows[r][c] == outline
    for r, row in enumerate(rows):
        for c, ch in enumerate(row):
            if ch == outline:
                assert not ((at(c - 1, r) or at(c + 1, r)) and (at(c, r - 1) or at(c, r + 1))), (x, y, c, r)
