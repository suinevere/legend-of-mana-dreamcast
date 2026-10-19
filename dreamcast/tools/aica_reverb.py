#----------------------
# | aica_reverb.py
# | Description: Generates the AICA DSP program that runs libspu's Studio Large reverb at 44.1 kHz, writes it as a C header, and simulates its impulse response.
# | Author: suinevere
# | Dependencies: aica_dsp.py, ../../port/tools/ps1_reverb.py
# | Globals: N/A
# | Params: <out.h>
# | Returns: process exit code
#----------------------
import math
import os
import sys

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _HERE)
sys.path.insert(0, os.path.normpath(os.path.join(_HERE, "..", "..", "port", "tools")))

import aica_dsp  # noqa: E402
from ps1_reverb import STUDIO_LARGE, s16  # noqa: E402

#----------------------
# | REVERB_RAM / REVERB_RAM_BYTES / RING_RBL / RING_RBP / RING_WORDS / RING_REG
# | Description: The ring's AICA RAM base and size, its RBL/RBP fields, its length in words and the common 0x2804 register value.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
REVERB_RAM = 0x1A8000
REVERB_RAM_BYTES = 0x10000
RING_RBL = 2
RING_RBP = REVERB_RAM // 2048
RING_WORDS = 8192 << RING_RBL
RING_REG = (RING_RBL << 13) | RING_RBP

#----------------------
# | STEPS
# | Description: Live steps in the program; steps STEPS..127 are all-zero words.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STEPS = 66

#----------------------
# | STEP_LEFT_DEPTH / STEP_LEFT_APF / STEP_RIGHT_DEPTH / STEP_RIGHT_APF
# | Description: The steps whose COEF carries vLOUT, vAPF2 times vLOUT, vROUT and vAPF2 times vROUT.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STEP_LEFT_DEPTH = 55
STEP_LEFT_APF = 56
STEP_RIGHT_DEPTH = 63
STEP_RIGHT_APF = 64

#----------------------
# | STEP_INPUT_L0 / STEP_INPUT_L1 / STEP_INPUT_R0 / STEP_INPUT_R1
# | Description: The steps whose COEF carries the input gain, vLIN for the left pair and vRIN for the right.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
STEP_INPUT_L0 = 0
STEP_INPUT_L1 = 1
STEP_INPUT_R0 = 3
STEP_INPUT_R1 = 4

#----------------------
# | ONE
# | Description: The largest COEF, standing in for a gain of 1.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ONE = 0x7FFF

#----------------------
# | VAPF2
# | Description: Studio Large's vAPF2, folded into the output depth COEFs.
# | Author: suinevere
# | Dependencies: STUDIO_LARGE
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
VAPF2 = STUDIO_LARGE["vAPF2"]

#----------------------
# | READS / WRITES
# | Description: The program's ring taps in MADRS order: 20 reads, as (register, minus register, plus one), then 8 writes.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
READS = (("dLSAME", None, 0), ("mLSAME", None, 1), ("dRDIFF", None, 0), ("mLDIFF", None, 1),
         ("dRSAME", None, 0), ("mRSAME", None, 1), ("dLDIFF", None, 0), ("mRDIFF", None, 1),
         ("mLCOMB1", None, 0), ("mLCOMB2", None, 0), ("mLCOMB3", None, 0), ("mLCOMB4", None, 0),
         ("mRCOMB1", None, 0), ("mRCOMB2", None, 0), ("mRCOMB3", None, 0), ("mRCOMB4", None, 0),
         ("mLAPF1", "dAPF1", 0), ("mLAPF2", "dAPF2", 0), ("mRAPF1", "dAPF1", 0), ("mRAPF2", "dAPF2", 0))
WRITES = ("mLSAME", "mLDIFF", "mRSAME", "mRDIFF", "mLAPF1", "mLAPF2", "mRAPF1", "mRAPF2")

#----------------------
# | WRITE_STEPS
# | Description: The odd step each write MADRS (20 + index) is stored on.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WRITE_STEPS = (43, 45, 47, 49, 51, 55, 59, 63)

#----------------------
# | T_LC .. T_TR2
# | Description: TEMP slots; all even, so a slot's last-sample value at slot + 1 never lands on another slot.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
T_LC, T_RC, T_LIN, T_RIN, T_U1, T_U2, T_U3, T_U4 = 0, 2, 4, 6, 8, 10, 12, 14
T_YLS, T_YLD, T_YRS, T_YRD, T_CL, T_CR = 16, 18, 20, 22, 24, 26
T_TL1, T_O1, T_TL2, T_TR1, T_O2, T_TR2 = 28, 30, 32, 34, 36, 38


#----------------------
# | _coef
# | Description: Saturates a raw COEF value to signed 16 bits.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: value -- integer
# | Returns: -32768..32767
#----------------------
def _coef(value):
    return max(-32768, min(32767, value))


#----------------------
# | _factor
# | Description: Converts a float gain to a signed COEF through aica_dsp.coef_word.
# | Author: suinevere
# | Dependencies: aica_dsp.coef_word, s16
# | Globals: N/A
# | Params: factor -- gain, -1.0..1.0
# | Returns: signed 16-bit COEF
#----------------------
def _factor(factor):
    return s16(aica_dsp.coef_word(factor))


#----------------------
# | depth_coefs
# | Description: The four COEF words for steps 55, 56, 63 and 64 that set the left and right output depth.
# | Author: suinevere
# | Dependencies: _coef, VAPF2, s16
# | Globals: N/A
# | Params: left -- vLOUT, right -- vROUT, signed 16-bit
# | Returns: (left, vAPF2*left, right, vAPF2*right) as u16 COEF words
#----------------------
def depth_coefs(left, right):
    v = s16(VAPF2)
    return (_coef(left) & 0xFFFF, _coef((v * left) >> 15) & 0xFFFF,
            _coef(right) & 0xFFFF, _coef((v * right) >> 15) & 0xFFFF)


#----------------------
# | _tap
# | Description: The MADRS for a register offset in 8-byte units, doubled for 44.1 kHz at rate 2.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: x -- register value, rate -- 2 for the real program, 1 for undoubled delays
# | Returns: (-8*x*rate/2) & 0x7FFF
#----------------------
def _tap(x, rate):
    return (-4 * x * rate) & 0x7FFF


#----------------------
# | _alu
# | Description: Sets a step's ALU fields, with Y always the step's COEF and B from TEMP[tra] unless bsel or zero.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: s -- step dict, coef -- signed COEF, fields -- other step fields
# | Returns: N/A
#----------------------
def _alu(s, coef, **fields):
    s["zero"] = 0
    s["ysel"] = 1
    s["coef"] = coef
    s.update(fields)


#----------------------
# | build
# | Description: Builds the Studio Large program: 128 steps (66 live) and 64 MADRS, asserting every field and tap is in range.
# | Author: suinevere
# | Dependencies: aica_dsp.step, _alu, _tap, _factor, depth_coefs, READS, WRITES, WRITE_STEPS, T_*, RING_WORDS
# | Globals: N/A
# | Params: preset -- raw register dict, rate -- 2 real or 1 undoubled, depth -- (vLOUT, vROUT)
# | Returns: (steps, madrs) -- 128 step dicts and 64 MADRS words
#----------------------
def build(preset=STUDIO_LARGE, rate=2, depth=(0, 0)):
    wall = s16(preset["vWALL"]) / 32768.0
    a = 1.0 - math.sqrt(1.0 - s16(preset["vIIR"]) / 32768.0)
    h_l = _factor(s16(preset["vLIN"]) / 65536.0)
    h_r = _factor(s16(preset["vRIN"]) / 65536.0)
    combs = [s16(preset["vCOMB%d" % k]) for k in (1, 2, 3, 4)]
    p1 = s16(preset["vAPF1"])
    p2 = s16(preset["vAPF2"])
    dl, dl_apf, dr, dr_apf = (s16(c) for c in depth_coefs(*depth))

    steps = [aica_dsp.step(zero=1) for _ in range(STEPS)] + [aica_dsp.step() for _ in range(128 - STEPS)]

    _alu(steps[0], h_l, xsel=1, ira=0x20, zero=1)
    _alu(steps[1], h_l, xsel=1, ira=0x22, bsel=1)
    _alu(steps[2], ONE, twt=1, twa=T_LC, tra=T_LC + 1, bsel=1)
    _alu(steps[3], h_r, twt=1, twa=T_LIN, xsel=1, ira=0x21, zero=1)
    _alu(steps[4], h_r, xsel=1, ira=0x22, bsel=1)
    _alu(steps[5], ONE, twt=1, twa=T_RC, tra=T_RC + 1, bsel=1)
    steps[6].update(twt=1, twa=T_RIN)

    for base, mems, tin, tu, ty in ((7, 0, T_LIN, T_U1, T_YLS), (11, 2, T_LIN, T_U2, T_YLD),
                                    (15, 4, T_RIN, T_U3, T_YRS), (19, 6, T_RIN, T_U4, T_YRD)):
        _alu(steps[base], _factor(wall), xsel=1, ira=mems, tra=tin)
        _alu(steps[base + 1], _factor(1.0 - a), twt=1, twa=tu, xsel=1, ira=mems + 1, zero=1)
        _alu(steps[base + 2], _factor(a), tra=tu, bsel=1)
        steps[base + 3].update(twt=1, twa=ty)

    for base, mems, tc in ((23, 8, T_CL), (34, 12, T_CR)):
        _alu(steps[base], combs[0], xsel=1, ira=mems, zero=1)
        for k in (1, 2, 3):
            _alu(steps[base + k], combs[k], xsel=1, ira=mems + k, bsel=1)
        steps[base + 4].update(twt=1, twa=tc)

    for index, slot in zip((42, 44, 46, 48), (T_YLS, T_YLD, T_YRS, T_YRD)):
        _alu(steps[index], 0, tra=slot)

    for base, mems, tc, t1, o, t2, d, d_apf, ewa in ((50, 16, T_CL, T_TL1, T_O1, T_TL2, dl, dl_apf, 0),
                                                     (58, 18, T_CR, T_TR1, T_O2, T_TR2, dr, dr_apf, 1)):
        _alu(steps[base], _coef(-p1), xsel=1, ira=mems, tra=tc)
        _alu(steps[base + 1], ONE, twt=1, twa=t1, xsel=1, ira=mems, zero=1)
        _alu(steps[base + 2], p1, tra=t1, bsel=1)
        steps[base + 3].update(twt=1, twa=o)
        _alu(steps[base + 4], _coef(-p2), xsel=1, ira=mems + 1, tra=o)
        _alu(steps[base + 5], d, twt=1, twa=t2, xsel=1, ira=mems + 1, zero=1)
        _alu(steps[base + 6], d_apf, tra=t2, bsel=1)
        steps[base + 7].update(ewt=1, ewa=ewa)

    for k in range(len(READS)):
        steps[2 * k + 1].update(mrd=1, masa=k)
        steps[2 * k + 3].update(iwt=1, iwa=k)
    for k, index in enumerate(WRITE_STEPS):
        steps[index].update(mwt=1, masa=len(READS) + k)

    madrs = [0] * 64
    for k, (name, minus, plus) in enumerate(READS):
        x = preset[name] - (preset[minus] if minus else 0)
        madrs[k] = _tap(x, rate) + plus
    for k, name in enumerate(WRITES):
        madrs[len(READS) + k] = _tap(preset[name], rate)

    limits = {"tra": 0x80, "twa": 0x80, "ira": 0x30, "iwa": 0x20, "masa": 0x40, "ewa": 0x10, "ysel": 4, "shift": 4}
    for i, s in enumerate(steps):
        for name in aica_dsp.FIELDS:
            assert 0 <= s[name] < limits.get(name, 2), (i, name, s[name])
        assert -32768 <= s["coef"] <= 32767, (i, s["coef"])
        if s["mrd"]:
            assert i & 1 and 1 <= i <= 39, i
        if s["mwt"]:
            assert i & 1, i
        if i >= STEPS:
            assert not any(aica_dsp.encode(s)), i
    assert len(steps) <= 128
    read_madrs = set(madrs[:len(READS)])
    write_madrs = set(madrs[len(READS):len(READS) + len(WRITES)])
    assert not read_madrs & write_madrs
    longest = max(preset[name] for name in WRITES + tuple(r[0] for r in READS))
    assert 4 * longest * rate + 1 < RING_WORDS
    for value in madrs:
        assert 0 <= value < RING_WORDS, value

    return steps, madrs


#----------------------
# | program_words
# | Description: Encodes 128 steps into the 512 MPRO words.
# | Author: suinevere
# | Dependencies: aica_dsp.encode
# | Globals: N/A
# | Params: steps -- 128 step dicts
# | Returns: list of 512 u16 words
#----------------------
def program_words(steps):
    words = []
    for s in steps:
        words.extend(aica_dsp.encode(s))
    return words


#----------------------
# | _array
# | Description: Formats a C array initialiser of u16 hex words, eight to a line.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: name -- C identifier, words -- u16 list
# | Returns: C source text
#----------------------
def _array(name, words):
    lines = ["static const unsigned short %s[%d] = {" % (name, len(words))]
    for i in range(0, len(words), 8):
        lines.append("    " + ", ".join("0x%04X" % w for w in words[i:i + 8]) + ",")
    lines.append("};")
    return "\n".join(lines)


#----------------------
# | write_header
# | Description: Writes aica_reverb_program.h with the ring placement, the live step numbers and the depth-0 COEF, MADRS and MPRO tables; the tables are guarded by AICA_REVERB_PROGRAM_ARRAYS so only the #define'ing .c file gets them.
# | Author: suinevere
# | Dependencies: build, program_words, _array, _factor, REVERB_RAM, REVERB_RAM_BYTES, RING_REG, STEPS, STEP_*, VAPF2
# | Globals: N/A
# | Params: path -- output file
# | Returns: N/A
#----------------------
def write_header(path):
    steps, madrs = build()
    coef = [s["coef"] & 0xFFFF for s in steps]
    defines = (
        ("AICA_REVERB_RAM", "0x%06Xu" % REVERB_RAM),
        ("AICA_REVERB_RAM_BYTES", "0x%05Xu" % REVERB_RAM_BYTES),
        ("AICA_REVERB_RING", "0x%04Xu" % RING_REG),
        ("AICA_REVERB_STEPS", "%d" % STEPS),
        ("AICA_REVERB_VAPF2", "0x%04X" % VAPF2),
        ("AICA_REVERB_STEP_LEFT_DEPTH", "%d" % STEP_LEFT_DEPTH),
        ("AICA_REVERB_STEP_LEFT_APF", "%d" % STEP_LEFT_APF),
        ("AICA_REVERB_STEP_RIGHT_DEPTH", "%d" % STEP_RIGHT_DEPTH),
        ("AICA_REVERB_STEP_RIGHT_APF", "%d" % STEP_RIGHT_APF),
        ("AICA_REVERB_STEP_INPUT_L0", "%d" % STEP_INPUT_L0),
        ("AICA_REVERB_STEP_INPUT_L1", "%d" % STEP_INPUT_L1),
        ("AICA_REVERB_STEP_INPUT_R0", "%d" % STEP_INPUT_R0),
        ("AICA_REVERB_STEP_INPUT_R1", "%d" % STEP_INPUT_R1),
        ("AICA_REVERB_INPUT_LEFT", "0x%04Xu" % coef[STEP_INPUT_L0]),
        ("AICA_REVERB_INPUT_RIGHT", "0x%04Xu" % coef[STEP_INPUT_R0]),
    )
    text = [
        "/*----------------------",
        " | aica_reverb_program.h",
        " | Description: Generated by dreamcast/tools/aica_reverb.py: libspu's Studio Large reverb as an AICA DSP program, its ring placement and the steps whose COEF the driver rewrites.",
        " | Author: suinevere",
        " | Dependencies: N/A",
        " | Globals: N/A",
        " | Params: N/A",
        " | Returns: N/A",
        " ----------------------*/",
        "#ifndef LOM_AICA_REVERB_PROGRAM_H",
        "#define LOM_AICA_REVERB_PROGRAM_H",
        "",
    ]
    text.extend("#define %s %s" % pair for pair in defines)
    text.append("")
    text.append("#ifdef AICA_REVERB_PROGRAM_ARRAYS")
    text.append(_array("aica_reverb_coef", coef))
    text.append(_array("aica_reverb_madrs", madrs))
    text.append(_array("aica_reverb_mpro", program_words(steps)))
    text.append("#endif")
    text.append("")
    text.append("#endif")
    text.append("")
    with open(path, "w", newline="\n") as f:
        f.write("\n".join(text))


#----------------------
# | impulse_response
# | Description: Simulates the program on aica_dsp.Dsp for a 16-bit impulse of 16384 on one MIXS bus at sample 0.
# | Author: suinevere
# | Dependencies: build, program_words, aica_dsp.Dsp, RING_WORDS
# | Globals: N/A
# | Params: seconds -- length, bus -- MIXS index, depth -- vLOUT and vROUT, rate -- 2 real or 1 undoubled
# | Returns: (left, right) EFREG 0 and 1 lists at 44.1 kHz
#----------------------
def impulse_response(seconds, bus, depth=0x7FFF, rate=2):
    steps, madrs = build(rate=rate, depth=(depth, depth))
    dsp = aica_dsp.Dsp(program_words(steps), [s["coef"] & 0xFFFF for s in steps], madrs, RING_WORDS)
    count = int(round(seconds * 44100))
    impulse = tuple((16384 << 4) if i == bus else 0 for i in range(16))
    silence = (0,) * 16
    out = dsp.run(impulse if n == 0 else silence for n in range(count))
    return [e[0] for e in out], [e[1] for e in out]


#----------------------
# | main
# | Description: Writes the generated header to the path given on the command line.
# | Author: suinevere
# | Dependencies: write_header
# | Globals: N/A
# | Params: argv -- [script, out.h]
# | Returns: 0 on success, 2 on bad usage
#----------------------
def main(argv):
    if len(argv) != 2:
        sys.stderr.write("usage: aica_reverb.py <out.h>\n")
        return 2
    write_header(argv[1])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
