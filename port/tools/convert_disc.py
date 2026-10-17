#----------------------
# | convert_disc.py
# | Description: Converts the user's PS1 disc image into Saturn or Dreamcast files: per-overlay data blobs in cd/data/OVL, decompressed stream and raw resources in cd/data/RES, and a root resource index.
# | Author: suinevere
# | Dependencies: disc_image.py, overlay_rules.py, targets.py, field_map.py, field_scene.py, wmap_layout.py, wmap_resources.py, wmap_memory.py, tools/splat_ext/decompress.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "tools", "splat_ext"))

import disc_image  # noqa: E402
import field_map  # noqa: E402
import field_party_data  # noqa: E402
import field_scene  # noqa: E402
import overlay_rules  # noqa: E402
import targets  # noqa: E402
import wmap_layout  # noqa: E402
import wmap_memory  # noqa: E402
import wmap_resources  # noqa: E402
from decompress import decompress  # noqa: E402

#----------------------
# | KIND_OVERLAY / KIND_STREAM / KIND_RAW / KIND_SKIP
# | Description: Resource kinds in RESINDEX.BIN: converted overlay data, decompressed stream data, raw bytes, and resources deliberately left off the disc.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
KIND_OVERLAY = 1
KIND_STREAM = 2
KIND_RAW = 3
KIND_SKIP = 4

#----------------------
# | STREAM_RESOURCES
# | Description: Resources read through cdrom_stream, with the largest decompressed size their destination holds; 1509 (portraits) fills FIELD's CD buffer (LOM_FIELD_CD_BUFFER_SIZE) and the maps of Home (195) and scene 857 (578) the load buffer.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STREAM_RESOURCES = {177: 0x3000, 178: 0xBE00, 195: 0x5A000, 578: 0x5A000, 1509: 0x19000}

#----------------------
# | convert_window_textures
# | Description: Converts FIELD's window texture stream: the first 64 halfwords as big-endian CLUTs, then the following 64x32 halfwords as 4bpp TIM pixels.
# | Author: suinevere
# | Dependencies: overlay_rules.ps1_to_saturn_color, overlay_rules.swap_nibbles
# | Globals: N/A
# | Params: buf -- decompressed resource 177 bytearray
# | Returns: N/A
#----------------------
def convert_window_textures(buf):
    for i in range(64):
        pos = i * 2
        (value,) = struct.unpack_from("<H", buf, pos)
        struct.pack_into(">H", buf, pos, overlay_rules.ps1_to_saturn_color(value))
    overlay_rules.swap_nibbles(buf, 64 * 2, 64 * 32 * 2)


#----------------------
# | convert_field_font
# | Description: Swaps FIELD's font glyph bitmaps to big-endian. Opcodes index FIELD_TEXT_GLYPH_RUN_OFFSETS starting at 20, so its real table starts 40 bytes in at 0x1580 (FIELD_TEXT_GLYPH_RUNS); swaps that table, sized by its own first little-endian entry, and leaves the glyph widths at 0x14E0 and the run bytes after the table raw.
# | Author: suinevere
# | Dependencies: overlay_rules.swap_words
# | Globals: N/A
# | Params: buf -- decompressed resource 178 bytearray
# | Returns: N/A
#----------------------
def convert_field_font(buf):
    overlay_rules.swap_words(buf, 0, 0x14E0 // 2, 2)
    offsets_start = 0x1580
    (table_size,) = struct.unpack_from("<H", buf, offsets_start)
    if table_size < 2 or table_size % 2 != 0 or offsets_start + table_size > len(buf):
        raise ValueError("field font glyph run offsets table size 0x%X is invalid" % table_size)
    overlay_rules.swap_words(buf, offsets_start, table_size // 2, 2)


#----------------------
# | convert_stream_map
# | Description: Converts a decompressed FIELD map stream in place through field_map.convert_map.
# | Author: suinevere
# | Dependencies: field_map.convert_map
# | Globals: N/A
# | Params: buf -- decompressed map bytearray
# | Returns: N/A
#----------------------
def convert_stream_map(buf):
    buf[:] = field_map.convert_map(buf)


#----------------------
# | STREAM_RULES
# | Description: Byte-order and pixel-format conversions applied to a stream resource after decompression and the size check.
# | Author: suinevere
# | Dependencies: convert_window_textures, convert_field_font, convert_stream_map, field_party_data.convert_portraits
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STREAM_RULES = {177: convert_window_textures, 178: convert_field_font, 195: convert_stream_map, 578: convert_stream_map, 1509: field_party_data.convert_portraits}

#----------------------
# | convert_tim_resource
# | Description: Converts an uncompressed PS1 TIM resource through overlay_rules.convert_tim and returns the converted copy.
# | Author: suinevere
# | Dependencies: overlay_rules.convert_tim
# | Globals: N/A
# | Params: data -- raw resource bytes
# | Returns: converted bytes
#----------------------
def convert_tim_resource(data):
    buf = bytearray(data)
    overlay_rules.convert_tim(buf, 0)
    return bytes(buf)


#----------------------
# | placeholder_actor_package
# | Description: Replaces an actor resource package with a zero header word, which field_unpack_resource_package matches to no section count, so the actor loads no data or textures.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: data -- raw resource bytes, ignored
# | Returns: four zero bytes
#----------------------
def placeholder_actor_package(data):
    return bytes(4)


#----------------------
# | RAW_RESOURCES
# | Description: Uncompressed resources exported after a converter, one Saturn file per PS1 resource: scene 13's map (191), FIELD's object image WAKU1.TIM (1498), the text image (1500), the scene files of scene 13 (1561), Home (1564, scene 16) and scene 857 (2405, where WMAP sends a new game after Home is placed), the party script packages N_SCRIPT.DAT (2615) and P_SCRIPT.DAT (2715), and a placeholder for the hero package PF_M0.IMG (2795) until actor packages get a converter.
# | Author: suinevere
# | Dependencies: field_map.convert_map, convert_tim_resource, field_scene.convert_scene, field_party_data.convert_party_script, placeholder_actor_package
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RAW_RESOURCES = {
    191: field_map.convert_map,
    1498: convert_tim_resource,
    1500: convert_tim_resource,
    1561: field_scene.convert_scene,
    1564: field_scene.convert_scene,
    2405: field_scene.convert_scene,
    2615: field_party_data.convert_party_script,
    2715: field_party_data.convert_party_script,
    2795: placeholder_actor_package,
}

#----------------------
# | PACKED_RESOURCES
# | Description: Resource ids exported as one blob built from many PS1 source resources instead of resource-for-resource: WMAP's 361 land layouts under wmap_layout.LAYOUT_FIRST and its 361 per-cell layout images under wmap_resources.LAYOUT_IMAGE_FIRST, so each set takes one index entry and one file.
# | Author: suinevere
# | Dependencies: wmap_layout.LAYOUT_FIRST, wmap_resources.LAYOUT_IMAGE_FIRST
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PACKED_RESOURCES = {wmap_layout.LAYOUT_FIRST, wmap_resources.LAYOUT_IMAGE_FIRST}

#----------------------
# | SKIPPED_RESOURCES
# | Description: Overlays of states that are not ported (MOVIE); cdrom_stream ignores them so the state's stub reports the halt. CLOAD is an overlay_rules overlay now, skipped on Saturn through SATURN_UNLINKED_OVERLAYS.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SKIPPED_RESOURCES = [11]

#----------------------
# | SKIPPED_SOUND_RESOURCES
# | Description: Sound resources the Saturn disc does not carry (no audio backend yet), spanning CD_RES_SOUND_EFFECT_SET through FIELD_FIXED_SONG_RESOURCE per include/main/cd_resources.h and field_audio_runtime.c's resource bases (FIELD_UPLOAD_BANK_RESOURCE, CD_RES_MUSIC_FILE, FIELD_SFX_SET_RESOURCE_BASE, FIELD_WEAPON_SFX_RESOURCE_BASE, FIELD_INSTRUMENT_BANK_RESOURCE_BASE); cdrom_queue_read ignores them instead of halting.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SKIPPED_SOUND_RESOURCES = list(range(21, 147))

#----------------------
# | DREAMCAST_RAW_SOUND_RESOURCES
# | Description: CD_RES_SOUND_EFFECT_SET and CD_RES_MSC_DATA, which TITLE's load_title_audio_bank and load_title_seq read; the Dreamcast runs those loaders, so it gets their raw PS1 bytes.
# | Author: suinevere
# | Dependencies: include/main/cd_resources.h, src/overlays/title/title.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST_RAW_SOUND_RESOURCES = (21, 23)


#----------------------
# | skipped_resources
# | Description: The resource indices a target's index marks KIND_SKIP besides its unlinked overlays and packs; a target without convert rules keeps DREAMCAST_RAW_SOUND_RESOURCES.
# | Author: suinevere
# | Dependencies: SKIPPED_RESOURCES, SKIPPED_SOUND_RESOURCES, DREAMCAST_RAW_SOUND_RESOURCES, wmap_resources.SKIPPED
# | Globals: N/A
# | Params: target -- targets.Target
# | Returns: list of resource indices
#----------------------
def skipped_resources(target):
    sound = SKIPPED_SOUND_RESOURCES
    if not target.convert_rules:
        sound = [index for index in sound if index not in DREAMCAST_RAW_SOUND_RESOURCES]
    return SKIPPED_RESOURCES + sound + wmap_resources.SKIPPED


#----------------------
# | TABLE_LBA / TABLE_SIZE
# | Description: Location and size of SKCDPOSE.DAT, from src/main/main.c.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TABLE_LBA = 24
TABLE_SIZE = 46488

#----------------------
# | EXPECTED_BOOT_FILE
# | Description: Boot executable name the disc's root directory must carry; identifies a genuine SLUS-01013 image.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXPECTED_BOOT_FILE = "SLUS_010.13"

#----------------------
# | CLEAN_IMAGE_HINT
# | Description: Shared tail of corruption error messages, prompting a re-check of the source image.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CLEAN_IMAGE_HINT = "is this a clean SLUS-01013 image?"


#----------------------
# | RESOURCE_INDEX_MAX
# | Description: Most entries the Saturn resource index holds; must match resource_index.c's RESOURCE_INDEX_MAX (its #ifndef default) and cdrom_saturn.c's CDROM_INDEX_BLOB_RESOURCE_MAX.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RESOURCE_INDEX_MAX = 256


#----------------------
# | DREAMCAST_RESOURCE_INDEX_MAX
# | Description: Most entries the Dreamcast resource index holds, carrying every live PS1 resource; must match dreamcast/makefile's -DRESOURCE_INDEX_MAX override.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST_RESOURCE_INDEX_MAX = 6144


#----------------------
# | FOLDER_CAPACITY
# | Description: Directory-table entries cd_dir.cxx reserves for each disc folder; must match its s_folders, and each table also holds the folder's . and .. records. Saturn only: Dreamcast fans RES out into subfolders instead (DREAMCAST_RES_FOLDER_CAPACITY).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FOLDER_CAPACITY = {"OVL": 64, "RES": 512}


#----------------------
# | DREAMCAST_RES_FOLDER_CAPACITY
# | Description: Files per Dreamcast cd/data/RES/<XX> subfolder; resource_path fans every non-overlay resource there by index >> 8, so no single directory needs Saturn's cd_dir.cxx-sized table.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST_RES_FOLDER_CAPACITY = 256


#----------------------
# | dreamcast_res_subfolder
# | Description: Two-uppercase-hex-digit Dreamcast RES subfolder name for a resource index (index >> 8), e.g. "01" for 0x123; cdrom_dc.c's cd_open builds the identical path.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: index -- PS1 resource index
# | Returns: subfolder name string
#----------------------
def dreamcast_res_subfolder(index):
    return "%02X" % (index >> 8)


#----------------------
# | FOLDER_FILE_PATTERNS
# | Description: Names of the files the converter owns in each disc folder, which the stale-file cleanup may delete; the makefile owns OVL's OVLnnC.BIN code files.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FOLDER_FILE_PATTERNS = {"OVL": r"OVL\d\d\.BIN", "RES": r"RES\d{4}\.BIN"}


#----------------------
# | encode_index
# | Description: Encodes RESINDEX.BIN: big-endian count, then {u16 index, u8 kind, u8 pad, u32 size} per entry.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: entries -- list of (index, kind, size)
# | Returns: bytes
#----------------------
def encode_index(entries):
    out = struct.pack(">I", len(entries))
    for index, kind, size in entries:
        out += struct.pack(">HBBI", index, kind, 0, size)
    return out


#----------------------
# | check_index_capacity
# | Description: Rejects an index with more entries than its target's resource index can hold: RESOURCE_INDEX_MAX for Saturn, DREAMCAST_RESOURCE_INDEX_MAX for the Dreamcast.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: RESOURCE_INDEX_MAX, DREAMCAST_RESOURCE_INDEX_MAX
# | Params: entries -- list of (index, kind, size), target -- converter Target, default targets.SATURN
# | Returns: N/A
#----------------------
def check_index_capacity(entries, target=targets.SATURN):
    limit = DREAMCAST_RESOURCE_INDEX_MAX if target.name == targets.DREAMCAST.name else RESOURCE_INDEX_MAX
    if len(entries) > limit:
        raise ValueError("RESINDEX.BIN has %d entries, the %s index holds %d" % (len(entries), target.name, limit))


#----------------------
# | check_folder_capacity
# | Description: Rejects an export whose disc folders hold more files than their target allows.
# |              Saturn checks flat OVL/RES against cd_dir.cxx's tables (the . and .. records count too); the Dreamcast checks OVL the same way, plus 256 files per fanned-out RES/<XX> subfolder.
# | Author: suinevere
# | Dependencies: resource_folder, dreamcast_res_subfolder
# | Globals: FOLDER_CAPACITY, DREAMCAST_RES_FOLDER_CAPACITY
# | Params: entries -- list of (index, kind, size), target -- converter Target, default targets.SATURN
# | Returns: N/A
#----------------------
def check_folder_capacity(entries, target=targets.SATURN):
    if target.name == targets.DREAMCAST.name:
        ovl_count = sum(2 for _, kind, _ in entries if kind == KIND_OVERLAY)
        if ovl_count + 2 > FOLDER_CAPACITY["OVL"]:
            raise ValueError("folder OVL has %d files, its directory table holds %d" % (ovl_count, FOLDER_CAPACITY["OVL"] - 2))
        buckets = {}
        for index, kind, _ in entries:
            if kind != KIND_SKIP and kind != KIND_OVERLAY:
                sub = dreamcast_res_subfolder(index)
                buckets[sub] = buckets.get(sub, 0) + 1
        for sub, count in sorted(buckets.items()):
            if count > DREAMCAST_RES_FOLDER_CAPACITY:
                raise ValueError("folder RES/%s has %d files, limit %d" % (sub, count, DREAMCAST_RES_FOLDER_CAPACITY))
        return
    counts = dict.fromkeys(FOLDER_CAPACITY, 0)
    for _, kind, _ in entries:
        if kind != KIND_SKIP:
            counts[resource_folder(kind)] += 2 if kind == KIND_OVERLAY else 1
    for folder, count in sorted(counts.items()):
        if count + 2 > FOLDER_CAPACITY[folder]:
            raise ValueError("folder %s has %d files, its directory table holds %d" % (folder, count, FOLDER_CAPACITY[folder] - 2))


#----------------------
# | check_stream_size
# | Description: Rejects a stream resource whose decompressed size exceeds its destination buffer.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: STREAM_RESOURCES
# | Params: index -- resource index, size -- decompressed size
# | Returns: N/A
#----------------------
def check_stream_size(index, size):
    if size > STREAM_RESOURCES[index]:
        raise ValueError("resource %d decompresses to 0x%X bytes, limit 0x%X" % (index, size, STREAM_RESOURCES[index]))


#----------------------
# | load_symbols
# | Description: Parses a splat symbol_addrs file into a name to address map.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: path -- symbol file path
# | Returns: dict of symbol name to PS1 address
#----------------------
def load_symbols(path):
    symbols = {}
    pattern = re.compile(r"^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;")
    with open(path, "r", encoding="utf-8") as handle:
        for line in handle:
            match = pattern.match(line)
            if match:
                symbols[match.group(1)] = int(match.group(2), 16)
    return symbols


#----------------------
# | load_spec_symbols
# | Description: Loads every symbol file an overlay names, in order, keeping the first definition of a name seen in more than one file.
# | Author: suinevere
# | Dependencies: load_symbols
# | Globals: N/A
# | Params: spec -- OverlaySpec, directory -- folder holding the symbol files
# | Returns: dict of symbol name to PS1 address
#----------------------
def load_spec_symbols(spec, directory):
    names = (spec.symbols_file,) if isinstance(spec.symbols_file, str) else spec.symbols_file
    symbols = {}
    for name in names:
        for symbol, addr in load_symbols(os.path.join(directory, name)).items():
            symbols.setdefault(symbol, addr)
    return symbols


#----------------------
# | default_image
# | Description: Picks the disc image from LOM_PS1_IMAGE or disc/us/LegendOfMana.bin.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: REPO
# | Params: N/A
# | Returns: image path
#----------------------
def default_image():
    return os.environ.get("LOM_PS1_IMAGE", os.path.join(REPO, "disc", "us", "LegendOfMana.bin"))


#----------------------
# | write_file
# | Description: Writes bytes or text, creating parent directories.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: path -- destination, data -- bytes or str
# | Returns: N/A
#----------------------
def write_file(path, data):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    if isinstance(data, str):
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(data)
    else:
        with open(path, "wb") as handle:
            handle.write(data)


#----------------------
# | resource_folder
# | Description: Disc folder that holds a resource's Saturn file: OVL for overlays, RES for everything else.
# | Author: suinevere
# | Dependencies: KIND_OVERLAY
# | Globals: N/A
# | Params: kind -- KIND_*
# | Returns: folder name
#----------------------
def resource_folder(kind):
    return "OVL" if kind == KIND_OVERLAY else "RES"


#----------------------
# | resource_path
# | Description: Path of a resource's disc file: overlays under cd/data/OVL, everything else under cd/data/RES.
# |              Saturn keeps RES flat; the Dreamcast fans it into RES/<XX> subfolders instead, since flat would overflow the (otherwise irrelevant) cd_dir.cxx table.
# | Author: suinevere
# | Dependencies: KIND_OVERLAY, resource_folder, dreamcast_res_subfolder
# | Globals: N/A
# | Params: out -- project directory, index -- PS1 resource index, kind -- KIND_*, target -- converter Target, default targets.SATURN
# | Returns: file path
#----------------------
def resource_path(out, index, kind, target=targets.SATURN):
    if kind == KIND_OVERLAY:
        return os.path.join(out, "cd", "data", resource_folder(kind), "OVL%02d.BIN" % index)
    if target.name == targets.DREAMCAST.name:
        return os.path.join(out, "cd", "data", "RES", dreamcast_res_subfolder(index), "RES%04d.BIN" % index)
    return os.path.join(out, "cd", "data", resource_folder(kind), "RES%04d.BIN" % index)


#----------------------
# | index_path
# | Description: Path of RESINDEX.BIN, which stays in the disc root.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: out -- Saturn project directory
# | Returns: file path
#----------------------
def index_path(out):
    return os.path.join(out, "cd", "data", "RESINDEX.BIN")


#----------------------
# | remove_stale_root_files
# | Description: Deletes OVL and RES files earlier builds wrote to the cd/data root, which now live in folders; RESINDEX.BIN stays.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: out -- Saturn project directory
# | Returns: N/A
#----------------------
def remove_stale_root_files(out):
    root = os.path.join(out, "cd", "data")
    if not os.path.isdir(root):
        return
    for name in os.listdir(root):
        path = os.path.join(root, name)
        if os.path.isfile(path) and re.fullmatch(r"(OVL\d\d[CH]?|RES\d{4})\.BIN", name):
            os.remove(path)


#----------------------
# | remove_stale_folder_files
# | Description: Deletes converter-owned files under cd/data/OVL and cd/data/RES (searched recursively, so the Dreamcast's fanned-out RES/<XX> subfolders are covered too) that the current export
# |              no longer writes, leaving every other file alone.
# | Author: suinevere
# | Dependencies: resource_path, KIND_SKIP
# | Globals: FOLDER_FILE_PATTERNS
# | Params: out -- project directory, entries -- list of (index, kind, size) the export wrote, target -- converter Target, default targets.SATURN
# | Returns: N/A
#----------------------
def remove_stale_folder_files(out, entries, target=targets.SATURN):
    written = {os.path.normcase(os.path.normpath(resource_path(out, index, kind, target)))
               for index, kind, _ in entries if kind != KIND_SKIP}
    for folder, pattern in sorted(FOLDER_FILE_PATTERNS.items()):
        directory = os.path.join(out, "cd", "data", folder)
        if not os.path.isdir(directory):
            continue
        for root, _dirs, names in os.walk(directory):
            for name in names:
                path = os.path.join(root, name)
                if re.fullmatch(pattern, name) and os.path.normcase(os.path.normpath(path)) not in written:
                    os.remove(path)


#----------------------
# | check_disc_identity
# | Description: Rejects an image whose root directory does not carry the SLUS-01013 boot executable.
# | Author: suinevere
# | Dependencies: disc_image.find_iso_file
# | Globals: EXPECTED_BOOT_FILE
# | Params: image -- DiscImage
# | Returns: N/A
#----------------------
def check_disc_identity(image):
    if disc_image.find_iso_file(image, EXPECTED_BOOT_FILE) is None:
        raise ValueError("wrong disc: expected SLUS-01013 (Legend of Mana, US); %s not found in the root directory"
                          % EXPECTED_BOOT_FILE)


#----------------------
# | check_resource_live
# | Description: Rejects a resource table entry that is not live or has a negative size.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: CLEAN_IMAGE_HINT
# | Params: index -- resource index, res -- disc_image.Resource
# | Returns: N/A
#----------------------
def check_resource_live(index, res):
    if not res.live or res.size < 0:
        raise ValueError("resource %d is dead or has a negative size; %s" % (index, CLEAN_IMAGE_HINT))


#----------------------
# | check_overlay_length
# | Description: Rejects a decompressed overlay shorter than its farthest conversion range requires.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: CLEAN_IMAGE_HINT
# | Params: spec -- OverlaySpec, overlay -- decompressed overlay bytes
# | Returns: N/A
#----------------------
def check_overlay_length(spec, overlay):
    min_len = max(end for _, end in spec.ranges) - spec.vram
    if len(overlay) < min_len:
        raise ValueError("%s is shorter than its conversion ranges; %s" % (spec.name, CLEAN_IMAGE_HINT))


#----------------------
# | dreamcast_raw_resources
# | Description: Live, positive-size resource indices no earlier export step claimed and that are not an overlay.
# |              The Dreamcast disc carries each one raw under its own PS1 index, so FIELD and WMAP find every scene, map, actor and script resource.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: table -- resource table, entries -- (index, kind, size) tuples already exported, overlays -- overlay_rules.OVERLAYS indices to exclude
# | Returns: sorted list of resource indices
#----------------------
def dreamcast_raw_resources(table, entries, overlays):
    claimed = {index for index, _, _ in entries}
    return sorted(i for i, res in enumerate(table)
                  if res.live and res.size > 0 and i not in overlays and i not in claimed)


#----------------------
# | export_wmap_layout_pack
# | Description: Packs every WMAP land layout (0x12EA..0x1452) into one blob under wmap_layout.LAYOUT_FIRST, writes it as a single KIND_RAW file, and appends its index entry.
# | Author: suinevere
# | Dependencies: wmap_layout.pack_layouts, disc_image.read_resource, write_file, resource_path, CLEAN_IMAGE_HINT, KIND_RAW
# | Globals: N/A
# | Params: image -- DiscImage, table -- resource table, out -- output directory, entries -- entries list to extend, target -- converter Target, default targets.SATURN
# | Returns: N/A
#----------------------
def export_wmap_layout_pack(image, table, out, entries, target=targets.SATURN):
    def read_layout(cell):
        res = table[wmap_layout.LAYOUT_FIRST + cell]
        if not res.live or res.size < 0:
            return None
        return disc_image.read_resource(image, res)

    try:
        packed = wmap_layout.pack_layouts(read_layout)
    except wmap_layout.LayoutFormatError as exc:
        raise ValueError("WMAP layouts: %s; %s" % (exc, CLEAN_IMAGE_HINT))
    write_file(resource_path(out, wmap_layout.LAYOUT_FIRST, KIND_RAW, target), packed)
    entries.append((wmap_layout.LAYOUT_FIRST, KIND_RAW, len(packed)))


#----------------------
# | export_wmap_layout_image_pack
# | Description: Packs every per-cell WMAP layout image (0x14DE..0x1646), decompressed and converted, into one sector-strided blob under wmap_resources.LAYOUT_IMAGE_FIRST and appends its index entry.
# | Author: suinevere
# | Dependencies: wmap_resources.pack_layout_images, disc_image.read_resource, decompress, write_file, resource_path, CLEAN_IMAGE_HINT, KIND_RAW
# | Globals: N/A
# | Params: image -- DiscImage, table -- resource table, out -- output directory, entries -- entries list to extend, target -- converter Target, default targets.SATURN
# | Returns: N/A
#----------------------
def export_wmap_layout_image_pack(image, table, out, entries, target=targets.SATURN):
    def read_image(cell):
        res = table[wmap_resources.LAYOUT_IMAGE_FIRST + cell]
        if not res.live or res.size < 1:
            return None
        return decompress(disc_image.read_resource(image, res)[1:])

    try:
        packed = wmap_resources.pack_layout_images(read_image)
    except wmap_resources.ResourceFormatError as exc:
        raise ValueError("WMAP layout images: %s; %s" % (exc, CLEAN_IMAGE_HINT))
    write_file(resource_path(out, wmap_resources.LAYOUT_IMAGE_FIRST, KIND_RAW, target), packed)
    entries.append((wmap_resources.LAYOUT_IMAGE_FIRST, KIND_RAW, len(packed)))


#----------------------
# | export_wmap_resources
# | Description: Exports every wmap_resources.RULES entry, decompressing stream entries after their flag byte, converting each unless convert_rules is false, and recording its exported size.
# | Author: suinevere
# | Dependencies: wmap_resources.RULES, disc_image.read_resource, decompress, check_resource_live, write_file, resource_path, CLEAN_IMAGE_HINT, KIND_STREAM, KIND_RAW
# | Globals: N/A
# | Params: image -- DiscImage, table -- resource table, out -- output directory, entries -- entries list to extend, convert_rules -- False writes the PS1 bytes unconverted, target -- converter Target, default targets.SATURN
# | Returns: dict of resource id to exported byte size
#----------------------
def export_wmap_resources(image, table, out, entries, convert_rules=True, target=targets.SATURN):
    sizes = {}
    for index, (kind, convert) in sorted(wmap_resources.RULES.items()):
        check_resource_live(index, table[index])
        data = bytearray(disc_image.read_resource(image, table[index]))
        code = KIND_RAW
        if kind == wmap_resources.STREAM:
            data = bytearray(decompress(bytes(data[1:])))
            code = KIND_STREAM
        size = len(data)
        if not convert_rules:
            convert = bytearray
        try:
            data = convert(data)
        except wmap_resources.ResourceFormatError as exc:
            raise ValueError("resource 0x%X: %s; %s" % (index, exc, CLEAN_IMAGE_HINT))
        if len(data) != size:
            raise ValueError("resource 0x%X changed size in conversion" % index)
        write_file(resource_path(out, index, code, target), bytes(data))
        entries.append((index, code, size))
        sizes[index] = size
    return sizes


#----------------------
# | check_buffer_users
# | Description: Rejects an export in which a resource read into a placed WMAP buffer is larger than that buffer.
# | Author: suinevere
# | Dependencies: wmap_memory.BUFFERS
# | Globals: N/A
# | Params: sizes -- resource id to exported byte size; a pack's first id carries one cell's item size
# | Returns: N/A
#----------------------
def check_buffer_users(sizes):
    for buf in wmap_memory.BUFFERS:
        for res_id in buf.users:
            if sizes.get(res_id, 0) > buf.size:
                raise ValueError("resource 0x%X (%d bytes) overflows %s (%d bytes)" % (res_id, sizes[res_id], buf.name, buf.size))


#----------------------
# | dreamcast_overlay_exported
# | Description: Whether an overlay is exported as KIND_OVERLAY for this target: with no region table, every overlay but SATURN_UNLINKED_OVERLAYS, else region membership minus DREAMCAST_UNLINKED_OVERLAYS for a target without convert rules.
# | Author: suinevere
# | Dependencies: targets.DREAMCAST_UNLINKED_OVERLAYS, targets.SATURN_UNLINKED_OVERLAYS
# | Globals: N/A
# | Params: target -- converter Target; name -- overlay name
# | Returns: True if the overlay should be converted and linked, False if it should be KIND_SKIP
#----------------------
def dreamcast_overlay_exported(target, name):
    if target.regions is None:
        return name not in targets.SATURN_UNLINKED_OVERLAYS
    if name not in target.regions:
        return False
    return target.convert_rules or name not in targets.DREAMCAST_UNLINKED_OVERLAYS


#----------------------
# | run
# | Description: Converts every overlay, stream and raw resource the target build loads, writes each overlay's generated function tables, and writes the index.
# |              A target without convert rules gets raw PS1 bytes outside its relocated overlays, plus every other live resource raw (WMAP's layout cells unpacked), and KIND_SKIP for overlays and resources it deliberately omits.
# | Author: suinevere
# | Dependencies: disc_image, overlay_rules, targets.by_name, decompress, encode_index, check_disc_identity, check_resource_live,
# |               check_overlay_length, check_stream_size, load_spec_symbols, STREAM_RULES, RAW_RESOURCES, export_wmap_layout_pack,
# |               export_wmap_layout_image_pack, export_wmap_resources, check_buffer_users, wmap_resources, resource_path, index_path,
# |               remove_stale_root_files, check_index_capacity, check_folder_capacity, remove_stale_folder_files,
# |               DREAMCAST_RAW_SOUND_RESOURCES, dreamcast_raw_resources, skipped_resources, dreamcast_overlay_exported
# | Globals: REPO
# | Params: args -- parsed command-line arguments
# | Returns: process exit code
#----------------------
def run(args):
    target = targets.by_name(args.target)
    image = disc_image.DiscImage(args.image)
    check_disc_identity(image)
    table = disc_image.parse_resource_table(image.read_sectors(TABLE_LBA, (TABLE_SIZE + 2047) // 2048)[:TABLE_SIZE])
    entries = []
    remove_stale_root_files(args.out)

    for index, spec in sorted(overlay_rules.OVERLAYS.items()):
        if not dreamcast_overlay_exported(target, spec.name):
            entries.append((index, KIND_SKIP, 0))
            continue
        check_resource_live(index, table[index])
        raw = disc_image.read_resource(image, table[index])
        overlay = decompress(raw[1:])
        check_overlay_length(spec, overlay)
        symbols = load_spec_symbols(spec, os.path.join(REPO, "config", "us", "symbols"))
        blob, script = overlay_rules.build_overlay(spec, overlay, symbols, target)
        write_file(resource_path(args.out, index, KIND_OVERLAY), bytes(blob))
        write_file(os.path.join(args.out, "gen", "%s_symbols.ld" % spec.name), script)
        source = overlay_rules.function_table_source(spec, overlay, symbols)
        if source is not None:
            write_file(os.path.join(args.out, "src", spec.name, "%s_function_tables.c" % spec.name), source)
        entries.append((index, KIND_OVERLAY, len(blob)))

    for index in sorted(STREAM_RESOURCES):
        check_resource_live(index, table[index])
        data = bytearray(decompress(disc_image.read_resource(image, table[index])[1:]))
        check_stream_size(index, len(data))
        rule = STREAM_RULES.get(index)
        if rule is not None and target.convert_rules:
            rule(data)
        write_file(resource_path(args.out, index, KIND_STREAM, target), bytes(data))
        entries.append((index, KIND_STREAM, len(data)))

    for index, rule in sorted(RAW_RESOURCES.items()):
        check_resource_live(index, table[index])
        data = disc_image.read_resource(image, table[index])
        if target.convert_rules:
            try:
                data = rule(data)
            except (field_map.MapFormatError, field_scene.SceneFormatError, field_party_data.PartyDataFormatError) as exc:
                raise ValueError("resource %d: %s; %s" % (index, exc, CLEAN_IMAGE_HINT))
        write_file(resource_path(args.out, index, KIND_RAW, target), data)
        entries.append((index, KIND_RAW, len(data)))

    if target.convert_rules:
        export_wmap_layout_pack(image, table, args.out, entries, target)
        export_wmap_layout_image_pack(image, table, args.out, entries, target)
        sizes = export_wmap_resources(image, table, args.out, entries, target=target)
        sizes[wmap_resources.LAYOUT_IMAGE_FIRST] = wmap_resources.LAYOUT_IMAGE_BYTES
        check_buffer_users(sizes)
    else:
        export_wmap_resources(image, table, args.out, entries, convert_rules=False, target=target)

    if not target.convert_rules:
        for index in DREAMCAST_RAW_SOUND_RESOURCES:
            check_resource_live(index, table[index])
            data = disc_image.read_resource(image, table[index])
            write_file(resource_path(args.out, index, KIND_RAW, target), data)
            entries.append((index, KIND_RAW, len(data)))

        overlays = set(overlay_rules.OVERLAYS)
        for index in dreamcast_raw_resources(table, entries, overlays):
            data = disc_image.read_resource(image, table[index])
            write_file(resource_path(args.out, index, KIND_RAW, target), data)
            entries.append((index, KIND_RAW, len(data)))

    claimed = {index for index, _, _ in entries}
    for index in skipped_resources(target):
        if index not in claimed:
            entries.append((index, KIND_SKIP, 0))

    check_index_capacity(entries, target)
    check_folder_capacity(entries, target)
    remove_stale_folder_files(args.out, entries, target)
    write_file(index_path(args.out), encode_index(entries))
    for index, kind, size in entries:
        print("resource %4d kind %d size 0x%X" % (index, kind, size))
    return 0


#----------------------
# | main
# | Description: Command-line entry: parses arguments and runs the conversion, reporting a bad image as one stderr line.
# | Author: suinevere
# | Dependencies: run
# | Globals: N/A
# | Params: argv -- command-line arguments
# | Returns: process exit code
#----------------------
def main(argv=None):
    parser = argparse.ArgumentParser(description="Convert the PS1 disc for the Saturn or Dreamcast build")
    parser.add_argument("--image", default=default_image())
    parser.add_argument("--target", choices=sorted(targets.TARGETS), default=targets.SATURN.name)
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)

    try:
        return run(args)
    except ValueError as exc:
        print("convert_disc: %s" % exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
