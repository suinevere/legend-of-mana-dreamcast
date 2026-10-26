#----------------------
# | test_vmu_portraits.py
# | Description: GOSUB's 48x48 pet and egg portraits become 48x32 one-bit VMU screens: scaled to 32x32, centred, transparent blank, outlined, dark set and mid-tones one pixel in four; the archive from the user's disc gives 77 of them.
# | Author: suinevere
# | Dependencies: port/tools/vmu_portraits.py, port/tools/convert_disc.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os
import sys

import pytest

DC = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPO = os.path.dirname(DC)
sys.path.insert(0, os.path.join(REPO, "port", "tools"))

import convert_disc  # noqa: E402
import vmu_portraits  # noqa: E402

WHITE, BLACK, GREY = 0x7FFF, 0x8000, 0x3DEF


def _portrait(fill):
    pixels = bytearray(48 * 24)
    for y in range(48):
        for x in range(48):
            i = fill(x, y)
            pixels[y * 24 + x // 2] |= i << (4 * (x & 1))
    return bytes(pixels)


def _on(screen, x, y):
    return screen[y * 6 + x // 8] >> (7 - (x & 7)) & 1


def test_a_transparent_portrait_is_blank():
    screen = vmu_portraits.screen((0,) * 16, _portrait(lambda x, y: 0))
    assert screen == bytes(192)


def test_dark_is_set_and_white_is_clear_but_outlined_inside_a_centred_square():
    palette = (0, BLACK, WHITE) + (0,) * 13
    screen = vmu_portraits.screen(palette, _portrait(lambda x, y: 1 if x < 24 else 2))
    assert len(screen) == 192
    for y in range(32):
        for x in range(48):
            want = 1 if 8 <= x < 24 or (24 <= x < 40 and (y in (0, 31) or x == 39)) else 0
            assert _on(screen, x, y) == want, (x, y)


def test_a_mid_grey_sets_one_pixel_in_four_inside_its_outline():
    palette = (0, GREY) + (0,) * 14
    screen = vmu_portraits.screen(palette, _portrait(lambda x, y: 1))
    inner = [(x, y) for y in range(1, 31) for x in range(9, 39)]
    assert all(_on(screen, x, y) == (not ((x - 8) & 1) and not (y & 1)) for x, y in inner)


def test_the_disc_gives_77_screens_pets_then_eggs():
    path = convert_disc.default_image()
    if not os.path.exists(path):
        pytest.skip("no PS1 disc image (set LOM_PS1_IMAGE)")
    screens = vmu_portraits.disc_screens(path)
    assert len(screens) == vmu_portraits.PET_SCREENS + vmu_portraits.EGG_SCREENS == 77
    assert all(len(s) == 192 for s in screens)
    assert all(any(s[y * 6] or s[y * 6 + 5] for y in range(32)) is False for s in screens)
    assert sum(1 for s in screens if sum(bin(b).count("1") for b in s) > 100) > 70


def test_the_generated_source_holds_every_screen_in_order():
    screens = [bytes([k]) * 192 for k in range(3)]
    source = vmu_portraits.c_source(screens)
    assert "lom_vmu_pet_screens[3][192]" in source and "__attribute__((aligned(4)))" in source
    assert source.count("0x02") >= 192
