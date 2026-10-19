#----------------------
# | test_scene_sweep.py
# | Description: Flycast check that the scene sweep disc reaches Home, reads its range from the VMU and sweeps scenes 18-21 to the end, with Home ok.
# | Author: suinevere
# | Dependencies: tools/scene_sweep.py, tools/flycast_run.py, stale_image.py, lom_scene_sweep_test.cdi, tests/pad_scripts/scene_sweep.pad
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.join(DC, "tools"))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import flycast_run  # noqa: E402
import scene_sweep  # noqa: E402
import stale_image  # noqa: E402


def test_sweep_walks_a_small_range():
    if flycast_run.find_flycast() is None:
        pytest.skip("no flycast")
    stale_image.require_fresh_test_image(DC)
    image = os.path.join(DC, "lom_scene_sweep_test.cdi")
    if not os.path.exists(image):
        pytest.skip("lom_scene_sweep_test.cdi not built")
    ledger = {}
    results = scene_sweep.run_batch(image, ledger, 18, 21, 4, seconds=600)
    print(scene_sweep.report(ledger))
    assert sorted(results) == [18, 19, 20, 21], results
    assert results[19]["status"] == "ok", results
    assert sum(1 for r in results.values() if r["status"] == "ok") >= 3, results
