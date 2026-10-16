#----------------------
# | test_stale_image_host.py
# | Description: Host tests for stale_image.require_fresh_test_image against temp files with controlled mtimes, no Flycast or build needed.
# | Author: suinevere
# | Dependencies: stale_image.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys
import tempfile

import pytest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import stale_image


def make_tree(tmp_path):
    for rel in ("src", os.path.join("tests", "pad_scripts")):
        os.makedirs(os.path.join(str(tmp_path), rel), exist_ok=True)
    os.makedirs(os.path.join(str(tmp_path), "..", "port"), exist_ok=True)


def touch(path, mtime):
    with open(path, "wb") as f:
        f.write(b"x")
    os.utime(path, (mtime, mtime))


def test_fails_when_elf_missing(tmp_path):
    make_tree(tmp_path)
    touch(os.path.join(str(tmp_path), "lom_test.cdi"), 1000)
    with pytest.raises(AssertionError):
        stale_image.require_fresh_test_image(str(tmp_path))


def test_fails_when_cdi_missing(tmp_path):
    make_tree(tmp_path)
    touch(os.path.join(str(tmp_path), "lom_test.elf"), 1000)
    with pytest.raises(AssertionError):
        stale_image.require_fresh_test_image(str(tmp_path))


def test_fails_when_elf_predates_a_source_file(tmp_path):
    make_tree(tmp_path)
    touch(os.path.join(str(tmp_path), "lom_test.elf"), 1000)
    touch(os.path.join(str(tmp_path), "lom_test.cdi"), 1000)
    touch(os.path.join(str(tmp_path), "src", "stale.c"), 2000)
    with pytest.raises(AssertionError):
        stale_image.require_fresh_test_image(str(tmp_path))


def test_fails_when_elf_predates_a_port_file(tmp_path):
    make_tree(tmp_path)
    touch(os.path.join(str(tmp_path), "lom_test.elf"), 1000)
    touch(os.path.join(str(tmp_path), "lom_test.cdi"), 1000)
    touch(os.path.join(str(tmp_path), "..", "port", "stale.c"), 2000)
    with pytest.raises(AssertionError):
        stale_image.require_fresh_test_image(str(tmp_path))


def test_fails_when_elf_predates_a_pad_script(tmp_path):
    make_tree(tmp_path)
    touch(os.path.join(str(tmp_path), "lom_test.elf"), 1000)
    touch(os.path.join(str(tmp_path), "lom_test.cdi"), 1000)
    touch(os.path.join(str(tmp_path), "tests", "pad_scripts", "stale.pad"), 2000)
    with pytest.raises(AssertionError):
        stale_image.require_fresh_test_image(str(tmp_path))


def test_returns_cdi_path_when_elf_is_newer(tmp_path):
    make_tree(tmp_path)
    touch(os.path.join(str(tmp_path), "src", "fresh.c"), 1000)
    touch(os.path.join(str(tmp_path), "lom_test.cdi"), 2000)
    touch(os.path.join(str(tmp_path), "lom_test.elf"), 2000)
    image = stale_image.require_fresh_test_image(str(tmp_path))
    assert image == os.path.join(str(tmp_path), "lom_test.cdi")
