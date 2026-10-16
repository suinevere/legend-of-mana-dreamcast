#----------------------
# | test_port_coverage.py
# | Description: Every port/**/*.c file must be in the Dreamcast makefile's PORT_SOURCES or in SATURN_ONLY below, so a new port file can't silently join only one backend.
# | Author: suinevere
# | Dependencies: dreamcast/makefile
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import re

PORT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.normpath(os.path.join(PORT, ".."))
DREAMCAST_DIR = os.path.join(REPO, "dreamcast")
DREAMCAST_MAKEFILE = os.path.join(DREAMCAST_DIR, "makefile")

SATURN_ONLY = {"boot/layout_pack.c"}


def _dreamcast_port_sources():
    with open(DREAMCAST_MAKEFILE, "r", encoding="utf-8") as f:
        text = f.read()
    match = re.search(r"^PORT_SOURCES\s*=\s*(.*?)(?<!\\)\n", text, re.MULTILINE | re.DOTALL)
    assert match, "dreamcast/makefile has no PORT_SOURCES assignment"
    raw = match.group(1).replace("\\\n", " ")
    sources = set()
    for token in raw.split():
        assert token.startswith("../port/"), f"unexpected PORT_SOURCES entry: {token}"
        sources.add(os.path.normpath(os.path.join(DREAMCAST_DIR, token)))
    return sources


def _all_port_sources():
    found = set()
    for root, _, files in os.walk(PORT):
        for name in files:
            if name.endswith(".c"):
                found.add(os.path.normpath(os.path.join(root, name)))
    return found


def test_every_port_source_is_tracked_for_dreamcast():
    dreamcast_sources = _dreamcast_port_sources()
    excluded = {os.path.normpath(os.path.join(PORT, rel)) for rel in SATURN_ONLY}
    untracked = sorted(
        path for path in _all_port_sources()
        if path not in dreamcast_sources and path not in excluded
    )
    assert not untracked, (
        "port/ sources missing from dreamcast/makefile's PORT_SOURCES and not "
        "declared Saturn-only in SATURN_ONLY: " + ", ".join(untracked)
    )


def test_saturn_only_entries_still_exist():
    for rel in SATURN_ONLY:
        path = os.path.normpath(os.path.join(PORT, rel))
        assert os.path.isfile(path), f"SATURN_ONLY lists a file that no longer exists: {rel}"
