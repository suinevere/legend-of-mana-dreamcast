#----------------------
# | test_gte_rtp_fast_host.py
# | Description: Builds tests/host/test_gte_rtp_fast.c, which includes port/psyq/gte_saturn.c, with the Saturn GTE test's generated tables, and checks the fast RTP path matches the generic one.
# | Author: suinevere
# | Dependencies: gcc or cc, saturn/tests/test_gte_host.py (prepare_tables), port/psyq/gte_saturn.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import shutil
import subprocess
import sys
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(REPO, "saturn", "tests"))

import test_gte_host


def test_gte_rtp_fast_matches_generic():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    directory = tempfile.mkdtemp(prefix="lom-gte-fast-")
    test_gte_host.prepare_tables(directory)
    psyq = os.path.join(REPO, "port", "psyq")
    exe = os.path.join(directory, "lom_gte_rtp_fast_test.exe")
    subprocess.run([cc, "-std=c11", "-O1", "-Wall", "-Wextra", "-fno-builtin",
                    "-I", psyq, "-I", os.path.join(directory, "gen"), "-I", os.path.join(REPO, "include", "sdk"),
                    "-o", exe, os.path.join(DC, "tests", "host", "test_gte_rtp_fast.c"), os.path.join(psyq, "libgte_tables.c")],
                   check=True, cwd=directory)
    result = subprocess.run([exe], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout
    assert "ok" in result.stdout
