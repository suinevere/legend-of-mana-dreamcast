/*----------------------
 | main_stubs.c
 | Description: Saturn and Dreamcast stand-ins for the PS1 main executable's libc copy; the Saturn's silent sound is in akao_stubs.c.
 | Author: suinevere
 | Dependencies: lom_prefix.h, decomp main/ and common/ headers
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "common/saved_game.h"
#include "main/main.h"
#include "main/game_state.h"

/*----------------------
 | bcopy
 | Description: Overlapping-safe byte copy with the Psy-Q argument order (source first).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: src -- source, dst -- destination, count -- bytes
 | Returns: dst
 ----------------------*/
void *bcopy(const unsigned char *src, unsigned char *dst, int count)
{
    int i;

    if (dst < src)
    {
        for (i = 0; i < count; i++)
        {
            dst[i] = src[i];
        }
    }
    else
    {
        for (i = count - 1; i >= 0; i--)
        {
            dst[i] = src[i];
        }
    }
    return dst;
}
