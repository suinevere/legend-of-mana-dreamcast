#----------------------
# | overlay_link.py
# | Description: Generates overlay entry vectors, per-overlay linker scripts and main's entry constants, and packs each OVLnnC.BIN file (32-byte header, then code).
# | Author: suinevere
# | Dependencies: overlay_rules.py, wmap_memory.py, targets.py, gen/<overlay>_symbols.ld, main ELF
# | Globals: REGIONS, WORK_AREA, ARENA_SIZE, SLOT_BASE, SECONDARY_BASE, FIELD_RENDER_BUFFERS, FIELD_CD_BUFFER, GNAME_RENDER_AREA, WSEL_RENDER_AREA, FIELD_PRIMARY_UPPER, OVERLAYS, MAIN_FIELD_DATA, BSS_BLOCKS, EXTRA_ASSERTS, EXTRA_SYMBOLS, USAGE
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
import wmap_memory

#----------------------
# | REGIONS
# | Description: Final HWRAM code regions, keyed by name, as (base, size); primary ends at the render arena.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
REGIONS = {"primary": (0x0607A000, 0x50000), "secondary": (0x060CA000, 0xC000)}

#----------------------
# | WORK_AREA
# | Description: Base of SGL's work area, which ends the overlay code regions; the secondary region stops here.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WORK_AREA = 0x060D6000

#----------------------
# | ARENA_SIZE
# | Description: Size of the render arena carved out of the top of the primary region.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ARENA_SIZE = 0x20000

#----------------------
# | SLOT_BASE
# | Description: Low Work RAM address of the shared overlay slot; must match saturn.linker and overlay_loader.h.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SLOT_BASE = 0x00210000

#----------------------
# | SECONDARY_BASE
# | Description: Offset of the secondary overlay slot within SLOT_BASE, for an overlay that shares the slot with another.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECONDARY_BASE = 0x70000

#----------------------
# | FIELD_RENDER_BUFFERS
# | Description: LWRAM address where FIELD's render buffers begin; FIELD's bss must stay below it.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FIELD_RENDER_BUFFERS = 0x00270678

#----------------------
# | FIELD_CD_BUFFER
# | Description: HWRAM address of FIELD's CD buffer at the tail of its code region; FIELD's code must end below it; must match saturn.linker.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FIELD_CD_BUFFER = 0x060BD000

#----------------------
# | GNAME_RENDER_AREA
# | Description: LWRAM address where GNAME's render area begins; GNAME's bss must stay below it.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
GNAME_RENDER_AREA = 0x002A0000

#----------------------
# | WSEL_RENDER_AREA
# | Description: LWRAM address of WSEL's render buffers (main's SECONDARY_OVERLAY_AT(0x30000)); WSEL's data and bss must stay below it.
# | Author: suinevere
# | Dependencies: SLOT_BASE, SECONDARY_BASE
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WSEL_RENDER_AREA = SLOT_BASE + SECONDARY_BASE + 0x30000

#----------------------
# | FIELD_PRIMARY_UPPER
# | Description: The PS1 page at 0x80100000 relocated into FIELD's LWRAM slot, where FIELD's databin keeps g_field_object_states.
# | Author: suinevere
# | Dependencies: overlay_rules.relocate_pointer, overlay_rules.OVERLAYS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FIELD_PRIMARY_UPPER = overlay_rules.relocate_pointer(0x80100000, overlay_rules.OVERLAYS[2])

#----------------------
# | Overlay
# | Description: One overlay: name, CD resource index, region name, entry function names in table order, whether it is limited by the render arena, whether it links as a real overlay, and the overlay names it links against.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Overlay = collections.namedtuple("Overlay", "name resource region entries below_arena real links_against")

#----------------------
# | OVERLAYS
# | Description: Overlays the builds load, in table order; Saturn skips targets.SATURN_UNLINKED_OVERLAYS (MENU).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAYS = [
    Overlay("checkps", 15, "primary", ["run_checkps"], True, True, []),
    Overlay("title", 4, "primary", ["run_title"], True, True, []),
    Overlay("field", 2, "primary", ["field_scene_reset", "field_restore_entry_music", "field_set_scene_parameters",
                                    "field_set_fade_target", "field_update_scene", "field_clear_node_accumulators",
                                    "field_build_frame_commands", "field_draw_frame", "field_flush_vram_uploads",
                                    "field_initialize_subsystems", "field_stop_song", "field_load_vram_resource"],
            False, True, []),
    Overlay("gname", 5, "secondary", ["gname_run"], False, True, ["field"]),
    Overlay("wsel", 14, "primary", ["wsel_main"], True, True, []),
    Overlay("wmap", 3, "primary", ["run_world_map"], False, True, []),
    Overlay("menu", 6, "secondary", ["func_801405B0"], False, True, ["field"]),
    Overlay("shop", 7, "secondary", ["shop_init", "shop_update"], False, True, ["field"]),
    Overlay("golem", 9, "secondary", ["golem_run"], False, True, ["field"]),
    Overlay("zukan", 8, "secondary", ["zukan_run"], False, True, ["field"]),
    Overlay("gover", 10, "secondary", ["gover_show_screen"], False, True, ["field"]),
    Overlay("movie", 11, "secondary", ["movie_play", "movie_init", "movie_update", "movie_service_video_ops"], False, True, ["field"]),
    Overlay("addhero", 18, "secondary", ["addhero_init", "addhero_state_step"], False, True, ["field"]),
    Overlay("niki", 17, "secondary", ["niki_init", "niki_update_frame"], False, True, ["field"]),
    Overlay("cload", 16, "secondary", ["cload_main"], False, True, ["field"]),
    Overlay("carda", 12, "secondary", ["carda_init", "carda_update_frame"], False, True, ["field"]),
    Overlay("gosub", 13, "secondary", ["gosub_open_screen_sequence", "gosub_update_frame"], False, True, ["field"]),
]

#----------------------
# | MAIN_FIELD_DATA
# | Description: FIELD data words that main's field_runtime.c reads or writes, by PS1 address; main's link gets their relocated FIELD slot addresses.
# | Author: suinevere
# | Dependencies: ../config/us/symbols/shared_symbol_addrs.txt, ../config/us/symbols/field_symbol_addrs.txt
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAIN_FIELD_DATA = {"g_pending_game_state": 0x8010D018, "g_field_scene_request_pending": 0x801158A4}

#----------------------
# | BSS_BLOCKS
# | Description: Per-overlay .bss_slot body text, keyed by overlay name; FIELD and WMAP reserve their tails before the common bss.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BSS_BLOCKS = {
    "checkps": "\t\t. = _lom_ovl_checkps_data_size;\n\t\t. = ALIGN(4);\n\t\t_lom_ovl_checkps_bss_start = .;\n"
               "\t\t*(.bss .bss.* COMMON)\n\t\t_lom_ovl_checkps_bss_end = .;\n",
    "title": "\t\t. = _lom_ovl_title_data_size;\n\t\t. = ALIGN(4);\n\t\t_lom_ovl_title_bss_start = .;\n"
             "\t\t*(.bss .bss.* COMMON)\n\t\t_lom_ovl_title_bss_end = .;\n",
    "field": "\t\t. = _lom_ovl_field_data_size;\n\t\t. = ALIGN(4);\n\t\t_lom_ovl_field_bss_start = .;\n"
             "\t\t. += _lom_ovl_field_tail_size;\n\t\t*(.bss .bss.* COMMON)\n\t\t_lom_ovl_field_bss_end = .;\n",
    "gname": "\t\t. = _lom_ovl_gname_data_size;\n\t\t. = ALIGN(4);\n\t\t_lom_ovl_gname_bss_start = .;\n"
             "\t\t*(.bss .bss.* COMMON)\n\t\t_lom_ovl_gname_bss_end = .;\n",
    "wsel": "\t\t. = _lom_ovl_wsel_data_size;\n\t\t. = ALIGN(4);\n\t\t_lom_ovl_wsel_bss_start = .;\n"
            "\t\t*(.bss .bss.* COMMON)\n\t\t_lom_ovl_wsel_bss_end = .;\n",
    "wmap": "\t\t. = _lom_ovl_wmap_data_size;\n\t\t. = ALIGN(4);\n\t\t_lom_ovl_wmap_bss_start = .;\n"
            "\t\t. += _lom_ovl_wmap_tail_size;\n\t\t*(.bss .bss.* COMMON)\n\t\t_lom_ovl_wmap_bss_end = .;\n",
}

#----------------------
# | EXTRA_ASSERTS
# | Description: Per-overlay extra linker ASSERT lines beyond the shared code-limit and no-ctors checks, keyed by overlay name.
# | Author: suinevere
# | Dependencies: FIELD_CD_BUFFER, FIELD_RENDER_BUFFERS, GNAME_RENDER_AREA, WSEL_RENDER_AREA, wmap_memory.place, wmap_memory.lowest, wmap_memory.BUFFERS, wmap_memory.HWRAM_SECONDARY_BASE, wmap_memory.HWRAM_SECONDARY_TOP
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXTRA_ASSERTS = {
    "checkps": ["ASSERT(_lom_ovl_checkps_bss_end <= 0x002E0000, \"CHECKPS overlay exceeds the LWRAM slot\")",
                "ASSERT(_lom_ovl_checkps_bss_end > _lom_ovl_checkps_bss_start, \"CHECKPS bss did not land in the slot\")"],
    "title": ["ASSERT(_lom_ovl_title_bss_end <= 0x002E0000, \"TITLE overlay exceeds the LWRAM slot\")"],
    "field": ["ASSERT(_lom_ovl_code_end <= 0x%08X, \"FIELD code reaches its CD buffer\")" % FIELD_CD_BUFFER,
              "ASSERT(_lom_ovl_field_bss_end <= 0x%08X, \"FIELD bss reaches its render buffers\")" % FIELD_RENDER_BUFFERS,
              "ASSERT(_lom_ovl_field_data_size % 4 == 0, \"FIELD data size is not word aligned\")",
              "ASSERT(_lom_primary_upper >= _lom_ovl_field_bss_start && _lom_primary_upper < _lom_ovl_field_bss_end, "
              "\"FIELD primary upper page is outside its bss\")"],
    "gname": ["ASSERT(_lom_ovl_gname_bss_end <= 0x%08X, \"GNAME data reaches its render area\")" % GNAME_RENDER_AREA],
    "wsel": ["ASSERT(_lom_ovl_wsel_bss_end <= 0x%08X, \"WSEL data reaches its render area\")" % WSEL_RENDER_AREA],
    "wmap": ["ASSERT(_lom_ovl_wmap_bss_end <= 0x%08X, \"WMAP bss reaches its LWRAM buffers\")"
             % wmap_memory.lowest(wmap_memory.place(wmap_memory.BUFFERS), "lwram")]
            + (["ASSERT(_lom_ovl_code_end <= 0x%08X, \"WMAP code reaches its HWRAM buffers\")"
                % wmap_memory.lowest(wmap_memory.place(wmap_memory.BUFFERS), "hwram")]
               if any(b.region == "hwram" for b in wmap_memory.BUFFERS) else [])
            + ["ASSERT(_lom_ovl_code_end <= 0x%08X, \"WMAP code reaches the HWRAM secondary region\")"
               % wmap_memory.HWRAM_SECONDARY_BASE]
            + ["ASSERT(_%s >= 0x%08X && _%s + 0x%X <= 0x%08X, \"WMAP buffer %s leaves the HWRAM secondary region\")"
               % (b.name, wmap_memory.HWRAM_SECONDARY_BASE, b.name, b.size, wmap_memory.HWRAM_SECONDARY_TOP, b.name)
               for b in wmap_memory.BUFFERS if b.region == "hwram_secondary"],
}


#----------------------
# | EXTRA_SYMBOLS
# | Description: Per-overlay linker symbol assignments, keyed by overlay name; FIELD points PRIMARY_OVERLAY_UPPER_ADDRESS into its own slot.
# | Author: suinevere
# | Dependencies: FIELD_PRIMARY_UPPER
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXTRA_SYMBOLS = {"field": ["_lom_primary_upper = 0x%08X;" % FIELD_PRIMARY_UPPER]}


#----------------------
# | region
# | Description: Resolves an overlay's HWRAM code region.
# | Author: suinevere
# | Dependencies: REGIONS
# | Globals: REGIONS
# | Params: ovl -- Overlay
# | Returns: (base, size) tuple
#----------------------
def region(ovl):
    return REGIONS[ovl.region]


#----------------------
# | arena_base
# | Description: First address of the render arena, at the top of the final primary region.
# | Author: suinevere
# | Dependencies: REGIONS, ARENA_SIZE
# | Globals: N/A
# | Params: N/A
# | Returns: HWRAM address
#----------------------
def arena_base():
    base, size = REGIONS["primary"]
    return base + size - ARENA_SIZE


#----------------------
# | code_limit
# | Description: Highest address an overlay's code may reach: the arena base when the overlay sits below it, else the region's end.
# | Author: suinevere
# | Dependencies: region, arena_base
# | Globals: N/A
# | Params: ovl -- Overlay
# | Returns: HWRAM address
#----------------------
def code_limit(ovl):
    base, size = region(ovl)
    return arena_base() if ovl.below_arena else base + size


#----------------------
# | entry_address
# | Description: Address of one entry vector, 16 bytes per entry in table order from the overlay's region base.
# | Author: suinevere
# | Dependencies: region
# | Globals: N/A
# | Params: ovl -- Overlay, fn -- entry function name
# | Returns: HWRAM address
#----------------------
def entry_address(ovl, fn):
    return region(ovl)[0] + 16 * ovl.entries.index(fn)


#----------------------
# | saturn_overlays
# | Description: The overlays the Saturn build links: OVERLAYS minus targets.SATURN_UNLINKED_OVERLAYS.
# | Author: suinevere
# | Dependencies: OVERLAYS, targets.SATURN_UNLINKED_OVERLAYS
# | Globals: OVERLAYS
# | Params: N/A
# | Returns: list of Overlay
#----------------------
def saturn_overlays():
    return [ovl for ovl in OVERLAYS if ovl.name not in targets.SATURN_UNLINKED_OVERLAYS]


#----------------------
# | code_floor
# | Description: Lowest HWRAM address occupied by any real overlay, or WORK_AREA when none are real.
# | Author: suinevere
# | Dependencies: region, saturn_overlays, WORK_AREA
# | Globals: OVERLAYS, WORK_AREA
# | Params: N/A
# | Returns: HWRAM address
#----------------------
def code_floor():
    bases = [region(o)[0] for o in saturn_overlays() if o.real]
    return min(bases + [WORK_AREA])


#----------------------
# | slot_base
# | Description: LWRAM address of an overlay's bss slot, offset into the secondary slot for a secondary-region overlay.
# | Author: suinevere
# | Dependencies: SLOT_BASE, SECONDARY_BASE
# | Globals: SLOT_BASE, SECONDARY_BASE
# | Params: ovl -- Overlay
# | Returns: LWRAM address
#----------------------
def slot_base(ovl):
    return SLOT_BASE + (SECONDARY_BASE if ovl.region == "secondary" else 0)


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
# | vectors_asm
# | Description: Generates an overlay's Saturn entry vector assembly.
# | Author: suinevere
# | Dependencies: file_block, vector_lines
# | Globals: N/A
# | Params: ovl -- Overlay
# | Returns: assembly source text
#----------------------
def vectors_asm(ovl):
    out = [file_block("%s_vectors.s" % ovl.name,
                      "Generated by saturn/tools/overlay_link.py; %s entry vectors, 16 bytes each at its code region base." % ovl.name.upper(),
                      "tools/overlay_link.py").rstrip("\n")]
    return "\n".join(out + vector_lines(ovl.entries)) + "\n"


#----------------------
# | overlay_ld
# | Description: Generates an overlay's linker script: its code section at the region base, a .ctors_check sanity section, its bss slot and discard rules, and its ASSERTs.
# | Author: suinevere
# | Dependencies: file_block, region, code_limit, slot_base, BSS_BLOCKS, EXTRA_ASSERTS, EXTRA_SYMBOLS
# | Globals: BSS_BLOCKS, EXTRA_ASSERTS, EXTRA_SYMBOLS
# | Params: ovl -- Overlay
# | Returns: linker script text
#----------------------
def overlay_ld(ovl):
    includes = ["INCLUDE gen/%s_symbols.ld" % ovl.name]
    if ovl.name in ("field", "wmap"):
        includes.append("INCLUDE gen/%s_pending.ld" % ovl.name)
    includes += EXTRA_SYMBOLS.get(ovl.name, [])
    asserts = ["ASSERT(_lom_ovl_code_end <= 0x%08X, \"%s code exceeds its HWRAM limit\")" % (code_limit(ovl), ovl.name.upper()),
               "ASSERT(SIZEOF(.ctors_check) == 0, \"%s has static constructors\")" % ovl.name.upper()]
    asserts += EXTRA_ASSERTS[ovl.name]
    return (file_block("%s.ld" % ovl.name,
                       "Generated by saturn/tools/overlay_link.py; links %s at its HWRAM code region with its .bss in the LWRAM slot." % ovl.name.upper(),
                       "tools/overlay_link.py, gen/%s_symbols.ld, main ELF" % ovl.name)
            + "OUTPUT_FORMAT(elf32-sh)\n" + "\n".join(includes) + "\n\nSECTIONS {\n"
            + "\t.code 0x%08X :\n\t{\n" % region(ovl)[0]
            + "\t\t_lom_ovl_code_start = .;\n\t\tKEEP(*(.vectors))\n\t\t*(.text .text.*)\n"
            + "\t\t*(.rodata .rodata.*)\n\t\t*(.data .data.*)\n\t\t. = ALIGN(4);\n\t\t_lom_ovl_code_end = .;\n\t}\n"
            + "\t.ctors_check (NOLOAD) :\n\t{\n\t\t*(.ctors*) *(.dtors*) *(.init_array*) *(.fini_array*)\n\t}\n"
            + "\t.bss_slot 0x%08X (NOLOAD) :\n\t{\n" % slot_base(ovl) + BSS_BLOCKS[ovl.name] + "\t}\n"
            + "\t/DISCARD/ :\n\t{\n\t\t*(.comment) *(.eh_frame) *(.rela*) *(.note*)\n\t}\n}\n\n"
            + "\n".join(asserts) + "\n")


#----------------------
# | main_field_data
# | Description: Relocates MAIN_FIELD_DATA's PS1 addresses into FIELD's LWRAM slot, as convert_disc.py does for gen/field_symbols.ld.
# | Author: suinevere
# | Dependencies: overlay_rules.relocate_pointer, overlay_rules.OVERLAYS, MAIN_FIELD_DATA
# | Globals: MAIN_FIELD_DATA
# | Params: N/A
# | Returns: dict of symbol name to slot address
#----------------------
def main_field_data():
    return {name: overlay_rules.relocate_pointer(address, overlay_rules.OVERLAYS[2])
            for name, address in MAIN_FIELD_DATA.items()}


#----------------------
# | entries_ld
# | Description: Generates the linker script of absolute entry vector addresses for every real overlay, the FIELD data words main uses, and the main link's code floor.
# | Author: suinevere
# | Dependencies: file_block, code_floor, saturn_overlays, entry_address, main_field_data
# | Globals: OVERLAYS
# | Params: N/A
# | Returns: linker script text
#----------------------
def entries_ld():
    out = [file_block("overlay_entries.ld",
                      "Generated by saturn/tools/overlay_link.py; overlay entry vector addresses, FIELD data addresses and the lowest live code region for main's link.",
                      "tools/overlay_link.py").rstrip("\n"),
           "_lom_code_floor = 0x%08X;" % code_floor()]
    for ovl in saturn_overlays():
        if ovl.real:
            out += ["_%s = 0x%08X;" % (fn, entry_address(ovl, fn)) for fn in ovl.entries]
    out += ["_%s = 0x%08X;" % item for item in sorted(main_field_data().items())]
    return "\n".join(out) + "\n"


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
# | Description: Packs the 32-byte header that opens an overlay's OVLnnC.BIN: magic, version, code base and size (zero for a static overlay), data size, and bss range.
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
    base, size = 0, 0
    if ovl.real:
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
# | Description: Extra Dreamcast-only vector slots appended after an overlay's real entries, never shared with Saturn's vectors_asm/entries_ld.
# |              FIELD's field_watch_entry is always compiled (dreamcast/src/overlays/field_watch_entry.c) so this slot resolves in every build; it is a no-op unless LOM_PAD_SCRIPT installs the real watcher.
# | Author: suinevere
# | Dependencies: dreamcast/src/overlays/field_watch_entry.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DC_EXTRA_ENTRIES = {"field": ["field_watch_entry"]}


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
# | Description: An overlay's real entries plus its Dreamcast-only extras, in vector-slot order.
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
# | Description: Every file "gen" writes for a target, in write order, as (file name, text) pairs.
# | Author: suinevere
# | Dependencies: entries_ld, vectors_asm, overlay_ld, dc_entries_ld, dc_regions_h, dc_vectors_asm, dc_overlay_ld, dc_overlays, saturn_overlays
# | Globals: OVERLAYS
# | Params: target -- targets.Target
# | Returns: list of (name, text)
#----------------------
def generated_files(target):
    if target.regions is None:
        files = [("overlay_entries.ld", entries_ld())]
        for ovl in saturn_overlays():
            if ovl.real:
                files += [("%s_vectors.s" % ovl.name, vectors_asm(ovl)), ("%s.ld" % ovl.name, overlay_ld(ovl))]
        return files
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
# | Description: The dry-run summary lines: each linked overlay's code region, and for the Dreamcast its data region and the linker used.
# | Author: suinevere
# | Dependencies: region, dc_overlays, tool_path, OVERLAYS, targets.DREAMCAST
# | Globals: OVERLAYS
# | Params: target -- targets.Target
# | Returns: list of lines
#----------------------
def summary(target):
    if target.regions is None:
        return ["%s: code 0x%08X" % (ovl.name, region(ovl)[0]) for ovl in OVERLAYS if ovl.real]
    lines = []
    for ovl in dc_overlays():
        r = targets.DREAMCAST.regions[ovl.name]
        lines.append("%s: code 0x%08X size 0x%X, data 0x%08X size 0x%X" % (ovl.name, r.code_base, r.code_size, r.data_base, r.data_size))
    return lines + ["linker: %s" % tool_path("ld")]


#----------------------
# | overlay_named
# | Description: Looks up an overlay the target links by name.
# | Author: suinevere
# | Dependencies: OVERLAYS, dc_overlays
# | Globals: OVERLAYS
# | Params: name -- overlay name, target -- targets.Target
# | Returns: Overlay; raises SystemExit for an overlay the target does not link
#----------------------
def overlay_named(name, target):
    choices = OVERLAYS if target.regions is None else dc_overlays()
    for ovl in choices:
        if ovl.name == name:
            return ovl
    raise SystemExit("overlay %s has no %s link" % (name, target.name))


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
USAGE = ("usage: overlay_link.py [--target saturn|dreamcast] [--dry-run] gen [--out DIR] | header NAME NM_FILE OUT [CODE_BIN]"
         " | link NAME MAIN_ELF WORK_DIR OUT OBJ... [--links DEP]... (dreamcast)")


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
    target, dry_run, out, links, words = targets.SATURN, False, None, [], []
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
# | Description: CLI entry point: "gen" (the default) writes the target's generated files, or prints them with --dry-run; "header" packs one OVLnnC.BIN from an nm dump; "link" links and packs a Dreamcast overlay.
# | Author: suinevere
# | Dependencies: parse_args, generated_files, summary, write_text, overlay_named, read_nm, pack_overlay_file, dc_link, USAGE
# | Globals: N/A
# | Params: argv -- CLI arguments, excluding the program name
# | Returns: process exit code
#----------------------
def main(argv):
    target, dry_run, out, links, words = parse_args(argv)
    command = words[0] if words else "gen"
    if command == "gen":
        files = generated_files(target)
        if dry_run:
            sys.stdout.write("".join(line + "\n" for line in summary(target))
                             + "".join("==> %s <==\n%s" % (name, text) for name, text in files))
            return 0
        out = out or "gen"
        os.makedirs(out, exist_ok=True)
        for name, text in files:
            write_text(os.path.join(out, name), text)
        return 0
    if command == "header" and len(words) >= 4:
        ovl = overlay_named(words[1], target)
        with open(words[2]) as f:
            symbols = read_nm(f.read())
        code = b""
        if len(words) > 4:
            with open(words[4], "rb") as f:
                code = f.read()
        with open(words[3], "wb") as f:
            f.write(pack_overlay_file(ovl, symbols, code))
        return 0
    if command == "link" and target.regions is not None and len(words) >= 6:
        os.makedirs(words[3], exist_ok=True)
        dc_link(overlay_named(words[1], target), words[2], words[3], words[4], words[5:], links_against=links)
        return 0
    raise SystemExit(USAGE)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
