#----------------------
# | gen_pending.py
# | Description: Generates halting C stubs and linker aliases for what FIELD and GNAME, or WMAP, objects reference but nothing defines; run it with `make pending` in saturn/.
# | Author: suinevere
# | Dependencies: sh2eb-elf-nm, overlay_rules.py, config/us/SLUS_010.13.yaml, config/us/overlays/{FIELD,GNAME}.BIN.yaml, config/us/symbols/*.txt
# | Globals: KNOWN_SDK_FUNCTIONS, CODE_YAMLS, SECONDARY_OVERLAY_RANGE, SLOT_OVERLAYS, OVERLAY_SLOTS, BUFFER_RANGES, BENIGN, OVERLAY_BENIGN, OVERLAY_LABELS
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import overlay_rules  # noqa: E402

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
# | SLOT_OVERLAYS
# | Description: overlay_rules.OVERLAYS keys of the overlays linked with FIELD and GNAME, whose packed ranges (data plus tail) map onto the LWRAM slot.
# | Author: suinevere
# | Dependencies: overlay_rules.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SLOT_OVERLAYS = (2, 5)

#----------------------
# | OVERLAY_SLOTS
# | Description: Per --overlay run, the overlay_rules.OVERLAYS keys whose packed ranges that run's references may resolve into; WMAP is never resident with FIELD or GNAME.
# | Author: suinevere
# | Dependencies: SLOT_OVERLAYS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAY_SLOTS = {"field": SLOT_OVERLAYS, "wmap": (3,), "dreamcast": ()}

#----------------------
# | OVERLAY_LABELS
# | Description: Per --overlay run, the output file stem and the overlay names its generated headers and halt messages use.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAY_LABELS = {"field": ("field", "FIELD and GNAME", "FIELD"), "wmap": ("wmap", "WMAP", "WMAP"),
                  "dreamcast": ("dc", "the Dreamcast main executable, CHECKPS and TITLE", "Dreamcast")}


#----------------------
# | slot_address
# | Description: Maps a PS1 address inside a slot overlay's packed ranges to its Saturn slot address, exactly as the converter packs it.
# | Author: suinevere
# | Dependencies: overlay_rules.packed_offset, overlay_rules.OVERLAYS, overlay_rules.SLOT_BASE
# | Globals: SLOT_OVERLAYS
# | Params: addr -- PS1 address, overlays -- overlay_rules.OVERLAYS keys to search
# | Returns: Saturn slot address, or None outside every packed range
#----------------------
def slot_address(addr, overlays=SLOT_OVERLAYS):
    for overlay in overlays:
        spec = overlay_rules.OVERLAYS[overlay]
        offset = overlay_rules.packed_offset(addr, spec)
        if offset is not None:
            return overlay_rules.SLOT_BASE + spec.base + offset
    return None


#----------------------
# | BUFFER_RANGES
# | Description: PS1 regions (start, Saturn buffer size, buffer symbol) that lom_prefix.h redirects to Saturn buffers of the same layout.
# | Author: suinevere
# | Dependencies: ../port/psyq/lom_prefix.h, src/boot/main_globals.c
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
# | Description: BUFFER_RANGES for the Dreamcast build, whose lom_sound_bank covers the full PS1 bank area TITLE copies into.
# | Author: suinevere
# | Dependencies: BUFFER_RANGES, dreamcast/src/boot/main_globals_dc.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DREAMCAST_BUFFER_RANGES = tuple((start, 0x4000 if symbol == "_lom_sound_bank" else size, symbol)
                                for start, size, symbol in BUFFER_RANGES)

#----------------------
# | OVERLAY_BUFFER_RANGES
# | Description: Per --overlay run, the redirected-buffer table its aliases resolve against.
# | Author: suinevere
# | Dependencies: BUFFER_RANGES, DREAMCAST_BUFFER_RANGES
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAY_BUFFER_RANGES = {"field": BUFFER_RANGES, "wmap": BUFFER_RANGES, "dreamcast": DREAMCAST_BUFFER_RANGES}

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
# | WMAP_SAVE_BIG_ENDIAN_OFFSETS
# | Description: g_saved_game byte offsets for WMAP raw externs that alias a field wmap_save_views.h's LOM_BIG_ENDIAN branch moves, overriding the PS1 (little-endian) address difference resolve_aliases would otherwise compute; each value is read straight off that struct's #if defined(LOM_BIG_ENDIAN) member order, not derived here.
# | Author: suinevere
# | Dependencies: src/overlays/wmap/internal/wmap_save_views.h (WmapSave.calendar, WmapLandStatus)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WMAP_SAVE_BIG_ENDIAN_OFFSETS = {
    # WmapSave.calendar.data on LOM_BIG_ENDIAN: day (0x2E4-0x2E5), placed_land_count (0x2E6), unknown_0x2e4 (0x2E7, no alias reads it).
    "g_wmap_saved_day": ("g_saved_game", 0x2E4),
    "g_wmap_placed_land_count": ("g_saved_game", 0x2E6),
    # WmapLandStatus.data on LOM_BIG_ENDIAN, for lands[0] (D_800432C8/D_800432C9 index by WMAP_NODE_RECORD_SIZE from this base): artifact_order (byte 0), placement_order (byte 1), position (byte 2), flags (byte 3).
    "D_800432C8": ("g_saved_game", 0x2F3),
    "D_800432C9": ("g_saved_game", 0x2F2),
}

#----------------------
# | resolve_aliases
# | Description: Maps names (splat auto names by their own address) to an equal-address defined symbol outside the secondary range, an offset into a defined size:/allow_addend symbol that contains them, a databin slot address, or a redirected buffer; names in offset_overrides get that fixed container and offset instead.
# |              A function (classify's rule over code_ranges) is never placed inside a container, since another overlay's data can span its address.
# | Author: suinevere
# | Dependencies: SYMBOL_LINE, SIZED_SYMBOL, SPLAT_AUTO_NAME, SECONDARY_OVERLAY_RANGE, slot_address, BUFFER_RANGES, classify
# | Globals: N/A
# | Params: names -- undefined names, symbol_lines -- symbol_addrs lines, targets -- names that linked objects or slot scripts define, slot_overlays -- overlays whose packed ranges may resolve a name, offset_overrides -- dict of name to (container, offset) that wins over the computed alias, buffer_ranges -- redirected-buffer table, code_ranges -- (start, end) code ranges that make a name a function
# | Returns: dict of name to linker expression ("_target", "_container + offset", a hex slot address or "_buffer + offset")
#----------------------
def resolve_aliases(names, symbol_lines, targets, slot_overlays=SLOT_OVERLAYS, offset_overrides=None, buffer_ranges=BUFFER_RANGES,
                    code_ranges=()):
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
    for name, container in (offset_overrides or {}).items():
        if name in aliases:
            aliases[name] = "_%s + 0x%X" % container
    return aliases


#----------------------
# | BENIGN
# | Description: Pending FIELD/GNAME functions that Home's path reaches with no input; each logs once and returns a safe value instead of halting.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BENIGN = {
    "field_load_actor_sequence_data": "void",
    "field_refresh_party_routes": "void",
    "field_reset_leader_position_history": "void",
    "field_update_object_tints": "void",
    "field_update_actor_input": "zero",
    "field_record_actor_position": "void",
    "field_render_effects": "void",
    "field_render_effect_frame8": "arg:cursor",
    "field_render_effect_frame16": "arg:cursor",
    "akao_set_master_volume": "zero",
    "cdrom_set_audio_volume": "void",
    "McxCardType": "zero",
    "record_game_diagnostic": "void",
    "field_update_actor_command": "void",
    "field_follow_leader_route": "void",
    "field_advance_actor_effects": "void",
    "field_collect_attack_sphere_hits": "void",
    "field_probe_actor_interaction": "void",
    "field_start_object_ground_effect": "void",
    "field_update_sequence_actor_binding": "void",
}

#----------------------
# | OVERLAY_BENIGN
# | Description: Per --overlay run, the BENIGN table it applies; WMAP's pending functions all halt.
# | Author: suinevere
# | Dependencies: BENIGN
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAY_BENIGN = {"field": BENIGN, "wmap": {}, "dreamcast": {}}


#----------------------
# | SATURN_PLATFORM / OVERLAY_PLATFORMS
# | Description: Per --overlay run, the generator command named in the outputs, the system header, the halt call and its message; runs not listed use the Saturn's.
# | Author: suinevere
# | Dependencies: saturn/src/system/saturn_system.h, dreamcast/src/system/dc_system.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SATURN_PLATFORM = ("saturn/tools/gen_pending.py (make pending in saturn/)", "saturn_system.h", "saturn_halt",
                   "%(overlay)s function %(name)s not ported yet")
OVERLAY_PLATFORMS = {"dreamcast": ("port/tools/gen_pending.py (make -C dreamcast pending)", "dc_system.h", "dc_halt",
                                   "pending: %(name)s")}


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
# | Description: Builds one BENIGN stub: logs once through saturn_log_once, then returns void, zero or its second argument unchanged.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: name -- function name, kind -- "void", "zero" or "arg:<param>"
# | Returns: C source text for the one function
#----------------------
def render_benign_function(name, kind):
    if kind == "void":
        signature = "void lom_pending_%s(void)" % name
        body = "    saturn_log_once(\"field: %s skipped\");\n" % name
    elif kind == "zero":
        signature = "int lom_pending_%s(void)" % name
        body = "    saturn_log_once(\"field: %s skipped\");\n    return 0;\n" % name
    elif kind.startswith("arg:"):
        param = kind[len("arg:"):]
        signature = "void *lom_pending_%s(void *lom_pending_unused0, void *%s)" % (name, param)
        body = "    saturn_log_once(\"field: %s skipped\");\n    return %s;\n" % (name, param)
    else:
        raise ValueError("unknown BENIGN kind for %s: %s" % (name, kind))
    return (
        "\n/*----------------------\n"
        " | %s\n"
        " | Description: Benign; Home's path reaches it with no input, so it logs once and returns safely instead of halting.\n"
        " | Author: suinevere\n"
        " | Dependencies: saturn_log_once\n"
        " | Globals: N/A\n"
        " | Params: N/A\n"
        " | Returns: N/A\n"
        " ----------------------*/\n"
        "%s __asm__(\"_%s\");\n"
        "%s\n"
        "{\n"
        "%s"
        "}\n" % (name, signature, name, signature, body)
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
def render(functions, data, aliases=None, benign=None, overlay="field"):
    stem, callers, halt = OVERLAY_LABELS[overlay]
    if benign:
        unmatched = sorted(set(benign) - set(functions))
        if unmatched:
            raise UnmatchedBenignNameError("BENIGN names with no matching pending function: %s" % ", ".join(unmatched))
    tool, system_header, halt_call, halt_text = OVERLAY_PLATFORMS.get(overlay, SATURN_PLATFORM)
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
        " ----------------------*/\n" % (stem, tool, callers, "src/boot/main_globals_dc.c" if overlay == "dreamcast" else "src/boot/main_globals.c")
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
# | main
# | Description: Collects the given objects' unresolved names, aliases those --objects or --alias-defined resolve, classifies the rest and writes the outputs.
# | Author: suinevere
# | Dependencies: run_nm, parse_nm, parse_ld_defined, resolve_aliases, classify, render, code_range
# | Globals: CODE_YAMLS
# | Params: argv -- command-line arguments
# | Returns: 0 on success, 1 on an unknown symbol
#----------------------
def main(argv=None):
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, "..", ".."))
    parser = argparse.ArgumentParser(description="Generate FIELD/GNAME or WMAP pending stubs.")
    parser.add_argument("--overlay", choices=sorted(OVERLAY_LABELS), default="field")
    parser.add_argument("--objects", nargs="+", required=True)
    parser.add_argument("--symbols", nargs="+", required=True)
    parser.add_argument("--defined", nargs="*", default=[])
    parser.add_argument("--alias-defined", nargs="*", default=[])
    parser.add_argument("--out-c", required=True)
    parser.add_argument("--out-ld", required=True)
    parser.add_argument("--nm", default="sh2eb-elf-nm")
    parser.add_argument("--yamls", nargs="+", default=[os.path.join(repo, p) for p in CODE_YAMLS])
    args = parser.parse_args(argv)

    undefined, defined = parse_nm(run_nm(args.nm, args.objects))
    other_objects = [p for p in args.defined if not p.endswith(LINKER_SCRIPT_SUFFIXES)]
    _, other_defined = parse_nm(run_nm(args.nm, other_objects))
    defined |= other_defined
    for path in args.defined:
        if path.endswith(LINKER_SCRIPT_SUFFIXES):
            with open(path, "r", encoding="utf-8") as handle:
                defined |= parse_ld_defined(handle.read())

    _, targets = parse_nm(run_nm(args.nm, args.objects))
    alias_objects = [p for p in args.alias_defined if not p.endswith(LINKER_SCRIPT_SUFFIXES)]
    _, alias_object_defined = parse_nm(run_nm(args.nm, alias_objects))
    targets |= alias_object_defined
    for path in args.alias_defined:
        if path.endswith(LINKER_SCRIPT_SUFFIXES):
            with open(path, "r", encoding="utf-8") as handle:
                targets |= parse_ld_defined(handle.read())

    symbol_lines = []
    for path in args.symbols:
        with open(path, "r", encoding="utf-8") as handle:
            symbol_lines.extend(handle.read().splitlines())

    ranges = [code_range(path) for path in args.yamls]
    overrides = WMAP_SAVE_BIG_ENDIAN_OFFSETS if args.overlay == "wmap" else None
    try:
        aliases = resolve_aliases(undefined - defined, symbol_lines, targets, OVERLAY_SLOTS[args.overlay], overrides,
                                  OVERLAY_BUFFER_RANGES[args.overlay], ranges)
        functions, data = classify(undefined - defined - set(aliases), symbol_lines, ranges)
    except UnknownSymbolError as error:
        print("gen_pending: %s" % error, file=sys.stderr)
        return 1

    c_text, ld_text = render(functions, data, aliases, benign=OVERLAY_BENIGN[args.overlay], overlay=args.overlay)
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
