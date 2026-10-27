/*----------------------
 | boot_descramble.c
 | Description: Undoes the slice scrambling a CD-R's boot binary carries: the file is read in order, each 32-byte slice going to the place a seeded shuffle picks, in 2 MB chunks halving down to 32 bytes, then a final partial slice.
 | Author: suinevere
 | Dependencies: boot_descramble.h, string.h
 | Globals: s_seed, s_slices
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <string.h>

#include "boot_descramble.h"

/*----------------------
 | BOOT_SLICE / BOOT_FIRST_CHUNK / BOOT_MAX_SLICES
 | Description: Slice bytes, the chunk size the shuffle starts from, and the most slices a chunk of a binary up to BOOT_DESCRAMBLE_MAX_BYTES holds.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BOOT_SLICE 32u
#define BOOT_FIRST_CHUNK (2048u * 1024u)
#define BOOT_MAX_SLICES 8192u

/*----------------------
 | s_seed / s_slices
 | Description: The shuffle's random state and the slice order of the current chunk.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static uint32_t s_seed;
static uint16_t s_slices[BOOT_MAX_SLICES];

/*----------------------
 | boot_random
 | Description: The scrambler's generator: steps the seed and returns a 16-bit value of at least 0xC000.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_seed
 | Params: N/A
 | Returns: the next value
 ----------------------*/
static uint32_t boot_random(void)
{
    s_seed = (s_seed * 2109u + 9273u) & 0x7FFFu;
    return (s_seed + 0xC000u) & 0xFFFFu;
}

/*----------------------
 | boot_descramble_chunk
 | Description: Places one chunk's slices, read in order from src, where the shuffle sends them in dst.
 | Author: suinevere
 | Dependencies: boot_random
 | Globals: s_slices
 | Params: src -- the chunk's scrambled bytes, dst -- where the chunk goes, chunk -- its size, a power of two of at least 32
 | Returns: N/A
 ----------------------*/
static void boot_descramble_chunk(const uint8_t *src, uint8_t *dst, uint32_t chunk)
{
    uint32_t count = chunk / BOOT_SLICE;
    uint32_t i;
    uint32_t pick;
    uint16_t swap;

    for (i = 0; i < count; i++)
    {
        s_slices[i] = (uint16_t)i;
    }
    for (i = count; i-- > 0;)
    {
        pick = (boot_random() * i) >> 16;
        swap = s_slices[i];
        s_slices[i] = s_slices[pick];
        s_slices[pick] = swap;
        memcpy(dst + BOOT_SLICE * s_slices[i], src, BOOT_SLICE);
        src += BOOT_SLICE;
    }
}

/*----------------------
 | boot_descramble
 | Description: Descrambles a whole boot binary, seeding the shuffle with its size.
 | Author: suinevere
 | Dependencies: boot_descramble_chunk
 | Globals: s_seed
 | Params: src -- the scrambled file, dst -- the binary, not overlapping src, size -- bytes in both
 | Returns: 0, or -1 when size passes BOOT_DESCRAMBLE_MAX_BYTES
 ----------------------*/
int boot_descramble(const uint8_t *src, uint8_t *dst, uint32_t size)
{
    uint32_t chunk;

    if (size > BOOT_DESCRAMBLE_MAX_BYTES)
    {
        return -1;
    }
    s_seed = size & 0xFFFFu;
    for (chunk = BOOT_FIRST_CHUNK; chunk >= BOOT_SLICE; chunk >>= 1)
    {
        while (size >= chunk)
        {
            boot_descramble_chunk(src, dst, chunk);
            src += chunk;
            dst += chunk;
            size -= chunk;
        }
    }
    memcpy(dst, src, size);
    return 0;
}
