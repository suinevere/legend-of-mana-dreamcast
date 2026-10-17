#----------------------
# | overlay_pointer_census.py
# | Description: Lists every aligned word in a secondary overlay's packed data that points into its own PS1 image, and which of them a pointer rule, function table or ignore entry already covers.
# | Author: suinevere
# | Dependencies: overlay_rules.py, convert_disc.py, build/us/overlays/<name>/<name>.raw, dreamcast/gen/*.ld
# | Globals: REPO, SECONDARY_VRAM
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import glob
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import convert_disc  # noqa: E402
import overlay_rules  # noqa: E402
import targets  # noqa: E402

#----------------------
# | REPO
# | Description: Repository root, two levels above this file.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

#----------------------
# | SECONDARY_VRAM
# | Description: PS1 address every secondary overlay loads at.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECONDARY_VRAM = 0x80140000


#----------------------
# | raw_path
# | Description: Path of an overlay's decompressed PS1 image, bss included, from the decomp build.
# | Author: suinevere
# | Dependencies: REPO
# | Globals: REPO
# | Params: name -- overlay name
# | Returns: file path
#----------------------
def raw_path(name):
    return os.path.join(REPO, "build", "us", "overlays", name, "%s.raw" % name)


#----------------------
# | spec_named
# | Description: The overlay_rules OverlaySpec with the given name.
# | Author: suinevere
# | Dependencies: overlay_rules.OVERLAYS
# | Globals: N/A
# | Params: name -- overlay name
# | Returns: OverlaySpec
#----------------------
def spec_named(name):
    return next(s for s in overlay_rules.OVERLAYS.values() if s.name == name)


#----------------------
# | load
# | Description: Reads an overlay's spec, decompressed image and symbol map.
# | Author: suinevere
# | Dependencies: spec_named, raw_path, convert_disc.load_spec_symbols
# | Globals: REPO
# | Params: name -- overlay name
# | Returns: (OverlaySpec, image bytes, dict of symbol name to PS1 address)
#----------------------
def load(name):
    spec = spec_named(name)
    with open(raw_path(name), "rb") as f:
        image = f.read()
    symbols = convert_disc.load_spec_symbols(spec, os.path.join(REPO, "config", "us", "symbols"))
    return spec, image, symbols


#----------------------
# | census
# | Description: Every aligned packed-data word whose value lies inside the overlay's PS1 image; "packed" values point into the packed data, "unpacked" ones at code or rodata the Dreamcast compiles from C.
# | Author: suinevere
# | Dependencies: load, overlay_rules.packed_offset
# | Globals: N/A
# | Params: name -- overlay name
# | Returns: list of (packed offset, value, kind)
#----------------------
def census(name):
    spec, image, _ = load(name)
    end = spec.vram + len(image)
    rows = []
    packed = 0
    for start, stop in spec.ranges:
        for addr in range(start + (-start % 4), stop - 3, 4):
            (value,) = struct.unpack_from("<I", image, addr - spec.vram)
            if spec.vram <= value < end:
                kind = "unpacked" if overlay_rules.packed_offset(value, spec) is None else "packed"
                rows.append((packed + addr - start, value, kind))
        packed += stop - start
    return rows


#----------------------
# | rule_offsets
# | Description: Packed offsets an overlay's POINTER_RULES entry relocates, recorded by running it with relocate_words swapped for a recorder.
# | Author: suinevere
# | Dependencies: overlay_rules.POINTER_RULES, overlay_rules.relocate_words, overlay_rules.symbol_offset
# | Globals: N/A
# | Params: spec -- OverlaySpec, image -- decompressed image, symbols -- symbol map
# | Returns: set of packed offsets
#----------------------
def rule_offsets(spec, image, symbols):
    rule = overlay_rules.POINTER_RULES.get(spec.name)
    if rule is None:
        return set()
    seen = set()
    original = overlay_rules.relocate_words
    overlay_rules.relocate_words = lambda buf, positions, s, t: seen.update(positions)
    try:
        buf = bytearray()
        for start, stop in spec.ranges:
            buf += image[start - spec.vram:stop - spec.vram]
        rule(buf, lambda n: overlay_rules.symbol_offset(symbols, spec, n), targets.DREAMCAST)
    finally:
        overlay_rules.relocate_words = original
    return seen


#----------------------
# | classified
# | Description: Packed offsets already handled: relocated by a pointer rule, inside a function table, or listed in IGNORED_POINTER_WORDS.
# | Author: suinevere
# | Dependencies: load, rule_offsets, overlay_rules.tables_for, overlay_rules.packed_offset, overlay_rules.IGNORED_POINTER_WORDS
# | Globals: N/A
# | Params: name -- overlay name
# | Returns: set of packed offsets
#----------------------
def classified(name):
    spec, image, symbols = load(name)
    out = rule_offsets(spec, image, symbols)
    for table, count in overlay_rules.tables_for(spec, image, symbols):
        base = overlay_rules.packed_offset(symbols[table], spec)
        out.update(base + 4 * i for i in range(count))
    out.update(overlay_rules.IGNORED_POINTER_WORDS.get(name, ()))
    return out


#----------------------
# | containing_symbol
# | Description: Name and offset of the symbol a PS1 address falls in, for readable census lines.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: symbols -- symbol map, addr -- PS1 address
# | Returns: "name+0xoff" or "?"
#----------------------
def containing_symbol(symbols, addr):
    best = None
    for name, value in symbols.items():
        if value <= addr and (best is None or value > best[1]):
            best = (name, value)
    return "?" if best is None else "%s+0x%X" % (best[0], addr - best[1])


#----------------------
# | packed_address
# | Description: PS1 address of a packed offset.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: spec -- OverlaySpec, offset -- packed offset
# | Returns: PS1 address
#----------------------
def packed_address(spec, offset):
    for start, stop in spec.ranges:
        if offset < stop - start:
            return start + offset
        offset -= stop - start
    raise ValueError("offset past the packed ranges")


#----------------------
# | linker_expressions
# | Description: Every "_name = expr;" assignment in the generated Dreamcast linker scripts.
# | Author: suinevere
# | Dependencies: dreamcast/gen/*.ld
# | Globals: REPO
# | Params: N/A
# | Returns: dict of name to expression
#----------------------
def linker_expressions():
    out = {}
    for path in glob.glob(os.path.join(REPO, "dreamcast", "gen", "*.ld")):
        with open(path) as f:
            for m in re.finditer(r"^_(\w+) = ([^;]+);", f.read(), re.M):
                out.setdefault(m.group(1), m.group(2))
    return out


#----------------------
# | field_references
# | Description: FIELD symbols whose PS1 address falls in an overlay's data or bss, which FIELD would read through a linker alias rather than the overlay's own symbol.
# | Author: suinevere
# | Dependencies: spec_named, raw_path, convert_disc.load_symbols
# | Globals: REPO
# | Params: name -- overlay name
# | Returns: sorted list of (FIELD symbol, PS1 address)
#----------------------
def field_references(name):
    spec = spec_named(name)
    lo = spec.ranges[0][0]
    hi = SECONDARY_VRAM + os.path.getsize(raw_path(name))
    field = convert_disc.load_symbols(os.path.join(REPO, "config", "us", "symbols", "field_symbol_addrs.txt"))
    return sorted((n, a) for n, a in field.items() if lo <= a < hi)


#----------------------
# | main
# | Description: Prints the census for one overlay, marking unclassified words, or with --field-refs the FIELD symbols inside its data and what each resolves to.
# | Author: suinevere
# | Dependencies: census, classified, load, containing_symbol, packed_address, field_references, linker_expressions
# | Globals: N/A
# | Params: argv -- CLI arguments
# | Returns: 0
#----------------------
def main(argv):
    parser = argparse.ArgumentParser(description="Secondary overlay pointer census.")
    parser.add_argument("--field-refs", action="store_true")
    parser.add_argument("name")
    args = parser.parse_args(argv)
    if args.field_refs:
        exprs = linker_expressions()
        for sym, addr in field_references(args.name):
            print("%s 0x%08X %s" % (sym, addr, exprs.get(sym, "(unresolved)")))
        return 0
    spec, _, symbols = load(args.name)
    done = classified(args.name)
    for offset, value, kind in census(args.name):
        print("0x%05X 0x%08X %-8s %-40s %s" % (offset, value, kind, containing_symbol(symbols, packed_address(spec, offset)),
                                             "ok" if offset in done else "UNCLASSIFIED"))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
