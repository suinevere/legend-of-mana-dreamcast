#----------------------
# | convert_sound.py
# | Description: Extracts Legend of Mana's AKAO instrument banks from the PS1 disc, decodes every sample to SCSP PCM and writes a manifest.
# | Author: suinevere
# | Dependencies: akao.py, spu_adpcm.py, disc_image.py, convert_disc.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import functools
import hashlib
import json
import os
import sys

import akao  # noqa: E402
import convert_disc  # noqa: E402
import disc_image  # noqa: E402
import spu_adpcm  # noqa: E402

#----------------------
# | SOUND_RESOURCES
# | Description: (resource, kind) for every sound resource the game loads, from cd_resources.h and field_audio_runtime.c.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SOUND_RESOURCES = sorted(
    [(21, "effect_set"), (22, "bank"), (146, "music")]
    + [(i, "music") for i in range(23, 74)]
    + [(i, "sfx_set") for i in range(81, 119)]
    + [(i, "weapon_sfx") for i in range(131, 142)]
    + [(i, "program") for i in (142, 143)]
    + [(i, "music") for i in range(147, 167)])

#----------------------
# | SOUND_RAM_BYTES
# | Description: SCSP sound RAM size, the budget the size summary is measured against.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SOUND_RAM_BYTES = 512 * 1024


#----------------------
# | sample_loop
# | Description: Loop of a decoded sample: the articulation loop address when it falls inside the sample, else the flag loop start, else 0.
# | Author: suinevere
# | Dependencies: spu_adpcm.BLOCK_SIZE, spu_adpcm.SAMPLES_PER_BLOCK
# | Globals: N/A
# | Params: sample -- spu_adpcm.Sample, offset -- sample byte offset, loop_addr -- articulation loop address
# | Returns: {"start", "end"} or None for a one-shot
#----------------------
def sample_loop(sample, offset, loop_addr):
    if not sample.loops:
        return None
    if offset <= loop_addr < offset + sample.blocks * spu_adpcm.BLOCK_SIZE:
        start = (loop_addr - offset) // spu_adpcm.BLOCK_SIZE * spu_adpcm.SAMPLES_PER_BLOCK
    elif sample.loop_start is not None:
        start = sample.loop_start
    else:
        start = 0
    return {"start": start, "end": sample.loop_end}


#----------------------
# | convert_bank
# | Description: Decodes each distinct sample of a bank, replacing loop bodies with their steady-state cycle, writes its PCM files and returns its manifest entry.
# | Author: suinevere
# | Dependencies: spu_adpcm, akao.root_pitch, akao.root_rate_hz, sample_loop
# | Globals: N/A
# | Params: bank -- akao.Bank, resource -- index, section -- section index, kind -- resource kind, out_dir -- output folder
# | Returns: dict manifest entry
#----------------------
def convert_bank(bank, resource, section, kind, out_dir):
    offsets = sorted({art.sample_addr for art in bank.articulations})
    first_loop = {}
    for art in bank.articulations:
        first_loop.setdefault(art.sample_addr, art.loop_addr)
    pcm16 = bytearray()
    pcm8 = bytearray()
    samples = []
    decoded_at = {}
    for number, offset in enumerate(offsets):
        decoded = decoded_at[offset] = spu_adpcm.decode(bank.sample_data, offset)
        loop = sample_loop(decoded, offset, first_loop[offset])
        pcm = decoded.pcm
        if loop is not None:
            body = spu_adpcm.steady_loop(bank.sample_data, offset, decoded, loop["start"])
            pcm = decoded.pcm[:loop["start"]] + body.pcm
            loop["end"] = len(pcm)
            loop["converged"] = body.converged
            loop["passes"] = body.passes
            loop["period"] = body.period
        samples.append({
            "index": number,
            "adpcm_offset": offset,
            "length": len(pcm),
            "loop": loop,
            "truncated": decoded.truncated,
            "pcm16_offset": len(pcm16),
            "pcm16_bytes": 2 * len(pcm),
            "pcm8_offset": len(pcm8),
            "pcm8_bytes": len(pcm),
        })
        pcm16 += spu_adpcm.to_pcm16_be(pcm)
        pcm8 += spu_adpcm.to_pcm8(pcm)
    articulations = []
    for number, art in enumerate(bank.articulations):
        loop = sample_loop(decoded_at[art.sample_addr], art.sample_addr, art.loop_addr)
        articulations.append({
            "index": number,
            "sample": offsets.index(art.sample_addr),
            "root_key": art.root_key,
            "fine_tune": art.fine_tune,
            "pitch": akao.root_pitch(art),
            "rate_hz": akao.root_rate_hz(art),
            "loop_start": None if loop is None else loop["start"],
            "adsr1": art.adsr1,
            "adsr2": art.adsr2,
        })
    stem = "res%04d_%d" % (resource, section)
    with open(os.path.join(out_dir, stem + ".s16"), "wb") as handle:
        handle.write(pcm16)
    with open(os.path.join(out_dir, stem + ".s8"), "wb") as handle:
        handle.write(pcm8)
    return {
        "resource": resource,
        "section": section,
        "kind": kind,
        "id": bank.header.id,
        "spu_dest_addr": bank.spu_dest_addr,
        "bank_id": bank.bank_id,
        "declared_sample_size": bank.declared_sample_size,
        "adpcm_bytes": len(bank.sample_data),
        "pcm16_bytes": len(pcm16),
        "pcm8_bytes": len(pcm8),
        "pcm16_file": stem + ".s16",
        "pcm8_file": stem + ".s8",
        "adpcm_sha1": hashlib.sha1(bank.sample_data).hexdigest(),
        "samples": samples,
        "articulations": articulations,
    }


#----------------------
# | song_entry
# | Description: Manifest entry for one song header.
# | Author: suinevere
# | Dependencies: akao.parse_song
# | Globals: N/A
# | Params: data -- resource bytes, offset -- song position, resource -- index, section -- section index
# | Returns: dict
#----------------------
def song_entry(data, offset, resource, section):
    song = akao.parse_song(data, offset)
    return {"resource": resource, "section": section, "id": song.header.id, "length": song.header.length,
            "reverb_type": song.header.reverb_type, "channel_mask": song.channel_mask & 0xFFFFFFFF,
            "channels": len(song.channel_offsets)}


#----------------------
# | resource_banks
# | Description: Splits one sound resource by kind into (section, bank offset, effect count) triples, recording songs and programs.
# | Author: suinevere
# | Dependencies: akao.parse_container, akao.parse_effect_table, akao.parse_header, song_entry
# | Globals: N/A
# | Params: data -- resource bytes, resource -- index, kind -- resource kind, report -- manifest being built
# | Returns: list of (section, bank offset, effect count or None)
#----------------------
def resource_banks(data, resource, kind, report):
    if kind == "program":
        report["programs"].append({"resource": resource, "section": 0, "id": akao.parse_header(data).id})
        return []
    if kind == "bank":
        return [(0, 0, None)]
    if kind == "weapon_sfx":
        entries, end = akao.parse_effect_table(data, 0)
        return [(0, end, len(entries))]
    sections = akao.parse_container(data)
    if kind == "sfx_set":
        out = []
        for number, offset in enumerate(sections):
            entries, end = akao.parse_effect_table(data, offset)
            out.append((number, end, len(entries)))
        return out
    if kind == "effect_set":
        report["programs"].append({"resource": resource, "section": 0, "id": akao.parse_header(data, sections[0]).id})
    else:
        for number, offset in enumerate(sections[:-1]):
            report["songs"].append(song_entry(data, offset, resource, number))
    return [(len(sections) - 1, sections[-1], None)]


#----------------------
# | totals
# | Description: Sums ADPCM, 16-bit and 8-bit PCM bytes over all banks, and again over banks with distinct sample data.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: banks -- manifest bank entries
# | Returns: dict
#----------------------
def totals(banks):
    seen = set()
    out = {"adpcm_bytes": 0, "pcm16_bytes": 0, "pcm8_bytes": 0, "unique_adpcm_bytes": 0, "unique_pcm16_bytes": 0, "unique_pcm8_bytes": 0}
    for bank in banks:
        for key in ("adpcm_bytes", "pcm16_bytes", "pcm8_bytes"):
            out[key] += bank[key]
        if bank["adpcm_sha1"] not in seen:
            seen.add(bank["adpcm_sha1"])
            for key in ("adpcm_bytes", "pcm16_bytes", "pcm8_bytes"):
                out["unique_" + key] += bank[key]
    return out


#----------------------
# | convert
# | Description: Converts every sound resource the reader returns, writing PCM files and manifest.json into out_dir.
# | Author: suinevere
# | Dependencies: SOUND_RESOURCES, resource_banks, convert_bank, akao.parse_bank, totals
# | Globals: N/A
# | Params: read -- callable mapping a resource index to bytes or None, out_dir -- output folder
# | Returns: manifest dict
#----------------------
def convert(read, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    report = {"banks": [], "songs": [], "programs": [], "missing": []}
    for resource, kind in SOUND_RESOURCES:
        data = read(resource)
        if not data:
            report["missing"].append(resource)
            continue
        for section, offset, effects in resource_banks(data, resource, kind, report):
            entry = convert_bank(akao.parse_bank(data, offset), resource, section, kind, out_dir)
            if effects is not None:
                entry["effects"] = effects
            report["banks"].append(entry)
    report["totals"] = totals(report["banks"])
    with open(os.path.join(out_dir, "manifest.json"), "w", encoding="utf-8", newline="\n") as handle:
        json.dump(report, handle, indent=1)
    return report


#----------------------
# | format_summary
# | Description: Formats per-bank and total ADPCM, 16-bit and 8-bit PCM sizes against the 512 KB sound RAM, plus loop convergence counts.
# | Author: suinevere
# | Dependencies: SOUND_RAM_BYTES
# | Globals: N/A
# | Params: report -- manifest dict
# | Returns: list of lines
#----------------------
def format_summary(report):
    lines = ["%-8s %-4s %-10s %4s %10s %10s %10s" % ("resource", "sec", "kind", "smp", "adpcm", "pcm16", "pcm8")]
    for bank in report["banks"]:
        lines.append("%-8d %-4d %-10s %4d %10d %10d %10d" % (bank["resource"], bank["section"], bank["kind"], len(bank["samples"]),
                                                          bank["adpcm_bytes"], bank["pcm16_bytes"], bank["pcm8_bytes"]))
    t = report["totals"]
    lines.append("TOTAL  %d banks: adpcm %d, pcm16 %d, pcm8 %d" % (len(report["banks"]), t["adpcm_bytes"], t["pcm16_bytes"], t["pcm8_bytes"]))
    lines.append("UNIQUE sample data: adpcm %d, pcm16 %d, pcm8 %d" % (t["unique_adpcm_bytes"], t["unique_pcm16_bytes"], t["unique_pcm8_bytes"]))
    loops = [s["loop"] for b in report["banks"] for s in b["samples"] if s["loop"]]
    settled = sorted(loop["passes"] for loop in loops if loop["converged"])
    lines.append("LOOPS %d looped samples, %d settled, %d in multi-pass cycles (passes min %d, median %d, max %d), %d truncated samples" % (
        len(loops), len(settled), sum(loop["period"] > 1 for loop in loops if loop["converged"]), settled[0] if settled else 0, settled[len(settled) // 2] if settled else 0, settled[-1] if settled else 0,
        sum(s["truncated"] for b in report["banks"] for s in b["samples"])))
    largest = max(report["banks"], key=lambda b: b["pcm16_bytes"], default=None)
    if largest is not None:
        lines.append("LARGEST bank res %d: adpcm %d, pcm16 %d (%.0f%% of sound RAM), pcm8 %d (%.0f%%)" % (
            largest["resource"], largest["adpcm_bytes"], largest["pcm16_bytes"], 100.0 * largest["pcm16_bytes"] / SOUND_RAM_BYTES,
            largest["pcm8_bytes"], 100.0 * largest["pcm8_bytes"] / SOUND_RAM_BYTES))
    return lines


#----------------------
# | read_live_resource
# | Description: Returns a live resource's bytes from the disc image, or None for an index that is out of range, dead or empty.
# | Author: suinevere
# | Dependencies: disc_image.read_resource
# | Globals: N/A
# | Params: image -- disc_image.DiscImage, table -- parsed resource table, index -- resource index
# | Returns: bytes or None
#----------------------
def read_live_resource(image, table, index):
    if index >= len(table) or not table[index].live or table[index].size <= 0:
        return None
    return disc_image.read_resource(image, table[index])


#----------------------
# | disc_reader
# | Description: Builds a resource reader over a disc image using the SKCDPOSE table location from convert_disc.
# | Author: suinevere
# | Dependencies: disc_image, read_live_resource, convert_disc.TABLE_LBA, convert_disc.TABLE_SIZE
# | Globals: N/A
# | Params: path -- disc image path
# | Returns: callable index -> bytes or None
#----------------------
def disc_reader(path):
    image = disc_image.DiscImage(path)
    blob = image.read_sectors(convert_disc.TABLE_LBA, (convert_disc.TABLE_SIZE + 2047) // 2048)[:convert_disc.TABLE_SIZE]
    return functools.partial(read_live_resource, image, disc_image.parse_resource_table(blob))


#----------------------
# | main
# | Description: Command-line entry: converts the disc's sound banks into the caller's chosen folder and prints the size summary.
# | Author: suinevere
# | Dependencies: disc_reader, convert, format_summary, convert_disc.default_image
# | Globals: N/A
# | Params: argv -- command-line arguments
# | Returns: process exit code
#----------------------
def main(argv=None):
    parser = argparse.ArgumentParser(description="Convert the PS1 sound banks to Saturn PCM")
    parser.add_argument("--image", default=convert_disc.default_image())
    parser.add_argument("--out", required=True)
    args = parser.parse_args(argv)
    report = convert(disc_reader(args.image), args.out)
    for line in format_summary(report):
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
