/*----------------------
 | aica_convert.c
 | Description: Pure integer conversions from PS1 SPU voice settings (pitch, volume, ADSR) to AICA channel register words.
 | Author: suinevere
 | Dependencies: aica_convert.h, aica_envelope_table.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "aica_convert.h"
#include "aica_envelope_table.h"

/*----------------------
 | PS1_PITCH_DENOMINATOR
 | Description: PS1 pitch 0x1000 times the SPU's 44100 Hz output rate: the playback ratio is pitch * rate / this.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PS1_PITCH_DENOMINATOR (4096ULL * 44100ULL)

/*----------------------
 | PS1_FULL_VOLUME
 | Description: The PS1 fixed volume that plays at 0 dB.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PS1_FULL_VOLUME 0x3FFFu

/*----------------------
 | TL_STEPS_PER_OCTAVE_Q16
 | Description: TL steps per amplitude octave in Q16: the AICA attenuates 2^(-TL/16) (Flycast sgc_if.cpp tl_lut).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TL_STEPS_PER_OCTAVE_Q16 (16ULL << 16)

/*----------------------
 | PAN_STEPS_PER_OCTAVE_Q16
 | Description: DIPAN steps per amplitude octave in Q16: each step is 8 TL units, half an octave (Flycast sgc_if.cpp SendLevel).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PAN_STEPS_PER_OCTAVE_Q16 (2ULL << 16)

/*----------------------
 | AICA_KRS_OFF
 | Description: KRS value that disables key rate scaling, so envelope times match the tables.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_KRS_OFF 0xFu

/*----------------------
 | log2_ratio_q16
 | Description: log2(num / den) in Q16 by repeated squaring, for num >= den > 0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: num, den -- the ratio's terms
 | Returns: Q16 base-2 logarithm
 ----------------------*/
static unsigned log2_ratio_q16(unsigned num, unsigned den)
{
    unsigned result = 0;
    unsigned bit;
    unsigned long long x;

    while (num >= (den << 1))
    {
        den <<= 1;
        result += 1u << 16;
    }
    x = ((unsigned long long)num << 30) / den;
    for (bit = 1u << 15; bit != 0; bit >>= 1)
    {
        x = (x * x) >> 30;
        if (x >= (2ULL << 30))
        {
            x >>= 1;
            result |= bit;
        }
    }
    return result;
}

/*----------------------
 | steps_for
 | Description: Rounded count of attenuation steps between two amplitudes.
 | Author: suinevere
 | Dependencies: log2_ratio_q16
 | Globals: N/A
 | Params: loud, quiet -- amplitudes, loud >= quiet > 0; steps_per_octave_q16 -- steps per halving in Q16
 | Returns: step count
 ----------------------*/
static unsigned steps_for(unsigned loud, unsigned quiet, unsigned long long steps_per_octave_q16)
{
    return (unsigned)(((unsigned long long)log2_ratio_q16(loud, quiet) * steps_per_octave_q16 + (1ULL << 31)) >> 32);
}

/*----------------------
 | aica_pitch_word
 | Description: AICA OCT (bits 14-11) and FNS (bits 9-0) for a PS1 4.12 pitch playing a sample stored at sample_rate_hz.
 | Author: suinevere
 | Dependencies: PS1_PITCH_DENOMINATOR
 | Globals: N/A
 | Params: ps1_pitch -- PS1 voice pitch, 0x1000 = 44100 Hz; sample_rate_hz -- the stored sample's rate
 | Returns: the channel +0x18 pitch word (OCT, FNS), clamped to OCT -8..7
 ----------------------*/
unsigned short aica_pitch_word(unsigned short ps1_pitch, unsigned sample_rate_hz)
{
    unsigned long long num = (unsigned long long)ps1_pitch * sample_rate_hz;
    unsigned long long den = PS1_PITCH_DENOMINATOR;
    unsigned long long scaled;
    int oct = 0;
    unsigned fns;

    if (num == 0)
        return (unsigned short)(8u << 11);
    if (num >= den)
    {
        while (oct < 8 && num >= (den << (oct + 1)))
            oct++;
    }
    else
    {
        while (oct > -9 && (num << -oct) < den)
            oct--;
    }
    if (oct >= 0)
        scaled = ((num << 10) + ((den << oct) >> 1)) / (den << oct);
    else
        scaled = ((num << (10 - oct)) + (den >> 1)) / den;
    fns = (unsigned)scaled - 1024u;
    if (fns >= 1024u)
    {
        oct++;
        fns = 0;
    }
    if (oct < -8)
        return (unsigned short)(8u << 11);
    if (oct > 7)
        return (unsigned short)((7u << 11) | 0x3FFu);
    return (unsigned short)((((unsigned)oct & 0xFu) << 11) | fns);
}

/*----------------------
 | ps1_volume_magnitude
 | Description: Magnitude of a PS1 voice volume register: a sweep value (bit 15) counts as full, a fixed value is 15-bit signed.
 | Author: suinevere
 | Dependencies: PS1_FULL_VOLUME
 | Globals: N/A
 | Params: raw -- register value; swept -- set to 1 for a sweep value
 | Returns: 0..0x4000
 ----------------------*/
static unsigned ps1_volume_magnitude(short raw, int *swept)
{
    unsigned value = (unsigned short)raw;

    if (value & 0x8000u)
    {
        *swept = 1;
        return PS1_FULL_VOLUME;
    }
    if (value & 0x4000u)
        return 0x8000u - value;
    return value;
}

/*----------------------
 | aica_level_pan
 | Description: AICA TL and DIPAN for a PS1 left/right volume pair: TL from the louder side, DIPAN attenuating the quieter side in 3 dB steps.
 | Author: suinevere
 | Dependencies: ps1_volume_magnitude, steps_for, PS1_FULL_VOLUME
 | Globals: N/A
 | Params: ps1_left, ps1_right -- PS1 volume registers; tl -- out 0 loudest..255 silent; dipan -- out 5-bit pan; swept -- out 1 when either side is in sweep mode
 | Returns: N/A
 ----------------------*/
void aica_level_pan(short ps1_left, short ps1_right, unsigned char *tl, unsigned char *dipan, int *swept)
{
    unsigned left;
    unsigned right;
    unsigned loud;
    unsigned quiet;
    unsigned steps;

    *swept = 0;
    left = ps1_volume_magnitude(ps1_left, swept);
    right = ps1_volume_magnitude(ps1_right, swept);
    loud = left > right ? left : right;
    quiet = left > right ? right : left;
    if (loud == 0)
    {
        *tl = 255;
        *dipan = 0;
        return;
    }
    steps = loud >= PS1_FULL_VOLUME ? 0 : steps_for(PS1_FULL_VOLUME, loud, TL_STEPS_PER_OCTAVE_Q16);
    *tl = (unsigned char)(steps > 255 ? 255 : steps);
    steps = quiet == 0 ? 15 : steps_for(loud, quiet, PAN_STEPS_PER_OCTAVE_Q16);
    if (steps > 15)
        steps = 15;
    *dipan = (unsigned char)((right < left ? 0x10u : 0u) | (steps == 0 ? 0u : steps));
}

/*----------------------
 | aica_envelope
 | Description: AICA envelope words for a PS1 ADSR pair, through the generated nearest-time rate tables.
 | Author: suinevere
 | Dependencies: aica_envelope_table.h, AICA_KRS_OFF
 | Globals: aica_attack_linear, aica_attack_exponential, aica_decrease_linear, aica_decrease_exponential, aica_decay_level
 | Params: adsr_low, adsr_high -- PS1 ADSR1/ADSR2; eg10 -- out channel +0x10 word (D2R, D1R, AR); eg14 -- out channel +0x14 word (LPSLNK, KRS, DL, RR)
 | Returns: N/A
 ----------------------*/
void aica_envelope(unsigned short adsr_low, unsigned short adsr_high, unsigned short *eg10, unsigned short *eg14)
{
    unsigned attack_rate = (adsr_low >> 8) & 0x7Fu;
    unsigned decay_rate = ((adsr_low >> 4) & 0xFu) << 2;
    unsigned sustain_level = adsr_low & 0xFu;
    unsigned sustain_rate = (adsr_high >> 6) & 0x7Fu;
    unsigned release_rate = (adsr_high & 0x1Fu) << 2;
    unsigned ar = (adsr_low & 0x8000u) ? aica_attack_exponential[attack_rate] : aica_attack_linear[attack_rate];
    unsigned d1r = aica_decrease_exponential[decay_rate];
    unsigned d2r = 0;
    unsigned rr = (adsr_high & 0x20u) ? aica_decrease_exponential[release_rate] : aica_decrease_linear[release_rate];

    if (adsr_high & 0x4000u)
        d2r = (adsr_high & 0x8000u) ? aica_decrease_exponential[sustain_rate] : aica_decrease_linear[sustain_rate];
    *eg10 = (unsigned short)((d2r << 11) | (d1r << 6) | ar);
    *eg14 = (unsigned short)((AICA_KRS_OFF << 10) | ((unsigned)aica_decay_level[sustain_level] << 5) | rr);
}

/*----------------------
 | aica_reverb_send
 | Description: AICA DSP send bus (ISEL) and level (IMXL) for a voice's reverb bit and pan: full send when enabled, bus picked by which side DIPAN favors.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: dipan -- voice's 5-bit DIPAN; enabled -- voice's reverb bit; isel -- out MIXS bus (0 left, 1 right, 2 centre); imxl -- out send level (15 full, 0 mute)
 | Returns: N/A
 ----------------------*/
void aica_reverb_send(unsigned char dipan, int enabled, unsigned char *isel, unsigned char *imxl)
{
    *imxl = enabled ? (unsigned char)15u : (unsigned char)0u;
    if ((dipan & 0xFu) < 2u)
        *isel = 2u;
    else if (dipan & 0x10u)
        *isel = 0u;
    else
        *isel = 1u;
}

/*----------------------
 | spu_pitch_mod
 | Description: PS1 pitch modulation: steps pitch by the previous voice's current sample scaled by its envelope, clamped to the SPU's maximum pitch.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: pitch -- this voice's PS1 pitch (4.12); sample -- previous voice's current signed PCM sample; envelope -- previous voice's envelope, 0..0x7FFF
 | Returns: modulated PS1 pitch, clamped to 0x3FFF
 ----------------------*/
unsigned short spu_pitch_mod(unsigned short pitch, int sample, int envelope)
{
    long long factor = (((long long)sample * envelope) >> 15) + 0x8000;
    long long scaled = ((long long)pitch * factor) >> 15;

    if (scaled > 0x3FFF)
        scaled = 0x3FFF;
    return (unsigned short)scaled;
}
