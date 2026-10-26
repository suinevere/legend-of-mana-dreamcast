#----------------------
# | vmu_flash.py
# | Description: Builds a 128 KB VMU flash image for the harness: a GAME from block 0 and optionally the pet save BASLUSP01013 as R2's kos_write stores it. --scatter interleaves a filler through the pet's blocks; --res builds R2's pet from RES1506.BIN.
# | Author: suinevere
# | Dependencies: dreamcast/tools/vmu_image.py, psk_file.py, struct
# | Globals: N/A
# | Params: OUT.bin GAME.vms [--pet PAYLOAD | --res RES1506.BIN] [--scatter] [--name NAME]
# | Returns: exit code 0, or 2 on a usage error
#----------------------
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "dreamcast", "tools"))

import vmu_image
import psk_file

#----------------------
# | PET / SHORT_DESC / APP_ID / ANIM_SPEED
# | Description: The pet save's PS1 name, and the package fields R2's card_store_kos.c sets: KOS_VMU_SHORT_DESC, KOS_VMU_APP_ID and icon_anim_speed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PET = "BASLUSP01013"
SHORT_DESC = "LEGEND OF MANA"
APP_ID = "LEGEND_OF_MANA"
ANIM_SPEED = 8

#----------------------
# | PS1_ICON_FLAG / PS1_CLUT / PS1_FRAMES / PS1_FRAME_BYTES / ICON_BYTES
# | Description: Offsets in a PS1 save's first block of the icon flag, palette and frames, a PS1 frame's size and a VMU icon frame's size, as vmu_format.c has them.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PS1_ICON_FLAG = 2
PS1_CLUT = 0x60
PS1_FRAMES = 0x80
PS1_FRAME_BYTES = 128
ICON_BYTES = 512

#----------------------
# | PET_FILE_BYTES / RT_SAVE_ANCHORS / RT_SAVE_COUNT_BLOCKS
# | Description: The size of R2's BASLUSP01013 file (six 8 KB blocks), the chain blocks rt_save keeps (every fourth, 22 of them) and the blocks its sector count is capped to.
# | Author: suinevere
# | Dependencies: pocketstation/src/vmu_rt/rt_file.s (rt_save_find)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PET_FILE_BYTES = 0xC000
RT_SAVE_ANCHORS = 22
RT_SAVE_COUNT_BLOCKS = 22


#----------------------
# | icon_from_ps1
# | Description: vmu_format.c's vmu_icon_from_ps1: the PS1 icon as an ARGB4444 palette (colour 0 transparent) and 32x32 frames doubled from 16x16, left pixel in the high nibble.
# | Author: suinevere
# | Dependencies: struct
# | Globals: PS1_ICON_FLAG, PS1_CLUT, PS1_FRAMES, PS1_FRAME_BYTES, ICON_BYTES
# | Params: ps1 -- the PS1 file's bytes
# | Returns: (frame count 1..3 or 0 without an "SC" header, 16 palette entries, frame bytes)
#----------------------
def icon_from_ps1(ps1):
    if ps1[:2] != b"SC":
        return 0, [0] * 16, b""
    count = ps1[PS1_ICON_FLAG] & 0x0F
    if not 1 <= count <= 3:
        count = 1
    palette = []
    for i in range(16):
        c, = struct.unpack_from("<H", ps1, PS1_CLUT + 2 * i)
        r, g, b = (c & 0x1F) >> 1, ((c >> 5) & 0x1F) >> 1, ((c >> 10) & 0x1F) >> 1
        palette.append(0 if c == 0 else 0xF000 | (r << 8) | (g << 4) | b)
    frames = bytearray(ICON_BYTES * count)
    for f in range(count):
        src = PS1_FRAMES + PS1_FRAME_BYTES * f
        for y in range(32):
            for x in range(0, 32, 2):
                byte = ps1[src + (y // 2) * 8 + (x // 2) // 2]
                pixel = (byte >> 4) if (x // 2) & 1 else (byte & 0x0F)
                frames[ICON_BYTES * f + y * 16 + x // 2] = (pixel << 4) | pixel
    return count, palette, bytes(frames)


#----------------------
# | package
# | Description: The VMS DATA package kos_write builds through KOS vmu_pkg_build: space-padded descriptions, the app id, the PS1 icon (or one blank frame), no eyecatch, the payload as stored_bytes keeps it, its size and the CRC.
# | Author: suinevere
# | Dependencies: icon_from_ps1, vmu_image.stored_bytes, vmu_image.vms_crc, struct
# | Globals: SHORT_DESC, APP_ID, ANIM_SPEED, ICON_BYTES
# | Params: ps1_name -- PS1 file name; payload -- the PS1 file
# | Returns: package bytes
#----------------------
def package(ps1_name, payload):
    count, palette, frames = icon_from_ps1(payload) if len(payload) >= 0x200 else (0, [0] * 16, b"")
    if count == 0:
        count, frames = 1, bytes(ICON_BYTES)
    header = bytearray(128)
    header[0:16] = SHORT_DESC.encode("ascii").ljust(16, b" ")
    header[16:48] = ps1_name.encode("ascii")[:32].ljust(32, b" ")
    header[48:64] = APP_ID.encode("ascii").ljust(16, b"\0")
    stored = vmu_image.stored_bytes(ps1_name, payload)
    struct.pack_into("<HHHHI", header, 0x40, count, ANIM_SPEED, 0, 0, len(stored))
    struct.pack_into("<I", header, vmu_image.RAW_LEN_OFFSET, len(payload))
    struct.pack_into("<16H", header, 0x60, *palette)
    data = bytearray(header + frames + stored)
    struct.pack_into("<H", data, 0x46, vmu_image.vms_crc(bytes(data)))
    return bytes(data)


#----------------------
# | r2_pet
# | Description: The pet save's payload as R2's CARDA writes it: RES1506.BIN with psk_file's reference pet transferred into it, zero-filled to the six-block file.
# | Author: suinevere
# | Dependencies: psk_file.pet_transfer, PET_FILE_BYTES
# | Globals: N/A
# | Params: res -- RES1506.BIN's bytes
# | Returns: PET_FILE_BYTES bytes
#----------------------
def r2_pet(res):
    pet = psk_file.REFERENCE_PET
    data = psk_file.pet_transfer(res, pet[0x16], pet[0x18], pet, psk_file.REFERENCE_LANDS)
    return data + bytes(PET_FILE_BYTES - len(data))


#----------------------
# | payload_offset
# | Description: Where package puts the payload: after the 128-byte header and one 512-byte frame per icon.
# | Author: suinevere
# | Dependencies: icon_from_ps1, ICON_BYTES
# | Globals: N/A
# | Params: payload -- the PS1 file
# | Returns: the payload's package offset
#----------------------
def payload_offset(payload):
    count = icon_from_ps1(payload)[0] if len(payload) >= 0x200 else 0
    return 128 + ICON_BYTES * max(count, 1)


#----------------------
# | rt_save_bytes
# | Description: The 24 bytes rt_save_find leaves in rt_save for an image: the sector count, sector 0's package page, then every fourth block of the pet's chain; zeros without a pet.
# | Author: suinevere
# | Dependencies: vmu_image, PET, RT_SAVE_ANCHORS, RT_SAVE_COUNT_BLOCKS
# | Globals: N/A
# | Params: img -- a flash image
# | Returns: 24 bytes
#----------------------
def rt_save_bytes(img):
    name = vmu_image.vmu_name_for_ps1(PET)
    if name not in [n for n, _ in vmu_image.files(img)]:
        return bytes(2 + RT_SAVE_ANCHORS)
    chain = vmu_image.chain(img, name)
    head = bytes(img[chain[0] * vmu_image.BLOCK:chain[0] * vmu_image.BLOCK + 128])
    page0 = 1 + 4 * struct.unpack_from("<H", head, 0x40)[0]
    length = struct.unpack_from("<I", head, 0x48)[0]
    sectors = 0xFF if length >= 0x8000 else length // 128
    cap = max(0, 4 * min(len(chain), RT_SAVE_COUNT_BLOCKS) - page0)
    anchors = [chain[4 * j] for j in range(RT_SAVE_ANCHORS) if 4 * j < len(chain)]
    anchors += [0] * (RT_SAVE_ANCHORS - len(anchors))
    return bytes([min(sectors, cap), page0] + anchors)


#----------------------
# | build
# | Description: The flash image: the GAME from block 0, then optionally a filler, as many blocks as leave the pet room, and the pet package with its payload set by set_ps1_file. The filler takes alternate top blocks, or with spread every third block below the pet's first spread blocks.
# | Author: suinevere
# | Dependencies: package, vmu_image
# | Globals: PET
# | Params: game -- GAME bytes; name -- GAME file name; pet -- pet payload or None; scatter -- interleave a filler; spread -- the pet's chain blocks kept whole before every third block goes to the filler, or None
# | Returns: 131072 bytes
#----------------------
def build(game, name, pet=None, scatter=False, spread=None):
    img = vmu_image.add_game(vmu_image.blank(), name, game)
    if pet is not None:
        pkg = package(PET, pet)
        if scatter:
            count = (len(pkg) + vmu_image.BLOCK - 1) // vmu_image.BLOCK
            room = vmu_image.free_blocks(img) - count
            if spread is None:
                gaps = [vmu_image.USER_BLOCKS - 1 - 2 * i for i in range(min(count, room))]
            else:
                gaps = [vmu_image.USER_BLOCKS - 3 - spread - 3 * i for i in range(min((count - spread) // 2, room))]
            img = vmu_image.add(img, "FILLER", b"\xA5" * (len(gaps) * vmu_image.BLOCK), blocks=gaps)
        img = vmu_image.add(img, vmu_image.vmu_name_for_ps1(PET), pkg)
        img = vmu_image.set_ps1_file(img, PET, pet)
    return img


#----------------------
# | main
# | Description: CLI: vmu_flash.py OUT.bin GAME.vms [--pet PAYLOAD | --res RES1506.BIN] [--scatter] [--name NAME]; the name defaults to the GAME file's stem in capitals.
# | Author: suinevere
# | Dependencies: build, r2_pet, argparse
# | Globals: N/A
# | Params: argv -- arguments, or None for sys.argv
# | Returns: 0
#----------------------
def main(argv=None):
    ap = argparse.ArgumentParser(description="Build a VMU flash image holding a GAME and optionally the pet save.")
    ap.add_argument("out")
    ap.add_argument("game")
    ap.add_argument("--pet")
    ap.add_argument("--res")
    ap.add_argument("--scatter", action="store_true")
    ap.add_argument("--name")
    args = ap.parse_args(argv)
    if args.scatter and not (args.pet or args.res):
        ap.error("--scatter needs --pet or --res")
    with open(args.game, "rb") as f:
        game = f.read()
    pet = None
    if args.pet:
        with open(args.pet, "rb") as f:
            pet = f.read()
    elif args.res:
        with open(args.res, "rb") as f:
            pet = r2_pet(f.read())
    name = args.name or os.path.splitext(os.path.basename(args.game))[0].upper()[:12]
    with open(args.out, "wb") as f:
        f.write(build(game, name, pet, args.scatter))
    return 0


if __name__ == "__main__":
    sys.exit(main())
