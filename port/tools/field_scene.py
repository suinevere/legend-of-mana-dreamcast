#----------------------
# | field_scene.py
# | Description: Converts one FIELD scene file so the big-endian Saturn reads it in place: swaps the fields the scene-entry path reads, converts actor TIMs and leaves the rest raw.
# | Author: suinevere
# | Dependencies: overlay_rules.py
# | Globals: SATURN_SCENE_BASE, HEADER_SIZE, SCENE_SECTIONS, LAYOUT_RECORD_FIELDS, LAYOUT_RECORD_BYTES, LAYOUT_RECORD_SIZE, ACTOR_IMAGE_COUNT, ACTOR_IMAGE_END, PORTRAIT_SIZE, TIM_MAGIC
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

from overlay_rules import ps1_to_saturn_color, swap_nibbles

#----------------------
# | SATURN_SCENE_BASE
# | Description: Default Saturn load address of a scene file, lom_load_buffer + 0x10000.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SATURN_SCENE_BASE = 0x00290000

#----------------------
# | HEADER_SIZE
# | Description: Bytes of the scene header: ten s32 section offsets from the start of the file.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HEADER_SIZE = 0x28

#----------------------
# | SCENE_SECTIONS
# | Description: The ten header entries in file order as (entry offset, name, rule, reason).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SCENE_SECTIONS = (
    (0x00, "layout", "layout", "count and records read by field_load_scene_actors and field_install_actor_action"),
    (0x04, "event_scripts", "script_table", "u16 offset table read by field_get_event_script; the script VM reads the bytecode bytewise"),
    (0x08, "strings", "strings", "u16 offset table read by FIELD_SCENE_STRING when a text window opens; the string bytes are read bytewise"),
    (0x0C, "actor_scripts", "script_table", "u16 offset table read by field_get_object_script_command; the actor commands are read bytewise"),
    (0x10, "records", "records", "offset table and record ids read by field_find_resource on scene entry (field_classify_menu_slots); payloads are read by script commands"),
    (0x14, "actors", "actors", "head, description offsets and descriptions read by field_update_scene"),
    (0x18, "geometry", "geometry", "offset table read by field_update_scene; the body is copied as words and read byte by byte"),
    (0x1C, "images", "images", "offset table read by field_update_scene, TIMs read by field_upload_actor_image"),
    (0x20, "portraits", "portraits", "count read by field_update_scene; the portraits are not read on scene entry"),
    (0x24, "group_bounds", "group_bounds", "count and bounds copied as u32 by field_update_scene"),
)

#----------------------
# | LAYOUT_RECORD_SIZE / LAYOUT_RECORD_FIELDS / LAYOUT_RECORD_BYTES
# | Description: FieldLayoutRecord size, its swapped fields as (offset, width) with control and position as u32 unions, and its u8 condition bytes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LAYOUT_RECORD_SIZE = 0x30
LAYOUT_RECORD_FIELDS = ((0x00, 4), (0x04, 2), (0x08, 4), (0x0C, 2), (0x0E, 2)) + tuple((0x10 + i * 2, 2) for i in range(16))
LAYOUT_RECORD_BYTES = ((0x06, 2),)

#----------------------
# | ACTOR_IMAGE_COUNT / ACTOR_IMAGE_END
# | Description: Most image indices an actor description lists, and the index that ends a shorter list.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ACTOR_IMAGE_COUNT = 6
ACTOR_IMAGE_END = 0xFF

#----------------------
# | PORTRAIT_SIZE
# | Description: Bytes of one scene portrait (FIELD_SCENE_PORTRAIT_SIZE).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PORTRAIT_SIZE = 1184

#----------------------
# | TIM_MAGIC
# | Description: First word of a TIM file.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TIM_MAGIC = 0x10


#----------------------
# | SceneFormatError
# | Description: Raised when a scene file is truncated, malformed or holds bytes this converter cannot account for.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class SceneFormatError(Exception):
    pass


#----------------------
# | _SceneConverter
# | Description: Walks one scene file section by section, writing converted fields into a copy and recording which bytes a rule touched.
# | Author: suinevere
# | Dependencies: SCENE_SECTIONS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class _SceneConverter:
    #----------------------
    # | __init__
    # | Description: Keeps the source bytes, an output copy, the touched mask and the set of blocks already converted.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: data -- PS1 scene bytes
    # | Returns: N/A
    #----------------------
    def __init__(self, data):
        self.data = data
        self.out = bytearray(data)
        self.touched = bytearray(len(data))
        self.seen = set()
        self.bounds = {}
        self.actor_count = 0

    #----------------------
    # | touch
    # | Description: Marks a byte range as handled; a range outside the file or handled twice raises SceneFormatError.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: offset -- start byte, length -- byte count
    # | Returns: N/A
    #----------------------
    def touch(self, offset, length):
        if length < 0 or offset < 0 or offset + length > len(self.data):
            raise SceneFormatError("range 0x%X+0x%X is outside the 0x%X-byte scene" % (offset, length, len(self.data)))
        if any(self.touched[offset:offset + length]):
            raise SceneFormatError("range 0x%X+0x%X is converted twice" % (offset, length))
        self.touched[offset:offset + length] = b"\x01" * length

    #----------------------
    # | read
    # | Description: Reads a little-endian u16 or u32 from the source scene; a read past the end raises SceneFormatError.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: offset -- byte offset, width -- 2 or 4
    # | Returns: unsigned value
    #----------------------
    def read(self, offset, width):
        if offset < 0 or offset + width > len(self.data):
            raise SceneFormatError("read 0x%X+%d is outside the 0x%X-byte scene" % (offset, width, len(self.data)))
        return struct.unpack_from("<H" if width == 2 else "<I", self.data, offset)[0]

    #----------------------
    # | field
    # | Description: Swaps one u16 or u32 field (a union word included) to big-endian and marks it touched.
    # | Author: suinevere
    # | Dependencies: touch, read
    # | Globals: N/A
    # | Params: offset -- file offset, width -- 2 or 4
    # | Returns: the field's source value
    #----------------------
    def field(self, offset, width):
        value = self.read(offset, width)
        self.touch(offset, width)
        struct.pack_into(">H" if width == 2 else ">I", self.out, offset, value)
        return value

    #----------------------
    # | inside
    # | Description: Raises SceneFormatError unless offset..offset+length lies inside the named section.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: name -- section name, offset -- start byte, length -- byte count
    # | Returns: N/A
    #----------------------
    def inside(self, name, offset, length):
        start, end = self.bounds[name]
        if length < 0 or offset < start or offset + length > end:
            raise SceneFormatError("0x%X+0x%X is outside the %s section 0x%X..0x%X" % (offset, length, name, start, end))

    #----------------------
    # | read_header
    # | Description: Converts the ten section offsets and derives each section's bounds; offsets must be in file order and inside the file.
    # | Author: suinevere
    # | Dependencies: field, SCENE_SECTIONS
    # | Globals: HEADER_SIZE
    # | Params: N/A
    # | Returns: N/A
    #----------------------
    def read_header(self):
        if len(self.data) < HEADER_SIZE:
            raise SceneFormatError("0x%X bytes is shorter than the scene header" % len(self.data))
        starts = [self.field(entry, 4) for entry, _, _, _ in SCENE_SECTIONS]
        ends = starts[1:] + [len(self.data)]
        previous = HEADER_SIZE
        for (_, name, _, _), start, end in zip(SCENE_SECTIONS, starts, ends):
            if start < previous or start > len(self.data):
                raise SceneFormatError("%s offset 0x%X is out of order or past the 0x%X-byte scene" % (name, start, len(self.data)))
            previous = start
            self.bounds[name] = (start, end)

    #----------------------
    # | convert_layout
    # | Description: Converts the layout count and each FieldLayoutRecord, control and position swapped as u32 unions.
    # | Author: suinevere
    # | Dependencies: field, touch, inside
    # | Globals: LAYOUT_RECORD_SIZE, LAYOUT_RECORD_FIELDS, LAYOUT_RECORD_BYTES
    # | Params: N/A
    # | Returns: N/A
    #----------------------
    def convert_layout(self):
        start, _ = self.bounds["layout"]
        self.inside("layout", start, 4)
        count = self.field(start, 4)
        self.inside("layout", start + 4, count * LAYOUT_RECORD_SIZE)
        for i in range(count):
            record = start + 4 + i * LAYOUT_RECORD_SIZE
            for offset, width in LAYOUT_RECORD_FIELDS:
                self.field(record + offset, width)
            for offset, width in LAYOUT_RECORD_BYTES:
                self.touch(record + offset, width)

    #----------------------
    # | convert_description
    # | Description: Converts a FieldSceneActorDescription: image bytes raw, palette and unkA u16, action count s32, then four u16 per action slot.
    # | Author: suinevere
    # | Dependencies: field, touch, inside, read
    # | Globals: ACTOR_IMAGE_COUNT, ACTOR_IMAGE_END
    # | Params: base -- file offset of the description
    # | Returns: list of the image indices it lists
    #----------------------
    def convert_description(self, base):
        self.inside("actors", base, 0x10)
        images = []
        for index in self.data[base + 2:base + 2 + ACTOR_IMAGE_COUNT]:
            if index == ACTOR_IMAGE_END:
                break
            images.append(index)
        if ("description", base) in self.seen:
            return images
        self.seen.add(("description", base))
        self.touch(base, 8)
        self.field(base + 8, 2)
        self.field(base + 0xA, 2)
        actions = self.field(base + 0xC, 4)
        if actions >= 0x80000000:
            actions = 0
        self.inside("actors", base + 0x10, actions * 8)
        for slot in range(actions * 4):
            self.field(base + 0x10 + slot * 2, 2)
        return images

    #----------------------
    # | convert_actors
    # | Description: Converts the actors head (volume, palette index, map word, actor count), the description offsets and the descriptions.
    # | Author: suinevere
    # | Dependencies: field, inside, convert_description
    # | Globals: N/A
    # | Params: N/A
    # | Returns: set of image indices the descriptions list
    #----------------------
    def convert_actors(self):
        start, _ = self.bounds["actors"]
        self.inside("actors", start, 8)
        for i in range(4):
            self.field(start + i * 2, 2)
        self.actor_count = self.read(start + 6, 2)
        self.inside("actors", start + 8, self.actor_count * 4)
        images = set()
        for i in range(self.actor_count):
            offset = self.field(start + 8 + i * 4, 4)
            images.update(self.convert_description(start + offset))
        return images

    #----------------------
    # | convert_geometry
    # | Description: Converts the geometry offset table (one entry per actor plus the end) and leaves the byte-read animation body raw.
    # | Author: suinevere
    # | Dependencies: field, touch, inside
    # | Globals: N/A
    # | Params: N/A
    # | Returns: N/A
    #----------------------
    def convert_geometry(self):
        start, end = self.bounds["geometry"]
        entries = self.actor_count + 1 if self.actor_count else 0
        self.inside("geometry", start, entries * 4)
        for i in range(entries):
            offset = self.field(start + i * 4, 4)
            if offset < entries * 4 or start + offset > end:
                raise SceneFormatError("geometry offset 0x%X is outside the section" % offset)
        self.touch(start + entries * 4, end - start - entries * 4)

    #----------------------
    # | convert_tim
    # | Description: Converts one paletted TIM: header and block words, CLUT colours through ps1_to_saturn_color, 4bpp pixels nibble-swapped and 8bpp pixels raw.
    # | Author: suinevere
    # | Dependencies: field, touch, inside, read, ps1_to_saturn_color, swap_nibbles
    # | Globals: TIM_MAGIC
    # | Params: base -- file offset of the TIM
    # | Returns: N/A
    #----------------------
    def convert_tim(self, base):
        if ("tim", base) in self.seen:
            return
        self.seen.add(("tim", base))
        self.inside("images", base, 20)
        if self.field(base, 4) != TIM_MAGIC:
            raise SceneFormatError("TIM at 0x%X has no TIM magic" % base)
        flags = self.field(base + 4, 4)
        depth = flags & 7
        if not flags & 8 or depth > 1:
            raise SceneFormatError("TIM at 0x%X is not a paletted 4bpp or 8bpp image" % base)
        clut_size = self.field(base + 8, 4)
        for i in range(4):
            self.field(base + 12 + i * 2, 2)
        colors = self.read(base + 16, 2) * self.read(base + 18, 2)
        if clut_size != 12 + colors * 2:
            raise SceneFormatError("TIM at 0x%X has CLUT size 0x%X for %d colours" % (base, clut_size, colors))
        self.inside("images", base + 20, colors * 2)
        self.touch(base + 20, colors * 2)
        for i in range(colors):
            pos = base + 20 + i * 2
            struct.pack_into(">H", self.out, pos, ps1_to_saturn_color(self.read(pos, 2)))
        pixels = base + 8 + clut_size
        self.inside("images", pixels, 12)
        pixel_size = self.field(pixels, 4)
        for i in range(4):
            self.field(pixels + 4 + i * 2, 2)
        if pixel_size < 12:
            raise SceneFormatError("TIM pixel block at 0x%X has size 0x%X" % (pixels, pixel_size))
        self.inside("images", pixels + 12, pixel_size - 12)
        self.touch(pixels + 12, pixel_size - 12)
        if depth == 0:
            swap_nibbles(self.out, pixels + 12, pixel_size - 12)

    #----------------------
    # | convert_images
    # | Description: Converts the image offset table, sized by its first entry, and every TIM it lists; a listed index past the table raises.
    # | Author: suinevere
    # | Dependencies: field, inside, read, convert_tim
    # | Globals: N/A
    # | Params: used -- image indices the actor descriptions list
    # | Returns: N/A
    #----------------------
    def convert_images(self, used):
        start, end = self.bounds["images"]
        if start == end:
            entries = 0
        else:
            self.inside("images", start, 4)
            first = self.read(start, 4)
            if first == 0 or first % 4 or start + first > end:
                raise SceneFormatError("image table size 0x%X is invalid" % first)
            entries = first // 4
        if any(index >= entries for index in used):
            raise SceneFormatError("an actor lists an image past the %d-entry image table" % entries)
        for i in range(entries):
            self.convert_tim(start + self.field(start + i * 4, 4))

    #----------------------
    # | convert_portraits
    # | Description: Converts the portrait count and leaves the count x PORTRAIT_SIZE portrait bytes raw.
    # | Author: suinevere
    # | Dependencies: field, touch, inside
    # | Globals: PORTRAIT_SIZE
    # | Params: N/A
    # | Returns: N/A
    #----------------------
    def convert_portraits(self):
        start, _ = self.bounds["portraits"]
        self.inside("portraits", start, 4)
        count = self.field(start, 4)
        self.inside("portraits", start + 4, count * PORTRAIT_SIZE)
        self.touch(start + 4, count * PORTRAIT_SIZE)

    #----------------------
    # | convert_group_bounds
    # | Description: Converts the group bounds count and the count u32 bounds after it.
    # | Author: suinevere
    # | Dependencies: field, inside
    # | Globals: N/A
    # | Params: N/A
    # | Returns: N/A
    #----------------------
    def convert_group_bounds(self):
        start, _ = self.bounds["group_bounds"]
        self.inside("group_bounds", start, 4)
        count = self.field(start, 4)
        self.inside("group_bounds", start + 4, count * 4)
        for i in range(count):
            self.field(start + 4 + i * 4, 4)

    #----------------------
    # | convert_records
    # | Description: Converts the records offset table (its first offset is the table size) and each record's u16 id; record payloads stay raw until a reader needs them.
    # | Author: suinevere
    # | Dependencies: field, inside, touch
    # | Globals: N/A
    # | Params: N/A
    # | Returns: N/A
    #----------------------
    def convert_records(self):
        start, end = self.bounds["records"]
        if start == end:
            return
        self.inside("records", start, 4)
        count = self.read(start, 4) >> 2
        self.inside("records", start, count * 4)
        offsets = [self.field(start + i * 4, 4) for i in range(count)]
        bounds = offsets + [end - start]
        for i in range(count):
            if bounds[i + 1] < bounds[i] + 2:
                raise SceneFormatError("record %d at 0x%X is shorter than its id" % (i, bounds[i]))
            self.inside("records", start + bounds[i], bounds[i + 1] - bounds[i])
            self.field(start + bounds[i], 2)
            self.touch(start + bounds[i] + 2, bounds[i + 1] - bounds[i] - 2)

    #----------------------
    # | convert_offset_table
    # | Description: Converts a section's leading u16 offset table, whose first entry is the table size, and leaves the bytes after it raw.
    # | Author: suinevere
    # | Dependencies: field, inside, touch
    # | Globals: N/A
    # | Params: name -- section name: strings, event_scripts or actor_scripts
    # | Returns: N/A
    #----------------------
    def convert_offset_table(self, name):
        start, end = self.bounds[name]
        if start == end:
            return
        self.inside(name, start, 2)
        count = self.read(start, 2) >> 1
        self.inside(name, start, count * 2)
        for i in range(count):
            self.field(start + i * 2, 2)
        self.touch(start + count * 2, end - start - count * 2)

    #----------------------
    # | convert
    # | Description: Converts the header and every ruled section, then checks every byte was handled.
    # | Author: suinevere
    # | Dependencies: read_header, convert_layout, convert_offset_table, convert_actors, convert_geometry, convert_images, convert_portraits, convert_group_bounds, convert_records, touch, check_coverage
    # | Globals: SCENE_SECTIONS
    # | Params: N/A
    # | Returns: converted bytes
    #----------------------
    def convert(self):
        self.read_header()
        self.convert_layout()
        for name in ("strings", "event_scripts", "actor_scripts"):
            self.convert_offset_table(name)
        used = self.convert_actors()
        self.convert_geometry()
        self.convert_images(used)
        self.convert_portraits()
        self.convert_group_bounds()
        self.convert_records()
        self.check_coverage()
        return bytes(self.out)

    #----------------------
    # | check_coverage
    # | Description: Raises SceneFormatError listing every byte range no rule touched.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: N/A
    # | Returns: N/A
    #----------------------
    def check_coverage(self):
        gaps = []
        pos = 0
        while pos < len(self.touched):
            if self.touched[pos]:
                pos += 1
                continue
            start = pos
            while pos < len(self.touched) and not self.touched[pos]:
                pos += 1
            gaps.append("0x%X..0x%X" % (start, pos))
        if gaps:
            raise SceneFormatError("bytes no rule touched: " + ", ".join(gaps))


#----------------------
# | convert_scene
# | Description: Converts one PS1 scene file for lom_load_buffer + 0x10000; its section offsets are file-relative, so load_base needs no relocation.
# | Author: suinevere
# | Dependencies: _SceneConverter
# | Globals: SATURN_SCENE_BASE
# | Params: data -- PS1 scene bytes, load_base -- Saturn address the scene is read to
# | Returns: converted bytes of the same length; raises SceneFormatError for a scene it cannot fully account for
#----------------------
def convert_scene(data, load_base=SATURN_SCENE_BASE):
    return _SceneConverter(bytes(data)).convert()
