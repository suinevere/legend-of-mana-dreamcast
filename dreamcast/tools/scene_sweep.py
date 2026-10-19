#----------------------
# | scene_sweep.py
# | Description: Runs the FIELD scene sweep on Flycast in batches, choosing each batch's range through a VMU file, resuming one scene past any halt, and keeping a JSON ledger of every scene's outcome and worst frame-probe rate.
# | Author: suinevere
# | Dependencies: flycast_run.py, vmu_image.py
# | Globals: N/A
# | Params: --ledger PATH [--first N] [--last M] [--batch K] [--runs R] [--seconds S] [--image PATH] [--report]
# | Returns: 0
#----------------------
import argparse
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import vmu_image

#----------------------
# | SWEEP_FILE
# | Description: VMU file name scene_sweep_kos.c reads the range from.
# | Author: suinevere
# | Dependencies: dreamcast/src/system/scene_sweep_kos.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SWEEP_FILE = "LOMSWEEP"

#----------------------
# | SLOW_VBL
# | Description: Worst probe rate above which report lists a scene as slow (Home holds 2.00-2.07).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SLOW_VBL = 2.2

#----------------------
# | SCENE_FIRST / SCENE_LAST
# | Description: The scene ids on the US disc, matching SCENE_SWEEP_FIRST/LAST in scene_sweep.h.
# | Author: suinevere
# | Dependencies: dreamcast/src/system/scene_sweep.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SCENE_FIRST = 0
SCENE_LAST = 929

#----------------------
# | DEFAULT_IMAGE
# | Description: The scripted disc whose PADSEL.TXT picks scene_sweep.pad.
# | Author: suinevere
# | Dependencies: dreamcast/makefile (PADSEL_SCRIPTS)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DEFAULT_IMAGE = os.path.join(os.path.dirname(HERE), "lom_scene_sweep_test.cdi")

#----------------------
# | JUMP_LINE / ARRIVE_LINE / OK_LINE / STUCK_LINE / PROBE_LINE / HALT_TEXT
# | Description: The log lines the sweep, the frame probe and dc_halt print.
# | Author: suinevere
# | Dependencies: field_watch_unit.c, frame_probe.c, dc_system.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
JUMP_LINE = re.compile(r"LOM: jump scene (\d+) spawn")
ARRIVE_LINE = re.compile(r"LOM: sweep arrive (\d+)")
OK_LINE = re.compile(r"LOM: sweep ok (\d+)")
STUCK_LINE = re.compile(r"LOM: sweep stuck (\d+) at (-?\d+)")
PROBE_LINE = re.compile(r"LOM: probe vbl (\d+\.\d+)")
HALT_TEXT = "LOM HALT:"


#----------------------
# | vmu_payload
# | Description: A blank VMU image holding the range as the LOMSWEEP file.
# | Author: suinevere
# | Dependencies: vmu_image.blank, vmu_image.add
# | Globals: SWEEP_FILE
# | Params: first, last -- scene range
# | Returns: VMU image bytes
#----------------------
def vmu_payload(first, last):
    return vmu_image.add(vmu_image.blank(), SWEEP_FILE, b"%d %d\n" % (first, last))


#----------------------
# | parse_run
# | Description: Turns one run's log into per-scene results: each jump opens a scene, ok/stuck/halt closes it, and its worst probe rate is taken after its arrival, skipping the first window.
# | Author: suinevere
# | Dependencies: JUMP_LINE, ARRIVE_LINE, OK_LINE, STUCK_LINE, PROBE_LINE, HALT_TEXT
# | Globals: N/A
# | Params: lines -- log lines, first, last -- the range swept
# | Returns: {scene: {"status", "vbl_max", "detail"}} for scenes opened inside the range
#----------------------
def parse_run(lines, first, last):
    results = {}
    current = None
    arrived = False
    skipped = False
    worst = None

    def close(status, detail):
        if current is not None and first <= current <= last:
            results[current] = {"status": status, "vbl_max": worst, "detail": detail}

    for line in lines:
        m = JUMP_LINE.search(line)
        if m:
            current, arrived, skipped, worst = int(m.group(1)), False, False, None
            continue
        if current is None:
            continue
        if HALT_TEXT in line:
            close("halt", line[line.index(HALT_TEXT):].strip())
            current = None
            continue
        m = ARRIVE_LINE.search(line)
        if m and int(m.group(1)) == current:
            arrived = True
            continue
        m = PROBE_LINE.search(line)
        if m and arrived:
            if not skipped:
                skipped = True
            else:
                rate = float(m.group(1))
                worst = rate if worst is None or rate > worst else worst
            continue
        m = OK_LINE.search(line)
        if m and int(m.group(1)) == current:
            close("ok", "")
            current = None
            continue
        m = STUCK_LINE.search(line)
        if m and int(m.group(1)) == current:
            close("stuck", "at %s" % m.group(2))
            current = None
    return results


#----------------------
# | open_scene
# | Description: The scene a run had jumped to but not closed when it ended.
# | Author: suinevere
# | Dependencies: JUMP_LINE, parse_run
# | Globals: N/A
# | Params: lines -- log lines, first, last -- the range swept
# | Returns: scene id, or None
#----------------------
def open_scene(lines, first, last):
    jumped = [int(m.group(1)) for m in (JUMP_LINE.search(line) for line in lines) if m]
    closed = parse_run(lines, first, last)
    if jumped and first <= jumped[-1] <= last and jumped[-1] not in closed:
        return jumped[-1]
    return None


#----------------------
# | next_first
# | Description: The lowest scene in the range the ledger has no result for.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: ledger -- {scene: result}, first, last -- range
# | Returns: scene id, or None when the range is done
#----------------------
def next_first(ledger, first, last):
    for scene in range(first, last + 1):
        if scene not in ledger:
            return scene
    return None


#----------------------
# | run_batch
# | Description: Runs one Flycast batch from the first unswept scene and merges its results into the ledger; a run that times out with a scene open records it as halted on timeout.
# | Author: suinevere
# | Dependencies: next_first, vmu_payload, parse_run, open_scene, flycast_run.run_flycast
# | Globals: N/A
# | Params: image -- scripted disc, ledger -- {scene: result} updated in place, first, last -- overall range, batch -- scenes per run, seconds -- run time limit
# | Returns: the new results, or {} when the range is done
#----------------------
def run_batch(image, ledger, first, last, batch, seconds):
    import flycast_run

    start = next_first(ledger, first, last)
    if start is None:
        return {}
    end = min(start + batch - 1, last)
    code, lines = flycast_run.run_flycast(image, seconds, until="LOM: sweep done", vmu_in=vmu_payload(start, end))
    results = parse_run(lines, start, end)
    if code != 0:
        stuck = open_scene(lines, start, end)
        if stuck is not None:
            results[stuck] = {"status": "halt", "vbl_max": None, "detail": "timeout" if code == 3 else "halt"}
    if not results:
        results[start] = {"status": "halt", "vbl_max": None,
                          "detail": "no scene reached (code %d): %s" % (code, " | ".join(lines[-3:]))}
    ledger.update(results)
    return results


#----------------------
# | report
# | Description: Summarises a ledger: counts by status, then every halt, stuck and slow scene with its detail.
# | Author: suinevere
# | Dependencies: SLOW_VBL
# | Globals: N/A
# | Params: ledger -- {scene: result}
# | Returns: report text
#----------------------
def report(ledger):
    counts = {}
    for result in ledger.values():
        counts[result["status"]] = counts.get(result["status"], 0) + 1
    out = ["scenes %d: %s" % (len(ledger), ", ".join("%s %d" % kv for kv in sorted(counts.items())))]
    for scene in sorted(ledger):
        result = ledger[scene]
        if result["status"] in ("halt", "stuck"):
            out.append("%s %d: %s" % (result["status"], scene, result["detail"]))
    for scene in sorted(ledger):
        rate = ledger[scene]["vbl_max"]
        if rate is not None and rate > SLOW_VBL:
            out.append("slow %d: %.2f" % (scene, rate))
    return "\n".join(out)


#----------------------
# | load_ledger / save_ledger
# | Description: Reads and writes the ledger JSON, keyed by scene id as text.
# | Author: suinevere
# | Dependencies: json
# | Globals: N/A
# | Params: path -- ledger file, ledger -- {scene: result}
# | Returns: load_ledger: {scene: result}; save_ledger: N/A
#----------------------
def load_ledger(path):
    if not os.path.exists(path):
        return {}
    with open(path) as f:
        return {int(k): v for k, v in json.load(f).items()}


def save_ledger(path, ledger):
    folder = os.path.dirname(os.path.abspath(path))
    os.makedirs(folder, exist_ok=True)
    with open(path, "w") as f:
        json.dump({str(k): ledger[k] for k in sorted(ledger)}, f, indent=1)


#----------------------
# | main
# | Description: Runs up to --runs batches into the ledger, saving after each, then prints the report.
# | Author: suinevere
# | Dependencies: load_ledger, run_batch, save_ledger, report
# | Globals: DEFAULT_IMAGE, SCENE_FIRST, SCENE_LAST
# | Params: argv -- command line
# | Returns: 0
#----------------------
def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--ledger", required=True)
    parser.add_argument("--first", type=int, default=SCENE_FIRST)
    parser.add_argument("--last", type=int, default=SCENE_LAST)
    parser.add_argument("--batch", type=int, default=40)
    parser.add_argument("--runs", type=int, default=1)
    parser.add_argument("--seconds", type=int, default=600)
    parser.add_argument("--image", default=DEFAULT_IMAGE)
    parser.add_argument("--report", action="store_true")
    args = parser.parse_args(argv)
    ledger = load_ledger(args.ledger)
    for _ in range(0 if args.report else args.runs):
        results = run_batch(args.image, ledger, args.first, args.last, args.batch, args.seconds)
        save_ledger(args.ledger, ledger)
        if not results:
            break
        print("swept %d-%d" % (min(results), max(results)), flush=True)
    print(report(ledger))
    return 0


if __name__ == "__main__":
    sys.exit(main())
