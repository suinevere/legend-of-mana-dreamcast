#----------------------
# | field_map.py
# | Description: Converts one FIELD SKmapDat map file so the big-endian Saturn reads it in place: swaps struct fields, relocates pointers and converts the CLUT.
# | Author: suinevere
# | Dependencies: overlay_rules.py
# | Globals: MAP_MAGIC, MAP_FORMAT_VERSIONS, MAP_TRAILERS, PS1_MAP_BASE, SATURN_MAP_BASE, MAP_FIELDS, MAP_BYTES, STRUCT_SIZES, NODE_RUN_COUNT_MASK, SPAN_COUNT_MASK, TILE_ANIM_LISTS, TILE_ANIM_KINDS, ANIM_KIND_MASK, CLUT_ROW_COLORS
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

from overlay_rules import ps1_to_saturn_color

#----------------------
# | MAP_MAGIC
# | Description: The eight bytes every map file starts with.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAP_MAGIC = b"SKmapDat"

#----------------------
# | MAP_FORMAT_VERSIONS
# | Description: The format_version values (u16 at 0x08) whose layout this converter knows: 0xF (scene 13's map 191) and 0x13 (Home's map 195).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAP_FORMAT_VERSIONS = (0xF, 0x13)

#----------------------
# | MAP_TRAILERS
# | Description: Allowed byte lengths of the block after the texels that no FIELD code reads, per format_version: map 191's 0x370, map 195's 0x599 and map 578's 0x51A.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAP_TRAILERS = {0xF: (0x370,), 0x13: (0x599, 0x51A)}

#----------------------
# | PS1_MAP_BASE
# | Description: PS1 address the map is loaded to; every non-null pointer in the file is relative to it.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PS1_MAP_BASE = 0x80180000

#----------------------
# | SATURN_MAP_BASE
# | Description: Default Saturn load address of the map, the start of lom_load_buffer.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SATURN_MAP_BASE = 0x00280000

#----------------------
# | MAP_FIELDS
# | Description: Swapped fields per struct as (offset, width, kind), from FieldResource/FieldMapHeader, FieldMapObject, FieldObjDef, FieldPartDef and the tint block.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAP_FIELDS = {
    "header": (
        (0x08, 2, "u16"),
        (0x0C, 4, "u32"),
        (0x10, 4, "u32"),
        (0x14, 4, "ptr"),
        (0x18, 4, "ptr"),
        (0x1C, 4, "ptr"),
        (0x20, 4, "ptr"),
    ),
    "object": (
        (0x00, 4, "ptr"),
        (0x04, 4, "ptr"),
        (0x08, 4, "ptr"),
        (0x0C, 4, "ptr"),
        (0x10, 4, "ptr"),
        (0x14, 4, "ptr"),
        (0x18, 4, "ptr"),
        (0x1C, 4, "ptr"),
        (0x20, 4, "ptr"),
        (0x26, 2, "u16"),
        (0x28, 2, "u16"),
        (0x2A, 2, "u16"),
        (0x2C, 4, "union32"),
        (0x30, 2, "u16"),
        (0x32, 2, "u16"),
    ),
    "obj_def": (
        (0x00, 4, "ptr"),
        (0x04, 4, "ptr"),
        (0x0C, 4, "union32"),
        (0x10, 2, "u16"),
        (0x12, 2, "u16"),
        (0x14, 2, "u16"),
        (0x16, 2, "u16"),
        (0x18, 2, "u16"),
        (0x1A, 2, "u16"),
        (0x1C, 4, "union32"),
    ),
    "part_def": (
        (0x00, 4, "ptr"),
        (0x08, 4, "union32"),
        (0x0C, 2, "u16"),
        (0x0E, 2, "u16"),
        (0x10, 2, "u16"),
        (0x12, 2, "u16"),
        (0x18, 2, "u16"),
        (0x1A, 2, "u16"),
        (0x1C, 2, "u16"),
        (0x1E, 2, "u16"),
    ),
    "tint": (
        (0x00, 2, "u16"),
        (0x02, 2, "u16"),
    ),
    "node_def": (
        (0x00, 4, "ptr"),
        (0x04, 4, "union32"),
        (0x0A, 2, "u16"),
        (0x0C, 2, "u16"),
        (0x0E, 2, "u16"),
        (0x10, 2, "u16"),
        (0x12, 2, "u16"),
        (0x14, 2, "u16"),
    ),
    "edge_def": (
        (0x00, 4, "ptr"),
        (0x04, 2, "u16"),
        (0x06, 2, "u16"),
        (0x08, 2, "u16"),
        (0x0A, 2, "u16"),
        (0x0C, 2, "u16"),
        (0x0E, 2, "u16"),
        (0x10, 2, "u16"),
        (0x12, 2, "u16"),
        (0x14, 2, "u16"),
    ),
    "header_rec": (
        (0x00, 4, "ptr"),
        (0x04, 2, "u16"),
        (0x06, 2, "u16"),
        (0x08, 2, "u16"),
    ),
    "anim_def": (
        (0x02, 2, "u16"),
        (0x04, 4, "union32"),
        (0x08, 4, "ptr"),
        (0x10, 4, "ptr"),
        (0x14, 4, "ptr"),
    ),
}

#----------------------
# | MAP_BYTES
# | Description: Byte spans per struct as (offset, width) that are declared u8 (magic, padding, a spawn record's facing and flags) and stay as they are.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAP_BYTES = {
    "header": ((0x00, 8), (0x0A, 2)),
    "object": ((0x24, 2),),
    "obj_def": ((0x08, 4),),
    "part_def": ((0x04, 4), (0x14, 4)),
    "tint": (),
    "node_def": ((0x08, 2), (0x16, 2)),
    "edge_def": ((0x16, 2),),
    "header_rec": ((0x0A, 2),),
    "anim_def": ((0x00, 2), (0x0C, 4)),
}

#----------------------
# | STRUCT_SIZES
# | Description: Byte size of each struct MAP_FIELDS describes; the tint block's colours follow its four-byte head.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STRUCT_SIZES = {"header": 0x24, "object": 0x34, "obj_def": 0x20, "part_def": 0x20, "tint": 0x04, "node_def": 0x18,
                "edge_def": 0x18, "header_rec": 0x0C, "anim_def": 0x18}

#----------------------
# | NODE_RUN_COUNT_MASK
# | Description: FieldNodeRun::count bits holding the number of points; a run whose count is zero ends a node's run list.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
NODE_RUN_COUNT_MASK = 0x7FFF

#----------------------
# | SPAN_COUNT_MASK
# | Description: FieldTweenSpan::count bits holding the keyframes a span covers (FIELD_SPAN_COUNT_MASK).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SPAN_COUNT_MASK = 0x7F

#----------------------
# | TILE_ANIM_LISTS
# | Description: Animation lists whose definitions use the tile view: a part grid and frame_count frames of rect_width * rect_height tile descriptors.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TILE_ANIM_LISTS = (0, 3)

#----------------------
# | TILE_ANIM_KINDS
# | Description: Handler kinds (FieldAnimDefFlags low three bits) whose frames are tile descriptors; uploads, movies, tweens and sound keys need their own rules.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TILE_ANIM_KINDS = (0, 1, 2)

#----------------------
# | ANIM_KIND_MASK
# | Description: FieldAnimDefFlags bits holding the handler kind.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ANIM_KIND_MASK = 0x7

#----------------------
# | CLUT_ROW_COLORS
# | Description: Colours in one row of an object's CLUT image (FIELD_OBJECT_IMAGE_WIDTH).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CLUT_ROW_COLORS = 256


#----------------------
# | MapFormatError
# | Description: Raised when a map file is not a format-0xF SKmapDat file this converter can fully account for.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class MapFormatError(Exception):
    pass


#----------------------
# | _MapConverter
# | Description: Walks one map from its header, writing converted fields into a copy and recording which bytes a rule touched.
# | Author: suinevere
# | Dependencies: MAP_FIELDS, MAP_BYTES, STRUCT_SIZES
# | Globals: PS1_MAP_BASE
# | Params: N/A
# | Returns: N/A
#----------------------
class _MapConverter:
    #----------------------
    # | __init__
    # | Description: Keeps the source bytes, an output copy, the touched mask and the set of structs already converted.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: data -- PS1 map bytes, load_base -- Saturn load address
    # | Returns: N/A
    #----------------------
    def __init__(self, data, load_base):
        self.data = data
        self.out = bytearray(data)
        self.touched = bytearray(len(data))
        self.load_base = load_base
        self.seen = set()
        self.point_count = 0

    #----------------------
    # | touch
    # | Description: Marks a byte range as handled; a range outside the file or handled twice raises MapFormatError.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: offset -- start byte, length -- byte count
    # | Returns: N/A
    #----------------------
    def touch(self, offset, length):
        if offset < 0 or offset + length > len(self.data):
            raise MapFormatError("range 0x%X+0x%X is outside the 0x%X-byte map" % (offset, length, len(self.data)))
        if any(self.touched[offset:offset + length]):
            raise MapFormatError("range 0x%X+0x%X is converted twice" % (offset, length))
        self.touched[offset:offset + length] = b"\x01" * length

    #----------------------
    # | read
    # | Description: Reads a little-endian u16 or u32 from the source map.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: offset -- byte offset, width -- 2 or 4
    # | Returns: unsigned value
    #----------------------
    def read(self, offset, width):
        return struct.unpack_from("<H" if width == 2 else "<I", self.data, offset)[0]

    #----------------------
    # | target
    # | Description: Turns a PS1 pointer into a file offset, or None for null; a pointer outside the map raises MapFormatError.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: PS1_MAP_BASE
    # | Params: value -- PS1 pointer, slot -- file offset it was read from
    # | Returns: file offset or None
    #----------------------
    def target(self, value, slot):
        if value == 0:
            return None
        if not PS1_MAP_BASE <= value < PS1_MAP_BASE + len(self.data):
            raise MapFormatError("pointer 0x%08X at 0x%X is outside the map" % (value, slot))
        return value - PS1_MAP_BASE

    #----------------------
    # | field
    # | Description: Converts one field: u16 and u32/union32 are swapped, ptr is relocated to the Saturn base.
    # | Author: suinevere
    # | Dependencies: touch, read, target
    # | Globals: N/A
    # | Params: offset -- file offset, width -- 2 or 4, kind -- u16, u32, ptr or union32
    # | Returns: the field's source value, or for ptr its file offset (None when null)
    #----------------------
    def field(self, offset, width, kind):
        self.touch(offset, width)
        value = self.read(offset, width)
        if kind == "ptr":
            pos = self.target(value, offset)
            struct.pack_into(">I", self.out, offset, 0 if pos is None else self.load_base + pos)
            return pos
        struct.pack_into(">H" if width == 2 else ">I", self.out, offset, value)
        return value

    #----------------------
    # | convert_struct
    # | Description: Converts every field of one struct and marks its byte spans; a struct already converted is skipped.
    # | Author: suinevere
    # | Dependencies: field, touch, MAP_FIELDS, MAP_BYTES
    # | Globals: N/A
    # | Params: name -- MAP_FIELDS key, base -- file offset of the struct
    # | Returns: dict of field offset to converted value, or None when already converted
    #----------------------
    def convert_struct(self, name, base):
        if (name, base) in self.seen:
            return None
        self.seen.add((name, base))
        for offset, width in MAP_BYTES[name]:
            self.touch(base + offset, width)
        return {offset: self.field(base + offset, width, kind) for offset, width, kind in MAP_FIELDS[name]}

    #----------------------
    # | pointer_array
    # | Description: Relocates a null-terminated pointer array, including the terminator, and returns the file offsets it lists.
    # | Author: suinevere
    # | Dependencies: field
    # | Globals: N/A
    # | Params: base -- file offset of the array
    # | Returns: list of file offsets
    #----------------------
    def pointer_array(self, base):
        if ("array", base) in self.seen:
            return []
        self.seen.add(("array", base))
        targets = []
        slot = base
        while True:
            pos = self.field(slot, 4, "ptr")
            if pos is None:
                return targets
            targets.append(pos)
            slot += 4

    #----------------------
    # | require_null
    # | Description: Raises MapFormatError when a list this converter cannot walk is present.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: pos -- file offset or None, what -- name for the message
    # | Returns: N/A
    #----------------------
    def require_null(self, pos, what):
        if pos is not None:
            raise MapFormatError("%s at 0x%X is not supported" % (what, pos))

    #----------------------
    # | convert_tint
    # | Description: Converts the tint block a FieldObjDef shares: u16 count, u16, then count four-byte colours left as bytes.
    # | Author: suinevere
    # | Dependencies: convert_struct, touch, STRUCT_SIZES
    # | Globals: N/A
    # | Params: base -- file offset of the block
    # | Returns: N/A
    #----------------------
    def convert_tint(self, base):
        fields = self.convert_struct("tint", base)
        if fields is not None:
            self.touch(base + STRUCT_SIZES["tint"], fields[0x00] * 4)

    #----------------------
    # | convert_part_def
    # | Description: Converts a FieldPartDef and marks its cols * rows tile descriptors as left as bytes.
    # | Author: suinevere
    # | Dependencies: convert_struct, touch
    # | Globals: N/A
    # | Params: base -- file offset of the definition
    # | Returns: N/A
    #----------------------
    def convert_part_def(self, base):
        fields = self.convert_struct("part_def", base)
        if fields is None or fields[0x00] is None:
            return
        word = fields[0x08]
        cols, rows = (word >> 16) & 0xFF, (word >> 24) & 0xFF
        self.touch(fields[0x00], cols * rows * 4)

    #----------------------
    # | convert_obj_def
    # | Description: Converts a FieldObjDef, its part definition list and its tint block.
    # | Author: suinevere
    # | Dependencies: convert_struct, pointer_array, convert_part_def, convert_tint
    # | Globals: N/A
    # | Params: base -- file offset of the definition
    # | Returns: N/A
    #----------------------
    def convert_obj_def(self, base):
        fields = self.convert_struct("obj_def", base)
        if fields is None:
            return
        if fields[0x00] is not None:
            for pos in self.pointer_array(fields[0x00]):
                self.convert_part_def(pos)
        if fields[0x04] is not None:
            self.convert_tint(fields[0x04])

    #----------------------
    # | convert_clut
    # | Description: Converts an object's CLUT image, image_size >> 8 rows of 256 colours plus image_size & 0xFF, with ps1_to_saturn_color.
    # | Author: suinevere
    # | Dependencies: touch, read, ps1_to_saturn_color
    # | Globals: CLUT_ROW_COLORS
    # | Params: base -- file offset of the image, image_size -- FieldMapObject::image_size
    # | Returns: N/A
    #----------------------
    def convert_clut(self, base, image_size):
        if ("clut", base) in self.seen:
            return
        self.seen.add(("clut", base))
        count = (image_size >> 8) * CLUT_ROW_COLORS + (image_size & 0xFF)
        self.touch(base, count * 2)
        for i in range(count):
            struct.pack_into(">H", self.out, base + i * 2, ps1_to_saturn_color(self.read(base + i * 2, 2)))

    #----------------------
    # | convert_list
    # | Description: Converts every struct of a singly linked list whose next pointer is at offset 0x00, returning each struct's fields.
    # | Author: suinevere
    # | Dependencies: convert_struct
    # | Globals: N/A
    # | Params: name -- MAP_FIELDS key, pos -- file offset of the head or None
    # | Returns: list of (file offset, fields) pairs
    #----------------------
    def convert_list(self, name, pos):
        items = []
        while pos is not None:
            fields = self.convert_struct(name, pos)
            if fields is None:
                break
            items.append((pos, fields))
            pos = fields[0x00]
        return items

    #----------------------
    # | convert_node_defs
    # | Description: Converts a node definition list and each node's point runs up to its zero-count terminator, noting the highest point index the runs use.
    # | Author: suinevere
    # | Dependencies: convert_list, field, STRUCT_SIZES
    # | Globals: NODE_RUN_COUNT_MASK
    # | Params: pos -- file offset of the first FieldNodeDef or None
    # | Returns: N/A
    #----------------------
    def convert_node_defs(self, pos):
        for base, _ in self.convert_list("node_def", pos):
            run = base + STRUCT_SIZES["node_def"]
            while True:
                count = self.field(run, 2, "u16") & NODE_RUN_COUNT_MASK
                first = self.field(run + 2, 2, "u16")
                run += 4
                if count == 0:
                    break
                self.point_count = max(self.point_count, first + count)

    #----------------------
    # | convert_points
    # | Description: Converts the header's point table: point_count (x, y) s16 pairs, as many as the node runs index.
    # | Author: suinevere
    # | Dependencies: field
    # | Globals: N/A
    # | Params: pos -- file offset of the table or None
    # | Returns: N/A
    #----------------------
    def convert_points(self, pos):
        if pos is None:
            return
        if self.point_count == 0:
            raise MapFormatError("point table at 0x%X but no node indexes it" % pos)
        for i in range(self.point_count * 2):
            self.field(pos + i * 2, 2, "u16")

    #----------------------
    # | convert_anim_defs
    # | Description: Converts one animation list: each definition, the count-table spans past its head until they cover last_frame, its part grid and its tile-descriptor frames.
    # | Author: suinevere
    # | Dependencies: convert_struct, field, touch, convert_part_def, STRUCT_SIZES
    # | Globals: SPAN_COUNT_MASK, TILE_ANIM_LISTS, TILE_ANIM_KINDS, ANIM_KIND_MASK
    # | Params: pos -- file offset of the first FieldAnimDef or None, group -- animation list index 0-3
    # | Returns: N/A
    #----------------------
    def convert_anim_defs(self, pos, group):
        while pos is not None:
            fields = self.convert_struct("anim_def", pos)
            if fields is None:
                return
            if group not in TILE_ANIM_LISTS:
                raise MapFormatError("animation list %d at 0x%X is not supported" % (group, pos))
            flags = fields[0x04]
            if flags & ANIM_KIND_MASK not in TILE_ANIM_KINDS:
                raise MapFormatError("animation kind %d at 0x%X is not supported" % (flags & ANIM_KIND_MASK, pos))
            last_frame, frame_count = (flags >> 8) & 0xFF, (flags >> 16) & 0xFF
            covered = self.data[pos] & SPAN_COUNT_MASK
            span = pos + STRUCT_SIZES["anim_def"]
            while covered <= last_frame:
                self.touch(span, 2)
                self.field(span + 2, 2, "u16")
                covered += self.data[span] & SPAN_COUNT_MASK
                span += 4
            if fields[0x10] is not None:
                self.convert_part_def(fields[0x10])
            if fields[0x14] is not None:
                width, height = self.data[pos + 0x0E], self.data[pos + 0x0F]
                self.touch(fields[0x14], frame_count * width * height * 4)
            pos = fields[0x08]

    #----------------------
    # | convert_object
    # | Description: Converts a FieldMapObject with its header records, node and edge definitions, four animation lists, object definitions and CLUT image.
    # | Author: suinevere
    # | Dependencies: convert_struct, convert_list, convert_node_defs, convert_anim_defs, pointer_array, convert_obj_def, convert_clut
    # | Globals: N/A
    # | Params: base -- file offset of the map object
    # | Returns: N/A
    #----------------------
    def convert_object(self, base):
        fields = self.convert_struct("object", base)
        if fields is None:
            return
        self.convert_list("header_rec", fields[0x10])
        self.convert_node_defs(fields[0x08])
        self.convert_list("edge_def", fields[0x0C])
        for group in range(4):
            self.convert_anim_defs(fields[0x14 + group * 4], group)
        if fields[0x00] is not None:
            for pos in self.pointer_array(fields[0x00]):
                self.convert_obj_def(pos)
        if fields[0x04] is not None:
            self.convert_clut(fields[0x04], fields[0x28])

    #----------------------
    # | convert
    # | Description: Checks the magic and version, converts the header, objects and texels, then checks every byte was handled.
    # | Author: suinevere
    # | Dependencies: convert_struct, pointer_array, convert_object, require_null, touch, check_coverage
    # | Globals: MAP_MAGIC, MAP_FORMAT_VERSIONS
    # | Params: N/A
    # | Returns: converted bytes
    #----------------------
    def convert(self):
        if len(self.data) < STRUCT_SIZES["header"] or self.data[:8] != MAP_MAGIC:
            raise MapFormatError("not an SKmapDat map")
        version = self.read(0x08, 2)
        if version not in MAP_FORMAT_VERSIONS:
            raise MapFormatError("format_version 0x%X is not one of %s" % (version, ", ".join("0x%X" % v for v in MAP_FORMAT_VERSIONS)))
        header = self.convert_struct("header", 0)
        self.require_null(header[0x18], "sequence table")
        if header[0x20] is not None:
            for pos in self.pointer_array(header[0x20]):
                self.convert_object(pos)
        self.convert_points(header[0x1C])
        texel_end = 0
        if header[0x14] is not None:
            self.touch(header[0x14], header[0x0C])
            texel_end = header[0x14] + header[0x0C]
        self.check_coverage(texel_end, MAP_TRAILERS[version])
        return bytes(self.out)

    #----------------------
    # | check_coverage
    # | Description: Raises MapFormatError listing every byte range no rule touched, allowing only an unread trailer of one of the format's known lengths after the texels.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: texel_end -- file offset just past the texels, or 0 when the map has none; trailer_sizes -- MAP_TRAILERS lengths for the format
    # | Returns: N/A
    #----------------------
    def check_coverage(self, texel_end, trailer_sizes):
        mask = bytearray(self.touched)
        if 0 < texel_end < len(mask) and len(mask) - texel_end in trailer_sizes:
            if any(mask[texel_end:]):
                raise MapFormatError("trailer at 0x%X is read by a converted structure" % texel_end)
            mask[texel_end:] = b"\x01" * (len(mask) - texel_end)
        gaps = []
        pos = 0
        while pos < len(mask):
            if mask[pos]:
                pos += 1
                continue
            start = pos
            while pos < len(mask) and not mask[pos]:
                pos += 1
            gaps.append("0x%X..0x%X" % (start, pos))
        if gaps:
            raise MapFormatError("bytes no rule touched: " + ", ".join(gaps))


#----------------------
# | convert_map
# | Description: Converts one PS1 map file for the Saturn load buffer; the result has the input's length.
# | Author: suinevere
# | Dependencies: _MapConverter
# | Globals: SATURN_MAP_BASE
# | Params: data -- PS1 map bytes, load_base -- Saturn address the map is read to
# | Returns: converted bytes; raises MapFormatError for a map it cannot fully account for
#----------------------
def convert_map(data, load_base=SATURN_MAP_BASE):
    return _MapConverter(bytes(data), load_base).convert()


#----------------------
# | _AnimCensus
# | Description: Walks a map like _MapConverter but only records each animation definition, of any kind and in any list, without converting or checking coverage.
# | Author: suinevere
# | Dependencies: _MapConverter, ANIM_KIND_MASK
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class _AnimCensus(_MapConverter):
    #----------------------
    # | __init__
    # | Description: Starts an empty list of definitions.
    # | Author: suinevere
    # | Dependencies: _MapConverter.__init__
    # | Globals: SATURN_MAP_BASE
    # | Params: data -- PS1 map bytes
    # | Returns: N/A
    #----------------------
    def __init__(self, data):
        _MapConverter.__init__(self, data, SATURN_MAP_BASE)
        self.anims = []

    #----------------------
    # | convert_anim_defs
    # | Description: Records (offset, list, kind, byte 1 of the head, flags) for each definition in one animation list.
    # | Author: suinevere
    # | Dependencies: convert_struct, ANIM_KIND_MASK
    # | Globals: N/A
    # | Params: pos -- file offset of the first FieldAnimDef or None, group -- animation list index 0-3
    # | Returns: N/A
    #----------------------
    def convert_anim_defs(self, pos, group):
        while pos is not None:
            fields = self.convert_struct("anim_def", pos)
            if fields is None:
                return
            self.anims.append((pos, group, fields[0x04] & ANIM_KIND_MASK, self.data[pos + 1], fields[0x04]))
            pos = fields[0x08]

    #----------------------
    # | check_coverage
    # | Description: A census reads only what it needs, so nothing is checked.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: texel_end, trailer_sizes -- ignored
    # | Returns: N/A
    #----------------------
    def check_coverage(self, texel_end, trailer_sizes):
        pass


#----------------------
# | anim_defs
# | Description: Lists every animation definition in a PS1 map, movies included.
# | Author: suinevere
# | Dependencies: _AnimCensus
# | Globals: N/A
# | Params: data -- PS1 map bytes
# | Returns: list of (offset, list, kind, head byte 1, flags)
#----------------------
def anim_defs(data):
    census = _AnimCensus(bytes(data))
    census.convert()
    return census.anims
