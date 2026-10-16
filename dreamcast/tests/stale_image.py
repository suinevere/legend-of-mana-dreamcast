#----------------------
# | stale_image.py
# | Description: Fails (not skips) a Flycast test when lom_test.elf predates any file under dreamcast/src, port/ or dreamcast/tests/pad_scripts.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os

#----------------------
# | WATCHED_DIRS
# | Description: Directories, relative to dreamcast/, whose newest file's mtime lom_test.elf must not predate.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
WATCHED_DIRS = ("src", os.path.join("..", "port"), os.path.join("tests", "pad_scripts"))


#----------------------
# | newest_mtime
# | Description: The newest modification time of any file under the given directories, walked recursively; missing directories are skipped.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: dc_root -- dreamcast/ directory, rel_dirs -- directories relative to dc_root to walk
# | Returns: newest mtime found, or 0.0 if none
#----------------------
def newest_mtime(dc_root, rel_dirs):
    newest = 0.0
    for rel in rel_dirs:
        base = os.path.normpath(os.path.join(dc_root, rel))
        if not os.path.isdir(base):
            continue
        for dirpath, _dirnames, filenames in os.walk(base):
            for name in filenames:
                path = os.path.join(dirpath, name)
                try:
                    mtime = os.path.getmtime(path)
                except OSError:
                    continue
                if mtime > newest:
                    newest = mtime
    return newest


#----------------------
# | require_fresh_test_image
# | Description: Returns the path to lom_test.cdi, raising AssertionError if it or lom_test.elf is missing or if lom_test.elf predates WATCHED_DIRS.
# | Author: suinevere
# | Dependencies: newest_mtime, WATCHED_DIRS
# | Globals: N/A
# | Params: dc_root -- dreamcast/ directory
# | Returns: path to dreamcast/lom_test.cdi
#----------------------
def require_fresh_test_image(dc_root):
    elf = os.path.join(dc_root, "lom_test.elf")
    cdi = os.path.join(dc_root, "lom_test.cdi")
    if not os.path.exists(elf) or not os.path.exists(cdi):
        raise AssertionError("lom_test.elf/cdi not built; run dreamcast\\compile.bat test-image")
    newest = newest_mtime(dc_root, WATCHED_DIRS)
    if os.path.getmtime(elf) < newest:
        raise AssertionError("lom_test.elf is older than dreamcast/src, port/ or dreamcast/tests/pad_scripts; "
                              "rebuild with dreamcast\\compile.bat test-image")
    return cdi
