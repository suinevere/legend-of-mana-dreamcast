"""----------------------
 | test_hooks.py
 | Description: Proves the hook harness: every call of a right C replacement matches its ARM across all scripts, a wrong one is caught at its first call, call-outs to ARM and kernel calls match.
 | Author: suinevere
 | Dependencies: pocketstation/runner/Makefile, lom-pocket image, pocketstation/tools/psk_split.py, psk_trace.py, gen_hooks.py, tests/hooks/*
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------"""

import glob
import os
import re
import shutil
import subprocess
import sys

import pytest

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(REPO, "pocketstation", "tools"))
import gen_hooks  # noqa: E402
import psk_file  # noqa: E402
import psk_split  # noqa: E402
import psk_trace  # noqa: E402

RES = os.path.join(REPO, psk_file.RES_PATH)
SCRIPTS = sorted(os.path.basename(p)[:-4] for p in glob.glob(os.path.join(HERE, "scripts", "*.pks")))
REFERENCE = os.path.join(REPO, "pocketstation", "build", "reference")
COVERAGE = os.path.join(REPO, "pocketstation", "build", "coverage")

pytestmark = [
    pytest.mark.skipif(shutil.which("docker") is None, reason="docker not available"),
    pytest.mark.skipif(not os.path.exists(RES), reason="RES1506.BIN not extracted"),
    pytest.mark.skipif(not glob.glob(os.path.join(REFERENCE, "*.pktr")), reason="no reference traces; run test_reference_traces.py --regen"),
]


def docker(*args):
    return subprocess.run(["docker", "run", "--rm", "-v", REPO + ":/work", "-w", "/work", "lom-pocket", *args],
                          capture_output=True, text=True)


def build(tag, hooks, *csrcs):
    r = docker("make", "-C", "pocketstation", "vendor", "kernel")
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-2000:]
    r = docker("make", "-C", "pocketstation/runner", "TAG=-" + tag, "HOOKS=../tests/hooks/" + hooks,
               "CSRCS=" + " ".join("../tests/hooks/" + c for c in csrcs))
    assert r.returncode == 0, r.stdout[-2000:] + r.stderr[-2000:]
    return "pocketstation/build/pkrun-" + tag


def run(binary, script, *extra):
    trace = "pocketstation/build/test/%s-%s.pktr" % (os.path.basename(binary), script)
    r = docker(binary, "--kernel", "pocketstation/build/openpsk-lom.bin", "--card", "pocketstation/build/rrl.card",
               "--script", "pocketstation/tests/scripts/%s.pks" % script, "--trace", trace, "--max-seconds", "600", "--idle-time",
               *extra)
    return r, trace


def compare(script, trace):
    return psk_trace.compare(psk_trace.read(os.path.join(REFERENCE, script + ".pktr")),
                             psk_trace.read(os.path.join(REPO, trace)))


def all_verified(binary):
    verified = 0
    for script in SCRIPTS:
        r, trace = run(binary, script, "--verify")
        assert r.returncode == 0, (script, [l for l in r.stdout.splitlines() if "verify" in l or "FATAL" in l][-3:])
        assert compare(script, trace) is None, script
        verified += int(re.search(r"PSK: verified calls (\d+)", r.stdout).group(1))
    assert verified > 0


@pytest.fixture(scope="module")
def split():
    with open(RES, "rb") as f:
        psk = psk_file.parse(f.read())
    cov = set()
    for p in glob.glob(os.path.join(COVERAGE, "*.txt")):
        with open(p) as f:
            cov.update(int(x, 16) for x in f.read().split())
    return psk_split.split(psk, cov), cov


def returns_through_lr(start):
    with open(os.path.join(REPO, "pocketstation", "build", "asm", "func_%08X.s" % start)) as f:
        last = f.read().strip().splitlines()[-1]
    return re.search(r"\b(bx\s+lr|mov\s+pc,\s*lr)\b", last) is not None


def test_good_leaf_is_the_smallest_covered_leaf(split):
    s, cov = split
    plain = {c & ~1 for c in cov}
    leaves = [f for f in s.functions.values()
              if not f.calls and not f.swis and not f.io and f.start in plain and returns_through_lr(f.start)]
    leaves.sort(key=lambda f: (f.end - f.start, f.start))
    with open(os.path.join(HERE, "hooks", "test_hooks_good.c")) as f:
        assert "rrl_func_%08X" % leaves[0].start in f.read()


def test_good_replacement_keeps_every_trace():
    all_verified(build("good", "good.txt", "test_hooks_good.c"))


def test_bad_replacement_is_caught_at_its_first_call(split):
    _, cov = split
    binary = build("bad", "bad.txt", "test_hooks_bad.c")
    caught = 0
    for script in SCRIPTS:
        with open(os.path.join(COVERAGE, script + ".txt")) as f:
            if "020028E9" not in f.read().split():
                continue
        r, _ = run(binary, script, "--verify")
        first = int(re.search(r"PSK: hook 020028E8 first call sample (\d+)", r.stdout).group(1))
        m = re.search(r"PSK: verify 020028E8 mismatch sample (\d+): (.*)", r.stdout)
        assert r.returncode == 6 and m and int(m.group(1)) == first, (script, first, r.stdout[-500:])
        caught += 1
    assert caught


def test_call_out_to_arm_keeps_every_trace(split):
    s, cov = split
    plain = {c & ~1 for c in cov}
    order = [a for a in psk_split.leaves_first(s) if len({c & ~1 for c in s.functions[a].calls}) == 1 and a in plain]
    assert order[0] == 0x02008294
    all_verified(build("callout", "callout.txt", "test_hooks_callout.c"))


def test_frame_wait_in_c_keeps_every_trace():
    all_verified(build("frame", "frame.txt", "test_hooks_frame.c"))


def test_psk_swi_returns_what_the_kernel_returns():
    binary = build("good", "good.txt", "test_hooks_good.c")
    with open(os.path.join(REPO, "pocketstation", "build", "swi_census.txt")) as f:
        pc = re.search(r"pc=([0-9A-F]{8})", f.readline()).group(1)
    arm, t1 = run(binary, "boot", "--swi-probe", pc)
    c, t2 = run(binary, "boot", "--swi-probe-c", pc)
    va = re.search(r"PSK: swi-probe %s r0=([0-9A-F]{8})" % pc, arm.stdout).group(1)
    vc = re.search(r"PSK: swi-probe %s r0=([0-9A-F]{8})" % pc, c.stdout).group(1)
    assert va == vc
    assert compare("boot", t2) is None


def test_gen_hooks_refuses_stack_arguments():
    with pytest.raises(ValueError):
        gen_hooks.parse("02000000 f 5 u32\n")
    with pytest.raises(ValueError):
        gen_hooks.parse("02000000 f regs\n02000000 g regs\n")
    assert "thunk_02000000" in gen_hooks.generate(gen_hooks.parse("02000000 f 2 u32\n"))
