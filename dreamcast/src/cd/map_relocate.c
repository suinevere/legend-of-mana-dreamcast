/*----------------------
 | map_relocate.c
 | Description: Load-time pointer rebasing for FIELD map files, which the PS1 uses in place at LOAD_BUFFER_ADDRESS but the Dreamcast loads elsewhere.
 | Author: suinevere
 | Dependencies: map_relocate.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "map_relocate.h"

#include <stddef.h>

/*----------------------
 | MAP_MAGIC_BYTES
 | Description: The eight bytes every SKmapDat map file starts with.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const char MAP_MAGIC_BYTES[8] = {'S', 'K', 'm', 'a', 'p', 'D', 'a', 't'};

/*----------------------
 | MAP_TEXEL_SIZE_OFFSET / MAP_TEXEL_POINTER_OFFSET
 | Description: Header offsets of the texel block's byte size and its PS1 pointer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MAP_TEXEL_SIZE_OFFSET 0x0Cu
#define MAP_TEXEL_POINTER_OFFSET 0x14u

/*----------------------
 | MAP_HEADER_BYTES
 | Description: Smallest buffer that can hold the header fields map_relocate reads.
 | Author: suinevere
 | Dependencies: MAP_TEXEL_POINTER_OFFSET
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MAP_HEADER_BYTES (MAP_TEXEL_POINTER_OFFSET + 4u)

/*----------------------
 | read_word
 | Description: Reads a little-endian 32-bit word, the byte order both the PS1 and the SH-4 use.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- first byte
 | Returns: the word
 ----------------------*/
static uint32_t read_word(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/*----------------------
 | write_word
 | Description: Writes a little-endian 32-bit word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- first byte, value -- word to store
 | Returns: N/A
 ----------------------*/
static void write_word(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

/*----------------------
 | is_map
 | Description: Whether a buffer starts with the SKmapDat magic.
 | Author: suinevere
 | Dependencies: MAP_MAGIC_BYTES
 | Globals: N/A
 | Params: map -- buffer
 | Returns: 1 for a map, 0 otherwise
 ----------------------*/
static int is_map(const uint8_t *map)
{
    unsigned i;

    for (i = 0; i < sizeof(MAP_MAGIC_BYTES); i++)
    {
        if (map[i] != (uint8_t)MAP_MAGIC_BYTES[i])
        {
            return 0;
        }
    }
    return 1;
}

int map_relocate(uint8_t *map, uint32_t capacity)
{
    uint32_t texel_pointer;
    uint32_t scan_end;
    uint32_t span;
    uint32_t real_base;
    uint32_t offset;
    uint32_t value;
    int rebased;

    if (capacity < MAP_HEADER_BYTES || !is_map(map))
    {
        return -1;
    }
    texel_pointer = read_word(map + MAP_TEXEL_POINTER_OFFSET);
    scan_end = capacity;
    span = capacity;
    if (texel_pointer != 0)
    {
        if (texel_pointer < MAP_RELOCATE_PS1_BASE || texel_pointer - MAP_RELOCATE_PS1_BASE >= capacity)
        {
            return 0;
        }
        scan_end = texel_pointer - MAP_RELOCATE_PS1_BASE;
        span = scan_end + read_word(map + MAP_TEXEL_SIZE_OFFSET) + MAP_RELOCATE_TRAILER_SLACK;
        if (span > capacity)
        {
            span = capacity;
        }
    }
    real_base = (uint32_t)(uintptr_t)map;
    rebased = 0;
    for (offset = 0; offset + 4u <= scan_end; offset += 4u)
    {
        value = read_word(map + offset);
        if (value >= MAP_RELOCATE_PS1_BASE && value - MAP_RELOCATE_PS1_BASE < span)
        {
            write_word(map + offset, value - MAP_RELOCATE_PS1_BASE + real_base);
            rebased++;
        }
    }
    return rebased;
}
