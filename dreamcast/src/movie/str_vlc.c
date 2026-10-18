/*----------------------
 | str_vlc.c
 | Description: Version-2 STR bitstream decoder: 10-bit DC per block, MPEG-1 Table B.14 AC codes, 6+10-bit escapes, into MDEC run-length halfwords.
 | Author: suinevere
 | Dependencies: str_vlc.h, mdec.h
 | Globals: s_codes, s_lookup
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "str_vlc.h"
#include "mdec.h"

/*----------------------
 | STR_VERSION / STR_MAGIC
 | Description: Version and magic halfwords of a version-2 frame header.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STR_VERSION 2
#define STR_MAGIC 0x3800

/*----------------------
 | STR_END_OF_FRAME
 | Description: 10-bit code in a block's DC position that ends a version-2 frame's data.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STR_END_OF_FRAME 0x1FF

/*----------------------
 | ENTRY_EOB_LEN / ENTRY_ESCAPE_LEN
 | Description: Code lengths of the two lookup entries with run and level zero: end of block and escape.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ENTRY_EOB_LEN 2
#define ENTRY_ESCAPE_LEN 6

/*----------------------
 | VlcCode
 | Description: One AC code as a bit string without its sign bit, with its run and level.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    const char *code;
    uint8_t run;
    uint8_t level;
} VlcCode;

/*----------------------
 | s_codes
 | Description: ISO 11172-2 Table B.14 with "11" as run 0 level 1, plus EOB "10" and escape "000001" as run 0 level 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const VlcCode s_codes[] = {
    {"10", 0, 0}, {"000001", 0, 0},
    {"11", 0, 1}, {"011", 1, 1}, {"0100", 0, 2}, {"0101", 2, 1}, {"00101", 0, 3}, {"00111", 3, 1}, {"00110", 4, 1},
    {"000110", 1, 2}, {"000111", 5, 1}, {"000101", 6, 1}, {"000100", 7, 1}, {"0000110", 0, 4}, {"0000100", 2, 2},
    {"0000111", 8, 1}, {"0000101", 9, 1}, {"00100110", 0, 5}, {"00100001", 0, 6}, {"00100101", 1, 3},
    {"00100100", 3, 2}, {"00100111", 10, 1}, {"00100011", 11, 1}, {"00100010", 12, 1}, {"00100000", 13, 1},
    {"0000001010", 0, 7}, {"0000001100", 1, 4}, {"0000001011", 2, 3}, {"0000001111", 4, 2}, {"0000001001", 5, 2},
    {"0000001110", 14, 1}, {"0000001101", 15, 1}, {"0000001000", 16, 1}, {"000000011101", 0, 8},
    {"000000011000", 0, 9}, {"000000010011", 0, 10}, {"000000010000", 0, 11}, {"000000011011", 1, 5},
    {"000000010100", 2, 4}, {"000000011100", 3, 3}, {"000000010010", 4, 3}, {"000000011110", 6, 2},
    {"000000010101", 7, 2}, {"000000010001", 8, 2}, {"000000011111", 17, 1}, {"000000011010", 18, 1},
    {"000000011001", 19, 1}, {"000000010111", 20, 1}, {"000000010110", 21, 1}, {"0000000011010", 0, 12},
    {"0000000011001", 0, 13}, {"0000000011000", 0, 14}, {"0000000010111", 0, 15}, {"0000000010110", 1, 6},
    {"0000000010101", 1, 7}, {"0000000010100", 2, 5}, {"0000000010011", 3, 4}, {"0000000010010", 5, 3},
    {"0000000010001", 9, 2}, {"0000000010000", 10, 2}, {"0000000011111", 22, 1}, {"0000000011110", 23, 1},
    {"0000000011101", 24, 1}, {"0000000011100", 25, 1}, {"0000000011011", 26, 1}, {"00000000011111", 0, 16},
    {"00000000011110", 0, 17}, {"00000000011101", 0, 18}, {"00000000011100", 0, 19}, {"00000000011011", 0, 20},
    {"00000000011010", 0, 21}, {"00000000011001", 0, 22}, {"00000000011000", 0, 23}, {"00000000010111", 0, 24},
    {"00000000010110", 0, 25}, {"00000000010101", 0, 26}, {"00000000010100", 0, 27}, {"00000000010011", 0, 28},
    {"00000000010010", 0, 29}, {"00000000010001", 0, 30}, {"00000000010000", 0, 31}, {"000000000011000", 0, 32},
    {"000000000010111", 0, 33}, {"000000000010110", 0, 34}, {"000000000010101", 0, 35}, {"000000000010100", 0, 36},
    {"000000000010011", 0, 37}, {"000000000010010", 0, 38}, {"000000000010001", 0, 39}, {"000000000010000", 0, 40},
    {"000000000011111", 1, 8}, {"000000000011110", 1, 9}, {"000000000011101", 1, 10}, {"000000000011100", 1, 11},
    {"000000000011011", 1, 12}, {"000000000011010", 1, 13}, {"000000000011001", 1, 14},
    {"0000000000010011", 1, 15}, {"0000000000010010", 1, 16}, {"0000000000010001", 1, 17},
    {"0000000000010000", 1, 18}, {"0000000000010100", 6, 3}, {"0000000000011010", 11, 2},
    {"0000000000011001", 12, 2}, {"0000000000011000", 13, 2}, {"0000000000010111", 14, 2},
    {"0000000000010110", 15, 2}, {"0000000000010101", 16, 2}, {"0000000000011111", 27, 1},
    {"0000000000011110", 28, 1}, {"0000000000011101", 29, 1}, {"0000000000011100", 30, 1},
    {"0000000000011011", 31, 1}};

/*----------------------
 | s_lookup
 | Description: Code entry for every value of the next 16 bits: length in bits 0-4, level in 5-10, run in 11-15; zero is invalid.
 | Author: suinevere
 | Dependencies: str_vlc_init
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint16_t s_lookup[65536];

/*----------------------
 | BitReader
 | Description: Little-endian 16-bit words read most significant bit first into a left-aligned 32-bit buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    const uint16_t *src;
    const uint16_t *end;
    uint32_t bits;
    int count;
} BitReader;

/*----------------------
 | str_vlc_init
 | Description: Fills every lookup slot whose top bits match a code with that code's entry.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_codes, s_lookup
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void str_vlc_init(void)
{
    unsigned i, len, prefix, first, slot;

    for (i = 0; i < 65536u; i++)
    {
        s_lookup[i] = 0;
    }
    for (i = 0; i < sizeof(s_codes) / sizeof(s_codes[0]); i++)
    {
        prefix = 0;
        for (len = 0; s_codes[i].code[len] != 0; len++)
        {
            prefix = (prefix << 1) | (unsigned)(s_codes[i].code[len] - '0');
        }
        first = prefix << (16 - len);
        for (slot = 0; slot < (1u << (16 - len)); slot++)
        {
            s_lookup[first + slot] = (uint16_t)(len | ((unsigned)s_codes[i].level << 5) | ((unsigned)s_codes[i].run << 11));
        }
    }
}

/*----------------------
 | peek_bits
 | Description: The next n bits (n at most 17) without consuming them; past the end the stream reads as zeros.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: r -- reader, n -- bit count
 | Returns: the bits right-aligned
 ----------------------*/
static uint32_t peek_bits(BitReader *r, int n)
{
    while (r->count < n)
    {
        uint32_t word = r->src < r->end ? *r->src++ : 0;

        r->bits |= word << (16 - r->count);
        r->count += 16;
    }
    return r->bits >> (32 - n);
}

/*----------------------
 | take_bits
 | Description: Reads and consumes the next n bits.
 | Author: suinevere
 | Dependencies: peek_bits
 | Globals: N/A
 | Params: r -- reader, n -- bit count, 1..16
 | Returns: the bits right-aligned
 ----------------------*/
static uint32_t take_bits(BitReader *r, int n)
{
    uint32_t v = peek_bits(r, n);

    r->bits <<= n;
    r->count -= n;
    return v;
}

/*----------------------
 | decode_block
 | Description: Writes one block's DC halfword, its AC codes and its EOB, stopping at limit.
 | Author: suinevere
 | Dependencies: take_bits, peek_bits
 | Globals: s_lookup
 | Params: r -- reader, out -- halfword output, at -- next halfword index, advanced past what was written, limit -- halfwords allowed, qscale -- frame quant scale
 | Returns: 0, 1 at the end-of-frame code, or -1 on an invalid code or reaching limit mid-block
 ----------------------*/
static int decode_block(BitReader *r, uint16_t *out, unsigned *at, unsigned limit, unsigned qscale)
{
    unsigned entry, len, run, level, dc;

    if (*at >= limit)
    {
        return -1;
    }
    dc = take_bits(r, 10);
    if (dc == STR_END_OF_FRAME)
    {
        return 1;
    }
    out[(*at)++] = (uint16_t)((qscale << 10) | dc);
    for (;;)
    {
        if (*at >= limit)
        {
            return -1;
        }
        entry = s_lookup[peek_bits(r, 16)];
        len = entry & 0x1F;
        run = entry >> 11;
        level = (entry >> 5) & 0x3F;
        if (len == 0)
        {
            return -1;
        }
        take_bits(r, (int)len);
        if (level == 0 && len == ENTRY_EOB_LEN)
        {
            out[(*at)++] = MDEC_EOB;
            return 0;
        }
        if (level == 0)
        {
            run = take_bits(r, 6);
            level = take_bits(r, 10);
        }
        else if (take_bits(r, 1))
        {
            level = (0x400 - level) & 0x3FF;
        }
        out[(*at)++] = (uint16_t)((run << 10) | level);
    }
}

/*----------------------
 | str_vlc_decode_frame
 | Description: Writes the command word, then blocks until the end-of-frame code or one halfword short of the header's word count, then EOB padding.
 | Author: suinevere
 | Dependencies: decode_block
 | Globals: N/A
 | Params: bitstream, bitstream_halfwords -- frame data, dst, dst_words -- output and its capacity
 | Returns: 0, or -1 with the output padded by EOB
 ----------------------*/
int str_vlc_decode_frame(const uint16_t *bitstream, unsigned bitstream_halfwords, uint32_t *dst, unsigned dst_words)
{
    BitReader r;
    uint16_t *out = (uint16_t *)&dst[1];
    unsigned words, limit;
    unsigned at = 0;
    int result = 0;

    if (dst_words < 1)
    {
        return -1;
    }
    words = bitstream_halfwords >= 4 ? bitstream[0] : 0;
    if (bitstream_halfwords < 4 || bitstream[1] != STR_MAGIC || bitstream[3] != STR_VERSION)
    {
        words = 0;
        result = -1;
    }
    if (words + 1 > dst_words)
    {
        words = dst_words - 1;
        result = -1;
    }
    limit = 2 * words;
    r.src = bitstream + 4;
    r.end = bitstream + bitstream_halfwords;
    r.bits = 0;
    r.count = 0;
    while (result == 0 && at + 1 < limit)
    {
        result = decode_block(&r, out, &at, limit, bitstream[2] & 0x3Fu);
    }
    if (result > 0)
    {
        result = 0;
    }
    dst[0] = 0x38000000u | words;
    while (at < limit)
    {
        out[at++] = MDEC_EOB;
    }
    return result;
}
