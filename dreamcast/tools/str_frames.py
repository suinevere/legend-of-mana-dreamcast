#----------------------
# | str_frames.py
# | Description: Reads PS1 STR movie data from the PS1 image: one frame's v2 bitstream for the host decoder, or raw 2352-byte sectors for ffmpeg.
# | Author: suinevere
# | Dependencies: port/tools/disc_image.py, port/tools/convert_disc.py
# | Globals: VIDEO_SECTOR_TYPE, STREAM_HEADER_BYTES, SECTOR_SCAN_LIMIT
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import struct
import sys

REPO = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
sys.path.insert(0, os.path.join(REPO, "port", "tools"))

import convert_disc  # noqa: E402
import disc_image  # noqa: E402

#----------------------
# | VIDEO_SECTOR_TYPE / STREAM_HEADER_BYTES
# | Description: Stream-header type of a video sector, and the header bytes ahead of each sector's 2016-byte payload.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
VIDEO_SECTOR_TYPE = 0x8001
STREAM_HEADER_BYTES = 32

#----------------------
# | SECTOR_SCAN_LIMIT
# | Description: Most sectors read in one batch while looking for a frame's sectors.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SECTOR_SCAN_LIMIT = 64


#----------------------
# | default_image
# | Description: The PS1 image the disc converter uses.
# | Author: suinevere
# | Dependencies: convert_disc.default_image
# | Globals: N/A
# | Params: N/A
# | Returns: image path
#----------------------
def default_image():
    return convert_disc.default_image()


#----------------------
# | resource_entry
# | Description: Opens the image and looks up one resource's table entry.
# | Author: suinevere
# | Dependencies: disc_image.DiscImage, disc_image.parse_resource_table, convert_disc.TABLE_LBA, convert_disc.TABLE_SIZE
# | Globals: N/A
# | Params: image_path -- PS1 image, resource -- resource index
# | Returns: (DiscImage, Resource)
#----------------------
def resource_entry(image_path, resource):
    image = disc_image.DiscImage(image_path)
    blob = image.read_sectors(convert_disc.TABLE_LBA, (convert_disc.TABLE_SIZE + 2047) // 2048)[:convert_disc.TABLE_SIZE]
    return image, disc_image.parse_resource_table(blob)[resource]


#----------------------
# | frame_bitstream
# | Description: Concatenates the payloads of one frame's video sectors in chunk order; stream frame numbers start at 1.
# | Author: suinevere
# | Dependencies: resource_entry
# | Globals: VIDEO_SECTOR_TYPE, STREAM_HEADER_BYTES, SECTOR_SCAN_LIMIT
# | Params: image_path -- PS1 image, resource -- movie resource index, frame_index -- 0-based frame
# | Returns: (bitstream bytes, width, height)
#----------------------
def frame_bitstream(image_path, resource, frame_index):
    image, res = resource_entry(image_path, resource)
    total = (res.size + 2047) // 2048
    want = frame_index + 1
    chunks = {}
    width = height = count = 0
    first = 0
    while first < total:
        batch = image.read_sectors(res.lba + first, min(SECTOR_SCAN_LIMIT, total - first))
        for k in range(len(batch) // 2048):
            sector = batch[k * 2048:(k + 1) * 2048]
            _, kind, chunk, chunk_count, frame, _, w, h = struct.unpack_from("<HHHHIIHH", sector)
            if kind == VIDEO_SECTOR_TYPE and frame == want:
                chunks[chunk] = sector[STREAM_HEADER_BYTES:]
                width, height, count = w, h, chunk_count
            elif chunks and frame > want:
                first = total
                break
        first += SECTOR_SCAN_LIMIT
    if not chunks or len(chunks) != count:
        raise ValueError("frame %d of resource 0x%X not found whole" % (frame_index, resource))
    return b"".join(chunks[i] for i in range(count)), width, height


#----------------------
# | raw_sectors
# | Description: Reads whole 2352-byte sectors of a resource, sync and XA subheader included, as ffmpeg's psxstr demuxer wants them.
# | Author: suinevere
# | Dependencies: resource_entry
# | Globals: N/A
# | Params: image_path -- PS1 image, resource -- resource index, first -- first sector, count -- sector count
# | Returns: bytes of length count * 2352
#----------------------
def raw_sectors(image_path, resource, first, count):
    image, res = resource_entry(image_path, resource)
    if image.stride != 2352:
        raise ValueError("raw sectors need a 2352-byte image")
    with open(image.path, "rb") as handle:
        handle.seek((res.lba + first) * 2352)
        return handle.read(count * 2352)


#----------------------
# | main
# | Description: "frame <res-hex> <n> <out>" writes one frame's bitstream; "raw <res-hex> <first> <count> <out>" writes raw sectors.
# | Author: suinevere
# | Dependencies: frame_bitstream, raw_sectors, default_image
# | Globals: N/A
# | Params: argv -- command line after the script name
# | Returns: process exit code
#----------------------
def main(argv):
    if len(argv) == 4 and argv[0] == "frame":
        data, width, height = frame_bitstream(default_image(), int(argv[1], 16), int(argv[2]))
        print("%dx%d" % (width, height))
    elif len(argv) == 5 and argv[0] == "raw":
        data = raw_sectors(default_image(), int(argv[1], 16), int(argv[2]), int(argv[3]))
    else:
        sys.stderr.write("usage: str_frames.py frame <res-hex> <n> <out> | raw <res-hex> <first> <count> <out>\n")
        return 2
    with open(argv[-1], "wb") as f:
        f.write(data)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
