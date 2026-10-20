#----------------------
# | vmu_image.py
# | Description: Builds, reads and extends Flycast's 128 KB VMU images (vmu_save_A1.bin) so Dreamcast tests can seed a card and check what the game wrote.
# |              Layout: 200 user blocks, directory blocks 253..241, FAT at 254, root at 255; data files take blocks from 199 downward.
# | Author: suinevere
# | Dependencies: struct
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

#----------------------
# | BLOCK / BLOCKS / USER_BLOCKS / ROOT / FAT / DIR_TOP / DIR_BLOCKS / FREE / END / DATA_TYPE / ENTRY
# | Description: VMU block size and count, user blocks, the system block numbers, FAT free and chain-end markers, the data file type and the directory entry size.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BLOCK = 512
BLOCKS = 256
USER_BLOCKS = 200
ROOT = 255
FAT = 254
DIR_TOP = 253
DIR_BLOCKS = 13
FREE = 0xFFFC
END = 0xFFFA
DATA_TYPE = 0x33
ENTRY = 32

#----------------------
# | ROOT_HEAD
# | Description: The first 0x60 bytes of the root block a fresh Flycast v2.7 VMU carries: magic, colour, timestamp, then FAT, directory, icon and block-count fields.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ROOT_HEAD = bytes.fromhex(
    "55555555555555555555555555555555" "01ffffff640000000000000000000000"
    "00000000000000000000000000000000" "19981127000059040000000000000000"
    "ff000000ff00fe000100fd000d000500" "c8001f00000080000000000000000000")


#----------------------
# | _fat / _put_fat
# | Description: Read every FAT entry, or write them all back.
# | Author: suinevere
# | Dependencies: struct
# | Globals: FAT, BLOCK
# | Params: img -- image bytes (bytearray for _put_fat), table -- 256 entries
# | Returns: _fat returns a list of entries
#----------------------
def _fat(img):
    return list(struct.unpack_from("<256H", img, FAT * BLOCK))


def _put_fat(img, table):
    struct.pack_into("<256H", img, FAT * BLOCK, *table)


#----------------------
# | _entries
# | Description: Offsets of the directory's entry slots, top block first.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: DIR_TOP, DIR_BLOCKS, BLOCK, ENTRY
# | Params: N/A
# | Returns: list of byte offsets
#----------------------
def _entries():
    return [(DIR_TOP - b) * BLOCK + i * ENTRY for b in range(DIR_BLOCKS) for i in range(BLOCK // ENTRY)]


#----------------------
# | blank
# | Description: A freshly formatted VMU image the way Flycast makes one.
# | Author: suinevere
# | Dependencies: ROOT_HEAD, _put_fat
# | Globals: BLOCK, BLOCKS, ROOT, FAT, DIR_TOP, DIR_BLOCKS, USER_BLOCKS, FREE, END
# | Params: N/A
# | Returns: 131072 bytes
#----------------------
def blank():
    img = bytearray(BLOCK * BLOCKS)
    img[ROOT * BLOCK:ROOT * BLOCK + len(ROOT_HEAD)] = ROOT_HEAD
    table = [FREE] * BLOCKS
    table[ROOT] = END
    table[FAT] = END
    for b in range(DIR_TOP, DIR_TOP - DIR_BLOCKS, -1):
        table[b] = b - 1
    table[DIR_TOP - DIR_BLOCKS + 1] = END
    _put_fat(img, table)
    return bytes(img)


#----------------------
# | free_blocks
# | Description: Free user blocks in an image.
# | Author: suinevere
# | Dependencies: _fat
# | Globals: USER_BLOCKS, FREE
# | Params: img -- image bytes
# | Returns: block count
#----------------------
def free_blocks(img):
    return sum(1 for entry in _fat(img)[:USER_BLOCKS] if entry == FREE)


#----------------------
# | files
# | Description: Every data file in an image, in directory order, with its whole blocks of data.
# | Author: suinevere
# | Dependencies: _fat, _entries, struct
# | Globals: BLOCK, DATA_TYPE, END
# | Params: img -- image bytes
# | Returns: list of (name, bytes)
#----------------------
def files(img):
    table = _fat(img)
    out = []
    for off in _entries():
        if img[off] != DATA_TYPE:
            continue
        first, = struct.unpack_from("<H", img, off + 2)
        name = img[off + 4:off + 16].rstrip(b"\0 ").decode("ascii")
        size, = struct.unpack_from("<H", img, off + 0x18)
        data = bytearray()
        block = first
        for _ in range(size):
            data += img[block * BLOCK:(block + 1) * BLOCK]
            block = table[block]
            if block == END:
                break
        out.append((name, bytes(data)))
    return out


#----------------------
# | add
# | Description: Adds a data file, taking free user blocks from the top down and chaining them in the FAT.
# | Author: suinevere
# | Dependencies: _fat, _put_fat, _entries, struct
# | Globals: BLOCK, USER_BLOCKS, FREE, END, DATA_TYPE
# | Params: img -- image bytes, name -- VMU file name (up to 12 ASCII characters), data -- file bytes
# | Returns: new image bytes; raises ValueError when the card or directory is full
#----------------------
def add(img, name, data):
    img = bytearray(img)
    table = _fat(img)
    count = max(1, (len(data) + BLOCK - 1) // BLOCK)
    free = [b for b in range(USER_BLOCKS - 1, -1, -1) if table[b] == FREE]
    if count > len(free):
        raise ValueError("VMU image has %d free blocks, %s needs %d" % (len(free), name, count))
    slot = next((off for off in _entries() if img[off] == 0), None)
    if slot is None:
        raise ValueError("VMU directory is full")
    chain = free[:count]
    padded = bytes(data) + b"\0" * (count * BLOCK - len(data))
    for i, block in enumerate(chain):
        img[block * BLOCK:(block + 1) * BLOCK] = padded[i * BLOCK:(i + 1) * BLOCK]
        table[block] = chain[i + 1] if i + 1 < count else END
    _put_fat(img, table)
    entry = bytearray(32)
    entry[0] = DATA_TYPE
    struct.pack_into("<H", entry, 2, chain[0])
    entry[4:16] = name.encode("ascii")[:12].ljust(12, b"\0")
    entry[0x10:0x18] = bytes.fromhex("2026101712000005")
    struct.pack_into("<HH", entry, 0x18, count, 0)
    img[slot:slot + 32] = entry
    return bytes(img)


#----------------------
# | LOM_PS1_PRODUCT
# | Description: Product code vmu_format.c strips from PS1 names before naming the VMU file.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LOM_PS1_PRODUCT = "BASLUS-01013"


#----------------------
# | vmu_name_for_ps1
# | Description: VMU file name the Dreamcast build gives a PS1 card file, as vmu_format.c's vmu_name_for_ps1 does.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: LOM_PS1_PRODUCT
# | Params: ps1_name -- PS1 file name without the device
# | Returns: VMU file name, at most 12 characters
#----------------------
def vmu_name_for_ps1(ps1_name):
    if not ps1_name.startswith(LOM_PS1_PRODUCT):
        h = 2166136261
        for c in ps1_name.encode("ascii"):
            h = ((h ^ c) * 16777619) & 0xFFFFFFFF
        return "LOMX%08X" % h
    return ("LOM" + ps1_name[len(LOM_PS1_PRODUCT):].upper())[:12]


#----------------------
# | package_payload
# | Description: The data a VMS package carries after its 128-byte header and icon frames.
# | Author: suinevere
# | Dependencies: struct
# | Globals: N/A
# | Params: data -- package bytes
# | Returns: payload bytes
#----------------------
def package_payload(data):
    icons, _, _, _, data_len = struct.unpack_from("<HHHHI", data, 0x40)
    start = 128 + 512 * icons
    return bytes(data[start:start + data_len])


#----------------------
# | ps1_file
# | Description: The PS1 file the Dreamcast build stored under a PS1 name, unwrapped from its VMS package.
# | Author: suinevere
# | Dependencies: files, vmu_name_for_ps1, package_payload
# | Globals: N/A
# | Params: img -- image bytes, ps1_name -- PS1 file name
# | Returns: file bytes, or None when the image has no such file
#----------------------
def ps1_file(img, ps1_name):
    want = vmu_name_for_ps1(ps1_name)
    for name, data in files(img):
        if name == want:
            return package_payload(data)
    return None


#----------------------
# | vms_crc
# | Description: The CRC a VMS package header carries: CRC-16 with polynomial 0x1021 and initial value 0 (XMODEM) over the package with the CRC field zeroed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: data -- bytes
# | Returns: 16-bit CRC
#----------------------
def vms_crc(data):
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) if crc & 0x8000 else (crc << 1)
            crc &= 0xFFFF
    return crc


#----------------------
# | set_ps1_file
# | Description: Replaces the payload of the package stored under a PS1 name with bytes of the same length, refreshing its CRC, in the blocks it already holds.
# | Author: suinevere
# | Dependencies: _fat, _entries, vmu_name_for_ps1, vms_crc, struct
# | Globals: BLOCK, DATA_TYPE, END
# | Params: img -- image bytes, ps1_name -- PS1 file name, payload -- new file bytes
# | Returns: new image bytes; raises ValueError when the file is missing or the size differs
#----------------------
def set_ps1_file(img, ps1_name, payload):
    img = bytearray(img)
    want = vmu_name_for_ps1(ps1_name).encode("ascii")
    table = _fat(img)
    for off in _entries():
        if img[off] != DATA_TYPE or img[off + 4:off + 16].rstrip(b"\0 ") != want:
            continue
        first, = struct.unpack_from("<H", img, off + 2)
        size, = struct.unpack_from("<H", img, off + 0x18)
        chain = [first]
        while len(chain) < size and table[chain[-1]] != END:
            chain.append(table[chain[-1]])
        data = bytearray(b"".join(img[b * BLOCK:(b + 1) * BLOCK] for b in chain))
        icons, _, _, _, data_len = struct.unpack_from("<HHHHI", data, 0x40)
        if data_len != len(payload):
            raise ValueError("%s holds %d bytes, not %d" % (ps1_name, data_len, len(payload)))
        start = 128 + 512 * icons
        data[start:start + data_len] = payload
        struct.pack_into("<H", data, 0x46, 0)
        struct.pack_into("<H", data, 0x46, vms_crc(bytes(data[:start + data_len])))
        for i, block in enumerate(chain):
            img[block * BLOCK:(block + 1) * BLOCK] = data[i * BLOCK:(i + 1) * BLOCK]
        return bytes(img)
    raise ValueError("no %s on the VMU" % ps1_name)
