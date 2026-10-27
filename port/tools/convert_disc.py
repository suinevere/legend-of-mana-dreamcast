
#----------------------
# | convert_disc.py
# | Description: Converts the user's PS1 disc image into Dreamcast files: per-overlay data blobs in cd/data/OVL, decompressed stream and raw resources in cd/data/RES, and a root resource index.
# | Author: suinevere
# | Dependencies: disc_image.py, overlay_rules.py, scene_patches.py, button_art.py, targets.py, wmap_resources.py, tools/splat_ext/decompress.py
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
REPO = getattr(sys, "_MEIPASS", None) or os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "tools", "splat_ext"))

import disc_image  # noqa: E402
import overlay_rules  # noqa: E402
import scene_patches  # noqa: E402
import button_art  # noqa: E402
import targets  # noqa: E402
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
# | RAW_RESOURCES
# | Description: Uncompressed resources exported raw ahead of the rest, keeping their RESINDEX.BIN order: scene 13's map (191), WAKU1.TIM (1498), the text image (1500), the scene files of scenes 13, 16 and 857 (1561, 1564, 2405), the party scripts (2615, 2715) and PF_M0.IMG (2795).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RAW_RESOURCES = (191, 1498, 1500, 1561, 1564, 2405, 2615, 2715, 2795)


#----------------------
# | SKIPPED_RESOURCES
# | Description: Resources skipped besides the unlinked overlays; none since MOVIE became an overlay_rules overlay.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SKIPPED_RESOURCES = []


#----------------------
# | SKIPPED_SOUND_RESOURCES
# | Description: Sound resources CD_RES_SOUND_EFFECT_SET through FIELD_FIXED_SONG_RESOURCE (include/main/cd_resources.h) the index marks KIND_SKIP, except DREAMCAST_RAW_SOUND_RESOURCES; cdrom_queue_read ignores them instead of halting.
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
# | Description: The resource indices the index marks KIND_SKIP besides the unlinked overlays: the sound range less DREAMCAST_RAW_SOUND_RESOURCES, and WMAP's skipped resources.
# | Author: suinevere
# | Dependencies: SKIPPED_RESOURCES, SKIPPED_SOUND_RESOURCES, DREAMCAST_RAW_SOUND_RESOURCES, wmap_resources.SKIPPED
# | Globals: N/A
# | Params: N/A
# | Returns: list of resource indices
#----------------------
def skipped_resources():
    sound = [index for index in SKIPPED_SOUND_RESOURCES if index not in DREAMCAST_RAW_SOUND_RESOURCES]
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
# | OVL_FOLDER_CAPACITY
# | Description: Directory entries the cd/data/OVL folder may hold, its . and .. records included.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVL_FOLDER_CAPACITY = 64


#----------------------
# | DREAMCAST_RES_FOLDER_CAPACITY
# | Description: Files per Dreamcast cd/data/RES/<XX> subfolder; resource_path fans every non-overlay resource there by index >> 8, so no single directory grows past it.
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
# | Description: Rejects an index with more entries than DREAMCAST_RESOURCE_INDEX_MAX.
# | Author: suinevere
# | Dependencies: DREAMCAST_RESOURCE_INDEX_MAX
# | Globals: N/A
# | Params: entries -- list of (index, kind, size)
# | Returns: N/A
#----------------------
def check_index_capacity(entries):
    if len(entries) > DREAMCAST_RESOURCE_INDEX_MAX:
        raise ValueError("RESINDEX.BIN has %d entries, the index holds %d" % (len(entries), DREAMCAST_RESOURCE_INDEX_MAX))


#----------------------
# | check_folder_capacity
# | Description: Rejects an export whose OVL folder holds more than OVL_FOLDER_CAPACITY entries or a RES/<XX> subfolder more than DREAMCAST_RES_FOLDER_CAPACITY files.
# | Author: suinevere
# | Dependencies: OVL_FOLDER_CAPACITY, DREAMCAST_RES_FOLDER_CAPACITY, dreamcast_res_subfolder
# | Globals: N/A
# | Params: entries -- list of (index, kind, size)
# | Returns: N/A
#----------------------
def check_folder_capacity(entries):
    ovl_count = sum(2 for _, kind, _ in entries if kind == KIND_OVERLAY)
    if ovl_count + 2 > OVL_FOLDER_CAPACITY:
        raise ValueError("folder OVL has %d files, its directory table holds %d" % (ovl_count, OVL_FOLDER_CAPACITY - 2))
    buckets = {}
    for index, kind, _ in entries:
        if kind != KIND_SKIP and kind != KIND_OVERLAY:
            sub = dreamcast_res_subfolder(index)
            buckets[sub] = buckets.get(sub, 0) + 1
    for sub, count in sorted(buckets.items()):
        if count > DREAMCAST_RES_FOLDER_CAPACITY:
            raise ValueError("folder RES/%s has %d files, limit %d" % (sub, count, DREAMCAST_RES_FOLDER_CAPACITY))


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
# | Description: Disc folder that holds a resource's file: OVL for overlays, RES for everything else.
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
# | Description: Path of a resource's disc file: overlays under cd/data/OVL, everything else fanned into cd/data/RES/<XX> subfolders.
# | Author: suinevere
# | Dependencies: KIND_OVERLAY, resource_folder, dreamcast_res_subfolder
# | Globals: N/A
# | Params: out -- project directory, index -- PS1 resource index, kind -- KIND_*
# | Returns: file path
#----------------------
def resource_path(out, index, kind):
    if kind == KIND_OVERLAY:
        return os.path.join(out, "cd", "data", resource_folder(kind), "OVL%02d.BIN" % index)
    return os.path.join(out, "cd", "data", "RES", dreamcast_res_subfolder(index), "RES%04d.BIN" % index)


#----------------------
# | index_path
# | Description: Path of RESINDEX.BIN, which stays in the disc root.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: out -- project directory
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
# | Params: out -- project directory
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
# | Params: out -- project directory, entries -- list of (index, kind, size) the export wrote
# | Returns: N/A
#----------------------
def remove_stale_folder_files(out, entries):
    written = {os.path.normcase(os.path.normpath(resource_path(out, index, kind)))
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
# | export_wmap_resources
# | Description: Exports every wmap_resources.RULES entry with the Dreamcast button art, decompressing stream entries after their flag byte.
# | Author: suinevere
# | Dependencies: wmap_resources.RULES, button_art.patch_raw, disc_image.read_resource, decompress, check_resource_live, write_file, resource_path, KIND_STREAM, KIND_RAW
# | Globals: N/A
# | Params: image -- DiscImage, table -- resource table, out -- output directory, entries -- entries list to extend
# | Returns: N/A
#----------------------
def export_wmap_resources(image, table, out, entries):
    for index, kind in sorted(wmap_resources.RULES.items()):
        check_resource_live(index, table[index])
        data = disc_image.read_resource(image, table[index])
        code = KIND_RAW
        if kind == wmap_resources.STREAM:
            data = decompress(bytes(data[1:]))
            code = KIND_STREAM
        write_file(resource_path(out, index, code), button_art.patch_raw(index, bytes(data)))
        entries.append((index, code, len(data)))


#----------------------
# | dreamcast_overlay_exported
# | Description: Whether an overlay is exported as KIND_OVERLAY: it has a Dreamcast region and is not in DREAMCAST_UNLINKED_OVERLAYS.
# | Author: suinevere
# | Dependencies: targets.DREAMCAST, targets.DREAMCAST_UNLINKED_OVERLAYS
# | Globals: N/A
# | Params: name -- overlay name
# | Returns: True if the overlay should be converted and linked, False if it should be KIND_SKIP
#----------------------
def dreamcast_overlay_exported(name):
    return name in targets.DREAMCAST.regions and name not in targets.DREAMCAST_UNLINKED_OVERLAYS


#----------------------
# | run
# | Description: Exports every overlay with its pointers relocated, every stream decompressed and every other live resource raw, with the Dreamcast button art where it applies,
# |              writes each overlay's generated function tables and linker symbols, and writes the index with KIND_SKIP for what it deliberately omits.
# | Author: suinevere
# | Dependencies: disc_image, overlay_rules, scene_patches.patch, button_art.patch_stream, button_art.patch_overlay, decompress, encode_index, check_disc_identity, check_resource_live,
# |               check_overlay_length, check_stream_size, load_spec_symbols, STREAM_RESOURCES, RAW_RESOURCES, export_wmap_resources, resource_path, index_path,
# |               remove_stale_root_files, check_index_capacity, check_folder_capacity, remove_stale_folder_files,
# |               DREAMCAST_RAW_SOUND_RESOURCES, dreamcast_raw_resources, skipped_resources, dreamcast_overlay_exported
# | Globals: REPO
# | Params: args -- parsed command-line arguments
# | Returns: process exit code
#----------------------
def run(args):
    image = disc_image.DiscImage(args.image)
    check_disc_identity(image)
    table = disc_image.parse_resource_table(image.read_sectors(TABLE_LBA, (TABLE_SIZE + 2047) // 2048)[:TABLE_SIZE])
    entries = []
    remove_stale_root_files(args.out)

    for index, spec in sorted(overlay_rules.OVERLAYS.items()):
        if not dreamcast_overlay_exported(spec.name):
            entries.append((index, KIND_SKIP, 0))
            continue
        check_resource_live(index, table[index])
        raw = disc_image.read_resource(image, table[index])
        overlay = decompress(raw[1:])
        check_overlay_length(spec, overlay)
        overlay = button_art.patch_overlay(index, overlay)
        symbols = load_spec_symbols(spec, os.path.join(REPO, "config", "us", "symbols"))
        blob, script = overlay_rules.build_overlay(spec, overlay, symbols)
        write_file(resource_path(args.out, index, KIND_OVERLAY), bytes(blob))
        write_file(os.path.join(args.out, "gen", "%s_symbols.ld" % spec.name), script)
        source = overlay_rules.function_table_source(spec, overlay, symbols)
        if source is not None:
            write_file(os.path.join(args.out, "src", spec.name, "%s_function_tables.c" % spec.name), source)
        entries.append((index, KIND_OVERLAY, len(blob)))

    for index in sorted(STREAM_RESOURCES):
        check_resource_live(index, table[index])
        data = decompress(disc_image.read_resource(image, table[index])[1:])
        check_stream_size(index, len(data))
        write_file(resource_path(args.out, index, KIND_STREAM), button_art.patch_stream(index, bytes(data)))
        entries.append((index, KIND_STREAM, len(data)))

    for index in RAW_RESOURCES:
        check_resource_live(index, table[index])
        data = disc_image.read_resource(image, table[index])
        write_file(resource_path(args.out, index, KIND_RAW), data)
        entries.append((index, KIND_RAW, len(data)))

    export_wmap_resources(image, table, args.out, entries)

    for index in DREAMCAST_RAW_SOUND_RESOURCES:
        check_resource_live(index, table[index])
        data = disc_image.read_resource(image, table[index])
        write_file(resource_path(args.out, index, KIND_RAW), data)
        entries.append((index, KIND_RAW, len(data)))

    for index in dreamcast_raw_resources(table, entries, set(overlay_rules.OVERLAYS)):
        data = scene_patches.patch(index, disc_image.read_resource(image, table[index]))
        write_file(resource_path(args.out, index, KIND_RAW), data)
        entries.append((index, KIND_RAW, len(data)))

    claimed = {index for index, _, _ in entries}
    for index in skipped_resources():
        if index not in claimed:
            entries.append((index, KIND_SKIP, 0))

    check_index_capacity(entries)
    check_folder_capacity(entries)
    remove_stale_folder_files(args.out, entries)
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
    parser = argparse.ArgumentParser(description="Convert the PS1 disc for the Dreamcast build")
    parser.add_argument("--image", default=default_image())
    parser.add_argument("--target", choices=sorted(targets.TARGETS), default=targets.DREAMCAST.name)
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)

    try:
        return run(args)
    except ValueError as exc:
        print("convert_disc: %s" % exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
