#----------------------
# | akao.py
# | Description: Parses Legend of Mana AKAO resources: headers, offset containers, instrument banks, effect tables and song headers.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import collections
import struct

#----------------------
# | MAGIC
# | Description: The four bytes that open every AKAO resource (include/main/audio/akao.h AkaoHeader).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MAGIC = b"AKAO"

#----------------------
# | HEADER_SIZE
# | Description: Bytes in an AkaoHeader.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
HEADER_SIZE = 0x10

#----------------------
# | BANK_HEADER_SIZE
# | Description: Bytes in an AkaoBankHeader; the articulation table follows it.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BANK_HEADER_SIZE = 0x40

#----------------------
# | ARTICULATION_SIZE
# | Description: Bytes in one AkaoArticulation entry.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ARTICULATION_SIZE = 0x10

#----------------------
# | SONG_CHANNELS_OFFSET
# | Description: Offset of AkaoSongDescriptor.channel_offsets from the song start.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SONG_CHANNELS_OFFSET = 0x40

#----------------------
# | ROOT_PITCH
# | Description: g_akao_pitch_table[0], the SPU pitch of an articulation playing its root key with no fine tune.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ROOT_PITCH = 0x1000

#----------------------
# | SPU_BASE_RATE
# | Description: Sample rate in Hz that SPU pitch 0x1000 plays at.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SPU_BASE_RATE = 44100

#----------------------
# | Header
# | Description: AkaoHeader fields after the magic: id, length, reverb type and the six BCD timestamp bytes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Header = collections.namedtuple("Header", "id length reverb_type timestamp")

#----------------------
# | Articulation
# | Description: AkaoArticulation: bank-relative sample and loop addresses, fine tune, root key and the two SPU ADSR halfwords.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Articulation = collections.namedtuple("Articulation", "sample_addr loop_addr fine_tune root_key adsr1 adsr2")

#----------------------
# | Bank
# | Description: A parsed instrument bank; sample_data is clamped to the resource while declared_sample_size keeps the header value.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Bank = collections.namedtuple("Bank", "header spu_dest_addr declared_sample_size bank_id unknown_20 articulations sample_data end")

#----------------------
# | Song
# | Description: AkaoSongDescriptor fields, with key map, note table and channel offsets resolved to absolute positions.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
Song = collections.namedtuple("Song", "header unk10 unk14 channel_mask voice_alloc_low_mask static_voice_mask key_map_offset note_table_offset channel_offsets")


#----------------------
# | parse_header
# | Description: Validates the AKAO magic at offset and returns the header fields.
# | Author: suinevere
# | Dependencies: MAGIC, Header
# | Globals: N/A
# | Params: data -- resource bytes, offset -- header position
# | Returns: Header
#----------------------
def parse_header(data, offset=0):
    if data[offset:offset + 4] != MAGIC:
        raise ValueError("no AKAO magic at 0x%X" % offset)
    ident, length, reverb = struct.unpack_from("<HHH", data, offset + 4)
    return Header(ident, length, reverb, bytes(data[offset + 10:offset + 16]))


#----------------------
# | parse_container
# | Description: Reads an AkaoContainerHeader: a section count then that many offsets from the container start.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: data -- resource bytes, offset -- container position
# | Returns: list of absolute section offsets
#----------------------
def parse_container(data, offset=0):
    (count,) = struct.unpack_from("<I", data, offset)
    return [offset + value for value in struct.unpack_from("<%dI" % count, data, offset + 4)]


#----------------------
# | parse_effect_table
# | Description: Reads an effect table: a count n, then n offsets from the table whose last one marks the table end.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: data -- resource bytes, offset -- table position
# | Returns: (absolute effect offsets, absolute table end)
#----------------------
def parse_effect_table(data, offset=0):
    (count,) = struct.unpack_from("<i", data, offset)
    offsets = struct.unpack_from("<%di" % count, data, offset + 4)
    return [offset + value for value in offsets[:-1]], offset + offsets[-1]


#----------------------
# | parse_bank
# | Description: Parses an AkaoBankHeader, its articulation table and the sample data that follows, clamped to the resource.
# | Author: suinevere
# | Dependencies: parse_header, Articulation, Bank
# | Globals: N/A
# | Params: data -- resource bytes, offset -- bank position
# | Returns: Bank
#----------------------
def parse_bank(data, offset=0):
    header = parse_header(data, offset)
    dest, size, bank_id, count, unknown = struct.unpack_from("<5I", data, offset + HEADER_SIZE)
    table = offset + BANK_HEADER_SIZE
    articulations = [Articulation(*struct.unpack_from("<IIhhHH", data, table + i * ARTICULATION_SIZE)) for i in range(count)]
    start = table + count * ARTICULATION_SIZE
    sample_data = bytes(data[start:start + size])
    return Bank(header, dest, size, bank_id, unknown, articulations, sample_data, start + len(sample_data))


#----------------------
# | parse_song
# | Description: Parses an AkaoSongDescriptor header; offsets are relative to their own fields and channels follow channel_mask bit order.
# | Author: suinevere
# | Dependencies: parse_header, Song
# | Globals: N/A
# | Params: data -- resource bytes, offset -- song position
# | Returns: Song
#----------------------
def parse_song(data, offset=0):
    header = parse_header(data, offset)
    unk10, unk14 = struct.unpack_from("<Ii", data, offset + 0x10)
    mask, alloc, static, _, key_map, note_table = struct.unpack_from("<iiiIii", data, offset + 0x20)
    channels = []
    for bit in range(32):
        if mask & (1 << bit):
            field = offset + SONG_CHANNELS_OFFSET + 2 * len(channels)
            channels.append(field + struct.unpack_from("<H", data, field)[0])
    return Song(header, unk10, unk14, mask, alloc, static,
                offset + 0x30 + key_map if key_map else None,
                offset + 0x34 + note_table if note_table else None,
                channels)


#----------------------
# | root_pitch
# | Description: SPU pitch of an articulation playing its root key, following akao_compute_pitch's fine-tune rules.
# | Author: suinevere
# | Dependencies: ROOT_PITCH
# | Globals: N/A
# | Params: art -- Articulation
# | Returns: 16-bit SPU pitch
#----------------------
def root_pitch(art):
    if art.fine_tune == 0:
        pitch = ROOT_PITCH << 8
    elif art.fine_tune < 0:
        pitch = (ROOT_PITCH * (art.fine_tune & 0xFFFF)) >> 8
    else:
        pitch = ((ROOT_PITCH * art.fine_tune) >> 7) + (ROOT_PITCH << 8)
    return (pitch >> 8) & 0xFFFF


#----------------------
# | root_rate_hz
# | Description: Playback rate in Hz of an articulation at its root key.
# | Author: suinevere
# | Dependencies: root_pitch, SPU_BASE_RATE, ROOT_PITCH
# | Globals: N/A
# | Params: art -- Articulation
# | Returns: integer rate in Hz
#----------------------
def root_rate_hz(art):
    return SPU_BASE_RATE * root_pitch(art) // ROOT_PITCH
