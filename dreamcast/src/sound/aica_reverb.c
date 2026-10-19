/*----------------------
 | aica_reverb.c
 | Description: Pure AICA DSP reverb COEF computation; no hardware access. Includes aica_reverb_program.h for its #defines only, not its generated tables.
 | Author: suinevere
 | Dependencies: aica_reverb.h, aica_reverb_program.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "aica_reverb.h"
#include "aica_reverb_program.h"

/*----------------------
 | clamp_coef
 | Description: Saturates a raw COEF value to signed 16 bits and returns it as an unsigned 16-bit word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: value -- integer
 | Returns: COEF word
 ----------------------*/
static unsigned short clamp_coef(int value)
{
    if (value > 32767)
        value = 32767;
    if (value < -32768)
        value = -32768;
    return (unsigned short)value;
}

/*----------------------
 | aica_reverb_coefs
 | Description: Input gain for steps 0, 1, 3, 4 and output depth and vAPF2*depth for steps 55, 56, 63, 64; all zero when off.
 | Author: suinevere
 | Dependencies: clamp_coef, AICA_REVERB_VAPF2, AICA_REVERB_INPUT_LEFT, AICA_REVERB_INPUT_RIGHT
 | Globals: N/A
 | Params: on -- reverb enabled, left -- left output depth (vLOUT), right -- right output depth (vROUT), out -- 8 COEF words
 | Returns: N/A
 ----------------------*/
void aica_reverb_coefs(int on, int left, int right, unsigned short out[8])
{
    short v = (short)AICA_REVERB_VAPF2;

    out[0] = (unsigned short)(on ? AICA_REVERB_INPUT_LEFT : 0u);
    out[1] = (unsigned short)(on ? AICA_REVERB_INPUT_LEFT : 0u);
    out[2] = (unsigned short)(on ? AICA_REVERB_INPUT_RIGHT : 0u);
    out[3] = (unsigned short)(on ? AICA_REVERB_INPUT_RIGHT : 0u);
    out[4] = on ? clamp_coef(left) : (unsigned short)0u;
    out[5] = on ? clamp_coef((v * (short)left) >> 15) : (unsigned short)0u;
    out[6] = on ? clamp_coef(right) : (unsigned short)0u;
    out[7] = on ? clamp_coef((v * (short)right) >> 15) : (unsigned short)0u;
}
