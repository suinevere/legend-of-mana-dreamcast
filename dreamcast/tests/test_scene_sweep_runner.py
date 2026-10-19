#----------------------
# | test_scene_sweep_runner.py
# | Description: Host checks for the scene sweep runner: the VMU range payload, the log parser's ok/stuck/halt cases and probe windows, the resume point and the report.
# | Author: suinevere
# | Dependencies: tools/scene_sweep.py, tools/vmu_image.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))

import scene_sweep
import vmu_image


def lines(*items):
    return ["LOM: " + item if not item.startswith("LOM HALT") else item for item in items]


def test_vmu_payload_holds_the_range():
    files = dict(vmu_image.files(scene_sweep.vmu_payload(20, 60)))
    assert files[scene_sweep.SWEEP_FILE].startswith(b"20 60\n")


def test_parse_ok_takes_the_worst_probe_after_the_first_window():
    got = scene_sweep.parse_run(lines("jump scene 5 spawn 0", "sweep arrive 5", "probe vbl 2.00 prims 1",
                                      "probe vbl 2.30 prims 1", "sweep ok 5"), 5, 5)
    assert got[5]["status"] == "ok"
    assert got[5]["vbl_max"] == 2.30


def test_parse_skips_probes_before_arrival_and_the_first_after():
    got = scene_sweep.parse_run(lines("jump scene 6 spawn 0", "probe vbl 9.99 prims 1", "sweep arrive 6",
                                      "probe vbl 2.50 prims 1", "probe vbl 2.10 prims 1", "sweep ok 6"), 6, 6)
    assert got[6]["vbl_max"] == 2.10


def test_parse_stuck_keeps_the_scene_reached():
    got = scene_sweep.parse_run(lines("jump scene 7 spawn 0", "sweep stuck 7 at 19"), 7, 7)
    assert got[7]["status"] == "stuck"
    assert got[7]["detail"] == "at 19"


def test_parse_halt_blames_the_open_scene():
    got = scene_sweep.parse_run(lines("jump scene 8 spawn 0", "sweep arrive 8", "LOM HALT: pending func_8001234"), 8, 9)
    assert got[8]["status"] == "halt"
    assert "pending func_8001234" in got[8]["detail"]
    assert 9 not in got


def test_parse_halt_before_arrival_blames_the_new_scene():
    got = scene_sweep.parse_run(lines("jump scene 8 spawn 0", "sweep arrive 8", "sweep ok 8", "jump scene 9 spawn 0",
                                      "LOM HALT: exception"), 8, 9)
    assert got[8]["status"] == "ok"
    assert got[9]["status"] == "halt"


def test_parse_ignores_jumps_outside_the_range():
    got = scene_sweep.parse_run(lines("jump scene 19 spawn 0", "sweep ok 19"), 0, 5)
    assert got == {}


def test_next_first_finds_the_first_gap():
    done = {"status": "ok", "vbl_max": None, "detail": ""}
    assert scene_sweep.next_first({0: done, 1: done, 3: done}, 0, 5) == 2
    assert scene_sweep.next_first({0: done, 1: done}, 0, 1) is None


def test_report_lists_halts_stucks_and_slow_scenes():
    ledger = {1: {"status": "halt", "vbl_max": None, "detail": "LOM HALT: x"},
              2: {"status": "stuck", "vbl_max": None, "detail": "at 19"},
              3: {"status": "ok", "vbl_max": 2.5, "detail": ""},
              4: {"status": "ok", "vbl_max": 2.0, "detail": ""}}
    text = scene_sweep.report(ledger)
    assert "halt 1" in text and "LOM HALT: x" in text
    assert "stuck 2" in text and "at 19" in text
    assert "slow 3" in text and "slow 4" not in text
