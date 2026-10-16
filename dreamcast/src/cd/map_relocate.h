/*----------------------
 | map_relocate.h
 | Description: Rebases the absolute PS1 pointers inside a loaded FIELD SKmapDat map onto the address it actually landed at on the Dreamcast.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_MAP_RELOCATE_H
#define LOM_MAP_RELOCATE_H

#include <stdint.h>

/*----------------------
 | MAP_RELOCATE_PS1_BASE
 | Description: PS1 address every map is built for (LOAD_BUFFER_ADDRESS); every non-null pointer in a map file is relative to it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MAP_RELOCATE_PS1_BASE 0x80180000u

/*----------------------
 | MAP_RELOCATE_MAX_BYTES
 | Description: Largest map the PS1 load region holds, LOAD_BUFFER_ADDRESS up to CD_STREAM_STAGING_ADDRESS at 0x801DA000.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MAP_RELOCATE_MAX_BYTES 0x5A000u

/*----------------------
 | MAP_RELOCATE_TRAILER_SLACK
 | Description: Bytes allowed past the texel block for the unread trailer, which is at most 0x599 bytes in every known map format.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define MAP_RELOCATE_TRAILER_SLACK 0x800u

/*----------------------
 | map_relocate
 | Description: Rebases every 4-aligned word before the texel block whose value lies inside the map's PS1 address span onto the map's real address.
 |              Matches the Saturn map converter's exact pointer set on maps 195 and 578, and leaves texels and the trailer untouched.
 | Author: suinevere
 | Dependencies: MAP_RELOCATE_PS1_BASE, MAP_RELOCATE_TRAILER_SLACK
 | Globals: N/A
 | Params: map -- loaded map bytes, capacity -- bytes the destination buffer holds
 | Returns: words rebased, or -1 when the buffer is not an SKmapDat map
 ----------------------*/
int map_relocate(uint8_t *map, uint32_t capacity);

#endif
