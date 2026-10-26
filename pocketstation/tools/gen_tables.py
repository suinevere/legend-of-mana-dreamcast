"""----------------------
 | gen_tables.py
 | Description: Writes the 256-byte pages of RES1506.BIN the GAME keeps, the map from file pages to them, and the pages it reads from the pet save with their CRC. Also writes the rows of the sprites the scripts draw, stored ready to copy.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/{rrl_rom.h,game.h,sprite.c,hud.c,screen.c,slots.c,sound.c,reel.c,rng.c,misc.c,system.c,app.c} (the tables, their index bounds and the bitmap offsets the C forms)
 | Globals: N/A
 | Params: RES1506.BIN OUT.s OUT.txt
 | Returns: N/A
 ----------------------"""

import struct
import sys

"""----------------------
 | BYTES_PER_LINE
 | Description: How many .byte values gen_tables.py writes on one line, as the runtime's own hand-written tables do.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
BYTES_PER_LINE = 16

"""----------------------
 | CARD_BASE
 | Description: The card address of the file's first byte, game.h's ROM_BASE; a table pointer less it is a file offset.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/game.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
CARD_BASE = 0x02000000

"""----------------------
 | ROM_SPRITES / ROM_FRAME_SETS / ROM_MARK_OFFSETS / ROM_ANIM_HOLD / ROM_SOUNDS / ROM_NIBBLES / ROM_STRIPS / ROM_ODDS_SEL / ROM_ODDS_SET / ROM_ODDS_NTH / ROM_BYTES / ROM_EVENT_ODDS / ROM_WORDS / ROM_SYMBOLS_10X9 / ROM_SYMBOLS_8X11 / ROM_YES / ROM_NO
 | Description: File offsets of the tables the C reads, from rrl_rom.h and the C files' own ROM_* names; ROM_YES and ROM_NO are app.c's two 7-row quit dialog pictures.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/rrl_rom.h, misc.c, slots.c, sound.c, sprite.c, app.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
ROM_SPRITES = 0xA03C
ROM_FRAME_SETS = 0xA384
ROM_MARK_OFFSETS = 0xA7A4
ROM_ANIM_HOLD = 0xA7B0
ROM_SOUNDS = 0xA7C8
ROM_NIBBLES = 0x9DF0
ROM_STRIPS = 0x9E00
ROM_ODDS_SEL = 0x9EDC
ROM_ODDS_SET = 0x9EEC
ROM_ODDS_NTH = 0x9EF8
ROM_BYTES = 0x9EFC
ROM_EVENT_ODDS = 0x9F0C
ROM_WORDS = 0x8094
ROM_SYMBOLS_10X9 = 0x6520
ROM_SYMBOLS_8X11 = 0x7DBC
ROM_YES = 0x64B0
ROM_NO = 0x64CC

"""----------------------
 | SPRITE_COUNT / SPRITE_ID_MAX
 | Description: The sprite descriptors before ROM_FRAME_SETS, whose bitmaps walk follows, and the highest id the C forms: hud.c's game_anim_draw of a free slot, 0xFF + ANIM_SPRITES, as on the ARM.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/sprite.c, hud.c (ANIM_SPRITES)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
SPRITE_COUNT = (ROM_FRAME_SETS - ROM_SPRITES) // 8
SPRITE_ID_MAX = 0xFF + 0x1C

"""----------------------
 | FRAME_TYPES / EVENT_SCENES / SOUND_COUNT / SYMBOL_BYTES / BYTE_INDEXES
 | Description: Object types 0x58-0x6A with a frame set, scene ids 7-14 with an event list, sounds 0-3, one 12-byte symbol picture, and the 256 values of a uint8_t index.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/sprite.c, rng.c, sound.c, reel.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FRAME_TYPES = 19
EVENT_SCENES = 8
SOUND_COUNT = 4
SYMBOL_BYTES = 12
BYTE_INDEXES = 0x100

"""----------------------
 | SYMBOL_WIDTH / SYMBOL_HEIGHT
 | Description: The size reel.c's game_reel_window_draw draws ROM_SYMBOLS_10X9's pictures at.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/reel.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
SYMBOL_WIDTH = 10
SYMBOL_HEIGHT = 9

"""----------------------
 | HASH_BUCKETS / HASH_SLOTS / EMPTY_SLOT
 | Description: The stored sprites' hash: 64 displacements chosen by (address >> 8) + 8 width, 64 slots by (address >> 2) plus the displacement, and the key byte of an empty slot.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu_game/sprite.s (g_sprite_find)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
HASH_BUCKETS = 64
HASH_SLOTS = 64
EMPTY_SLOT = 0xFF

"""----------------------
 | STORED_SPRITES
X
 | Author: suinevere
 | Dependencies: pocketstation/tools/project_frame.py (sprite_draws), pocketstation/tests/scripts/*.pks
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
STORED_SPRITES = (
    (0x50A0, 19), (0x5100, 19), (0x5160, 19), (0x51C0, 19), (0x53A0, 18), (0x5400, 18), (0x56A0, 20), (0x5700, 20),
    (0x5820, 14), (0x5868, 14), (0x5AC0, 18), (0x5B20, 18), (0x63E0, 15), (0x6448, 15), (0x652C, 10), (0x6538, 10),
    (0x6544, 10), (0x6550, 10), (0x655C, 10), (0x6568, 10), (0x6598, 4), (0x659C, 4), (0x65A0, 4), (0x65A4, 4),
    (0x65A8, 4), (0x65AC, 4), (0x65B0, 4), (0x65C0, 4), (0x65C4, 4), (0x65D8, 4), (0x65EC, 4), (0x65F8, 6),
    (0x6608, 9), (0x6620, 19), (0x66CC, 23), (0x68C4, 27), (0x6AAC, 13), (0x7C50, 11), (0x7C70, 11), (0x7D38, 9),
    (0x7DB0, 7), (0x7DB8, 7), (0x7EBC, 25), (0x7F00, 28), (0x8538, 21),
)

"""----------------------
 | MAP_FIRST / MAP_END / HIGH_FIRST
 | Description: The file pages app_tb_map covers (0x4A00-0xA9FF, every table the C reads), and the first page of the high span the constant-base readers address as app_hi.
 | Author: suinevere
 | Dependencies: walk
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
MAP_FIRST = 0x4A
MAP_END = 0xAA
HIGH_FIRST = 0x9D

"""----------------------
 | GAME_PAGES
 | Description: File pages the GAME carries: every mapped page, since the slim pet save holds only the PS1 file's first 0x400 bytes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: MAP_FIRST, MAP_END
 | Params: N/A
 | Returns: N/A
 ----------------------"""
GAME_PAGES = tuple(range(MAP_FIRST, MAP_END))

"""----------------------
 | HEADER
 | Description: tables.s's file header; a generated file carries only this, with no further comments.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
HEADER = """;----------------------
; | tables.s
; | Description: Generated by pocketstation/tools/gen_tables.py from RES1506.BIN: app_tb_map, app_hi, app_tables, the app_tb_PP page bases and app_tb_pages, then app_sprites and g_sprite_find's hash. Do not edit.
; | Author: suinevere
; | Dependencies: pocketstation/tools/gen_tables.py, RES1506.BIN
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
"""

"""----------------------
 | LISTING_HEADER
 | Description: tables.txt's file header; the kept span's line and one line per table follow.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
LISTING_HEADER = """#----------------------
# | tables.txt
# | Description: Generated by pocketstation/tools/gen_tables.py from RES1506.BIN: "kept START END", then "table START END NAME" per table (hex, END exclusive), "page PP game|save" and "sprite OFFSET WIDTH HEIGHT MASK". Do not edit.
# | Author: suinevere
# | Dependencies: pocketstation/tools/gen_tables.py, RES1506.BIN
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
"""


"""----------------------
 | word
 | Description: The little-endian 32-bit word at a file offset.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes; off -- file offset
 | Returns: the word
 ----------------------"""
def word(data, off):
    return struct.unpack_from("<I", data, off)[0]


"""----------------------
 | pointer
 | Description: The table pointer at a file offset, as a file offset; fails when it points outside the file.
 | Author: suinevere
 | Dependencies: word, CARD_BASE
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes; off -- file offset of the pointer
 | Returns: the file offset it points to
 ----------------------"""
def pointer(data, off):
    p = word(data, off) - CARD_BASE
    if not 0 <= p < len(data):
        raise ValueError("pointer at %04X leaves the file: %08X" % (off, word(data, off)))
    return p


"""----------------------
 | runs_to_next
 | Description: Each start's extent up to the next start or cut above it, so a sprite's later frames, or a list, run until the next one begins.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: starts -- file offsets; cuts -- further offsets that end a run
 | Returns: sorted (start, end) pairs
 ----------------------"""
def runs_to_next(starts, cuts):
    ends = sorted(set(starts) | set(cuts))
    return [(s, min(e for e in ends if e > s)) for s in sorted(set(starts))]


"""----------------------
 | strip_starts
 | Description: ROM_STRIPS' strip pointers, read until the first strip begins.
 | Author: suinevere
 | Dependencies: pointer, ROM_STRIPS
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes
 | Returns: (strip start offsets, offset past the pointers)
 ----------------------"""
def strip_starts(data):
    strips = []
    off = ROM_STRIPS
    while not strips or off < min(strips):
        strips.append(pointer(data, off))
        off += 4
    return strips, off


"""----------------------
 | bitmap_runs
 | Description: Each sprite descriptor's bitmap and the frames after it, up to the next bitmap or table.
 | Author: suinevere
 | Dependencies: pointer, runs_to_next, ROM_*, SPRITE_COUNT
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes
 | Returns: sorted (start, end) file offsets, end exclusive
 ----------------------"""
def bitmap_runs(data):
    bitmaps = [pointer(data, ROM_SPRITES + 8 * i + 4) for i in range(SPRITE_COUNT)]
    return runs_to_next(bitmaps, [ROM_SYMBOLS_10X9, ROM_SYMBOLS_8X11, ROM_WORDS, ROM_NIBBLES, ROM_SPRITES])


"""----------------------
 | shift_left / shift_right
 | Description: sprite.c's ARM register shifts on 32 bits: amounts of 32 and above give zero.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/sprite.c
 | Globals: N/A
 | Params: v -- value; n -- shift
 | Returns: the shifted value
 ----------------------"""
def shift_left(v, n):
    return 0 if n >= 32 else (v << n) & 0xFFFFFFFF


def shift_right(v, n):
    return 0 if n >= 32 else v >> n


"""----------------------
 | mirror_row
 | Description: sprite.c's mirror_row: reverses the low width bits of a row.
 | Author: suinevere
 | Dependencies: shift_left, pocketstation/src/vmu/sprite.c
 | Globals: N/A
 | Params: v -- row bits; width -- bits in the row
 | Returns: the mirrored row
 ----------------------"""
def mirror_row(v, width):
    out, bit = 0, shift_left(1, (width - 1) & 0xFF)
    for _ in range(width):
        if v & 1:
            out |= bit
        v >>= 1
        bit >>= 1
    return out


"""----------------------
 | unpack_rows
 | Description: sprite.c's unpack_rows at a file offset: height rows of width bits packed LSB first across little-endian words, mirrored when asked; height 0 gives 256 rows as in the C.
 | Author: suinevere
 | Dependencies: word, shift_left, shift_right, mirror_row, pocketstation/src/vmu/sprite.c
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes; src -- file offset; width -- bits a row; height -- rows; mirror -- nonzero to mirror
 | Returns: (list of row words, file offset after the last word read)
 ----------------------"""
def unpack_rows(data, src, width, height, mirror):
    keep = ~shift_left(0xFFFFFFFF, width) & 0xFFFFFFFF
    acc, have, rows = 0, 0, []
    for _ in range(height or 256):
        if have < width:
            w = word(data, src)
            src += 4
            row = acc | shift_left(w, have & 0xFF)
            acc = shift_right(w, (width - have) & 0xFF)
            have += 32
        else:
            row = acc
            acc = shift_right(acc, width)
        row &= keep
        if mirror:
            row = mirror_row(row, width)
        rows.append(row)
        have -= width
    return rows, src


"""----------------------
 | drawable_sprites
 | Description: Every bitmap the C can unpack that is not whole bytes wide: each descriptor's frames on its own stride inside its run, and every strip byte's 10x9 symbol; fails on an unaligned one.
 | Author: suinevere
 | Dependencies: word, pointer, bitmap_runs, strip_starts, ROM_SPRITES, ROM_SYMBOLS_10X9, SPRITE_COUNT, SYMBOL_BYTES, SYMBOL_WIDTH, SYMBOL_HEIGHT
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes
 | Returns: sorted (offset, width, height, mask) tuples, one per offset and width
 ----------------------"""
def drawable_sprites(data):
    ends = dict(bitmap_runs(data))
    found = {}
    for i in range(SPRITE_COUNT):
        packed = word(data, ROM_SPRITES + 8 * i)
        width, height, mask = (packed >> 16) & 0xFF, packed >> 24, (packed >> 6) & 1
        if width >= 32 or not width % 8:
            continue
        start = pointer(data, ROM_SPRITES + 8 * i + 4)
        stride = 4 * ((width * height + 31) // 32) * (1 + mask)
        for at in range(start, ends[start] - stride + 1, stride):
            found[(at, width, height)] = max(found.get((at, width, height), 0), mask)
    strips, _ = strip_starts(data)
    for symbol in set(data[min(strips):max(strips)]):
        found.setdefault((ROM_SYMBOLS_10X9 + SYMBOL_BYTES * symbol, SYMBOL_WIDTH, SYMBOL_HEIGHT), 0)
    heights = {}
    for at, width, height in found:
        if heights.setdefault((at, width), height) != height:
            raise ValueError("bitmap %04X is drawn %d wide at two heights" % (at, width))
        if at & 3:
            raise ValueError("bitmap %04X is not 4-aligned, and g_sprite_find ignores address bits 0-1" % at)
    return sorted((at, width, height, mask) for (at, width, height), mask in found.items())


"""----------------------
 | stored_sprites
 | Description: drawable_sprites' tuples that STORED_SPRITES names, failing when it names one that is not drawable.
 | Author: suinevere
 | Dependencies: drawable_sprites, STORED_SPRITES
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes
 | Returns: sorted (offset, width, height, mask) tuples
 ----------------------"""
def stored_sprites(data):
    keep = set(STORED_SPRITES)
    out = [s for s in drawable_sprites(data) if s[:2] in keep]
    if len(out) != len(keep):
        raise ValueError("STORED_SPRITES names %d sprites that are not drawable" % (len(keep) - len(out)))
    return out


"""----------------------
 | stored_rows
 | Description: A drawable sprite's rows as g_sprite_draw copies them: each row's low (width + 7) / 8 bytes, image rows then, when masked, mask rows.
 | Author: suinevere
 | Dependencies: unpack_rows
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes; sprite -- a drawable_sprites tuple
 | Returns: bytes
 ----------------------"""
def stored_rows(data, sprite):
    at, width, height, mask = sprite
    rows, after = unpack_rows(data, at, width, height, 0)
    if mask:
        rows += unpack_rows(data, after, width, height, 0)[0]
    out = b"".join(r.to_bytes(4, "little")[:(width + 7) // 8] for r in rows)
    if len(out) > 256:
        raise ValueError("bitmap %04X's rows take %d bytes, more than an offset byte reaches" % (at, len(out)))
    return out


"""----------------------
 | sprite_hash
 | Description: Places every stored sprite in one of 64 slots, slot (address >> 2) + displacement[((address >> 8) + 8 width) % 64] modulo 64, largest buckets first.
 | Author: suinevere
 | Dependencies: HASH_BUCKETS, HASH_SLOTS
 | Globals: N/A
 | Params: sprites -- stored_sprites' tuples
 | Returns: (displacements, list of HASH_SLOTS sprite tuples or None)
 ----------------------"""
def sprite_hash(sprites):
    buckets = {}
    for s in sprites:
        buckets.setdefault(((s[0] >> 8) + 8 * s[1]) % HASH_BUCKETS, []).append(s)
    shift = [0] * HASH_BUCKETS
    slots = [None] * HASH_SLOTS
    for b, members in sorted(buckets.items(), key=lambda kv: (-len(kv[1]), kv[0])):
        lows = [(s[0] >> 2) % HASH_SLOTS for s in members]
        if len(set(lows)) != len(lows):
            raise ValueError("bucket %d holds two sprites with one slot" % b)
        for d in range(HASH_SLOTS):
            if all(slots[(q + d) % HASH_SLOTS] is None for q in lows):
                break
        else:
            raise ValueError("bucket %d fits no displacement" % b)
        shift[b] = d
        for q, s in zip(lows, members):
            slots[(q + d) % HASH_SLOTS] = s
    return shift, slots


"""----------------------
 | walk
 | Description: Every table the C reads and the pointees it follows (sprite bitmaps, frame sets, strips, event lists), each widened to the indexes the C can form.
 | Author: suinevere
 | Dependencies: pointer, runs_to_next, strip_starts, bitmap_runs, ROM_*, SPRITE_ID_MAX, FRAME_TYPES, EVENT_SCENES, SOUND_COUNT, SYMBOL_BYTES, BYTE_INDEXES
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes
 | Returns: sorted (start, end, name) triples in file offsets, end exclusive
 ----------------------"""
def walk(data):
    tables = [
        (ROM_SPRITES, ROM_SPRITES + 8 * (SPRITE_ID_MAX + 1), "ROM_SPRITES descriptors, ids up to a free anim slot's 0xFF+ANIM_SPRITES (hud.c)"),
        (ROM_FRAME_SETS, ROM_FRAME_SETS + 4 * FRAME_TYPES, "ROM_FRAME_SETS pointers, object types 0x58-0x6A (sprite.c)"),
        (ROM_MARK_OFFSETS, ROM_ANIM_HOLD, "ROM_MARK_OFFSETS, three halfwords from +6 at most (hud.c game_icon_draw)"),
        (ROM_ANIM_HOLD, ROM_ANIM_HOLD + BYTE_INDEXES - 1, "ROM_ANIM_HOLD and ROM_ANIM_DELAY, a byte per anim id below 0xFF (slots.c)"),
        (ROM_SOUNDS, ROM_SOUNDS + 4 * (SOUND_COUNT + 1), "ROM_SOUNDS, start and end of sounds 0-3; the samples are never read (sound.c)"),
        (ROM_NIBBLES, ROM_NIBBLES + BYTE_INDEXES // 2, "ROM_NIBBLES_9DF0, a nibble per byte index (misc.c game_nibble)"),
        (ROM_ODDS_SEL, ROM_ODDS_SET, "ROM_ODDS_SEL, sel_a | sel_b << 1 (rng.c)"),
        (ROM_ODDS_SET, ROM_ODDS_NTH, "ROM_ODDS_SET, up to ROM_ODDS_NTH (rng.c)"),
        (ROM_ODDS_NTH, ROM_BYTES, "ROM_ODDS_NTH, one word (rng.c)"),
        (ROM_BYTES, ROM_BYTES + BYTE_INDEXES, "ROM_BYTES_9EFC, a byte per byte index (misc.c game_byte_is)"),
        (ROM_EVENT_ODDS, ROM_EVENT_ODDS + 4 * EVENT_SCENES, "ROM_EVENT_ODDS pointers, scene ids 7-14 (rng.c)"),
        (ROM_WORDS, ROM_WORDS + 2 * BYTE_INDEXES, "ROM_WORDS_8094, a halfword per byte index (system.c, rng.c)"),
        (ROM_SYMBOLS_10X9, ROM_SYMBOLS_10X9 + SYMBOL_BYTES * BYTE_INDEXES, "ROM_SYMBOLS_10X9, a strip byte's symbol (reel.c symbols_draw)"),
        (ROM_SYMBOLS_8X11, ROM_SYMBOLS_8X11 + SYMBOL_BYTES * BYTE_INDEXES, "ROM_SYMBOLS_8X11, a strip or bag byte's symbol (reel.c, screen.c)"),
        (ROM_YES, ROM_YES + 28, "the quit dialog's yes picture, 7 rows (app.c)"),
        (ROM_NO, ROM_NO + 28, "the quit dialog's no picture, 7 rows (app.c)"),
    ]
    strips, off = strip_starts(data)
    tables.append((ROM_STRIPS, off, "ROM_STRIPS pointers, strip n's start and end up to the first strip (reel.c)"))
    tables.append((min(strips), max(strips), "ROM_STRIPS strip data (reel.c game_strip_load)"))
    for s, e in bitmap_runs(data):
        tables.append((s, e, "a sprite bitmap and its later frames, up to the next bitmap or table (sprite.c, hud.c)"))
    sets = [pointer(data, ROM_FRAME_SETS + 4 * i) for i in range(FRAME_TYPES)]
    for s, e in runs_to_next(sets, [ROM_MARK_OFFSETS]):
        tables.append((s, e, "a frame set, up to the next set or ROM_MARK_OFFSETS (sprite.c game_frame_info)"))
    lists = [pointer(data, ROM_EVENT_ODDS + 4 * i) for i in range(EVENT_SCENES)]
    for s, e in runs_to_next(lists, [ROM_SPRITES]):
        tables.append((s, e, "an event odds list, up to the next list or ROM_SPRITES (rng.c game_event_roll)"))
    for s, e, name in tables:
        if not 0 <= s < e <= len(data):
            raise ValueError("%s leaves the file: %04X-%04X" % (name, s, e))
    return sorted(tables)


"""----------------------
 | kept_span
 | Description: The span tables.s stores: the lowest table start, rounded down to a word so sprite rows stay 4-aligned, to the highest table end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: tables -- walk's triples
 | Returns: (start, end) file offsets, end exclusive
 ----------------------"""
def kept_span(tables):
    return min(s for s, _, _ in tables) & ~3, max(e for _, e, _ in tables)


"""----------------------
 | page_kinds
 | Description: How the assembly reads each byte: random (an indexed table), unpack (a whole-byte-wide bitmap or 8x11 symbol), putrows (the quit dialog), wide (32-wide plots) or stored. The most specific table wins.
 | Author: suinevere
 | Dependencies: walk, bitmap_runs, word, pointer, ROM_*, SPRITE_COUNT
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes
 | Returns: list of 0x10000 kind strings, "" for unlisted bytes
 ----------------------"""
def page_kinds(data):
    runs = set(bitmap_runs(data))
    widths = {}
    for i in range(SPRITE_COUNT):
        packed = word(data, ROM_SPRITES + 8 * i)
        widths.setdefault(pointer(data, ROM_SPRITES + 8 * i + 4), []).append((packed >> 16) & 0xFF)
    order = []
    for s, e, name in walk(data):
        if (s, e) in runs:
            ws = widths[s]
            kind = "wide" if all(w >= 32 for w in ws) else "unpack" if any(w % 8 == 0 for w in ws) else "stored"
        elif s == ROM_SYMBOLS_10X9:
            kind = "stored"
        elif s == ROM_SYMBOLS_8X11:
            kind = "unpack"
        elif s in (ROM_YES, ROM_NO):
            kind = "putrows"
        else:
            kind = "random"
        order.append((e - s, s, e, kind))
    kinds = [""] * 0x10000
    for _, s, e, kind in sorted(order, reverse=True):
        kinds[s:e] = [kind] * (e - s)
    return kinds


"""----------------------
 | write_tables
 | Description: Writes HEADER, app_tb_map (each mapped page's GAME page), app_hi, app_tables (rt_rom8's base) and an app_tb_PP base for every kept page PP, app_tb_first, app_tb_count, then app_tb_pages.
 | Author: suinevere
 | Dependencies: HEADER, GAME_PAGES, MAP_FIRST, MAP_END, HIGH_FIRST, byte_lines
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes; out -- the open output file
 | Returns: N/A
 ----------------------"""
def write_tables(data, out):
    pages = sorted(GAME_PAGES)
    out.write(HEADER)
    out.write("app_tb_map:\n")
    byte_lines([">(app_tb_pages+$%04X)" % (pages.index(p) << 8) if p in pages else 0xFF
                for p in range(MAP_FIRST, MAP_END)], out)
    out.write("app_hi\tEQU\tapp_tb_pages+$%04X-$%04X\n" % (pages.index(HIGH_FIRST) << 8, HIGH_FIRST << 8))
    out.write("app_tables\tEQU\tapp_tb_pages-$%04X\n" % (MAP_FIRST << 8))
    for first in pages:
        out.write("app_tb_%02X\tEQU\tapp_tb_pages+$%04X-$%04X\n" % (first, pages.index(first) << 8, first << 8))
    out.write("app_tb_first\tEQU\t$%02X\napp_tb_count\tEQU\t$%02X\n" % (MAP_FIRST, MAP_END - MAP_FIRST))
    out.write("\t.cnop 0, $100\n")
    out.write("app_tb_pages:\n")
    for p in pages:
        byte_lines(data[p << 8:(p + 1) << 8], out)


"""----------------------
 | byte_lines
 | Description: Writes values as .byte lines of BYTES_PER_LINE, numbers as $XX and strings as they are.
 | Author: suinevere
 | Dependencies: BYTES_PER_LINE
 | Globals: N/A
 | Params: values -- ints or expression strings; out -- the open output file
 | Returns: N/A
 ----------------------"""
def byte_lines(values, out):
    text = [v if isinstance(v, str) else "$%02X" % v for v in values]
    for i in range(0, len(text), BYTES_PER_LINE):
        out.write("\t.byte " + ", ".join(text[i:i + BYTES_PER_LINE]) + "\n")


"""----------------------
 | write_sprites
 | Description: Writes app_sprites, every stored sprite's rows, then g_sprite_find's hash. Per slot it holds the key's address >> 8, width, height (bit 7 for a mask) and the rows' address.
 | Author: suinevere
 | Dependencies: stored_rows, sprite_hash, byte_lines, EMPTY_SLOT
 | Globals: N/A
 | Params: data -- RES1506.BIN's bytes; sprites -- stored_sprites' tuples; out -- the open output file
 | Returns: N/A
 ----------------------"""
def write_sprites(data, sprites, out):
    shift, slots = sprite_hash(sprites)
    at, offsets = 0, {}
    out.write("app_sprites:\n")
    for sprite in sprites:
        rows = stored_rows(data, sprite)
        offsets[sprite] = at
        byte_lines(rows, out)
        at += len(rows)
    out.write("g_spr_hash:\n")
    byte_lines(shift, out)
    out.write("g_spr_kb:\n")
    byte_lines([EMPTY_SLOT if s is None else s[0] >> 8 for s in slots], out)
    out.write("g_spr_kw:\n")
    byte_lines([0 if s is None else s[1] for s in slots], out)
    out.write("g_spr_kh:\n")
    byte_lines([0 if s is None else s[2] | s[3] << 7 for s in slots], out)
    out.write("g_spr_lo:\n")
    byte_lines([0 if s is None else "<(app_sprites+$%04X)" % offsets[s] for s in slots], out)
    out.write("g_spr_hi:\n")
    byte_lines([0 if s is None else ">(app_sprites+$%04X)" % offsets[s] for s in slots], out)


"""----------------------
 | write_listing
 | Description: Writes LISTING_HEADER, the kept span, every table walk found, where each mapped page is read from and every sprite stored unpacked.
 | Author: suinevere
 | Dependencies: LISTING_HEADER, GAME_PAGES, MAP_FIRST, MAP_END
 | Globals: N/A
 | Params: tables -- walk's triples; span -- kept_span's pair; sprites -- stored_sprites' tuples; out -- the open output file
 | Returns: N/A
 ----------------------"""
def write_listing(tables, span, sprites, out):
    out.write(LISTING_HEADER)
    out.write("kept %04X %04X\n" % span)
    for s, e, name in tables:
        out.write("table %04X %04X %s\n" % (s, e, name))
    for p in range(MAP_FIRST, MAP_END):
        out.write("page %02X %s\n" % (p, "game" if p in GAME_PAGES else "save"))
    for at, width, height, mask in sprites:
        out.write("sprite %04X %d %d %d\n" % (at, width, height, mask))


"""----------------------
 | main
 | Description: CLI: gen_tables.py RES1506.BIN OUT.s OUT.txt.
 | Author: suinevere
 | Dependencies: walk, kept_span, stored_sprites, write_tables, write_sprites, write_listing
 | Globals: N/A
 | Params: argv -- arguments, or None for sys.argv
 | Returns: 0, or 2 on a usage error
 ----------------------"""
def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 3:
        sys.stderr.write("usage: gen_tables.py RES1506.BIN OUT.s OUT.txt\n")
        return 2
    with open(argv[0], "rb") as f:
        data = f.read()
    tables = walk(data)
    span = kept_span(tables)
    sprites = stored_sprites(data)
    with open(argv[1], "w", newline="\n") as f:
        write_tables(data, f)
        write_sprites(data, sprites, f)
    with open(argv[2], "w", newline="\n") as f:
        write_listing(tables, span, sprites, f)
    return 0


if __name__ == "__main__":
    sys.exit(main())
