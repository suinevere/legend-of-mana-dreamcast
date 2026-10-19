#----------------------
# | ps1_reverb.py
# | Description: Reads libspu's reverb presets and work-area starts from SLUS_010.13 and renders the PS1 SPU reverb formula as a reference.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

#----------------------
# | REGISTERS
# | Description: The 32 SPU reverb registers in the table order libspu stores them, 0x1DC0-0x1DFE.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
REGISTERS = ("dAPF1", "dAPF2", "vIIR", "vCOMB1", "vCOMB2", "vCOMB3", "vCOMB4", "vWALL", "vAPF1", "vAPF2",
             "mLSAME", "mRSAME", "mLCOMB1", "mRCOMB1", "mLCOMB2", "mRCOMB2", "dLSAME", "dRSAME", "mLDIFF", "mRDIFF",
             "mLCOMB3", "mRCOMB3", "mLCOMB4", "mRCOMB4", "dLDIFF", "dRDIFF", "mLAPF1", "mRAPF1", "mLAPF2", "mRAPF2",
             "vLIN", "vRIN")

#----------------------
# | STUDIO_LARGE
# | Description: libspu reverb mode 4, Studio Large, the only mode any AKAO song on the US disc selects.
# | Author: suinevere
# | Dependencies: REGISTERS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STUDIO_LARGE = dict(zip(REGISTERS, (0x00E3, 0x00A9, 0x6F60, 0x4FA8, 0xBCE0, 0x4510, 0xBEF0, 0xA680, 0x5680, 0x52C0,
    0x0DFB, 0x0B58, 0x0D09, 0x0A3C, 0x0BD9, 0x0973, 0x0B59, 0x08DA, 0x08D9, 0x05E9, 0x07EC, 0x04B0, 0x06EF, 0x03D2,
    0x05EA, 0x031D, 0x031C, 0x0238, 0x0154, 0x00AA, 0x8000, 0x8000)))

#----------------------
# | STUDIO_LARGE_START
# | Description: libspu's work-area start value for Studio Large; the area starts at SPU address STUDIO_LARGE_START*8.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STUDIO_LARGE_START = 0xF204

#----------------------
# | PRESET_TABLE
# | Description: SLUS_010.13 virtual address of libspu's D_8003E5E0, ten 68-byte reverb preset entries (a u32 mask then the 32 registers).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PRESET_TABLE = 0x8003E5E0

#----------------------
# | START_TABLE
# | Description: SLUS_010.13 virtual address of libspu's D_8003D130, ten u32 work-area start entries by reverb mode.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
START_TABLE = 0x8003D130

#----------------------
# | EXE_LOAD_BASE
# | Description: Virtual address SLUS_010.13 file offset 0 loads at; file offset = address - EXE_LOAD_BASE.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXE_LOAD_BASE = 0x8000F800

#----------------------
# | _PRESET_ENTRY_SIZE
# | Description: Bytes in one preset table entry: a u32 mask plus the 32 u16 registers.
# | Author: suinevere
# | Dependencies: REGISTERS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
_PRESET_ENTRY_SIZE = 4 + 2 * len(REGISTERS)

#----------------------
# | _PRESET_COUNT
# | Description: Reverb modes libspu's preset and start tables hold.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
_PRESET_COUNT = 10


#----------------------
# | s16
# | Description: Reinterprets a 16-bit register value as signed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- 0..65535
# | Returns: -32768..32767
#----------------------
def s16(value):
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


#----------------------
# | _clamp16
# | Description: Saturates a value to the signed 16-bit range a buffer store or output goes through.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- integer
# | Returns: value limited to -32768..32767
#----------------------
def _clamp16(value):
    return max(-32768, min(32767, value))


#----------------------
# | _mul
# | Description: The SPU's fixed-point multiply: a signed value times a signed Q15 coefficient, arithmetic-shifted back down.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: a -- value, b -- Q15 coefficient
# | Returns: (a * b) >> 15
#----------------------
def _mul(a, b):
    return (a * b) >> 15


#----------------------
# | read_presets
# | Description: Reads libspu's ten reverb presets out of SLUS_010.13's bytes.
# | Author: suinevere
# | Dependencies: PRESET_TABLE, EXE_LOAD_BASE, REGISTERS, _PRESET_ENTRY_SIZE, _PRESET_COUNT
# | Globals: N/A
# | Params: exe -- SLUS_010.13 file bytes
# | Returns: list of 10 dicts, register name to raw 16-bit value
#----------------------
def read_presets(exe):
    base = PRESET_TABLE - EXE_LOAD_BASE
    presets = []
    for i in range(_PRESET_COUNT):
        offset = base + i * _PRESET_ENTRY_SIZE
        values = struct.unpack_from("<I%dH" % len(REGISTERS), exe, offset)
        presets.append(dict(zip(REGISTERS, values[1:])))
    return presets


#----------------------
# | read_starts
# | Description: Reads libspu's ten work-area start values out of SLUS_010.13's bytes.
# | Author: suinevere
# | Dependencies: START_TABLE, EXE_LOAD_BASE, _PRESET_COUNT
# | Globals: N/A
# | Params: exe -- SLUS_010.13 file bytes
# | Returns: list of 10 ints, the low 16 bits of each u32 entry
#----------------------
def read_starts(exe):
    base = START_TABLE - EXE_LOAD_BASE
    values = struct.unpack_from("<%dI" % _PRESET_COUNT, exe, base)
    return [value & 0xFFFF for value in values]


#----------------------
# | render
# | Description: Renders the PS1 SPU reverb formula over a 22,050 Hz mono-per-channel input through one preset's work area.
# | Author: suinevere
# | Dependencies: s16, _clamp16, _mul, STUDIO_LARGE_START
# | Globals: N/A
# | Params: preset -- dict of raw register values, left/right -- 22,050 Hz sample lists, vlout/vrout -- output volume registers, start -- work-area start value
# | Returns: (out_l, out_r) sample lists, one entry per input tick
#----------------------
def render(preset, left, right, vlout=0x7FFF, vrout=0x7FFF, start=STUDIO_LARGE_START):
    size = (0x80000 - start * 8) // 2
    buf = [0] * size
    pos = 0

    v_iir = s16(preset["vIIR"])
    v_wall = s16(preset["vWALL"])
    v_comb = [s16(preset["vCOMB%d" % k]) for k in (1, 2, 3, 4)]
    v_apf1 = s16(preset["vAPF1"])
    v_apf2 = s16(preset["vAPF2"])
    v_lout = s16(vlout)
    v_rout = s16(vrout)

    d_apf1 = preset["dAPF1"] * 4
    d_apf2 = preset["dAPF2"] * 4
    m_same = (preset["mLSAME"] * 4, preset["mRSAME"] * 4)
    d_same = (preset["dLSAME"] * 4, preset["dRSAME"] * 4)
    m_diff = (preset["mLDIFF"] * 4, preset["mRDIFF"] * 4)
    d_diff = (preset["dLDIFF"] * 4, preset["dRDIFF"] * 4)
    m_comb_l = [preset["mLCOMB%d" % k] * 4 for k in (1, 2, 3, 4)]
    m_comb_r = [preset["mRCOMB%d" % k] * 4 for k in (1, 2, 3, 4)]
    m_apf1 = (preset["mLAPF1"] * 4, preset["mRAPF1"] * 4)
    m_apf2 = (preset["mLAPF2"] * 4, preset["mRAPF2"] * 4)

    out_l = []
    out_r = []
    for tick in range(len(left)):
        lin = left[tick]
        rin = right[tick]

        def at(offset):
            return buf[(pos + offset) % size]

        def put(offset, value):
            buf[(pos + offset) % size] = _clamp16(value)

        old_lsame = at(m_same[0] - 1)
        put(m_same[0], _mul(lin + _mul(at(d_same[0]), v_wall) - old_lsame, v_iir) + old_lsame)

        old_rsame = at(m_same[1] - 1)
        put(m_same[1], _mul(rin + _mul(at(d_same[1]), v_wall) - old_rsame, v_iir) + old_rsame)

        old_ldiff = at(m_diff[0] - 1)
        put(m_diff[0], _mul(lin + _mul(at(d_diff[1]), v_wall) - old_ldiff, v_iir) + old_ldiff)

        old_rdiff = at(m_diff[1] - 1)
        put(m_diff[1], _mul(rin + _mul(at(d_diff[0]), v_wall) - old_rdiff, v_iir) + old_rdiff)

        l_out = sum(_mul(v_comb[k], at(m_comb_l[k])) for k in range(4))
        r_out = sum(_mul(v_comb[k], at(m_comb_r[k])) for k in range(4))

        apf1_l_old = at(m_apf1[0] - d_apf1)
        l_out -= _mul(v_apf1, apf1_l_old)
        put(m_apf1[0], l_out)
        l_out = _mul(l_out, v_apf1) + apf1_l_old

        apf1_r_old = at(m_apf1[1] - d_apf1)
        r_out -= _mul(v_apf1, apf1_r_old)
        put(m_apf1[1], r_out)
        r_out = _mul(r_out, v_apf1) + apf1_r_old

        apf2_l_old = at(m_apf2[0] - d_apf2)
        l_out -= _mul(v_apf2, apf2_l_old)
        put(m_apf2[0], l_out)
        l_out = _mul(l_out, v_apf2) + apf2_l_old

        apf2_r_old = at(m_apf2[1] - d_apf2)
        r_out -= _mul(v_apf2, apf2_r_old)
        put(m_apf2[1], r_out)
        r_out = _mul(r_out, v_apf2) + apf2_r_old

        out_l.append(_clamp16(_mul(l_out, v_lout)))
        out_r.append(_clamp16(_mul(r_out, v_rout)))

        pos = (pos + 1) % size

    return out_l, out_r
