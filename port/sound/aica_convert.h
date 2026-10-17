/*----------------------
 | aica_convert.h
 | Description: Pure conversions from PS1 SPU voice settings (pitch, volume, ADSR) to AICA channel register words; no hardware access.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_AICA_CONVERT_H
#define LOM_AICA_CONVERT_H

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | aica_pitch_word
 | Description: AICA OCT (bits 14-11) and FNS (bits 9-0) for a PS1 4.12 pitch playing a sample stored at sample_rate_hz.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ps1_pitch -- PS1 voice pitch, 0x1000 = 44100 Hz; sample_rate_hz -- the stored sample's rate
 | Returns: the channel +0x18 pitch word (OCT, FNS), clamped to OCT -8..7
 ----------------------*/
unsigned short aica_pitch_word(unsigned short ps1_pitch, unsigned sample_rate_hz);

/*----------------------
 | aica_level_pan
 | Description: AICA TL and DIPAN for a PS1 left/right volume pair: TL from the louder side, DIPAN attenuating the quieter side in 3 dB steps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: ps1_left, ps1_right -- PS1 volume registers; tl -- out 0 loudest..255 silent; dipan -- out 5-bit pan; swept -- out 1 when either side is in sweep mode
 | Returns: N/A
 ----------------------*/
void aica_level_pan(short ps1_left, short ps1_right, unsigned char *tl, unsigned char *dipan, int *swept);

/*----------------------
 | aica_envelope
 | Description: AICA envelope words for a PS1 ADSR pair, through the generated nearest-time rate tables.
 | Author: suinevere
 | Dependencies: aica_envelope_table.h
 | Globals: N/A
 | Params: adsr_low, adsr_high -- PS1 ADSR1/ADSR2; eg10 -- out channel +0x10 word (D2R, D1R, AR); eg14 -- out channel +0x14 word (LPSLNK, KRS, DL, RR)
 | Returns: N/A
 ----------------------*/
void aica_envelope(unsigned short adsr_low, unsigned short adsr_high, unsigned short *eg10, unsigned short *eg14);

#ifdef __cplusplus
}
#endif

#endif
