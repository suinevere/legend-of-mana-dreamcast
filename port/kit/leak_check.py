#----------------------
# | leak_check.py
# | Description: Fails when any 256-byte chunk of the PS1 disc's user data or of any converter source appears anywhere in the kit data, so no game bytes ship.
# | Author: suinevere
# | Dependencies: port/kit/{manifest,sources}.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import hashlib
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")))

from port.kit import manifest, sources  # noqa: E402

#----------------------
# | CHUNK / STRIDE
# | Description: Chunk size hashed from the game data, and the step the kit data is scanned at: every byte, so a copy at any alignment is found.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CHUNK = 256
STRIDE = 1

#----------------------
# | MIN_DISTINCT
# | Description: Fewest distinct byte values a chunk needs to count, so zero runs with a stray flag and ISO descriptor filler, which every disc shares, are not taken for game data.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
MIN_DISTINCT = 16

#----------------------
# | SHIPPED
# | Description: The kit files other than track03.lit, scanned whole.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SHIPPED = ("track01.bin", "track02.raw", "manifest.json", "disc.gdi")


#----------------------
# | _digest
# | Description: An 8-byte BLAKE2b digest of a chunk.
# | Author: suinevere
# | Dependencies: hashlib
# | Globals: N/A
# | Params: chunk -- bytes
# | Returns: bytes
#----------------------
def _digest(chunk):
    return hashlib.blake2b(chunk, digest_size=8).digest()


#----------------------
# | _informative
# | Description: Whether a chunk holds at least MIN_DISTINCT distinct byte values.
# | Author: suinevere
# | Dependencies: MIN_DISTINCT
# | Globals: N/A
# | Params: chunk -- bytes
# | Returns: bool
#----------------------
def _informative(chunk):
    return len(set(chunk)) >= MIN_DISTINCT


#----------------------
# | chunk_hashes
# | Description: Digests of every size-aligned chunk of each blob that _informative accepts.
# | Author: suinevere
# | Dependencies: _digest, _informative
# | Globals: N/A
# | Params: blobs -- iterable of bytes, size -- chunk size
# | Returns: set of digests
#----------------------
def chunk_hashes(blobs, size=CHUNK):
    out = set()
    for blob in blobs:
        for at in range(0, len(blob) - size + 1, size):
            chunk = blob[at:at + size]
            if _informative(chunk):
                out.add(_digest(chunk))
    return out


#----------------------
# | scan
# | Description: Offsets in data, at stride steps, where a chunk _informative accepts has its digest in hashes.
# | Author: suinevere
# | Dependencies: _digest, _informative
# | Globals: N/A
# | Params: data -- bytes, hashes -- chunk_hashes result, size -- chunk size, stride -- step
# | Returns: list of int
#----------------------
def scan(data, hashes, size=CHUNK, stride=STRIDE):
    hits = []
    for at in range(0, len(data) - size + 1, stride):
        chunk = data[at:at + size]
        if _informative(chunk) and _digest(chunk) in hashes:
            hits.append(at)
    return hits


#----------------------
# | unallowed
# | Description: The hits not inside a range of the allow list, which names data the user agreed ships because the repo already publishes its source.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: hits -- list of (kit file, offset), allow -- list of {file, offset, length, source}
# | Returns: list of (kit file, offset)
#----------------------
def unallowed(hits, allow):
    return [(name, at) for name, at in hits
            if not any(a["file"] == name and a["offset"] <= at < a["offset"] + a["length"] for a in allow)]


#----------------------
# | _disc_user_data
# | Description: Each PS1 sector's payload from offset 24, 2324 bytes so Form 2 (XA audio, movie) sectors count whole as well as Form 1, in pieces of 32768 sectors.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: image -- PS1 .bin
# | Returns: generator of bytes
#----------------------
def _disc_user_data(image):
    with open(image, "rb") as handle:
        while True:
            raw = handle.read(2352 * 32768)
            if not raw:
                return
            yield b"".join(raw[i + 24:i + 24 + 2324] for i in range(0, len(raw), 2352))


#----------------------
# | main
# | Description: Hashes the disc and the converter output, scans every shipped kit file, prints each hit outside the allow list and exits 1 if there is one.
# | Author: suinevere
# | Dependencies: chunk_hashes, scan, unallowed, manifest.read_literals, sources.build, _disc_user_data, SHIPPED
# | Globals: N/A
# | Params: argv -- --kit DIR --image BIN [--allow JSON]
# | Returns: exit code
#----------------------
def main(argv=None):
    parser = argparse.ArgumentParser(description="Fail if kit data holds game bytes")
    parser.add_argument("--kit", required=True)
    parser.add_argument("--image", required=True)
    parser.add_argument("--allow", help="JSON list of {file, offset, length, source} ranges that may ship")
    args = parser.parse_args(argv)
    allow = []
    if args.allow:
        with open(args.allow, encoding="utf-8") as handle:
            allow = json.load(handle)
    hashes = chunk_hashes(_disc_user_data(args.image))
    with tempfile.TemporaryDirectory() as work:
        hashes |= chunk_hashes(sources.build(args.image, work).values())
    hits = []
    for name in SHIPPED:
        with open(os.path.join(args.kit, name), "rb") as handle:
            hits += [(name, at) for at in scan(handle.read(), hashes)]
    for offset, data in manifest.read_literals(os.path.join(args.kit, "track03.lit")):
        name = "track03.lit@%d" % offset
        hits += [(name, at) for at in scan(data, hashes)]
    bad = unallowed(hits, allow)
    for name, at in bad:
        print("game bytes in %s at +%d" % (name, at))
    for entry in allow:
        print("allowed: %s +%d..+%d (%s)" % (entry["file"], entry["offset"], entry["offset"] + entry["length"], entry["source"]))
    print("leak check: %d hits (%d allowed) over %d chunks" % (len(bad), len(hits) - len(bad), len(hashes)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
