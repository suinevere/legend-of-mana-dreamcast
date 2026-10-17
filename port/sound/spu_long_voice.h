/*----------------------
 | spu_long_voice.h
 | Description: Voices whose sample is longer than one AICA channel reaches play from a looping ring that each tick refills by decoding ADPCM block by block, the way the PS1 SPU walks a sample.
 | Author: suinevere
 | Dependencies: spu_backend.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_SPU_LONG_VOICE_H
#define LOM_SPU_LONG_VOICE_H

#include "spu_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | SPU_LONG_RING_RAM / SPU_LONG_RING_FRAMES / SPU_LONG_RING_STRIDE
 | Description: AICA offset of voice 0's ring, frames per ring, and bytes between voices' rings.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_LONG_RING_RAM 0x1D0000u
#define SPU_LONG_RING_FRAMES 4096u
#define SPU_LONG_RING_STRIDE 0x2000u

/*----------------------
 | SPU_LONG_GUARD_FRAMES
 | Description: Frames the refill stays behind the play position.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_LONG_GUARD_FRAMES 64u

/*----------------------
 | spu_long_voice_start
 | Description: Starts streaming a voice: fills its ring from the span's start block and keys the channel on looping over the ring.
 | Author: suinevere
 | Dependencies: spu_sample_map_ram, spu_adpcm_decode_block
 | Globals: N/A
 | Params: backend -- sound chip, voice -- 0..23, setup -- the key-on setup with the streamed span
 | Returns: N/A
 ----------------------*/
void spu_long_voice_start(const SpuBackend *backend, int voice, const SpuVoiceSetup *setup);

/*----------------------
 | spu_long_voice_stop
 | Description: Drops a voice's stream state, before a new key-on of that voice.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: voice -- 0..23
 | Returns: N/A
 ----------------------*/
void spu_long_voice_stop(int voice);

/*----------------------
 | spu_long_voice_service
 | Description: Once per tick: refills every streaming voice's ring up to just behind its play position and keys off voices whose sample has ended.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: backend -- sound chip
 | Returns: N/A
 ----------------------*/
void spu_long_voice_service(const SpuBackend *backend);

/*----------------------
 | spu_long_voice_active / spu_long_voice_finished
 | Description: Whether a voice is streaming, and whether its stream played to an unrepeated end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: voice -- 0..23
 | Returns: 1 or 0
 ----------------------*/
int spu_long_voice_active(int voice);
int spu_long_voice_finished(int voice);

#ifdef __cplusplus
}
#endif

#endif
