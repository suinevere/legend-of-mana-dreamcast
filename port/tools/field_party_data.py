#----------------------
# | field_party_data.py
# | Description: Converts FIELD's party resources so the big-endian Saturn reads them in place: the party script packages (2615, 2715) and the portrait set (1509).
# | Author: suinevere
# | Dependencies: overlay_rules.py
# | Globals: PARTY_SCRIPT_SECTIONS, ACTION_SLOT_HALFWORDS, PORTRAIT_CLUT_COLORS, PORTRAIT_PIXEL_BYTES, PORTRAIT_SIZE
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

from overlay_rules import ps1_to_saturn_color, swap_nibbles, swap_words

#----------------------
# | PARTY_SCRIPT_SECTIONS
# | Description: Leading s32 section offsets of a party script package: action table, script page start, script page end (field_load_party_script_page).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PARTY_SCRIPT_SECTIONS = 3

#----------------------
# | ACTION_SLOT_HALFWORDS
# | Description: u16 fields of one FieldActionSlot: command, flags, animation, parameter.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ACTION_SLOT_HALFWORDS = 4

#----------------------
# | PORTRAIT_CLUT_COLORS / PORTRAIT_PIXEL_BYTES / PORTRAIT_SIZE
# | Description: One portrait (FIELD_PORTRAIT_SIZE): a 16-colour palette strip, then a 12x48-word (48x48 4bpp) image block, as field_upload_player_icons uploads them.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PORTRAIT_CLUT_COLORS = 16
PORTRAIT_PIXEL_BYTES = 12 * 48 * 2
PORTRAIT_SIZE = PORTRAIT_CLUT_COLORS * 2 + PORTRAIT_PIXEL_BYTES


#----------------------
# | PartyDataFormatError
# | Description: Raised when a party resource does not have the expected layout.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class PartyDataFormatError(ValueError):
    pass


#----------------------
# | convert_party_script
# | Description: Swaps a party script package's three section offsets and its action table (s32 count, then count FieldActionSlot of four u16); the script page stays raw like scene scripts.
# | Author: suinevere
# | Dependencies: swap_words
# | Globals: PARTY_SCRIPT_SECTIONS, ACTION_SLOT_HALFWORDS
# | Params: data -- raw resource bytes
# | Returns: converted bytes
#----------------------
def convert_party_script(data):
    buf = bytearray(data)
    header = PARTY_SCRIPT_SECTIONS * 4
    if len(buf) < header:
        raise PartyDataFormatError("party script is shorter than its header")
    table, page, end = struct.unpack_from("<3i", buf, 0)
    if not header <= table <= page <= end == len(buf):
        raise PartyDataFormatError("party script sections 0x%X 0x%X 0x%X do not fit 0x%X bytes" % (table, page, end, len(buf)))
    if table + 4 > page:
        raise PartyDataFormatError("party script action table has no count")
    (count,) = struct.unpack_from("<i", buf, table)
    if count < 0 or table + 4 + count * ACTION_SLOT_HALFWORDS * 2 > page:
        raise PartyDataFormatError("party script action count %d overruns the script page" % count)
    swap_words(buf, 0, PARTY_SCRIPT_SECTIONS, 4)
    swap_words(buf, table, 1, 4)
    swap_words(buf, table + 4, count * ACTION_SLOT_HALFWORDS, 2)
    return bytes(buf)


#----------------------
# | convert_portraits
# | Description: Converts the decompressed portrait set: swaps the u32 count and offsets, runs each portrait's palette through ps1_to_saturn_color and nibble-swaps its 4bpp pixels.
# | Author: suinevere
# | Dependencies: ps1_to_saturn_color, swap_nibbles, swap_words
# | Globals: PORTRAIT_CLUT_COLORS, PORTRAIT_PIXEL_BYTES, PORTRAIT_SIZE
# | Params: buf -- decompressed resource 1509 bytearray, converted in place
# | Returns: N/A
#----------------------
def convert_portraits(buf):
    if len(buf) < 4:
        raise PartyDataFormatError("portrait set is shorter than its count")
    (count,) = struct.unpack_from("<I", buf, 0)
    if 4 + count * 4 > len(buf):
        raise PartyDataFormatError("portrait offsets overrun the set")
    offsets = struct.unpack_from("<%dI" % count, buf, 4)
    for offset in offsets:
        if offset < 4 + count * 4 or offset + PORTRAIT_SIZE > len(buf):
            raise PartyDataFormatError("portrait at 0x%X lies outside the set" % offset)
    if len(set(offsets)) != count:
        raise PartyDataFormatError("portrait offsets repeat")
    swap_words(buf, 0, count + 1, 4)
    for offset in offsets:
        for i in range(PORTRAIT_CLUT_COLORS):
            pos = offset + i * 2
            (value,) = struct.unpack_from("<H", buf, pos)
            struct.pack_into(">H", buf, pos, ps1_to_saturn_color(value))
        swap_nibbles(buf, offset + PORTRAIT_CLUT_COLORS * 2, PORTRAIT_PIXEL_BYTES)
