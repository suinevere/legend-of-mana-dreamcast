#----------------------
# | main_data.py
# | Description: Generates gen/main_data.c: storage for main-executable symbols the C build never defines, copied from SLUS_010.13 where they lie in its data and zeroed where they lie in its BSS, laid out range by range so gen_pending can alias PS1 addresses into it.
# | Author: suinevere
# | Dependencies: akao_data.py (load_symbols), libgte_tables.py (read_iso_file, EXE_NAME), disc_image.py, config/us/symbols/*.txt
# | Globals: N/A
# | Params: --image PATH --out DIR
# | Returns: 0
#----------------------
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import akao_data  # noqa: E402

#----------------------
# | MAIN_SYMBOLS
# | Description: Main-executable symbols referenced by C code but defined nowhere in it (g_field_marker_overlay_enabled in the SLUS data, the rest in its BSS).
# | Author: suinevere
# | Dependencies: config/us/symbols/*.txt
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAIN_SYMBOLS = ("g_field_marker_overlay_enabled", "g_game_diagnostic_status", "g_game_diagnostic_code",
                "g_game_diagnostic_arg0", "g_game_diagnostic_arg1", "g_last_error_status")

#----------------------
# | SPAN_CAP
# | Description: Largest span given one symbol when the next known symbol is further away; g_field_marker_overlay_enabled is s32[2].
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SPAN_CAP = 0x40

#----------------------
# | EXE_LOAD_BASE / EXE_HEADER_SIZE / POINTER_LOW / POINTER_HIGH
# | Description: The PS-X EXE's load address and header size, and the address window a copied word must not fall in unless reviewed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXE_LOAD_BASE = 0x80010000
EXE_HEADER_SIZE = 0x800
POINTER_LOW = 0x80010000
POINTER_HIGH = 0x80200000

#----------------------
# | POINTER_FREE_REVIEWED
# | Description: (start, end) ranges whose pointer-looking words were checked by hand and hold no pointers.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
POINTER_FREE_REVIEWED = ()


#----------------------
# | symbol_spans
# | Description: Each main symbol's address and size: up to the next known US symbol, capped at SPAN_CAP, rounded up to 2 bytes.
# | Author: suinevere
# | Dependencies: akao_data.load_symbols, MAIN_SYMBOLS, SPAN_CAP
# | Globals: N/A
# | Params: N/A
# | Returns: list of (name, address, size)
#----------------------
def symbol_spans():
    by_name, by_address = akao_data.load_symbols()
    addresses = sorted(by_address)
    spans = []
    for name in MAIN_SYMBOLS:
        addr = by_name[name]
        following = [a for a in addresses if a > addr]
        size = min(following[0] - addr if following else SPAN_CAP, SPAN_CAP)
        spans.append((name, addr, (size + 1) & ~1))
    return spans


#----------------------
# | ranges
# | Description: The spans merged into word-aligned PS1 ranges, joining gaps under 0x100.
# | Author: suinevere
# | Dependencies: symbol_spans
# | Globals: N/A
# | Params: N/A
# | Returns: list of (start, end)
#----------------------
def ranges():
    out = []
    for _, addr, size in sorted(symbol_spans(), key=lambda s: s[1]):
        start, end = addr & ~3, (addr + size + 3) & ~3
        if out and start - out[-1][1] < 0x100:
            out[-1] = (out[-1][0], max(out[-1][1], end))
        else:
            out.append((start, end))
    return out


#----------------------
# | block_offset
# | Description: Offset of a PS1 address inside lom_main_data.
# | Author: suinevere
# | Dependencies: ranges
# | Globals: N/A
# | Params: addr -- PS1 address
# | Returns: byte offset, or None outside every range
#----------------------
def block_offset(addr):
    base = 0
    for start, end in ranges():
        if start <= addr < end:
            return base + addr - start
        base += end - start
    return None


#----------------------
# | exe_end
# | Description: End address of the executable's text and data as loaded.
# | Author: suinevere
# | Dependencies: EXE_LOAD_BASE
# | Globals: N/A
# | Params: exe -- SLUS bytes
# | Returns: address
#----------------------
def exe_end(exe):
    load, size = struct.unpack_from("<2I", exe, 0x18)
    return load + size


#----------------------
# | block_bytes
# | Description: One range's initial bytes: the SLUS's where it holds data, zeros past its end; refuses pointer-looking words in unreviewed ranges.
# | Author: suinevere
# | Dependencies: exe_end, POINTER_FREE_REVIEWED
# | Globals: N/A
# | Params: exe -- SLUS bytes, start, end -- range
# | Returns: bytes
#----------------------
def block_bytes(exe, start, end):
    load = struct.unpack_from("<I", exe, 0x18)[0]
    out = bytearray(end - start)
    limit = exe_end(exe)
    for addr in range(start, min(end, limit)):
        out[addr - start] = exe[addr - load + EXE_HEADER_SIZE]
    reviewed = any(a <= start and end <= b for a, b in POINTER_FREE_REVIEWED)
    for i in range(0, len(out) - 3, 4):
        word = struct.unpack_from("<I", out, i)[0]
        if POINTER_LOW <= word < POINTER_HIGH and not reviewed:
            raise ValueError("pointer-looking word 0x%08X at 0x%08X" % (word, start + i))
    return bytes(out)


#----------------------
# | generate_from_exe
# | Description: The text of gen/main_data.c for an executable.
# | Author: suinevere
# | Dependencies: ranges, block_bytes, symbol_spans
# | Globals: N/A
# | Params: exe -- SLUS bytes
# | Returns: C source text
#----------------------
def generate_from_exe(exe):
    data = b"".join(block_bytes(exe, start, end) for start, end in ranges())
    lines = [
        "/*----------------------",
        " | main_data.c",
        " | Description: Generated by port/tools/main_data.py from the user's SLUS_010.13: storage for %s, ranges %s back to back."
        % (", ".join(name for name, _, _ in symbol_spans()), ", ".join("0x%08X-0x%08X" % r for r in ranges())),
        " | Author: suinevere",
        " | Dependencies: port/tools/main_data.py, gen/dc_pending.ld (address aliases into the block)",
        " | Globals: lom_main_data",
        " | Params: N/A",
        " | Returns: N/A",
        " ----------------------*/",
        "unsigned char lom_main_data[0x%X] __attribute__((aligned(4))) = {" % len(data),
    ]
    for i in range(0, len(data), 16):
        lines.append("    " + " ".join("0x%02X," % b for b in data[i:i + 16]))
    lines.append("};")
    return "\n".join(lines) + "\n"


#----------------------
# | buffer_ranges
# | Description: gen_pending buffer-range tuples aliasing each range's PS1 addresses into lom_main_data.
# | Author: suinevere
# | Dependencies: ranges
# | Globals: N/A
# | Params: N/A
# | Returns: tuple of (start, size, linker expression)
#----------------------
def buffer_ranges():
    out = []
    base = 0
    for start, end in ranges():
        out.append((start, end - start, "_lom_main_data + 0x%X" % base if base else "_lom_main_data"))
        base += end - start
    return tuple(out)


#----------------------
# | main
# | Description: Command-line entry: writes gen/main_data.c under --out.
# | Author: suinevere
# | Dependencies: generate_from_exe, libgte_tables.read_iso_file, disc_image.DiscImage
# | Globals: N/A
# | Params: argv -- argument list
# | Returns: process exit code
#----------------------
def main(argv=None):
    import disc_image
    import libgte_tables

    parser = argparse.ArgumentParser(description="Generate storage for undefined main-executable symbols")
    parser.add_argument("--image", default=os.environ.get("LOM_PS1_IMAGE", os.path.join(akao_data.REPO, "disc", "us", "LegendOfMana.bin")))
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    exe = libgte_tables.read_iso_file(disc_image.DiscImage(args.image), libgte_tables.EXE_NAME)
    os.makedirs(os.path.join(args.out, "gen"), exist_ok=True)
    with open(os.path.join(args.out, "gen", "main_data.c"), "w", encoding="utf-8", newline="\n") as handle:
        handle.write(generate_from_exe(exe))
    return 0


if __name__ == "__main__":
    sys.exit(main())
