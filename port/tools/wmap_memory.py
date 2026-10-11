#----------------------
# | wmap_memory.py
# | Description: WMAP's Saturn memory table: each large linker buffer's PS1 address and gap, Saturn size and region, the resources read into it, and its placement.
# | Author: suinevere
# | Dependencies: disc_image.py, decompress (tools/splat_ext)
# | Globals: Buffer, SLOT_BASE, LWRAM_TOP, HWRAM_BASE, HWRAM_TOP, HWRAM_SECONDARY_BASE, HWRAM_SECONDARY_TOP, REGION_TOPS, ALIGN, MEASURED, DATA_TAIL_ESTIMATE, BUFFERS
# | Params: N/A
# | Returns: N/A
#----------------------
import collections
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools", "splat_ext"))

#----------------------
# | Buffer
# | Description: One WMAP linker buffer: symbol name, PS1 address, PS1 gap to the next symbol, Saturn size, region ("lwram", "hwram" or "hwram_secondary"), resource ids read into it, and an optional group name. Buffers sharing a group name are PS1-adjacent and must be placed as one contiguous span, each member at its PS1 offset relative to the group's lowest ps1_addr; a buffer with group=None is its own one-member group.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Buffer = collections.namedtuple("Buffer", "name ps1_addr ps1_gap size region users group", defaults=(None,))

#----------------------
# | SLOT_BASE
# | Description: Start of the LWRAM overlay slot where WMAP's data and bss begin; must match overlay_rules.SLOT_BASE.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SLOT_BASE = 0x00210000

#----------------------
# | LWRAM_TOP
# | Description: End of overlay_loader_init's LWRAM reservation (FIELD's actor heap end); LWRAM buffers are placed downward from here.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LWRAM_TOP = 0x002FA000

#----------------------
# | HWRAM_BASE
# | Description: Base of the primary HWRAM code region WMAP's code links at.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HWRAM_BASE = 0x0607A000

#----------------------
# | HWRAM_TOP
# | Description: End of the primary HWRAM code region; HWRAM buffers are placed downward from here, above WMAP's code.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HWRAM_TOP = 0x060CA000

#----------------------
# | HWRAM_SECONDARY_BASE
# | Description: Base of the HWRAM secondary code region; only GNAME's code and FIELD's CD buffer use it, and neither is resident with WMAP.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HWRAM_SECONDARY_BASE = 0x060CA000

#----------------------
# | HWRAM_SECONDARY_TOP
# | Description: End of the HWRAM secondary code region, where SGL's work area begins; hwram_secondary buffers are placed downward from here.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HWRAM_SECONDARY_TOP = 0x060D6000

#----------------------
# | REGION_TOPS
# | Description: Each placement region's top address, from which its buffers are placed downward.
# | Author: suinevere
# | Dependencies: LWRAM_TOP, HWRAM_TOP, HWRAM_SECONDARY_TOP
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
REGION_TOPS = {"lwram": LWRAM_TOP, "hwram": HWRAM_TOP, "hwram_secondary": HWRAM_SECONDARY_TOP}

#----------------------
# | ALIGN
# | Description: Alignment of every placed buffer, enough for the s32 and u_long accesses WMAP makes into them.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ALIGN = 8

#----------------------
# | MEASURED
# | Description: WMAP's SH-2 section totals from tools/wmap_measure.py on 2026-10-09.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MEASURED = {"text": 301832, "rodata": 36, "data": 0, "bss": 0}

#----------------------
# | DATA_TAIL_ESTIMATE
# | Description: Bytes of PS1 overlay data kept in the slot beside the placed buffers: the initialised data plus the small zero-filled globals between buffers.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DATA_TAIL_ESTIMATE = 189644

#----------------------
# | WMAP_ANIMATION_BANK_GROUP
# | Description: Group name for the six animation banks; wmap_land_event_17.c, wmap_map_events.c and wmap_travel_sequences.c index past one bank's own end expecting the next bank directly above it (bank_N base + 0x2000*k), so they must stay PS1-adjacent and in PS1 order on Saturn too.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WMAP_ANIMATION_BANK_GROUP = "wmap_animation_banks"

#----------------------
# | LOAD_SPAN_GROUP / LOAD_SPAN_START / LOAD_SPAN_END
# | Description: g_wmap_load_buffer's PS1 scratch span, 0x30000 bytes that deliberately overlap every model-bundle global after it up to g_wmap_man_bundle_empty_slot's end; all zero in the PS1 image and placed as one group so each symbol keeps its PS1 offset from the load buffer.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LOAD_SPAN_GROUP = "wmap_load_span"
LOAD_SPAN_START = 0x800DCF18
LOAD_SPAN_END = 0x8010CF18

#----------------------
# | LOAD_SPAN_SYMBOLS
# | Description: Every WMAP symbol inside the load span with its PS1 address, from config/us/symbols/wmap_symbol_addrs.txt.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LOAD_SPAN_SYMBOLS = [
    ("g_wmap_load_buffer", 0x800DCF18), ("g_wmap_entry_palette", 0x800DCF2C),
    ("g_wmap_mnt_bundle_model_pack_1", 0x800DD7B4), ("g_wmap_mnt_bundle_model_pack_2", 0x800DDE54),
    ("g_wmap_shared_bundle_model_pack_1", 0x800DDEF4), ("g_wmap_effect_model_buffer_1000", 0x800DDF18),
    ("g_wmap_mnt_bundle_model_pack_3", 0x800DE39C), ("g_wmap_fig_bundle_model_pack_2", 0x800DEA88),
    ("g_wmap_shared_bundle_model_pack_2", 0x800DEED0), ("g_wmap_effect_model_buffer_2000", 0x800DEF18),
    ("g_wmap_mnt_bundle_model_pack_4", 0x800DF378), ("g_wmap_fig_bundle_model_pack_3", 0x800DFA64),
    ("g_wmap_jul_bundle_model_pack_3", 0x800DFEAC), ("g_wmap_fig_bundle_model_pack_4", 0x800E0114),
    ("g_wmap_mnt_bundle_empty_slot", 0x800E0354), ("g_wmap_jul_bundle_model_pack_4", 0x800E0720),
    ("g_wmap_fig_bundle_model_pack_5", 0x800E08E4), ("g_wmap_fig_bundle_empty_slot", 0x800E0EB4),
    ("g_wmap_effect_model_buffer_4000", 0x800E0F18), ("g_wmap_jul_bundle_model_pack_5", 0x800E0F94),
    ("g_wmap_effect_model_buffer_5000", 0x800E1F18), ("g_wmap_mgc_bundle_model_pack_3", 0x800E2B1C),
    ("g_wmap_mgc_bundle_model_pack_4", 0x800E2DD8), ("g_wmap_mgc_bundle_model_pack_5", 0x800E33AC),
    ("g_wmap_mgc_bundle_empty_slot", 0x800E3980), ("g_wmap_effect_model_buffer_8000", 0x800E4F18),
    ("g_wmap_man_bundle_model_pack_1", 0x800E7F64), ("g_wmap_jul_bundle_model_pack_6", 0x800E91C0),
    ("g_wmap_man_bundle_model_pack_2", 0x800EA560), ("g_wmap_man_bundle_model_pack_3", 0x800EAA54),
    ("g_wmap_man_bundle_model_pack_4", 0x800EBED8), ("g_wmap_man_bundle_model_pack_5", 0x800ECEB4),
    ("g_wmap_effect_model_buffer_10000", 0x800ECF18), ("g_wmap_man_bundle_model_pack_6", 0x800ED310),
    ("g_wmap_man_bundle_model_pack_7", 0x800EE2EC), ("g_wmap_man_bundle_model_pack_8", 0x800F2238),
    ("g_wmap_man_bundle_model_pack_9", 0x800F3214), ("g_wmap_jul_bundle_empty_slot", 0x800F420C),
    ("g_wmap_man_bundle_empty_slot", 0x800FAE60),
]

#----------------------
# | LOAD_SPAN_USERS
# | Description: Resources read into the load span, keyed by the span symbol they start at: g_wmap_load_buffer's TIMs, VRAM pages, layout image (0x14DE stands for every cell) and Home's base model, and Home's two effect model packs at g_wmap_effect_model_buffer_5000 (+0 and +0x2000).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LOAD_SPAN_USERS = {
    "g_wmap_load_buffer": [0x10C4, 0x10C6, 0x10C8, 0x10CA, 0x10CB, 0x11B9, 0x14DE] + list(range(0x10CE, 0x10D8)) + list(range(0x1453, 0x145D)),
    "g_wmap_effect_model_buffer_5000": [0x11B7, 0x11B8],
}


#----------------------
# | load_span_buffers
# | Description: One LWRAM Buffer per load-span symbol, each sized from its PS1 address to the span's end (the scratch it may use) and grouped so place() keeps every PS1 offset.
# | Author: suinevere
# | Dependencies: Buffer, LOAD_SPAN_SYMBOLS, LOAD_SPAN_USERS, LOAD_SPAN_END, LOAD_SPAN_GROUP
# | Globals: N/A
# | Params: N/A
# | Returns: list of Buffer
#----------------------
def load_span_buffers():
    return [Buffer(name, addr, LOAD_SPAN_END - addr, LOAD_SPAN_END - addr, "lwram", LOAD_SPAN_USERS.get(name, []), LOAD_SPAN_GROUP)
            for name, addr in LOAD_SPAN_SYMBOLS]


#----------------------
# | BUFFERS
# | Description: The large WMAP linker buffers, each sized to its largest measured user rather than its PS1 symbol gap; grouped buffers (see WMAP_ANIMATION_BANK_GROUP) stay at their PS1 gap since no single-buffer user size applies to a shared contiguous span. The load span (load_span_buffers) is one LWRAM group mirroring PS1 0x800DCF18..0x8010CF18. g_wmap_sprite_actors moved to "hwram" per Step 6. g_wmap_packet_buffer_0 moved to "hwram_secondary" because the slot also holds WMAP's 19,896-byte PS1 rodata. g_wmap_frames is the two 0x7E40-byte WmapFrames; D_801295BC is only frame 0's disp_env inside it.
# | Author: suinevere
# | Dependencies: Buffer, WMAP_ANIMATION_BANK_GROUP, load_span_buffers
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BUFFERS = [
    Buffer("g_wmap_land_image_pack", 0x8013C628, 0x45C00, 0x41F40, "lwram", [0x10C5]),
    Buffer("g_wmap_help_image", 0x8019D6E0, 0x10400, 0xF220, "lwram",
           [0x114B, 0x114C, 0x114D, 0x114E, 0x114F, 0x1150, 0x1151, 0x1152, 0x1153]),
    Buffer("g_wmap_packet_buffer_0", 0x8010CF18, 0x8000, 0x8000, "hwram_secondary", []),
    Buffer("g_wmap_packet_buffer_1", 0x80114F18, 0x8000, 0x8000, "lwram", []),
] + load_span_buffers() + [
    Buffer("g_wmap_sprite_actors", 0x800D9268, 0x2BD4, 0x2BD4, "hwram", []),
    Buffer("g_wmap_effect_texture_buffer_0", 0x80182E40, 0x8400, 0x8220, "lwram", [
        0x110A, 0x111B, 0x1125, 0x112C, 0x1133, 0x113D, 0x1147, 0x1155, 0x115A, 0x1163, 0x116E, 0x1176,
        0x117F, 0x1187, 0x118F, 0x1197, 0x11A0, 0x11A7, 0x11B0, 0x11BA, 0x11C3, 0x11CC, 0x11D5, 0x11E0,
        0x11ED, 0x11F9, 0x1205, 0x120E, 0x122C, 0x1233, 0x123E, 0x1246, 0x124B]),
    Buffer("g_wmap_effect_texture_buffer_1", 0x8018B240, 0x8400, 0x8220, "lwram", [
        0x110B, 0x111C, 0x1126, 0x112D, 0x1134, 0x113E, 0x1148, 0x1156, 0x115B, 0x1164, 0x116F, 0x1177,
        0x1180, 0x1188, 0x1190, 0x1198, 0x11A1, 0x11A8, 0x11B1, 0x11BB, 0x11C4, 0x11CD, 0x11D6, 0x11E1,
        0x11EE, 0x11FA, 0x1206, 0x120F, 0x122D, 0x1234, 0x123F, 0x1247, 0x124C]),
    Buffer("g_wmap_effect_texture_buffer_2", 0x80193640, 0x9C00, 0x8220, "lwram", [
        0x110C, 0x111D, 0x1127, 0x112E, 0x1135, 0x113F, 0x115C, 0x1165, 0x1170, 0x1178, 0x1181, 0x1189,
        0x1191, 0x1199, 0x11A2, 0x11A9, 0x11B2, 0x11BC, 0x11C5, 0x11CE, 0x11D7, 0x11E2, 0x11EF, 0x11FB,
        0x1207, 0x1210, 0x122E, 0x1235, 0x1240, 0x124D]),
    Buffer("g_wmap_animation_bank_0", 0x8011D538, 0x2000, 0x2000, "lwram", [
        0x1108, 0x111E, 0x1129, 0x112F, 0x1136, 0x1140, 0x1145, 0x1146, 0x1158, 0x115D, 0x1168, 0x1173,
        0x1179, 0x1184, 0x118A, 0x1192, 0x119A, 0x11A3, 0x11AA, 0x11B4, 0x11BD, 0x11C7, 0x11CF, 0x11D8,
        0x11E8, 0x11F0, 0x11FC, 0x1208, 0x1209, 0x120A, 0x1211, 0x121B, 0x1223, 0x122F, 0x1236, 0x1248,
        0x124E], WMAP_ANIMATION_BANK_GROUP),
    Buffer("g_wmap_animation_bank_1", 0x8011F538, 0x2000, 0x2000, "lwram", [
        0x1109, 0x111F, 0x1128, 0x1130, 0x1137, 0x1141, 0x1146, 0x1159, 0x115E, 0x1169, 0x1174, 0x117A,
        0x1185, 0x118B, 0x1193, 0x119B, 0x11A4, 0x11AB, 0x11B3, 0x11BE, 0x11C6, 0x11D0, 0x11D9, 0x11E9,
        0x11F1, 0x11FD, 0x1209, 0x1212, 0x1230, 0x1237, 0x1249, 0x124F], WMAP_ANIMATION_BANK_GROUP),
    Buffer("g_wmap_animation_bank_2", 0x80121538, 0x2000, 0x2000, "lwram", [
        0x1120, 0x112A, 0x1131, 0x1142, 0x115F, 0x116A, 0x1175, 0x1186, 0x118C, 0x1194, 0x119C, 0x11A5,
        0x11AC, 0x11B5, 0x11BF, 0x11C8, 0x11D1, 0x11DA, 0x11EA, 0x11F2, 0x11FE, 0x120A, 0x1213, 0x1231,
        0x1238, 0x1244, 0x1250], WMAP_ANIMATION_BANK_GROUP),
    Buffer("g_wmap_animation_bank_3", 0x80123538, 0x2000, 0x2000, "lwram", [
        0x10D8, 0x10DA, 0x10DC, 0x10DE, 0x1121, 0x1149, 0x116B, 0x11B6, 0x11C9, 0x11EB, 0x11F3, 0x11FF,
        0x1214, 0x1243], WMAP_ANIMATION_BANK_GROUP),
    Buffer("g_wmap_animation_bank_4", 0x80125538, 0x2000, 0x2000, "lwram",
           [0x1123, 0x116C, 0x11EC, 0x11F4, 0x1241], WMAP_ANIMATION_BANK_GROUP),
    Buffer("g_wmap_animation_bank_5", 0x80127538, 0x2000, 0x2000, "lwram",
           [0x1124, 0x116D, 0x1242], WMAP_ANIMATION_BANK_GROUP),
    Buffer("g_wmap_frames", 0x80129560, 0xFC80, 0xFC80, "lwram", []),
    Buffer("g_wmap_resident_land_animation_data", 0x801ADBA0, 0x2000, 0xA68, "lwram", [0x10C7]),
]


#----------------------
# | align_down
# | Description: Rounds an address down to ALIGN.
# | Author: suinevere
# | Dependencies: ALIGN
# | Globals: N/A
# | Params: value -- address
# | Returns: aligned address
#----------------------
def align_down(value):
    return value & ~(ALIGN - 1)


#----------------------
# | place
# | Description: Places buffers downward from their region's top in list order, each ALIGN-aligned. A buffer's group (if any) is placed as one contiguous span sized to cover every member's PS1 address and gap, with each member kept at its PS1 offset from the group's lowest ps1_addr; an ungrouped buffer is its own one-member group and places exactly as before.
# | Author: suinevere
# | Dependencies: REGION_TOPS, align_down
# | Globals: N/A
# | Params: buffers -- list of Buffer
# | Returns: dict of buffer name to Saturn address
#----------------------
def place(buffers):
    cursor = dict(REGION_TOPS)
    placed = {}
    placed_groups = set()
    for buf in buffers:
        key = buf.group or buf.name
        if key in placed_groups:
            continue
        placed_groups.add(key)
        members = [b for b in buffers if (b.group or b.name) == key]
        base = min(b.ps1_addr for b in members)
        span = max(b.ps1_addr + b.ps1_gap for b in members) - base
        cursor[buf.region] = align_down(cursor[buf.region] - span)
        group_base = cursor[buf.region]
        for member in members:
            placed[member.name] = group_base + (member.ps1_addr - base)
    return placed


#----------------------
# | lowest
# | Description: The lowest placed address in a region, which the region's other contents must stay below.
# | Author: suinevere
# | Dependencies: BUFFERS
# | Globals: BUFFERS
# | Params: placed -- name to address, region -- "lwram", "hwram" or "hwram_secondary"
# | Returns: address
#----------------------
def lowest(placed, region):
    return min(placed[b.name] for b in BUFFERS if b.region == region)


#----------------------
# | linker_lines
# | Description: Linker assignments defining each placed buffer's C symbol.
# | Author: suinevere
# | Dependencies: BUFFERS
# | Globals: BUFFERS
# | Params: placed -- name to address
# | Returns: list of linker script lines
#----------------------
def linker_lines(placed):
    return ["_%s = 0x%08X;" % (b.name, placed[b.name]) for b in BUFFERS]


#----------------------
# | WMAP_STREAM_IDS
# | Description: Resource ids WMAP reads through cdrom_stream (wmap_main.c:1636 WMAP_INTERFACE_IMAGE_RESOURCE 0x10C4, wmap_main.c:1665 WMAP_LAND_IMAGE_PACK_RESOURCE 0x10C5, wmap_main.c:1685 WMAP_RESIDENT_LAND_IMAGE_RESOURCE 0x10C6) and wmap_main.c:1574's per-cell layout image, whose cell 0 id 0x14DE stands for all 361; the Saturn build delivers these decompressed, same as convert_disc.STREAM_RESOURCES. Excludes wmap_resource_support.c's func_80065078 (unreachable from WMAP, no callers in src/overlays/wmap).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WMAP_STREAM_IDS = {0x10C4, 0x10C5, 0x10C6, 0x14DE}

#----------------------
# | user_size
# | Description: Size of a resource as the Saturn build delivers it: decompressed after the leading flag byte for a stream resource (convert_disc's table or WMAP_STREAM_IDS), raw otherwise.
# | Author: suinevere
# | Dependencies: disc_image.read_resource, decompress, convert_disc.STREAM_RESOURCES, WMAP_STREAM_IDS
# | Globals: WMAP_STREAM_IDS
# | Params: image -- DiscImage, table -- resource table, res_id -- resource index
# | Returns: byte size
#----------------------
def user_size(image, table, res_id):
    import convert_disc
    import disc_image
    from decompress import decompress
    data = disc_image.read_resource(image, table[res_id])
    if res_id in convert_disc.STREAM_RESOURCES or res_id in WMAP_STREAM_IDS:
        return len(decompress(data[1:]))
    return len(data)
