#----------------------
# | paths.py
# | Description: Finds the repo root, or the PyInstaller bundle root when frozen, and puts the converter's module folders on sys.path.
# | Author: suinevere
# | Dependencies: port/tools, tools/splat_ext, pocketstation/tools
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

#----------------------
# | FROZEN / REPO
# | Description: Whether this runs from a PyInstaller bundle, and the root that config/us/symbols and the converter modules resolve from.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FROZEN = bool(getattr(sys, "frozen", False))
REPO = getattr(sys, "_MEIPASS", None) if FROZEN else os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))

#----------------------
# | MODULE_DIRS
# | Description: Folders holding the converter modules the kit imports, relative to REPO.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MODULE_DIRS = (os.path.join("port", "tools"), os.path.join("tools", "splat_ext"), os.path.join("pocketstation", "tools"))


#----------------------
# | setup
# | Description: Puts MODULE_DIRS (source run) or the bundle root (frozen) on sys.path once.
# | Author: suinevere
# | Dependencies: FROZEN, REPO, MODULE_DIRS
# | Globals: sys.path
# | Params: N/A
# | Returns: N/A
#----------------------
def setup():
    for folder in ([REPO] if FROZEN else [os.path.join(REPO, d) for d in MODULE_DIRS]):
        if folder not in sys.path:
            sys.path.insert(0, folder)
