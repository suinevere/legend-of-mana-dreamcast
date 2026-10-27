#----------------------
# | disc_input.py
# | Description: Resolves the user's .cue or .bin to the PS1 data-track .bin and checks it is the dump the kit was made from.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import hashlib
import os
import re


#----------------------
# | DiscError
# | Description: A problem with the user's disc files, worded for the person running the kit.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class DiscError(Exception):
    pass


#----------------------
# | FILE_LINE / TRACK_LINE
# | Description: A cue sheet's FILE "name" BINARY line and TRACK nn MODE line.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
FILE_LINE = re.compile(r'^\s*FILE\s+"?(.+?)"?\s+BINARY\s*$', re.I)
TRACK_LINE = re.compile(r"^\s*TRACK\s+\d+\s+(\S+)", re.I)


#----------------------
# | resolve
# | Description: The absolute path of the first data track's .bin named by a cue sheet, or the .bin itself.
# | Author: suinevere
# | Dependencies: FILE_LINE, TRACK_LINE, DiscError
# | Globals: N/A
# | Params: path -- the user's .cue or .bin, possibly quoted
# | Returns: str
#----------------------
def resolve(path):
    path = os.path.abspath(path.strip().strip('"'))
    if not os.path.isfile(path):
        raise DiscError("%s does not exist" % path)
    ext = os.path.splitext(path)[1].lower()
    if ext == ".bin":
        return path
    if ext != ".cue":
        raise DiscError("give the kit the game's .cue or .bin, not %s" % os.path.basename(path))
    current = None
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            match = FILE_LINE.match(line)
            if match:
                current = os.path.join(os.path.dirname(path), match.group(1))
                continue
            match = TRACK_LINE.match(line)
            if match and current and match.group(1).upper() != "AUDIO":
                if not os.path.isfile(current):
                    raise DiscError("the cue sheet names %s, which is not next to it" % os.path.basename(current))
                return current
    raise DiscError("%s names no data track" % os.path.basename(path))


#----------------------
# | sha1
# | Description: The file's SHA-1, reading 8 MB at a time and reporting progress.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: path -- file, progress -- optional callable(done, total)
# | Returns: hex str
#----------------------
def sha1(path, progress=None):
    total = os.path.getsize(path)
    digest = hashlib.sha1()
    done = 0
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(8 << 20), b""):
            digest.update(chunk)
            done += len(chunk)
            if progress:
                progress(done, total)
    return digest.hexdigest()


#----------------------
# | check
# | Description: Raises DiscError naming both SHA-1s when the .bin is not the dump the kit expects.
# | Author: suinevere
# | Dependencies: sha1, DiscError
# | Globals: N/A
# | Params: path -- data-track .bin, expected -- hex SHA-1 from the manifest, progress -- optional callable(done, total)
# | Returns: N/A
#----------------------
def check(path, expected, progress=None):
    actual = sha1(path, progress)
    if actual != expected:
        raise DiscError("%s has SHA-1 %s, but the kit needs Legend of Mana (USA) SLUS-01013 with SHA-1 %s; make a clean redump-style dump of your disc and try again"
                        % (os.path.basename(path), actual, expected))
