#----------------------
# | overlay_rules.py
# | Description: Per-overlay rules that pack an overlay's data ranges into the Saturn LWRAM slot or a Dreamcast arena region, convert them to big-endian for the Saturn and emit linker symbols.
# | Author: suinevere
# | Dependencies: config/us/symbols/<overlay>_symbol_addrs.txt, wmap_memory.py, targets.py
# | Globals: OVERLAYS, SLOT_BASE, FUNCTION_TABLES, AUTO_FUNCTION_TABLES, C_DEFINED, WSEL_TIMS, POINTER_RULES
# | Params: N/A
# | Returns: N/A
#----------------------
import collections
import functools
import struct

import targets
import wmap_memory

#----------------------
# | SLOT_BASE
# | Description: Low Work RAM address of the shared overlay slot; must match saturn.linker and overlay_loader.h.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SLOT_BASE = 0x00210000

#----------------------
# | SECONDARY_BASE
# | Description: Offset of the secondary overlay slot within SLOT_BASE, for an overlay that shares the slot with another.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECONDARY_BASE = 0x70000

#----------------------
# | OverlaySpec
# | Description: One overlay: name, PS1 load address, packed data ranges, a symbol file name or tuple of names, TIM symbols, a convert(buf, off) rule, a slot base offset and a tail start address dropped from the disc blob, symbols placed at fixed Saturn addresses outside the packed ranges, and the ranges before those cuts.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OverlaySpec = collections.namedtuple("OverlaySpec", "name vram ranges symbols_file tims convert base tail_from placed unsplit_ranges")
OverlaySpec.__new__.__defaults__ = (0, None, (), None)

#----------------------
# | FUNCTION_TABLES
# | Description: Per overlay name, the function-pointer tables (name, entry count) defined in generated C from their PS1 entries instead of linked as raw overlay data.
# |              g_field_object_flag_handlers mixes built-in animation ids below TABLE_LITERAL_LIMIT with handler pointers, so detection skips it and it is listed here.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FUNCTION_TABLES = {
    "field": (("g_field_script_op_table", 64), ("g_field_script_pair_op_table", 32), ("g_field_script_ext_op_table", 16),
              ("g_field_object_flag_handlers", 16)),
    "wmap": (("D_800D7314", 6), ("D_800D732C", 22), ("D_800D7384", 4), ("D_800D7394", 4), ("D_800D73A4", 6),
             ("D_800D73BC", 6), ("D_800D73D4", 6), ("D_800D73EC", 6), ("D_800D7404", 6), ("D_800D741C", 6),
             ("D_800D7434", 6), ("D_800D744C", 4), ("D_800D745C", 6), ("g_wmap_effect_resource_loaders", 36),
             ("D_800D6028", 8), ("D_800D6048", 18), ("D_800D6090", 4), ("D_800D60A0", 6), ("D_800D60B8", 4),
             ("D_800D60C8", 4), ("D_800D60D8", 4), ("D_800D60E8", 6), ("D_800D6100", 8), ("D_800D6120", 6),
             ("D_800D6138", 6), ("g_wmap_view_sequence_steps", 2), ("g_wmap_land_focus_steps", 10),
             ("g_wmap_land_entry_steps", 4), ("g_wmap_land_return_steps", 6), ("g_wmap_special_travel_steps", 20),
             ("g_wmap_special_return_steps", 14), ("g_wmap_land_effect_13_sequence_9_steps", 6)),
}


#----------------------
# | C_DEFINED
# | Description: Per overlay name, data symbols a Saturn C file defines instead of the packed image, because their PS1 value points into main memory the converter cannot place.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
C_DEFINED = {
    "wmap": ("g_wmap_controller_ports",),
}


#----------------------
# | ps1_to_saturn_color
# | Description: Converts one PS1 15-bit colour to a VDP1 RGB555 pixel, keeping 0 as transparent.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- little-endian PS1 colour word
# | Returns: Saturn colour word with bit 15 set unless transparent
#----------------------
def ps1_to_saturn_color(value):
    if value == 0:
        return 0
    return (value & 0x7FFF) | 0x8000


#----------------------
# | swap_words
# | Description: Rewrites count little-endian words of the given size as big-endian, in place.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: buf -- bytearray, offset -- start byte, count -- word count, size -- 2 or 4
# | Returns: N/A
#----------------------
def swap_words(buf, offset, count, size):
    fmt = "H" if size == 2 else "I"
    for i in range(count):
        pos = offset + i * size
        (value,) = struct.unpack_from("<" + fmt, buf, pos)
        struct.pack_into(">" + fmt, buf, pos, value)


#----------------------
# | swap_nibbles
# | Description: Swaps the two 4bpp pixels in each of count bytes, in place, so PS1 low-nibble-first pixels read high-nibble-first on the Saturn.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: buf -- bytearray, offset -- start byte, count -- byte count
# | Returns: N/A
#----------------------
def swap_nibbles(buf, offset, count):
    for pos in range(offset, offset + count):
        b = buf[pos]
        buf[pos] = ((b & 0x0F) << 4) | (b >> 4)


#----------------------
# | convert_colors
# | Description: Converts count PS1 colour words to big-endian Saturn RGB555, in place.
# | Author: suinevere
# | Dependencies: ps1_to_saturn_color
# | Globals: N/A
# | Params: buf -- bytearray, offset -- start byte, count -- colour count
# | Returns: N/A
#----------------------
def convert_colors(buf, offset, count):
    for i in range(count):
        pos = offset + i * 2
        (value,) = struct.unpack_from("<H", buf, pos)
        struct.pack_into(">H", buf, pos, ps1_to_saturn_color(value))


#----------------------
# | convert_block_header
# | Description: Swaps a TIM block header (bnum, x, y, w, h) to big-endian and returns its fields.
# | Author: suinevere
# | Dependencies: swap_words
# | Globals: N/A
# | Params: buf -- bytearray, offset -- block start
# | Returns: (bnum, w, h) as read before swapping
#----------------------
def convert_block_header(buf, offset):
    bnum, _x, _y, w, h = struct.unpack_from("<IHHHH", buf, offset)
    swap_words(buf, offset, 1, 4)
    swap_words(buf, offset + 4, 4, 2)
    return bnum, w, h


#----------------------
# | convert_tim
# | Description: Converts a PS1 TIM in place; the 8-byte header stays raw because game code reads its flag byte at +4.
# | Author: suinevere
# | Dependencies: convert_block_header, convert_colors
# | Globals: N/A
# | Params: buf -- bytearray, offset -- TIM start
# | Returns: total TIM size in bytes
#----------------------
def convert_tim(buf, offset):
    flags = buf[offset + 4]
    pos = offset + 8
    if flags & 8:
        bnum, w, h = convert_block_header(buf, pos)
        convert_colors(buf, pos + 12, w * h)
        pos += bnum
    bnum, w, h = convert_block_header(buf, pos)
    data = pos + 12
    mode = flags & 3
    if mode == 0:
        for i in range(w * h * 2):
            b = buf[data + i]
            buf[data + i] = ((b & 0x0F) << 4) | (b >> 4)
    elif mode == 2:
        convert_colors(buf, data, w * h)
    return pos + bnum - offset


#----------------------
# | packed_offset
# | Description: Maps a PS1 address inside one of an overlay's ranges to its offset in the packed slot image.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: addr -- PS1 address, spec -- OverlaySpec
# | Returns: packed offset, or None when the address is outside every range
#----------------------
def packed_offset(addr, spec):
    base = 0
    for start, end in spec.ranges:
        if start <= addr < end:
            return base + addr - start
        base += end - start
    return None


#----------------------
# | split_ranges
# | Description: Removes each span from the ranges it falls in, so a placed buffer takes no space in the packed slot image.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: ranges -- list of (start, end), spans -- list of (start, end) to remove
# | Returns: list of (start, end) in address order, empty pieces dropped
#----------------------
def split_ranges(ranges, spans):
    out = []
    for start, end in ranges:
        cursor = start
        for s0, s1 in sorted(spans):
            if s1 <= cursor or s0 >= end:
                continue
            if s0 > cursor:
                out.append((cursor, s0))
            cursor = max(cursor, s1)
        if cursor < end:
            out.append((cursor, end))
    return out


#----------------------
# | symbol_offset
# | Description: Packed offset of a named overlay symbol; build_overlay binds it to one overlay and passes it to the convert rules as off.
# | Author: suinevere
# | Dependencies: packed_offset
# | Globals: N/A
# | Params: symbols -- name to PS1 address, spec -- OverlaySpec, name -- symbol name
# | Returns: packed offset, or None when the symbol is outside every range
#----------------------
def symbol_offset(symbols, spec, name):
    return packed_offset(symbols[name], spec)


#----------------------
# | data_base
# | Description: Address the target loads an overlay's packed data blob at: the Saturn slot plus the overlay's slot offset, or its Dreamcast region's data base.
# | Author: suinevere
# | Dependencies: targets.Target
# | Globals: SLOT_BASE
# | Params: spec -- OverlaySpec, target -- targets.Target
# | Returns: address; raises ValueError for an overlay the target gives no region
#----------------------
def data_base(spec, target):
    if target.regions is None:
        return SLOT_BASE + spec.base
    if spec.name not in target.regions:
        raise ValueError("overlay %s has no %s region" % (spec.name, target.name))
    return target.regions[spec.name].data_base


#----------------------
# | relocate_pointer
# | Description: Maps a PS1 pointer into the overlay's packed ranges onto the target's address for it; other values pass through.
# | Author: suinevere
# | Dependencies: packed_offset, data_base, targets.SATURN
# | Globals: N/A
# | Params: value -- PS1 pointer value, spec -- OverlaySpec, target -- targets.Target, Saturn by default
# | Returns: relocated pointer value
#----------------------
def relocate_pointer(value, spec, target=targets.SATURN):
    offset = packed_offset(value, spec)
    return value if offset is None else data_base(spec, target) + offset


#----------------------
# | relocate_words
# | Description: Relocates the little-endian PS1 pointer words at the given packed offsets in place, writing them back little-endian for a target that keeps PS1 byte order.
# | Author: suinevere
# | Dependencies: relocate_pointer
# | Globals: N/A
# | Params: buf -- packed bytearray, positions -- packed offsets of pointer words, spec -- OverlaySpec, target -- targets.Target
# | Returns: N/A
#----------------------
def relocate_words(buf, positions, spec, target):
    for pos in positions:
        (src,) = struct.unpack_from("<I", buf, pos)
        struct.pack_into("<I", buf, pos, relocate_pointer(src, spec, target))


#----------------------
# | TITLE_SAVE_LAYOUT_TEX_COUNT / TITLE_SAVE_LAYOUT_TEX_STRIDE
# | Description: Entry count and stride of g_save_layout_tex_table, whose word at +8 points at a save-layout TIM inside TITLE's packed range.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TITLE_SAVE_LAYOUT_TEX_COUNT = 11
TITLE_SAVE_LAYOUT_TEX_STRIDE = 0x10


#----------------------
# | convert_title
# | Description: TITLE rules: menu TIM table, save-layout texture and layout tables, saved-game templates and state words.
# | Author: suinevere
# | Dependencies: swap_words, relocate_pointer
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset
# | Returns: N/A
#----------------------
def convert_title(buf, off):
    swap_words(buf, off("g_title_menu_tim_table"), 3, 4)
    table = off("g_save_layout_tex_table")
    for i in range(TITLE_SAVE_LAYOUT_TEX_COUNT):
        entry = table + i * TITLE_SAVE_LAYOUT_TEX_STRIDE
        swap_words(buf, entry, 4, 2)
        (src,) = struct.unpack_from("<I", buf, entry + 8)
        struct.pack_into(">I", buf, entry + 8, relocate_pointer(src, OVERLAYS[4]))
        swap_words(buf, entry + 12, 1, 4)
    layout = off("g_save_layout_table")
    for i in range(27):
        entry = layout + i * 0x18
        buf[entry + 3] = buf[entry]
        swap_words(buf, entry + 4, 8, 2)
        swap_words(buf, entry + 20, 1, 4)
    for name, words in (("g_new_game_template", 0xC9A), ("g_field_start_template", 0xC9A),
                        ("g_hero_template_type_0", 0x94), ("g_hero_template_type_1", 0x94)):
        swap_words(buf, off(name), words, 4)
    swap_words(buf, off("g_title_menu_exit_state"), (0x80102670 - 0x80102640) // 4, 4)
    swap_words(buf, off("g_last_input_state"), (0x801026CC - 0x80102694) // 4, 4)


#----------------------
# | relocate_title_pointers
# | Description: TITLE pointer rule for a little-endian target: relocates each save-layout texture entry's TIM pointer and leaves every other byte raw.
# | Author: suinevere
# | Dependencies: relocate_words, OVERLAYS, TITLE_SAVE_LAYOUT_TEX_COUNT, TITLE_SAVE_LAYOUT_TEX_STRIDE
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset, target -- targets.Target
# | Returns: N/A
#----------------------
def relocate_title_pointers(buf, off, target):
    table = off("g_save_layout_tex_table")
    positions = [table + i * TITLE_SAVE_LAYOUT_TEX_STRIDE + 8 for i in range(TITLE_SAVE_LAYOUT_TEX_COUNT)]
    relocate_words(buf, positions, OVERLAYS[4], target)


#----------------------
# | convert_checkps
# | Description: CHECKPS rules for the US path: the state words and the glyph CLUT prefix; tables only read bytewise stay raw.
# | Author: suinevere
# | Dependencies: swap_words, convert_colors
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset
# | Returns: N/A
#----------------------
def convert_checkps(buf, off):
    swap_words(buf, off("g_checkps_state"), 1, 4)
    swap_words(buf, off("g_cd_irq_code_sum"), 1, 4)
    convert_colors(buf, off("g_glyph_clut_prefix"), 6)


#----------------------
# | convert_field
# | Description: FIELD rules: the text window layout rectangles (u16); the other tables stay raw until Home's path reads them.
# | Author: suinevere
# | Dependencies: swap_words
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset
# | Returns: N/A
#----------------------
def convert_field(buf, off):
    swap_words(buf, off("g_field_text_window_layouts"), 0x200 // 2, 2)


#----------------------
# | repack_cursor_bits
# | Description: Moves a TabCursorEntry's x:9 and sprite_idx:7 bitfields from the PS1 LSB-first layout to the SH-2 MSB-first layout.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- little-endian u16 holding x in bits 0-8 and sprite_idx in bits 9-15
# | Returns: u16 with x in bits 7-15 and sprite_idx in bits 0-6
#----------------------
def repack_cursor_bits(value):
    return ((value & 0x1FF) << 7) | (value >> 9)


#----------------------
# | to_s16
# | Description: Reinterprets a 16-bit value as signed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- 0 to 0xFFFF
# | Returns: -0x8000 to 0x7FFF
#----------------------
def to_s16(value):
    return value - 0x10000 if value & 0x8000 else value


#----------------------
# | convert_gname
# | Description: GNAME rules: offset words, glyph CLUT words, tab cursor bitfields, the name-data header with its u16 offset arrays, and the layout sprite sequence.
# | Author: suinevere
# | Dependencies: swap_words, repack_cursor_bits, to_s16
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset
# | Returns: N/A
#----------------------
def convert_gname(buf, off):
    swap_words(buf, off("g_panel_char_offsets"), 5, 4)
    swap_words(buf, off("g_kanji_cat_entries"), 10, 4)
    glyphs = off("g_glyph_table")
    for i in range(39):
        swap_words(buf, glyphs + i * 8 + 4, 1, 4)
    cursor = off("g_tab_cursor_pos")
    for i in range(13):
        pos = cursor + i * 4
        (value,) = struct.unpack_from("<H", buf, pos)
        struct.pack_into(">H", buf, pos, repack_cursor_bits(value))
    swap_words(buf, off("g_kanji_entry_offsets"), 45, 4)
    base = off("g_panel_data_base")
    header = struct.unpack_from("<5I", buf, base)
    swap_words(buf, base, 5, 4)
    (history_first,) = struct.unpack_from("<H", buf, base + header[3])
    for table, count in ((header[1], 138), (header[2], 2698), (header[3], history_first // 2), (header[4], 256)):
        swap_words(buf, base + table, count, 2)
    sequence = off("g_layout_sprite_sequence")
    for i in range(20):
        pos = sequence + i * 8
        swap_words(buf, pos, 1, 4)
        (value,) = struct.unpack_from("<I", buf, pos + 4)
        struct.pack_into(">hh", buf, pos + 4, to_s16(value & 0xFFFF), to_s16(value >> 16))


#----------------------
# | convert_poly_ft4
# | Description: Swaps one POLY_FT4's tag, vertex coordinates, clut and tpage words; the single-byte rgb/code/uv fields and pad halfwords are left as-is.
# | Author: suinevere
# | Dependencies: swap_words
# | Globals: N/A
# | Params: buf -- bytearray, offset -- POLY_FT4 start
# | Returns: N/A
#----------------------
def convert_poly_ft4(buf, offset):
    swap_words(buf, offset, 1, 4)
    swap_words(buf, offset + 8, 2, 2)
    swap_words(buf, offset + 14, 1, 2)
    swap_words(buf, offset + 16, 2, 2)
    swap_words(buf, offset + 22, 1, 2)
    swap_words(buf, offset + 24, 2, 2)
    swap_words(buf, offset + 32, 2, 2)


#----------------------
# | convert_poly_g4
# | Description: Swaps one POLY_G4's tag and four vertex coordinate pairs; the single-byte rgb/code/pad fields are left as-is.
# | Author: suinevere
# | Dependencies: swap_words
# | Globals: N/A
# | Params: buf -- bytearray, offset -- POLY_G4 start
# | Returns: N/A
#----------------------
def convert_poly_g4(buf, offset):
    swap_words(buf, offset, 1, 4)
    swap_words(buf, offset + 8, 2, 2)
    swap_words(buf, offset + 16, 2, 2)
    swap_words(buf, offset + 24, 2, 2)
    swap_words(buf, offset + 32, 2, 2)


#----------------------
# | convert_sprt
# | Description: Swaps one SPRT's tag, vertex coordinates, clut and size words; the single-byte rgb/code/uv fields are left as-is.
# | Author: suinevere
# | Dependencies: swap_words
# | Globals: N/A
# | Params: buf -- bytearray, offset -- SPRT start
# | Returns: N/A
#----------------------
def convert_sprt(buf, offset):
    swap_words(buf, offset, 1, 4)
    swap_words(buf, offset + 8, 2, 2)
    swap_words(buf, offset + 14, 1, 2)
    swap_words(buf, offset + 16, 2, 2)


#----------------------
# | convert_menu_triangle
# | Description: Swaps one WmapMenuTriangle's tag and three vertex coordinate pairs; the single-byte rgb/code/pad fields are left as-is.
# | Author: suinevere
# | Dependencies: swap_words
# | Globals: N/A
# | Params: buf -- bytearray, offset -- WmapMenuTriangle start
# | Returns: N/A
#----------------------
def convert_menu_triangle(buf, offset):
    swap_words(buf, offset, 1, 4)
    swap_words(buf, offset + 8, 2, 2)
    swap_words(buf, offset + 16, 2, 2)
    swap_words(buf, offset + 24, 2, 2)


#----------------------
# | WSEL_TIMS
# | Description: WSEL's eight embedded TIMs in sprite-layer order.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WSEL_TIMS = ["g_wsel_land_map_tim", "g_wsel_world_map_tim", "g_wsel_cursor_tim", "g_wsel_world_overlay_tim",
             "g_wsel_hero_tim_default", "g_wsel_hero_tim_alternate", "g_wsel_hero_shadow_tim", "g_wsel_prompt_tim"]


#----------------------
# | convert_wsel
# | Description: Swaps the halfwords of WSEL's eight sprite records and each TIM's flags word, which wsel_upload_tim reads as a u32 value, not a byte.
# | Author: suinevere
# | Dependencies: swap_words, WSEL_TIMS
# | Globals: N/A
# | Params: buf -- overlay data, off -- symbol name to buffer offset
# | Returns: N/A
#----------------------
def convert_wsel(buf, off):
    sprites = off("g_wsel_sprites")
    for i in range(8):
        swap_words(buf, sprites + i * 24 + 4, 10, 2)
    for name in WSEL_TIMS:
        swap_words(buf, off(name) + 4, 1, 4)


#----------------------
# | WMAP_SPIRIT_CELL_POSITIONS / WMAP_SPIRIT_CELL_POSITION_COUNT
# | Description: Packed offset and entry count of D_8004FC74, the (x, y) screen positions wmap_map_display.c copies as one word into each cell's spirit sprite, two 6x6 grids.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WMAP_SPIRIT_CELL_POSITIONS = 0x000
WMAP_SPIRIT_CELL_POSITION_COUNT = 72

#----------------------
# | WMAP_SPIRIT_ROW_SPRITES / WMAP_SPIRIT_ROW_SPRITE_COUNT / WMAP_SPIRIT_ROW_SPRITE_STRIDE
# | Description: Packed offset, count and stride of D_8004FD9C, the spirit row's sprite records whose leading (x, y) word wmap_map_display.c copies into each row sprite.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WMAP_SPIRIT_ROW_SPRITES = 0x128
WMAP_SPIRIT_ROW_SPRITE_COUNT = 8
WMAP_SPIRIT_ROW_SPRITE_STRIDE = 20


#----------------------
# | convert_wmap_spirit_positions
# | Description: Swaps the spirit sprites' (x, y) halfword pairs that WMAP copies as whole words into SPRT x0/y0, so each half lands big-endian in its field.
# | Author: suinevere
# | Dependencies: swap_words, WMAP_SPIRIT_CELL_POSITIONS, WMAP_SPIRIT_ROW_SPRITES
# | Globals: N/A
# | Params: buf -- packed WMAP bytearray (D_8004FC74 and D_8004FD9C sit at fixed offsets because the first packed range starts at 0x8004FC74)
# | Returns: N/A
#----------------------
def convert_wmap_spirit_positions(buf):
    swap_words(buf, WMAP_SPIRIT_CELL_POSITIONS, WMAP_SPIRIT_CELL_POSITION_COUNT * 2, 2)
    for i in range(WMAP_SPIRIT_ROW_SPRITE_COUNT):
        swap_words(buf, WMAP_SPIRIT_ROW_SPRITES + i * WMAP_SPIRIT_ROW_SPRITE_STRIDE, 2, 2)


#----------------------
# | WMAP_LABEL_TABLES
# | Description: PS1 addresses of the map label tables wmap_map_labels.c reads: shade triangles, label positions, label sprite templates, glyph runs, scales, widths, palette row, glyph rectangles and placed glyphs.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WMAP_LABEL_SHADE_TRIANGLES = 0x800515F4
WMAP_LABEL_POSITIONS = 0x800519E4
WMAP_LABEL_SPRITES = (0x80051A08, 0x80051A1C, 0x80051A30, 0x80051A44, 0x80051A58)
WMAP_LABEL_GLYPH_RUNS = 0x80051A6C
WMAP_LABEL_SCALES = 0x800D0230
WMAP_LABEL_WIDTHS = 0x800D0254
WMAP_LABEL_PALETTE_ROW = 0x800D0368
WMAP_LABEL_GLYPHS = 0x800D03EC
WMAP_LABEL_CHARS = 0x800D040C


#----------------------
# | convert_wmap_map_labels
# | Description: Swaps the map label tables' multi-byte fields to big-endian; colour, code and single-byte fields stay as they are.
# | Author: suinevere
# | Dependencies: packed_offset, swap_words, convert_sprt, WMAP_LABEL_* constants, OVERLAYS
# | Globals: N/A
# | Params: buf -- packed WMAP bytearray
# | Returns: N/A
#----------------------
def convert_wmap_map_labels(buf):
    spec = OVERLAYS[3]
    at = lambda addr: packed_offset(addr, spec)
    for i in range(36):
        base = at(WMAP_LABEL_SHADE_TRIANGLES) + i * 28
        swap_words(buf, base, 1, 4)
        for vertex in (8, 16, 24):
            swap_words(buf, base + vertex, 2, 2)
    swap_words(buf, at(WMAP_LABEL_POSITIONS), 18, 2)
    for addr in WMAP_LABEL_SPRITES:
        convert_sprt(buf, at(addr))
    swap_words(buf, at(WMAP_LABEL_GLYPH_RUNS), 6, 4)
    swap_words(buf, at(WMAP_LABEL_SCALES), 9, 4)
    swap_words(buf, at(WMAP_LABEL_WIDTHS), 69, 4)
    swap_words(buf, at(WMAP_LABEL_PALETTE_ROW), 1, 4)
    for i in range(4):
        swap_words(buf, at(WMAP_LABEL_GLYPHS) + i * 8 + 4, 2, 2)
    for i in range(8):
        swap_words(buf, at(WMAP_LABEL_CHARS) + i * 6 + 2, 1, 2)


#----------------------
# | convert_wmap
# | Description: WMAP rules for the Home path: backdrop/menu/spirit GPU primitives, the input-script pointer table and its scripts, word and halfword
# |              tables (scale steps, cell-effect steps, information layout, route headings), the texture-info, cell-effect-quad, glyph, glyph-placement,
# |              preview-texture-cell and artifact-transfer-frame tables, the DVECTOR position tables, the land-attribute flags and spirit-sprite and
# |              land-count-tier lookups the artifact-placement path reads, the cursor-limit and map-grid tables wmap_update_map_view/wmap_project_map_grid
# |              read every frame, the artifact carousel's mesh, slot, image and rotation tables (visible from WMAP entry, since the carousel starts
# |              VISIBLE and a new game's first unplaced artifact is Home), and the party sprites' palette and texture-page words.
# | Author: suinevere
# | Dependencies: swap_words, relocate_pointer, convert_poly_ft4, convert_poly_g4, convert_sprt, convert_menu_triangle, convert_wmap_spirit_positions, convert_wmap_map_labels
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset
# | Returns: N/A
#----------------------
def convert_wmap(buf, off):
    for name in ("g_wmap_backdrop_front_quads", "g_wmap_backdrop_back_quads"):
        base = off(name)
        for i in range(4):
            convert_poly_ft4(buf, base + i * 0x28)
    for name in ("g_wmap_screen_fade_quad", "g_wmap_loading_quad", "g_wmap_preview_quad_template"):
        convert_poly_ft4(buf, off(name))
    convert_poly_g4(buf, off("g_wmap_backdrop_gradient_template"))
    convert_sprt(buf, off("g_wmap_prompt_sprite_template"))
    triangles = off("g_wmap_menu_triangles")
    for i in range(28):
        convert_menu_triangle(buf, triangles + i * 0x1C)
    level = off("g_wmap_spirit_level_sprites")
    accent = off("g_wmap_spirit_accent_sprites")
    for i in range(16):
        convert_sprt(buf, level + i * 0x14)
        convert_sprt(buf, accent + i * 0x14)

    scripts = off("g_wmap_input_scripts")
    for i in range(3):
        pos = scripts + i * 4
        (src,) = struct.unpack_from("<I", buf, pos)
        struct.pack_into(">I", buf, pos, relocate_pointer(src, OVERLAYS[3]))
    for name, count in (("D_800D0460", 50), ("D_800D04C4", 26), ("D_800D04F8", 38)):
        swap_words(buf, off(name), count, 2)

    swap_words(buf, off("g_wmap_preview_travel_frame_bounds"), 10, 4)
    swap_words(buf, off("g_wmap_land_scale_steps"), 5, 4)
    swap_words(buf, off("g_wmap_cell_effect_steps"), 42, 4)
    swap_words(buf, off("g_wmap_information_group_starts"), 11, 4)
    swap_words(buf, off("g_wmap_artifact_return_offsets"), 65, 4)
    swap_words(buf, off("D_800D036C"), 64, 2)
    swap_words(buf, off("g_wmap_route_headings"), 10, 2)
    convert_wmap_spirit_positions(buf)
    convert_wmap_map_labels(buf)

    textures = off("D_800CBBE8")
    for i in range(17):
        entry = textures + i * 0x1C
        swap_words(buf, entry, 1, 2)
        swap_words(buf, entry + 4, 1, 2)
        swap_words(buf, entry + 6, 1, 2)
        swap_words(buf, entry + 8, 8, 2)
        swap_words(buf, entry + 24, 2, 2)

    quads = off("g_wmap_cell_effect_quads")
    for i in range(16):
        entry = quads + i * 0x30
        for v in range(4):
            swap_words(buf, entry + v * 8, 4, 2)

    glyphs = off("g_wmap_information_glyphs")
    for i in range(28):
        swap_words(buf, glyphs + i * 8 + 4, 1, 2)
    placements = off("g_wmap_information_placements")
    for i in range(313):
        swap_words(buf, placements + i * 4, 1, 2)

    cells = off("g_wmap_preview_textures")
    for i in range(6):
        group = cells + i * 8
        swap_words(buf, group, 1, 2)
        swap_words(buf, group + 4, 1, 2)
        swap_words(buf, group + 6, 1, 2)

    swap_words(buf, off("g_wmap_preview_map_positions"), 18, 2)
    swap_words(buf, off("g_wmap_empty_marker_positions"), 18, 2)

    frames = off("g_wmap_preview_travel_frames")
    for i in range(126):
        swap_words(buf, frames + i * 8, 2, 2)

    swap_words(buf, off("g_wmap_artifact_pickup_frames"), 740 * 6, 2)
    swap_words(buf, off("g_wmap_artifact_return_frames"), 280 * 6, 2)

    attributes = off("g_wmap_land_attributes")
    for i in range(64):
        swap_words(buf, attributes + i * 12 + 8, 1, 4)
    swap_words(buf, off("g_wmap_spirit_sprites"), 57, 4)
    swap_words(buf, off("g_wmap_land_count_tiers"), 64, 4)

    swap_words(buf, off("g_wmap_cell_focus_offsets"), 58 * 2, 2)
    swap_words(buf, off("D_800CCBF4"), 65, 4)
    swap_words(buf, off("g_wmap_carousel_textures"), 1176, 2)
    swap_words(buf, off("g_wmap_carousel_vertices"), 97 * 4, 2)
    swap_words(buf, off("g_wmap_carousel_faces"), 504, 2)
    swap_words(buf, off("g_wmap_carousel_translation"), 4, 4)
    swap_words(buf, off("g_wmap_carousel_angles"), 82 * 2, 2)
    swap_words(buf, off("g_wmap_artifact_positions"), 12 * 2, 2)
    images = off("g_wmap_artifact_images")
    for i in range(64):
        swap_words(buf, images + i * 0x18 + 4, 10, 2)

    sprites = off("g_wmap_sprite_textures")
    for i in range(48):
        swap_words(buf, sprites + i * 0x1C + 8, 9, 2)

    swap_words(buf, off("g_wmap_cursor_limits"), 4, 4)
    swap_words(buf, off("g_wmap_grid_vertices"), 729 * 4, 2)
    swap_words(buf, off("g_wmap_grid_vertex_indices"), 2704, 2)


#----------------------
# | convert_menu
# | Description: MENU has no big-endian rules yet; Saturn skips it through targets.SATURN_UNLINKED_OVERLAYS, so reaching this is a bug.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset
# | Returns: N/A; always raises ValueError
#----------------------
def convert_menu(buf, off):
    raise ValueError("MENU has no Saturn data rules")


#----------------------
# | convert_dreamcast_only
# | Description: Convert rule for a secondary overlay only the Dreamcast links; Saturn skips it through targets.SATURN_UNLINKED_OVERLAYS, so reaching this is a bug.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset
# | Returns: N/A; always raises ValueError
#----------------------
def convert_dreamcast_only(buf, off):
    raise ValueError("this overlay has no Saturn data rules")


#----------------------
# | OVERLAYS
# | Description: Overlays the converter packs, keyed by CD resource index; WMAP's large buffers are cut out of its ranges and placed by wmap_memory. Secondaries from MENU on are Dreamcast-only; MENU, ZUKAN, GOSUB and CARDA also pack the tables and strings that sit in rodata bins before their code.
# | Author: suinevere
# | Dependencies: convert_title, convert_checkps, convert_field, convert_gname, convert_wsel, convert_wmap, convert_menu, WSEL_TIMS, split_ranges, wmap_memory.place, wmap_memory.BUFFERS
# | Globals: SECONDARY_BASE
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAYS = {
    2: OverlaySpec("field", 0x8004FC70, [(0x800CBF44, 0x80123FD8)], ("field_symbol_addrs.txt", "shared_symbol_addrs.txt"),
                   [], convert_field, 0, 0x800F2268),
    4: OverlaySpec("title", 0x8004FC70, [(0x800522E8, 0x801026CC)], "title_symbol_addrs.txt",
                   ["g_title_menu_tim_0", "g_title_menu_tim_1"] + ["g_save_layout_tim_%02d" % i for i in range(11)],
                   convert_title),
    15: OverlaySpec("checkps", 0x8004FC70, [(0x8004FCC4, 0x8004FD14), (0x80052428, 0x8005D060)], "checkps_symbol_addrs.txt",
                    ["g_checkps_image_asset"], convert_checkps),
    5: OverlaySpec("gname", 0x80140000, [(0x80142C98, 0x8014F7B0)], "gname_symbol_addrs.txt",
                   ["g_name_entry_tim"], convert_gname, SECONDARY_BASE, None),
    6: OverlaySpec("menu", 0x80140000, [(0x8014057C, 0x801405B0), (0x8014FBF4, 0x8016B9DC)], "menu_symbol_addrs.txt",
                   [], convert_menu, SECONDARY_BASE, None),
    7: OverlaySpec("shop", 0x80140000, [(0x80142D04, 0x80145CE4)], "shop_symbol_addrs.txt",
                   [], convert_dreamcast_only, SECONDARY_BASE, None),
    8: OverlaySpec("zukan", 0x80140000, [(0x8014001C, 0x80140E00), (0x8014471C, 0x80157D7C)], "zukan_symbol_addrs.txt",
                   [], convert_dreamcast_only, SECONDARY_BASE, None),
    13: OverlaySpec("gosub", 0x80140000, [(0x80140068, 0x80140080), (0x80147058, 0x8016B8C8)], "gosub_symbol_addrs.txt",
                   [], convert_dreamcast_only, SECONDARY_BASE, None),
    12: OverlaySpec("carda", 0x80140000, [(0x8014008C, 0x80140094), (0x801400C4, 0x801400CC), (0x801401B8, 0x801401C8), (0x80140244, 0x8014024C), (0x8014B038, 0x8016F000)], "carda_symbol_addrs.txt",
                   [], convert_dreamcast_only, SECONDARY_BASE, None),
    16: OverlaySpec("cload", 0x80140000, [(0x80145E9C, 0x8016B258)], "cload_symbol_addrs.txt",
                   [], convert_dreamcast_only, SECONDARY_BASE, None),
    17: OverlaySpec("niki", 0x80140000, [(0x801470F8, 0x8016DA80)], "niki_symbol_addrs.txt",
                   [], convert_dreamcast_only, SECONDARY_BASE, None),
    18: OverlaySpec("addhero", 0x80140000, [(0x80140088, 0x80140098), (0x80140114, 0x8014011C), (0x80146FA4, 0x8016D940)], "addhero_symbol_addrs.txt",
                   [], convert_dreamcast_only, SECONDARY_BASE, None),
    10: OverlaySpec("gover", 0x80140000, [(0x80140704, 0x80140704)], "gover_symbol_addrs.txt",
                   [], convert_dreamcast_only, SECONDARY_BASE, None),
    9: OverlaySpec("golem", 0x80140000, [(0x8014287C, 0x8014C28C)], "golem_symbol_addrs.txt",
                   [], convert_dreamcast_only, SECONDARY_BASE, None),
    14: OverlaySpec("wsel", 0x8004FC70, [(0x80052608, 0x800CA8E4)], "wsel_symbol_addrs.txt",
                    WSEL_TIMS, convert_wsel),
    3: OverlaySpec("wmap", 0x8004FC70,
                   split_ranges([(0x8004FC74, 0x80054A2C), (0x800C4588, 0x801B32D8)],
                                [(b.ps1_addr, b.ps1_addr + b.ps1_gap) for b in wmap_memory.BUFFERS]),
                   ("wmap_symbol_addrs.txt", "shared_symbol_addrs.txt"), [], convert_wmap, 0, 0x801AFBA0,
                   tuple(wmap_memory.place(wmap_memory.BUFFERS).items()),
                   [(0x8004FC74, 0x80054A2C), (0x800C4588, 0x801B32D8)]),
}


#----------------------
# | target_spec
# | Description: The spec as a target packs it: a target without convert rules keeps any cut buffers inside its data at their PS1 offsets and places nothing.
# | Author: suinevere
# | Dependencies: OverlaySpec
# | Globals: N/A
# | Params: spec -- OverlaySpec, target -- targets.Target
# | Returns: OverlaySpec
#----------------------
def target_spec(spec, target):
    if target.convert_rules or spec.unsplit_ranges is None:
        return spec
    return spec._replace(ranges=spec.unsplit_ranges, placed=())


#----------------------
# | relocate_wmap_pointers
# | Description: WMAP pointer rule for a little-endian target: relocates the three input-script pointers, the only packed-range pointers its Home path reads.
# | Author: suinevere
# | Dependencies: relocate_words, target_spec, OVERLAYS
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset, target -- targets.Target
# | Returns: N/A
#----------------------
def relocate_wmap_pointers(buf, off, target):
    scripts = off("g_wmap_input_scripts")
    relocate_words(buf, [scripts + i * 4 for i in range(3)], target_spec(OVERLAYS[3], target), target)


#----------------------
# | MENU_CONTENT_TABLE_WORDS / MENU_SCRIPT_POINTERS
# | Description: g_menu_content_table's word count, every word a pointer or 0, and g_script_table's leading pointer count after its count word.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MENU_CONTENT_TABLE_WORDS = 32
MENU_SCRIPT_POINTERS = 11


#----------------------
# | relocate_menu_pointers
# | Description: MENU pointer rule for a little-endian target: relocates the content table and the script table's pointers, the only packed-range pointers in its data.
# | Author: suinevere
# | Dependencies: relocate_words, OVERLAYS, MENU_CONTENT_TABLE_WORDS, MENU_SCRIPT_POINTERS
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset, target -- targets.Target
# | Returns: N/A
#----------------------
def relocate_menu_pointers(buf, off, target):
    content = off("g_menu_content_table")
    script = off("g_script_table")
    positions = [content + i * 4 for i in range(MENU_CONTENT_TABLE_WORDS)]
    positions += [script + 4 + i * 4 for i in range(MENU_SCRIPT_POINTERS)]
    relocate_words(buf, positions, OVERLAYS[6], target)


#----------------------
# | POINTER_RULES
# | Description: Per overlay name, the pointer relocation a target without convert rules runs instead; an overlay with no packed-range pointers has no entry.
# | Author: suinevere
# | Dependencies: relocate_title_pointers, relocate_wmap_pointers, relocate_menu_pointers
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
POINTER_RULES = {"title": relocate_title_pointers, "wmap": relocate_wmap_pointers, "menu": relocate_menu_pointers}


#----------------------
# | IGNORED_POINTER_WORDS
# | Description: Per overlay name, packed offsets the pointer census flags whose values only look like addresses (counts, colours, ids), so no rule relocates them.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
IGNORED_POINTER_WORDS = {}


#----------------------
# | AUTO_FUNCTION_TABLES
# | Description: Overlays whose every all-code-pointer data symbol is also defined in C, because their step, command and callback tables are too many to list by hand.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
AUTO_FUNCTION_TABLES = ("field", "wmap")


#----------------------
# | detect_function_tables
# | Description: Finds packed data symbols whose bytes, up to the next symbol, are all addresses of symbols outside the packed ranges (code).
# | Author: suinevere
# | Dependencies: packed_offset
# | Globals: N/A
# | Params: spec -- OverlaySpec, overlay -- decompressed overlay bytes, symbols -- name to PS1 address
# | Returns: list of (name, entry count) in address order
#----------------------
def detect_function_tables(spec, overlay, symbols):
    code = {addr for addr in symbols.values() if packed_offset(addr, spec) is None and spec.vram <= addr < spec.vram + len(overlay)}
    by_addr = {}
    for name, addr in sorted(symbols.items()):
        by_addr.setdefault(addr, name)
    addrs = sorted(by_addr)
    found = []
    for i, addr in enumerate(addrs):
        if packed_offset(addr, spec) is None:
            continue
        end = addrs[i + 1] if i + 1 < len(addrs) else addr
        words = []
        cursor = addr
        while cursor < end and packed_offset(cursor, spec) is not None and cursor + 4 <= spec.vram + len(overlay):
            words.append(struct.unpack_from("<I", overlay, cursor - spec.vram)[0])
            cursor += 4
        if words and all(word in code for word in words):
            found.append((by_addr[addr], len(words)))
    return found


#----------------------
# | tables_for
# | Description: An overlay's function tables: its FUNCTION_TABLES entries, plus every detected table for an AUTO_FUNCTION_TABLES overlay, each name once.
# | Author: suinevere
# | Dependencies: FUNCTION_TABLES, AUTO_FUNCTION_TABLES, detect_function_tables, C_DEFINED
# | Globals: N/A
# | Params: spec -- OverlaySpec, overlay -- decompressed overlay bytes, symbols -- name to PS1 address
# | Returns: tuple of (name, entry count)
#----------------------
def tables_for(spec, overlay, symbols):
    tables = list(FUNCTION_TABLES.get(spec.name, ()))
    if spec.name in AUTO_FUNCTION_TABLES:
        listed = {name for name, _ in tables} | set(C_DEFINED.get(spec.name, ()))
        tables += [entry for entry in detect_function_tables(spec, overlay, symbols) if entry[0] not in listed]
    return tuple(tables)


#----------------------
# | TABLE_LITERAL_LIMIT
# | Description: Function-table entries below this are plain numbers (FIELD_OBJECT_HANDLER_FUNCTION_MIN) and are emitted as literal casts rather than named.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TABLE_LITERAL_LIMIT = 0x100


#----------------------
# | function_table_source
# | Description: Builds C definitions of an overlay's function tables (tables_for), naming each PS1 entry through the symbol map and casting entries below TABLE_LITERAL_LIMIT; raises ValueError for any other entry with no symbol.
# | Author: suinevere
# | Dependencies: tables_for, TABLE_LITERAL_LIMIT
# | Globals: N/A
# | Params: spec -- OverlaySpec, overlay -- decompressed overlay bytes, symbols -- name to PS1 address
# | Returns: C source text, or None when the overlay has no function tables
#----------------------
def function_table_source(spec, overlay, symbols):
    tables = tables_for(spec, overlay, symbols)
    if not tables:
        return None
    names = {}
    for name, addr in sorted(symbols.items()):
        names.setdefault(addr, name)
    entries = []
    for table, count in tables:
        start = symbols[table] - spec.vram
        row = []
        for i in range(count):
            (addr,) = struct.unpack_from("<I", overlay, start + i * 4)
            if addr < TABLE_LITERAL_LIMIT:
                row.append((None, addr))
                continue
            if addr not in names:
                raise ValueError("%s[%d] = 0x%08X names no symbol" % (table, i, addr))
            row.append((names[addr], addr))
        entries.append((table, row))
    functions = sorted({name for _, row in entries for name, _ in row if name is not None})
    out = ["/*----------------------",
           " | %s_function_tables.c" % spec.name,
           " | Description: Generated by tools/convert_disc.py from the PS1 %s overlay: its function-pointer tables, with each entry named through the symbol map." % spec.name.upper(),
           " | Author: suinevere",
           " | Dependencies: tools/overlay_rules.py FUNCTION_TABLES",
           " | Globals: %s" % ", ".join(table for table, _ in tables),
           " | Params: N/A",
           " | Returns: N/A",
           " ----------------------*/"]
    out += ["extern void %s();" % name for name in functions]
    for table, row in entries:
        out.append("")
        out.append("void (*%s[%d])() = {" % (table, len(row)))
        out += ["    %s," % (name if name is not None else "(void (*)())0x%X" % value) for name, value in row]
        out.append("};")
    return "\n".join(out) + "\n"


#----------------------
# | build_overlay
# | Description: Packs an overlay's ranges, drops its all-zero tail, applies its TIM and data rules (or only its pointer rule for a target without convert rules),
# |              and builds its linker symbol script at the target's data address without the FUNCTION_TABLES or C_DEFINED names, giving placed symbols their fixed addresses.
# | Author: suinevere
# | Dependencies: target_spec, packed_offset, symbol_offset, convert_tim, tables_for, C_DEFINED, POINTER_RULES, data_base, targets.SATURN
# | Globals: N/A
# | Params: spec -- OverlaySpec, overlay -- decompressed overlay bytes, symbols -- name to PS1 address, target -- targets.Target, Saturn by default
# | Returns: (packed bytearray, linker script text); raises ValueError when the blob overflows the target's data region
#----------------------
def build_overlay(spec, overlay, symbols, target=targets.SATURN):
    spec = target_spec(spec, target)
    buf = bytearray()
    for start, end in spec.ranges:
        buf += overlay[start - spec.vram:end - spec.vram]

    tail_size = 0
    if spec.tail_from is not None:
        last_start, last_end = spec.ranges[-1]
        if not last_start <= spec.tail_from <= last_end:
            raise ValueError("overlay %s tail_from 0x%08X is outside its last range" % (spec.name, spec.tail_from))
        tail_size = last_end - spec.tail_from
        buf = buf[:len(buf) - tail_size]

    off = functools.partial(symbol_offset, symbols, spec)

    if target.convert_rules:
        for name in spec.tims:
            convert_tim(buf, off(name))
        spec.convert(buf, off)
    elif spec.name in POINTER_RULES:
        POINTER_RULES[spec.name](buf, off, target)

    base = data_base(spec, target)
    if target.regions is not None and len(buf) > target.regions[spec.name].data_size:
        raise ValueError("overlay %s data is 0x%X bytes, its %s region holds 0x%X"
                         % (spec.name, len(buf), target.name, target.regions[spec.name].data_size))

    lines = ["_lom_ovl_%s_data_size = 0x%08X;" % (spec.name, len(buf)),
             "_lom_ovl_%s_tail_size = 0x%08X;" % (spec.name, tail_size)]
    in_c = {name for name, _ in tables_for(spec, overlay, symbols)} | set(C_DEFINED.get(spec.name, ()))
    placed = dict(spec.placed)
    for name in sorted(symbols, key=lambda n: symbols[n]):
        offset = packed_offset(symbols[name], spec)
        if offset is not None and name not in in_c and name not in placed:
            lines.append("_%s = 0x%08X;" % (name, base + offset))
    lines += ["_%s = 0x%08X;" % (name, addr) for name, addr in spec.placed]
    return buf, "\n".join(lines) + "\n"
