#----------------------
# | save_state.py
# | Description: Reads Mednafen Saturn save states: Work RAM in SH-2 order, VDP1 VRAM, linker-map symbols, the frame probe ring buffer and the render probe ring buffer.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: MDFN_MAGIC, WORK_HIGH_BASE, WORK_LOW_BASE, VDP1_VRAM_SIZE, FRAME_PROBE_SLOTS, FRAME_PROBE_SAMPLES, FRAME_PROBE_VBLANK_OFFSET, FRAME_PROBE_FRT_OFFSET, FRAME_PROBE_CURRENT_OFFSET, RENDER_PROBE_SLOTS, RENDER_PROBE_BUCKETS, RENDER_PROBE_TICKS_OFFSET, RENDER_PROBE_PRIMITIVES_OFFSET, RENDER_PROBE_MISSES_OFFSET, RENDER_PROBE_COMMANDS_OFFSET, RENDER_PROBE_DROPPED_OFFSET, RENDER_PROBE_CURRENT_OFFSET, RENDER_PROBE_WALK, RENDER_PROBE_LOOKUP, RENDER_PROBE_UPLOAD, RENDER_PROBE_EMIT, SYMBOL_LINE_RE
# | Params: N/A
# | Returns: N/A
#----------------------
import gzip
import re
import struct
import sys

#----------------------
# | MDFN_MAGIC
# | Description: The 8-byte header Mednafen writes at the start of a decompressed save state body.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MDFN_MAGIC = b"MDFNSVST"

#----------------------
# | WORK_HIGH_BASE
# | Description: SH-2 address of the start of Saturn high Work RAM (1 MB).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WORK_HIGH_BASE = 0x06000000

#----------------------
# | WORK_LOW_BASE
# | Description: SH-2 address of the start of Saturn low Work RAM (1 MB).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WORK_LOW_BASE = 0x00200000

#----------------------
# | VDP1_VRAM_SIZE
# | Description: Bytes in the VDP1 VRAM section of a Saturn save state (half a megabyte).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
VDP1_VRAM_SIZE = 0x80000

#----------------------
# | FRAME_PROBE_SLOTS
# | Description: Ring-buffer slot count in the FrameProbe struct.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FRAME_PROBE_SLOTS = 8

#----------------------
# | FRAME_PROBE_SAMPLES
# | Description: Samples stored per slot in each of the FrameProbe arrays.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FRAME_PROBE_SAMPLES = 5

#----------------------
# | FRAME_PROBE_VBLANK_OFFSET
# | Description: Byte offset of FrameProbe.vblank[8][5] (u32 each) from the probe's base address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FRAME_PROBE_VBLANK_OFFSET = 0x00

#----------------------
# | FRAME_PROBE_FRT_OFFSET
# | Description: Byte offset of FrameProbe.frt[8][5] (u16 each) from the probe's base address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FRAME_PROBE_FRT_OFFSET = 0xA0

#----------------------
# | FRAME_PROBE_CURRENT_OFFSET
# | Description: Byte offset of FrameProbe.current (u32 slot index) from the probe's base address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FRAME_PROBE_CURRENT_OFFSET = 0xF0

#----------------------
# | RENDER_PROBE_SLOTS
# | Description: Ring-buffer slot count in the RenderProbe struct.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_SLOTS = 8

#----------------------
# | RENDER_PROBE_BUCKETS
# | Description: Timed buckets stored per slot in RenderProbe.ticks.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_BUCKETS = 4

#----------------------
# | RENDER_PROBE_WALK
# | Description: Bucket index for the whole ordering-table walk, including the nested LOOKUP, UPLOAD and EMIT time.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_WALK = 0

#----------------------
# | RENDER_PROBE_LOOKUP
# | Description: Bucket index for texture cache lookup time.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_LOOKUP = 1

#----------------------
# | RENDER_PROBE_UPLOAD
# | Description: Bucket index for texture upload time.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_UPLOAD = 2

#----------------------
# | RENDER_PROBE_EMIT
# | Description: Bucket index for VDP1 command emit time.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_EMIT = 3

#----------------------
# | RENDER_PROBE_TICKS_OFFSET
# | Description: Byte offset of RenderProbe.ticks[8][4] (u32 each) from the probe's base address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_TICKS_OFFSET = 0x00

#----------------------
# | RENDER_PROBE_PRIMITIVES_OFFSET
# | Description: Byte offset of RenderProbe.primitives[8] (u32 each) from the probe's base address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_PRIMITIVES_OFFSET = 0x80

#----------------------
# | RENDER_PROBE_MISSES_OFFSET
# | Description: Byte offset of RenderProbe.misses[8] (u32 each) from the probe's base address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_MISSES_OFFSET = 0xA0

#----------------------
# | RENDER_PROBE_COMMANDS_OFFSET
# | Description: Byte offset of RenderProbe.commands[8] (u32 each) from the probe's base address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_COMMANDS_OFFSET = 0xC0

#----------------------
# | RENDER_PROBE_DROPPED_OFFSET
# | Description: Byte offset of RenderProbe.dropped[8] (u32 each) from the probe's base address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_DROPPED_OFFSET = 0xE0

#----------------------
# | RENDER_PROBE_CURRENT_OFFSET
# | Description: Byte offset of RenderProbe.current (u32 slot index) from the probe's base address.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RENDER_PROBE_CURRENT_OFFSET = 0x100

#----------------------
# | SYMBOL_LINE_RE
# | Description: Matches one linker-map line of the form "0xADDRESS NAME", ignoring surrounding whitespace.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SYMBOL_LINE_RE = re.compile(r"^\s*0x([0-9a-fA-F]+)\s+(\w+)\s*$")


#----------------------
# | _is_identifier_name
# | Description: Reports whether a byte string looks like a save-state section name: starts with a letter or underscore, all chars alnum or underscore.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: name_bytes -- candidate section name bytes
# | Returns: bool
#----------------------
def _is_identifier_name(name_bytes):
    if not name_bytes:
        return False
    first = name_bytes[0:1]
    if not (first.isalpha() or first == b"_"):
        return False
    for i in range(len(name_bytes)):
        c = name_bytes[i:i + 1]
        if not (c.isalnum() or c == b"_"):
            return False
    return True


#----------------------
# | sections
# | Description: Scans decompressed save-state bytes for every <len byte><name><u32 LE size><payload> section, skipping the MDFNSVST magic if present.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: MDFN_MAGIC
# | Params: data -- decompressed save-state bytes
# | Returns: list of (name, payload offset, size)
#----------------------
def sections(data):
    start = len(MDFN_MAGIC) if data[:len(MDFN_MAGIC)] == MDFN_MAGIC else 0
    found = []
    n = len(data)
    i = start
    while i < n:
        length = data[i]
        if 3 <= length <= 31 and i + 1 + length + 4 <= n:
            name_bytes = data[i + 1:i + 1 + length]
            if _is_identifier_name(name_bytes):
                size = struct.unpack_from("<I", data, i + 1 + length)[0]
                payload_off = i + 1 + length + 4
                if payload_off + size <= n:
                    found.append((name_bytes.decode("ascii"), payload_off, size))
        i += 1
    return found


#----------------------
# | _find_named_section
# | Description: Locates the first section with the given exact name via its length-prefixed marker and returns its payload bytes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: data -- decompressed save-state bytes; name -- section name bytes
# | Returns: payload bytes
#----------------------
def _find_named_section(data, name):
    marker = bytes([len(name)]) + name
    try:
        idx = data.index(marker)
    except ValueError:
        raise KeyError(name)
    size = struct.unpack_from("<I", data, idx + len(marker))[0]
    start = idx + len(marker) + 4
    return data[start:start + size]


#----------------------
# | _find_vdp1_vram
# | Description: Locates the first VRAM section whose size matches the VDP1 VRAM size and returns its payload, byte order as stored.
# | Author: suinevere
# | Dependencies: VDP1_VRAM_SIZE
# | Globals: N/A
# | Params: data -- decompressed save-state bytes
# | Returns: payload bytes
#----------------------
def _find_vdp1_vram(data):
    for name, offset, size in sections(data):
        if name == "VRAM" and size == VDP1_VRAM_SIZE:
            return data[offset:offset + size]
    raise KeyError("VRAM")


#----------------------
# | _byteswap_pairs
# | Description: Swaps adjacent byte pairs, converting a little-endian Work RAM dump back to SH-2 big-endian order.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: data -- little-endian Work RAM bytes
# | Returns: big-endian bytes, same length
#----------------------
def _byteswap_pairs(data):
    out = bytearray(data)
    out[0::2], out[1::2] = out[1::2], out[0::2]
    return bytes(out)


#----------------------
# | SaveState
# | Description: Holds a loaded save state's Work RAM (SH-2 order) and VDP1 VRAM, with address-mapped reads.
# | Author: suinevere
# | Dependencies: WORK_HIGH_BASE, WORK_LOW_BASE
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class SaveState:
    #----------------------
    # | SaveState.__init__
    # | Description: Stores the decoded Work RAM and VDP1 VRAM buffers for a loaded save state.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: work_high -- 1 MB high Work RAM, SH-2 order; work_low -- 1 MB low Work RAM, SH-2 order; vdp1_vram -- VDP1 VRAM payload, byte order as stored
    # | Returns: N/A
    #----------------------
    def __init__(self, work_high, work_low, vdp1_vram):
        self.work_high = work_high
        self.work_low = work_low
        self.vdp1_vram = vdp1_vram

    #----------------------
    # | SaveState._locate
    # | Description: Maps an SH-2 address to its backing buffer and byte offset, raising KeyError outside Work RAM.
    # | Author: suinevere
    # | Dependencies: WORK_HIGH_BASE, WORK_LOW_BASE
    # | Globals: N/A
    # | Params: addr -- SH-2 address
    # | Returns: (buffer, offset)
    #----------------------
    def _locate(self, addr):
        if WORK_HIGH_BASE <= addr < WORK_HIGH_BASE + len(self.work_high):
            return self.work_high, addr - WORK_HIGH_BASE
        if WORK_LOW_BASE <= addr < WORK_LOW_BASE + len(self.work_low):
            return self.work_low, addr - WORK_LOW_BASE
        raise KeyError(addr)

    #----------------------
    # | SaveState.u32
    # | Description: Reads a big-endian 32-bit word from Work RAM at an SH-2 address.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: addr -- SH-2 address
    # | Returns: unsigned 32-bit value
    #----------------------
    def u32(self, addr):
        buf, off = self._locate(addr)
        return struct.unpack_from(">I", buf, off)[0]

    #----------------------
    # | SaveState.u16
    # | Description: Reads a big-endian 16-bit word from Work RAM at an SH-2 address.
    # | Author: suinevere
    # | Dependencies: N/A
    # | Globals: N/A
    # | Params: addr -- SH-2 address
    # | Returns: unsigned 16-bit value
    #----------------------
    def u16(self, addr):
        buf, off = self._locate(addr)
        return struct.unpack_from(">H", buf, off)[0]


#----------------------
# | load
# | Description: Decompresses a Mednafen save state and decodes Work RAM (SH-2 order) and VDP1 VRAM into a SaveState.
# | Author: suinevere
# | Dependencies: SaveState, _find_named_section, _find_vdp1_vram, _byteswap_pairs
# | Globals: N/A
# | Params: path -- path to a gzip-compressed Mednafen save state
# | Returns: SaveState
#----------------------
def load(path):
    with open(path, "rb") as f:
        data = gzip.decompress(f.read())
    work_high = _byteswap_pairs(_find_named_section(data, b"WorkRAMH"))
    work_low = _byteswap_pairs(_find_named_section(data, b"WorkRAML"))
    vdp1_vram = _find_vdp1_vram(data)
    return SaveState(work_high, work_low, vdp1_vram)


#----------------------
# | map_symbols
# | Description: Parses a linker map's symbol lines ("0xADDRESS NAME") into a name-to-address dict.
# | Author: suinevere
# | Dependencies: SYMBOL_LINE_RE
# | Globals: N/A
# | Params: map_text -- full text of a linker map file
# | Returns: dict of symbol name to address
#----------------------
def map_symbols(map_text):
    symbols = {}
    for line in map_text.splitlines():
        match = SYMBOL_LINE_RE.match(line)
        if match:
            symbols[match.group(2)] = int(match.group(1), 16)
    return symbols


#----------------------
# | frame_probe
# | Description: Decodes a FrameProbe at addr: 8 slots of vblank[5] (u32) and frt[5] (u16), tagged with the current slot index.
# | Author: suinevere
# | Dependencies: FRAME_PROBE_SLOTS, FRAME_PROBE_SAMPLES, FRAME_PROBE_VBLANK_OFFSET, FRAME_PROBE_FRT_OFFSET, FRAME_PROBE_CURRENT_OFFSET
# | Globals: N/A
# | Params: state -- loaded SaveState; addr -- SH-2 address of the FrameProbe
# | Returns: list of dicts, one per slot: slot, vblank, frt, current
#----------------------
def frame_probe(state, addr):
    entries = []
    for slot in range(FRAME_PROBE_SLOTS):
        vblank = []
        for sample in range(FRAME_PROBE_SAMPLES):
            word_index = slot * FRAME_PROBE_SAMPLES + sample
            vblank.append(state.u32(addr + FRAME_PROBE_VBLANK_OFFSET + word_index * 4))
        frt = []
        for sample in range(FRAME_PROBE_SAMPLES):
            word_index = slot * FRAME_PROBE_SAMPLES + sample
            frt.append(state.u16(addr + FRAME_PROBE_FRT_OFFSET + word_index * 2))
        entries.append({"slot": slot, "vblank": vblank, "frt": frt})
    current = state.u32(addr + FRAME_PROBE_CURRENT_OFFSET)
    for entry in entries:
        entry["current"] = entry["slot"] == current
    return entries


#----------------------
# | render_probe
# | Description: Decodes a RenderProbe at addr: 8 slots of ticks[4] (u32, WALK/LOOKUP/UPLOAD/EMIT) plus primitives, misses, commands and dropped (u32 each), tagged with the current slot index and a computed walk_only.
# | Author: suinevere
# | Dependencies: RENDER_PROBE_SLOTS, RENDER_PROBE_BUCKETS, RENDER_PROBE_TICKS_OFFSET, RENDER_PROBE_PRIMITIVES_OFFSET, RENDER_PROBE_MISSES_OFFSET, RENDER_PROBE_COMMANDS_OFFSET, RENDER_PROBE_DROPPED_OFFSET, RENDER_PROBE_CURRENT_OFFSET, RENDER_PROBE_WALK, RENDER_PROBE_LOOKUP, RENDER_PROBE_UPLOAD, RENDER_PROBE_EMIT
# | Globals: N/A
# | Params: state -- loaded SaveState; addr -- SH-2 address of the RenderProbe
# | Returns: list of dicts, one per slot: slot, ticks, primitives, misses, commands, dropped, walk_only, current
#----------------------
def render_probe(state, addr):
    entries = []
    for slot in range(RENDER_PROBE_SLOTS):
        ticks = []
        for bucket in range(RENDER_PROBE_BUCKETS):
            word_index = slot * RENDER_PROBE_BUCKETS + bucket
            ticks.append(state.u32(addr + RENDER_PROBE_TICKS_OFFSET + word_index * 4))
        primitives = state.u32(addr + RENDER_PROBE_PRIMITIVES_OFFSET + slot * 4)
        misses = state.u32(addr + RENDER_PROBE_MISSES_OFFSET + slot * 4)
        commands = state.u32(addr + RENDER_PROBE_COMMANDS_OFFSET + slot * 4)
        dropped = state.u32(addr + RENDER_PROBE_DROPPED_OFFSET + slot * 4)
        walk_only = ticks[RENDER_PROBE_WALK] - ticks[RENDER_PROBE_LOOKUP] - ticks[RENDER_PROBE_UPLOAD] - ticks[RENDER_PROBE_EMIT]
        entries.append({
            "slot": slot,
            "ticks": ticks,
            "primitives": primitives,
            "misses": misses,
            "commands": commands,
            "dropped": dropped,
            "walk_only": walk_only,
        })
    current = state.u32(addr + RENDER_PROBE_CURRENT_OFFSET)
    for entry in entries:
        entry["current"] = entry["slot"] == current
    return entries


#----------------------
# | main
# | Description: CLI entry point; loads a save state and linker map, then prints the frame probe and, if present, the render probe and g_heap_probe_peak.
# | Author: suinevere
# | Dependencies: load, map_symbols, frame_probe, render_probe
# | Globals: N/A
# | Params: argv -- [prog, state_path, map_path]
# | Returns: process exit code
#----------------------
def main(argv):
    if len(argv) != 3:
        print("usage: save_state.py STATE MAP", file=sys.stderr)
        return 2
    state_path, map_path = argv[1], argv[2]
    state = load(state_path)
    with open(map_path, "r", encoding="utf-8", errors="replace") as f:
        symbols = map_symbols(f.read())
    frame_addr = symbols.get("g_frame_probe")
    if frame_addr is not None:
        print("frame_probe:")
        for entry in frame_probe(state, frame_addr):
            print("  %r" % entry)
    render_addr = symbols.get("g_render_probe")
    if render_addr is not None:
        print("render_probe:")
        for entry in render_probe(state, render_addr):
            print("  %r" % entry)
    heap_addr = symbols.get("g_heap_probe_peak")
    if heap_addr is not None:
        print("g_heap_probe_peak: %d" % state.u32(heap_addr))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
