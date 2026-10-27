
#----------------------
# | overlay_rules.py
# | Description: Per-overlay rules that pack an overlay's data ranges into its Dreamcast arena region, relocate its pointers and emit linker symbols.
# | Author: suinevere
# | Dependencies: config/us/symbols/<overlay>_symbol_addrs.txt, targets.py
# | Globals: OVERLAYS, FUNCTION_TABLES, AUTO_FUNCTION_TABLES, C_DEFINED, POINTER_RULES
# | Params: N/A
# | Returns: N/A
#----------------------
import collections
import functools
import struct

import overlay_text
import targets

#----------------------
# | OverlaySpec
# | Description: One overlay: name, PS1 load address, packed data ranges, a symbol file name or tuple of names, and a tail start address dropped from the disc blob.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OverlaySpec = collections.namedtuple("OverlaySpec", "name vram ranges symbols_file tail_from")
OverlaySpec.__new__.__defaults__ = (None,)

#----------------------
# | FUNCTION_TABLES
# | Description: Per overlay name, the function-pointer tables (name, entry count) defined in generated C from their PS1 entries instead of linked as raw overlay data.
# |              g_field_object_flag_handlers mixes built-in animation ids below TABLE_LITERAL_LIMIT with handler pointers, so detection skips it and it is listed here.
# |              g_wmap_effect35_emitter_b_steps is followed by non-pointer data before the next symbol, so detection skips it too.
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
             ("g_wmap_effect35_emitter_b_steps", 6), ("g_wmap_land_entry_steps", 4), ("g_wmap_land_return_steps", 6), ("g_wmap_special_travel_steps", 20),
             ("g_wmap_special_return_steps", 14), ("g_wmap_land_effect_13_sequence_9_steps", 6),
             ("D_800D4DB4", 6)),
}


#----------------------
# | C_DEFINED
# | Description: Per overlay name, data symbols a port C file (dreamcast/src/overlays) defines instead of the packed image, because their PS1 value points into main memory the converter cannot place.
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
# | MAIN_OWNED
# | Description: PS1 FIELD-range variables main's field_runtime.c reads and writes; a linked-arena target gives them storage in main (dreamcast/src/boot/main_globals_dc.c), so no overlay may pin its own copy or FIELD's world-map request never reaches main.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAIN_OWNED = ("g_pending_game_state", "g_field_scene_request_pending")


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
# | Description: Address the target loads an overlay's packed data blob at: its arena region's data base.
# | Author: suinevere
# | Dependencies: targets.Target
# | Globals: N/A
# | Params: spec -- OverlaySpec, target -- targets.Target
# | Returns: address; raises ValueError for an overlay the target gives no region
#----------------------
def data_base(spec, target):
    if spec.name not in target.regions:
        raise ValueError("overlay %s has no %s region" % (spec.name, target.name))
    return target.regions[spec.name].data_base


#----------------------
# | relocate_pointer
# | Description: Maps a PS1 pointer into the overlay's packed ranges onto the target's address for it; other values pass through.
# | Author: suinevere
# | Dependencies: packed_offset, data_base, targets.DREAMCAST
# | Globals: N/A
# | Params: value -- PS1 pointer value, spec -- OverlaySpec, target -- targets.Target, the Dreamcast by default
# | Returns: relocated pointer value
#----------------------
def relocate_pointer(value, spec, target=targets.DREAMCAST):
    offset = packed_offset(value, spec)
    return value if offset is None else data_base(spec, target) + offset


#----------------------
# | relocate_words
# | Description: Relocates the little-endian PS1 pointer words at the given packed offsets in place, writing them back little-endian.
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
# | FIELD_RODATA_BINS
# | Description: FIELD's splat rodata bins before its code holding tables FIELD's C reads (marker and small-icon frames; golem shape, weapon-class and menu element tables); packed on the Dreamcast only, ahead of FIELD's data range.
# | Author: suinevere
# | Dependencies: config/us/overlays/FIELD.BIN.yaml (unk2_rodata_2_after_a838c, unk2_rodata_after_bdca4)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FIELD_RODATA_BINS = ((0x800513E8, 0x8005146C), (0x80051888, 0x80051F28))

#----------------------
# | OVERLAYS
# | Description: Overlays the converter packs, keyed by CD resource index; MENU, ZUKAN, GOSUB and CARDA also pack the tables and strings that sit in rodata bins before their code, and FIELD its FIELD_RODATA_BINS.
# | Author: suinevere
# | Dependencies: FIELD_RODATA_BINS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAYS = {
    2: OverlaySpec("field", 0x8004FC70, list(FIELD_RODATA_BINS) + [(0x800CBF44, 0x80123FD8)], ("field_symbol_addrs.txt", "shared_symbol_addrs.txt"), 0x800F2268),
    4: OverlaySpec("title", 0x8004FC70, [(0x800522E8, 0x801026CC)], "title_symbol_addrs.txt"),
    15: OverlaySpec("checkps", 0x8004FC70, [(0x8004FCC4, 0x8004FD14), (0x80052428, 0x8005D060)], "checkps_symbol_addrs.txt"),
    5: OverlaySpec("gname", 0x80140000, [(0x80142C98, 0x8014F7B0)], "gname_symbol_addrs.txt"),
    6: OverlaySpec("menu", 0x80140000, [(0x8014057C, 0x801405B0), (0x8014FBF4, 0x8016B9DC)], "menu_symbol_addrs.txt"),
    7: OverlaySpec("shop", 0x80140000, [(0x80142D04, 0x80145CE4)], "shop_symbol_addrs.txt"),
    8: OverlaySpec("zukan", 0x80140000, [(0x8014001C, 0x80140E00), (0x8014471C, 0x80157D7C)], "zukan_symbol_addrs.txt"),
    13: OverlaySpec("gosub", 0x80140000, [(0x80140068, 0x80140080), (0x80147058, 0x8016B8C8)], "gosub_symbol_addrs.txt"),
    12: OverlaySpec("carda", 0x80140000, [(0x8014008C, 0x80140094), (0x801400C4, 0x801400CC), (0x801401B8, 0x801401C8), (0x80140244, 0x8014024C), (0x8014B038, 0x8016F000)], "carda_symbol_addrs.txt"),
    16: OverlaySpec("cload", 0x80140000, [(0x80145E9C, 0x8016B258)], "cload_symbol_addrs.txt"),
    17: OverlaySpec("niki", 0x80140000, [(0x801470F8, 0x8016DA80)], "niki_symbol_addrs.txt"),
    18: OverlaySpec("addhero", 0x80140000, [(0x80140088, 0x80140098), (0x80140114, 0x8014011C), (0x80146FA4, 0x8016D940)], "addhero_symbol_addrs.txt"),
    10: OverlaySpec("gover", 0x80140000, [(0x80140704, 0x80140704)], "gover_symbol_addrs.txt"),
    11: OverlaySpec("movie", 0x80140000, [(0x80141914, 0x80141914)], "movie_symbol_addrs.txt"),
    9: OverlaySpec("golem", 0x80140000, [(0x8014287C, 0x8014C28C)], "golem_symbol_addrs.txt"),
    14: OverlaySpec("wsel", 0x8004FC70, [(0x80052608, 0x800CA8E4)], "wsel_symbol_addrs.txt"),
    3: OverlaySpec("wmap", 0x8004FC70, [(0x8004FC74, 0x80054A2C), (0x800C4588, 0x801B32D8)], ("wmap_symbol_addrs.txt", "shared_symbol_addrs.txt"), 0x801AFBA0),
}


#----------------------
# | WMAP_SFX_BUFFERS
# | Description: Entries in g_wmap_sfx_buffers (0xFC bytes), each a PS1 pointer to an AKAO sound inside WMAP's packed range.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WMAP_SFX_BUFFERS = 0xFC // 4


#----------------------
# | relocate_wmap_pointers
# | Description: WMAP pointer rule for a little-endian target: relocates the three input-script pointers and the sound table, without which every map sound fails AKAO's header check.
# | Author: suinevere
# | Dependencies: relocate_words, OVERLAYS, WMAP_SFX_BUFFERS
# | Globals: N/A
# | Params: buf -- packed bytearray, off -- symbol name to packed offset, target -- targets.Target
# | Returns: N/A
#----------------------
def relocate_wmap_pointers(buf, off, target):
    scripts = off("g_wmap_input_scripts")
    sounds = off("g_wmap_sfx_buffers")
    positions = [scripts + i * 4 for i in range(3)] + [sounds + i * 4 for i in range(WMAP_SFX_BUFFERS)]
    relocate_words(buf, positions, OVERLAYS[3], target)


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
# | TEXT_TABLES
# | Description: PS1 address of the message table whose empty PocketStation entries the Dreamcast build fills, per overlay.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TEXT_TABLES = {"carda": 0x8014B038, "cload": 0x80145E9C, "niki": 0x801470F8, "addhero": 0x80146FA4}

#----------------------
# | TEXT_REPLACEMENTS
# | Description: Message lines the Dreamcast build rewords in every card overlay's table, by index, with the US bytes they replace: a save's VMU budget is 12 blocks, not the PS1 card's 2.
# | Author: suinevere
# | Dependencies: src/card/vmu_format.c (VMU_FILE_OVERHEAD_BLOCKS, VMU_BLOCKS_PER_PS1_BLOCK)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TEXT_REPLACEMENTS = {21: (b"Uses 2 Blocks.", "Uses 12 Blocks."),
                     45: (b"Requir\x1f\x022 Blocks\x17save.", "Required 12 Blocks to save.")}


#----------------------
# | append_text_table
# | Description: Appends a rebuilt copy of an overlay's message table holding the English lines, and moves every symbol inside the old table to the copy.
# | Author: suinevere
# | Dependencies: overlay_text.load_lines, overlay_text.encode_overlay, overlay_text.rebuild_table, packed_offset
# | Globals: TEXT_TABLES, TEXT_REPLACEMENTS
# | Params: buf -- packed bytearray, extended in place; spec -- OverlaySpec; symbols -- name to PS1 address
# | Returns: dict of moved symbol name to packed offset
#----------------------
def append_text_table(buf, spec, symbols):
    table = TEXT_TABLES[spec.name]
    lines = {i: overlay_text.encode_overlay(text) for i, text in overlay_text.load_lines().get(spec.name, {}).items()}
    replacements = {i: (old, overlay_text.encode_overlay(text)) for i, (old, text) in TEXT_REPLACEMENTS.items()}
    new, old_size = overlay_text.rebuild_table(bytes(buf), packed_offset(table, spec), lines, overlay_text.text_table.TWO_BYTE_CODES["us"], replacements)
    buf += bytes(-len(buf) % 4)
    start = len(buf)
    buf += new
    return {name: start + addr - table for name, addr in symbols.items() if table <= addr < table + old_size}


#----------------------
# | TEXT_SWAPS
# | Description: Same-length string replacements the Dreamcast build makes in an overlay's packed data, by overlay name; none at present.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TEXT_SWAPS = {}


#----------------------
# | swap_text
# | Description: Replaces each old string with its same-length new one in place, so no other string or pointer moves.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: buf -- packed bytearray, changed in place; swaps -- (old, new) byte-string pairs
# | Returns: N/A; raises ValueError when lengths differ or an old string is not found exactly once
#----------------------
def swap_text(buf, swaps):
    for old, new in swaps:
        at = buf.find(old)
        if len(old) != len(new) or at < 0 or buf.find(old, at + 1) >= 0:
            raise ValueError("text swap %r -> %r needs equal lengths and exactly one match" % (old, new))
        buf[at:at + len(old)] = new


#----------------------
# | build_overlay
# | Description: Packs an overlay's ranges, drops its all-zero tail, applies its pointer rule and text changes,
# |              and builds its linker symbol script at the target's data address without the FUNCTION_TABLES, C_DEFINED or MAIN_OWNED names.
# | Author: suinevere
# | Dependencies: packed_offset, symbol_offset, tables_for, C_DEFINED, MAIN_OWNED, POINTER_RULES, data_base, append_text_table, swap_text, TEXT_TABLES, TEXT_SWAPS, targets.DREAMCAST
# | Globals: N/A
# | Params: spec -- OverlaySpec, overlay -- decompressed overlay bytes, symbols -- name to PS1 address, target -- targets.Target, the Dreamcast by default
# | Returns: (packed bytearray, linker script text); raises ValueError when the blob overflows the target's data region
#----------------------
def build_overlay(spec, overlay, symbols, target=targets.DREAMCAST):
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
    if spec.name in POINTER_RULES:
        POINTER_RULES[spec.name](buf, off, target)

    moved = {}
    if spec.name in TEXT_TABLES:
        moved = append_text_table(buf, spec, symbols)
    if spec.name in TEXT_SWAPS:
        swap_text(buf, TEXT_SWAPS[spec.name])

    base = data_base(spec, target)
    if len(buf) > target.regions[spec.name].data_size:
        raise ValueError("overlay %s data is 0x%X bytes, its %s region holds 0x%X"
                         % (spec.name, len(buf), target.name, target.regions[spec.name].data_size))

    lines = ["_lom_ovl_%s_data_size = 0x%08X;" % (spec.name, len(buf)),
             "_lom_ovl_%s_tail_size = 0x%08X;" % (spec.name, tail_size)]
    in_c = {name for name, _ in tables_for(spec, overlay, symbols)} | set(C_DEFINED.get(spec.name, ())) | set(MAIN_OWNED)
    for name in sorted(symbols, key=lambda n: symbols[n]):
        offset = packed_offset(symbols[name], spec)
        if name in moved:
            lines.append("_%s = 0x%08X;" % (name, base + moved[name]))
        elif offset is not None and name not in in_c:
            lines.append("_%s = 0x%08X;" % (name, base + offset))
    return buf, "\n".join(lines) + "\n"
