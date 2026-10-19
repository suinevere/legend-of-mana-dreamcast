/*----------------------
 | spu_noise.c
 | Description: Steps and renders the PS1 SPU noise generator's LFSR sequence, and gives the AICA pitch and sample span that play the rendered table as PS1 noise voices do.
 | Author: suinevere
 | Dependencies: spu_noise.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "spu_noise.h"

/*----------------------
 | spu_noise_next
 | Description: One step of the PS1 SPU noise LFSR: parity from bits 15, 12, 11 and 10 inverted, then level = level*2 + parity, kept to 16 bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: level -- current 16-bit level
 | Returns: the next 16-bit level
 ----------------------*/
unsigned spu_noise_next(unsigned level)
{
    unsigned parity = ((level >> 15) ^ (level >> 12) ^ (level >> 11) ^ (level >> 10) ^ 1u) & 1u;

    return (level * 2u + parity) & 0xFFFFu;
}

/*----------------------
 | spu_noise_render
 | Description: Fills a buffer with the noise sequence from level 0, storing the high byte of each stepped level.
 | Author: suinevere
 | Dependencies: spu_noise_next
 | Globals: N/A
 | Params: out -- frames bytes, frames -- count
 | Returns: N/A
 ----------------------*/
void spu_noise_render(signed char *out, unsigned frames)
{
    unsigned level = 0u;
    unsigned i;

    for (i = 0; i < frames; i++)
    {
        level = spu_noise_next(level);
        out[i] = (signed char)(level >> 8);
    }
}

/*----------------------
 | spu_noise_pitch
 | Description: The AICA pitch to play the noise table at for an SPU noise clock: min(((n&3)+4) << (n>>2) >> 5, 0x1000), at least 1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: clock -- SpuSetNoiseClock's low 6 bits
 | Returns: PS1 pitch units, 1..0x1000
 ----------------------*/
unsigned short spu_noise_pitch(unsigned clock)
{
    unsigned step = (clock & 3u) + 4u;
    unsigned shift = clock >> 2;
    unsigned pitch = (step << shift) >> 5;

    if (pitch > 0x1000u)
        pitch = 0x1000u;
    if (pitch == 0u)
        pitch = 1u;
    return (unsigned short)pitch;
}

/*----------------------
 | spu_noise_span
 | Description: The sample span a noise voice plays: the uploaded table, looping from its start, never streamed.
 | Author: suinevere
 | Dependencies: SPU_NOISE_RAM, SPU_NOISE_FRAMES
 | Globals: N/A
 | Params: span -- out
 | Returns: N/A
 ----------------------*/
void spu_noise_span(SpuSampleSpan *span)
{
    span->pcm_offset = SPU_NOISE_RAM;
    span->frames = SPU_NOISE_FRAMES;
    span->loop_start = 0u;
    span->loops = 1;
    span->streamed = 0;
    span->start_block = 0u;
    span->loop_block = 0u;
}
