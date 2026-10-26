"""----------------------
 | untaken.py
 | Description: Lists, per function, every branch probe outcome that no scripted record takes: a condition's true or false, a case label, an && or || operand. It also says whether a synthetic record takes it.
 | Author: suinevere
 | Dependencies: pocketstation/build/record/probes.txt (gen_record.py), call_compare.py (read_records), make vmu-record
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import call_compare  # noqa: E402

"""----------------------
 | PROBES / REC_DIR / ACKNOWLEDGED
 | Description: The default probe list, recording directory and acknowledged-untaken list, repo-relative.
 | Author: suinevere
 | Dependencies: gen_record.py, make vmu-record, pocketstation/src/vmu_game/untaken.txt
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""
PROBES = "pocketstation/build/record/probes.txt"
REC_DIR = "pocketstation/build/record/rec"
ACKNOWLEDGED = "pocketstation/src/vmu_game/untaken.txt"


"""----------------------
 | Outcome
 | Description: One probe outcome: the function, the probe's index, line and kind, and which outcome (true, false, or taken for a case or default label) with its path-key bit.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: name, probe, line, kind, which, bit
 | Returns: N/A
 ----------------------"""
class Outcome:
    def __init__(self, name, probe, line, kind, which, bit):
        self.name, self.probe, self.line, self.kind, self.which, self.bit = name, probe, line, kind, which, bit

    """----------------------
     | Outcome.key
     | Description: The outcome as the acknowledged list names it: function, probe index and outcome.
     | Author: suinevere
     | Dependencies: N/A
     | Globals: N/A
     | Params: N/A
     | Returns: (name, probe, which)
     ----------------------"""
    def key(self):
        return self.name, self.probe, self.which


"""----------------------
 | outcomes
 | Description: Every outcome of every probe of the named functions, from probes.txt. A case or default label has one (taken, bit 2k), any other probe two (true at 2k, false at 2k+1).
 | Author: suinevere
 | Dependencies: Outcome
 | Globals: N/A
 | Params: names -- function names; probes -- probes.txt's path
 | Returns: dict name -> list of Outcome
 ----------------------"""
def outcomes(names, probes=PROBES) -> dict:
    out = {n: [] for n in names}
    with open(probes) as f:
        for line in f:
            name, k, src, kind = line.split()
            if name not in out:
                continue
            k = int(k)
            if kind.split(":")[-1] in ("case", "default"):
                out[name].append(Outcome(name, k, int(src), kind, "taken", 2 * k))
            else:
                out[name].append(Outcome(name, k, int(src), kind, "true", 2 * k))
                out[name].append(Outcome(name, k, int(src), kind, "false", 2 * k + 1))
    return out


"""----------------------
 | untaken
 | Description: The outcomes of one function that no scripted record takes, each with whether some synthetic record takes it.
 | Author: suinevere
 | Dependencies: outcomes, call_compare.read_records
 | Globals: N/A
 | Params: name -- the function; rec_dir -- the recording directory; probes -- probes.txt's path
 | Returns: list of (Outcome, taken by a synthetic record)
 ----------------------"""
def untaken(name, rec_dir=REC_DIR, probes=PROBES) -> list:
    path = os.path.join(rec_dir, name + ".rec")
    recs = call_compare.read_records(path) if os.path.exists(path) else []
    scripted, synthetic = set(), set()
    for r in recs:
        (synthetic if r.synthetic else scripted).update(r.key_bits)
    return [(o, o.bit in synthetic) for o in outcomes([name], probes)[name] if o.bit not in scripted]


"""----------------------
 | acknowledged
 | Description: The acknowledged-untaken list: one line per outcome no record takes, FUNCTION PROBE OUTCOME REASON, with # comments and blank lines skipped.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: path -- the list's path
 | Returns: dict (name, probe, which) -> reason
 ----------------------"""
def acknowledged(path=ACKNOWLEDGED) -> dict:
    out = {}
    with open(path) as f:
        for line in f:
            if not line.strip() or line.startswith("#"):
                continue
            name, k, which, reason = line.rstrip("\n").split(" ", 3)
            out[(name, int(k), which)] = reason
    return out


"""----------------------
 | main
 | Description: CLI: untaken.py NAME...; prints NAME PROBE OUTCOME LINE KIND synthetic|none for each outcome no scripted record takes, none marking one no record takes at all.
 | Author: suinevere
 | Dependencies: untaken
 | Globals: N/A
 | Params: argv -- command-line arguments
 | Returns: process exit code
 ----------------------"""
def main(argv=None) -> int:
    p = argparse.ArgumentParser()
    p.add_argument("names", nargs="+")
    p.add_argument("--rec", default=REC_DIR)
    p.add_argument("--probes", default=PROBES)
    a = p.parse_args(argv)
    for name in a.names:
        for o, synth in untaken(name, a.rec, a.probes):
            print("%s %d %s %d %s %s" % (name, o.probe, o.which, o.line, o.kind, "synthetic" if synth else "none"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
