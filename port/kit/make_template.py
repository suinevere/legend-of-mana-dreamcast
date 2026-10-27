#----------------------
# | make_template.py
# | Description: CI side of the release kit: turns the GDI built from the PS1 disc into kit data with every disc-derived byte of track03 zeroed, plus the hole manifest that says what refills each gap.
# | Author: suinevere
# | Dependencies: port/kit/{sector,iso9660,elf32,sources,manifest,disc_input}.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import os
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")))

from port.kit import disc_input, elf32, iso9660, manifest, sector, sources  # noqa: E402

#----------------------
# | BOOT_VADDR / TRACK03_LBA / ELF_HOLES
# | Description: Where 1ST_READ.BIN loads (lom.elf's entry), track03's first LBA, and the ELF symbols whose whole bytes come from the disc.
# | Author: suinevere
# | Dependencies: dreamcast/dreamcast.ld, dreamcast/gdi/disc.gdi
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
BOOT_VADDR = 0x8C010000
TRACK03_LBA = 45000
ELF_HOLES = (("gen/libgte_tables", "lom_libgte_tables"), ("gen/main_data", "lom_main_data"), ("gen/vmu_pet_screens", "lom_vmu_pet_screens"))

#----------------------
# | COPIED
# | Description: The GDI files that hold no disc data and ship as they are.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
COPIED = ("disc.gdi", "track01.bin", "track02.raw")


#----------------------
# | _hole
# | Description: One manifest hole entry.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: offset -- track03 user-data byte offset, length -- bytes, source -- source name, source_offset -- offset into the source
# | Returns: dict
#----------------------
def _hole(offset, length, source, source_offset=0):
    return {"offset": offset, "length": length, "source": source, "source_offset": source_offset}


#----------------------
# | plan_holes
# | Description: Every track03 byte range the disc fills: whole converter files, the ELF's disc blocks (AKAO without its relocated pointer words) and the VMU GAME's RES1506 pages.
# | Author: suinevere
# | Dependencies: ELF_HOLES, BOOT_VADDR, _hole
# | Globals: N/A
# | Params: files -- iso9660.list_files result, srcs -- sources.build result, first_read_offset -- 1ST_READ.BIN's user-data byte offset, syms -- elf32.symbols result, akao_pointers -- pointer word indices, rrl -- RRL.VMS bytes
# | Returns: list of hole dicts sorted by offset
#----------------------
def plan_holes(files, srcs, first_read_offset, syms, akao_pointers, rrl):
    holes = []
    for name, data in srcs.items():
        if not name.startswith("cd/"):
            continue
        path = name[3:]
        if path not in files:
            raise ValueError("converter output %s is not on the built disc" % path)
        start, size = files[path]
        if size != len(data):
            raise ValueError("%s is %d bytes on the disc but the converter wrote %d" % (path, size, len(data)))
        holes.append(_hole(start * 2048, size, name))
    for source, symbol in ELF_HOLES:
        if source in srcs:
            holes.append(_hole(first_read_offset + syms[symbol][0] - BOOT_VADDR, len(srcs[source]), source))
    if "gen/akao_data" in srcs:
        base = first_read_offset + syms["lom_akao_data"][0] - BOOT_VADDR
        pointers = set(akao_pointers)
        words = len(srcs["gen/akao_data"]) // 4
        run = None
        for index in range(words + 1):
            plain = index < words and index not in pointers
            if plain and run is None:
                run = index
            elif not plain and run is not None:
                holes.append(_hole(base + run * 4, (index - run) * 4, "gen/akao_data", run * 4))
                run = None
    if "vmu/rrl_pages" in srcs:
        pages = srcs["vmu/rrl_pages"]
        at = rrl.find(pages)
        if at < 0 or rrl.find(pages, at + 1) >= 0:
            raise ValueError("RRL.VMS holds the RES1506 pages %s" % ("nowhere" if at < 0 else "more than once"))
        holes.append(_hole(files["VMU/RRL.VMS"][0] * 2048 + at, len(pages), "vmu/rrl_pages"))
    return sorted(holes, key=lambda h: h["offset"])


#----------------------
# | _read_file
# | Description: The bytes of one ISO file from a raw track.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: read_user -- sector reader, entry -- (sector, size)
# | Returns: bytes
#----------------------
def _read_file(read_user, entry):
    start, size = entry
    return b"".join(read_user(start + i) for i in range((size + 2047) // 2048))[:size]


#----------------------
# | _check_no_overlap
# | Description: Raises if two holes cover the same byte.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: holes -- plan_holes result
# | Returns: N/A
#----------------------
def _check_no_overlap(holes):
    for a, b in zip(holes, holes[1:]):
        if a["offset"] + a["length"] > b["offset"]:
            raise ValueError("holes %s and %s overlap" % (a["source"], b["source"]))


#----------------------
# | _template_runs
# | Description: Streams track03, checks each hole's bytes equal its source, zeroes them, and yields the non-zero user data of each sector, or the whole raw sector for one that does not re-encode as Mode 1.
# | Author: suinevere
# | Dependencies: sector
# | Globals: TRACK03_LBA
# | Params: track03 -- raw track path, holes -- sorted holes, srcs -- sources.build result, raw_sectors -- list collecting raw sector indices
# | Returns: generator of (offset, bytes)
#----------------------
def _template_runs(track03, holes, srcs, raw_sectors):
    sectors = os.path.getsize(track03) // sector.SECTOR
    hole_index = 0
    with open(track03, "rb") as handle:
        for i in range(sectors):
            raw = handle.read(sector.SECTOR)
            original = sector.user_data(raw)
            user = bytearray(original)
            lo, hi = i * 2048, (i + 1) * 2048
            while hole_index < len(holes) and holes[hole_index]["offset"] + holes[hole_index]["length"] <= lo:
                hole_index += 1
            touched = False
            j = hole_index
            while j < len(holes) and holes[j]["offset"] < hi:
                h = holes[j]
                a, b = max(lo, h["offset"]), min(hi, h["offset"] + h["length"])
                start = h["source_offset"] + a - h["offset"]
                if bytes(user[a - lo:b - lo]) != srcs[h["source"]][start:start + b - a]:
                    raise ValueError("track03 bytes at %d do not match %s" % (a, h["source"]))
                user[a - lo:b - lo] = bytes(b - a)
                touched = True
                j += 1
            if sector.encode(TRACK03_LBA + i, original) != raw:
                if touched:
                    raise ValueError("sector %d holds disc data but does not re-encode as Mode 1" % i)
                raw_sectors.append(i)
                yield -1 - i, raw
            elif any(user):
                yield lo, bytes(user)


#----------------------
# | build_kit
# | Description: Checks 1ST_READ.BIN holds lom.elf, plans and checks every hole, writes the literal runs, the copied tracks and the manifest.
# | Author: suinevere
# | Dependencies: plan_holes, _template_runs, _check_no_overlap, iso9660, elf32, sources, manifest, disc_input
# | Globals: TRACK03_LBA, BOOT_VADDR, COPIED
# | Params: gdi -- built GDI folder, elf -- lom.elf, image -- PS1 .bin, out -- kit folder, tag -- release tag, sha -- git commit
# | Returns: manifest dict
#----------------------
def build_kit(gdi, elf, image, out, tag, sha):
    track03 = os.path.join(gdi, "track03.bin")
    read_user = iso9660.raw_reader(track03)
    files = iso9660.list_files(read_user, TRACK03_LBA)
    first_read = _read_file(read_user, files["1ST_READ.BIN"])
    for vaddr, _, data in elf32.load_segments(elf):
        at = vaddr - BOOT_VADDR
        if first_read[at:at + len(data)] != data:
            raise ValueError("1ST_READ.BIN does not hold lom.elf's segment at 0x%08X" % vaddr)
    with tempfile.TemporaryDirectory() as work:
        srcs = sources.build(image, work)
    holes = plan_holes(files, srcs, files["1ST_READ.BIN"][0] * 2048, elf32.symbols(elf),
                       sources.akao_pointer_words(image), _read_file(read_user, files["VMU/RRL.VMS"]))
    _check_no_overlap(holes)
    os.makedirs(out, exist_ok=True)
    raw_sectors = []
    manifest.write_literals(os.path.join(out, "track03.lit"), _template_runs(track03, holes, srcs, raw_sectors))
    for name in COPIED:
        shutil.copyfile(os.path.join(gdi, name), os.path.join(out, name))
    data = {
        "version": manifest.VERSION, "kit_tag": tag, "git_sha": sha, "ps1_sha1": disc_input.sha1(image),
        "tracks": [{"name": n, "lba": lba, "sectors": os.path.getsize(os.path.join(gdi, n)) // sector.SECTOR,
                    "sha1": disc_input.sha1(os.path.join(gdi, n))}
                   for n, lba in (("track01.bin", 0), ("track02.raw", 600), ("track03.bin", TRACK03_LBA))],
        "holes": holes, "raw_sectors": raw_sectors,
    }
    manifest.write(out, data)
    return data


#----------------------
# | main
# | Description: Command line for build_kit.
# | Author: suinevere
# | Dependencies: build_kit
# | Globals: N/A
# | Params: argv -- arguments
# | Returns: exit code
#----------------------
def main(argv=None):
    parser = argparse.ArgumentParser(description="Make release kit data from a GDI built from the PS1 disc")
    parser.add_argument("--gdi", required=True)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--image", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--sha", required=True)
    args = parser.parse_args(argv)
    data = build_kit(args.gdi, args.elf, args.image, args.out, args.tag, args.sha)
    print("kit data: %d holes, %d raw sectors" % (len(data["holes"]), len(data["raw_sectors"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
