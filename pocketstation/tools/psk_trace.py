"""----------------------
 | psk_trace.py
 | Description: Reads, writes and renders PKTR1 trace files: per-sample LCD frames, button masks and work-RAM CRCs plus a final card CRC.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import argparse
import os
import struct
import zlib
from dataclasses import dataclass, field

"""----------------------
 | MAGIC
 | Description: File header of a trace.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
MAGIC = b"PKTR1"

"""----------------------
 | RECORD
 | Description: One sample: index, buttons, 128 bytes of VRAM, work-RAM CRC.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
RECORD = struct.Struct("<II128sI")


"""----------------------
 | Sample
 | Description: One trace sample.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class Sample:
    index: int
    buttons: int
    vram: bytes
    ram_crc: int


"""----------------------
 | Trace
 | Description: A whole trace: its samples and the CRC32 of the card image at the end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class Trace:
    samples: list = field(default_factory=list)
    card_crc: int = 0


"""----------------------
 | Divergence
 | Description: The first difference between two traces: sample index, kind (vram, ram, buttons, length, card) and detail.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
@dataclass
class Divergence:
    index: int
    kind: str
    detail: str


"""----------------------
 | compare
 | Description: Finds the first sample where two traces differ, checking buttons, then VRAM, then RAM, then length and card.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: a -- expected trace; b -- actual trace
 | Returns: None when equal, else a Divergence
 ----------------------"""
def compare(a: Trace, b: Trace):
    for sa, sb in zip(a.samples, b.samples):
        if sa.buttons != sb.buttons:
            return Divergence(sa.index, "buttons", "%02X != %02X" % (sa.buttons, sb.buttons))
        if sa.vram != sb.vram:
            return Divergence(sa.index, "vram", "frames differ")
        if sa.ram_crc != sb.ram_crc:
            return Divergence(sa.index, "ram", "%08X != %08X" % (sa.ram_crc, sb.ram_crc))
    if len(a.samples) != len(b.samples):
        n = min(len(a.samples), len(b.samples))
        return Divergence(n, "length", "%d != %d samples" % (len(a.samples), len(b.samples)))
    if a.card_crc != b.card_crc:
        return Divergence(len(a.samples), "card", "%08X != %08X" % (a.card_crc, b.card_crc))
    return None


"""----------------------
 | read
 | Description: Loads a PKTR1 file.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- trace file
 | Returns: Trace; raises ValueError on a malformed file
 ----------------------"""
def read(path: str) -> Trace:
    with open(path, "rb") as f:
        data = f.read()
    if data[:5] != MAGIC or (len(data) - 9) % RECORD.size:
        raise ValueError("not a PKTR1 trace: %s" % path)
    t = Trace()
    for off in range(5, len(data) - 4, RECORD.size):
        t.samples.append(Sample(*RECORD.unpack_from(data, off)))
    t.card_crc = struct.unpack_from("<I", data, len(data) - 4)[0]
    return t


"""----------------------
 | write
 | Description: Saves a Trace in PKTR1 format.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: trace -- the trace; path -- output file
 | Returns: N/A
 ----------------------"""
def write(trace: Trace, path: str) -> None:
    with open(path, "wb") as f:
        f.write(MAGIC)
        for s in trace.samples:
            f.write(RECORD.pack(s.index, s.buttons, s.vram, s.ram_crc))
        f.write(struct.pack("<I", trace.card_crc))


"""----------------------
 | pixel
 | Description: Whether a pixel of a 32x32 frame is lit; one 32-bit word per row, LSB is column 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: vram -- 128 bytes; x, y -- 0..31
 | Returns: bool
 ----------------------"""
def pixel(vram: bytes, x: int, y: int) -> bool:
    return bool(vram[y * 4 + (x >> 3)] >> (x & 7) & 1)


"""----------------------
 | ascii
 | Description: Renders a frame as 32 lines of '#' and '.'.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: vram -- 128 bytes
 | Returns: str
 ----------------------"""
def ascii(vram: bytes) -> str:
    return "\n".join("".join("#" if pixel(vram, x, y) else "." for x in range(32)) for y in range(32))


"""----------------------
 | _png_bytes
 | Description: Encodes a frame as a greyscale PNG scaled up, lit pixels dark as on the LCD.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: vram -- 128 bytes; scale -- pixel size
 | Returns: PNG file bytes
 ----------------------"""
def _png_bytes(vram: bytes, scale: int = 8) -> bytes:
    rows = b""
    for y in range(32 * scale):
        line = bytes(0x20 if pixel(vram, x // scale, y // scale) else 0xD0 for x in range(32 * scale))
        rows += b"\0" + line

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    size = 32 * scale
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 0, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


"""----------------------
 | png
 | Description: Writes the chosen samples of a trace as sample-NNNNN.png files.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: trace -- the trace; indices -- sample indices; out_dir -- output directory
 | Returns: list of written paths
 ----------------------"""
def png(trace: Trace, indices, out_dir: str) -> list:
    os.makedirs(out_dir, exist_ok=True)
    out = []
    for i in indices:
        path = os.path.join(out_dir, "sample-%05d.png" % i)
        with open(path, "wb") as f:
            f.write(_png_bytes(trace.samples[i].vram))
        out.append(path)
    return out


"""----------------------
 | main
 | Description: CLI: `png TRACE OUT [--count N]` writes N evenly spaced frames; `diff A B` prints the first divergence and both frames.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("png")
    p.add_argument("trace")
    p.add_argument("out")
    p.add_argument("--count", type=int, default=10)
    p = sub.add_parser("diff")
    p.add_argument("a")
    p.add_argument("b")
    args = ap.parse_args(argv)
    if args.cmd == "diff":
        a, b = read(args.a), read(args.b)
        d = compare(a, b)
        if d is None:
            print("traces match (%d samples)" % len(a.samples))
            return 0
        print("first divergence: sample %d: %s (%s)" % (d.index, d.kind, d.detail))
        if d.index < min(len(a.samples), len(b.samples)):
            la = ascii(a.samples[d.index].vram).splitlines()
            lb = ascii(b.samples[d.index].vram).splitlines()
            print("%-32s   %s" % ("expected", "actual"))
            for x, y in zip(la, lb):
                print("%s   %s" % (x, y))
        return 1
    if args.cmd == "png":
        t = read(args.trace)
        n = len(t.samples)
        idx = sorted({min(n - 1, i * n // args.count) for i in range(args.count)})
        for path in png(t, idx, args.out):
            print(path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
