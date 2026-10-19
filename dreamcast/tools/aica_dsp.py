#----------------------
# | aica_dsp.py
# | Description: Encodes AICA DSP instruction steps and ports Flycast's DSP interpreter (dsp.cpp, dsp_interp.cpp) so a generated reverb program can be checked on the host.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: FIELDS
# | Params: N/A
# | Returns: N/A
#----------------------

#----------------------
# | FIELDS
# | Description: The step() keyword fields, in MPRO bit order; every field defaults to 0 when omitted.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FIELDS = ("tra", "twt", "twa", "xsel", "ysel", "ira", "iwt", "iwa", "table", "mwt", "mrd", "ewt", "ewa",
          "adrl", "frcl", "shift", "yrl", "negb", "zero", "bsel", "nofl", "masa", "adreb", "nxadr")


#----------------------
# | step
# | Description: Builds one DSP step's field dict, every FIELDS key defaulting to 0, plus a signed 16-bit "coef" (default 0).
# | Author: suinevere
# | Dependencies: FIELDS
# | Globals: N/A
# | Params: fields -- keyword overrides, keys must be in FIELDS or "coef"
# | Returns: dict with every FIELDS key and "coef"
#----------------------
def step(**fields):
    s = {name: 0 for name in FIELDS}
    s["coef"] = 0
    for key, value in fields.items():
        if key not in s:
            raise KeyError(key)
        s[key] = value
    return s


#----------------------
# | encode
# | Description: Packs a step's fields into the four MPRO words Flycast's DecodeInst would invert.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: s -- a step() dict
# | Returns: (w0, w1, w2, w3), each 0..0xFFFF
#----------------------
def encode(s):
    w0 = (s["tra"] << 9) | (s["twt"] << 8) | (s["twa"] << 1)
    w1 = (s["xsel"] << 15) | (s["ysel"] << 13) | (s["ira"] << 7) | (s["iwt"] << 6) | (s["iwa"] << 1)
    w2 = ((s["table"] << 15) | (s["mwt"] << 14) | (s["mrd"] << 13) | (s["ewt"] << 12) | (s["ewa"] << 8)
          | (s["adrl"] << 7) | (s["frcl"] << 6) | (s["shift"] << 4) | (s["yrl"] << 3) | (s["negb"] << 2)
          | (s["zero"] << 1) | s["bsel"])
    w3 = (s["nofl"] << 15) | (s["masa"] << 9) | (s["adreb"] << 8) | (s["nxadr"] << 7)
    return (w0 & 0xFFFF, w1 & 0xFFFF, w2 & 0xFFFF, w3 & 0xFFFF)


#----------------------
# | coef_word
# | Description: Converts a float gain factor to the COEF table's signed-16-bit-as-u16 encoding.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: factor -- gain, typically -1.0..1.0
# | Returns: clamp(round(factor*32768), -32768, 32767) & 0xFFFF
#----------------------
def coef_word(factor):
    value = round(factor * 32768.0)
    value = max(-32768, min(32767, value))
    return value & 0xFFFF


#----------------------
# | _to_s16
# | Description: Reinterprets a 16-bit value as signed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- 0..65535 or already-signed
# | Returns: -32768..32767
#----------------------
def _to_s16(value):
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


#----------------------
# | _to_s32
# | Description: Reinterprets a 32-bit value as signed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- 0..0xFFFFFFFF or already-signed
# | Returns: -2**31..2**31-1
#----------------------
def _to_s32(value):
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


#----------------------
# | pack
# | Description: Line-for-line port of Flycast's PACK: floats a 24-bit linear sample into the AICA's packed 16-bit SRAM format.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- signed 24-bit sample, sign-extended
# | Returns: packed 16-bit word, 0..0xFFFF
#----------------------
def pack(value):
    sign = (value >> 23) & 1
    temp = (value ^ (value << 1)) & 0xFFFFFF
    exponent = 0
    for _ in range(12):
        if temp & 0x800000:
            break
        temp <<= 1
        exponent += 1

    if exponent < 12:
        val = value << exponent
    else:
        val = value << 11
    val = (val >> 11) & 0x7FF
    val |= sign << 15
    val |= exponent << 11

    return val & 0xFFFF


#----------------------
# | unpack
# | Description: Line-for-line port of Flycast's UNPACK: expands the AICA's packed 16-bit SRAM format back to a 24-bit linear sample.
# | Author: suinevere
# | Dependencies: _to_s32
# | Globals: N/A
# | Params: word -- packed 16-bit word, 0..0xFFFF
# | Returns: signed 24-bit sample, sign-extended to a Python int
#----------------------
def unpack(word):
    word &= 0xFFFF
    sign = (word >> 15) & 1
    exponent = (word >> 11) & 0xF
    mantissa = word & 0x7FF

    uval = mantissa << 11
    uval |= sign << 22
    if exponent > 11:
        exponent = 11
    else:
        uval ^= 1 << 22
    uval |= sign << 23

    uval = _to_s32(uval << 8) >> 8
    uval >>= exponent

    return uval


#----------------------
# | _decode_step
# | Description: Line-for-line port of Flycast's DecodeInst: splits one step's four MPRO words into its fields.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: w0, w1, w2, w3 -- the step's MPRO words, each 0..0xFFFF
# | Returns: (tra, twt, twa, xsel, ysel, ira, iwt, iwa, table, mwt, mrd, ewt, ewa, adrl, frcl, shift, yrl, negb, zero, bsel, nofl, masa, adreb, nxadr)
#----------------------
def _decode_step(w0, w1, w2, w3):
    tra = (w0 >> 9) & 0x7F
    twt = bool(w0 & 0x100)
    twa = (w0 >> 1) & 0x7F

    xsel = bool(w1 & 0x8000)
    ysel = (w1 >> 13) & 3
    ira = (w1 >> 7) & 0x3F
    iwt = bool(w1 & 0x40)
    iwa = (w1 >> 1) & 0x1F

    table = bool(w2 & 0x8000)
    mwt = bool(w2 & 0x4000)
    mrd = bool(w2 & 0x2000)
    ewt = bool(w2 & 0x1000)
    ewa = (w2 >> 8) & 0x0F
    adrl = bool(w2 & 0x80)
    frcl = bool(w2 & 0x40)
    shift = (w2 >> 4) & 3
    yrl = bool(w2 & 8)
    negb = bool(w2 & 4)
    zero = bool(w2 & 2)
    bsel = bool(w2 & 1)

    nofl = bool(w3 & 0x8000)
    masa = (w3 >> 9) & 0x3F
    adreb = bool(w3 & 0x100)
    nxadr = bool(w3 & 0x80)

    return (tra, twt, twa, xsel, ysel, ira, iwt, iwa, table, mwt, mrd, ewt, ewa,
            adrl, frcl, shift, yrl, negb, zero, bsel, nofl, masa, adreb, nxadr)


#----------------------
# | Dsp
# | Description: Port of Flycast's AICA DSP interpreter (state + runStep), simulating sample by sample for host-side checks.
# | Author: suinevere
# | Dependencies: pack, unpack, _decode_step, _to_s16
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class Dsp:
    #----------------------
    # | Dsp.__init__
    # | Description: Pre-decodes the 128 MPRO steps once and sets up DSP state the way Flycast's dsp::init does.
    # | Author: suinevere
    # | Dependencies: _decode_step
    # | Globals: N/A
    # | Params: mpro -- 512 MPRO words; coef -- 128 COEF words; madrs -- 64 MADRS words; rbl_words -- ring buffer length in words
    # | Returns: N/A
    #----------------------
    def __init__(self, mpro, coef, madrs, rbl_words):
        mpro = list(mpro)
        coef = list(coef)
        madrs = list(madrs)
        if len(mpro) != 512:
            raise ValueError("mpro must have 512 words")
        if len(coef) != 128:
            raise ValueError("coef must have 128 words")
        if len(madrs) != 64:
            raise ValueError("madrs must have 64 words")

        self.coef = [_to_s16(c) for c in coef]
        self.madrs = [m & 0xFFFF for m in madrs]
        self.rbl_mask = rbl_words - 1
        self.ring = [0x6000] * rbl_words

        self.steps = []
        for i in range(128):
            w0, w1, w2, w3 = (w & 0xFFFF for w in mpro[i * 4:i * 4 + 4])
            if w0 == 0 and w1 == 0 and w2 == 0 and w3 == 0:
                self.steps.append(None)
            else:
                self.steps.append(_decode_step(w0, w1, w2, w3))

        self.mems = [0] * 32
        self.temp = [0] * 128
        self.mdec_ct = 1
        self.efreg = [0] * 16

    #----------------------
    # | Dsp.run
    # | Description: Line-for-line port of Flycast's runStep, run once per MIXS sample; MDEC_CT, MEMS, TEMP and EFREG persist across samples as in the real DSP.
    # | Author: suinevere
    # | Dependencies: pack, unpack, _to_s16
    # | Globals: N/A
    # | Params: mixs -- iterable of 16-tuples, each a sample's MIXS0..15 as 20-bit ints
    # | Returns: list of 16-tuples, EFREG after each sample, each entry signed 16-bit
    #----------------------
    def run(self, mixs):
        out = []
        steps = self.steps
        temp = self.temp
        mems = self.mems
        ring = self.ring
        coef = self.coef
        madrs = self.madrs
        rbl_mask = self.rbl_mask
        efreg = self.efreg
        mdec_ct = self.mdec_ct

        for mix in mixs:
            acc = 0
            shifted = 0
            x = 0
            y = 0
            b = 0
            inputs = 0
            memval = [0, 0, 0, 0]
            frc_reg = 0
            y_reg = 0
            adrs_reg = 0

            for i in range(128):
                decoded = steps[i]

                if decoded is None:
                    x = temp[mdec_ct & 0x7F]
                    y = frc_reg
                    acc = ((x * y) >> 12) + x
                    continue

                (tra, twt, twa, xsel, ysel, ira, iwt, iwa, table, mwt, mrd, ewt, ewa,
                 adrl, frcl, shift, yrl, negb, zero, bsel, nofl, masa, adreb, nxadr) = decoded

                if ira <= 0x1F:
                    inputs = mems[ira]
                elif ira <= 0x2F:
                    inputs = mix[ira - 0x20] << 4
                else:
                    inputs = 0

                if iwt:
                    mems[iwa] = memval[i & 3]

                if zero:
                    b = 0
                else:
                    b = acc if bsel else temp[(tra + mdec_ct) & 0x7F]
                    if negb:
                        b = -b

                x = inputs if xsel else temp[(tra + mdec_ct) & 0x7F]

                if ysel == 0:
                    y = frc_reg
                elif ysel == 1:
                    y = coef[i] >> 3
                elif ysel == 2:
                    y = y_reg >> 11
                else:
                    y = (y_reg >> 4) & 0x0FFF

                if yrl:
                    y_reg = inputs

                shifted = acc if shift == 0 or shift == 3 else acc << 1
                if shift < 2:
                    shifted = min(max(shifted, -0x00800000), 0x007FFFFF)

                acc = ((x * y) >> 12) + b

                if twt:
                    temp[(twa + mdec_ct) & 0x7F] = shifted

                if frcl:
                    frc_reg = (shifted & 0x0FFF) if shift == 3 else (shifted >> 11)

                if i & 1 and (mrd or mwt):
                    assert not table
                    assert not adreb
                    assert not nxadr
                    addr = (madrs[masa] + mdec_ct) & rbl_mask
                    if mrd:
                        memval[(i + 2) & 3] = unpack(ring[addr])
                    if mwt:
                        ring[addr] = pack(shifted)

                if adrl:
                    adrs_reg = (shifted >> 12) if shift == 3 else (inputs >> 16)

                if ewt:
                    efreg[ewa] = shifted >> 8

            mdec_ct -= 1
            if mdec_ct == 0:
                mdec_ct = rbl_mask + 1

            out.append(tuple(_to_s16(v) for v in efreg))

        self.mdec_ct = mdec_ct
        return out
