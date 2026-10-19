#----------------------
# | test_aica_dsp.py
# | Description: Checks dreamcast/tools/aica_dsp.py's step encoder and DSP interpreter port against Flycast's PACK/UNPACK, DecodeInst, and the runStep quirks: the 1-step SHIFTED delay, the two-step MRD/IWT rule, the TEMP ring crossing samples, and the all-zero step shortcut.
# | Author: suinevere
# | Dependencies: tools/aica_dsp.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import aica_dsp  # noqa: E402


def _program(steps, madrs_overrides=None, rbl_words=128):
    mpro = [0] * 512
    coef = [0] * 128
    for index, s in steps.items():
        w0, w1, w2, w3 = aica_dsp.encode(s)
        mpro[index * 4:index * 4 + 4] = [w0, w1, w2, w3]
        coef[index] = s["coef"]
    madrs = [0] * 64
    if madrs_overrides:
        for index, value in madrs_overrides.items():
            madrs[index] = value
    return aica_dsp.Dsp(mpro, coef, madrs, rbl_words)


def _decode_like_flycast(w0, w1, w2, w3):
    return {
        "tra": (w0 >> 9) & 0x7F,
        "twt": 1 if w0 & 0x100 else 0,
        "twa": (w0 >> 1) & 0x7F,
        "xsel": 1 if w1 & 0x8000 else 0,
        "ysel": (w1 >> 13) & 3,
        "ira": (w1 >> 7) & 0x3F,
        "iwt": 1 if w1 & 0x40 else 0,
        "iwa": (w1 >> 1) & 0x1F,
        "table": 1 if w2 & 0x8000 else 0,
        "mwt": 1 if w2 & 0x4000 else 0,
        "mrd": 1 if w2 & 0x2000 else 0,
        "ewt": 1 if w2 & 0x1000 else 0,
        "ewa": (w2 >> 8) & 0x0F,
        "adrl": 1 if w2 & 0x80 else 0,
        "frcl": 1 if w2 & 0x40 else 0,
        "shift": (w2 >> 4) & 3,
        "yrl": 1 if w2 & 8 else 0,
        "negb": 1 if w2 & 4 else 0,
        "zero": 1 if w2 & 2 else 0,
        "bsel": 1 if w2 & 1 else 0,
        "nofl": 1 if w3 & 0x8000 else 0,
        "masa": (w3 >> 9) & 0x3F,
        "adreb": 1 if w3 & 0x100 else 0,
        "nxadr": 1 if w3 & 0x80 else 0,
    }


def test_pack_unpack_basics():
    assert aica_dsp.pack(0) == 0x6000
    assert aica_dsp.unpack(0x6000) == 0
    assert aica_dsp.unpack(0) == 0x400000


def test_pack_unpack_round_trip_within_packing_error():
    span = 2 * 0x7FFFFF
    for i in range(2000):
        v = -0x7FFFFF + (i * span) // 1999
        r = aica_dsp.unpack(aica_dsp.pack(v))
        tolerance = abs(v) >> 11
        assert abs(r - v) <= tolerance, (v, r, tolerance)


def test_encode_matches_decode_inst_at_max_fields():
    maxed = aica_dsp.step(
        tra=0x7F, twt=1, twa=0x7F, xsel=1, ysel=3, ira=0x3F, iwt=1, iwa=0x1F,
        table=1, mwt=1, mrd=1, ewt=1, ewa=0xF, adrl=1, frcl=1, shift=3, yrl=1,
        negb=1, zero=1, bsel=1, nofl=1, masa=0x3F, adreb=1, nxadr=1,
    )
    w0, w1, w2, w3 = aica_dsp.encode(maxed)
    decoded = _decode_like_flycast(w0, w1, w2, w3)
    for name in aica_dsp.FIELDS:
        assert decoded[name] == maxed[name], (name, decoded[name], maxed[name])


def test_pass_through_mixs_and_coef():
    steps = {
        0: aica_dsp.step(xsel=1, ira=0x20, ysel=1, zero=1, coef=0x7FFF),
        1: aica_dsp.step(ewt=1, ewa=0),
    }
    dsp = _program(steps)
    mix = [0] * 16
    mix[0] = 1000 << 4
    out = dsp.run([tuple(mix)])
    assert out[0][0] == 999


def test_read_latency_two_step_rule():
    steps = {
        0: aica_dsp.step(xsel=1, ira=0x20, ysel=1, zero=1, coef=0x7FFF),
        1: aica_dsp.step(mwt=1, masa=0),
        3: aica_dsp.step(mrd=1, masa=1),
        5: aica_dsp.step(iwt=1, iwa=0),
        6: aica_dsp.step(xsel=1, ira=0, ysel=1, zero=1, coef=0x7FFF),
        7: aica_dsp.step(ewt=1, ewa=0),
    }
    dsp = _program(steps, madrs_overrides={0: 0, 1: 5}, rbl_words=64)

    mixs = []
    for i in range(8):
        mix = [0] * 16
        if i == 0:
            mix[0] = 1000 << 4
        mixs.append(tuple(mix))

    out = dsp.run(mixs)
    for i, sample in enumerate(out):
        if i == 5:
            assert sample[0] != 0, out
            assert abs(sample[0] - 999) <= 3, out
        else:
            assert sample[0] == 0, out


def test_read_latency_needs_two_steps_between_mrd_and_iwt():
    steps = {
        0: aica_dsp.step(xsel=1, ira=0x20, ysel=1, zero=1, coef=0x7FFF),
        1: aica_dsp.step(mwt=1, masa=0),
        3: aica_dsp.step(mrd=1, masa=1, iwt=1, iwa=0),
        6: aica_dsp.step(xsel=1, ira=0, ysel=1, zero=1, coef=0x7FFF),
        7: aica_dsp.step(ewt=1, ewa=0),
    }
    dsp = _program(steps, madrs_overrides={0: 0, 1: 5}, rbl_words=64)

    mixs = []
    for i in range(8):
        mix = [0] * 16
        if i == 0:
            mix[0] = 1000 << 4
        mixs.append(tuple(mix))

    out = dsp.run(mixs)
    assert all(sample[0] == 0 for sample in out), out


def test_temp_ring_crosses_samples():
    steps = {
        0: aica_dsp.step(xsel=1, ira=0x20, ysel=1, zero=1, coef=0x7FFF),
        1: aica_dsp.step(twt=1, twa=4),
        2: aica_dsp.step(tra=5),
        3: aica_dsp.step(ewt=1, ewa=0),
    }
    dsp = _program(steps)

    mix_write = [0] * 16
    mix_write[0] = 1000 << 4
    mix_read = [0] * 16

    out = dsp.run([tuple(mix_write), tuple(mix_read)])
    assert out[1][0] == 999


def test_zero_step_between_alu_steps_changes_acc():
    steps = {
        0: aica_dsp.step(xsel=1, ira=0x20, ysel=1, zero=1, coef=0x7FFF),
        1: aica_dsp.step(twt=1, twa=0, frcl=1),
        3: aica_dsp.step(xsel=1, ira=0x21, zero=0, bsel=1),
        4: aica_dsp.step(ewt=1, ewa=0),
    }
    dsp = _program(steps)

    mix = [0] * 16
    mix[0] = 1000 << 4
    out = dsp.run([tuple(mix)])

    acc0 = ((mix[0] << 4) * 4095) >> 12
    temp_value = acc0
    frc = temp_value >> 11
    acc_shortcut = ((temp_value * frc) >> 12) + temp_value

    expected = acc_shortcut >> 8
    expected &= 0xFFFF
    expected = expected - 0x10000 if expected & 0x8000 else expected

    naive_unchanged = acc0 >> 8

    assert out[0][0] == expected
    assert expected != 0
    assert expected != naive_unchanged
