#----------------------
# | sector.py
# | Description: Encodes 2352-byte CD Mode 1 sectors (sync, BCD MSF header, EDC, ECC P and Q) from 2048 bytes of user data, as mkdcdisc writes them in the GDI's tracks.
# | Author: suinevere
# | Dependencies: ECMA-130 Annex A/B
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import operator

#----------------------
# | SECTOR / USER
# | Description: Raw sector size and Mode 1 user-data size.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECTOR = 2352
USER = 2048

#----------------------
# | SYNC
# | Description: The 12-byte sync pattern every raw data sector starts with.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SYNC = b"\x00" + b"\xff" * 10 + b"\x00"


#----------------------
# | _tables
# | Description: Builds the GF(2^8) forward and backward ECC tables as 256-byte translate tables, and the EDC CRC table for the reflected polynomial 0xD8018001.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: (forward bytes, backward bytes, edc list)
#----------------------
def _tables():
    forward, backward, edc = bytearray(256), bytearray(256), [0] * 256
    for i in range(256):
        j = ((i << 1) ^ (0x11D if i & 0x80 else 0)) & 0xFF
        forward[i] = j
        backward[i ^ j] = i
        v = i
        for _ in range(8):
            v = (v >> 1) ^ (0xD8018001 if v & 1 else 0)
        edc[i] = v
    return bytes(forward), bytes(backward), edc


#----------------------
# | FORWARD / BACKWARD / EDC_TABLE
# | Description: The tables _tables builds, computed once at import.
# | Author: suinevere
# | Dependencies: _tables
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FORWARD, BACKWARD, EDC_TABLE = _tables()


#----------------------
# | _gathers
# | Description: One itemgetter per minor step that picks that step's byte for every major column of an ECC block, wrapping at major_count * minor_count.
# | Author: suinevere
# | Dependencies: operator.itemgetter
# | Globals: N/A
# | Params: major_count, minor_count, major_mult, minor_inc -- ECMA-130 block shape
# | Returns: list of itemgetters, one per minor step
#----------------------
def _gathers(major_count, minor_count, major_mult, minor_inc):
    size = major_count * minor_count
    bases = [(major >> 1) * major_mult + (major & 1) for major in range(major_count)]
    return [operator.itemgetter(*[(base + minor * minor_inc) % size for base in bases]) for minor in range(minor_count)]


#----------------------
# | P_GATHERS / Q_GATHERS
# | Description: Gathers for the P block (86 columns of 24) and the Q block (52 diagonals of 43).
# | Author: suinevere
# | Dependencies: _gathers
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
P_GATHERS = _gathers(86, 24, 2, 86)
Q_GATHERS = _gathers(52, 43, 86, 88)


#----------------------
# | _xor
# | Description: XORs two equal-length byte strings.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: a, b -- byte strings of the same length
# | Returns: bytes
#----------------------
def _xor(a, b):
    return (int.from_bytes(a, "little") ^ int.from_bytes(b, "little")).to_bytes(len(a), "little")


#----------------------
# | _ecc_block
# | Description: Computes one ECC block's parity over src for every column at once: the column values, then the column values XOR the column sum.
# | Author: suinevere
# | Dependencies: FORWARD, BACKWARD, _xor
# | Globals: FORWARD, BACKWARD
# | Params: src -- sector bytes from offset 12, gathers -- P_GATHERS or Q_GATHERS
# | Returns: bytes, twice the column count
#----------------------
def _ecc_block(src, gathers):
    width = len(gathers[0](src))
    a = c = bytes(width)
    for gather in gathers:
        row = bytes(gather(src))
        a = _xor(a, row).translate(FORWARD)
        c = _xor(c, row)
    a = _xor(a.translate(FORWARD), c).translate(BACKWARD)
    return a + _xor(a, c)


#----------------------
# | _bcd
# | Description: Packs 0-99 as one BCD byte.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- 0 to 99
# | Returns: int
#----------------------
def _bcd(value):
    return ((value // 10) << 4) | (value % 10)


#----------------------
# | encode
# | Description: Builds the raw Mode 1 sector at absolute LBA lba holding user, the way mkdcdisc writes the GDI's data tracks.
# | Author: suinevere
# | Dependencies: SYNC, EDC_TABLE, P_GATHERS, Q_GATHERS, _ecc_block, _bcd
# | Globals: N/A
# | Params: lba -- absolute disc LBA (track03 starts at 45000), user -- 2048 bytes
# | Returns: 2352 bytes
#----------------------
def encode(lba, user):
    if len(user) != USER:
        raise ValueError("Mode 1 user data must be %d bytes, got %d" % (USER, len(user)))
    x = lba + 150
    head = SYNC + bytes([_bcd(x // 4500), _bcd((x // 75) % 60), _bcd(x % 75), 1]) + user
    edc = 0
    table = EDC_TABLE
    for byte in head:
        edc = table[(edc ^ byte) & 0xFF] ^ (edc >> 8)
    tail = edc.to_bytes(4, "little") + bytes(8)
    body = head[12:] + tail
    p = _ecc_block(body, P_GATHERS)
    q = _ecc_block(body + p, Q_GATHERS)
    return head + tail + p + q


#----------------------
# | user_data
# | Description: The 2048 user-data bytes of a raw Mode 1 sector.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: raw -- 2352-byte sector
# | Returns: bytes
#----------------------
def user_data(raw):
    return bytes(raw[16:16 + USER])
