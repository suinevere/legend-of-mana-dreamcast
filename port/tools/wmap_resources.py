#----------------------
# | wmap_resources.py
# | Description: Converts the PS1 disc resources WMAP's Home path loads so the big-endian Saturn code parses them in place, and packs the 361 per-cell layout images.
# | Author: suinevere
# | Dependencies: overlay_rules.py
# | Globals: ResourceFormatError, STREAM, RAW, ANIMATION_TABLE_WORDS, INTERFACE_CLUTS, INTERFACE_RAW8, RESIDENT_CLUTS, RESIDENT_RAW8, LAND_IMAGE_WORDS, LAND_IMAGE_ROWS, LAND_IMAGE_CLUTS, LAND_ANIMATION_OFFSET, LAYOUT_IMAGE_FIRST, LAYOUT_IMAGE_COUNT, LAYOUT_IMAGE_BYTES, LAYOUT_IMAGE_STRIDE_SECTORS, SECTOR, RULES, UNBUFFERED, SKIPPED
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

from overlay_rules import convert_block_header, convert_tim, ps1_to_saturn_color, swap_words


#----------------------
# | ResourceFormatError
# | Description: Raised when a WMAP resource does not have the layout its converter expects.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class ResourceFormatError(ValueError):
    pass


#----------------------
# | STREAM / RAW
# | Description: RULES kinds: a stream resource is decompressed from its bytes after the leading flag byte before converting, a raw one is converted as read.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STREAM = "stream"
RAW = "raw"

#----------------------
# | ANIMATION_TABLE_WORDS
# | Description: s16 offsets leading an animation bank: 32 sequence offsets then 32 frame offsets (WMAP_ANIMATION_SEQUENCES); everything after them is byte data.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ANIMATION_TABLE_WORDS = 64

#----------------------
# | INTERFACE_CLUTS / INTERFACE_RAW8
# | Description: Palette runs (x, y, words) and 8bpp texel rectangles (x, y, w, h) in 0x10C4's VRAM page: the backdrop quads' palettes, a land palette row, and the 8bpp artifact icons (WMAP_ARTIFACT_TPAGE, g_wmap_artifact_images).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
INTERFACE_CLUTS = [(768, 248, 16), (768, 249, 16), (896, 254, 64)]
INTERFACE_RAW8 = [(896, 0, 128, 176)]

#----------------------
# | RESIDENT_CLUTS / RESIDENT_RAW8
# | Description: Palette runs and 8bpp rectangles in 0x10C6's VRAM page: the marker, spirit and label palette column at x 736, the carousel, label, glyph, error and artifact palette rows, and the 8bpp ring texture.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RESIDENT_CLUTS = [(736, y, 16) for y in range(352, 448)] + [
    (704, 505, 256), (752, 506, 16), (752, 507, 16), (720, 508, 48), (704, 509, 96), (704, 511, 256)]
RESIDENT_RAW8 = [(768, 256, 48, 224)]

#----------------------
# | LAND_IMAGE_WORDS / LAND_IMAGE_ROWS
# | Description: Size of one land image in the land image pack: the 64x64-word cache slot rectangle of D_800CBBE8 that wmap_map_display.c uploads raw.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LAND_IMAGE_WORDS = 64
LAND_IMAGE_ROWS = 64

#----------------------
# | LAND_IMAGE_CLUTS
# | Description: Palette rows inside each land image, relative to its slot: rows 62 and 63, four 16-colour palettes each (D_800CBBE8's clut words).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LAND_IMAGE_CLUTS = [(0, 62, 64), (0, 63, 64)]

#----------------------
# | LAND_ANIMATION_OFFSET
# | Description: Offset of a land's animation bank from its image, as wmap_map_display.c sets animation_data = image + 0x2004.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LAND_ANIMATION_OFFSET = 0x2004

#----------------------
# | LAYOUT_IMAGE_FIRST / LAYOUT_IMAGE_COUNT / LAYOUT_IMAGE_BYTES
# | Description: The per-cell layout images wmap_main.c streams as g_wmap_layout_id + 0x14DE: one 4bpp CLUT TIM of 73,792 bytes per WSEL cell.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LAYOUT_IMAGE_FIRST = 0x14DE
LAYOUT_IMAGE_COUNT = 361
LAYOUT_IMAGE_BYTES = 73792

#----------------------
# | SECTOR / LAYOUT_IMAGE_STRIDE_SECTORS
# | Description: CD sector size and the whole sectors each layout image takes in its pack; must match layout_pack.c's image pack stride.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECTOR = 2048
LAYOUT_IMAGE_STRIDE_SECTORS = 37


#----------------------
# | convert_tim_image
# | Description: Converts a whole-resource TIM through overlay_rules.convert_tim, keeping its raw header for the flag byte the code tests.
# | Author: suinevere
# | Dependencies: overlay_rules.convert_tim
# | Globals: N/A
# | Params: buf -- resource bytearray
# | Returns: buf
#----------------------
def convert_tim_image(buf):
    if len(buf) < 20 or buf[0] != 0x10:
        raise ResourceFormatError("not a TIM")
    if convert_tim(buf, 0) > len(buf):
        raise ResourceFormatError("TIM blocks run past the resource")
    return buf


#----------------------
# | convert_animation_bank
# | Description: Swaps an animation bank's 64 leading s16 offsets; its 4-byte step records and sprite or quad frames are bytes and stay raw.
# | Author: suinevere
# | Dependencies: overlay_rules.swap_words
# | Globals: ANIMATION_TABLE_WORDS
# | Params: buf -- resource bytearray
# | Returns: buf
#----------------------
def convert_animation_bank(buf):
    if len(buf) < ANIMATION_TABLE_WORDS * 2:
        raise ResourceFormatError("animation bank is shorter than its offset tables")
    swap_words(buf, 0, ANIMATION_TABLE_WORDS, 2)
    return buf


#----------------------
# | convert_model_pack
# | Description: Swaps a BTP model pack for wmap_draw_model: s32 count and offsets, then per model its s16 counts, s16 vertex fields and s16 face indices; face codes and UV/colour attributes are bytes.
# | Author: suinevere
# | Dependencies: overlay_rules.swap_words
# | Globals: N/A
# | Params: buf -- resource bytearray
# | Returns: buf
#----------------------
def convert_model_pack(buf):
    (count,) = struct.unpack_from("<i", buf, 0)
    if not 0 < count < 64 or 4 + count * 4 > len(buf):
        raise ResourceFormatError("model pack count %d is invalid" % count)
    offsets = struct.unpack_from("<%di" % count, buf, 4)
    swap_words(buf, 0, 1 + count, 4)
    for offset in sorted(set(offsets)):
        if offset < 4 + count * 4 or offset + 4 > len(buf):
            raise ResourceFormatError("model offset 0x%X is outside the pack" % offset)
        vertex_count, face_count = struct.unpack_from("<hh", buf, offset)
        faces = offset + 4 + vertex_count * 8
        end = faces + face_count * 26
        if vertex_count < 0 or face_count < 0 or end > len(buf):
            raise ResourceFormatError("model at 0x%X runs past the pack" % offset)
        swap_words(buf, offset, 2 + vertex_count * 4, 2)
        for i in range(face_count):
            swap_words(buf, faces + i * 10 + 2, 4, 2)
    return buf


#----------------------
# | convert_vram_words
# | Description: Converts a rectangle of raw VRAM words for vram_regions: palette words become big-endian Saturn colours, 8bpp texels stay as bytes, and every other word is 4bpp texels with each byte's nibbles swapped.
# | Author: suinevere
# | Dependencies: overlay_rules.ps1_to_saturn_color
# | Globals: N/A
# | Params: buf -- bytearray, offset -- first word, x, y -- VRAM origin, width, height -- size in words, cluts -- (x, y, words) runs, raw8 -- (x, y, w, h) rectangles
# | Returns: N/A
#----------------------
def convert_vram_words(buf, offset, x, y, width, height, cluts, raw8):
    for row in range(height):
        vy = y + row
        row_cluts = [(cx, cx + n) for cx, cy, n in cluts if cy == vy]
        row_raw8 = [(rx, rx + rw) for rx, ry, rw, rh in raw8 if ry <= vy < ry + rh]
        pos = offset + row * width * 2
        for col in range(width):
            vx = x + col
            if any(lo <= vx < hi for lo, hi in row_cluts):
                (value,) = struct.unpack_from("<H", buf, pos)
                struct.pack_into(">H", buf, pos, ps1_to_saturn_color(value))
            elif not any(lo <= vx < hi for lo, hi in row_raw8):
                for b in (pos, pos + 1):
                    buf[b] = ((buf[b] & 0x0F) << 4) | (buf[b] >> 4)
            pos += 2


#----------------------
# | convert_vram_page
# | Description: Converts a TIM flagged 16bpp without a CLUT that really holds a VRAM page of 4bpp and 8bpp texels and palettes; the header stays raw and the block header is swapped.
# | Author: suinevere
# | Dependencies: overlay_rules.convert_block_header, convert_vram_words
# | Globals: N/A
# | Params: buf -- resource bytearray, cluts -- palette runs, raw8 -- 8bpp rectangles
# | Returns: buf
#----------------------
def convert_vram_page(buf, cluts, raw8):
    if len(buf) < 20 or buf[0] != 0x10 or buf[4] != 2:
        raise ResourceFormatError("VRAM page is not a 16bpp TIM without a CLUT")
    _bnum, x, y, width, height = struct.unpack_from("<IHHHH", buf, 8)
    if 20 + width * height * 2 > len(buf):
        raise ResourceFormatError("VRAM page pixels run past the resource")
    convert_block_header(buf, 8)
    convert_vram_words(buf, 20, x, y, width, height, cluts, raw8)
    return buf


#----------------------
# | convert_interface_page
# | Description: Converts 0x10C4, the interface VRAM page at (704, 0).
# | Author: suinevere
# | Dependencies: convert_vram_page
# | Globals: INTERFACE_CLUTS, INTERFACE_RAW8
# | Params: buf -- decompressed resource bytearray
# | Returns: buf
#----------------------
def convert_interface_page(buf):
    return convert_vram_page(buf, INTERFACE_CLUTS, INTERFACE_RAW8)


#----------------------
# | convert_resident_page
# | Description: Converts 0x10C6, the resident VRAM page at (704, 256).
# | Author: suinevere
# | Dependencies: convert_vram_page
# | Globals: RESIDENT_CLUTS, RESIDENT_RAW8
# | Params: buf -- decompressed resource bytearray
# | Returns: buf
#----------------------
def convert_resident_page(buf):
    return convert_vram_page(buf, RESIDENT_CLUTS, RESIDENT_RAW8)


#----------------------
# | convert_land_image_pack
# | Description: Converts 0x10C5: its s32 offset table (-1 for no land, repeats for shared lands), then each distinct land's 64x64-word image with its palette rows and the animation bank after it.
# | Author: suinevere
# | Dependencies: overlay_rules.swap_words, convert_vram_words
# | Globals: LAND_IMAGE_WORDS, LAND_IMAGE_ROWS, LAND_IMAGE_CLUTS, LAND_ANIMATION_OFFSET, ANIMATION_TABLE_WORDS
# | Params: buf -- decompressed resource bytearray
# | Returns: buf
#----------------------
def convert_land_image_pack(buf):
    (first,) = struct.unpack_from("<i", buf, 0)
    if first < 4 or first % 4 != 0 or first > len(buf):
        raise ResourceFormatError("land image pack table size 0x%X is invalid" % first)
    offsets = struct.unpack_from("<%di" % (first // 4), buf, 0)
    swap_words(buf, 0, first // 4, 4)
    need = LAND_ANIMATION_OFFSET + ANIMATION_TABLE_WORDS * 2
    for offset in sorted({o for o in offsets if o >= 0}):
        if offset < first or offset + need > len(buf):
            raise ResourceFormatError("land image at 0x%X runs past the pack" % offset)
        convert_vram_words(buf, offset, 0, 0, LAND_IMAGE_WORDS, LAND_IMAGE_ROWS, LAND_IMAGE_CLUTS, [])
        swap_words(buf, offset + LAND_ANIMATION_OFFSET, ANIMATION_TABLE_WORDS, 2)
    return buf


#----------------------
# | pack_layout_images
# | Description: Converts each per-cell layout image and pads it to LAYOUT_IMAGE_STRIDE_SECTORS sectors, concatenated in cell order, for the one exported image pack.
# | Author: suinevere
# | Dependencies: convert_tim_image
# | Globals: LAYOUT_IMAGE_COUNT, LAYOUT_IMAGE_STRIDE_SECTORS, SECTOR
# | Params: read_image -- cell to decompressed image bytes or None, count -- cells to pack
# | Returns: packed bytes
#----------------------
def pack_layout_images(read_image, count=LAYOUT_IMAGE_COUNT):
    stride = LAYOUT_IMAGE_STRIDE_SECTORS * SECTOR
    out = bytearray()
    for cell in range(count):
        data = read_image(cell)
        if data is None:
            raise ResourceFormatError("layout image %d is missing" % cell)
        if len(data) > stride:
            raise ResourceFormatError("layout image %d is 0x%X bytes, its stride holds 0x%X" % (cell, len(data), stride))
        out += convert_tim_image(bytearray(data)) + bytes(stride - len(data))
    return bytes(out)


#----------------------
# | RULES
# | Description: Home-path resource id to (kind, convert): the interface, land and resident images, land and party animation banks, party sprites, loading, placement, help and tutorial images, and Home's MHM effect set.
# | Author: suinevere
# | Dependencies: convert_tim_image, convert_animation_bank, convert_model_pack, convert_interface_page, convert_resident_page, convert_land_image_pack
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RULES = {0x10C4: (STREAM, convert_interface_page), 0x10C5: (STREAM, convert_land_image_pack),
         0x10C6: (STREAM, convert_resident_page), 0x10C7: (RAW, convert_animation_bank),
         0x10C8: (RAW, convert_tim_image), 0x10C9: (RAW, convert_animation_bank),
         0x10CA: (RAW, convert_tim_image), 0x10CB: (RAW, convert_tim_image),
         0x10CC: (RAW, convert_animation_bank), 0x10CD: (RAW, convert_animation_bank)}
RULES.update({res_id: (RAW, convert_tim_image) for res_id in range(0x10CE, 0x10D8)})
RULES.update({res_id: (RAW, convert_tim_image) for res_id in range(0x114B, 0x1154)})
RULES.update({res_id: (RAW, convert_tim_image) for res_id in range(0x11B0, 0x11B3)})
RULES.update({res_id: (RAW, convert_animation_bank) for res_id in range(0x11B3, 0x11B7)})
RULES.update({res_id: (RAW, convert_model_pack) for res_id in range(0x11B7, 0x11BA)})
RULES.update({res_id: (RAW, convert_tim_image) for res_id in range(0x1453, 0x145D)})

#----------------------
# | UNBUFFERED
# | Description: RULES ids read into overlay-local tables (D_800DBE98, D_800DC298, D_800DC698) rather than a placed wmap_memory buffer.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
UNBUFFERED = {0x10C9, 0x10CC, 0x10CD}

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
