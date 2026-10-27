
#----------------------
# | overlay_link.py
# | Description: Generates overlay entry vectors, per-overlay linker scripts and main's entry constants, and packs each OVLnnC.BIN file (32-byte header, then code).
# | Author: suinevere
# | Dependencies: overlay_rules.py, targets.py, gen/<overlay>_symbols.ld, main ELF
# | Globals: OVERLAYS, USAGE, DC_SYMBOL_ALIASES, DC_EXTRA_ENTRIES, DC_ENTRY_ALIASES
# | Params: N/A
# | Returns: N/A
#----------------------
import collections
import os
import struct
import subprocess
import sys

import overlay_rules
import targets

#----------------------
# | Overlay
# | Description: One overlay: name, CD resource index, entry function names in table order, and the overlay names it links against.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Overlay = collections.namedtuple("Overlay", "name resource entries links_against")

#----------------------
# | OVERLAYS
# | Description: Overlays the build links, in table order.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAYS = [
    Overlay("checkps", 15, ["run_checkps"], []),
    Overlay("title", 4, ["run_title"], []),
    Overlay("field", 2, ["field_scene_reset", "field_restore_entry_music", "field_set_scene_parameters",
                "field_set_fade_target", "field_update_scene", "field_clear_node_accumulators",
                "field_build_frame_commands", "field_draw_frame", "field_flush_vram_uploads",
                "field_initialize_subsystems", "field_stop_song", "field_load_vram_resource"], []),
    Overlay("gname", 5, ["gname_run"], ["field"]),
    Overlay("wsel", 14, ["wsel_main"], []),
    Overlay("wmap", 3, ["run_world_map"], []),
    Overlay("menu", 6, ["func_801405B0"], ["field"]),
    Overlay("shop", 7, ["shop_init", "shop_update"], ["field"]),
    Overlay("golem", 9, ["golem_run"], ["field"]),
    Overlay("zukan", 8, ["zukan_run"], ["field"]),
    Overlay("gover", 10, ["gover_show_screen"], ["field"]),
    Overlay("movie", 11, ["movie_play", "movie_init", "movie_update", "movie_service_video_ops"], ["field"]),
    Overlay("addhero", 18, ["addhero_init", "addhero_state_step"], ["field"]),
    Overlay("niki", 17, ["niki_init", "niki_update_frame"], ["field"]),
    Overlay("cload", 16, ["cload_main"], ["field"]),
    Overlay("carda", 12, ["carda_init", "carda_update_frame"], ["field"]),
    Overlay("gosub", 13, ["gosub_open_screen_sequence", "gosub_update_frame"], ["field"]),
]

#----------------------
# | file_block
# | Description: Builds the suinevere file header block text for a generated file.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: name -- file name, description -- one-sentence description, dependencies -- Dependencies field text
# | Returns: header block text, newline terminated
#----------------------
def file_block(name, description, dependencies):
    return ("/*----------------------\n | %s\n | Description: %s\n | Author: suinevere\n | Dependencies: %s\n"
            " | Globals: N/A\n | Params: N/A\n | Returns: N/A\n ----------------------*/\n"
            % (name, description, dependencies))


#----------------------
# | vector_lines
# | Description: Assembly lines of a .vectors section, one 16-byte jump-through-literal stub per entry in table order; SH-2 and SH-4 share the encoding.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: entries -- entry function names in table order
# | Returns: list of assembly lines
#----------------------
def vector_lines(entries):
    out = ["\t.section .vectors,\"ax\""]
    for fn in entries:
        out += ["\t.balign 16", "\tmov.l\t1f,r0", "\tjmp\t@r0", "\tnop", "\tnop", "1:\t.long\t_%s" % fn, "\t.long\t0"]
    return out


#----------------------
# | read_nm
# | Description: Parses nm -g output into a name-to-address map, stripping one leading underscore and skipping undefined symbols.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: text -- nm output text
# | Returns: dict of symbol name to address
#----------------------
def read_nm(text):
    out = {}
    for line in text.splitlines():
        parts = line.split()
        if len(parts) == 3:
            name = parts[2][1:] if parts[2].startswith("_") else parts[2]
            out[name] = int(parts[0], 16)
    return out


#----------------------
# | pack_header
# | Description: Packs the 32-byte header that opens an overlay's OVLnnC.BIN: magic, version, code base and size, data size, and bss range.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: ovl -- Overlay, symbols -- name to address map from read_nm
# | Returns: 32-byte big-endian header
#----------------------
def pack_header(ovl, symbols):
    data_size = symbols["lom_ovl_%s_data_size" % ovl.name]
    bss_start = symbols["lom_ovl_%s_bss_start" % ovl.name]
    bss_end = symbols["lom_ovl_%s_bss_end" % ovl.name]
    base = symbols["lom_ovl_code_start"]
    size = symbols["lom_ovl_code_end"] - base
    return struct.pack(">4sIIIIIII", b"LOMC", 1, base, size, data_size, bss_start, bss_end, 0)


#----------------------
# | pack_overlay_file
# | Description: Builds an overlay's OVLnnC.BIN: its header followed by its code; raises ValueError when the code length disagrees with the header.
# | Author: suinevere
# | Dependencies: pack_header
# | Globals: N/A
# | Params: ovl -- Overlay, symbols -- name to address map from read_nm, code -- code bytes (empty for a static overlay)
# | Returns: header bytes followed by code bytes
#----------------------
def pack_overlay_file(ovl, symbols, code):
    header = pack_header(ovl, symbols)
    if struct.unpack_from(">I", header, 12)[0] != len(code):
        raise ValueError("%s code is %d bytes, header says %d" % (ovl.name, len(code), struct.unpack_from(">I", header, 12)[0]))
    return header + code


#----------------------
# | dc_overlays
# | Description: The overlays the Dreamcast target gives an arena region, in OVERLAYS table order.
# | Author: suinevere
# | Dependencies: OVERLAYS, targets.DREAMCAST
# | Globals: OVERLAYS
# | Params: N/A
# | Returns: list of Overlay
#----------------------
def dc_overlays():
    return [ovl for ovl in OVERLAYS if ovl.name in targets.DREAMCAST.regions]


#----------------------
# | dc_vectors_asm
# | Description: Generates an overlay's Dreamcast entry vector assembly, placed at its arena code base.
# | Author: suinevere
# | Dependencies: file_block, vector_lines
# | Globals: N/A
# | Params: ovl -- Overlay
# | Returns: assembly source text
#----------------------
def dc_vectors_asm(ovl):
    out = [file_block("%s_vectors.s" % ovl.name,
                      "Generated by port/tools/overlay_link.py --target dreamcast; %s entry vectors, 16 bytes each at its arena code base." % ovl.name.upper(),
                      "port/tools/overlay_link.py").rstrip("\n")]
    return "\n".join(out + vector_lines(dc_all_entries(ovl))) + "\n"


#----------------------
# | dc_overlay_ld
# | Description: Generates an overlay's Dreamcast linker script: its data and pending scripts, code at its region's code base, .bss after the packed data blob, and an ASSERT per region limit.
# | Author: suinevere
# | Dependencies: file_block, targets.DREAMCAST
# | Globals: N/A
# | Params: ovl -- Overlay
# | Returns: linker script text
#----------------------
def dc_overlay_ld(ovl):
    region = targets.DREAMCAST.regions[ovl.name]
    name, upper = ovl.name, ovl.name.upper()
    asserts = ["ASSERT(_lom_ovl_code_end <= 0x%08X, \"%s code exceeds its Dreamcast code region\")"
               % (region.code_base + region.code_size, upper),
               "ASSERT(SIZEOF(.ctors_check) == 0, \"%s has static constructors\")" % upper,
               "ASSERT(_lom_ovl_%s_bss_end <= 0x%08X, \"%s data and bss exceed its Dreamcast data region\")"
               % (name, region.data_base + region.data_size, upper)]
    return (file_block("%s.ld" % name,
                       "Generated by port/tools/overlay_link.py --target dreamcast; links %s at its arena code region with its .bss after its data blob." % upper,
                       "port/tools/overlay_link.py, gen/%s_symbols.ld, lom.elf" % name)
            + "OUTPUT_FORMAT(elf32-shl)\nINCLUDE gen/%s_symbols.ld\nINCLUDE gen/%s_pending.ld\n\n" % (name, name)
            + "".join("_%s = _%s;\n" % (call, target) for call, target in DC_SYMBOL_ALIASES.items())
            + "\nSECTIONS {\n"
            + "\t. = 0x%08X;\n\t.code :\n\t{\n" % region.code_base
            + "\t\t_lom_ovl_code_start = .;\n\t\tKEEP(*(.vectors))\n\t\t*(.text .text.*)\n"
            + "\t\t*(.rodata .rodata.*)\n\t\t*(.data .data.*)\n\t\t. = ALIGN(4);\n\t\t_lom_ovl_code_end = .;\n\t}\n"
            + "\t.ctors_check (NOLOAD) :\n\t{\n\t\t*(.ctors*) *(.dtors*) *(.init_array*) *(.fini_array*)\n\t}\n"
            + "\t. = 0x%08X;\n\t.bss_slot (NOLOAD) :\n\t{\n" % region.data_base
            + "\t\t. = _lom_ovl_%s_data_size;\n\t\t_lom_ovl_%s_bss_start = .;\n" % (name, name)
            + "\t\t. += _lom_ovl_%s_tail_size;\n\t\t. = ALIGN(4);\n" % name
            + "\t\t*(.bss .bss.* COMMON)\n\t\t_lom_ovl_%s_bss_end = .;\n\t}\n" % name
            + "\t/DISCARD/ :\n\t{\n\t\t*(.comment) *(.eh_frame) *(.rela*) *(.note*)\n\t}\n}\n\n"
            + "\n".join(asserts) + "\n")


#----------------------
# | DC_SYMBOL_ALIASES
# | Description: Calls every Dreamcast overlay links to a port function instead of main's library one: the BIOS card file calls, which newlib would otherwise answer, go to the VMU backend.
# | Author: suinevere
# | Dependencies: dreamcast/src/card/card_vmu.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DC_SYMBOL_ALIASES = {"open": "lom_card_open", "read": "lom_card_read", "write": "lom_card_write",
                     "close": "lom_card_close", "rename": "lom_card_rename"}


#----------------------
# | dc_strip_names
# | Description: Symbols to strip from main's copy before an overlay links: those the overlay's objects define, and the DC_SYMBOL_ALIASES names its script defines.
# | Author: suinevere
# | Dependencies: DC_SYMBOL_ALIASES
# | Globals: N/A
# | Params: nm -- `nm -g --defined-only` output for the overlay's objects
# | Returns: list of symbol names
#----------------------
def dc_strip_names(nm):
    names = [line.split()[2] for line in nm.splitlines() if len(line.split()) == 3]
    return names + ["_" + call for call in DC_SYMBOL_ALIASES]


#----------------------
# | DC_EXTRA_ENTRIES
# | Description: Extra vector slots appended after an overlay's real entries.
# |              FIELD's field_watch_entry, WMAP's wmap_watch_entry and TITLE's title_debug_entry are always compiled (dreamcast/src/overlays/) so these slots resolve in every build; each is a no-op unless LOM_PAD_SCRIPT or LOM_DEBUG_MENU installs its hooks.
# | Author: suinevere
# | Dependencies: dreamcast/src/overlays/field_watch_entry.c, dreamcast/src/overlays/wmap_watch_entry.c, dreamcast/src/overlays/title_debug_entry.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DC_EXTRA_ENTRIES = {"field": ["field_watch_entry"], "wmap": ["wmap_watch_entry"], "title": ["title_debug_entry"]}


#----------------------
# | DC_ENTRY_ALIASES
# | Description: Per secondary overlay, extra names FIELD or main call an entry by, since PS1 secondaries share 0x80140000 and FIELD names some entries by address (func_80140004).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DC_ENTRY_ALIASES = {"gname": {"gname_run": ["func_80140004"]}, "shop": {"shop_init": ["func_80140004"]}, "niki": {"niki_init": ["niki_addhero_init"]}, "addhero": {"addhero_init": ["niki_addhero_init"]}, "gover": {"gover_show_screen": ["func_80140004"]}}


#----------------------
# | dc_all_entries
# | Description: An overlay's real entries plus its extras, in vector-slot order.
# | Author: suinevere
# | Dependencies: DC_EXTRA_ENTRIES
# | Globals: N/A
# | Params: ovl -- Overlay
# | Returns: list of entry function names
#----------------------
def dc_all_entries(ovl):
    return list(ovl.entries) + DC_EXTRA_ENTRIES.get(ovl.name, [])


#----------------------
# | dc_exported_entries
# | Description: Every name main's link gets for an overlay's vector slots: each entry and its DC_ENTRY_ALIASES, at code base plus 16 per slot.
# | Author: suinevere
# | Dependencies: dc_all_entries, DC_ENTRY_ALIASES, targets.DREAMCAST
# | Globals: N/A
# | Params: ovl -- Overlay
# | Returns: list of (exported name, address)
#----------------------
def dc_exported_entries(ovl):
    base = targets.DREAMCAST.regions[ovl.name].code_base
    aliases = DC_ENTRY_ALIASES.get(ovl.name, {})
    out = []
    for i, fn in enumerate(dc_all_entries(ovl)):
        for name in [fn] + aliases.get(fn, []):
            out.append((name, base + 16 * i))
    return out


#----------------------
# | dc_entries_ld
# | Description: Generates the Dreamcast main link's overlay entry vector addresses, one per entry (plus any DC_EXTRA_ENTRIES) of every linked overlay with an arena region.
# |              Unlinked overlays are left out, since a script assignment would override main's resident copies of their entries.
# | Author: suinevere
# | Dependencies: file_block, dc_overlays, dc_exported_entries, targets.DREAMCAST_UNLINKED_OVERLAYS
# | Globals: N/A
# | Params: N/A
# | Returns: linker script text
#----------------------
def dc_entries_ld():
    out = [file_block("overlay_entries.ld",
                      "Generated by port/tools/overlay_link.py --target dreamcast; overlay entry vector addresses for lom.elf's link.",
                      "port/tools/overlay_link.py, port/tools/targets.py").rstrip("\n")]
    for ovl in dc_overlays():
        if ovl.name in targets.DREAMCAST_UNLINKED_OVERLAYS:
            continue
        for name, address in dc_exported_entries(ovl):
            line = "_%s = 0x%08X;" % (name, address)
            if line not in out:
                out.append(line)
    return "\n".join(out) + "\n"


#----------------------
# | dc_regions_h
# | Description: Generates the C header giving the Dreamcast loader the arena bounds and each overlay's resource index, name and regions.
# | Author: suinevere
# | Dependencies: file_block, dc_overlays, targets.DREAMCAST, targets.ARENA_BASE, targets.ARENA_END
# | Globals: N/A
# | Params: N/A
# | Returns: header text
#----------------------
def dc_regions_h():
    rows = []
    for ovl in dc_overlays():
        r = targets.DREAMCAST.regions[ovl.name]
        rows.append("    { %d, \"%s\", 0x%08Xu, 0x%08Xu, 0x%08Xu, 0x%08Xu }"
                    % (ovl.resource, ovl.name.upper(), r.code_base, r.code_size, r.data_base, r.data_size))
    return (file_block("overlay_regions.h",
                       "Generated by port/tools/overlay_link.py --target dreamcast; the overlay arena and each overlay's code and data regions.",
                       "port/tools/overlay_link.py, port/tools/targets.py")
            + "#ifndef LOM_OVERLAY_REGIONS_H\n#define LOM_OVERLAY_REGIONS_H\n\n"
            + "#define OVERLAY_ARENA_BASE 0x%08Xu\n#define OVERLAY_ARENA_END 0x%08Xu\n\n" % (targets.ARENA_BASE, targets.ARENA_END)
            + "#define OVERLAY_REGION_TABLE \\\n" + ", \\\n".join(rows) + "\n\n#endif\n")


#----------------------
# | generated_files
# | Description: Every file "gen" writes, in write order, as (file name, text) pairs.
# | Author: suinevere
# | Dependencies: dc_entries_ld, dc_regions_h, dc_vectors_asm, dc_overlay_ld, dc_overlays
# | Globals: N/A
# | Params: N/A
# | Returns: list of (name, text)
#----------------------
def generated_files():
    files = [("overlay_entries.ld", dc_entries_ld()), ("overlay_regions.h", dc_regions_h())]
    for ovl in dc_overlays():
        files += [("%s_vectors.s" % ovl.name, dc_vectors_asm(ovl)), ("%s.ld" % ovl.name, dc_overlay_ld(ovl))]
    return files


#----------------------
# | tool_path
# | Description: Path of a Dreamcast binutils program: the linker from KOS_LD (or sh-elf-ld), and its nm or objcopy beside it.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: program -- "ld", "nm" or "objcopy"
# | Returns: executable path or name
#----------------------
def tool_path(program):
    ld = os.environ.get("KOS_LD") or "sh-elf-ld"
    return ld if program == "ld" else ld[:-2] + program


#----------------------
# | summary
# | Description: The dry-run summary lines: each linked overlay's code and data region, and the linker used.
# | Author: suinevere
# | Dependencies: dc_overlays, tool_path, targets.DREAMCAST
# | Globals: N/A
# | Params: N/A
# | Returns: list of lines
#----------------------
def summary():
    lines = []
    for ovl in dc_overlays():
        r = targets.DREAMCAST.regions[ovl.name]
        lines.append("%s: code 0x%08X size 0x%X, data 0x%08X size 0x%X" % (ovl.name, r.code_base, r.code_size, r.data_base, r.data_size))
    return lines + ["linker: %s" % tool_path("ld")]


#----------------------
# | overlay_named
# | Description: Looks up a linked overlay by name.
# | Author: suinevere
# | Dependencies: dc_overlays
# | Globals: N/A
# | Params: name -- overlay name
# | Returns: Overlay; raises SystemExit for an overlay that has no link
#----------------------
def overlay_named(name):
    for ovl in dc_overlays():
        if ovl.name == name:
            return ovl
    raise SystemExit("overlay %s has no link" % name)


#----------------------
# | dc_link
# | Description: Links one Dreamcast overlay against lom.elf with the overlay's own symbols stripped from main's copy, keeps its nm dump, then packs its OVLnnC.BIN.
# |              For each links_against dependency (e.g. GNAME against FIELD), this overlay's own names are stripped from that dependency's elf first, so its own objects still win.
# | Author: suinevere
# | Dependencies: tool_path, read_nm, pack_overlay_file, gen/<overlay>.ld
# | Globals: N/A
# | Params: ovl -- Overlay, main_elf -- lom.elf path, work -- directory for intermediate files, packed -- OVLnnC.BIN path, objects -- overlay object files, links_against -- names of already-linked overlays in work to also resolve against
# | Returns: N/A
#----------------------
def dc_link(ovl, main_elf, work, packed, objects, links_against=()):
    name = ovl.name
    defined = os.path.join(work, "%s.defined" % name)
    main_copy = os.path.join(work, "%s_main.elf" % name)
    elf = os.path.join(work, "%s.elf" % name)
    code = os.path.join(work, "%s.code.bin" % name)
    nm = subprocess.run([tool_path("nm"), "-g", "--defined-only"] + objects, check=True, capture_output=True, text=True).stdout
    write_text(defined, "".join(symbol + "\n" for symbol in dc_strip_names(nm)))
    subprocess.run([tool_path("objcopy"), "--strip-symbols=" + defined, main_elf, main_copy], check=True)
    just_symbols = ["--just-symbols=" + main_copy]
    for dep in links_against:
        dep_elf = os.path.join(work, "%s.elf" % dep)
        dep_copy = os.path.join(work, "%s_%s.elf" % (name, dep))
        subprocess.run([tool_path("objcopy"), "--strip-symbols=" + defined, dep_elf, dep_copy], check=True)
        just_symbols.append("--just-symbols=" + dep_copy)
    subprocess.run([tool_path("ld"), "-T", "gen/%s.ld" % name] + just_symbols +
                   ["-Map", os.path.join(work, "%s.map" % name), "-o", elf] + objects, check=True)
    subprocess.run([tool_path("objcopy"), "-O", "binary", "-j", ".code", elf, code], check=True)
    linked = subprocess.run([tool_path("nm"), elf], check=True, capture_output=True, text=True).stdout
    write_text(os.path.join(work, "%s.nm" % name), linked)
    symbols = read_nm(linked)
    with open(code, "rb") as f:
        body = f.read()
    with open(packed, "wb") as f:
        f.write(pack_overlay_file(ovl, symbols, body))


#----------------------
# | USAGE
# | Description: Command-line usage text.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
USAGE = ("usage: overlay_link.py [--target dreamcast] [--dry-run] gen [--out DIR]"
         " | link NAME MAIN_ELF WORK_DIR OUT OBJ... [--links DEP]...")


#----------------------
# | parse_args
# | Description: Splits the CLI arguments into the target, the dry-run flag, the --out directory, repeated --links dependency names and the remaining positional words.
# | Author: suinevere
# | Dependencies: targets.by_name, USAGE
# | Globals: N/A
# | Params: argv -- CLI arguments, excluding the program name
# | Returns: (target, dry_run, out or None, links list, positional list)
#----------------------
def parse_args(argv):
    target, dry_run, out, links, words = targets.DREAMCAST, False, None, [], []
    i = 0
    while i < len(argv):
        if argv[i] in ("--target", "--out", "--links") and i + 1 >= len(argv):
            raise SystemExit(USAGE)
        if argv[i] == "--target":
            target = targets.by_name(argv[i + 1])
            i += 2
        elif argv[i] == "--out":
            out = argv[i + 1]
            i += 2
        elif argv[i] == "--links":
            links.append(argv[i + 1])
            i += 2
        elif argv[i] == "--dry-run":
            dry_run = True
            i += 1
        else:
            words.append(argv[i])
            i += 1
    return target, dry_run, out, links, words


#----------------------
# | write_text
# | Description: Writes text to a file with LF line endings, regardless of host platform.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: path -- output file path, text -- file contents
# | Returns: N/A
#----------------------
def write_text(path, text):
    with open(path, "w", newline="\n") as f:
        f.write(text)


#----------------------
# | main
# | Description: CLI entry point: "gen" (the default) writes the generated link files, or prints them with --dry-run; "link" links and packs one overlay.
# | Author: suinevere
# | Dependencies: parse_args, generated_files, summary, write_text, overlay_named, dc_link, USAGE
# | Globals: N/A
# | Params: argv -- CLI arguments, excluding the program name
# | Returns: process exit code
#----------------------
def main(argv):
    _target, dry_run, out, links, words = parse_args(argv)
    command = words[0] if words else "gen"
    if command == "gen":
        files = generated_files()
        if dry_run:
            sys.stdout.write("".join(line + "\n" for line in summary())
                             + "".join("==> %s <==\n%s" % (name, text) for name, text in files))
            return 0
        out = out or "gen"
        os.makedirs(out, exist_ok=True)
        for name, text in files:
            write_text(os.path.join(out, name), text)
        return 0
    if command == "link" and len(words) >= 6:
        os.makedirs(words[3], exist_ok=True)
        dc_link(overlay_named(words[1]), words[2], words[3], words[4], words[5:], links_against=links)
        return 0
    raise SystemExit(USAGE)




if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
