/*----------------------
 | aica_regs.h
 | Description: AICA register addresses and the pure packing of a voice setup into channel register words, plus the EG readback to PS1 envelope level mapping.
 | Author: suinevere
 | Dependencies: spu_backend.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_AICA_REGS_H
#define LOM_AICA_REGS_H

#include "spu_backend.h"

/*----------------------
 | AICA_REG_BASE / AICA_RAM_BASE / AICA_CHANNEL_STRIDE / AICA_COMMON
 | Description: SH-4 addresses of the AICA registers and sound RAM, the bytes between channels' registers, and the common registers' offset.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_REG_BASE 0xA0700000u
#define AICA_RAM_BASE 0xA0800000u
#define AICA_CHANNEL_STRIDE 0x80u
#define AICA_COMMON 0x2800u

/*----------------------
 | AICA_CHANNEL_WORDS
 | Description: Channel registers a key-on writes, +0x00 through +0x28 in 4-byte steps.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_CHANNEL_WORDS 11

/*----------------------
 | AICA_KYONB / AICA_KYONEX / AICA_LPCTL
 | Description: Channel word 0 bits: key on, execute key changes on every channel, and loop.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AICA_KYONB 0x4000u
#define AICA_KYONEX 0x8000u
#define AICA_LPCTL 0x0200u

/*----------------------
 | aica_channel_words
 | Description: Packs a voice setup into channel words +0x00..+0x28: 16- or 8-bit PCM from the span, loop, envelope, pitch, full direct send with pan, TL, the filter off and the DSP send; no key bits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: setup -- voice setup, words -- out AICA_CHANNEL_WORDS words
 | Returns: N/A
 ----------------------*/
void aica_channel_words(const SpuVoiceSetup *setup, unsigned short words[AICA_CHANNEL_WORDS]);

/*----------------------
 | aica_envelope_level
 | Description: PS1 envelope level (0..0x7FFF) for an AICA EG readback: 0 once the envelope passes 0x3BF or the channel is not keyed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: reg_2810 -- common register 0x2810 for the channel, keyed -- whether the voice was keyed on
 | Returns: level
 ----------------------*/
int aica_envelope_level(unsigned short reg_2810, int keyed);

#endif
