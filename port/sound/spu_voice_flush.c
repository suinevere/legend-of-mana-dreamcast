/*----------------------
 | spu_voice_flush.c
 | Description: Turns the SPU register shadow's changes into backend calls: key off, live pitch, level and envelope changes, key on through the sample map or a streamed voice, and CD volume.
 | Author: suinevere
 | Dependencies: spu_shim.h, spu_shadow.h, spu_sample_map.h, spu_long_voice.h, aica_convert.h, stdio.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

#include "aica_convert.h"
#include "spu_long_voice.h"
#include "spu_sample_map.h"
#include "spu_shadow.h"
#include "spu_shim.h"

/*----------------------
 | VOICE_HALFWORDS / VOICE_VOLUME_LEFT .. VOICE_REPEAT
 | Description: Shadow halfwords per voice, and each register's index within a voice's block.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define VOICE_HALFWORDS 8
#define VOICE_VOLUME_LEFT 0
#define VOICE_VOLUME_RIGHT 1
#define VOICE_PITCH 2
#define VOICE_START 3
#define VOICE_ADSR_LOW 4
#define VOICE_ADSR_HIGH 5
#define VOICE_REPEAT 7

/*----------------------
 | SHADOW_PITCH_MOD / SHADOW_NOISE / SHADOW_REVERB / SHADOW_CD_VOLUME
 | Description: Shadow indexes of the low halfword of the pitch-modulation, noise and reverb bitmaps, and of the left CD volume.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SHADOW_PITCH_MOD (0x190 / 2)
#define SHADOW_NOISE (0x194 / 2)
#define SHADOW_REVERB (0x198 / 2)
#define SHADOW_CD_VOLUME (0x1B0 / 2)

/*----------------------
 | UNWRITTEN_MEMORY
 | Description: Unwritten key-on addresses remembered so each is logged once.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define UNWRITTEN_MEMORY 16

/*----------------------
 | s_backend
 | Description: The sound chip the shim drives, or NULL.
 | Author: suinevere
 | Dependencies: SpuBackend
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const SpuBackend *s_backend;

/*----------------------
 | s_unwritten / s_unwritten_next / s_logged_pitch_mod / s_logged_noise / s_logged_reverb / s_logged_sweep
 | Description: Log-once memory: recent unwritten key-on addresses and the unsupported features already reported.
 | Author: suinevere
 | Dependencies: UNWRITTEN_MEMORY
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_unwritten[UNWRITTEN_MEMORY];
static unsigned s_unwritten_next;
static int s_logged_pitch_mod;
static int s_logged_noise;
static int s_logged_reverb;
static int s_logged_sweep;

/*----------------------
 | spu_shim_set_backend
 | Description: Sets the sound chip the shim drives; NULL makes every call silent.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_backend
 | Params: backend -- sound chip or NULL
 | Returns: N/A
 ----------------------*/
void spu_shim_set_backend(const SpuBackend *backend)
{
    s_backend = backend;
}

/*----------------------
 | spu_shim_backend
 | Description: The sound chip the shim drives, or NULL.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_backend
 | Params: N/A
 | Returns: backend or NULL
 ----------------------*/
const SpuBackend *spu_shim_backend(void)
{
    return s_backend;
}

/*----------------------
 | spu_shim_log
 | Description: Sends one line to the backend's log, if it has one.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_backend
 | Params: message -- text
 | Returns: N/A
 ----------------------*/
void spu_shim_log(const char *message)
{
    if (s_backend != NULL && s_backend->log != NULL)
        s_backend->log(message);
}

/*----------------------
 | log_once
 | Description: Logs a message the first time its flag is clear.
 | Author: suinevere
 | Dependencies: spu_shim_log
 | Globals: N/A
 | Params: flag -- log-once flag, message -- text
 | Returns: N/A
 ----------------------*/
static void log_once(int *flag, const char *message)
{
    if (*flag)
        return;
    *flag = 1;
    spu_shim_log(message);
}

/*----------------------
 | log_unwritten
 | Description: Logs a key-on at an address no upload filled, once per address among the last UNWRITTEN_MEMORY.
 | Author: suinevere
 | Dependencies: spu_shim_log
 | Globals: s_unwritten, s_unwritten_next
 | Params: spu_addr -- SPU byte address
 | Returns: N/A
 ----------------------*/
static void log_unwritten(unsigned spu_addr)
{
    char line[48];
    unsigned i;

    for (i = 0; i < UNWRITTEN_MEMORY; i++)
        if (s_unwritten[i] == spu_addr + 1u)
            return;
    s_unwritten[s_unwritten_next] = spu_addr + 1u;
    s_unwritten_next = (s_unwritten_next + 1u) % UNWRITTEN_MEMORY;
    snprintf(line, sizeof(line), "spu: key-on at unwritten 0x%05X", spu_addr);
    spu_shim_log(line);
}

/*----------------------
 | dirty
 | Description: Whether a shadow halfword changed in this flush.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: bits -- spu_shadow_take_dirty bits, index -- shadow index
 | Returns: 1 or 0
 ----------------------*/
static int dirty(const unsigned bits[8], unsigned index)
{
    return (bits[index / 32u] >> (index % 32u)) & 1u;
}

/*----------------------
 | reg
 | Description: A voice register's current shadow value.
 | Author: suinevere
 | Dependencies: lom_spu_shadow
 | Globals: lom_spu_shadow
 | Params: voice -- 0..23, field -- VOICE_* index
 | Returns: register value
 ----------------------*/
static unsigned short reg(int voice, int field)
{
    return lom_spu_shadow[voice * VOICE_HALFWORDS + field];
}

/*----------------------
 | level_pan
 | Description: TL and DIPAN for a voice's volume registers, logging a sweep volume once.
 | Author: suinevere
 | Dependencies: aica_level_pan, log_once
 | Globals: s_logged_sweep
 | Params: voice -- 0..23, tl / dipan -- out
 | Returns: N/A
 ----------------------*/
static void level_pan(int voice, unsigned char *tl, unsigned char *dipan)
{
    int swept;

    aica_level_pan((short)reg(voice, VOICE_VOLUME_LEFT), (short)reg(voice, VOICE_VOLUME_RIGHT), tl, dipan, &swept);
    if (swept)
        log_once(&s_logged_sweep, "spu: volume sweep");
}

/*----------------------
 | s_key_ons / s_loudest_tl
 | Description: Key-ons sent to the backend and the lowest total level among them since spu_shim_key_stats_take last read them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_key_ons;
static unsigned s_loudest_tl = 0xFFu;

/*----------------------
 | key_on
 | Description: Keys a voice on at its start register: the sample map gives the span, a too-long sample streams, an unwritten address logs and stays silent.
 | Author: suinevere
 | Dependencies: spu_sample_map_locate, aica_pitch_word, aica_envelope, level_pan, spu_long_voice_stop, spu_long_voice_start, log_unwritten
 | Globals: s_backend, s_key_ons, s_loudest_tl
 | Params: voice -- 0..23
 | Returns: N/A
 ----------------------*/
static void key_on(int voice)
{
    SpuVoiceSetup setup;

    if (spu_sample_map_locate(reg(voice, VOICE_START), reg(voice, VOICE_REPEAT), &setup.span) != 0)
    {
        log_unwritten((unsigned)reg(voice, VOICE_START) * 8u);
        return;
    }
    setup.pitch = aica_pitch_word(reg(voice, VOICE_PITCH), 44100);
    level_pan(voice, &setup.tl, &setup.dipan);
    aica_envelope(reg(voice, VOICE_ADSR_LOW), reg(voice, VOICE_ADSR_HIGH), &setup.eg10, &setup.eg14);
    spu_long_voice_stop(voice);
    s_key_ons++;
    if (setup.tl < s_loudest_tl)
        s_loudest_tl = setup.tl;
    if (setup.span.streamed)
        spu_long_voice_start(s_backend, voice, &setup);
    else
        s_backend->key_on(voice, &setup);
}

/*----------------------
 | update_voice
 | Description: Applies a voice's changed pitch, volume and envelope registers while it plays.
 | Author: suinevere
 | Dependencies: dirty, aica_pitch_word, level_pan, aica_envelope
 | Globals: s_backend
 | Params: bits -- dirty bits, voice -- 0..23
 | Returns: N/A
 ----------------------*/
static void update_voice(const unsigned bits[8], int voice)
{
    unsigned base = (unsigned)voice * VOICE_HALFWORDS;

    if (dirty(bits, base + VOICE_PITCH))
        s_backend->set_pitch(voice, aica_pitch_word(reg(voice, VOICE_PITCH), 44100));
    if (dirty(bits, base + VOICE_VOLUME_LEFT) || dirty(bits, base + VOICE_VOLUME_RIGHT))
    {
        unsigned char tl;
        unsigned char dipan;

        level_pan(voice, &tl, &dipan);
        s_backend->set_level(voice, tl, dipan);
    }
    if (dirty(bits, base + VOICE_ADSR_LOW) || dirty(bits, base + VOICE_ADSR_HIGH))
    {
        unsigned short eg10;
        unsigned short eg14;

        aica_envelope(reg(voice, VOICE_ADSR_LOW), reg(voice, VOICE_ADSR_HIGH), &eg10, &eg14);
        s_backend->set_envelope(voice, eg10, eg14);
    }
}

/*----------------------
 | flush_globals
 | Description: Logs unsupported pitch modulation, noise and reverb once each, and passes CD volume changes to the backend.
 | Author: suinevere
 | Dependencies: dirty, log_once
 | Globals: lom_spu_shadow, s_backend, s_logged_pitch_mod, s_logged_noise, s_logged_reverb
 | Params: bits -- dirty bits
 | Returns: N/A
 ----------------------*/
static void flush_globals(const unsigned bits[8])
{
    if ((dirty(bits, SHADOW_PITCH_MOD) || dirty(bits, SHADOW_PITCH_MOD + 1)) && (lom_spu_shadow[SHADOW_PITCH_MOD] | lom_spu_shadow[SHADOW_PITCH_MOD + 1]))
        log_once(&s_logged_pitch_mod, "spu: pitch modulation");
    if ((dirty(bits, SHADOW_NOISE) || dirty(bits, SHADOW_NOISE + 1)) && (lom_spu_shadow[SHADOW_NOISE] | lom_spu_shadow[SHADOW_NOISE + 1]))
        log_once(&s_logged_noise, "spu: noise");
    if ((dirty(bits, SHADOW_REVERB) || dirty(bits, SHADOW_REVERB + 1)) && (lom_spu_shadow[SHADOW_REVERB] | lom_spu_shadow[SHADOW_REVERB + 1]))
        log_once(&s_logged_reverb, "spu: reverb");
    if ((dirty(bits, SHADOW_CD_VOLUME) || dirty(bits, SHADOW_CD_VOLUME + 1)) && s_backend->cd_volume != NULL)
        s_backend->cd_volume((short)lom_spu_shadow[SHADOW_CD_VOLUME], (short)lom_spu_shadow[SHADOW_CD_VOLUME + 1]);
}

/*----------------------
 | spu_voice_flush
 | Description: Applies the SPU register changes since the last call through the backend (key off first, then live changes, then key on), then refills streamed voices; run after each AKAO tick.
 | Author: suinevere
 | Dependencies: spu_shadow_take_dirty, update_voice, key_on, flush_globals, spu_long_voice_service
 | Globals: spu_shadow_key_events, s_backend
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void spu_voice_flush(void)
{
    unsigned bits[8];
    unsigned on;
    unsigned off;
    int voice;

    if (spu_shadow_take_dirty(bits) && s_backend != NULL)
    {
        on = spu_shadow_key_events[0] | ((unsigned)spu_shadow_key_events[1] << 16);
        off = spu_shadow_key_events[2] | ((unsigned)spu_shadow_key_events[3] << 16);
        for (voice = 0; voice < SPU_VOICE_COUNT; voice++)
            if (off & (1u << voice))
                s_backend->key_off(voice);
        for (voice = 0; voice < SPU_VOICE_COUNT; voice++)
            if (!(on & (1u << voice)))
                update_voice(bits, voice);
        for (voice = 0; voice < SPU_VOICE_COUNT; voice++)
            if (on & (1u << voice))
                key_on(voice);
        flush_globals(bits);
    }
    if (s_backend != NULL)
        spu_long_voice_service(s_backend);
}

/*----------------------
 | spu_shim_key_stats_take
 | Description: Reads and resets the key-on count and the lowest total level since the last call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_key_ons, s_loudest_tl
 | Params: loudest_tl -- receives the lowest AICA total level (0 loudest, 0xFF when nothing keyed)
 | Returns: the key-on count
 ----------------------*/
unsigned spu_shim_key_stats_take(unsigned *loudest_tl)
{
    unsigned count = s_key_ons;

    *loudest_tl = s_loudest_tl;
    s_key_ons = 0;
    s_loudest_tl = 0xFFu;
    return count;
}
