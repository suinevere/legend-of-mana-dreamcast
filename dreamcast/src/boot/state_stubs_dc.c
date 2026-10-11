/*----------------------
 | state_stubs_dc.c
 | Description: Entry points of overlays the Dreamcast boot path reaches before they are ported; movies return at once, as on the Saturn (saturn/src/boot/state_stubs.c).
 | Author: suinevere
 | Dependencies: lom_prefix.h, overlays/movie/movie.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "overlays/movie/movie.h"

/*----------------------
 | movie_play
 | Description: Movies are not ported; returns at once so the state loop moves on to TITLE.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: movie_index -- ignored
 | Returns: N/A
 ----------------------*/
void movie_play(u16 movie_index)
{
    (void)movie_index;
}
