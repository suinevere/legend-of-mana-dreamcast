#----------------------
# | test_vmu_art.py
# | Description: The Dreamcast copy of the ranch ring menu's icon sheet (resource 0xBEE) shows our VMU drawing in the PocketStation's cell and is unchanged everywhere else.
# | Author: suinevere
# | Dependencies: port/tools/vmu_art.py, port/tools/scene_patches.py, port/tools/convert_disc.py, port/tools/disc_image.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import struct
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(REPO, "port", "tools"))

import convert_disc  # noqa: E402
import disc_image  # noqa: E402
import scene_patches  # noqa: E402
import vmu_art  # noqa: E402


@pytest.fixture(scope="module")
def sheet():
    path = convert_disc.default_image()
    if not os.path.exists(path):
        pytest.skip("no PS1 disc image (set LOM_PS1_IMAGE)")
    image = disc_image.DiscImage(path)
    blob = image.read_sectors(convert_disc.TABLE_LBA, (convert_disc.TABLE_SIZE + 2047) // 2048)[:convert_disc.TABLE_SIZE]
    table = disc_image.parse_resource_table(blob)
    return disc_image.read_resource(image, table[vmu_art.RING_SHEET_RESOURCE])


def _pixels(data):
    clut_len = struct.unpack_from("<I", data, 8)[0]
    start = 8 + clut_len + 12
    width = struct.unpack_from("<H", data, 8 + clut_len + 8)[0] * 2
    return [[(data[start + y * width + x // 2] >> (4 * (x & 1))) & 15 for x in range(width * 2)] for y in range(256)]


def test_the_grid_is_a_32_by_32_drawing_in_the_cells_palette():
    assert len(vmu_art.VMU_ICON) == 32 and all(len(row) == 32 for row in vmu_art.VMU_ICON)
    used = {int(c, 16) for row in vmu_art.VMU_ICON for c in row}
    assert used <= set(vmu_art.PALETTE_INDICES) | {0}
    assert len(used) > 5


def test_the_sheet_shows_the_vmu_in_the_pocketstation_cell_only(sheet):
    out = scene_patches.patch(vmu_art.RING_SHEET_RESOURCE, sheet)
    assert len(out) == len(sheet)
    clut_len = struct.unpack_from("<I", sheet, 8)[0]
    assert out[:8 + clut_len + 12] == sheet[:8 + clut_len + 12]
    before, after = _pixels(sheet), _pixels(out)
    x0, y0 = vmu_art.CELL_X, vmu_art.CELL_Y
    for y in range(256):
        for x in range(256):
            if x0 <= x < x0 + 32 and y0 <= y < y0 + 32:
                assert after[y][x] == int(vmu_art.VMU_ICON[y - y0][x - x0], 16), (x, y)
            else:
                assert after[y][x] == before[y][x], (x, y)
    palette = [struct.unpack_from("<H", sheet, 20 + (vmu_art.ICON * 16 + i) * 2)[0] for i in range(16)]
    assert all(palette[i] != 0 for i in vmu_art.PALETTE_INDICES)


def test_an_unexpected_sheet_is_refused():
    with pytest.raises(ValueError):
        vmu_art.patch_ring_sheet(b"\x10\x00\x00\x00\x09\x00\x00\x00" + bytes(64))
