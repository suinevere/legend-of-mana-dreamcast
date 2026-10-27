#----------------------
# | iso9660.py
# | Description: Lists every file in the ISO 9660 filesystem of a raw data track, with its track-relative start sector and size.
# | Author: suinevere
# | Dependencies: port/kit/sector.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import struct

from port.kit import sector


#----------------------
# | raw_reader
# | Description: A reader returning the 2048 user bytes of track-relative sector i of a raw 2352-byte track file.
# | Author: suinevere
# | Dependencies: sector.SECTOR, sector.user_data
# | Globals: N/A
# | Params: path -- raw track file
# | Returns: callable(int) -> bytes
#----------------------
def raw_reader(path):
    def read(i):
        with open(path, "rb") as handle:
            handle.seek(i * sector.SECTOR)
            return sector.user_data(handle.read(sector.SECTOR))
    return read


#----------------------
# | _records
# | Description: Yields (name, extent, size, is_dir) for each directory record in a directory's extent, skipping the self and parent entries and each sector's zero padding.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: read_user -- sector reader, start -- track-relative first sector, size -- directory size in bytes
# | Returns: generator
#----------------------
def _records(read_user, start, size):
    for i in range((size + 2047) // 2048):
        data = read_user(start + i)
        pos = 0
        while pos < 2048 and data[pos]:
            length = data[pos]
            extent = struct.unpack_from("<I", data, pos + 2)[0]
            data_size = struct.unpack_from("<I", data, pos + 10)[0]
            is_dir = bool(data[pos + 25] & 2)
            ident = data[pos + 33:pos + 33 + data[pos + 32]]
            if ident not in (b"\x00", b"\x01"):
                yield ident.decode("ascii").split(";")[0].upper(), extent, data_size, is_dir
            pos += length


#----------------------
# | list_files
# | Description: Walks the ISO from its primary volume descriptor at track sector 16 and maps every file's path to its track-relative start sector and size.
# | Author: suinevere
# | Dependencies: _records
# | Globals: N/A
# | Params: read_user -- sector reader, track_lba -- the track's absolute start LBA
# | Returns: dict path -> (sector, size)
#----------------------
def list_files(read_user, track_lba):
    pvd = read_user(16)
    if pvd[0:6] != b"\x01CD001":
        raise ValueError("no ISO 9660 primary volume descriptor at sector 16")
    root_extent = struct.unpack_from("<I", pvd, 156 + 2)[0] - track_lba
    root_size = struct.unpack_from("<I", pvd, 156 + 10)[0]
    files = {}
    pending = [("", root_extent, root_size)]
    while pending:
        prefix, start, size = pending.pop()
        for name, extent, data_size, is_dir in _records(read_user, start, size):
            path = prefix + name
            if is_dir:
                pending.append((path + "/", extent - track_lba, data_size))
            else:
                files[path] = (extent - track_lba, data_size)
    return files
