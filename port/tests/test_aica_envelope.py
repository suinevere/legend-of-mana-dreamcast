#----------------------
# | test_aica_envelope.py
# | Description: Tests the PS1-to-AICA envelope rate tables: monotonic, in range, committed header current, worst time error reported.
# | Author: suinevere
# | Dependencies: tools/aica_envelope.py, src/sound/aica_envelope_table.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import math
import os
import sys

TOOLS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools")
sys.path.insert(0, TOOLS)
import aica_envelope as se  # noqa: E402

PORT = os.path.normpath(os.path.join(TOOLS, ".."))


def test_tables_are_monotonic_and_in_range():
    tables, _, _ = se.build_tables()
    for name, rates in tables.items():
        assert len(rates) == 128
        assert all(0 <= r <= 31 for r in rates), name
        assert all(a >= b for a, b in zip(rates, rates[1:])), name
        assert rates[se.PS1_INFINITE_RATE] == 0


def test_fastest_rates_map_to_fast_aica_rates():
    tables, _, _ = se.build_tables()
    assert tables["aica_attack_linear"][0] >= 30
    assert tables["aica_decrease_exponential"][0] >= 28


def test_decay_levels():
    levels = se.decay_levels()
    assert levels[15] == 0 and levels[0] == 8
    assert all(a >= b for a, b in zip(levels, levels[1:]))


def test_worst_time_error_is_finite():
    _, worst, clamped = se.build_tables()
    assert math.isfinite(worst)
    print("worst envelope time error %.2fx in range, %d clamped entries" % (2.0 ** worst, clamped))


def test_committed_header_matches_generator():
    with open(os.path.join(PORT, "sound", "aica_envelope_table.h"), encoding="utf-8") as handle:
        assert handle.read() == se.header_text()


FLYCAST_AEG_ATTACK_TIME = [-1, -1, 8100.0, 6900.0, 6000.0, 4800.0, 4000.0, 3400.0, 3000.0, 2400.0, 2000.0, 1700.0, 1500.0,
                           1200.0, 1000.0, 860.0, 760.0, 600.0, 500.0, 430.0, 380.0, 300.0, 250.0, 220.0, 190.0, 150.0, 130.0, 110.0, 95.0,
                           76.0, 63.0, 55.0, 47.0, 38.0, 31.0, 27.0, 24.0, 19.0, 15.0, 13.0, 12.0, 9.4, 7.9, 6.8, 6.0, 4.7, 3.8, 3.4, 3.0, 2.4,
                           2.0, 1.8, 1.6, 1.3, 1.1, 0.93, 0.85, 0.65, 0.53, 0.44, 0.40, 0.35, 0.0, 0.0]
FLYCAST_AEG_DSR_TIME = [-1, -1, 118200.0, 101300.0, 88600.0, 70900.0, 59100.0, 50700.0, 44300.0, 35500.0, 29600.0, 25300.0, 22200.0, 17700.0,
                        14800.0, 12700.0, 11100.0, 8900.0, 7400.0, 6300.0, 5500.0, 4400.0, 3700.0, 3200.0, 2800.0, 2200.0, 1800.0, 1600.0, 1400.0, 1100.0,
                        920.0, 790.0, 690.0, 550.0, 460.0, 390.0, 340.0, 270.0, 230.0, 200.0, 170.0, 140.0, 110.0, 98.0, 85.0, 68.0, 57.0, 49.0, 43.0, 34.0,
                        28.0, 25.0, 22.0, 18.0, 14.0, 12.0, 11.0, 8.5, 7.1, 6.1, 5.4, 4.3, 3.6, 3.1]


def test_time_tables_match_flycast():
    assert se.AICA_AR_MS[2:] == FLYCAST_AEG_ATTACK_TIME[2:]
    assert se.AICA_DR_MS[2:] == FLYCAST_AEG_DSR_TIME[2:]
