
#----------------------
# | gen_pending.py
# | Description: Generates halting C stubs and linker aliases for what the Dreamcast objects reference but nothing defines; run it with `make -C dreamcast pending`.
# | Author: suinevere
# | Dependencies: sh-elf-nm, overlay_rules.py, config/us/SLUS_010.13.yaml, config/us/overlays/{FIELD,GNAME}.BIN.yaml, config/us/symbols/*.txt
# | Globals: KNOWN_SDK_FUNCTIONS, CODE_YAMLS, SECONDARY_OVERLAY_RANGE, BUFFER_RANGES, DREAMCAST_BUFFER_RANGES, DREAMCAST_BENIGN, OVERLAY_LABELS, PLATFORM
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import akao_data  # noqa: E402
import main_data  # noqa: E402
import overlay_rules  # noqa: E402
import targets  # noqa: E402

#----------------------
# | CODE_YAMLS
# | Description: Splat configs, relative to the repository root, whose first segment's code range marks a referenced address as a function.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CODE_YAMLS = (
    "config/us/SLUS_010.13.yaml",
    "config/us/overlays/FIELD.BIN.yaml",
    "config/us/overlays/GNAME.BIN.yaml",
)

#----------------------
# | SECONDARY_OVERLAY_RANGE
# | Description: PS1 addresses shared by every secondary overlay (GNAME, GOSUB, CLOAD, MOVIE...), where an equal address does not mean the same symbol.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECONDARY_OVERLAY_RANGE = (0x80140000, 0x80180000)

#----------------------
# | DREAMCAST_FIELD_SLOTS
# | Description: Where a Dreamcast overlay linked against FIELD resolves names inside FIELD's packed data: FIELD's own Dreamcast data region, resident whenever such an overlay runs.
# | Author: suinevere
# | Dependencies: slot_address
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST_FIELD_SLOTS = (("dreamcast", 2),)

#----------------------
# | OVERLAY_LABELS
# | Description: Per --overlay run, the output file stem and the overlay names its generated headers and halt messages use.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAY_LABELS = {"dreamcast": ("dc", "the Dreamcast main executable, CHECKPS and TITLE", "Dreamcast")}


#----------------------
# | slot_address
# | Description: Maps a PS1 address inside a ("dreamcast", key) overlay's packed ranges to that overlay's Dreamcast data region, exactly as the converter packs it.
# | Author: suinevere
# | Dependencies: overlay_rules.packed_offset, overlay_rules.OVERLAYS, targets.DREAMCAST
# | Globals: N/A
# | Params: addr -- PS1 address, overlays -- ("dreamcast", overlay_rules.OVERLAYS key) pairs to search
# | Returns: region address, or None outside every packed range
#----------------------
def slot_address(addr, overlays=()):
    for _, key in overlays:
        spec = overlay_rules.OVERLAYS[key]
        offset = overlay_rules.packed_offset(addr, spec)
        if offset is not None:
            return targets.DREAMCAST.regions[spec.name].data_base + offset
    return None


#----------------------
# | BUFFER_RANGES
# | Description: PS1 regions (start, size, buffer symbol) that lom_prefix.h redirects to port buffers of the same layout.
# | Author: suinevere
# | Dependencies: ../port/psyq/lom_prefix.h, dreamcast/src/boot/main_globals_dc.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BUFFER_RANGES = (
    (0x801ED000, 0x1000, "_lom_resident_state"),
    (0x80180000, 0x800, "_lom_load_buffer"),
    (0x8013C000, 0x100, "_lom_sound_bank"),
    (0x801E1200, 0xBE00, "_lom_font_buffer"),
    (0x1F800000, 0x400, "_lom_scratchpad"),
    (0x801DE000, 0x3000, "_lom_text_cache"),
)

#----------------------
# | DREAMCAST_BUFFER_RANGES
# | Description: BUFFER_RANGES for the Dreamcast build, whose lom_sound_bank covers the full PS1 bank area TITLE copies into, plus AKAO's PS1 data and BSS spans, which gen/akao_data.c lays out as on the PS1, and the main symbols gen/main_data.c gives storage.
# | Author: suinevere
# | Dependencies: BUFFER_RANGES, dreamcast/src/boot/main_globals_dc.c, akao_data.py (gen/akao_data.c), main_data.py (gen/main_data.c)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST_BUFFER_RANGES = tuple((start, 0x4000 if symbol == "_lom_sound_bank" else size, symbol)
                                for start, size, symbol in BUFFER_RANGES) + (
    (akao_data.DATA_START, akao_data.DATA_END - akao_data.DATA_START, "_lom_akao_data"),
    (akao_data.BSS_START, akao_data.BSS_END - akao_data.BSS_START, "_lom_akao_bss"),
) + main_data.buffer_ranges()

#----------------------
# | KNOWN_SDK_FUNCTIONS
# | Description: Psy-Q library function names that are classified as functions when no symbol file lists them.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
KNOWN_SDK_FUNCTIONS = frozenset({
    "rsin", "rcos", "ratan2", "csin", "ccos", "SquareRoot0", "RotMatrix", "VectorNormal",
    "ApplyMatrixLV", "CompMatrix", "ScaleMatrix",
})

#----------------------
# | SYMBOL_LINE
# | Description: Matches a splat symbol_addrs line, capturing the name, the address and the trailing comment.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SYMBOL_LINE = re.compile(r"^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;(.*)$")

#----------------------
# | SIZED_SYMBOL
# | Description: Matches a symbol_addrs comment marking a sized symbol that other names may point inside, capturing its size.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SIZED_SYMBOL = re.compile(r"size:(0x[0-9A-Fa-f]+)\s+allow_addend:True")

#----------------------
# | SPLAT_AUTO_NAME
# | Description: Matches splat's address-derived names, which no symbol file lists; func_ names are code and the rest are data.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SPLAT_AUTO_NAME = re.compile(r"^(func|D|jtbl)_[0-9A-Fa-f]{8}$")

#----------------------
# | LD_ASSIGNMENT
# | Description: Matches a top-level linker-script symbol assignment, capturing the name.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LD_ASSIGNMENT = re.compile(r"^\s*(\w+)\s*=[^=]")

#----------------------
# | LINKER_SCRIPT_SUFFIXES
# | Description: File suffixes of --defined paths read as linker scripts rather than passed to nm.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LINKER_SCRIPT_SUFFIXES = (".ld", ".linker")


#----------------------
# | UnknownSymbolError
# | Description: Raised when referenced names are in no symbol file and are neither known SDK functions nor splat auto names.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class UnknownSymbolError(Exception):
    pass


#----------------------
# | parse_symbol_lines
# | Description: Maps each symbol name to its address and whether any of its lines carries type:func.
# | Author: suinevere
# | Dependencies: SYMBOL_LINE
# | Globals: N/A
# | Params: symbol_lines -- iterable of symbol_addrs lines
# | Returns: dict of name to (address, is_func)
#----------------------
def parse_symbol_lines(symbol_lines):
    symbols = {}
    for line in symbol_lines:
        match = SYMBOL_LINE.match(line)
        if not match:
            continue
        name, addr, rest = match.group(1), int(match.group(2), 16), match.group(3)
        is_func = "type:func" in rest
        if name in symbols:
            old_addr, old_func = symbols[name]
            symbols[name] = (old_addr, old_func or is_func)
        else:
            symbols[name] = (addr, is_func)
    return symbols


#----------------------
# | classify
# | Description: Splits names into functions (type:func, a code-range address, a known SDK function or a splat func_ name) and data.
# | Author: suinevere
# | Dependencies: parse_symbol_lines, KNOWN_SDK_FUNCTIONS, SPLAT_AUTO_NAME
# | Globals: N/A
# | Params: names -- symbol names, symbol_lines -- symbol_addrs lines, code_ranges -- (start, end) pairs, end exclusive
# | Returns: (sorted function names, sorted data names); raises UnknownSymbolError
#----------------------
def classify(names, symbol_lines, code_ranges):
    symbols = parse_symbol_lines(symbol_lines)
    functions, data, unknown = [], [], []
    for name in sorted(set(names)):
        if name not in symbols:
            auto = SPLAT_AUTO_NAME.match(name)
            if name in KNOWN_SDK_FUNCTIONS or (auto and auto.group(1) == "func"):
                functions.append(name)
            elif auto:
                data.append(name)
            else:
                unknown.append(name)
            continue
        addr, is_func = symbols[name]
        if is_func or any(start <= addr < end for start, end in code_ranges):
            functions.append(name)
        else:
            data.append(name)
    if unknown:
        raise UnknownSymbolError("symbols in no symbol file: %s" % ", ".join(unknown))
    return functions, data


#----------------------
# | resolve_aliases
# | Description: Maps names (splat auto names by their own address) to an equal-address defined symbol outside the secondary range.
# |              Or an offset into a defined size:/allow_addend symbol that contains them, a databin slot address, or a redirected buffer.
# |              A function (classify's rule over code_ranges) is never placed inside a container, since another overlay's data can span its address.
# | Author: suinevere
# | Dependencies: SYMBOL_LINE, SIZED_SYMBOL, SPLAT_AUTO_NAME, SECONDARY_OVERLAY_RANGE, slot_address, DREAMCAST_BUFFER_RANGES, classify
# | Globals: N/A
# | Params: names -- undefined names, symbol_lines -- symbol_addrs lines, targets -- names that linked objects or slot scripts define, slot_overlays -- ("dreamcast", key) overlays whose packed ranges may resolve a name, buffer_ranges -- redirected-buffer table, code_ranges -- (start, end) code ranges that make a name a function
# | Returns: dict of name to linker expression ("_target", "_container + offset", a hex slot address or "_buffer + offset")
#----------------------
def resolve_aliases(names, symbol_lines, targets, slot_overlays=(), buffer_ranges=DREAMCAST_BUFFER_RANGES, code_ranges=()):
    addresses, by_address, containers = {}, {}, set()
    for line in symbol_lines:
        match = SYMBOL_LINE.match(line)
        if match:
            name, addr = match.group(1), int(match.group(2), 16)
            addresses.setdefault(name, set()).add(addr)
            by_address.setdefault(addr, set()).add(name)
            sized = SIZED_SYMBOL.search(match.group(3))
            if sized and name in targets:
                containers.add((addr, int(sized.group(1), 16), name))
    aliases = {}
    known = [name for name in set(names) if name in addresses or SPLAT_AUTO_NAME.match(name)]
    functions = set(classify(known, symbol_lines, code_ranges)[0])
    for name in sorted(set(names)):
        own = addresses.get(name)
        if own is None and SPLAT_AUTO_NAME.match(name):
            own = {int(name[-8:], 16)}
        own = sorted(own or ())
        for addr in own:
            if SECONDARY_OVERLAY_RANGE[0] <= addr < SECONDARY_OVERLAY_RANGE[1]:
                continue
            candidates = sorted(other for other in by_address.get(addr, ()) if other != name and other in targets)
            if candidates:
                aliases[name] = "_" + candidates[0]
                break
        if name in aliases or name in functions:
            continue
        for addr in own:
            container = next(((other, addr - start) for start, size, other in sorted(containers)
                              if other != name and start < addr < start + size), None)
            if container is not None:
                aliases[name] = "_%s + 0x%X" % container
                break
        if name in aliases:
            continue
        for addr in own:
            slot = slot_address(addr, slot_overlays)
            if slot is not None:
                aliases[name] = "0x%08X" % slot
                break
            buffer = next(((symbol, addr - start) for start, size, symbol in buffer_ranges if start <= addr < start + size), None)
            if buffer is not None:
                aliases[name] = "%s + 0x%X" % buffer
                break
    return aliases


#----------------------
# | DREAMCAST_BENIGN
# | Description: Pending Dreamcast functions GNAME's path reaches with no input once FIELD got its own overlay (Task 6); McxCardType is the PS1's memory-card probe, absent hardware on the Dreamcast.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST_BENIGN = {
    "McxCardType": "zero",
    "cdrom_set_audio_volume": "void",

}

#----------------------
# | PLATFORM
# | Description: The generator command named in the outputs, the system header, the halt call and its message.
# | Author: suinevere
# | Dependencies: dreamcast/src/system/dc_system.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PLATFORM = ("port/tools/gen_pending.py (make -C dreamcast pending)", "dc_system.h", "dc_halt", "pending: %(name)s")


#----------------------
# | UnmatchedBenignNameError
# | Description: Raised when render() is given a BENIGN name that matches none of its pending function names.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class UnmatchedBenignNameError(Exception):
    pass


#----------------------
# | render_benign_function
# | Description: Builds one BENIGN stub: logs once through a static-flag-guarded dc_log call, then returns void, zero or its second argument unchanged.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: name -- function name, kind -- "void", "zero" or "arg:<param>", overlay -- OVERLAY_LABELS key, selecting the logging call
# | Returns: C source text for the one function
#----------------------
def render_benign_function(name, kind):
    log_dep = "dc_log"
    log_line = ("    static int logged;\n    if (!logged) { logged = 1; dc_log(\"field: %s skipped\"); }\n" % name)
    if kind == "void":
        signature = "void lom_pending_%s(void)" % name
        body = log_line
    elif kind == "zero":
        signature = "int lom_pending_%s(void)" % name
        body = log_line + "    return 0;\n"
    elif kind.startswith("arg:"):
        param = kind[len("arg:"):]
        signature = "void *lom_pending_%s(void *lom_pending_unused0, void *%s)" % (name, param)
        body = log_line + "    return %s;\n" % param
    else:
        raise ValueError("unknown BENIGN kind for %s: %s" % (name, kind))
    header = (
        "\n/*----------------------\n"
        " | %s\n"
        " | Description: Benign; Home's path reaches it with no input, so it logs once and returns safely instead of halting.\n"
        " | Author: suinevere\n"
        " | Dependencies: %s\n"
        " | Globals: N/A\n"
        " | Params: N/A\n"
        " | Returns: N/A\n"
        " ----------------------*/\n" % (name, log_dep)
    )
    return (
        header
        + "%s __asm__(\"_%s\");\n%s\n{\n%s}\n" % (signature, name, signature, body)
    )


#----------------------
# | render
# | Description: Builds <overlay>_pending.c (a halting stub per function, or a BENIGN function's safe stub) and <overlay>_pending.ld (resolved aliases, then one trap-buffer alias per data symbol).
# | Author: suinevere
# | Dependencies: render_benign_function, OVERLAY_LABELS
# | Globals: N/A
# | Params: functions -- function names, data -- data names, aliases -- dict of name to linker expression, benign -- dict of name to BENIGN kind, overlay -- OVERLAY_LABELS key
# | Returns: (C source text, linker script text); raises UnmatchedBenignNameError
#----------------------
def render(functions, data, aliases=None, benign=None, overlay="dreamcast"):
    stem, callers, halt = OVERLAY_LABELS[overlay]
    if benign:
        unmatched = sorted(set(benign) - set(functions))
        if unmatched:
            raise UnmatchedBenignNameError("BENIGN names with no matching pending function: %s" % ", ".join(unmatched))
    tool, system_header, halt_call, halt_text = PLATFORM
    c_parts = [
        "/*----------------------\n"
        " | %s_pending.c\n"
        " | Description: Generated by %s; halting stubs for functions %s call%s that are not ported yet, except BENIGN's safe stubs.\n"
        " | Author: suinevere\n"
        " | Dependencies: %s\n"
        " | Globals: N/A\n"
        " | Params: N/A\n"
        " | Returns: N/A\n"
        " ----------------------*/\n"
        "#include \"%s\"\n" % (stem, tool, callers, "" if " and " in callers else "s", system_header, system_header)
    ]
    for name in sorted(functions):
        if benign and name in benign:
            c_parts.append(render_benign_function(name, benign[name]))
            continue
        c_parts.append(
            "\n/*----------------------\n"
            " | %s\n"
            " | Description: Not ported yet; halts with its name, linked as %s through an asm label so no header prototype conflicts.\n"
            " | Author: suinevere\n"
            " | Dependencies: %s\n"
            " | Globals: N/A\n"
            " | Params: N/A\n"
            " | Returns: never\n"
            " ----------------------*/\n"
            "void lom_pending_%s(void) __asm__(\"_%s\");\n"
            "void lom_pending_%s(void)\n"
            "{\n"
            "    %s(\"%s\");\n"
            "}\n" % (name, name, halt_call, name, name, name, halt_call, halt_text % {"overlay": halt, "name": name})
        )
    ld_parts = [
        "/*----------------------\n"
        " | %s_pending.ld\n"
        " | Description: Generated by %s; aliases %s references to equal-address, slot or redirected-buffer symbols, else to the trap buffer.\n"
        " | Author: suinevere\n"
        " | Dependencies: %s (lom_trap_buffer)\n"
        " | Globals: lom_trap_buffer\n"
        " | Params: N/A\n"
        " | Returns: N/A\n"
        " ----------------------*/\n" % (stem, tool, callers, "src/boot/main_globals_dc.c")
    ]
    for name, target in sorted((aliases or {}).items()):
        ld_parts.append("_%s = %s;\n" % (name, target))
    for name in sorted(data):
        ld_parts.append("_%s = _lom_trap_buffer;\n" % name)
    return "".join(c_parts), "".join(ld_parts)


#----------------------
# | parse_nm
# | Description: Reads nm output into undefined and defined global names with the SH ELF leading underscore removed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: lines -- nm output lines
# | Returns: (set of undefined names, set of defined names)
#----------------------
def parse_nm(lines):
    undefined, defined = set(), set()
    for line in lines:
        parts = line.split()
        if len(parts) < 2:
            continue
        kind, name = parts[-2], parts[-1]
        if name.startswith("_"):
            name = name[1:]
        if kind == "U":
            undefined.add(name)
        elif kind in ("w", "v"):
            continue
        else:
            defined.add(name)
    return undefined, defined


#----------------------
# | parse_ld_defined
# | Description: Collects the names a linker script assigns, with the SH ELF leading underscore removed.
# | Author: suinevere
# | Dependencies: LD_ASSIGNMENT
# | Globals: N/A
# | Params: text -- linker script text
# | Returns: set of names
#----------------------
def parse_ld_defined(text):
    names = set()
    for line in text.splitlines():
        match = LD_ASSIGNMENT.match(line)
        if match:
            name = match.group(1)
            names.add(name[1:] if name.startswith("_") else name)
    return names


#----------------------
# | code_range
# | Description: Reads a splat config's code range, from its first c subsegment with an explicit offset to its first data or databin subsegment.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: yaml_path -- splat yaml of the main executable or an overlay
# | Returns: (start, end) PS1 addresses, end exclusive
#----------------------
def code_range(yaml_path):
    start = vram = code = None
    with open(yaml_path, "r", encoding="utf-8") as handle:
        for line in handle:
            stripped = line.strip()
            if stripped.startswith("- name:") and vram is None:
                start = None
            elif stripped.startswith("start:") and start is None:
                start = int(stripped.split(":", 1)[1].split()[0], 16)
            elif stripped.startswith("vram:") and vram is None:
                vram = int(stripped.split(":", 1)[1].split()[0], 16)
            elif stripped.startswith("- [") and start is not None and vram is not None:
                fields = [f.strip() for f in stripped[3:].split("]")[0].split(",")]
                if len(fields) < 2 or not fields[0].startswith("0x"):
                    continue
                addr = vram + int(fields[0], 16) - start
                if fields[1] == "c" and code is None:
                    code = addr
                elif fields[1] in ("data", "databin") and code is not None:
                    return code, addr
    raise ValueError("no code then data subsegments in %s" % yaml_path)


#----------------------
# | run_nm
# | Description: Runs sh2eb-elf-nm over object files and returns its output lines.
# | Author: suinevere
# | Dependencies: sh2eb-elf-nm on PATH
# | Globals: N/A
# | Params: nm -- nm executable, objects -- object paths
# | Returns: list of lines
#----------------------
def run_nm(nm, objects):
    if not objects:
        return []
    out = subprocess.run([nm, "-g"] + list(objects), check=True, capture_output=True, text=True).stdout
    return out.splitlines()


#----------------------
# | defined_by
# | Description: Names the given objects and linker scripts define.
# | Author: suinevere
# | Dependencies: run_nm, parse_nm, parse_ld_defined, LINKER_SCRIPT_SUFFIXES
# | Globals: N/A
# | Params: nm -- nm executable, paths -- object and linker script paths
# | Returns: set of names
#----------------------
def defined_by(nm, paths):
    _, names = parse_nm(run_nm(nm, [p for p in paths if not p.endswith(LINKER_SCRIPT_SUFFIXES)]))
    for path in paths:
        if path.endswith(LINKER_SCRIPT_SUFFIXES):
            with open(path, "r", encoding="utf-8") as handle:
                names |= parse_ld_defined(handle.read())
    return names


#----------------------
# | GROUP_SCRIPT
# | Description: Matches an overlay group's data script file name, <overlay>_symbols.ld, capturing the overlay name.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
GROUP_SCRIPT = re.compile(r"^(\w+)_symbols\.ld$")


#----------------------
# | group_overlay
# | Description: Names the overlay an --overlay-group belongs to, from its data script, when the Dreamcast gives that overlay a region.
# | Author: suinevere
# | Dependencies: GROUP_SCRIPT, targets.DREAMCAST
# | Globals: N/A
# | Params: group -- object and script paths
# | Returns: overlay name, or None
#----------------------
def group_overlay(group):
    for path in group:
        match = GROUP_SCRIPT.match(os.path.basename(path))
        if match and match.group(1) in targets.DREAMCAST.regions:
            return match.group(1)
    return None


#----------------------
# | own_data_aliases
# | Description: Maps names that fall inside an overlay's own packed data, by symbol file or splat auto-name address, to their Dreamcast data-region address.
# | Author: suinevere
# | Dependencies: SYMBOL_LINE, SPLAT_AUTO_NAME, overlay_rules.packed_offset, targets.DREAMCAST
# | Globals: N/A
# | Params: names -- unresolved names, symbol_lines -- symbol_addrs lines, overlay -- overlay name
# | Returns: dict of name to address
#----------------------
def own_data_aliases(names, symbol_lines, overlay):
    spec = next(s for s in overlay_rules.OVERLAYS.values() if s.name == overlay)
    base = targets.DREAMCAST.regions[overlay].data_base
    addresses = {}
    for line in symbol_lines:
        match = SYMBOL_LINE.match(line)
        if match:
            addresses.setdefault(match.group(1), []).append(int(match.group(2), 16))
    out = {}
    for name in sorted(names):
        own = addresses.get(name, [int(name[-8:], 16)] if SPLAT_AUTO_NAME.match(name) else [])
        for addr in own:
            offset = overlay_rules.packed_offset(addr, spec)
            if offset is not None:
                out[name] = base + offset
                break
    return out


#----------------------
# | render_group
# | Description: Builds an overlay's <overlay>_pending.ld: its own unnamed data at a fixed address. dependency_aliases adds a dependent's equal-address names (e.g. GNAME's play_menu_sfx) aliased to this overlay's own symbol, so linking the dependent against this overlay's elf resolves them.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: overlay -- overlay name, aliases -- dict of name to data-region address (int), dependency_aliases -- dict of name to linker expression (str, e.g. "_field_play_sound"), for a dependent group's names this overlay's own objects resolve by equal PS1 address
# | Returns: linker script text
#----------------------
def render_group(overlay, aliases, dependency_aliases=None):
    lines = ["/*----------------------",
             " | %s_pending.ld" % overlay,
             " | Description: Generated by port/tools/gen_pending.py (make -C dreamcast pending); %s's own data its symbol files leave unnamed, plus any dependent overlay's equal-address names." % overlay.upper(),
             " | Author: suinevere",
             " | Dependencies: gen/%s_symbols.ld" % overlay,
             " | Globals: N/A",
             " | Params: N/A",
             " | Returns: N/A",
             " ----------------------*/"]
    lines += ["_%s = 0x%08X;" % (name, addr) for name, addr in sorted(aliases.items())]
    lines += ["_%s = %s;" % (name, target) for name, target in sorted((dependency_aliases or {}).items())]
    return "\n".join(lines) + "\n"


#----------------------
# | main
# | Description: Collects names the objects, or each --overlay-group (one overlay's objects and data script, linked against the objects alone, plus any --group-links dependency's own objects), leave unresolved.
# |              A group's own unnamed data goes to <overlay>_pending.ld in --group-out-dir; the rest is aliased, stubbed and written as before.
# | Author: suinevere
# | Dependencies: run_nm, parse_nm, defined_by, group_overlay, own_data_aliases, render_group, resolve_aliases, classify, render, code_range
# | Globals: CODE_YAMLS
# | Params: argv -- command-line arguments
# | Returns: 0 on success, 1 on an unknown symbol
#----------------------
def main(argv=None):
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, "..", ".."))
    parser = argparse.ArgumentParser(description="Generate the Dreamcast pending stubs.")
    parser.add_argument("--overlay", choices=sorted(OVERLAY_LABELS), default="dreamcast")
    parser.add_argument("--objects", nargs="+", required=True)
    parser.add_argument("--symbols", nargs="+", required=True)
    parser.add_argument("--defined", nargs="*", default=[])
    parser.add_argument("--alias-defined", nargs="*", default=[])
    parser.add_argument("--overlay-group", nargs="+", action="append", default=[])
    parser.add_argument("--group-links", nargs="+", action="append", default=[],
                        help="NAME:DEP pairs; NAME's group also resolves against DEP's group objects (e.g. gname:field)")
    parser.add_argument("--group-out-dir", default=None)
    parser.add_argument("--out-c", required=True)
    parser.add_argument("--out-ld", required=True)
    parser.add_argument("--nm", default="sh-elf-nm")
    parser.add_argument("--yamls", nargs="+", default=[os.path.join(repo, p) for p in CODE_YAMLS])
    args = parser.parse_args(argv)

    symbol_lines = []
    for path in args.symbols:
        with open(path, "r", encoding="utf-8") as handle:
            symbol_lines.extend(handle.read().splitlines())

    undefined, object_defined = parse_nm(run_nm(args.nm, args.objects))
    resolved = object_defined | defined_by(args.nm, args.defined)
    missing = undefined - resolved

    group_by_overlay = {group_overlay(group): group for group in args.overlay_group}
    links = {}
    for pair in args.group_links:
        for item in pair:
            name, dep = item.split(":", 1)
            links.setdefault(name, []).append(dep)

    group_undefined_by_overlay = {}
    group_defined_by_overlay = {}
    for group in args.overlay_group:
        overlay = group_overlay(group)
        group_undefined_by_overlay[overlay], _ = parse_nm(run_nm(args.nm, [p for p in group if not p.endswith(LINKER_SCRIPT_SUFFIXES)]))
        group_defined_by_overlay[overlay] = defined_by(args.nm, group)

    #----------------------
    # | Cross-overlay equal-address aliasing
    # | Description: A dependent's names left unresolved by exact name (e.g. GNAME's play_menu_sfx) go through resolve_aliases against each dependency's own defined names (field_play_sound shares its PS1 address). A hit lands in the dependency's own pending.ld, not the dependent's, and drops out of the dependent's own missing contribution.
    #----------------------
    dependency_aliases_by_overlay = {}
    consumed_by_dependent = {}
    for dependent, deps in links.items():
        if dependent not in group_undefined_by_overlay:
            continue
        remaining = group_undefined_by_overlay[dependent] - resolved - group_defined_by_overlay[dependent]
        for dep in deps:
            remaining -= group_defined_by_overlay.get(dep, set())
        if not remaining:
            continue
        for dep in deps:
            dep_defined = group_defined_by_overlay.get(dep)
            if not dep_defined:
                continue
            slots = DREAMCAST_FIELD_SLOTS if dep == "field" else ()
            found = resolve_aliases(remaining, symbol_lines, dep_defined, slots, DREAMCAST_BUFFER_RANGES)
            if found:
                dependency_aliases_by_overlay.setdefault(dep, {}).update(found)
                consumed_by_dependent.setdefault(dependent, set()).update(found)

    for group in args.overlay_group:
        overlay = group_overlay(group)
        linked_defined = set()
        for dep in links.get(overlay, []):
            linked_defined |= group_defined_by_overlay.get(dep, set())
        group_missing = (group_undefined_by_overlay[overlay] - resolved - group_defined_by_overlay[overlay]
                         - linked_defined - consumed_by_dependent.get(overlay, set()))
        if args.group_out_dir is not None and overlay is not None:
            local = own_data_aliases(group_missing, symbol_lines, overlay)
            with open(os.path.join(args.group_out_dir, "%s_pending.ld" % overlay), "w", encoding="utf-8", newline="\n") as handle:
                handle.write(render_group(overlay, local, dependency_aliases_by_overlay.get(overlay)))
            group_missing -= set(local)
        missing |= group_missing
    alias_targets = object_defined | defined_by(args.nm, args.alias_defined)

    ranges = [code_range(path) for path in args.yamls]
    try:
        aliases = resolve_aliases(missing, symbol_lines, alias_targets, (), DREAMCAST_BUFFER_RANGES, ranges)
        functions, data = classify(missing - set(aliases), symbol_lines, ranges)
    except UnknownSymbolError as error:
        print("gen_pending: %s" % error, file=sys.stderr)
        return 1

    applicable_benign = {name: kind for name, kind in DREAMCAST_BENIGN.items() if name in functions}
    c_text, ld_text = render(functions, data, aliases, benign=applicable_benign, overlay=args.overlay)
    with open(args.out_c, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(c_text)
    with open(args.out_ld, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(ld_text)
    print("gen_pending: %d functions, %d data symbols, %d aliases" % (len(functions), len(data), len(aliases)))
    for name, target in sorted(aliases.items()):
        print("gen_pending: alias %s = %s" % (name, target))
    return 0


if __name__ == "__main__":
    sys.exit(main())
