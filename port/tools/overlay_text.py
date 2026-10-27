#----------------------
# | overlay_text.py
# | Description: Encodes English lines as US text bytes and rebuilds text tables with them in the empty entries, for the Dreamcast converter.
# | Author: suinevere
# | Dependencies: pocketstation_en.py, tools/overlays/text_table.py
# | Globals: CONTROLS, ARGUMENT_CONTROLS, CONTROL_CODES
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re
import sys

REPO = getattr(sys, "_MEIPASS", None) or os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if REPO not in sys.path:
    sys.path.insert(0, REPO)
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import pocketstation_en  # noqa: E402
from tools.overlays import text_table  # noqa: E402

#----------------------
# | CONTROLS / ARGUMENT_CONTROLS
# | Description: Scene text control names in code order and the codes that take an argument byte, as tools/scenes/strings.py has them; copied because that module needs PyYAML, which the Dreamcast image lacks.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CONTROLS = (
    'end', 'newline', 'wait_newline', 'wait_clear', 'clear', 'wait', 'finish',
    'choice', 'two_spaces', 'three_spaces', 'four_spaces', 'spaces',
    'short_delay', 'delay', 'macro', 'inline_text', 'color', 'default_color',
    'prefixed_glyph_run', 'indent',
)
ARGUMENT_CONTROLS = frozenset((11, 13, 14, 16))

#----------------------
# | CONTROL_CODES
# | Description: Scene text control name to its byte, in the order tools/scenes/strings.py decodes them.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: CONTROLS
# | Params: N/A
# | Returns: N/A
#----------------------
CONTROL_CODES = {name: code for code, name in enumerate(CONTROLS)}

#----------------------
# | encode_overlay
# | Description: An overlay message line as bytes: printable ASCII and a zero terminator.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: text -- line
# | Returns: bytes; raises ValueError for other characters
#----------------------
def encode_overlay(text):
    if any(not 0x20 <= ord(c) <= 0x7E or c in "{}" for c in text):
        raise ValueError("overlay line %r is not plain ASCII" % text)
    return text.encode("ascii") + b"\x00"


#----------------------
# | encode_scene
# | Description: A scene line as bytes: printable ASCII, with {name} or {name:arg} controls as their codes; the line supplies its own {end} or {finish}.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: CONTROL_CODES, ARGUMENT_CONTROLS
# | Params: text -- line
# | Returns: bytes; raises ValueError for an unknown control or character
#----------------------
def encode_scene(text):
    out = bytearray()
    for control, plain in re.findall(r"\{([^}]*)\}|([^{])", text):
        if plain:
            if not 0x20 <= ord(plain) <= 0x7E or plain == "}":
                raise ValueError("scene line %r has a character outside ASCII" % text)
            out += plain.encode("ascii")
            continue
        name, _, arg = control.partition(":")
        if name not in CONTROL_CODES or (CONTROL_CODES[name] in ARGUMENT_CONTROLS) != bool(arg):
            raise ValueError("scene line %r has an unknown control {%s}" % (text, control))
        out.append(CONTROL_CODES[name])
        if arg:
            out.append(int(arg, 0))
    return bytes(out)


#----------------------
# | load_lines
# | Description: The committed lines by group and index.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: pocketstation_en.LINES
# | Params: N/A
# | Returns: dict of group to dict of index to line
#----------------------
def load_lines():
    return {group: dict(entries) for group, entries in pocketstation_en.LINES.items()}


#----------------------
# | rebuild_table
# | Description: A copy of the text table at start with each given line written into its empty entry and each replacement over the entry holding its expected bytes; every other entry keeps its bytes.
# | Author: suinevere
# | Dependencies: text_table.parse
# | Globals: N/A
# | Params: data -- bytes holding the table, start -- table offset, lines -- index to encoded bytes (terminator included), codes -- two-byte codes,
# |         replacements -- index to (expected entry bytes, encoded bytes), or None
# | Returns: (new table bytes, size of the old table); raises ValueError if a line targets a non-empty entry or a replacement's entry differs
#----------------------
def rebuild_table(data, start, lines, codes, replacements=None):
    old = text_table.parse(data, start, codes)
    strings = []
    for entry in old.entries:
        if replacements and entry.index in replacements:
            expected, new = replacements[entry.index]
            if entry.data != expected:
                raise ValueError("text entry %d is %r, not %r" % (entry.index, entry.data, expected))
            strings.append(new)
        elif entry.index in lines:
            if entry.data:
                raise ValueError("text entry %d is not empty" % entry.index)
            strings.append(lines[entry.index])
        else:
            strings.append(entry.data + b"\x00")
    offsets = bytearray()
    body = bytearray()
    head = 2 * len(strings)
    for s in strings:
        offsets += (head + len(body)).to_bytes(2, "little")
        body += s
    if head + len(body) > 0xFFFF:
        raise ValueError("rebuilt text table is 0x%X bytes, past a u16 offset" % (head + len(body)))
    return bytes(offsets + body), old.size
