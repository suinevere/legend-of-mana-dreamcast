/*----------------------
 | spu_backend.h
 | Description: The hardware side of the SPU shim: a voice's AICA setup and the calls a sound chip backend answers.
 | Author: suinevere
 | Dependencies: spu_sample_map.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_SPU_BACKEND_H
#define LOM_SPU_BACKEND_H

#include "spu_sample_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | SPU_VOICE_COUNT
 | Description: PS1 SPU voices; voice v plays on AICA channel v.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_VOICE_COUNT 24

/*----------------------
 | SpuVoiceSetup
 | Description: Everything a key-on writes: the sample span, the AICA pitch word, TL and DIPAN, and the two envelope words.
 | Author: suinevere
 | Dependencies: SpuSampleSpan
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct SpuVoiceSetup
{
    SpuSampleSpan span;
    unsigned short pitch;
    unsigned char tl;
    unsigned char dipan;
    unsigned short eg10;
    unsigned short eg14;
} SpuVoiceSetup;

/*----------------------
 | SpuBackend
 | Description: Sound chip calls: PCM upload, key on and off, live pitch, level and envelope changes, envelope level and play position readback, CD/stream volume, and a log line; any but the first seven may be NULL.
 | Author: suinevere
 | Dependencies: SpuVoiceSetup
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct SpuBackend
{
    void (*upload)(unsigned aica_offset, const short *pcm, unsigned bytes);
    void (*key_on)(int voice, const SpuVoiceSetup *setup);
    void (*key_off)(int voice);
    void (*set_pitch)(int voice, unsigned short pitch);
    void (*set_level)(int voice, unsigned char tl, unsigned char dipan);
    void (*set_envelope)(int voice, unsigned short eg10, unsigned short eg14);
    int (*envelope)(int voice);
    unsigned (*position)(int voice);
    void (*cd_volume)(int left, int right);
    void (*log)(const char *message);
} SpuBackend;

#ifdef __cplusplus
}
#endif

#endif
