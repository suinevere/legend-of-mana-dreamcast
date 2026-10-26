#----------------------
# | vmu_image.py
# | Description: Builds, reads and extends Flycast's 128 KB VMU images (vmu_save_A1.bin) so Dreamcast tests can seed a card and check what the game wrote.
# |              Layout: 200 user blocks, directory blocks 253..241, FAT at 254, root at 255; data files take blocks from 199 downward, a GAME blocks from 0 upward.
# | Author: suinevere
# | Dependencies: struct
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

#----------------------
# | BLOCK / BLOCKS / USER_BLOCKS / ROOT / FAT / DIR_TOP / DIR_BLOCKS / FREE / END / DATA_TYPE / GAME_TYPE / ENTRY
# | Description: VMU block size and count, user blocks, the system block numbers, FAT free and chain-end markers, the data and GAME file types and the directory entry size.
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
GAME_TYPE = 0xCC
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
# | _chain
# | Description: The blocks of the file whose directory entry is at an offset, in FAT order.
# | Author: suinevere
# | Dependencies: struct
# | Globals: END
# | Params: img -- image bytes, table -- FAT entries, off -- directory entry offset
# | Returns: list of block numbers
#----------------------
def _chain(img, table, off):
    block, = struct.unpack_from("<H", img, off + 2)
    size, = struct.unpack_from("<H", img, off + 0x18)
    out = []
    for _ in range(size):
        out.append(block)
        block = table[block]
        if block == END:
            break
    return out


#----------------------
# | files
# | Description: Every data and GAME file in an image, in directory order, with its whole blocks of data.
# | Author: suinevere
# | Dependencies: _fat, _entries, _chain
# | Globals: BLOCK, DATA_TYPE, GAME_TYPE
# | Params: img -- image bytes
# | Returns: list of (name, bytes)
#----------------------
def files(img):
    table = _fat(img)
    out = []
    for off in _entries():
        if img[off] not in (DATA_TYPE, GAME_TYPE):
            continue
        name = img[off + 4:off + 16].rstrip(b"\0 ").decode("ascii")
        out.append((name, b"".join(bytes(img[b * BLOCK:(b + 1) * BLOCK]) for b in _chain(img, table, off))))
    return out


#----------------------
# | chain
# | Description: The blocks a named data or GAME file holds, in FAT order.
# | Author: suinevere
# | Dependencies: _fat, _entries, _chain
# | Globals: DATA_TYPE, GAME_TYPE
# | Params: img -- image bytes, name -- VMU file name
# | Returns: list of block numbers; raises ValueError when there is no such file
#----------------------
def chain(img, name):
    table = _fat(img)
    for off in _entries():
        if img[off] in (DATA_TYPE, GAME_TYPE) and img[off + 4:off + 16].rstrip(b"\0 ").decode("ascii") == name:
            return _chain(img, table, off)
    raise ValueError("no %s on the VMU" % name)


#----------------------
# | _place
# | Description: Writes a file into the given blocks, chains them in the FAT and fills the first free directory slot.
# | Author: suinevere
# | Dependencies: _fat, _put_fat, _entries, struct
# | Globals: BLOCK, END
# | Params: img -- image bytearray, name -- VMU file name, data -- file bytes, blocks -- blocks in chain order, kind -- file type byte, header -- header block offset
# | Returns: N/A; raises ValueError when the directory is full
#----------------------
def _place(img, name, data, blocks, kind, header):
    table = _fat(img)
    slot = next((off for off in _entries() if img[off] == 0), None)
    if slot is None:
        raise ValueError("VMU directory is full")
    count = len(blocks)
    padded = bytes(data) + b"\0" * (count * BLOCK - len(data))
    for i, block in enumerate(blocks):
        img[block * BLOCK:(block + 1) * BLOCK] = padded[i * BLOCK:(i + 1) * BLOCK]
        table[block] = blocks[i + 1] if i + 1 < count else END
    _put_fat(img, table)
    entry = bytearray(32)
    entry[0] = kind
    struct.pack_into("<H", entry, 2, blocks[0])
    entry[4:16] = name.encode("ascii")[:12].ljust(12, b"\0")
    entry[0x10:0x18] = bytes.fromhex("2026101712000005")
    struct.pack_into("<HH", entry, 0x18, count, header)
    img[slot:slot + 32] = entry


#----------------------
# | add
# | Description: Adds a data file, taking free user blocks from the top down, or the given blocks, and chaining them in the FAT.
# | Author: suinevere
# | Dependencies: _fat, _place
# | Globals: BLOCK, USER_BLOCKS, FREE, DATA_TYPE
# | Params: img -- image bytes, name -- VMU file name (up to 12 ASCII characters), data -- file bytes, blocks -- optional free blocks in chain order
# | Returns: new image bytes; raises ValueError when the card or directory is full or a given block is taken
#----------------------
def add(img, name, data, blocks=None):
    img = bytearray(img)
    table = _fat(img)
    count = max(1, (len(data) + BLOCK - 1) // BLOCK)
    free = [b for b in range(USER_BLOCKS - 1, -1, -1) if table[b] == FREE]
    if blocks is not None:
        if len(blocks) != count or any(b not in free for b in blocks):
            raise ValueError("%s needs %d free blocks, not %s" % (name, count, list(blocks)))
        free = list(blocks)
    if count > len(free):
        raise ValueError("VMU image has %d free blocks, %s needs %d" % (len(free), name, count))
    _place(img, name, data, free[:count], DATA_TYPE, 0)
    return bytes(img)


#----------------------
# | add_game
# | Description: Adds a GAME in contiguous blocks from block 0 up, its header in block 1, the way the BIOS runs a GAME from flash address 0.
# | Author: suinevere
# | Dependencies: _fat, _entries, _place
# | Globals: BLOCK, USER_BLOCKS, FREE, GAME_TYPE
# | Params: img -- image bytes, name -- VMU file name (up to 12 ASCII characters), data -- the GAME image
# | Returns: new image bytes; raises ValueError when a GAME is already there or its blocks are taken
#----------------------
def add_game(img, name, data):
    img = bytearray(img)
    if any(img[off] == GAME_TYPE for off in _entries()):
        raise ValueError("VMU image already holds a GAME")
    table = _fat(img)
    count = max(1, (len(data) + BLOCK - 1) // BLOCK)
    if count > USER_BLOCKS or any(table[b] != FREE for b in range(count)):
        raise ValueError("blocks 0..%d are not free for %s" % (count - 1, name))
    _place(img, name, data, list(range(count)), GAME_TYPE, 1)
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
# | PACK_WINDOW / PACK_MIN_MATCH / PACK_MAX_MATCH / PACK_LEN_EXTEND / RAW_LEN_OFFSET
# | Description: save_pack.c's LZSS limits and extension nibble, and the VMS header word card_store_kos.c keeps a PS1 file's uncompressed size in.
# | Author: suinevere
# | Dependencies: src/card/save_pack.c, src/card/card_store_kos.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PACK_WINDOW = 4096
PACK_MIN_MATCH = 3
PACK_MAX_MATCH = 4096
PACK_LEN_EXTEND = 15
RAW_LEN_OFFSET = 0x4C


#----------------------
# | save_pack
# | Description: Compresses bytes into save_pack.c's LZSS format, greedily taking the longest match among the last 64 positions sharing a 3-byte prefix.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: PACK_WINDOW, PACK_MIN_MATCH, PACK_MAX_MATCH, PACK_LEN_EXTEND
# | Params: data -- bytes
# | Returns: compressed bytes
#----------------------
def save_pack(data):
    out = bytearray()
    seen = {}
    pos = 0
    flag_at = 0
    bit = 8
    while pos < len(data):
        if bit == 8:
            flag_at = len(out)
            out.append(0)
            bit = 0
        best, dist = 0, 0
        limit = min(len(data) - pos, PACK_MAX_MATCH)
        if limit >= PACK_MIN_MATCH:
            for cand in reversed(seen.get(data[pos:pos + 3], [])[-64:]):
                if pos - cand > PACK_WINDOW:
                    break
                n = 0
                while n < limit and data[cand + n] == data[pos + n]:
                    n += 1
                if n > best:
                    best, dist = n, pos - cand
                    if n == limit:
                        break
        step = best if best >= PACK_MIN_MATCH else 1
        if step == 1:
            out[flag_at] |= 1 << bit
            out.append(data[pos])
        else:
            nib = min(best - PACK_MIN_MATCH, PACK_LEN_EXTEND)
            out += bytes(((dist - 1) & 0xFF, (((dist - 1) >> 4) & 0xF0) | nib))
            if nib == PACK_LEN_EXTEND:
                rest = best - PACK_MIN_MATCH - nib
                while True:
                    out.append(min(rest, 255))
                    if rest < 255:
                        break
                    rest -= 255
        for i in range(pos, pos + step):
            if i + 3 <= len(data):
                seen.setdefault(data[i:i + 3], []).append(i)
        pos += step
        bit += 1
    return bytes(out)


#----------------------
# | save_unpack
# | Description: Decompresses save_pack.c's LZSS format into exactly size bytes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: PACK_MIN_MATCH, PACK_LEN_EXTEND
# | Params: data -- compressed bytes, size -- uncompressed size
# | Returns: bytes; raises ValueError on a corrupt stream
#----------------------
def save_unpack(data, size):
    out = bytearray()
    i = 0
    flags = 0
    bit = 8
    try:
        while len(out) < size:
            if bit == 8:
                flags = data[i]
                i += 1
                bit = 0
            if flags & (1 << bit):
                out.append(data[i])
                i += 1
            else:
                dist = (data[i] | ((data[i + 1] & 0xF0) << 4)) + 1
                nib = data[i + 1] & 0x0F
                length = nib + PACK_MIN_MATCH
                i += 2
                if nib == PACK_LEN_EXTEND:
                    while True:
                        step = data[i]
                        i += 1
                        length += step
                        if step != 255:
                            break
                if dist > len(out) or len(out) + length > size:
                    raise ValueError("corrupt LZSS stream")
                for _ in range(length):
                    out.append(out[-dist])
            bit += 1
    except IndexError:
        raise ValueError("truncated LZSS stream")
    return bytes(out)


#----------------------
# | PET_PS1_NAME / PET_KEPT_BYTES
# | Description: Ring Ring Land's PS1 file and the prefix of it card_store_kos.c keeps uncompressed: CARDA reads only its first 0x400 bytes.
# | Author: suinevere
# | Dependencies: src/card/vmu_format.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PET_PS1_NAME = "BASLUSP01013"
PET_KEPT_BYTES = 0x400


#----------------------
# | kept_bytes
# | Description: vmu_format.c's vmu_kept_bytes: how many leading bytes of a PS1 file are stored uncompressed in place of the whole, or 0 to store it whole.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: PET_PS1_NAME, PET_KEPT_BYTES
# | Params: ps1_name -- PS1 file name, size -- the file's byte count
# | Returns: bytes kept, or 0
#----------------------
def kept_bytes(ps1_name, size):
    return PET_KEPT_BYTES if ps1_name.startswith(PET_PS1_NAME) and size > PET_KEPT_BYTES else 0


#----------------------
# | stored_bytes
# | Description: What card_store_kos.c stores for a PS1 file: its kept prefix, else the file LZSS-compressed unless that saves nothing.
# | Author: suinevere
# | Dependencies: kept_bytes, save_pack
# | Globals: N/A
# | Params: ps1_name -- PS1 file name, payload -- file bytes
# | Returns: stored bytes
#----------------------
def stored_bytes(ps1_name, payload):
    kept = kept_bytes(ps1_name, len(payload))
    if kept:
        return bytes(payload[:kept])
    packed = save_pack(payload)
    return packed if len(packed) < len(payload) else bytes(payload)


#----------------------
# | package_payload
# | Description: The PS1 file a VMS package carries after its 128-byte header and icon frames, decompressed unless its stored length equals the uncompressed size at RAW_LEN_OFFSET; a kept prefix comes back zero-filled.
# | Author: suinevere
# | Dependencies: kept_bytes, save_unpack, struct
# | Globals: RAW_LEN_OFFSET
# | Params: data -- package bytes
# | Returns: payload bytes
#----------------------
def package_payload(data):
    icons, _, _, _, data_len = struct.unpack_from("<HHHHI", data, 0x40)
    raw_len, = struct.unpack_from("<I", data, RAW_LEN_OFFSET)
    start = 128 + 512 * icons
    stored = bytes(data[start:start + data_len])
    desc = bytes(data[16:48]).rstrip(b" \0").decode("ascii", "replace")
    kept = kept_bytes(desc, raw_len)
    if data_len == raw_len:
        return stored
    if kept and data_len == kept:
        return stored + bytes(raw_len - kept)
    return save_unpack(stored, raw_len)


#----------------------
# | build_package
# | Description: A VMS package as card_store_kos.c builds one: the long description, icon frames, the payload as stored_bytes keeps it, its uncompressed size and the CRC.
# | Author: suinevere
# | Dependencies: stored_bytes, vms_crc, struct
# | Globals: RAW_LEN_OFFSET
# | Params: desc_long -- long description, payload -- PS1 file bytes, icons -- icon frame bytes, a multiple of 512
# | Returns: package bytes
#----------------------
def build_package(desc_long, payload, icons=b"\0" * 512):
    stored = stored_bytes(desc_long, payload)
    header = bytearray(128)
    header[0:16] = b"LEGEND OF MANA".ljust(16)
    header[16:48] = desc_long.encode("ascii")[:32].ljust(32)
    struct.pack_into("<HHHHI", header, 0x40, len(icons) // 512, 8, 0, 0, len(stored))
    struct.pack_into("<I", header, RAW_LEN_OFFSET, len(payload))
    package = bytearray(bytes(header) + icons + stored)
    struct.pack_into("<H", package, 0x46, vms_crc(bytes(package)))
    return bytes(package)


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
# | Description: Replaces the PS1 file stored under a PS1 name with new bytes, keeping its header and icons, recompressing it and growing or shrinking its block chain to fit.
# | Author: suinevere
# | Dependencies: _fat, _put_fat, _entries, _chain, vmu_name_for_ps1, stored_bytes, vms_crc, struct
# | Globals: BLOCK, DATA_TYPE, END, FREE, USER_BLOCKS, RAW_LEN_OFFSET
# | Params: img -- image bytes, ps1_name -- PS1 file name, payload -- new file bytes
# | Returns: new image bytes; raises ValueError when the file is missing or the card has no room
#----------------------
def set_ps1_file(img, ps1_name, payload):
    img = bytearray(img)
    want = vmu_name_for_ps1(ps1_name).encode("ascii")
    table = _fat(img)
    for off in _entries():
        if img[off] != DATA_TYPE or img[off + 4:off + 16].rstrip(b"\0 ") != want:
            continue
        blocks = _chain(img, table, off)
        old = b"".join(bytes(img[b * BLOCK:(b + 1) * BLOCK]) for b in blocks)
        icons, = struct.unpack_from("<H", old, 0x40)
        start = 128 + 512 * icons
        stored = stored_bytes(ps1_name, payload)
        data = bytearray(old[:start] + stored)
        struct.pack_into("<I", data, 0x48, len(stored))
        struct.pack_into("<I", data, RAW_LEN_OFFSET, len(payload))
        struct.pack_into("<H", data, 0x46, 0)
        struct.pack_into("<H", data, 0x46, vms_crc(bytes(data)))
        count = (len(data) + BLOCK - 1) // BLOCK
        for block in blocks[count:]:
            table[block] = FREE
        blocks = blocks[:count]
        free = [b for b in range(USER_BLOCKS - 1, -1, -1) if table[b] == FREE and b not in blocks]
        if count > len(blocks) + len(free):
            raise ValueError("no room for %s" % ps1_name)
        blocks += free[:count - len(blocks)]
        data += b"\0" * (count * BLOCK - len(data))
        for i, block in enumerate(blocks):
            img[block * BLOCK:(block + 1) * BLOCK] = data[i * BLOCK:(i + 1) * BLOCK]
            table[block] = blocks[i + 1] if i + 1 < count else END
        _put_fat(img, table)
        struct.pack_into("<H", img, off + 0x18, count)
        return bytes(img)
    raise ValueError("no %s on the VMU" % ps1_name)
