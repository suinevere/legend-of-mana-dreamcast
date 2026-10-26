"""----------------------
 | state_compare.py
 | Description: Compares a Phase 1 state dump (game_host --state) with a VMU one (vmu_runner --state) frame by frame, field by field through the RAM map's field list, and the VMU screen's middle 32 columns with the original's frame buffer fb at the same frame point.
 | Author: suinevere
 | Dependencies: gen_ram.py (fields JSON)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import json
import sys

"""----------------------
 | PS_FRAME / VMU_FRAME
 | Description: Bytes per frame: PocketStation RAM 0x200-0x7FF and 128 bytes of VRAM; vmu_ram and the 192-byte frame buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
PS_FRAME = 0x600 + 128
VMU_FRAME = 0x400 + 192

"""----------------------
 | PS_FB
 | Description: Where the original's frame buffer fb (RAM 0x240-0x2BF, one little-endian word per row, pixel x in bit x) sits in a PocketStation frame.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
PS_FB = 0x240 - 0x200

"""----------------------
 | SIZES
 | Description: Bytes per element of each map type, and whether it is signed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
SIZES = {"u8": (1, False), "s8": (1, True), "u16": (2, False), "s16": (2, True), "u32": (4, False)}

"""----------------------
 | HANDLERS
 | Description: The code addresses the original's interrupt slots hold, by the handler id the VMU game stores instead.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
HANDLERS = {0: 0, 0x02002F8C: 1, 0x02003E70: 2, 0x02003B64: 3, 0x020005A0: 4}

"""----------------------
 | ROM_BASE
 | Description: Card address of the game file's first byte.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
ROM_BASE = 0x02000000


"""----------------------
 | value
 | Description: One little-endian element of a frame's bytes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: buf -- frame bytes; at -- offset; type -- map type
 | Returns: the element
 ----------------------"""
def value(buf: bytes, at: int, type: str) -> int:
    size, signed = SIZES[type]
    return int.from_bytes(buf[at:at + size], "little", signed=signed)


"""----------------------
 | expected
 | Description: The VMU value a PocketStation value must become under a field's conversion.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ps -- PocketStation value; conv -- None, handler or file
 | Returns: the value, or None when the PocketStation value has no VMU form
 ----------------------"""
def expected(ps: int, conv):
    if conv == "handler":
        return HANDLERS.get(ps)
    if conv == "file":
        return ps - ROM_BASE if ps >= ROM_BASE else (0 if ps == 0 else None)
    return ps


"""----------------------
 | reverse8
 | Description: A byte's bits end for end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: b -- byte
 | Returns: reversed byte
 ----------------------"""
def reverse8(b: int) -> int:
    return int("{:08b}".format(b)[::-1], 2)


"""----------------------
 | lcd_bytes
 | Description: The VMU frame buffer the original's 32 row words should give: each row a blank byte, the word's four bytes with their bits reversed, a blank byte.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: rows -- 128 bytes of row words
 | Returns: 192 bytes
 ----------------------"""
def lcd_bytes(rows: bytes) -> bytes:
    return b"".join(b"\0" + bytes(REVERSED[b] for b in rows[y * 4:y * 4 + 4]) + b"\0" for y in range(32))


"""----------------------
 | REVERSED
 | Description: reverse8 of every byte.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
REVERSED = [reverse8(b) for b in range(256)]


"""----------------------
 | compare_frame
 | Description: The first difference in one frame: a field element (fields without both addresses or marked nocmp are skipped), or, when pixels is set, a pixel in VMU coordinates (x 8-39 against the original's fb, the other columns against blank). The VMU draws straight into its frame buffer, so its counterpart is fb, not the LCD a frame behind it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: fields -- field list; ps -- PocketStation frame; vmu -- VMU frame; pixels -- whether to compare the frame buffer
 | Returns: a description, or None when the frame matches
 ----------------------"""
def compare_frame(fields: list, ps: bytes, vmu: bytes, pixels: bool = True):
    for f in fields:
        if f["vmu"] is None or f["ps"] is None or f.get("nocmp"):
            continue
        psz, vsz = SIZES[f["type"]][0], SIZES[f["vmu_type"]][0]
        for i in range(f["count"]):
            p = value(ps, f["ps"] - 0x200 + i * psz, f["type"])
            v = value(vmu, f["vmu"] + i * vsz, f["vmu_type"])
            if expected(p, f["conv"]) != v:
                name = f["name"] if f["count"] == 1 else "%s[%d]" % (f["name"], i)
                return "field %s ps %d vmu %d" % (name, p, v)
    rows, fb = ps[PS_FB:PS_FB + 128], vmu[0x400:]
    if not pixels or fb == lcd_bytes(rows):
        return None
    for y in range(32):
        for x in range(48):
            on = fb[y * 6 + (x >> 3)] >> (7 - (x & 7)) & 1
            want = rows[y * 4 + ((x - 8) >> 3)] >> ((x - 8) & 7) & 1 if 8 <= x < 40 else 0
            if on != want:
                return "fb x %d y %d" % (x, y)
    return None


"""----------------------
 | compare
 | Description: Compares two state dumps frame by frame, leaving out the pixels of the frames the VMU does not draw: those where its cycle16 has just stepped to an odd value, as the main loop then keeps the last picture.
 | Author: suinevere
 | Dependencies: compare_frame
 | Globals: N/A
 | Params: fields -- field list; ps, vmu -- whole dumps
 | Returns: the result line and whether they are the same
 ----------------------"""
def compare(fields: list, ps: bytes, vmu: bytes):
    n, m = len(ps) // PS_FRAME, len(vmu) // VMU_FRAME
    cycle = next((f["vmu"] for f in fields if f["name"] == "cycle16"), None)
    for k in range(min(n, m)):
        frame = vmu[k * VMU_FRAME:(k + 1) * VMU_FRAME]
        skipped = cycle is not None and k > 0 and frame[cycle] & 1 and frame[cycle] != vmu[(k - 1) * VMU_FRAME + cycle]
        d = compare_frame(fields, ps[k * PS_FRAME:(k + 1) * PS_FRAME], frame, not skipped)
        if d:
            return "frame %d %s" % (k, d), False
    if n != m:
        return "frames ps %d vmu %d" % (n, m), False
    return "same %d" % n, True


"""----------------------
 | main
 | Description: CLI: state_compare.py FIELDS.json PS.state VMU.state; prints 'same <frames>' or the first difference.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: 0 when the same, 1 on a difference, 2 on usage
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) != 3:
        print("usage: state_compare.py FIELDS.json PS.state VMU.state", file=sys.stderr)
        return 2
    with open(argv[0]) as f:
        fields = json.load(f)
    with open(argv[1], "rb") as f:
        ps = f.read()
    with open(argv[2], "rb") as f:
        vmu = f.read()
    line, same = compare(fields, ps, vmu)
    print(line)
    return 0 if same else 1


if __name__ == "__main__":
    sys.exit(main())
