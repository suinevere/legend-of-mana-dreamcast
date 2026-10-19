/*----------------------
 | aica_reverb.h
 | Description: Pure computation of the AICA DSP reverb COEF words the driver rewrites live: the input gain and output depth steps, on or off.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_AICA_REVERB_H
#define LOM_AICA_REVERB_H

/*----------------------
 | aica_reverb_coefs
 | Description: The eight COEF words for reverb input steps 0, 1, 3, 4 and output depth steps 55, 56, 63, 64, in that order: input and depth gains when on, all zero when off.
 | Author: suinevere
 | Dependencies: AICA_REVERB_VAPF2, AICA_REVERB_INPUT_LEFT, AICA_REVERB_INPUT_RIGHT
 | Globals: N/A
 | Params: on -- reverb enabled, left -- left output depth (vLOUT), right -- right output depth (vROUT), out -- 8 COEF words
 | Returns: N/A
 ----------------------*/
void aica_reverb_coefs(int on, int left, int right, unsigned short out[8]);

#endif
