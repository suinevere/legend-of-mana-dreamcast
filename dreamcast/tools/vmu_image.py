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
