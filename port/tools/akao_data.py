#----------------------
# | akao_data.py
# | Description: Extracts the AKAO driver's PS1 data block from the user's SLUS_010.13 into gen/akao_data.c, relocating its pointer tables to Dreamcast symbols, and declares a zeroed block for its BSS; gen_pending maps AKAO's PS1 addresses onto both.
# | Author: suinevere
# | Dependencies: libgte_tables.py (read_iso_file, exe_slice, EXE_NAME), disc_image.py, config/us/symbols/*.txt
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import glob
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = getattr(sys, "_MEIPASS", None) or os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)

import disc_image  # noqa: E402
import libgte_tables  # noqa: E402

#----------------------
# | DATA_START / DATA_END
# | Description: The PS1 .data/.sdata span holding AKAO's initialized symbols, g_akao_irq_timing through g_resident_song_section_offsets.
# | Author: suinevere
# | Dependencies: config/us/symbols
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DATA_START = 0x8003D160
DATA_END = 0x8003F000

#----------------------
# | BSS_START / BSS_END
# | Description: The PS1 .bss span holding AKAO's zeroed symbols, g_akao_seq_channels through g_akao_effect_voice_masks; nothing else lives there.
# | Author: suinevere
# | Dependencies: config/us/symbols
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BSS_START = 0x80049130
BSS_END = 0x8004FC70

#----------------------
# | SONG_GLOBALS
# | Description: The song loader's zeroed PS1 BSS globals that sit among FIELD and save globals, defined on their own with their PS1 sizes.
# | Author: suinevere
# | Dependencies: src/main/audio/game_audio.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SONG_GLOBALS = (
    "unsigned int g_current_song_handle;",
    "unsigned char g_music_sequence[0x400] __attribute__((aligned(4)));",
)

#----------------------
# | POINTER_TABLES
# | Description: Symbols in the data block whose words are pointers, with their word counts: three handler tables and the LFO waveform table.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
POINTER_TABLES = (
    ("g_akao_command_handlers", 256),
    ("g_akao_opcode_handlers", 96),
    ("g_akao_opcode_handlers_ext", 32),
    ("g_akao_lfo_waveforms", 16),
)

#----------------------
# | SYMBOL_PATTERN
# | Description: One "name = 0xADDR;" line of a symbol file.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SYMBOL_PATTERN = re.compile(r"^\s*(\w+)\s*=\s*0x([0-9A-Fa-f]+)\s*;")


#----------------------
# | load_symbols
# | Description: Every name and address in config/us/symbols, as name -> address and address -> names.
# | Author: suinevere
# | Dependencies: SYMBOL_PATTERN
# | Globals: N/A
# | Params: directory -- symbol folder
# | Returns: (dict name -> address, dict address -> sorted names)
#----------------------
def load_symbols(directory=os.path.join(REPO, "config", "us", "symbols")):
    by_name, by_address = {}, {}
    for path in sorted(glob.glob(os.path.join(directory, "*.txt"))):
        with open(path, "r", encoding="utf-8") as handle:
            for line in handle:
                match = SYMBOL_PATTERN.match(line)
                if match:
                    name, address = match.group(1), int(match.group(2), 16)
                    by_name.setdefault(name, address)
                    by_address.setdefault(address, set()).add(name)
    return by_name, {address: sorted(names) for address, names in by_address.items()}


#----------------------
# | pointer_word
# | Description: C initializer for one pointer word: 0 stays 0, an address inside the data block becomes lom_akao_data plus its offset, any other address the function named there.
# | Author: suinevere
# | Dependencies: DATA_START, DATA_END
# | Globals: N/A
# | Params: value -- PS1 word, by_address -- address -> names, functions -- set the referenced names are added to
# | Returns: initializer text
#----------------------
def pointer_word(value, by_address, functions):
    if value == 0:
        return "0x00000000"
    if DATA_START <= value < DATA_END:
        return "(unsigned int)lom_akao_data + 0x%X" % (value - DATA_START)
    names = [name for name in by_address.get(value, ()) if not name.startswith("D_")]
    if not names:
        raise ValueError("no symbol names the pointer 0x%08X" % value)
    functions.add(names[0])
    return "(unsigned int)%s" % names[0]


#----------------------
# | generate
# | Description: The text of gen/akao_data.c for a disc image.
# | Author: suinevere
# | Dependencies: libgte_tables.read_iso_file, libgte_tables.exe_slice, load_symbols, pointer_word, POINTER_TABLES
# | Globals: N/A
# | Params: image_path -- PS1 disc image
# | Returns: C source text
#----------------------
def generate(image_path):
    exe = libgte_tables.read_iso_file(disc_image.DiscImage(image_path), libgte_tables.EXE_NAME)
    data = libgte_tables.exe_slice(exe, DATA_START, DATA_END - DATA_START)
    by_name, by_address = load_symbols()
    pointer_words = set()
    for name, count in POINTER_TABLES:
        start = (by_name[name] - DATA_START) // 4
        pointer_words.update(range(start, start + count))
    functions = set()
    words = []
    for index in range(len(data) // 4):
        value = struct.unpack_from("<I", data, index * 4)[0]
        words.append(pointer_word(value, by_address, functions) if index in pointer_words else "0x%08X" % value)
    lines = [
        "/*----------------------",
        " | akao_data.c",
        " | Description: Generated by port/tools/akao_data.py from the user's SLUS_010.13: AKAO's PS1 data block (0x%08X-0x%08X) with its pointer tables relocated, its zeroed BSS block (0x%08X-0x%08X), and the song loader's two BSS globals."
        % (DATA_START, DATA_END, BSS_START, BSS_END),
        " | Author: suinevere",
        " | Dependencies: port/tools/akao_data.py, gen/dc_pending.ld (address aliases into both blocks)",
        " | Globals: lom_akao_data, lom_akao_bss, g_current_song_handle, g_music_sequence",
        " | Params: N/A",
        " | Returns: N/A",
        " ----------------------*/",
    ]
    lines += ["extern char %s[];" % name for name in sorted(functions)]
    lines.append("")
    lines.append("unsigned int lom_akao_data[0x%X] __attribute__((aligned(32))) = {" % len(words))
    lines += ["    %s," % word for word in words]
    lines.append("};")
    lines.append("")
    lines.append("unsigned int lom_akao_bss[0x%X] __attribute__((aligned(32)));" % ((BSS_END - BSS_START) // 4))
    lines.append("")
    lines += list(SONG_GLOBALS)
    return "\n".join(lines) + "\n"


#----------------------
# | main
# | Description: Command-line entry: writes gen/akao_data.c under --out.
# | Author: suinevere
# | Dependencies: generate
# | Globals: N/A
# | Params: argv -- argument list
# | Returns: process exit code
#----------------------
def main(argv=None):
    parser = argparse.ArgumentParser(description="Extract AKAO's data block from SLUS_010.13")
    parser.add_argument("--image", default=os.environ.get("LOM_PS1_IMAGE", os.path.join(REPO, "disc", "us", "LegendOfMana.bin")))
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    os.makedirs(os.path.join(args.out, "gen"), exist_ok=True)
    with open(os.path.join(args.out, "gen", "akao_data.c"), "w", encoding="utf-8", newline="\n") as handle:
        handle.write(generate(args.image))
    return 0


if __name__ == "__main__":
    sys.exit(main())
