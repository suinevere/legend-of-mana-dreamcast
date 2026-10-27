#----------------------
# | libgte_tables.py
# | Description: Extracts the Psy-Q libgte lookup tables from SLUS_010.13 on the user's disc into gen/libgte_tables.bin and an offsets header.
# | Author: suinevere
# | Dependencies: disc_image.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import hashlib
import os
import struct
import sys

#----------------------
# | HERE
# | Description: Directory holding this tool and disc_image.py.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HERE = os.path.dirname(os.path.abspath(__file__))

#----------------------
# | REPO
# | Description: Root of the decomp repository, where disc/us/LegendOfMana.bin is looked for by default.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: HERE
# | Params: N/A
# | Returns: N/A
#----------------------
REPO = getattr(sys, "_MEIPASS", None) or os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)

import disc_image  # noqa: E402

#----------------------
# | EXE_NAME
# | Description: ISO9660 name of the PS1 main executable that links libgte.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXE_NAME = "SLUS_010.13"

#----------------------
# | EXPECTED_SHA1
# | Description: SHA-1 of the US SLUS_010.13 that config/us/SLUS_010.13.yaml describes and the table addresses come from.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXPECTED_SHA1 = "d11dfdd50d412ac3fa3e2eb80fbde138da118f27"

#----------------------
# | EXE_HEADER_SIZE
# | Description: Size of the PS-X EXE header that precedes the loaded image in the file.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXE_HEADER_SIZE = 0x800

#----------------------
# | ISO_PVD_LBA
# | Description: Sector of the ISO9660 primary volume descriptor.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ISO_PVD_LBA = 16

#----------------------
# | ISO_SECTOR
# | Description: Bytes of user data per CD sector.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ISO_SECTOR = 2048

#----------------------
# | TABLES
# | Description: The libgte tables as (name, PS1 address, byte size): sine, CORDIC arctangents, square root, inverse square root, packed sine/cosine and arctangent.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TABLES = (
    ("rsin", 0x80036470, 0x802),
    ("cordic", 0x80036C80, 0x18),
    ("sqrt", 0x80036CB0, 0x180),
    ("inv_sqrt", 0x80036E44, 0x180),
    ("rcossin", 0x800372E0, 0x4000),
    ("ratan", 0x8003B2E0, 0x802),
)

#----------------------
# | BLOCKS
# | Description: Contiguous (name, start, end) address ranges copied into the blob; geom keeps the gaps because SquareRoot0 and VectorNormal index below their tables.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BLOCKS = (
    ("geom", 0x80036470, 0x80036FC4),
    ("trig", 0x800372E0, 0x8003BAE2),
)


#----------------------
# | read_iso_file
# | Description: Reads a file from the ISO9660 root directory of a disc image by name, ignoring the ;1 version suffix.
# | Author: suinevere
# | Dependencies: disc_image.DiscImage.read_sectors
# | Globals: ISO_PVD_LBA, ISO_SECTOR
# | Params: image -- DiscImage, name -- file name
# | Returns: the file's bytes
#----------------------
def read_iso_file(image, name):
    pvd = image.read_sectors(ISO_PVD_LBA, 1)
    if pvd[0] != 1 or pvd[1:6] != b"CD001":
        raise ValueError("no ISO9660 primary volume descriptor")
    root_lba, root_size = struct.unpack_from("<I", pvd, 158)[0], struct.unpack_from("<I", pvd, 166)[0]
    directory = image.read_sectors(root_lba, (root_size + ISO_SECTOR - 1) // ISO_SECTOR)
    pos = 0
    while pos < len(directory):
        length = directory[pos]
        if length == 0:
            pos = (pos // ISO_SECTOR + 1) * ISO_SECTOR
            continue
        ident = directory[pos + 33:pos + 33 + directory[pos + 32]].decode("ascii", "replace").split(";")[0]
        if ident.upper() == name.upper():
            lba, size = struct.unpack_from("<II", directory, pos + 2)[0], struct.unpack_from("<I", directory, pos + 10)[0]
            return image.read_sectors(lba, (size + ISO_SECTOR - 1) // ISO_SECTOR)[:size]
        pos += length
    raise FileNotFoundError("%s not found in the disc's root directory" % name)


#----------------------
# | exe_slice
# | Description: Returns bytes of a PS-X EXE at a PS1 address, mapped through the header's load address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: EXE_HEADER_SIZE
# | Params: exe -- executable bytes, address -- PS1 address, size -- byte count
# | Returns: the bytes
#----------------------
def exe_slice(exe, address, size):
    if exe[:8] != b"PS-X EXE":
        raise ValueError("not a PS-X EXE")
    load = struct.unpack_from("<I", exe, 0x18)[0]
    offset = address - load + EXE_HEADER_SIZE
    if address < load or offset + size > len(exe):
        raise ValueError("0x%08X+0x%X is outside the executable" % (address, size))
    return exe[offset:offset + size]


#----------------------
# | layout
# | Description: Blob offsets of every block and table; blocks are stored in order, each padded to a multiple of four bytes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: BLOCKS, TABLES
# | Params: N/A
# | Returns: (dict of name to blob offset, total blob size)
#----------------------
def layout():
    offsets = {}
    pos = 0
    for name, start, end in BLOCKS:
        offsets[name] = pos
        for table, address, size in TABLES:
            if start <= address and address + size <= end:
                offsets[table] = pos + address - start
        pos += (end - start + 3) & ~3
    return offsets, pos


#----------------------
# | build_blob
# | Description: Concatenates the executable's blocks into the table blob, padding each to four bytes.
# | Author: suinevere
# | Dependencies: exe_slice, layout
# | Globals: BLOCKS
# | Params: exe -- executable bytes
# | Returns: blob bytes
#----------------------
def build_blob(exe):
    blob = bytearray()
    for _, start, end in BLOCKS:
        blob += exe_slice(exe, start, end - start)
        blob += bytes(-len(blob) % 4)
    return bytes(blob)


#----------------------
# | render_header
# | Description: Renders gen/libgte_tables.h: one byte-offset define per block and table plus the blob size.
# | Author: suinevere
# | Dependencies: layout
# | Globals: N/A
# | Params: N/A
# | Returns: header text
#----------------------
def render_header():
    offsets, total = layout()
    lines = ["/*----------------------",
             " | libgte_tables.h",
             " | Description: Generated by tools/libgte_tables.py: byte offsets of the libgte tables inside gen/libgte_tables.bin.",
             " | Author: suinevere",
             " | Dependencies: N/A",
             " | Globals: N/A",
             " | Params: N/A",
             " | Returns: N/A",
             " ----------------------*/",
             "#ifndef LIBGTE_TABLES_H",
             "#define LIBGTE_TABLES_H",
             ""]
    for name, value in offsets.items():
        lines.append("#define LOM_LIBGTE_%s_OFFSET 0x%05X" % (name.upper(), value))
    lines += ["#define LOM_LIBGTE_TABLES_SIZE 0x%05X" % total, "", "#endif", ""]
    return "\n".join(lines)


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
# | run
# | Description: Reads SLUS_010.13 from the image, checks its hash, and writes gen/libgte_tables.bin and gen/libgte_tables.h; a missing or unreadable image raises ValueError.
# | Author: suinevere
# | Dependencies: read_iso_file, build_blob, render_header, write_file
# | Globals: EXE_NAME
# | Params: image_path -- disc image, out -- output root, expected_sha1 -- required hash or None
# | Returns: N/A
#----------------------
def run(image_path, out, expected_sha1):
    image = disc_image.DiscImage(image_path)
    exe = read_iso_file(image, EXE_NAME)
    digest = hashlib.sha1(exe).hexdigest()
    if expected_sha1 is not None and digest != expected_sha1:
        raise ValueError("%s has SHA-1 %s, expected %s" % (EXE_NAME, digest, expected_sha1))
    write_file(os.path.join(out, "gen", "libgte_tables.bin"), build_blob(exe))
    write_file(os.path.join(out, "gen", "libgte_tables.h"), render_header())


#----------------------
# | main
# | Description: Command-line entry: extracts the tables from LOM_PS1_IMAGE or disc/us/LegendOfMana.bin into the caller's chosen folder, reporting failures, including a corrupt ISO directory, in one line.
# | Author: suinevere
# | Dependencies: run
# | Globals: REPO, EXPECTED_SHA1
# | Params: argv -- command-line arguments
# | Returns: process exit code
#----------------------
def main(argv=None):
    parser = argparse.ArgumentParser(description="Extract the libgte tables from SLUS_010.13")
    parser.add_argument("--image", default=os.environ.get("LOM_PS1_IMAGE", os.path.join(REPO, "disc", "us", "LegendOfMana.bin")))
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    try:
        run(args.image, args.out, EXPECTED_SHA1)
    except (ValueError, FileNotFoundError, IndexError, struct.error) as error:
        print("libgte_tables: %s" % error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
