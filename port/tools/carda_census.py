#----------------------
# | carda_census.py
# | Description: Lists every scene event-script command that opens CARDA, with its mode, by walking a PS1 disc image's scene files.
# | Author: suinevere
# | Dependencies: disc_image, tools.scenes.event_scripts, tools.scenes.scene_format
# | Globals: PVD_LBA, ROOT_DIRECTORY_RECORD_OFFSET, CARDA_COMMANDS
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))
sys.path.insert(0, os.path.dirname(__file__))

from disc_image import DiscImage, PVD_LBA, ROOT_DIRECTORY_RECORD_OFFSET
from tools.scenes.event_scripts import read_event_scripts
from tools.scenes.scene_format import SceneHeader

#----------------------
# | CARDA_COMMANDS
# | Description: Misc subcommand names of the two event commands that open CARDA.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CARDA_COMMANDS = ("open_carda", "open_carda_scene")


#----------------------
# | walk_iso
# | Description: Yields (path, lba, size) for every file below a directory record, depth first.
# | Author: suinevere
# | Dependencies: DiscImage.read_sectors
# | Globals: N/A
# | Params: image -- DiscImage, lba, size -- directory extent, prefix -- path so far
# | Returns: generator of (path, lba, size)
#----------------------
def walk_iso(image, lba, size, prefix=""):
    data = image.read_sectors(lba, (size + 2047) // 2048)[:size]
    pos = 0
    while pos < len(data):
        length = data[pos]
        if length == 0:
            pos = (pos // 2048 + 1) * 2048
            continue
        child_lba, child_size = struct.unpack_from("<I", data, pos + 2)[0], struct.unpack_from("<I", data, pos + 10)[0]
        flags = data[pos + 25]
        name = bytes(data[pos + 33:pos + 33 + data[pos + 32]]).decode("ascii", "ignore").split(";")[0]
        if name not in ("\x00", "\x01"):
            path = prefix + "/" + name
            if flags & 2:
                yield from walk_iso(image, child_lba, child_size, path)
            else:
                yield path, child_lba, child_size
        pos += length


#----------------------
# | census
# | Description: Decodes the event scripts of every .IMG file and collects the CARDA-opening commands.
# | Author: suinevere
# | Dependencies: walk_iso, SceneHeader.parse, read_event_scripts
# | Globals: CARDA_COMMANDS, PVD_LBA, ROOT_DIRECTORY_RECORD_OFFSET
# | Params: image_path -- PS1 disc image
# | Returns: (list of (path, offset, command, operands), count of scenes read, list of unreadable paths)
#----------------------
def census(image_path):
    image = DiscImage(image_path)
    pvd = image.read_sectors(PVD_LBA, 1)
    root_lba = struct.unpack_from("<I", pvd, ROOT_DIRECTORY_RECORD_OFFSET + 2)[0]
    root_size = struct.unpack_from("<I", pvd, ROOT_DIRECTORY_RECORD_OFFSET + 10)[0]
    found, scenes, failed = [], 0, []
    for path, lba, size in walk_iso(image, root_lba, root_size):
        if not path.upper().endswith(".IMG"):
            continue
        data = image.read_sectors(lba, (size + 2047) // 2048)[:size]
        try:
            events = read_event_scripts(data, SceneHeader.parse(data))
        except (ValueError, struct.error, IndexError):
            failed.append(path)
            continue
        scenes += 1
        for instruction in events["instructions"]:
            if instruction.get("subcommand_name") in CARDA_COMMANDS:
                found.append((path, instruction["offset"], instruction["subcommand_name"], instruction.get("operands")))
    return found, scenes, failed


#----------------------
# | main
# | Description: Prints the census for one disc image.
# | Author: suinevere
# | Dependencies: census
# | Globals: N/A
# | Params: argv -- command line
# | Returns: process exit code
#----------------------
def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image")
    args = parser.parse_args(argv)
    found, scenes, failed = census(args.image)
    for path, offset, command, operands in found:
        print("%s 0x%X %s %s" % (path, offset, command, operands))
    print("%d scenes read, %d unreadable, %d CARDA commands" % (scenes, len(failed), len(found)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
