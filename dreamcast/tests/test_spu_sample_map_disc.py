#----------------------
# | test_spu_sample_map_disc.py
# | Description: Uploads every sound bank on the US disc through the C sample map and checks each sample decodes bit-exactly against port/tools/spu_adpcm.py, fits SPU RAM, and that the streamed-sample count holds.
# | Author: suinevere
# | Dependencies: tests/host/spu_decode_cli.c, ../port/sound/spu_sample_map.c, ../port/tools/convert_sound.py, ../port/tools/spu_adpcm.py, ../disc/us/LegendOfMana.bin
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import array
import os
import shutil
import subprocess
import sys
import tempfile

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(REPO, "port", "tools"))

import akao  # noqa: E402
import convert_disc  # noqa: E402
import convert_sound  # noqa: E402
import spu_adpcm  # noqa: E402

STREAMED_SAMPLES = 56


def _banks():
    image = convert_disc.default_image()
    if not os.path.isfile(image):
        pytest.skip("no US disc image")
    read = convert_sound.disc_reader(image)
    report = {"banks": [], "songs": [], "programs": [], "missing": []}
    for resource, kind in convert_sound.SOUND_RESOURCES:
        data = read(resource)
        if not data:
            continue
        for _, offset, _ in convert_sound.resource_banks(data, resource, kind, report):
            yield resource, akao.parse_bank(data, offset)


def test_every_bank_decodes_bit_exactly():
    cc = shutil.which("gcc") or shutil.which("cc")
    if cc is None:
        pytest.skip("no host C compiler")
    tmp = tempfile.mkdtemp(prefix="lom_spu_disc_")
    exe = os.path.join(tmp, "spu_decode_cli.exe")
    sound = os.path.join(REPO, "port", "sound")
    subprocess.run([cc, "-std=gnu11", "-O2", "-Wall", "-I", sound, "-o", exe,
                    os.path.join(DC, "tests", "host", "spu_decode_cli.c"), os.path.join(sound, "spu_sample_map.c")], check=True)
    banks = 0
    streamed = 0
    mismatches = []
    for resource, bank in _banks():
        banks += 1
        assert bank.spu_dest_addr + len(bank.sample_data) <= 0x80000, resource
        adpcm = os.path.join(tmp, "bank.adpcm")
        raw = os.path.join(tmp, "bank.raw")
        with open(adpcm, "wb") as handle:
            handle.write(bank.sample_data)
        offsets = sorted({art.sample_addr for art in bank.articulations})
        blocks = [str((bank.spu_dest_addr + offset) // 16) for offset in offsets]
        out = subprocess.run([exe, adpcm, "%x" % bank.spu_dest_addr, raw] + blocks, check=True,
                             capture_output=True, text=True).stdout.split("\n")
        image = array.array("h")
        with open(raw, "rb") as handle:
            image.frombytes(handle.read())
        for offset, line in zip(offsets, out):
            _, frames, is_streamed = (int(v) for v in line.split())
            pcm = spu_adpcm.decode(bank.sample_data, offset).pcm
            if not pcm and frames == -1:
                continue
            first = (bank.spu_dest_addr + offset) // 2 * 7 // 2
            if list(image[first:first + len(pcm)]) != pcm or frames != len(pcm):
                mismatches.append((resource, offset, frames, len(pcm)))
            streamed += is_streamed
    assert banks == 123
    assert not mismatches, mismatches[:10]
    assert streamed == STREAMED_SAMPLES
