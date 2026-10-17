/*----------------------
 | aica_stream_ring.c
 | Description: Pure ring arithmetic for the AICA stream: free space and the split of a write at the wrap.
 | Author: suinevere
 | Dependencies: aica_stream.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "aica_stream.h"

/*----------------------
 | aica_ring_free
 | Description: Frames writable from the write position before coming within guard frames of the play position; equal positions mean an empty ring.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: write / play -- ring positions, frames -- ring size, guard -- frames kept clear
 | Returns: frame count
 ----------------------*/
unsigned aica_ring_free(unsigned write, unsigned play, unsigned frames, unsigned guard)
{
    unsigned gap = (play + frames - write) % frames;

    if (gap == 0)
        gap = frames;
    return gap > guard ? gap - guard : 0u;
}

/*----------------------
 | aica_ring_split
 | Description: Splits a write of count frames at the ring's wrap.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: write -- ring position, count -- frames, frames -- ring size, first -- out frames before the wrap
 | Returns: frames after the wrap
 ----------------------*/
unsigned aica_ring_split(unsigned write, unsigned count, unsigned frames, unsigned *first)
{
    unsigned room = frames - write;

    *first = count < room ? count : room;
    return count - *first;
}
