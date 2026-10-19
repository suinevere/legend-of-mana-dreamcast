/*----------------------
 | spu_shim.h
 | Description: Entry points of the shared SPU shim that the platform calls: choose the sound chip backend, flush SPU register changes to it each tick, and log through it.
 | Author: suinevere
 | Dependencies: spu_backend.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_SPU_SHIM_H
#define LOM_SPU_SHIM_H

#include "spu_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | spu_shim_set_backend
 | Description: Sets the sound chip the shim drives; NULL makes every call silent.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: backend -- sound chip or NULL
 | Returns: N/A
 ----------------------*/
void spu_shim_set_backend(const SpuBackend *backend);

/*----------------------
 | spu_shim_backend
 | Description: The sound chip the shim drives, or NULL.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: backend or NULL
 ----------------------*/
const SpuBackend *spu_shim_backend(void);

/*----------------------
 | spu_shim_log
 | Description: Sends one line to the backend's log, if it has one.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: message -- text
 | Returns: N/A
 ----------------------*/
void spu_shim_log(const char *message);

/*----------------------
 | spu_voice_flush
 | Description: Applies the SPU register changes since the last call through the backend, then refills streamed voices; run after each AKAO tick.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void spu_voice_flush(void);

/*----------------------
 | spu_shim_key_stats_take
 | Description: Reads and resets the key-on count and the lowest total level since the last call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: loudest_tl -- receives the lowest AICA total level
 | Returns: the key-on count
 ----------------------*/
unsigned spu_shim_key_stats_take(unsigned *loudest_tl);

/*----------------------
 | spu_shim_set_noise_clock
 | Description: Stores the SPU noise clock SpuSetNoiseClock set and marks it changed for the next flush.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: clock -- SpuSetNoiseClock's low 6 bits
 | Returns: N/A
 ----------------------*/
void spu_shim_set_noise_clock(unsigned clock);

/*----------------------
 | spu_shim_noise_clock
 | Description: The SPU noise clock last set by SpuSetNoiseClock, or 0 before any call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: the clock
 ----------------------*/
unsigned spu_shim_noise_clock(void);

/*----------------------
 | spu_shim_noise_clock_take
 | Description: Reads and resets whether the noise clock changed since the last call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: clock -- receives the clock, only when returning 1
 | Returns: 1 once per change, else 0
 ----------------------*/
int spu_shim_noise_clock_take(unsigned *clock);

/*----------------------
 | spu_voice_flush_reset
 | Description: Forgets every voice's cached DSP send, the reverb and noise bitmap copies, the noise clock change flag, the pitch-modulation mask, and the held-voice mask; call from SpuInit alongside spu_shadow_reset and spu_sample_map_reset.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void spu_voice_flush_reset(void);

#ifdef __cplusplus
}
#endif

#endif
