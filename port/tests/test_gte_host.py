#----------------------
# | test_gte_host.py
# | Description: Builds the host C tests for the port's software GTE with the host gcc, runs them, and checks the libgte functions against the PS1 code run by psx_oracle.
# | Author: suinevere
# | Dependencies: tests/host/test_gte.c, tests/psx_oracle.py, ../port/psyq/gte_soft.c, ../port/psyq/libgte_tables.c, port/tools/libgte_tables.py, ../include/sdk/libgte.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import collections
import os
import random
import shutil
import subprocess
import sys

import pytest

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(PORT), "port", "tools"))

import libgte_tables  # noqa: E402
import psx_oracle  # noqa: E402

INT32_MIN = -0x80000000
INT32_MAX = 0x7FFFFFFF
ANGLES = [0, 1, 2, 511, 512, 1023, 1024, 1025, 1500, 2047, 2048, 2049, 3071, 3072, 3073, 4095, 4096, 4097,
          8191, 8192, 8197, -1, -2, -1024, -2048, -3072, -4095, -4096, -4097, -12345, 0x7FFF, -0x8000,
          INT32_MAX, INT32_MIN, INT32_MIN + 1]
RATAN_PAIRS = [(0, 0), (0, 5), (5, 0), (0, -5), (-5, 0), (3, 4), (-3, 4), (3, -4), (-3, -4), (4, 3), (-4, 3),
               (4, -3), (-4, -3), (7, 7), (-7, 7), (7, -7), (-7, -7), (1, INT32_MAX), (INT32_MAX, 1),
               (-INT32_MAX, 1), (1, -INT32_MAX), (0x1FFFFF, 0x200000), (0x200000, 0x1FFFFF), (0x300000, 0x400000),
               (0x400000, 0x300000), (0x2007FE, 0x2007FF), (INT32_MAX, INT32_MAX), (INT32_MIN, 5), (5, INT32_MIN),
               (INT32_MIN, INT32_MIN), (1023, 1), (1, 1023), (100, 99), (99, 100)]
SQRT_VALUES = [0, 1, 2, 3, 4, 15, 16, 63, 64, 65, 127, 128, 255, 256, 257, 4096, 0xFFFF, 0x10000, 0x3FFFFFFF,
               0x40000000, 0x7FFFFFFE, INT32_MAX, -1, -2, -3, -64, -65, -1000, -0x10000, INT32_MIN, INT32_MIN + 1]
TRAP_LIMITS = {"VectorNormal": 10, "VectorNormalS": 10}
DIVERGENT_CASES = [("VectorNormal", -0x8000, -0x8000, -0x8000),
                   ("CompMatrix", 4096, 0, 0, 0, 4096, 0, 0, 0, 4096, 0, INT32_MAX, 0, 0,
                    4096, 0, 0, 0, 4096, 0, 0, 0, 4096, 0, 1, 0, 0, *([0] * 13)),
                   ("ratan2", 0, INT32_MIN)]
NORMAL_VECTORS = [(0, 0, 0), (1, 0, 0), (0, -1, 0), (0, 0, 1), (3, 4, 0), (-3, 0, 4), (1, 1, 1), (4096, 0, 0),
                  (0, -4096, 0), (100, 200, 300), (0x7FFF, 0, 0), (-0x8000, 0, 0), (26000, 26000, 26000),
                  (-26000, 26000, -26000), (100000, 0, 0), (0x12345678, 3, -7), (0x10000, 0x10001, -0x10000),
                  (-0x7FFF0000, 9, 9)]


def image_path():
    path = psx_oracle.default_image()
    return path if os.path.exists(path) else None


def prepare_tables(directory):
    path = image_path()
    if path is not None:
        libgte_tables.run(path, directory, libgte_tables.EXPECTED_SHA1)
        return True
    _, total = libgte_tables.layout()
    libgte_tables.write_file(os.path.join(directory, "gen", "libgte_tables.bin"), bytes(total))
    libgte_tables.write_file(os.path.join(directory, "gen", "libgte_tables.h"), libgte_tables.render_header())
    return False


def build(directory):
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    real = prepare_tables(directory)
    exe = os.path.join(directory, "lom_gte_test.exe")
    psyq = os.path.join(PORT, "..", "port", "psyq")
    cmd = [cc, "-std=c11", "-Wall", "-Wextra", "-Werror", "-fno-builtin",
           "-I", psyq, "-I", os.path.join(directory, "gen"), "-I", os.path.join(PORT, "..", "include", "sdk"),
           "-o", exe,
           os.path.join(PORT, "tests", "host", "test_gte.c"),
           os.path.join(psyq, "gte_soft.c"),
           os.path.join(psyq, "libgte_tables.c")]
    subprocess.run(cmd, check=True, cwd=directory)
    return exe, real


def random_matrix(rng, lo=-0x8000, hi=0x7FFF):
    return ([rng.randint(lo, hi) for _ in range(10)]
            + [rng.randint(INT32_MIN, INT32_MAX) if rng.random() < 0.5 else rng.randint(-0x8000, 0x7FFF)
               for _ in range(3)])


def rotation_matrix(rng):
    return [rng.choice([4096, -4096, 0, rng.randint(-4096, 4096)]) for _ in range(9)] + [rng.randint(-5, 5)] + [
        rng.randint(-100000, 100000) for _ in range(3)]


def build_cases():
    rng = random.Random(0x1013)
    cases = []
    for angle in ANGLES + [rng.randint(-50000, 50000) for _ in range(150)]:
        for name in ("rsin", "rcos", "csin", "ccos"):
            cases.append((name, angle))
    for y, x in RATAN_PAIRS + [(rng.randint(-5000, 5000), rng.randint(-5000, 5000)) for _ in range(150)] + [
            (rng.randint(INT32_MIN + 1, INT32_MAX), rng.randint(INT32_MIN + 1, INT32_MAX)) for _ in range(150)]:
        cases.append(("ratan2", y, x))
    for value in SQRT_VALUES + [rng.randint(0, INT32_MAX) for _ in range(200)] + [
            rng.randint(0, 0x10000) for _ in range(100)] + [rng.randint(INT32_MIN, -1) for _ in range(50)]:
        cases.append(("SquareRoot0", value))
    vectors = NORMAL_VECTORS + [tuple(rng.randint(-18000, 18000) for _ in range(3)) for _ in range(100)] + [
        tuple(rng.randint(-50, 50) for _ in range(3)) for _ in range(50)] + [
        tuple(rng.randint(INT32_MIN, INT32_MAX) for _ in range(3)) for _ in range(50)]
    for vector in vectors:
        cases.append(("VectorNormal",) + vector)
        cases.append(("VectorNormalS",) + vector)
    for _ in range(150):
        angles = [rng.choice(ANGLES[:-3]) if rng.random() < 0.3 else rng.randint(-0x8000, 0x7FFF) for _ in range(3)]
        start = random_matrix(rng)
        cases.append(("RotMatrix", *angles, *start))
        cases.append(("RotMatrix_gte", *angles, *start))
    for _ in range(100):
        angle = rng.choice(ANGLES) if rng.random() < 0.3 else rng.randint(-100000, 100000)
        for name in ("RotMatrixX", "RotMatrixY", "RotMatrixZ"):
            cases.append((name, angle, *(rotation_matrix(rng) if rng.random() < 0.5 else random_matrix(rng))))
    for _ in range(200):
        vector = [rng.choice([0, 1, -1, 0x7FFF, 0x8000, -0x8000, INT32_MAX, INT32_MIN, rng.randint(-100000, 100000)])
                  if rng.random() < 0.4 else rng.randint(INT32_MIN, INT32_MAX) for _ in range(3)]
        matrix = rotation_matrix(rng) if rng.random() < 0.5 else random_matrix(rng)
        cases.append(("ApplyMatrixLV", *matrix, *vector))
    for _ in range(200):
        pick = rng.random()
        m0 = rotation_matrix(rng) if pick < 0.5 else random_matrix(rng)
        m1 = rotation_matrix(rng) if pick < 0.7 else random_matrix(rng)
        cases.append(("CompMatrix", *m0, *m1, *random_matrix(rng)))
    for _ in range(200):
        vector = [rng.choice([4096, 0, -4096, 2048, INT32_MAX, INT32_MIN, rng.randint(-0x10000, 0x10000)])
                  if rng.random() < 0.5 else rng.randint(INT32_MIN, INT32_MAX) for _ in range(3)]
        cases.append(("ScaleMatrix", *random_matrix(rng), *vector))
    return cases


def test_gte_host_suite(tmp_path):
    exe, _ = build(str(tmp_path))
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "OK" in result.stdout


def test_libgte_functions_match_the_ps1_code(tmp_path):
    if image_path() is None:
        pytest.skip("LOM_PS1_IMAGE (or disc/us/LegendOfMana.bin) is absent; the libgte tables and PS1 code come from it")
    exe, real = build(str(tmp_path))
    assert real
    machine = psx_oracle.Machine(psx_oracle.load_exe(image_path()))
    cases = []
    expected = []
    traps = collections.Counter()
    for case in build_cases():
        try:
            expected.append(machine.run_case(case))
        except psx_oracle.PS1Exception:
            traps[case[0]] += 1
            continue
        cases.append(case)
    for name, count in traps.items():
        assert count <= TRAP_LIMITS.get(name, 0), "%d %s cases trap on the PS1" % (count, name)
    stdin = "".join(" ".join(str(x) for x in case) + "\n" for case in cases)
    result = subprocess.run([exe, "--cases"], input=stdin, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout[-2000:]
    lines = result.stdout.splitlines()
    assert len(lines) == len(cases)
    mismatches = []
    for case, want, line in zip(cases, expected, lines):
        got = [int(x) for x in line.split()]
        if got != want:
            mismatches.append("%s\n  ps1    %s\n  port   %s" % (" ".join(map(str, case)), want, got))
    assert not mismatches, "%d of %d cases differ:\n%s" % (len(mismatches), len(cases), "\n".join(mismatches[:15]))


def test_divergent_inputs_trap_on_the_ps1():
    if image_path() is None:
        pytest.skip("LOM_PS1_IMAGE (or disc/us/LegendOfMana.bin) is absent; the PS1 code comes from it")
    machine = psx_oracle.Machine(psx_oracle.load_exe(image_path()))
    for case in DIVERGENT_CASES:
        with pytest.raises(psx_oracle.PS1Exception):
            machine.run_case(case)
