/*----------------------
 | rgb24_display.c
 | Description: Converts a PS1 24-bit display area in the VRAM mirror to RGB565 rows; a 24-bit pixel's three bytes straddle halfwords, so each row is read as bytes.
 | Author: suinevere
 | Dependencies: rgb24_display.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "rgb24_display.h"

/*----------------------
 | VRAM_ROWS
 | Description: Rows in the PS1 VRAM mirror, where reads wrap.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VRAM_ROWS 512

/*----------------------
 | row_byte
 | Description: One byte of a mirror row, little-endian within its halfword, with the byte offset wrapping at the row's end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: row -- mirror row, row_bytes -- its length in bytes, offset -- byte offset, any size
 | Returns: the byte
 ----------------------*/
static unsigned row_byte(const uint16_t *row, unsigned row_bytes, unsigned offset)
{
    unsigned at = offset % row_bytes;

    return (row[at >> 1] >> ((at & 1u) * 8u)) & 0xFFu;
}

/*----------------------
 | rgb24_to_rgb565
 | Description: Reads R, G, B bytes per pixel from the mirror starting at halfword column x_hw and writes RGB565; rows inside the mirror are read straight through, rows crossing its edge byte by byte with wrapping.
 |              Word-aligned rows convert four pixels from three little-endian words, since the per-byte loop cost a 24-bit movie about 10 ms a frame.
 | Author: suinevere
 | Dependencies: row_byte
 | Globals: N/A
 | Params: vram, vram_pitch_hw, x_hw, y, width_px, height, dst, dst_pitch_px -- as rgb24_display.h describes
 | Returns: N/A
 ----------------------*/
void rgb24_to_rgb565(const uint16_t *vram, unsigned vram_pitch_hw, unsigned x_hw, unsigned y, unsigned width_px, unsigned height,
                     uint16_t *dst, unsigned dst_pitch_px)
{
    unsigned row_bytes = vram_pitch_hw * 2u;
    unsigned r, p;

    for (r = 0; r < height; r++)
    {
        const uint16_t *row = vram + ((y + r) % VRAM_ROWS) * vram_pitch_hw;
        uint16_t *out = dst + r * dst_pitch_px;
        unsigned at = x_hw * 2u;

        if (at + width_px * 3u <= row_bytes)
        {
            const uint8_t *src = (const uint8_t *)row + at;

            p = 0;
            if (((uintptr_t)src & 3u) == 0 && ((uintptr_t)out & 3u) == 0)
            {
                const uint32_t *word = (const uint32_t *)src;
                uint32_t *pair = (uint32_t *)out;

                for (; p + 4u <= width_px; p += 4u, word += 3, pair += 2)
                {
                    uint32_t a = word[0];
                    uint32_t b = word[1];
                    uint32_t c = word[2];

                    __builtin_prefetch(word + 16);
                    pair[0] = ((a & 0xF8u) << 8) | ((a >> 5) & 0x7E0u) | ((a >> 19) & 0x1Fu) |
                              ((((a >> 16) & 0xF800u) | ((b << 3) & 0x7E0u) | ((b >> 11) & 0x1Fu)) << 16);
                    pair[1] = ((b >> 8) & 0xF800u) | ((b >> 21) & 0x7E0u) | ((c >> 3) & 0x1Fu) |
                              (((c & 0xF800u) | ((c >> 13) & 0x7E0u) | (c >> 27)) << 16);
                }
                src += p * 3u;
            }
            for (; p < width_px; p++, src += 3)
            {
                out[p] = (uint16_t)(((unsigned)(src[0] >> 3) << 11) | ((unsigned)(src[1] >> 2) << 5) | (unsigned)(src[2] >> 3));
            }
            continue;
        }
        for (p = 0; p < width_px; p++, at += 3u)
        {
            unsigned red = row_byte(row, row_bytes, at);
            unsigned green = row_byte(row, row_bytes, at + 1u);
            unsigned blue = row_byte(row, row_bytes, at + 2u);

            out[p] = (uint16_t)(((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3));
        }
    }
}
