"""----------------------
 | psk_file.py
 | Description: Parses PocketStation executable files and builds or reads 128 KB PS1 memory-card images holding them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

from dataclasses import dataclass

"""----------------------
 | RES_PATH
 | Description: Test-disc copy of CD resource 0x5E2, the Ring Ring Land executable.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
RES_PATH = "dreamcast/build-test/cd-data/RES/05/RES1506.BIN"

"""----------------------
 | FLASH1_BASE
 | Description: Address where the executing file is mapped linearly.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FLASH1_BASE = 0x02000000

"""----------------------
 | CARD_BYTES
 | Description: Size of a whole PS1 memory card image.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
CARD_BYTES = 0x20000

"""----------------------
 | BLOCK_BYTES
 | Description: Size of one memory card block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
BLOCK_BYTES = 0x2000

"""----------------------
 | FRAME_BYTES
 | Description: Size of one directory frame in block 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FRAME_BYTES = 0x80

"""----------------------
 | DEFAULT_NAME
 | Description: Directory filename for Ring Ring Land; the 7th character P marks a PocketStation executable.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
DEFAULT_NAME = "BASLUSP01013RRLAND"


"""----------------------
 | PskFile
 | Description: Header fields of a parsed PocketStation file plus its raw bytes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class PskFile:
    blocks: int
    title: str
    ident: str
    viewer_frames: int
    icon_list: int
    func_count: int
    entry: int
    body_offset: int
    data: bytes


"""----------------------
 | parse
 | Description: Decodes a PocketStation file header, computes the body offset and validates the entry point.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: data -- the whole file
 | Returns: PskFile; raises ValueError on a malformed file
 ----------------------"""
def parse(data: bytes) -> PskFile:
    if len(data) < 0x80 or data[0:2] != b"SC":
        raise ValueError("not a memory card file")
    ident = data[0x52:0x56].decode("ascii", "replace")
    if ident not in ("MCX0", "MCX1"):
        raise ValueError("not a PocketStation executable: %r" % ident)
    icons = data[2] & 0xF
    viewer_frames = int.from_bytes(data[0x50:0x52], "little")
    icon_list = data[0x56]
    func_count = data[0x57]
    entry = int.from_bytes(data[0x5C:0x60], "little")
    off = 0x80 + icons * 0x80
    if ident == "MCX1":
        off += 0x800
    off += (func_count * 8 + 0x7F) & ~0x7F
    off += viewer_frames * 0x80
    off += icon_list * 8
    if off >= len(data):
        raise ValueError("body offset 0x%X outside file" % off)
    target = (entry & ~1) - FLASH1_BASE
    if target < off or target >= len(data):
        raise ValueError("entry 0x%08X outside body" % entry)
    title = data[4:0x44].split(b"\0")[0].decode("shift_jis", "replace")
    return PskFile(
        blocks=(len(data) + BLOCK_BYTES - 1) // BLOCK_BYTES,
        title=title,
        ident=ident,
        viewer_frames=viewer_frames,
        icon_list=icon_list,
        func_count=func_count,
        entry=entry,
        body_offset=off,
        data=data,
    )


"""----------------------
 | _seal
 | Description: Writes the XOR checksum of a directory frame's first 0x7F bytes into its last byte.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: img -- card image; frame -- frame index 0..15
 | Returns: N/A
 ----------------------"""
def _seal(img: bytearray, frame: int) -> None:
    base = frame * FRAME_BYTES
    x = 0
    for b in img[base : base + 0x7F]:
        x ^= b
    img[base + 0x7F] = x


"""----------------------
 | card_image
 | Description: Builds a 128 KB memory card holding the files in order from block 1, with a valid directory.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: files -- list of (directory name, file bytes)
 | Returns: the card image; raises ValueError past 15 blocks
 ----------------------"""
def card_image(files: list[tuple[str, bytes]]) -> bytes:
    img = bytearray(CARD_BYTES)
    img[0:2] = b"MC"
    _seal(img, 0)
    block = 1
    for name, data in files:
        n = max(1, (len(data) + BLOCK_BYTES - 1) // BLOCK_BYTES)
        if block + n - 1 > 15:
            raise ValueError("files need more than 15 blocks")
        for i in range(n):
            f = block + i
            base = f * FRAME_BYTES
            state = 0x51 if i == 0 else (0x53 if i == n - 1 else 0x52)
            img[base : base + 4] = state.to_bytes(4, "little")
            if i == 0:
                img[base + 4 : base + 8] = (n * BLOCK_BYTES).to_bytes(4, "little")
                raw = name.encode("ascii")[:20]
                img[base + 0x0A : base + 0x0A + len(raw)] = raw
            link = 0xFFFF if i == n - 1 else f
            img[base + 8 : base + 10] = link.to_bytes(2, "little")
            _seal(img, f)
        img[block * BLOCK_BYTES : block * BLOCK_BYTES + len(data)] = data
        block += n
    for f in range(block, 16):
        base = f * FRAME_BYTES
        img[base : base + 4] = (0xA0).to_bytes(4, "little")
        img[base + 8 : base + 10] = (0xFFFF).to_bytes(2, "little")
        _seal(img, f)
    return bytes(img)


"""----------------------
 | TRANSFER_OFFSET
 | Description: File offset of the pet-transfer state the JP game writes over the SAVE DATA AREA marker.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
TRANSFER_OFFSET = 0x300

"""----------------------
 | SKIPPED_LANDS
 | Description: Land ids the JP transfer skips; they take no bit and no mana mask.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
SKIPPED_LANDS = frozenset({14, 20, 22, 28, 29})

"""----------------------
 | REFERENCE_PET
 | Description: A fixed 0x60-byte PetRecord used for test cards: named, species and egg species 1, level 5, modest stats.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
REFERENCE_PET = (
    bytes([0x10, 0x11, 0x12, 0x13]) + bytes(17)
    + bytes([1, 1, 0])
    + (5 | (120 << 8)).to_bytes(4, "little")
    + (60).to_bytes(2, "little") + (20).to_bytes(2, "little")
    + bytes(8)
    + b"".join((40 + 4 * i).to_bytes(2, "little") for i in range(8))
    + bytes(4) + bytes([0xFF, 0xFF, 0xFF, 0xFF]) + bytes(2)
    + (0).to_bytes(2, "little")
    + bytes([0xFF, 0xFF, 0xFF, 0x00])
    + bytes(4)
    + bytes([0x22] * 8) + bytes([0x22] * 4)
    + bytes(4)
    + (0x1234567).to_bytes(4, "little")
)

"""----------------------
 | REFERENCE_LANDS
 | Description: Placed lands for test cards: the first six plus a handful of later ones, as a mid-game save would have.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
REFERENCE_LANDS = frozenset({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12})


"""----------------------
 | pet_transfer
 | Description: Writes the JP game's pet-transfer state into a copy of the file, as card_prepare_pet_transfer does under VERSION_JP.
 | Author: suinevere
 | Dependencies: src/main/card_callbacks.c (layout)
 | Globals: N/A
 | Params: res -- file bytes; egg_species, level -- pet fields; pet -- 0x60-byte PetRecord; placed_lands -- land ids; maxed_mana -- land id to mask of levels at 6
 | Returns: the patched file bytes
 ----------------------"""
def pet_transfer(res: bytes, egg_species: int, level: int, pet: bytes, placed_lands, maxed_mana=None) -> bytes:
    if len(pet) != 0x60:
        raise ValueError("a PetRecord is 0x60 bytes")
    maxed_mana = maxed_mana or {}
    bits = 0
    bit = 0
    early = 0
    count = 0
    masks = bytearray()
    for land in range(1, 32):
        if land in SKIPPED_LANDS:
            continue
        on = land in placed_lands
        if on:
            bits |= 1 << bit
            count += 1
            if land < 7:
                early += 1
        bit += 1
        if land >= 7:
            masks.append(maxed_mana.get(land, 0) & 0xFF if on else 0)
    t = bytearray(0xB8)
    t[0x3C:0x40] = bytes([egg_species & 0xFF, level & 0xFF, early, count])
    t[0x40:0x44] = bits.to_bytes(4, "little")
    t[0x44:0x58] = masks
    t[0x58:0xB8] = pet
    out = bytearray(res)
    out[TRANSFER_OFFSET:TRANSFER_OFFSET + len(t)] = t
    return bytes(out)


"""----------------------
 | reference_card
 | Description: The card every reference trace runs on: Ring Ring Land holding REFERENCE_PET, as a JP game would have sent it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: res -- RES1506 bytes
 | Returns: 128 KB card image
 ----------------------"""
def reference_card(res: bytes) -> bytes:
    data = pet_transfer(res, REFERENCE_PET[0x16], REFERENCE_PET[0x18], REFERENCE_PET, REFERENCE_LANDS)
    return card_image([(DEFAULT_NAME, data)])


"""----------------------
 | card_files
 | Description: Reads every file back out of a card image by following directory links from each first block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: image -- 128 KB card image
 | Returns: list of (directory name, file bytes padded to whole blocks)
 ----------------------"""
def card_files(image: bytes) -> list[tuple[str, bytes]]:
    out = []
    for f in range(1, 16):
        base = f * FRAME_BYTES
        if image[base] != 0x51:
            continue
        name = image[base + 0x0A : base + 0x1F].split(b"\0")[0].decode("ascii", "replace")
        chunks = []
        cur = f
        while True:
            chunks.append(image[cur * BLOCK_BYTES : (cur + 1) * BLOCK_BYTES])
            link = int.from_bytes(image[cur * FRAME_BYTES + 8 : cur * FRAME_BYTES + 10], "little")
            if link == 0xFFFF or len(chunks) > 15:
                break
            cur = link + 1
        out.append((name, b"".join(chunks)))
    return out
