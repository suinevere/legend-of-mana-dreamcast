/*----------------------
 | spu_shadow.c
 | Description: The PS1 SPU register shadow AKAO writes instead of the hardware, and change detection against the last flushed copy.
 | Author: suinevere
 | Dependencies: spu_shadow.h
 | Globals: lom_spu_shadow, spu_shadow_key_events
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "spu_shadow.h"

volatile unsigned short lom_spu_shadow[SPU_SHADOW_HALFWORDS];
unsigned short spu_shadow_key_events[SPU_SHADOW_KEY_COUNT];

/*----------------------
 | s_flushed
 | Description: The shadow as of the last spu_shadow_take_dirty.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned short s_flushed[SPU_SHADOW_HALFWORDS];

/*----------------------
 | spu_shadow_reset
 | Description: Zeroes the shadow, its last-flushed copy and the latched key events.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_spu_shadow, spu_shadow_key_events
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void spu_shadow_reset(void)
{
    int i;

    for (i = 0; i < SPU_SHADOW_HALFWORDS; i++)
    {
        lom_spu_shadow[i] = 0;
        s_flushed[i] = 0;
    }
    for (i = 0; i < SPU_SHADOW_KEY_COUNT; i++)
        spu_shadow_key_events[i] = 0;
}

/*----------------------
 | spu_shadow_take_dirty
 | Description: Marks every halfword changed since the last call, plus any non-zero key register, then latches and clears the key registers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_spu_shadow, spu_shadow_key_events
 | Params: out_bits -- receives one bit per halfword (bit i % 32 of word i / 32)
 | Returns: non-zero if any bit is set
 ----------------------*/
int spu_shadow_take_dirty(unsigned out_bits[8])
{
    unsigned any = 0;
    int i;

    for (i = 0; i < 8; i++)
        out_bits[i] = 0;
    for (i = 0; i < SPU_SHADOW_HALFWORDS; i++)
    {
        unsigned short value = lom_spu_shadow[i];
        int key = i >= SPU_SHADOW_KEY_FIRST && i < SPU_SHADOW_KEY_FIRST + SPU_SHADOW_KEY_COUNT;

        if (key)
        {
            spu_shadow_key_events[i - SPU_SHADOW_KEY_FIRST] = value;
            lom_spu_shadow[i] = 0;
            if (value == 0)
                continue;
        }
        else if (value == s_flushed[i])
        {
            continue;
        }
        s_flushed[i] = key ? 0 : value;
        out_bits[i / 32] |= 1u << (i % 32);
        any = 1;
    }
    return (int)any;
}
