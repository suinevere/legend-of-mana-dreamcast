#!/usr/bin/env python3
#----------------------
# | scenario.py
# | Description: Shared scenarios for the VMU interface: compiles a .scn file to the op bytes both scenario_host and rt_test run, writes its input events for the harness, writes the test table and pet payload both sides hold, and decodes rt_test's RT_LOG bytes into event-log lines.
# | Author: suinevere
# | Dependencies: sys
# | Globals: N/A
# | Params: compile IN.scn OUT.bin | inputs IN.scn OUT | table OUT | pet OUT | decode LOG
# | Returns: process exit code
#----------------------
import sys

#----------------------
# | OPS
# | Description: Every scenario op by name: its opcode byte and the byte widths of its arguments, each stored big-endian after the opcode. Mask and unmask clear and set IE bit 7 on the VMU and do nothing on the host.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OPS = {
    "end": (0x00, ()),
    "tick": (0x01, (1,)),
    "sound": (0x02, (4, 2)),
    "sound_stop": (0x03, ()),
    "enable": (0x04, (2,)),
    "disable": (0x05, (2,)),
    "ack": (0x06, (2,)),
    "idle": (0x07, (2,)),
    "press": (0x08, (1, 2)),
    "release": (0x09, (1, 2)),
    "dock": (0x0A, (1, 2)),
    "save_write": (0x0B, (2, 2)),
    "save_read": (0x0C, (2, 2)),
    "rom": (0x0D, (2,)),
    "draw": (0x0E, (1, 1, 1)),
    "present": (0x0F, ()),
    "buzz": (0x10, (1,)),
    "lcd": (0x11, (1,)),
    "power": (0x12, (1,)),
    "log_ram": (0x13, (2,)),
    "ram_write": (0x14, (2, 1)),
    "log_buttons": (0x15, ()),
    "log_sys": (0x16, ()),
    "log_levels": (0x17, ()),
    "log_run": (0x18, (2, 1)),
    "fill": (0x19, (2, 1, 1)),
    "mask": (0x1A, ()),
    "unmask": (0x1B, ()),
}

#----------------------
# | INPUT_OPS
# | Description: The ops the harness drives from outside the GAME, which inputs() writes out and rt_test steps over.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
INPUT_OPS = ("press", "release", "dock")

#----------------------
# | RAM_OPS / RAM_START / RAM_END / SAVE_OPS / SAVE_START / SAVE_END
# | Description: The ops whose first argument is a vmu_ram offset (with a byte count for log_run and fill), and the offsets they may use: past the runtime's rt_reserved block and below the end of work RAM; and the save ops, whose 128 RAM bytes must lie inside vmu_ram; both sides refuse, with status 1, any outside work RAM below rt_save (SAVE_START-SAVE_END).
# | Author: suinevere
# | Dependencies: pocketstation/src/game/ram_map.txt
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RAM_OPS = ("log_ram", "ram_write", "log_run", "fill")
RAM_START = 0x020
RAM_END = 0x300
SAVE_OPS = ("save_write", "save_read")
SAVE_START = 0x100
SAVE_END = 0x2E8

#----------------------
# | TABLE_BYTES / PET_BYTES / PET_ICONS
# | Description: The test table's size, which rom offsets stay below, and the test pet payload's size and the icon frames its PS1 header asks for.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TABLE_BYTES = 512
PET_BYTES = 8192
PET_ICONS = 2

#----------------------
# | PORT3
# | Description: Scenario button bits (VMU_BTN_* plus Mode 0x20 and Sleep 0x40) and the port 3 bits the harness takes for them: A, right, left, down, up, Mode, Sleep.
# | Author: suinevere
# | Dependencies: pocketstation/harness/vmu_harness.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PORT3 = ((0x01, 0x10), (0x02, 0x08), (0x04, 0x04), (0x08, 0x02), (0x10, 0x01), (0x20, 0x40), (0x40, 0x80))

#----------------------
# | EVENTS
# | Description: Event-log tags as rt_test writes them to RT_LOG (rt.inc's RT_EV_*): each tag's line name and the byte widths of its big-endian fields.
# | Author: suinevere
# | Dependencies: pocketstation/src/vmu_rt/rt.inc
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EVENTS = {
    0x68: ("h", (2,)),
    0x69: ("i", (2,)),
    0x72: ("r", (1,)),
    0x77: ("w", (2, 1)),
    0x78: ("x", (1,)),
    0x6D: ("m", (2, 1)),
    0x45: ("error", (1,)),
    0x62: ("b", (1,)),
    0x73: ("s", (1,)),
    0x6C: ("l", (2,)),
    0x76: ("v", (2, 1)),
    0x70: ("p", ()),
}


#----------------------
# | parse
# | Description: Reads a scenario's text, one op a line with '#' comments, checking each op's name, argument count, argument widths and RAM offsets. Lines between repeat N and end_repeat, which nest, are expanded N times; input ops may not repeat, as their idle indices would.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: OPS, RAM_OPS, RAM_START, RAM_END, INPUT_OPS
# | Params: text -- the .scn contents
# | Returns: list of (name, values); raises ValueError on a bad line
#----------------------
def parse(text):
    ops = []
    blocks = []
    for n, line in enumerate(text.splitlines(), 1):
        words = line.split("#", 1)[0].split()
        if not words:
            continue
        name, args = words[0], words[1:]
        if name == "repeat":
            if len(args) != 1 or int(args[0], 0) < 1:
                raise ValueError("line %d: repeat takes a count of at least 1" % n)
            blocks.append((int(args[0], 0), ops))
            ops = []
            continue
        if name == "end_repeat":
            if args or not blocks:
                raise ValueError("line %d: end_repeat without a repeat" % n)
            count, outer = blocks.pop()
            ops = outer + ops * count
            continue
        if blocks and name in INPUT_OPS:
            raise ValueError("line %d: %s cannot repeat" % (n, name))
        if name not in OPS:
            raise ValueError("line %d: unknown op %r" % (n, name))
        widths = OPS[name][1]
        if len(args) != len(widths):
            raise ValueError("line %d: %s takes %d arguments" % (n, name, len(widths)))
        values = [int(a, 0) for a in args]
        for v, w in zip(values, widths):
            if not 0 <= v < 1 << (8 * w):
                raise ValueError("line %d: %s argument %d out of range" % (n, name, v))
        count = values[1] if name in ("log_run", "fill") else 1
        if name in RAM_OPS and not (RAM_START <= values[0] and values[0] + count <= RAM_END):
            raise ValueError("line %d: RAM offset 0x%X is outside the game's bank 1 and WRAM bytes" % (n, values[0]))
        if name in SAVE_OPS and values[1] + 128 > 0x400:
            raise ValueError("line %d: save RAM offset 0x%X runs past vmu_ram" % (n, values[1]))
        if name == "rom" and values[0] >= TABLE_BYTES:
            raise ValueError("line %d: table offset 0x%X is past the test table" % (n, values[0]))
        if name == "draw" and (values[0] >= 32 or values[1] >= 6):
            raise ValueError("line %d: draw row %d byte %d is off the screen" % (n, values[0], values[1]))
        ops.append((name, values))
    if blocks:
        raise ValueError("repeat without end_repeat")
    return ops


#----------------------
# | compile_ops
# | Description: Encodes parsed ops as opcode bytes with big-endian arguments, ending with the end op.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: OPS
# | Params: ops -- parse output
# | Returns: bytes
#----------------------
def compile_ops(ops):
    out = bytearray()
    for name, values in ops:
        code, widths = OPS[name]
        out.append(code)
        for v, w in zip(values, widths):
            out += v.to_bytes(w, "big")
    out.append(OPS["end"][0])
    return bytes(out)


#----------------------
# | port3
# | Description: Turns a scenario button mask into the harness's port 3 bits.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: PORT3
# | Params: mask -- scenario button bits
# | Returns: port 3 bits
#----------------------
def port3(mask):
    return sum(bit for vmu, bit in PORT3 if mask & vmu)


#----------------------
# | inputs
# | Description: Writes the input ops in the harness's --inputs format: press or release with port 3 bits, or dock, then the idle index.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: INPUT_OPS
# | Params: ops -- parse output
# | Returns: the file's text
#----------------------
def inputs(ops):
    lines = []
    for name, values in ops:
        if name not in INPUT_OPS:
            continue
        if name == "dock":
            lines.append("dock %d %d" % (values[0], values[1]))
        else:
            lines.append("%s 0x%02X %d" % (name, port3(values[0]), values[1]))
    return "".join(line + "\n" for line in lines)


#----------------------
# | test_table
# | Description: The test GAME's 512 table bytes, made up here so no disc data is needed: a pattern whose two halves differ.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: TABLE_BYTES
# | Params: N/A
# | Returns: bytes
#----------------------
def test_table():
    return bytes((i * 73 + (i >> 8) * 151 + 5) & 0xFF for i in range(TABLE_BYTES))


#----------------------
# | test_pet
# | Description: The test pet payload, made up here: a PS1 "SC" header asking for PET_ICONS icon frames, so its package carries that many and its sector 0 is the package's page 1 + 4 * PET_ICONS, then a pattern.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: PET_BYTES, PET_ICONS
# | Params: N/A
# | Returns: bytes
#----------------------
def test_pet():
    data = bytearray((i * 29 + (i >> 7) * 11 + 3) & 0xFF for i in range(PET_BYTES))
    data[0:3] = b"SC" + bytes([0x10 | PET_ICONS])
    return bytes(data)


#----------------------
# | decode
# | Description: Turns RT_LOG bytes into event-log lines in scenario_host's text: the line name, then each field in fixed-width hex, a RAM offset in 3 digits.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: EVENTS
# | Params: log -- the bytes
# | Returns: list of lines; raises ValueError on an unknown tag or a cut-short event
#----------------------
def decode(log):
    lines = []
    i = 0
    while i < len(log):
        tag = log[i]
        if tag not in EVENTS:
            raise ValueError("unknown event tag 0x%02X at %d" % (tag, i))
        name, widths = EVENTS[tag]
        i += 1
        if i + sum(widths) > len(log):
            raise ValueError("event %s cut short at %d" % (name, i))
        fields = []
        for w in widths:
            v = int.from_bytes(log[i:i + w], "big")
            fields.append("%0*X" % (3 if name == "m" and not fields else 2 * w, v))
            i += w
        lines.append(" ".join([name] + fields))
    return lines


#----------------------
# | main
# | Description: CLI: compile IN.scn OUT.bin, inputs IN.scn OUT, table OUT, pet OUT, or decode LOG to stdout.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: argv -- arguments, or None for sys.argv
# | Returns: 0, or 2 on a usage error
#----------------------
def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if len(argv) == 3 and argv[0] == "compile":
        with open(argv[1]) as f:
            data = compile_ops(parse(f.read()))
        with open(argv[2], "wb") as f:
            f.write(data)
        return 0
    if len(argv) == 3 and argv[0] == "inputs":
        with open(argv[1]) as f:
            text = inputs(parse(f.read()))
        with open(argv[2], "w", newline="\n") as f:
            f.write(text)
        return 0
    if len(argv) == 2 and argv[0] in ("table", "pet"):
        with open(argv[1], "wb") as f:
            f.write(test_table() if argv[0] == "table" else test_pet())
        return 0
    if len(argv) == 2 and argv[0] == "decode":
        with open(argv[1], "rb") as f:
            sys.stdout.write("".join(line + "\n" for line in decode(f.read())))
        return 0
    sys.stderr.write("usage: scenario.py compile IN.scn OUT.bin | inputs IN.scn OUT | table OUT | pet OUT | decode LOG\n")
    return 2


if __name__ == "__main__":
    sys.exit(main())
