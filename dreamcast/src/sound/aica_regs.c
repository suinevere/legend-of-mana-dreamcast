/*----------------------
 | aica_regs.c
 | Description: Pure AICA register packing for voice key-ons and the EG readback mapping; no hardware access.
 | Author: suinevere
 | Dependencies: aica_regs.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "aica_regs.h"

/*----------------------
 | AICA_EG_SILENT
 | Description: First EG readback value that counts as silence; Flycast reports 0x1FFF beyond 0x3BF.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_EG_SILENT 0x3C0u

/*----------------------
 | AICA_DISDL_FULL / AICA_LPOFF
 | Description: Full direct send level (0 dB) in word 9, and the filter-off bit in word 10.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_DISDL_FULL 0x0F00u
#define AICA_LPOFF 0x0020u

/*----------------------
 | aica_channel_words
 | Description: Packs a voice setup into channel words +0x00..+0x28: 16-bit PCM from the span, loop, envelope, pitch, full direct send with pan, TL and the filter off; no key bits.
 | Author: suinevere
 | Dependencies: AICA_LPCTL, AICA_DISDL_FULL, AICA_LPOFF
 | Globals: N/A
 | Params: setup -- voice setup, words -- out AICA_CHANNEL_WORDS words
 | Returns: N/A
 ----------------------*/
void aica_channel_words(const SpuVoiceSetup *setup, unsigned short words[AICA_CHANNEL_WORDS])
{
    words[0] = (unsigned short)((setup->span.loops ? AICA_LPCTL : 0u) | ((setup->span.pcm_offset >> 16) & 0x7Fu));
    words[1] = (unsigned short)(setup->span.pcm_offset & 0xFFFFu);
    words[2] = (unsigned short)setup->span.loop_start;
    words[3] = (unsigned short)setup->span.frames;
    words[4] = setup->eg10;
    words[5] = setup->eg14;
    words[6] = setup->pitch;
    words[7] = 0;
    words[8] = 0;
    words[9] = (unsigned short)(AICA_DISDL_FULL | setup->dipan);
    words[10] = (unsigned short)(((unsigned)setup->tl << 8) | AICA_LPOFF);
}

/*----------------------
 | aica_envelope_level
 | Description: PS1 envelope level (0..0x7FFF) for an AICA EG readback: 0 once the envelope passes 0x3BF or the channel is not keyed, else linear from 0x7FFF at full down to 1.
 | Author: suinevere
 | Dependencies: AICA_EG_SILENT
 | Globals: N/A
 | Params: reg_2810 -- common register 0x2810 for the channel, keyed -- whether the voice was keyed on
 | Returns: level
 ----------------------*/
int aica_envelope_level(unsigned short reg_2810, int keyed)
{
    unsigned eg = reg_2810 & 0x1FFFu;
    unsigned level;

    if (!keyed || eg >= AICA_EG_SILENT)
        return 0;
    level = (AICA_EG_SILENT - 1u - eg) * 0x7FFFu / (AICA_EG_SILENT - 1u);
    return level == 0 ? 1 : (int)level;
}
