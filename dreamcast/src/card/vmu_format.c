/*----------------------
 | vmu_format.c
 | Description: Pure conversions between PS1 memory-card files and VMU files, kept free of KallistiOS so host tests can run them.
 | Author: suinevere
 | Dependencies: vmu_format.h
 | Globals: s_wordmark_glyphs
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "vmu_format.h"

/*----------------------
 | VMU_HEADER_BYTES / VMU_ICON_BYTES / VMU_FILE_OVERHEAD_BLOCKS / VMU_BLOCKS_PER_PS1_BLOCK
 | Description: VMS header and palette size, one 32x32 4bpp icon frame, and the VMU blocks budgeted per file and per compressed PS1 block, 12 for a two-block save;
 |              the largest finished save takes 11, and an over-budget save fails its write rather than halting.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VMU_HEADER_BYTES 128
#define VMU_ICON_BYTES 512
#define VMU_FILE_OVERHEAD_BLOCKS 2
#define VMU_BLOCKS_PER_PS1_BLOCK 5

/*----------------------
 | PS1_ICON_FLAG_OFFSET / PS1_CLUT_OFFSET / PS1_FRAME_OFFSET / PS1_FRAME_BYTES
 | Description: Where a PS1 save's icon frame count, CLUT and 16x16 4bpp frames sit in its first block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PS1_ICON_FLAG_OFFSET 2
#define PS1_CLUT_OFFSET 0x60
#define PS1_FRAME_OFFSET 0x80
#define PS1_FRAME_BYTES 128

/*----------------------
 | s_wordmark_glyphs
 | Description: 5x7 glyphs for M, A, N, one row per byte with bit 4 the leftmost pixel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const unsigned char s_wordmark_glyphs[3][7] = {
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
};

/*----------------------
 | fnv1a
 | Description: 32-bit FNV-1a hash of a string.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: text -- NUL-terminated string
 | Returns: hash
 ----------------------*/
static unsigned int fnv1a(const char *text)
{
    unsigned int hash = 2166136261u;

    while (*text != 0)
    {
        hash = (hash ^ (unsigned char)*text++) * 16777619u;
    }
    return hash;
}

/*----------------------
 | vmu_name_for_ps1
 | Description: VMU file name for a PS1 card file: "LOM" plus the name's suffix after the product code, uppercased and cut to fit, else "LOMX" plus a hash.
 | Author: suinevere
 | Dependencies: fnv1a
 | Globals: N/A
 | Params: ps1_name -- PS1 file name without the "bu00:" device, out -- 13-byte destination
 | Returns: N/A
 ----------------------*/
void vmu_name_for_ps1(const char *ps1_name, char out[VMU_NAME_LEN + 1])
{
    static const char hex[] = "0123456789ABCDEF";
    const char *product = LOM_PS1_PRODUCT;
    const char *suffix = ps1_name;
    unsigned int hash;
    int i;

    while (*product != 0 && *suffix == *product)
    {
        product++;
        suffix++;
    }
    out[0] = 'L';
    out[1] = 'O';
    out[2] = 'M';
    if (*product != 0)
    {
        hash = fnv1a(ps1_name);
        out[3] = 'X';
        for (i = 0; i < 8; i++)
        {
            out[4 + i] = hex[(hash >> (28 - 4 * i)) & 0xF];
        }
        out[12] = 0;
        return;
    }
    for (i = 3; i < VMU_NAME_LEN && *suffix != 0; i++, suffix++)
    {
        out[i] = (*suffix >= 'a' && *suffix <= 'z') ? (char)(*suffix - 'a' + 'A') : *suffix;
    }
    out[i] = 0;
}

/*----------------------
 | vmu_ps1_name_from_desc
 | Description: The PS1 file name a package's long description holds, cut at the first space or NUL, since VMS headers pad it with spaces.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: desc -- description field, desc_bytes -- its size, out -- 21-byte destination
 | Returns: N/A
 ----------------------*/
void vmu_ps1_name_from_desc(const char *desc, int desc_bytes, char out[21])
{
    int i;

    for (i = 0; i < desc_bytes && i < 20 && desc[i] != 0 && desc[i] != ' '; i++)
    {
        out[i] = desc[i];
    }
    out[i] = 0;
}

/*----------------------
 | vmu_icon_from_ps1
 | Description: Converts a PS1 save's icon to VMU form: ARGB4444 palette with colour 0x0000 transparent, and each 16x16 frame doubled to 32x32 with the left pixel in the high nibble.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ps1_file -- first block of the PS1 file, palette -- 16 colours out, frames -- up to three 512-byte frames out
 | Returns: frame count 1..3, or 0 when the file has no "SC" header
 ----------------------*/
int vmu_icon_from_ps1(const unsigned char *ps1_file, unsigned short palette[16], unsigned char frames[3 * 512])
{
    int count;
    int i;
    int f;
    int x;
    int y;

    if (ps1_file[0] != 'S' || ps1_file[1] != 'C')
    {
        return 0;
    }
    count = ps1_file[PS1_ICON_FLAG_OFFSET] & 0x0F;
    if (count < 1 || count > 3)
    {
        count = 1;
    }
    for (i = 0; i < 16; i++)
    {
        unsigned int c = ps1_file[PS1_CLUT_OFFSET + 2 * i] | (ps1_file[PS1_CLUT_OFFSET + 2 * i + 1] << 8);
        unsigned int r = (c & 0x1F) >> 1;
        unsigned int g = ((c >> 5) & 0x1F) >> 1;
        unsigned int b = ((c >> 10) & 0x1F) >> 1;

        palette[i] = (unsigned short)(c == 0 ? 0 : (0xF000 | (r << 8) | (g << 4) | b));
    }
    for (f = 0; f < count; f++)
    {
        const unsigned char *src = ps1_file + PS1_FRAME_OFFSET + PS1_FRAME_BYTES * f;
        unsigned char *dst = frames + VMU_ICON_BYTES * f;

        for (y = 0; y < 32; y++)
        {
            for (x = 0; x < 32; x += 2)
            {
                unsigned char byte = src[(y / 2) * 8 + (x / 2) / 2];
                unsigned char pixel = ((x / 2) & 1) ? (byte >> 4) : (byte & 0x0F);

                dst[y * 16 + x / 2] = (unsigned char)((pixel << 4) | pixel);
            }
        }
    }
    return count;
}

/*----------------------
 | vmu_blocks_for_file
 | Description: VMU blocks a VMS package of the given data size and icon frame count occupies.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: data_bytes -- payload size, icon_frames -- icon frame count
 | Returns: block count
 ----------------------*/
unsigned int vmu_blocks_for_file(unsigned int data_bytes, int icon_frames)
{
    unsigned int total = VMU_HEADER_BYTES + VMU_ICON_BYTES * (unsigned int)icon_frames + data_bytes;

    return (total + VMU_BLOCK_BYTES - 1) / VMU_BLOCK_BYTES;
}

/*----------------------
 | vmu_pkg_crc
 | Description: The CRC a VMS package header carries, CRC-16 with polynomial 0x1021 from zero, over the package with its CRC field zeroed; KallistiOS keeps its own copy private.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: buf -- package bytes, size -- byte count
 | Returns: 16-bit CRC
 ----------------------*/
unsigned short vmu_pkg_crc(const unsigned char *buf, int size)
{
    unsigned int crc = 0;
    int i;
    int bit;

    for (i = 0; i < size; i++)
    {
        crc ^= (unsigned int)buf[i] << 8;
        for (bit = 0; bit < 8; bit++)
        {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
        }
    }
    return (unsigned short)crc;
}

/*----------------------
 | ps1_blocks_free
 | Description: Free blocks the virtual 15-block PS1 card reports: what the VMU's free space can hold compressed, never more than the PS1 card has left.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: vmu_free_blocks -- free VMU blocks, ps1_used_blocks -- PS1 blocks our files use
 | Returns: 0..15
 ----------------------*/
int ps1_blocks_free(int vmu_free_blocks, int ps1_used_blocks)
{
    int room = (vmu_free_blocks - VMU_FILE_OVERHEAD_BLOCKS) / VMU_BLOCKS_PER_PS1_BLOCK;
    int left = PS1_CARD_BLOCKS - ps1_used_blocks;

    if (vmu_free_blocks < VMU_FILE_OVERHEAD_BLOCKS)
    {
        room = 0;
    }
    if (room > left)
    {
        room = left;
    }
    return room < 0 ? 0 : room;
}

/*----------------------
 | vmu_lcd_wordmark
 | Description: Draws "MANA" in 2x-scaled 5x7 glyphs, centred on the 48x32 1bpp VMU screen, with bit 7 of each byte the leftmost pixel.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_wordmark_glyphs
 | Params: out -- 192-byte bitmap
 | Returns: N/A
 ----------------------*/
void vmu_lcd_wordmark(unsigned char out[192])
{
    static const unsigned char letters[4] = {0, 1, 2, 1};
    int i;
    int g;
    int row;
    int col;

    for (i = 0; i < 192; i++)
    {
        out[i] = 0;
    }
    for (g = 0; g < 4; g++)
    {
        for (row = 0; row < 14; row++)
        {
            for (col = 0; col < 10; col++)
            {
                if ((s_wordmark_glyphs[letters[g]][row / 2] >> (4 - col / 2)) & 1)
                {
                    int x = 2 + g * 11 + col;
                    int y = 9 + row;

                    out[y * 6 + x / 8] |= (unsigned char)(0x80 >> (x % 8));
                }
            }
        }
    }
}
