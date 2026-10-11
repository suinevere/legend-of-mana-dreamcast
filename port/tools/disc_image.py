#----------------------
# | disc_image.py
# | Description: Reads user-data sectors from a PS1 disc image or a mounted drive, and parses its ISO9660 and SKCDPOSE tables.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import collections
import os
import struct

#----------------------
# | Resource
# | Description: One SKCDPOSE entry: resource index, start LBA, byte size, and whether the entry is in use.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Resource = collections.namedtuple("Resource", "index lba size live")

#----------------------
# | SECTOR_SYNC
# | Description: The 12-byte sync pattern that starts every raw 2352-byte CD sector.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECTOR_SYNC = bytes([0x00] + [0xFF] * 10 + [0x00])

#----------------------
# | LOM_PS1_IMAGE_ENV
# | Description: Environment variable name suggested in disc image error messages.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LOM_PS1_IMAGE_ENV = "LOM_PS1_IMAGE"


#----------------------
# | _resolve_drive_path
# | Description: Resolves a drive letter (E:, E:\\) or a directory that is a mounted drive root to its raw Windows device path.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: path -- user-supplied path
# | Returns: device path such as \\.\E:, or None when path is not a drive
#----------------------
def _resolve_drive_path(path):
    stripped = str(path).rstrip("\\/")
    if len(stripped) == 2 and stripped[1] == ":":
        return "\\\\.\\" + stripped
    if os.path.isdir(path) and os.path.ismount(path):
        drive = os.path.splitdrive(os.path.abspath(path))[0]
        if drive:
            return "\\\\.\\" + drive
    return None


#----------------------
# | DiscImage
# | Description: Random access to 2048-byte user data sectors of a raw (2352) or cooked (2048) disc image, or a mounted drive.
# | Author: suinevere
# | Dependencies: _resolve_drive_path
# | Globals: SECTOR_SYNC, LOM_PS1_IMAGE_ENV, PVD_LBA
# | Params: path -- image file, drive letter or mounted drive directory
# | Returns: N/A
#----------------------
class DiscImage:
    #----------------------
    # | __init__
    # | Description: Opens a drive or image file and detects whether its sectors are raw 2352-byte or cooked 2048-byte.
    # | Author: suinevere
    # | Dependencies: _resolve_drive_path, read_sectors
    # | Globals: SECTOR_SYNC, LOM_PS1_IMAGE_ENV, PVD_LBA
    # | Params: path -- image file, drive letter or mounted drive directory
    # | Returns: N/A; raises ValueError for a missing, unreadable or unrecognised image
    #----------------------
    def __init__(self, path):
        self._source = path
        device = _resolve_drive_path(path)
        if device is not None:
            self.path = device
            self.stride, self.offset = 2048, 0
            self.read_sectors(PVD_LBA, 1)
            return
        try:
            size = os.path.getsize(path)
            with open(path, "rb") as handle:
                head = handle.read(12)
        except OSError as exc:
            raise ValueError("cannot open disc image %s (set %s to a valid path): %s" % (path, LOM_PS1_IMAGE_ENV, exc))
        self.path = path
        if size % 2352 == 0 and head == SECTOR_SYNC:
            self.stride, self.offset = 2352, 24
        elif size % 2048 == 0 and size > 0:
            self.stride, self.offset = 2048, 0
        else:
            raise ValueError("unrecognised disc image: %s" % path)

    #----------------------
    # | read_sectors
    # | Description: Returns the user data of count consecutive sectors starting at lba; a read failure or a sector past the end raises ValueError.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: LOM_PS1_IMAGE_ENV
    # | Params: lba -- first sector, count -- sector count
    # | Returns: bytes of length count * 2048
    #----------------------
    def read_sectors(self, lba, count):
        out = bytearray()
        try:
            with open(self.path, "rb") as handle:
                for i in range(count):
                    handle.seek((lba + i) * self.stride + self.offset)
                    chunk = handle.read(2048)
                    if len(chunk) != 2048:
                        raise ValueError("sector %d is past the end of disc image %s" % (lba + i, self._source))
                    out += chunk
        except OSError as exc:
            raise ValueError("cannot read disc image %s (set %s to a valid path): %s" % (self._source, LOM_PS1_IMAGE_ENV, exc))
        return bytes(out)


#----------------------
# | bcd
# | Description: Decodes one binary-coded-decimal byte.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- BCD byte
# | Returns: integer value
#----------------------
def bcd(value):
    return (value >> 4) * 10 + (value & 0x0F)


#----------------------
# | parse_resource_table
# | Description: Parses SKCDPOSE.DAT: 8-byte entries of BCD minute, second, sector, live flag, then a little-endian size.
# | Author: suinevere
# | Dependencies: bcd
# | Globals: N/A
# | Params: blob -- table bytes
# | Returns: list of Resource in index order
#----------------------
def parse_resource_table(blob):
    table = []
    for index in range(len(blob) // 8):
        m, s, f, flag = blob[index * 8:index * 8 + 4]
        (size,) = struct.unpack_from("<i", blob, index * 8 + 4)
        lba = (bcd(m) * 60 + bcd(s)) * 75 + bcd(f) - 150
        table.append(Resource(index, lba, size, flag != 0))
    return table


#----------------------
# | read_resource
# | Description: Reads exactly a resource's bytes from the image.
# | Author: suinevere
# | Dependencies: DiscImage.read_sectors
# | Globals: N/A
# | Params: image -- DiscImage, res -- Resource
# | Returns: bytes of length res.size
#----------------------
def read_resource(image, res):
    sectors = (res.size + 2047) // 2048
    return image.read_sectors(res.lba, sectors)[:res.size]


#----------------------
# | PVD_LBA
# | Description: Fixed sector of the ISO9660 Primary Volume Descriptor.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PVD_LBA = 16

#----------------------
# | ROOT_DIRECTORY_RECORD_OFFSET
# | Description: Byte offset of the root directory record within the Primary Volume Descriptor.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ROOT_DIRECTORY_RECORD_OFFSET = 156


#----------------------
# | find_iso_file
# | Description: Finds a root-directory file by name, ignoring the ISO ;version suffix; used to confirm the disc's identity.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: PVD_LBA, ROOT_DIRECTORY_RECORD_OFFSET
# | Params: image -- DiscImage, name -- file name without the ;version suffix
# | Returns: (lba, size) tuple, or None when not found
#----------------------
def find_iso_file(image, name):
    pvd = image.read_sectors(PVD_LBA, 1)
    root_lba = struct.unpack_from("<I", pvd, ROOT_DIRECTORY_RECORD_OFFSET + 2)[0]
    root_size = struct.unpack_from("<I", pvd, ROOT_DIRECTORY_RECORD_OFFSET + 10)[0]
    data = image.read_sectors(root_lba, (root_size + 2047) // 2048)[:root_size]
    target = name.upper()
    pos = 0
    while pos < len(data):
        length = data[pos]
        if length == 0:
            pos += 1
            continue
        name_len = data[pos + 32]
        file_id = bytes(data[pos + 33:pos + 33 + name_len]).decode("ascii", "ignore").upper()
        if file_id.split(";")[0] == target:
            lba = struct.unpack_from("<I", data, pos + 2)[0]
            size = struct.unpack_from("<I", data, pos + 10)[0]
            return lba, size
        pos += length
    return None
