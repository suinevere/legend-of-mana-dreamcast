"""----------------------
 | call_compare.py
 | Description: Reads and writes the recording build's call records (DIR/NAME.rec) and its per-script call counts, and checks an assembly routine against every record of its C function through vmu_harness --call.
 | Author: suinevere
 | Dependencies: pocketstation/runner/vmu_record.c (the writer), pocketstation/harness/vmu_harness.c (--call), vmu_callgraph.py, pocketstation/src/vmu_game/manifest.txt, game.inc, Makefile
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------
 Checking a routine: make vmu-record (records), make -C pocketstation/harness, make -C pocketstation/src/vmu_game call,
 then call_compare.py FUNCTION. Each record becomes build/call/FUNCTION/K.state: every bank 1, work RAM and XRAM byte
 FILL, the record's vmu_ram fields placed through hw.json, its frame buffer, the manifest's ARGS in registers or
 ARG_BLOCK, a poison line per routine in the image (DESTROYS less RESULT), the record's vmu_* results to replay and, for
 each vmu_save_read that succeeds, the 128 bytes it leaves in vmu_ram, which the harness copies into the fields there.
 The harness writes K.out; the call passes when its status is returned, its RESULT matches, its rt_* calls map to the
 record's vmu_* calls with the same arguments in the same order, and every byte of bank 1, work RAM and XRAM is what
 the record leaves (fields from vmu_ram after, the screen from the frame buffer after, a pointer's pointee after, the
 rest unchanged), only value slots in ARG_BLOCK, a struct pointee's padding and the R0-R3 the routine declares left
 free; and every register outside DESTROYS and RESULT (ACC, B, C, TRL, TRH, VRMAD, FPR, PSW's flags) comes back as it
 went in, PSW's bank bits always. Registers no argument sets enter as 0xA5 (PSW's flags too, so CY is set).
 Record layout, little-endian, records back to back in DIR/NAME.rec:
   u32 magic "REC1" | u32 record length | u16 version (1) | u16 function id (sorted-name order) | u16 key bits
   key bytes (ceil(bits/8)): bit 2k / 2k+1 = probe k's condition held / did not, bit 2k = case label k reached
   u8 depth (wrapped calls open below this one) | u8 flags (bit 0 synthetic) | u32 frame (vmu_runner sample count)
   u8 n + script name | u8 n + function name
   u8 nargs, each: u8 kind (0 value, 1 pointer, 2 null pointer) | u8 size | size bytes (value or pointee before)
                   | for a pointer, size bytes of the pointee after
   u8 result size | result bytes
   vmu_ram before (0x400) | frame buffer before (192) | vmu_ram after (0x400) | frame buffer after (192)
   u32 n vmu_* calls, each: u8 id (HW_NAMES) | u8 depth of the call making it | u8 nargs | u32 args | u32 return
 DIR/calls-SCRIPT.txt: NAME ENTERED RETURNED per function entered in that run.
 DIR/rom-SCRIPT.txt: START END per run of table offsets vmu_rom8 read in that run, hex, END exclusive.
"""

import argparse
import glob
import json
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vmu_callgraph as vc  # noqa: E402
import vmu_flash  # noqa: E402

"""----------------------
 | MAGIC / VERSION / RAM_BYTES / FB_BYTES
 | Description: A record's first word, the format version, and the sizes of vmu_ram and the frame buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
MAGIC = 0x31434552
VERSION = 1
RAM_BYTES = 0x400
FB_BYTES = 192

"""----------------------
 | HANDLER
 | Description: The game's interrupt handler label, which --call's rt_idle stand-in runs once for each vmu_irq_pending a record shows inside a vmu_idle.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu_game/system.s (g_irq_callback), pocketstation/harness/vmu_harness.c (enter_handler)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
HANDLER = "g_irq_callback"


"""----------------------
 | HW_NAMES
 | Description: The logged vmu_hw.h calls by id, in vmu_record.c's HW_* order.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
HW_NAMES = ["vmu_present", "vmu_idle", "vmu_irq_enable", "vmu_irq_disable", "vmu_irq_ack", "vmu_irq_pending",
            "vmu_irq_levels", "vmu_irq_mask", "vmu_set_handler", "vmu_tick_mode", "vmu_sound_play",
            "vmu_sound_stop", "vmu_sys_flags", "vmu_power", "vmu_save_read", "vmu_save_write", "vmu_buzz",
            "vmu_lcd", "vmu_buttons"]


"""----------------------
 | Arg
 | Description: One recorded argument: kind (0 value, 1 pointer, 2 null pointer), its bytes or the pointee's before the call, and the pointee's after it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Arg:
    def __init__(self, kind: int, before: bytes, after: bytes = b""):
        self.kind, self.before, self.after = kind, before, after

    @property
    def value(self) -> int:
        return int.from_bytes(self.before, "little")


"""----------------------
 | HwCall
 | Description: One vmu_* call made inside a recorded call: id, name, depth of the game call making it, arguments and return.
 | Author: suinevere
 | Dependencies: HW_NAMES
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class HwCall:
    def __init__(self, id_: int, depth: int, args: list, ret: int):
        self.id, self.depth, self.args, self.ret = id_, depth, args, ret

    @property
    def name(self) -> str:
        return HW_NAMES[self.id]


"""----------------------
 | Record
 | Description: One recorded call: function id and name, path key, depth, synthetic flag, frame, script, arguments, result, RAM and frame buffer before and after, vmu_* calls and the raw bytes it was read from.
 | Author: suinevere
 | Dependencies: Arg, HwCall
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Record:
    def __init__(self, **kw):
        self.__dict__.update(kw)

    @property
    def key_bits(self) -> set:
        return {i for i in range(self.nbits) if self.key[i >> 3] >> (i & 7) & 1}


"""----------------------
 | parse
 | Description: Reads one record from a buffer.
 | Author: suinevere
 | Dependencies: Record, Arg, HwCall
 | Globals: MAGIC, VERSION, RAM_BYTES, FB_BYTES
 | Params: buf -- bytes; at -- the record's offset
 | Returns: (Record, offset past it)
 ----------------------"""
def parse(buf: bytes, at: int = 0):
    magic, length, version, fid, nbits = struct.unpack_from("<IIHHH", buf, at)
    if magic != MAGIC or version != VERSION or at + length > len(buf):
        raise ValueError("bad record at %d" % at)
    p = at + 14
    nkey = (nbits + 7) // 8
    key = buf[p:p + nkey]
    p += nkey
    depth, flags, frame = struct.unpack_from("<BBI", buf, p)
    p += 6
    strings = []
    for _ in range(2):
        n = buf[p]
        strings.append(buf[p + 1:p + 1 + n].decode())
        p += 1 + n
    args = []
    nargs = buf[p]
    p += 1
    for _ in range(nargs):
        kind, size = buf[p], buf[p + 1]
        p += 2
        before = buf[p:p + size]
        p += size
        after = b""
        if kind == 1:
            after = buf[p:p + size]
            p += size
        args.append(Arg(kind, before, after))
    rsize = buf[p]
    result = buf[p + 1:p + 1 + rsize]
    p += 1 + rsize
    blocks = []
    for size in (RAM_BYTES, FB_BYTES, RAM_BYTES, FB_BYTES):
        blocks.append(buf[p:p + size])
        p += size
    (nhw,) = struct.unpack_from("<I", buf, p)
    p += 4
    hw = []
    for _ in range(nhw):
        id_, hdepth, hn = buf[p], buf[p + 1], buf[p + 2]
        p += 3
        hargs = list(struct.unpack_from("<%dI" % hn, buf, p))
        p += 4 * hn
        (ret,) = struct.unpack_from("<I", buf, p)
        p += 4
        hw.append(HwCall(id_, hdepth, hargs, ret))
    if p != at + length:
        raise ValueError("record at %d is %d bytes, fields take %d" % (at, length, p - at))
    rec = Record(fid=fid, nbits=nbits, key=key, depth=depth, synthetic=bool(flags & 1), frame=frame,
                 script=strings[0], name=strings[1], args=args, result=result, ram_before=blocks[0],
                 fb_before=blocks[1], ram_after=blocks[2], fb_after=blocks[3], hw=hw, raw=buf[at:p])
    return rec, p


"""----------------------
 | encode
 | Description: Writes a record back to bytes in the layout parse reads.
 | Author: suinevere
 | Dependencies: Record
 | Globals: MAGIC, VERSION
 | Params: rec -- the record
 | Returns: bytes
 ----------------------"""
def encode(rec: Record) -> bytes:
    body = bytearray()
    body += struct.pack("<HHH", VERSION, rec.fid, rec.nbits) + bytes(rec.key)
    body += struct.pack("<BBI", rec.depth, 1 if rec.synthetic else 0, rec.frame)
    for s in (rec.script, rec.name):
        b = s.encode()
        body += bytes([len(b)]) + b
    body += bytes([len(rec.args)])
    for a in rec.args:
        body += bytes([a.kind, len(a.before)]) + a.before
        if a.kind == 1:
            body += a.after
    body += bytes([len(rec.result)]) + rec.result
    body += rec.ram_before + rec.fb_before + rec.ram_after + rec.fb_after
    body += struct.pack("<I", len(rec.hw))
    for c in rec.hw:
        body += bytes([c.id, c.depth, len(c.args)]) + struct.pack("<%dI" % len(c.args), *c.args) + struct.pack("<I", c.ret)
    return struct.pack("<II", MAGIC, len(body) + 8) + bytes(body)


"""----------------------
 | read_records
 | Description: Every record in one .rec file, in file order.
 | Author: suinevere
 | Dependencies: parse
 | Globals: N/A
 | Params: path -- the .rec file
 | Returns: list of Record
 ----------------------"""
def read_records(path: str) -> list:
    with open(path, "rb") as f:
        buf = f.read()
    out, at = [], 0
    while at < len(buf):
        rec, at = parse(buf, at)
        out.append(rec)
    return out


"""----------------------
 | call_counts
 | Description: Calls entered and returned per function, summed over a recording directory's calls-SCRIPT.txt files; the synthetic run's counts only when asked.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: rec_dir -- the recording directory; synthetic -- whether to include calls-synthetic.txt
 | Returns: dict name -> (entered, returned)
 ----------------------"""
def call_counts(rec_dir: str, synthetic: bool = False) -> dict:
    out = {}
    for path in sorted(glob.glob(os.path.join(rec_dir, "calls-*.txt"))):
        if not synthetic and os.path.basename(path) == "calls-synthetic.txt":
            continue
        with open(path) as f:
            for line in f:
                name, entered, returned = line.split()
                e, r = out.get(name, (0, 0))
                out[name] = (e + int(entered), r + int(returned))
    return out


"""----------------------
 | REPO / PATHS
 | Description: The repository root and the default paths call_compare.py works from, repo-relative so the same paths serve the host and the lom-pocket container mounted at /work.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu_game/Makefile, pocketstation/harness/Makefile, make vmu-record
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
PATHS = {
    "rec": "pocketstation/build/record/rec",
    "image": "pocketstation/build/vmu_game/callimg.bin",
    "map": "pocketstation/build/vmu_game/callimg.map",
    "hw": "pocketstation/build/vmu_game/hw.json",
    "manifest": "pocketstation/src/vmu_game/manifest.txt",
    "harness": "pocketstation/build/vmu_harness",
    "game_inc": "pocketstation/src/vmu_game/game.inc",
    "game_h": "pocketstation/src/vmu/game.h",
}

"""----------------------
 | FILL / POISON / BANK1_BYTES / WRAM_BYTES / XRAM_BYTES / INDIRECT
 | Description: The byte every RAM, work RAM and XRAM byte no field or argument holds starts as, so a stray write of 0 shows; the harness's poison byte; the sizes the harness dumps; and the indirect registers R0-R3 at bank 1 0x00-0x03.
 | Author: suinevere
 | Dependencies: pocketstation/harness/vmu_harness.c (POISON, write_out)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FILL = 0x5A
POISON = 0xA5
BANK1_BYTES = 0x100
WRAM_BYTES = 0x200
XRAM_BYTES = 0xC6
INDIRECT = ("R0", "R1", "R2", "R3")

"""----------------------
 | FLASH_BYTES / RT_SAVE_ADDR
 | Description: The size of a whole VMU flash image, as vmu_flash.py builds one with the pet save, and rt_save's work RAM address (rt_file.s RT_SAVE_HI:RT_SAVE_LO).
 | Author: suinevere
 | Dependencies: pocketstation/tools/vmu_flash.py, pocketstation/src/vmu_rt/rt_file.s
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
FLASH_BYTES = 0x20000
RT_SAVE_ADDR = 0x1E8

"""----------------------
 | SAVE_BLOCK
 | Description: The bytes one vmu_save_read copies into vmu_ram.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/vmu_host.c (vmu_save_read), pocketstation/harness/vmu_harness.c (SAVE_BLOCK)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
SAVE_BLOCK = 0x80

"""----------------------
 | SOUND_COUNTS
 | Description: vmu_sound_play's counts per sample for rt_sound_play's rate in C (0 slow, else fast), VMU_SOUND_SLOW and VMU_SOUND_FAST.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu/vmu_hw.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
SOUND_COUNTS = (500, 71)

"""----------------------
 | RT_ARGS
 | Description: How each vmu_* call's recorded arguments come from the rt_* routine's registers at entry, as the runtime doc's table has them; a recorded vmu_* call missing here has no rt_* counterpart and is reported, never skipped.
 | Author: suinevere
 | Dependencies: docs/en/technical/reference/vmu-runtime.md (Routines), pocketstation/harness/vmu_harness.c (RT_ENTRIES)
 | Globals: SOUND_COUNTS
 | Params: N/A
 | Returns: N/A
 ----------------------"""
RT_ARGS = {
    "vmu_present": lambda r, h: [],
    "vmu_idle": lambda r, h: [],
    "vmu_irq_enable": lambda r, h: [r["TRH"] << 8 | r["TRL"]],
    "vmu_irq_disable": lambda r, h: [r["TRH"] << 8 | r["TRL"]],
    "vmu_irq_ack": lambda r, h: [r["TRH"] << 8 | r["TRL"]],
    "vmu_irq_pending": lambda r, h: [],
    "vmu_irq_levels": lambda r, h: [],
    "vmu_irq_mask": lambda r, h: [],
    "vmu_set_handler": lambda r, h: [h(r["TRH"] << 8 | r["TRL"])],
    "vmu_tick_mode": lambda r, h: [r["ACC"]],
    "vmu_sound_play": lambda r, h: [r["TRH"] << 16 | r["TRL"] << 8 | r["B"], SOUND_COUNTS[1 if r["C"] else 0]],
    "vmu_sound_stop": lambda r, h: [],
    "vmu_sys_flags": lambda r, h: [],
    "vmu_power": lambda r, h: [r["ACC"]],
    "vmu_save_read": lambda r, h: [r["ACC"], r["TRH"] << 8 | r["TRL"]],
    "vmu_save_write": lambda r, h: [r["ACC"], r["TRH"] << 8 | r["TRL"]],
    "vmu_buzz": lambda r, h: [r["ACC"]],
    "vmu_lcd": lambda r, h: [r["ACC"]],
    "vmu_buttons": lambda r, h: [],
}

"""----------------------
 | RESULT_REGS
 | Description: The registers a manifest RESULT occupies, little-endian byte order, which a routine's poison list leaves alone.
 | Author: suinevere
 | Dependencies: pocketstation/src/vmu_game/manifest.txt (RESULT)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
RESULT_REGS = {"ACC": ["ACC"], "B": ["B"], "C": ["C"], "TR": ["TRL", "TRH"], "TRB": ["B", "TRL", "TRH"]}


"""----------------------
 | Field / Slot
 | Description: A field's place (gen_ram.py hw: name, vmu_ram offset, bank1 or wram, address, size); a mem: argument's or result's place in ARG_BLOCK (name, work RAM address, size, pointer or not, struct layout or None).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Field:
    def __init__(self, name, offset, space, addr, size):
        self.name, self.offset, self.space, self.addr, self.size = name, offset, space, addr, size


class Slot:
    def __init__(self, name, addr, size, pointer=False, layout=None):
        self.name, self.addr, self.size, self.pointer, self.layout = name, addr, size, pointer, layout


"""----------------------
 | hw_placement
 | Description: Reads gen_ram.py hw's placement JSON.
 | Author: suinevere
 | Dependencies: Field
 | Globals: N/A
 | Params: path -- hw.json
 | Returns: dict name -> Field
 ----------------------"""
def hw_placement(path: str) -> dict:
    with open(path) as f:
        return {h["name"]: Field(h["name"], h["offset"], h["space"], h["addr"], h["size"]) for h in json.load(f)}


"""----------------------
 | arg_block
 | Description: ARG_BLOCK's work RAM address and size, read from game.inc so the assembly and the compare share one definition.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- game.inc
 | Returns: (address, size)
 ----------------------"""
def arg_block(path: str) -> tuple:
    text = open(path).read()
    values = {}
    for name in ("ARG_BLOCK", "ARG_BLOCK_SIZE"):
        m = re.search(r"^%s\s+EQU\s+(\$[0-9A-Fa-f]+|\d+)\s*$" % name, text, re.M | re.I)
        if not m:
            raise ValueError("%s has no %s" % (path, name))
        values[name] = int(m.group(1)[1:], 16) if m.group(1).startswith("$") else int(m.group(1))
    return values["ARG_BLOCK"], values["ARG_BLOCK_SIZE"]


"""----------------------
 | C_SIZES / struct_layouts
 | Description: The sizes of the C scalar types game.h's structs use, and every typedef struct in game.h laid out by the C's natural alignment, so SpriteInfo and FrameInfo compare field by field and never by their padding.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- game.h
 | Returns: dict struct name -> (size, [(field, offset, size)])
 ----------------------"""
C_SIZES = {"uint8_t": 1, "int8_t": 1, "uint16_t": 2, "int16_t": 2, "uint32_t": 4, "int32_t": 4}


def struct_layouts(path: str) -> dict:
    text = re.sub(r"/\*.*?\*/", "", open(path).read(), flags=re.S)
    out = {}
    for body, name in re.findall(r"typedef\s+struct\s*\{([^}]*)\}\s*(\w+)\s*;", text):
        fields, at, align = [], 0, 1
        for decl in body.split(";"):
            decl = decl.strip()
            if not decl:
                continue
            ctype, names = decl.split(None, 1)
            if ctype not in C_SIZES:
                raise ValueError("%s: %s has a %s field" % (path, name, ctype))
            size = C_SIZES[ctype]
            align = max(align, size)
            for member in names.split(","):
                m = re.fullmatch(r"\s*(\w+)\s*(?:\[(\d+)\])?\s*", member)
                count = int(m.group(2)) if m.group(2) else 1
                at = (at + size - 1) // size * size
                fields.append((m.group(1), at, size * count))
                at += size * count
        out[name] = ((at + align - 1) // align * align, fields)
    return out


"""----------------------
 | read_out
 | Description: Reads vmu_harness --call's output: status, cycles, stack_max and stack_depth, the registers on return (reg) and at entry (entry), each runtime call as (vmu name, rt name, registers at entry), and bank 1, work RAM and XRAM.
 | Author: suinevere
 | Dependencies: pocketstation/harness/vmu_harness.c (write_out)
 | Globals: N/A
 | Params: path -- the output file
 | Returns: dict
 ----------------------"""
def read_out(path: str) -> dict:
    out = {"reg": {}, "entry": {}, "hw": []}
    with open(path) as f:
        for line in f:
            words = line.split()
            if not words:
                continue
            tag = words[0]
            if tag == "status":
                out["status"] = words[1]
            elif tag in ("cycles", "stack_depth"):
                out[tag] = int(words[1])
            elif tag == "stack_max":
                out[tag] = int(words[1], 16)
            elif tag in ("reg", "entry"):
                out[tag][words[1]] = int(words[2], 16)
            elif tag == "hw":
                regs = dict(zip(("ACC", "B", "C", "TRH", "TRL"), (int(w, 16) for w in words[3:8])))
                out["hw"].append((words[1], words[2], regs))
            elif tag in ("bank1", "wram", "xram"):
                out[tag] = bytes.fromhex(words[1])
    return out


"""----------------------
 | Mapping
 | Description: One function's manifest row read for --call: routine, ARGS as (param, REG), RESULT, the poison list (DESTROYS less the result's registers), and the C parameter and result types for struct layouts.
 | Author: suinevere
 | Dependencies: vmu_callgraph (parse_manifest, discover, split_params), RESULT_REGS
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Mapping:
    def __init__(self, row: list, func=None, layouts=None):
        self.name, self.routine, args, self.result, destroys, self.status = row[0], row[3], row[4], row[5], row[6], row[7]
        self.args = [] if args == "-" else [tuple(a.split("=", 1)) for a in args.split(",")]
        self.result = None if self.result == "-" else self.result
        keep = set(RESULT_REGS.get(self.result, []))
        self.destroys = [r for r in ([] if destroys == "-" else destroys.split(",")) if r not in keep]
        layouts = layouts or {}
        params = vc.split_params(func.params) if func else []
        self.arg_layouts = [layouts.get(re.sub(r"^const\s+", "", p).split()[0]) for p in params] + [None] * len(self.args)
        self.result_layout = layouts.get(func.ret.strip()) if func else None

    @property
    def poison(self) -> str:
        return ",".join(self.destroys) or "-"


"""----------------------
 | Context
 | Description: Everything one call_compare run needs: paths, manifest mappings, function ids, image labels, field placement, ARG_BLOCK and struct layouts. For a flash image with the pet save it also holds rt_save's bytes.
 | Author: suinevere
 | Dependencies: Mapping, hw_placement, arg_block, struct_layouts, vmu_callgraph, vmu_flash.rt_save_bytes, RT_SAVE_ADDR
 | Globals: PATHS, REPO
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Context:
    def __init__(self, **paths):
        self.paths = dict(PATHS, **{k: v for k, v in paths.items() if v})
        p = lambda k: os.path.join(REPO, self.paths[k])
        rows = vc.parse_manifest(p("manifest"))
        funcs = vc.discover()
        self.layouts = struct_layouts(p("game_h"))
        self.maps = {n: Mapping(r, funcs.get(n), self.layouts) for n, r in rows.items()}
        self.fids = {n: i for i, n in enumerate(sorted(n for n, f in funcs.items() if not f.inline))}
        self.labels = {}
        with open(p("map")) as f:
            for line in f:
                addr, name = line.split()
                self.labels.setdefault(name.lower(), int(addr, 16))
        self.fields = hw_placement(p("hw"))
        self.block, self.block_size = arg_block(p("game_inc"))
        self.rt_save = None
        if os.path.getsize(p("image")) == FLASH_BYTES:
            with open(p("image"), "rb") as f:
                self.rt_save = vmu_flash.rt_save_bytes(f.read())
        self.owner = {}
        for fl in self.fields.values():
            for k in range(fl.size):
                self.owner[(fl.space, fl.addr + k)] = (fl.name, k)

    """----------------------
     | Context.handler_id
     | Description: The function id vmu_set_handler records for the routine at a code address, 0xFFFF for none, as rt_set_handler takes an address.
     | Author: suinevere
     | Dependencies: N/A
     | Globals: N/A
     | Params: addr -- the TRH:TRL address
     | Returns: the id, or 0x10000 | addr when no manifest routine is there
     ----------------------"""
    def handler_id(self, addr: int) -> int:
        if addr == 0:
            return 0xFFFF
        for m in self.maps.values():
            if self.labels.get(m.routine.lower()) == addr and m.name in self.fids:
                return self.fids[m.name]
        return 0x10000 | addr

    """----------------------
     | Context.name_of
     | Description: What owns a byte of bank 1, work RAM or XRAM, for a difference: its field, R0-R3, rt_reserved, rt_input, the arg block, rt_save, the icons, or unused.
     | Author: suinevere
     | Dependencies: N/A
     | Globals: INDIRECT
     | Params: space -- bank1, wram or xram; addr -- the address
     | Returns: the name
     ----------------------"""
    def name_of(self, space: str, addr: int) -> str:
        if (space, addr) in self.owner:
            return self.owner[(space, addr)][0]
        if space == "bank1":
            return INDIRECT[addr] if addr < 4 else "rt_reserved" if addr < 0x20 else "rt_input" if addr >= 0xF9 else "unused bank1"
        if space == "wram" and self.block <= addr < self.block + self.block_size:
            return "arg block"
        return "rt_save" if space == "wram" and addr >= 0x1E8 else "icons" if space == "xram" else "unused " + space


"""----------------------
 | Unsupported
 | Description: A record --call cannot set up (a null pointer, a mapping that does not fit the record), reported as that call's difference.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
class Unsupported(Exception):
    pass


"""----------------------
 | Setup
 | Description: One record turned into a --call state: the registers, the ARG_BLOCK slots, the state file's text (a successful vmu_save_read's block taken from vmu_ram after, as the record holds no save image), and the bank 1, work RAM and XRAM the call must leave, with the bytes it may leave as they like. With a pet save in the image it also sets rt_save.
 | Author: suinevere
 | Dependencies: Context, Mapping, Slot, Unsupported
 | Globals: FILL, INDIRECT, BANK1_BYTES, WRAM_BYTES, XRAM_BYTES, FB_BYTES, SAVE_BLOCK
 | Params: ctx -- the Context; m -- the function's Mapping; rec -- the Record
 | Returns: N/A
 ----------------------"""
class Setup:
    def __init__(self, ctx: Context, m: Mapping, rec):
        if len(m.args) != len(rec.args):
            raise Unsupported("the manifest maps %d arguments, the record has %d" % (len(m.args), len(rec.args)))
        regs, self.slots, filled, at = {}, [], [], ctx.block
        for (param, reg), arg, layout in zip(m.args, rec.args, m.arg_layouts):
            if arg.kind == 2:
                raise Unsupported("null pointer argument %s" % param)
            v = arg.value
            if reg in ("ACC", "B", "C"):
                regs[reg] = v & 0xFF
            elif reg == "TR":
                regs["TRL"], regs["TRH"] = v & 0xFF, v >> 8 & 0xFF
            elif reg == "TRB":
                regs["B"], regs["TRL"], regs["TRH"] = v & 0xFF, v >> 8 & 0xFF, v >> 16 & 0xFF
            elif reg.startswith("mem:"):
                self.slots.append(Slot(param, at, len(arg.before), arg.kind == 1, layout))
                filled.append((self.slots[-1], arg))
                at += len(arg.before)
            else:
                raise Unsupported("argument %s maps to unknown register %s" % (param, reg))
        self.result_slot = None
        if m.result and m.result.startswith("over:"):
            if any(s.pointer for s in self.slots):
                raise Unsupported("an over: result may only overlap value arguments")
            self.result_slot = Slot("result", ctx.block, len(rec.result), False, m.result_layout)
            at = max(at, ctx.block + len(rec.result))
        elif m.result and m.result.startswith("mem:"):
            self.result_slot = Slot("result", at, len(rec.result), False, m.result_layout)
            at += len(rec.result)
        if at > ctx.block + ctx.block_size:
            raise Unsupported("mem: arguments take %d bytes, ARG_BLOCK has %d" % (at - ctx.block, ctx.block_size))
        self.regs = regs
        lines = ["fill %02X" % FILL, "ram " + rec.ram_before.hex(), "fb " + rec.fb_before.hex()]
        lines += ["poke wram %03X %s" % (s.addr, arg.before.hex()) for s, arg in filled if s.size]
        if getattr(ctx, "rt_save", None) is not None:
            lines.append("poke wram %03X %s" % (RT_SAVE_ADDR, ctx.rt_save.hex()))
        lines += ["reg %s %02X" % (r, v) for r, v in sorted(regs.items())]
        for other in ctx.maps.values():
            if other.routine.lower() in ctx.labels:
                lines.append("poison %s %s" % (other.routine, other.poison))
        lines += ["ret %X" % c.ret for c in rec.hw]
        if HANDLER.lower() in ctx.labels:
            lines.append("handler " + HANDLER)
            lines += ["idle %d" % sum(1 for d in rec.hw[i + 1:next((j for j in range(i + 1, len(rec.hw)) if rec.hw[j].depth <= c.depth), len(rec.hw))]
                                      if d.name == "vmu_irq_pending" and d.depth == c.depth + 1)
                      for i, c in enumerate(rec.hw) if c.name == "vmu_idle"]
        lines += ["load " + rec.ram_after[c.args[1]:c.args[1] + SAVE_BLOCK].hex() for c in rec.hw
                  if c.name == "vmu_save_read" and c.ret == 0]
        self.text = "\n".join(lines) + "\n"
        self.want = {"bank1": [FILL] * BANK1_BYTES, "wram": [FILL] * WRAM_BYTES, "xram": [FILL] * XRAM_BYTES}
        for fl in ctx.fields.values():
            for k in range(fl.size):
                self.want[fl.space][fl.addr + k] = rec.ram_after[fl.offset + k]
        self.want["xram"][:FB_BYTES] = list(rec.fb_after)
        if getattr(ctx, "rt_save", None) is not None:
            self.want["wram"][RT_SAVE_ADDR:RT_SAVE_ADDR + len(ctx.rt_save)] = list(ctx.rt_save)
        self.free = set()
        for s, arg in filled:
            used = {at + k for _, at, size in s.layout[1] for k in range(size)} if s.layout else set(range(s.size))
            for k in range(s.size):
                if s.pointer and k in used:
                    self.want["wram"][s.addr + k] = arg.after[k]
                else:
                    self.free.add(("wram", s.addr + k))
        if self.result_slot:
            self.free |= {("wram", self.result_slot.addr + k) for k in range(self.result_slot.size)}
        self.free |= {("bank1", i) for i, r in enumerate(INDIRECT) if r in m.destroys}


"""----------------------
 | first_difference
 | Description: The first way a --call output differs from its record, in this order: the status, the result (field by field for a struct), the hardware calls (each rt_* entry mapped to its vmu_* call and arguments), every byte of bank 1, work RAM and XRAM against what the record says the call leaves, then the registers (register_difference).
 | Author: suinevere
 | Dependencies: Context, Setup, RT_ARGS, RESULT_REGS
 | Globals: N/A
 | Params: ctx -- the Context; m -- the Mapping; rec -- the Record; setup -- its Setup; out -- read_out's dict
 | Returns: the difference as "field F (...)", "pixel x,y (...)", "result (...)", "hw call J (...)", "register R (...)" or "status S", or None when the call matches
 ----------------------"""
def first_difference(ctx: Context, m: Mapping, rec, setup: Setup, out: dict):
    if out.get("status") != "returned":
        return "status %s" % out.get("status", "missing")
    diff = result_difference(m, rec, setup, out)
    if diff:
        return diff
    got = []
    for vmu, rt, regs in out["hw"]:
        got.append((vmu, RT_ARGS[vmu](regs, ctx.handler_id) if vmu in RT_ARGS else None))
    for j in range(max(len(got), len(rec.hw))):
        if j < len(rec.hw) and rec.hw[j].name not in RT_ARGS:
            return "hw call %d (%s has no rt_* counterpart)" % (j, rec.hw[j].name)
        want = (rec.hw[j].name, list(rec.hw[j].args)) if j < len(rec.hw) else None
        have = got[j] if j < len(got) else None
        if want != have:
            show = lambda c: "%s(%s)" % (c[0], ", ".join("0x%X" % a for a in c[1])) if c else "nothing"
            return "hw call %d (want %s, got %s)" % (j, show(want), show(have))
    for space in ("bank1", "wram", "xram"):
        for addr, (w, g) in enumerate(zip(setup.want[space], out[space])):
            if w == g or (space, addr) in setup.free:
                continue
            if space == "xram" and addr < FB_BYTES:
                bit = next(b for b in range(8) if (w ^ g) & (0x80 >> b))
                return "pixel %d,%d (want %d, got %d)" % (addr % 6 * 8 + bit, addr // 6, w >> (7 - bit) & 1, g >> (7 - bit) & 1)
            slot = [s for s in setup.slots if space == "wram" and s.addr <= addr < s.addr + s.size]
            name = "arg " + slot[0].name if slot else ctx.name_of(space, addr)
            return "field %s (%s 0x%03X: want 0x%02X, got 0x%02X)" % (name, space, addr, w, g)
    return register_difference(m, out)


"""----------------------
 | PSW_BANKS / PSW_FLAGS / REG_GROUPS
 | Description: PSW's RAM and indirect bank bits, which every routine must leave as at entry (RAM bank 1), and its flags CY, AC and OV, which DESTROYS PSW covers (P follows ACC and is never compared); and the output registers each DESTROYS name stands for.
 | Author: suinevere
 | Dependencies: pocketstation/harness/vmu_harness.c (OUT_REGS)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
PSW_BANKS = 0x1A
PSW_FLAGS = 0xC4
REG_GROUPS = {"ACC": ["ACC"], "B": ["B"], "C": ["C"], "TRL": ["TRL"], "TRH": ["TRH"], "VRMAD": ["VRMAD1", "VRMAD2"], "FPR": ["FPR"]}


"""----------------------
 | register_difference
 | Description: How a --call's registers on return break its declaration: a register outside DESTROYS and RESULT that differs from its entry value, PSW's bank bits changed, or its flags changed without PSW in DESTROYS.
 | Author: suinevere
 | Dependencies: RESULT_REGS, REG_GROUPS, PSW_BANKS, PSW_FLAGS
 | Globals: N/A
 | Params: m -- the Mapping; out -- read_out's dict
 | Returns: "register R (...)" or None
 ----------------------"""
def register_difference(m: Mapping, out: dict):
    entry, ret = out["entry"], out["reg"]
    keep = set(RESULT_REGS.get(m.result, []))
    for name, regs in REG_GROUPS.items():
        if name in m.destroys or name in keep:
            continue
        for r in regs:
            if ret[r] != entry[r]:
                return "register %s (not in DESTROYS: entry 0x%02X, return 0x%02X)" % (r, entry[r], ret[r])
    if (ret["PSW"] ^ entry["PSW"]) & PSW_BANKS:
        return "register PSW bank bits (entry 0x%02X, return 0x%02X)" % (entry["PSW"], ret["PSW"])
    if "PSW" not in m.destroys and (ret["PSW"] ^ entry["PSW"]) & PSW_FLAGS:
        return "register PSW flags (not in DESTROYS: entry 0x%02X, return 0x%02X)" % (entry["PSW"], ret["PSW"])
    return None


"""----------------------
 | result_difference
 | Description: How a --call result differs from the record's: from the RESULT registers, or from its ARG_BLOCK slot, a struct compared field by field.
 | Author: suinevere
 | Dependencies: RESULT_REGS
 | Globals: N/A
 | Params: m -- the Mapping; rec -- the Record; setup -- its Setup; out -- read_out's dict
 | Returns: "result ..." or None
 ----------------------"""
def result_difference(m: Mapping, rec, setup: Setup, out: dict):
    want = bytes(rec.result)
    if m.result is None:
        return "result (recorded %d bytes, the manifest maps none)" % len(want) if want else None
    if m.result in RESULT_REGS:
        got = bytes(out["reg"][r] for r in RESULT_REGS[m.result])
        if got != want[:len(got)].ljust(len(got), b"\0") or any(want[len(got):]):
            return "result (want 0x%s, got 0x%s)" % (want[::-1].hex(), got[::-1].hex())
        return None
    s = setup.result_slot
    got = out["wram"][s.addr:s.addr + s.size]
    fields = s.layout[1] if s.layout else [("", 0, s.size)]
    for name, at, size in fields:
        if got[at:at + size] != want[at:at + size]:
            return "result%s (want 0x%s, got 0x%s)" % (" field " + name if name else "", want[at:at + size][::-1].hex(),
                                                        got[at:at + size][::-1].hex())
    return None


"""----------------------
 | run_harness
 | Description: Runs vmu_harness --call once per state in one shell script, directly when the harness can run here, else in the lom-pocket container with the repository at /work.
 | Author: suinevere
 | Dependencies: subprocess, docker (lom-pocket)
 | Globals: REPO
 | Params: ctx -- the Context; routine -- the label; work -- repo-relative directory of K.state files; count -- how many; docker -- True, False or None to decide
 | Returns: N/A
 ----------------------"""
def run_harness(ctx: Context, routine: str, work: str, count: int, docker=None):
    p = ctx.paths
    lines = ["%s --flash %s --map %s --hw %s --call %s --state %s/%d.state --out %s/%d.out > %s/%d.log 2>&1"
             % (p["harness"], p["image"], p["map"], p["hw"], routine, work, k, work, k, work, k) for k in range(count)]
    script = work + "/run.sh"
    with open(os.path.join(REPO, script), "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    if docker is None:
        docker = not (os.name == "posix" and os.access(os.path.join(REPO, p["harness"]), os.X_OK))
    cmd = ["sh", script]
    if docker:
        cmd = ["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket"] + cmd
    r = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True, env=dict(os.environ, MSYS_NO_PATHCONV="1"))
    if r.returncode != 0:
        raise RuntimeError("harness run failed: " + r.stdout[-2000:] + r.stderr[-2000:])


"""----------------------
 | check
 | Description: Runs every record of one function through vmu_harness --call against its routine and compares each outcome with the record; also notes each path key with no passing record and the cycles of each passing call, overall and by path key.
 | Author: suinevere
 | Dependencies: Context, Setup, Unsupported, read_records, run_harness, read_out, first_difference
 | Globals: REPO
 | Params: function -- the C name; ctx -- the Context; work -- repo-relative work directory, or None for pocketstation/build/call/FUNCTION; docker -- see run_harness; script -- keep only records from this script, or None for every script; frame_min, frame_max -- keep only records whose frame is in this inclusive range, or None for no bound; depth -- keep only records at exactly this call depth, or None for no filter (the shallowest depth of a recursive function's records is its top-level calls, the rest nested inside another call already measured)
 | Returns: dict: records (count), passed, first ((index, difference, Record) of the first failing record, or None), untested (key bit lists, in first-seen order), cycles (per passing call), cycles_by_key (sorted key tuple -> list of cycles)
 ----------------------"""
def check(function: str, ctx: Context, work: str = None, docker=None, script: str = None, frame_min: int = None,
          frame_max: int = None, depth: int = None) -> dict:
    m = ctx.maps.get(function)
    if m is None or m.routine.startswith("inlined:"):
        raise ValueError("%s is not a routine in the manifest" % function)
    if m.routine.lower() not in ctx.labels:
        raise ValueError("%s's routine %s is not in %s" % (function, m.routine, ctx.paths["map"]))
    path = os.path.join(REPO, ctx.paths["rec"], function + ".rec")
    if not os.path.exists(path):
        raise ValueError("no records for %s in %s" % (function, ctx.paths["rec"]))
    recs = read_records(path)
    for rec in recs:
        if ctx.fids.get(rec.name) != rec.fid:
            raise ValueError("%s's record id %d is not the manifest's %s" % (rec.name, rec.fid, ctx.fids.get(rec.name)))
    if script is not None:
        recs = [rec for rec in recs if rec.script == script]
    if frame_min is not None:
        recs = [rec for rec in recs if rec.frame >= frame_min]
    if frame_max is not None:
        recs = [rec for rec in recs if rec.frame <= frame_max]
    if depth is not None:
        recs = [rec for rec in recs if rec.depth == depth]
    work = work or "pocketstation/build/call/" + function
    os.makedirs(os.path.join(REPO, work), exist_ok=True)
    for name in os.listdir(os.path.join(REPO, work)):
        if re.fullmatch(r"\d+\.(state|out|log)|run\.sh", name):
            os.remove(os.path.join(REPO, work, name))
    setups = []
    for k, rec in enumerate(recs):
        try:
            setups.append(Setup(ctx, m, rec))
        except Unsupported as e:
            setups.append(str(e))
            continue
        with open(os.path.join(REPO, work, "%d.state" % k), "w", newline="\n") as f:
            f.write(setups[-1].text)
    if recs:
        run_harness(ctx, m.routine, work, len(recs), docker)
    result = {"records": len(recs), "passed": 0, "first": None, "untested": [], "cycles": [], "cycles_by_key": {}}
    keys, passed_keys = {}, set()
    for k, (rec, setup) in enumerate(zip(recs, setups)):
        key = tuple(sorted(rec.key_bits))
        keys.setdefault(key, k)
        if isinstance(setup, str):
            diff = setup
        else:
            out_path = os.path.join(REPO, work, "%d.out" % k)
            if not os.path.exists(out_path):
                log = open(os.path.join(REPO, work, "%d.log" % k)).read().strip()
                diff = "status no output (%s)" % log.splitlines()[-1] if log else "status no output"
            else:
                out = read_out(out_path)
                diff = first_difference(ctx, m, rec, setup, out)
                if diff is None:
                    result["cycles"].append(out["cycles"])
                    result["cycles_by_key"].setdefault(key, []).append(out["cycles"])
        if diff is None:
            result["passed"] += 1
            passed_keys.add(key)
        elif result["first"] is None:
            result["first"] = (k, diff, rec)
    result["untested"] = [list(key) for key in sorted(keys, key=keys.get) if key not in passed_keys]
    return result


"""----------------------
 | main
 | Description: CLI. call_compare.py FUNCTION [--rec DIR] [--image IMG] [--map MAP] [--hw HW.json] [--manifest M] [--work DIR] [--docker|--local] [--script NAME] [--frame-min N] [--frame-max N] [--depth N] [--cycles] checks every record of FUNCTION (restricted to --script, the --frame-min/--frame-max range and --depth when given) and prints "ok N", or the first difference as "call K: field F" / "pixel x,y" / "result" / "hw call J" with "failed F of N"; then "untested key BITS" for each path key with no passing record, and with --cycles the passing calls' cycles overall and, when more than one path key passed, one "cycles key BITS ..." line per key. call_compare.py list FILE.rec prints one line per record.
 | Author: suinevere
 | Dependencies: read_records, Context, check
 | Globals: PATHS
 | Params: argv -- command-line arguments
 | Returns: 0 when every record passes, 1 when one differs, 2 on a usage error
 ----------------------"""
def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) == 2 and argv[0] == "list":
        for rec in read_records(argv[1]):
            print("%s %s frame %d depth %d%s key %s args %s result %s hw %d" % (
                rec.name, rec.script, rec.frame, rec.depth, " synthetic" if rec.synthetic else "",
                ",".join(str(b) for b in sorted(rec.key_bits)) or "-",
                ",".join(a.before.hex() for a in rec.args) or "-", rec.result.hex() or "-", len(rec.hw)))
        return 0
    ap = argparse.ArgumentParser(prog="call_compare.py")
    ap.add_argument("function")
    for name in ("rec", "image", "map", "hw", "manifest", "work"):
        ap.add_argument("--" + name)
    ap.add_argument("--docker", dest="docker", action="store_true", default=None)
    ap.add_argument("--local", dest="docker", action="store_false")
    ap.add_argument("--cycles", action="store_true")
    ap.add_argument("--script")
    ap.add_argument("--frame-min", type=int)
    ap.add_argument("--frame-max", type=int)
    ap.add_argument("--depth", type=int)
    try:
        a = ap.parse_args(argv)
    except SystemExit:
        return 2
    try:
        ctx = Context(rec=a.rec, image=a.image, map=a.map, hw=a.hw, manifest=a.manifest)
        r = check(a.function, ctx, a.work, a.docker, script=a.script, frame_min=a.frame_min, frame_max=a.frame_max,
                  depth=a.depth)
    except (ValueError, RuntimeError, OSError) as e:
        print("call_compare: %s" % e, file=sys.stderr)
        return 2
    if r["first"] is None:
        print("ok %d" % r["records"])
    else:
        k, diff, rec = r["first"]
        print("call %d: %s [%s frame %d]" % (k, diff, rec.script, rec.frame))
        print("failed %d of %d" % (r["records"] - r["passed"], r["records"]))
    for key in r["untested"]:
        print("untested key %s" % (",".join(str(b) for b in key) or "-"))
    if a.cycles and r["cycles"]:
        c = r["cycles"]
        print("cycles mean %d min %d max %d" % ((sum(c) + len(c) // 2) // len(c), min(c), max(c)))
        if len(r["cycles_by_key"]) > 1:
            for key in sorted(r["cycles_by_key"]):
                kc = r["cycles_by_key"][key]
                print("cycles key %s mean %d min %d max %d n %d" % (
                    ",".join(str(b) for b in key) or "-", (sum(kc) + len(kc) // 2) // len(kc), min(kc), max(kc), len(kc)))
    return 0 if r["first"] is None else 1


if __name__ == "__main__":
    sys.exit(main())
