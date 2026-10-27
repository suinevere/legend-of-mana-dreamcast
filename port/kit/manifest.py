#----------------------
# | manifest.py
# | Description: The release kit's manifest.json and the zlib literal-run file it ships for track03.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import json
import os
import struct
import zlib

#----------------------
# | VERSION
# | Description: Kit data format version; the executable refuses any other.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
VERSION = 1

#----------------------
# | RUN_HEADER
# | Description: Each literal run's header: track03 user-data byte offset (signed 64-bit; -1 - n marks raw sector n) and length (u32).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RUN_HEADER = struct.Struct("<qI")


#----------------------
# | write
# | Description: Saves kit_dir/manifest.json.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: kit_dir -- kit data folder, data -- manifest dict
# | Returns: N/A
#----------------------
def write(kit_dir, data):
    with open(os.path.join(kit_dir, "manifest.json"), "w", encoding="utf-8", newline="\n") as handle:
        json.dump(data, handle, indent=1, sort_keys=True)


#----------------------
# | read
# | Description: Loads kit_dir/manifest.json.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: kit_dir -- kit data folder
# | Returns: dict
#----------------------
def read(kit_dir):
    with open(os.path.join(kit_dir, "manifest.json"), encoding="utf-8") as handle:
        return json.load(handle)


#----------------------
# | write_literals
# | Description: Writes (offset, bytes) runs as one zlib stream of RUN_HEADER records.
# | Author: suinevere
# | Dependencies: RUN_HEADER
# | Globals: N/A
# | Params: path -- output file, runs -- iterable of (offset, bytes) in track order
# | Returns: N/A
#----------------------
def write_literals(path, runs):
    packer = zlib.compressobj(9)
    with open(path, "wb") as handle:
        for offset, data in runs:
            handle.write(packer.compress(RUN_HEADER.pack(offset, len(data)) + data))
        handle.write(packer.flush())


#----------------------
# | read_literals
# | Description: Yields the (offset, bytes) runs write_literals stored, decompressing a megabyte at a time.
# | Author: suinevere
# | Dependencies: RUN_HEADER
# | Globals: N/A
# | Params: path -- literal file
# | Returns: generator of (offset, bytes)
#----------------------
def read_literals(path):
    unpacker = zlib.decompressobj()
    buffer = bytearray()
    pos = 0
    with open(path, "rb") as handle:
        chunks = iter(lambda: handle.read(1 << 20), b"")
        finished = False
        while True:
            while len(buffer) - pos >= RUN_HEADER.size:
                offset, length = RUN_HEADER.unpack_from(buffer, pos)
                end = pos + RUN_HEADER.size + length
                if len(buffer) < end:
                    break
                yield offset, bytes(buffer[pos + RUN_HEADER.size:end])
                pos = end
            if finished:
                break
            del buffer[:pos]
            pos = 0
            chunk = next(chunks, None)
            if chunk is None:
                buffer += unpacker.flush()
                finished = True
            else:
                buffer += unpacker.decompress(chunk)
    if pos != len(buffer):
        raise ValueError("%s ends partway through a literal run" % path)
