#----------------------
# | fill.py
# | Description: User side of the release kit: converts the user's PS1 disc, writes each hole back into the template and re-encodes every track03 sector, then checks every track's SHA-1 before the GDI folder appears.
# | Author: suinevere
# | Dependencies: port/kit/{manifest,sector,sources,disc_input}.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import hashlib
import os
import shutil
import tempfile

from port.kit import disc_input, manifest, sector, sources

#----------------------
# | SCRATCH_BYTES
# | Description: Room the converter's temporary cd/data tree needs on top of the GDI itself.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SCRATCH_BYTES = 600 << 20

#----------------------
# | COPIED
# | Description: The kit files copied into the GDI folder as they are.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
COPIED = ("disc.gdi", "track01.bin", "track02.raw")


#----------------------
# | KitError
# | Description: A kit failure worded for the person running it.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
class KitError(Exception):
    pass


#----------------------
# | needed_bytes
# | Description: Disk space a run needs: every track plus converter scratch.
# | Author: suinevere
# | Dependencies: SCRATCH_BYTES, sector.SECTOR
# | Globals: N/A
# | Params: m -- manifest dict
# | Returns: int
#----------------------
def needed_bytes(m):
    return sum(t["sectors"] for t in m["tracks"]) * sector.SECTOR + SCRATCH_BYTES


#----------------------
# | write_track03
# | Description: Streams track03 to dest: literal runs (raw sectors at negative offsets), hole bytes from srcs, zeros elsewhere, each sector encoded as Mode 1.
# | Author: suinevere
# | Dependencies: manifest.read_literals, sector.encode
# | Globals: N/A
# | Params: kit_dir -- kit folder, m -- manifest, srcs -- sources.build result, dest -- output path, progress -- callable(done, total) or None
# | Returns: hex SHA-1 of what was written
#----------------------
def write_track03(kit_dir, m, srcs, dest, progress=None):
    track = next(t for t in m["tracks"] if t["name"] == "track03.bin")
    total, lba = track["sectors"], track["lba"]
    literals = manifest.read_literals(os.path.join(kit_dir, "track03.lit"))
    pending = next(literals, None)
    holes = m["holes"]
    hole_index = 0
    digest = hashlib.sha1()
    with open(dest, "wb") as handle:
        for i in range(total):
            lo, hi = i * 2048, (i + 1) * 2048
            raw = None
            user = bytearray(2048)
            while pending is not None and (pending[0] == -1 - i or 0 <= pending[0] < hi):
                offset, data = pending
                if offset < 0:
                    raw = data
                else:
                    user[offset - lo:offset - lo + len(data)] = data
                pending = next(literals, None)
            while hole_index < len(holes) and holes[hole_index]["offset"] + holes[hole_index]["length"] <= lo:
                hole_index += 1
            j = hole_index
            while j < len(holes) and holes[j]["offset"] < hi:
                h = holes[j]
                a, b = max(lo, h["offset"]), min(hi, h["offset"] + h["length"])
                start = h["source_offset"] + a - h["offset"]
                user[a - lo:b - lo] = srcs[h["source"]][start:start + b - a]
                j += 1
            out = raw if raw is not None else sector.encode(lba + i, bytes(user))
            handle.write(out)
            digest.update(out)
            if progress and (i % 4096 == 0 or i == total - 1):
                progress(i + 1, total)
    return digest.hexdigest()


#----------------------
# | _load_manifest
# | Description: Reads the kit's manifest, raising KitError when the folder is missing or from another format version.
# | Author: suinevere
# | Dependencies: manifest.read, manifest.VERSION, KitError
# | Globals: N/A
# | Params: kit_dir -- kit folder
# | Returns: dict
#----------------------
def _load_manifest(kit_dir):
    try:
        m = manifest.read(kit_dir)
    except (OSError, ValueError):
        raise KitError("no kit data in %s; keep the kit folder next to the program" % kit_dir)
    if m.get("version") != manifest.VERSION:
        raise KitError("this kit's data is format %s but the program reads format %s; use the program and kit folder from the same release"
                       % (m.get("version"), manifest.VERSION))
    return m


#----------------------
# | make_gdi
# | Description: Checks the kit and the disc, writes the GDI into out_dir.tmp, checks every track's SHA-1 and only then renames it to out_dir.
# | Author: suinevere
# | Dependencies: _load_manifest, disc_input, sources, write_track03, needed_bytes, KitError, COPIED
# | Globals: N/A
# | Params: kit_dir -- kit folder, image_path -- the user's data-track .bin, out_dir -- GDI folder to create, overwrite -- replace an existing out_dir, progress -- callable(message)
# | Returns: out_dir
#----------------------
def make_gdi(kit_dir, image_path, out_dir, overwrite=False, progress=print):
    m = _load_manifest(kit_dir)
    if os.path.exists(out_dir) and not overwrite:
        raise KitError("%s already exists; pick another folder or pass --overwrite" % out_dir)
    parent = os.path.dirname(os.path.abspath(out_dir))
    os.makedirs(parent, exist_ok=True)
    free = shutil.disk_usage(parent)[2]
    if free < needed_bytes(m):
        raise KitError("%s has %d MB free but the GDI needs %d MB" % (parent, free >> 20, needed_bytes(m) >> 20))
    progress("Checking your disc image")
    try:
        disc_input.check(image_path, m["ps1_sha1"])
    except disc_input.DiscError as exc:
        raise KitError(str(exc))
    tmp = out_dir + ".tmp"
    shutil.rmtree(tmp, ignore_errors=True)
    os.makedirs(tmp)
    try:
        with tempfile.TemporaryDirectory(dir=parent) as work:
            progress("Converting the PS1 data")
            srcs = sources.build(image_path, work)
            progress("Writing track03.bin")
            got = write_track03(kit_dir, m, srcs, os.path.join(tmp, "track03.bin"),
                                lambda done, total: progress("  %d%%" % (done * 100 // total)))
        for name in COPIED:
            shutil.copyfile(os.path.join(kit_dir, name), os.path.join(tmp, name))
        for track in m["tracks"]:
            actual = got if track["name"] == "track03.bin" else disc_input.sha1(os.path.join(tmp, track["name"]))
            if actual != track["sha1"]:
                raise KitError("%s came out with SHA-1 %s instead of %s; please report this with your disc's SHA-1"
                               % (track["name"], actual, track["sha1"]))
    except BaseException:
        shutil.rmtree(tmp, ignore_errors=True)
        raise
    if os.path.exists(out_dir):
        shutil.rmtree(out_dir)
    os.rename(tmp, out_dir)
    return out_dir
