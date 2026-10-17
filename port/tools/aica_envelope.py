#----------------------
# | aica_envelope.py
# | Description: Maps PS1 SPU ADSR rates to the AICA envelope rate with the nearest phase time in log space and generates the C table header.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import math
import os
import sys

#----------------------
# | PS1_RATE_HZ
# | Description: The SPU envelope ticks once per output sample at this rate.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PS1_RATE_HZ = 44100

#----------------------
# | PS1_INFINITE_RATE
# | Description: The 7-bit PS1 envelope rate that never changes the level.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PS1_INFINITE_RATE = 0x7F

#----------------------
# | AICA_AR_MS
# | Description: AICA attack times in ms by effective rate 0-63 (Flycast core/hw/aica/sgc_if.cpp AEG_Attack_Time, the same values as the SCSP's); rates 0-1 never move.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
AICA_AR_MS = [math.inf, math.inf, 8100.0, 6900.0, 6000.0, 4800.0, 4000.0, 3400.0, 3000.0, 2400.0, 2000.0, 1700.0, 1500.0,
              1200.0, 1000.0, 860.0, 760.0, 600.0, 500.0, 430.0, 380.0, 300.0, 250.0, 220.0, 190.0, 150.0, 130.0, 110.0, 95.0,
              76.0, 63.0, 55.0, 47.0, 38.0, 31.0, 27.0, 24.0, 19.0, 15.0, 13.0, 12.0, 9.4, 7.9, 6.8, 6.0, 4.7, 3.8, 3.4, 3.0, 2.4,
              2.0, 1.8, 1.6, 1.3, 1.1, 0.93, 0.85, 0.65, 0.53, 0.44, 0.40, 0.35, 0.0, 0.0]

#----------------------
# | AICA_DR_MS
# | Description: AICA decay and release times in ms over the full 96 dB range by effective rate 0-63 (Flycast sgc_if.cpp AEG_DSR_Time); rates 0-1 never move.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
AICA_DR_MS = [math.inf, math.inf, 118200.0, 101300.0, 88600.0, 70900.0, 59100.0, 50700.0, 44300.0, 35500.0, 29600.0, 25300.0,
              22200.0, 17700.0, 14800.0, 12700.0, 11100.0, 8900.0, 7400.0, 6300.0, 5500.0, 4400.0, 3700.0, 3200.0, 2800.0, 2200.0,
              1800.0, 1600.0, 1400.0, 1100.0, 920.0, 790.0, 690.0, 550.0, 460.0, 390.0, 340.0, 270.0, 230.0, 200.0, 170.0, 140.0,
              110.0, 98.0, 85.0, 68.0, 57.0, 49.0, 43.0, 34.0, 28.0, 25.0, 22.0, 18.0, 14.0, 12.0, 11.0, 8.5, 7.1, 6.1, 5.4, 4.3,
              3.6, 3.1]

#----------------------
# | INSTANT_MS
# | Description: Stand-in time for the AICA's instant attack so it can be compared in log space.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
INSTANT_MS = 0.01

#----------------------
# | EG_RANGE_DB
# | Description: The AICA envelope's full decay range, over which AICA_DR_MS is timed.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EG_RANGE_DB = 96.0

#----------------------
# | DL_STEP_DB
# | Description: Attenuation of one AICA decay-level (DL) step.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DL_STEP_DB = 3.0


#----------------------
# | _shift_step
# | Description: Splits a 7-bit PS1 envelope rate into its cycle count and level step per the SPU's envelope algorithm.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: rate -- 0-127, increase -- True for a rising phase
# | Returns: (samples per step, absolute step)
#----------------------
def _shift_step(rate, increase):
    shift, low = rate >> 2, rate & 3
    step = (7 - low) if increase else (8 - low)
    return 1 << max(0, shift - 11), step << max(0, 11 - shift)


#----------------------
# | ps1_ms
# | Description: PS1 phase time in ms: rising phases 0 to full, linear falls full to 0, exponential falls across EG_RANGE_DB.
# | Author: suinevere
# | Dependencies: _shift_step, PS1_RATE_HZ, EG_RANGE_DB
# | Globals: N/A
# | Params: rate -- 0-127, increase -- rising phase, exponential -- exponential mode
# | Returns: float ms, inf for PS1_INFINITE_RATE
#----------------------
def ps1_ms(rate, increase, exponential):
    if rate == PS1_INFINITE_RATE:
        return math.inf
    cycles, step = _shift_step(rate, increase)
    if increase and exponential:
        samples = math.ceil(0x6000 / step) * cycles + math.ceil((0x7FFF - 0x6000) / step) * cycles * 4
    elif increase or not exponential:
        samples = math.ceil(0x7FFF / step) * cycles
    else:
        db_per_step = -20.0 * math.log10(max(1.0 - step / 32768.0, 1e-9))
        samples = cycles * EG_RANGE_DB / db_per_step
    return 1000.0 * samples / PS1_RATE_HZ


#----------------------
# | aica_ms
# | Description: AICA phase time in ms for a 5-bit rate register with key scaling off.
# | Author: suinevere
# | Dependencies: AICA_AR_MS, AICA_DR_MS, INSTANT_MS
# | Globals: N/A
# | Params: rate -- 0-31, attack -- True for AR, False for D1R/D2R/RR
# | Returns: float ms
#----------------------
def aica_ms(rate, attack):
    ms = (AICA_AR_MS if attack else AICA_DR_MS)[2 * rate]
    return INSTANT_MS if ms == 0.0 else ms


#----------------------
# | nearest_rate
# | Description: The AICA rate whose time is nearest a PS1 time in log space; an infinite PS1 time maps to rate 0.
# | Author: suinevere
# | Dependencies: aica_ms
# | Globals: N/A
# | Params: ms -- PS1 phase time, attack -- True for an attack rate
# | Returns: (rate 0-31, absolute log2 time error)
#----------------------
def nearest_rate(ms, attack):
    if math.isinf(ms):
        return 0, 0.0
    best = min(range(1, 32), key=lambda r: abs(math.log2(aica_ms(r, attack) / ms)))
    return best, abs(math.log2(aica_ms(best, attack) / ms))


#----------------------
# | TABLES
# | Description: (C name, rising phase, exponential mode) of each generated 128-entry rate table.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TABLES = [("aica_attack_linear", True, False), ("aica_attack_exponential", True, True),
          ("aica_decrease_linear", False, False), ("aica_decrease_exponential", False, True)]


#----------------------
# | build_tables
# | Description: Builds every rate table, the worst log2 time error over PS1 times inside the AICA's range, and how many entries clamp outside it.
# | Author: suinevere
# | Dependencies: TABLES, ps1_ms, nearest_rate, aica_ms
# | Globals: N/A
# | Params: N/A
# | Returns: (dict name -> list of 128 rates, worst in-range log2 error, clamped entry count)
#----------------------
def build_tables():
    out, worst, clamped = {}, 0.0, 0
    for name, increase, exponential in TABLES:
        rates = []
        for rate in range(128):
            ms = ps1_ms(rate, increase, exponential)
            aica, error = nearest_rate(ms, increase)
            rates.append(aica)
            if math.isinf(ms):
                continue
            if aica_ms(31, increase) <= ms <= aica_ms(1, increase):
                worst = max(worst, error)
            else:
                clamped += 1
        out[name] = rates
    return out, worst, clamped


#----------------------
# | decay_levels
# | Description: AICA DL for each 4-bit PS1 sustain level, whose amplitude is (SL + 1) / 16 of full.
# | Author: suinevere
# | Dependencies: DL_STEP_DB
# | Globals: N/A
# | Params: N/A
# | Returns: list of 16 DL values
#----------------------
def decay_levels():
    return [int(round(-20.0 * math.log10((sl + 1) / 16.0) / DL_STEP_DB)) for sl in range(16)]


#----------------------
# | header_text
# | Description: Generates aica_envelope_table.h with the rate tables and the sustain-to-DL table.
# | Author: suinevere
# | Dependencies: build_tables, decay_levels
# | Globals: N/A
# | Params: N/A
# | Returns: C header string
#----------------------
def header_text():
    tables, worst, clamped = build_tables()
    lines = ["/*----------------------", " | aica_envelope_table.h",
             " | Description: Generated by port/tools/aica_envelope.py: PS1 SPU 7-bit ADSR rate to nearest AICA 5-bit rate, and PS1 sustain level to AICA DL.",
             " | Author: suinevere", " | Dependencies: port/tools/aica_envelope.py", " | Globals: N/A", " | Params: N/A", " | Returns: N/A",
             " ----------------------*/", "#ifndef LOM_AICA_ENVELOPE_TABLE_H", "#define LOM_AICA_ENVELOPE_TABLE_H", "",
             "/* Worst phase time error %.2fx inside the AICA range; %d entries clamp to its fastest or slowest rate. */" % (2.0 ** worst, clamped), ""]
    for name, _, _ in TABLES:
        lines.append("static const unsigned char %s[128] = {" % name)
        rates = tables[name]
        for row in range(0, 128, 16):
            lines.append("    " + ", ".join("%2d" % r for r in rates[row:row + 16]) + ",")
        lines += ["};", ""]
    lines.append("static const unsigned char aica_decay_level[16] = {")
    lines.append("    " + ", ".join("%d" % d for d in decay_levels()) + ",")
    lines += ["};", "", "#endif", ""]
    return "\n".join(lines)


if __name__ == "__main__":
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "sound", "aica_envelope_table.h")
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(header_text())
    sys.exit(0)
