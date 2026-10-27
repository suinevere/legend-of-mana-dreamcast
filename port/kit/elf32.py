#----------------------
# | elf32.py
# | Description: Reads symbol values and sizes and the loadable segments of a 32-bit little-endian ELF such as dreamcast/lom.elf.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import struct


#----------------------
# | _read
# | Description: The whole file's bytes, its section headers as (name offset, type, offset, size, link, entsize) and its ELF header fields.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: path -- ELF file
# | Returns: (bytes, list of tuples, header tuple)
#----------------------
def _read(path):
    with open(path, "rb") as handle:
        data = handle.read()
    if len(data) < 52 or data[0:4] != b"\x7fELF" or data[4] != 1 or data[5] != 1:
        raise ValueError("%s is not a 32-bit little-endian ELF" % path)
    header = struct.unpack_from("<HHIIIIIHHHHHH", data, 16)
    shoff, shentsize, shnum = header[5], header[10], header[11]
    sections = []
    for i in range(shnum):
        name, kind, _, _, offset, size, link, _, _, entsize = struct.unpack_from("<IIIIIIIIII", data, shoff + i * shentsize)
        sections.append((name, kind, offset, size, link, entsize))
    return data, sections, header


#----------------------
# | symbols
# | Description: Maps each named symbol, with sh-elf's leading underscore removed, to (value, size).
# | Author: suinevere
# | Dependencies: _read
# | Globals: N/A
# | Params: path -- ELF file
# | Returns: dict name -> (value, size)
#----------------------
def symbols(path):
    data, sections, _ = _read(path)
    out = {}
    for _, kind, offset, size, link, entsize in sections:
        if kind != 2:
            continue
        strtab = sections[link][2]
        for pos in range(offset, offset + size, entsize):
            name_off, value, sym_size = struct.unpack_from("<III", data, pos)
            if not name_off:
                continue
            end = data.index(b"\x00", strtab + name_off)
            name = data[strtab + name_off:end].decode("ascii", "replace")
            out[name[1:] if name.startswith("_") else name] = (value, sym_size)
    return out


#----------------------
# | load_segments
# | Description: (vaddr, memsz, file bytes) for every PT_LOAD segment that has file bytes, in file order.
# | Author: suinevere
# | Dependencies: _read
# | Globals: N/A
# | Params: path -- ELF file
# | Returns: list of tuples
#----------------------
def load_segments(path):
    data, _, header = _read(path)
    phoff, phentsize, phnum = header[4], header[8], header[9]
    out = []
    for i in range(phnum):
        kind, offset, vaddr, _, filesz, memsz, _, _ = struct.unpack_from("<IIIIIIII", data, phoff + i * phentsize)
        if kind == 1 and filesz:
            out.append((vaddr, memsz, data[offset:offset + filesz]))
    return out
