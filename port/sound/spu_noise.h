/*----------------------
 | spu_noise.h
 | Description: The PS1 SPU noise generator's sequence, rendered once into a looping 8-bit table, plus the noise pitch and voice span the AICA plays it with.
 | Author: suinevere
 | Dependencies: spu_sample_map.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_SPU_NOISE_H
#define LOM_SPU_NOISE_H

#include "spu_sample_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | SPU_NOISE_RAM
 | Description: AICA byte offset the noise table is uploaded to, past the reverb ring.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_NOISE_RAM 0x1B8000u

/*----------------------
 | SPU_NOISE_FRAMES
 | Description: Frames in the rendered noise table; the LFSR's period (65,535) keeps them all distinct.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_NOISE_FRAMES 32768u

/*----------------------
 | spu_noise_next
 | Description: One step of the PS1 SPU noise LFSR: parity from bits 15, 12, 11 and 10 inverted, then level = level*2 + parity.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: level -- current 16-bit level
 | Returns: the next 16-bit level
 ----------------------*/
unsigned spu_noise_next(unsigned level);

/*----------------------
 | spu_noise_render
 | Description: Fills a buffer with the noise sequence from level 0, storing the high byte of each stepped level.
 | Author: suinevere
 | Dependencies: spu_noise_next
 | Globals: N/A
 | Params: out -- frames bytes, frames -- count
 | Returns: N/A
 ----------------------*/
void spu_noise_render(signed char *out, unsigned frames);

/*----------------------
 | spu_noise_pitch
 | Description: The AICA pitch to play the noise table at for an SPU noise clock, so one table entry advances per 44.1 kHz noise step.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: clock -- SpuSetNoiseClock's low 6 bits
 | Returns: PS1 pitch units, 1..0x1000
 ----------------------*/
unsigned short spu_noise_pitch(unsigned clock);

/*----------------------
 | spu_noise_span
 | Description: The sample span a noise voice plays: the uploaded table, looping from its start, never streamed.
 | Author: suinevere
 | Dependencies: SPU_NOISE_RAM, SPU_NOISE_FRAMES
 | Globals: N/A
 | Params: span -- out
 | Returns: N/A
 ----------------------*/
void spu_noise_span(SpuSampleSpan *span);

#ifdef __cplusplus
}
#endif

#endif
