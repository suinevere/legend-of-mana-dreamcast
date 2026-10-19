/*----------------------
 | spu_voice_flush.c
 | Description: Turns the SPU register shadow's changes into backend calls: key off, live pitch, level and envelope changes, key on through the sample map, the noise table or a streamed voice, and CD volume.
 | Author: suinevere
 | Dependencies: spu_shim.h, spu_shadow.h, spu_sample_map.h, spu_noise.h, spu_long_voice.h, aica_convert.h, stdio.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

#include "aica_convert.h"
#include "spu_long_voice.h"
#include "spu_noise.h"
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
 | s_unwritten / s_unwritten_next / s_logged_sweep
 | Description: Log-once memory: recent unwritten key-on addresses and the unsupported features already reported.
 | Author: suinevere
 | Dependencies: UNWRITTEN_MEMORY
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_unwritten[UNWRITTEN_MEMORY];
static unsigned s_unwritten_next;
static int s_logged_sweep;

/*----------------------
 | s_modulated / s_mod_pitch
 | Description: Bitmask of voices spu_pitch_mod currently drives, and each driven voice's last AICA pitch word sent.
 | Author: suinevere
 | Dependencies: SPU_VOICE_COUNT
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_modulated;
static unsigned short s_mod_pitch[SPU_VOICE_COUNT];

/*----------------------
 | s_send / s_prev_reverb_low / s_prev_reverb_high / s_prev_noise_low / s_prev_noise_high
 | Description: Each voice's last DSP send sent to the backend (isel | imxl << 4, 0xFF unknown), and the reverb and noise bitmap halfwords as of the last flush, to find which voices' bits changed.
 | Author: suinevere
 | Dependencies: SPU_VOICE_COUNT
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_send[SPU_VOICE_COUNT];
static unsigned short s_prev_reverb_low;
static unsigned short s_prev_reverb_high;
static unsigned short s_prev_noise_low;
static unsigned short s_prev_noise_high;

/*----------------------
 | s_held
 | Description: Bitmask of voices currently held: set in key_on, cleared when a voice is keyed off, cleared entirely by spu_voice_flush_reset.
 | Author: suinevere
 | Dependencies: SPU_VOICE_COUNT
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_held;

/*----------------------
 | s_noise_clock / s_noise_clock_changed
 | Description: The SPU noise clock last set by SpuSetNoiseClock, and whether it changed since the last spu_shim_noise_clock_take.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_noise_clock;
static int s_noise_clock_changed;

/*----------------------
 | spu_shim_set_backend
 | Description: Sets the sound chip the shim drives and forgets every voice's last DSP send; NULL makes every call silent.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_backend, s_send
 | Params: backend -- sound chip or NULL
 | Returns: N/A
 ----------------------*/
void spu_shim_set_backend(const SpuBackend *backend)
{
    int voice;

    s_backend = backend;
    for (voice = 0; voice < SPU_VOICE_COUNT; voice++)
        s_send[voice] = 0xFFu;
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
 | reverb_enabled
 | Description: Whether a voice's bit is set in the reverb bitmap.
 | Author: suinevere
 | Dependencies: lom_spu_shadow
 | Globals: lom_spu_shadow
 | Params: voice -- 0..23
 | Returns: 1 or 0
 ----------------------*/
static int reverb_enabled(int voice)
{
    unsigned index = (unsigned)voice < 16u ? SHADOW_REVERB : SHADOW_REVERB + 1u;
    unsigned bit = (unsigned)voice < 16u ? (unsigned)voice : (unsigned)voice - 16u;

    return (lom_spu_shadow[index] >> bit) & 1u;
}

/*----------------------
 | noise_enabled
 | Description: Whether a voice's bit is set in the noise (NON) bitmap.
 | Author: suinevere
 | Dependencies: lom_spu_shadow
 | Globals: lom_spu_shadow
 | Params: voice -- 0..23
 | Returns: 1 or 0
 ----------------------*/
static int noise_enabled(int voice)
{
    unsigned index = (unsigned)voice < 16u ? SHADOW_NOISE : SHADOW_NOISE + 1u;
    unsigned bit = (unsigned)voice < 16u ? (unsigned)voice : (unsigned)voice - 16u;

    return (lom_spu_shadow[index] >> bit) & 1u;
}

/*----------------------
 | pitch_mod_enabled
 | Description: Whether a voice's bit is set in the pitch-modulation (PMON) bitmap.
 | Author: suinevere
 | Dependencies: lom_spu_shadow
 | Globals: lom_spu_shadow
 | Params: voice -- 0..23
 | Returns: 1 or 0
 ----------------------*/
static int pitch_mod_enabled(int voice)
{
    unsigned index = (unsigned)voice < 16u ? SHADOW_PITCH_MOD : SHADOW_PITCH_MOD + 1u;
    unsigned bit = (unsigned)voice < 16u ? (unsigned)voice : (unsigned)voice - 16u;

    return (lom_spu_shadow[index] >> bit) & 1u;
}

/*----------------------
 | update_send
 | Description: Recomputes a voice's DSP send from its DIPAN and reverb bit, and tells the backend only when it changed since the last send.
 | Author: suinevere
 | Dependencies: aica_reverb_send, reverb_enabled
 | Globals: s_backend, s_send
 | Params: voice -- 0..23, dipan -- the voice's current DIPAN
 | Returns: N/A
 ----------------------*/
static void update_send(int voice, unsigned char dipan)
{
    unsigned char isel;
    unsigned char imxl;
    unsigned char combo;

    if (s_backend->set_send == NULL)
        return;
    aica_reverb_send(dipan, reverb_enabled(voice), &isel, &imxl);
    combo = (unsigned char)(isel | (unsigned)(imxl << 4));
    if (combo == s_send[voice])
        return;
    s_backend->set_send(voice, isel, imxl);
    s_send[voice] = combo;
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
 | Description: Keys a voice on at its start register: a set NON bit plays the looping noise table at the noise clock's pitch as 8-bit PCM, else the sample map gives the span (a too-long sample streams, an unwritten address logs and stays silent); the DSP send comes from DIPAN and the reverb bit; clears the voice's pitch-modulation mask so the next tick re-pitches it unconditionally; marks the voice held.
 | Author: suinevere
 | Dependencies: noise_enabled, spu_noise_span, spu_noise_pitch, spu_shim_noise_clock, spu_sample_map_locate, aica_pitch_word, aica_envelope, level_pan, aica_reverb_send, reverb_enabled, spu_long_voice_stop, spu_long_voice_start, log_unwritten
 | Globals: s_backend, s_key_ons, s_loudest_tl, s_send, s_modulated, s_held
 | Params: voice -- 0..23
 | Returns: N/A
 ----------------------*/
static void key_on(int voice)
{
    SpuVoiceSetup setup;

    if (noise_enabled(voice))
    {
        spu_noise_span(&setup.span);
        setup.pcm8 = 1;
        setup.pitch = aica_pitch_word(spu_noise_pitch(spu_shim_noise_clock()), 44100);
    }
    else
    {
        setup.pcm8 = 0;
        if (spu_sample_map_locate(reg(voice, VOICE_START), reg(voice, VOICE_REPEAT), &setup.span) != 0)
        {
            log_unwritten((unsigned)reg(voice, VOICE_START) * 8u);
            return;
        }
        setup.pitch = aica_pitch_word(reg(voice, VOICE_PITCH), 44100);
    }
    level_pan(voice, &setup.tl, &setup.dipan);
    aica_envelope(reg(voice, VOICE_ADSR_LOW), reg(voice, VOICE_ADSR_HIGH), &setup.eg10, &setup.eg14);
    aica_reverb_send(setup.dipan, reverb_enabled(voice), &setup.isel, &setup.imxl);
    s_send[voice] = (unsigned char)(setup.isel | (unsigned)(setup.imxl << 4));
    s_modulated &= ~(1u << voice);
    s_held |= 1u << voice;
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
 | Description: Applies a voice's changed pitch (unless it is a noise voice, whose pitch tracks the noise clock instead, or a pitch-modulated voice, whose pitch tracks the previous voice instead), volume, envelope and DSP send registers while it plays.
 | Author: suinevere
 | Dependencies: dirty, noise_enabled, aica_pitch_word, level_pan, update_send, aica_envelope
 | Globals: s_backend, s_modulated
 | Params: bits -- dirty bits, voice -- 0..23
 | Returns: N/A
 ----------------------*/
static void update_voice(const unsigned bits[8], int voice)
{
    unsigned base = (unsigned)voice * VOICE_HALFWORDS;

    if (dirty(bits, base + VOICE_PITCH) && !noise_enabled(voice) && !(s_modulated & (1u << voice)))
        s_backend->set_pitch(voice, aica_pitch_word(reg(voice, VOICE_PITCH), 44100));
    if (dirty(bits, base + VOICE_VOLUME_LEFT) || dirty(bits, base + VOICE_VOLUME_RIGHT))
    {
        unsigned char tl;
        unsigned char dipan;

        level_pan(voice, &tl, &dipan);
        s_backend->set_level(voice, tl, dipan);
        update_send(voice, dipan);
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
 | resend_changed_voices
 | Description: Re-sends every voice whose reverb bit toggled between prev and current, computing each one's current DIPAN.
 | Author: suinevere
 | Dependencies: level_pan, update_send
 | Globals: N/A
 | Params: prev / current -- the halfword's bits as of the last flush and now, first_voice -- 0 or 16
 | Returns: N/A
 ----------------------*/
static void resend_changed_voices(unsigned short prev, unsigned short current, int first_voice)
{
    unsigned short changed = (unsigned short)(prev ^ current);
    int voice;

    for (voice = first_voice; voice < first_voice + 16 && voice < SPU_VOICE_COUNT; voice++)
        if (changed & (1u << (voice - first_voice)))
        {
            unsigned char tl;
            unsigned char dipan;

            level_pan(voice, &tl, &dipan);
            update_send(voice, dipan);
        }
}

/*----------------------
 | resend_noise_voices
 | Description: Re-keys every held, still-sounding voice whose NON bit toggled, since the AICA latches SA only at key-on; skips voices keyed on or off this tick and forgets held voices whose one-shot already ended.
 | Author: suinevere
 | Dependencies: key_on
 | Globals: s_held, s_backend
 | Params: prev / current -- the halfword's bits as of the last flush and now, first_voice -- 0 or 16, on / off -- this tick's key-on and key-off masks
 | Returns: N/A
 ----------------------*/
static void resend_noise_voices(unsigned short prev, unsigned short current, int first_voice, unsigned on, unsigned off)
{
    unsigned short changed = (unsigned short)(prev ^ current);
    int voice;

    for (voice = first_voice; voice < first_voice + 16 && voice < SPU_VOICE_COUNT; voice++)
        if ((changed & (1u << (voice - first_voice))) && !(on & (1u << voice)) && !(off & (1u << voice)) && (s_held & (1u << voice)))
        {
            if (s_backend->envelope(voice) > 0)
                key_on(voice);
            else
                s_held &= ~(1u << voice);
        }
}

/*----------------------
 | resend_noise_pitch
 | Description: When the noise clock changed since the last flush, re-pitches every voice with its NON bit currently set.
 | Author: suinevere
 | Dependencies: spu_shim_noise_clock_take, spu_noise_pitch, aica_pitch_word, noise_enabled
 | Globals: s_backend
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void resend_noise_pitch(void)
{
    unsigned clock;
    unsigned short pitch;
    int voice;

    if (!spu_shim_noise_clock_take(&clock))
        return;
    pitch = aica_pitch_word(spu_noise_pitch(clock), 44100);
    for (voice = 0; voice < SPU_VOICE_COUNT; voice++)
        if (noise_enabled(voice))
            s_backend->set_pitch(voice, pitch);
}

/*----------------------
 | flush_globals
 | Description: Re-keys held voices whose NON bit changed, re-sends voices whose reverb bit changed, and passes CD volume changes to the backend.
 | Author: suinevere
 | Dependencies: dirty, resend_noise_voices, resend_changed_voices
 | Globals: lom_spu_shadow, s_backend, s_prev_reverb_low, s_prev_reverb_high, s_prev_noise_low, s_prev_noise_high
 | Params: bits -- dirty bits, on / off -- this tick's key-on and key-off masks
 | Returns: N/A
 ----------------------*/
static void flush_globals(const unsigned bits[8], unsigned on, unsigned off)
{
    if (dirty(bits, SHADOW_NOISE) || dirty(bits, SHADOW_NOISE + 1))
    {
        unsigned short low = lom_spu_shadow[SHADOW_NOISE];
        unsigned short high = lom_spu_shadow[SHADOW_NOISE + 1];

        resend_noise_voices(s_prev_noise_low, low, 0, on, off);
        resend_noise_voices(s_prev_noise_high, high, 16, on, off);
        s_prev_noise_low = low;
        s_prev_noise_high = high;
    }
    if (dirty(bits, SHADOW_REVERB) || dirty(bits, SHADOW_REVERB + 1))
    {
        unsigned short low = lom_spu_shadow[SHADOW_REVERB];
        unsigned short high = lom_spu_shadow[SHADOW_REVERB + 1];

        resend_changed_voices(s_prev_reverb_low, low, 0);
        resend_changed_voices(s_prev_reverb_high, high, 16);
        s_prev_reverb_low = low;
        s_prev_reverb_high = high;
    }
    if ((dirty(bits, SHADOW_CD_VOLUME) || dirty(bits, SHADOW_CD_VOLUME + 1)) && s_backend->cd_volume != NULL)
        s_backend->cd_volume((short)lom_spu_shadow[SHADOW_CD_VOLUME], (short)lom_spu_shadow[SHADOW_CD_VOLUME + 1]);
}

/*----------------------
 | pitch_mod_service
 | Description: For each voice 1-23 with its PMON bit set and NON clear, re-pitches it each tick from the previous voice's current sample and envelope, sending only when the AICA pitch word changed; a voice leaving the mask gets its base pitch written once, unless it just turned noisy, where key_on/resend_noise_pitch own the pitch instead.
 | Author: suinevere
 | Dependencies: pitch_mod_enabled, noise_enabled, spu_pitch_mod, aica_pitch_word, reg
 | Globals: s_backend, s_modulated, s_mod_pitch
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void pitch_mod_service(void)
{
    int voice;

    if (s_backend == NULL || s_backend->sample == NULL)
        return;
    for (voice = 1; voice < SPU_VOICE_COUNT; voice++)
    {
        unsigned bit = 1u << voice;

        if (pitch_mod_enabled(voice) && !noise_enabled(voice))
        {
            unsigned short pitch = spu_pitch_mod(reg(voice, VOICE_PITCH), s_backend->sample(voice - 1), s_backend->envelope(voice - 1));
            unsigned short word = aica_pitch_word(pitch, 44100);

            if (!(s_modulated & bit) || word != s_mod_pitch[voice])
                s_backend->set_pitch(voice, word);
            s_mod_pitch[voice] = word;
            s_modulated |= bit;
        }
        else if (s_modulated & bit)
        {
            if (!noise_enabled(voice))
                s_backend->set_pitch(voice, aica_pitch_word(reg(voice, VOICE_PITCH), 44100));
            s_modulated &= ~bit;
        }
    }
}

/*----------------------
 | spu_voice_flush
 | Description: Applies the SPU register changes since the last call through the backend (key off first, then live changes, then key on), re-pitches noise voices on a clock change even with nothing else dirty, refills streamed voices, then applies pitch modulation; run after each AKAO tick.
 | Author: suinevere
 | Dependencies: spu_shadow_take_dirty, update_voice, key_on, flush_globals, resend_noise_pitch, spu_long_voice_service, pitch_mod_service
 | Globals: spu_shadow_key_events, s_backend, s_held
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
            {
                s_backend->key_off(voice);
                s_held &= ~(1u << voice);
            }
        for (voice = 0; voice < SPU_VOICE_COUNT; voice++)
            if (!(on & (1u << voice)))
                update_voice(bits, voice);
        for (voice = 0; voice < SPU_VOICE_COUNT; voice++)
            if (on & (1u << voice))
                key_on(voice);
        flush_globals(bits, on, off);
    }
    if (s_backend != NULL)
    {
        resend_noise_pitch();
        spu_long_voice_service(s_backend);
        pitch_mod_service();
    }
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

/*----------------------
 | spu_shim_set_noise_clock
 | Description: Stores the SPU noise clock SpuSetNoiseClock set and marks it changed for the next flush.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_noise_clock, s_noise_clock_changed
 | Params: clock -- SpuSetNoiseClock's low 6 bits
 | Returns: N/A
 ----------------------*/
void spu_shim_set_noise_clock(unsigned clock)
{
    s_noise_clock = clock;
    s_noise_clock_changed = 1;
}

/*----------------------
 | spu_shim_noise_clock
 | Description: The SPU noise clock last set by SpuSetNoiseClock, or 0 before any call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_noise_clock
 | Params: N/A
 | Returns: the clock
 ----------------------*/
unsigned spu_shim_noise_clock(void)
{
    return s_noise_clock;
}

/*----------------------
 | spu_shim_noise_clock_take
 | Description: Reads and resets whether the noise clock changed since the last call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_noise_clock, s_noise_clock_changed
 | Params: clock -- receives the clock, only when returning 1
 | Returns: 1 once per change, else 0
 ----------------------*/
int spu_shim_noise_clock_take(unsigned *clock)
{
    if (!s_noise_clock_changed)
        return 0;
    *clock = s_noise_clock;
    s_noise_clock_changed = 0;
    return 1;
}

/*----------------------
 | spu_voice_flush_reset
 | Description: Forgets every voice's cached DSP send, the reverb and noise bitmap copies, the noise clock change flag, which voices pitch modulation drives, and which voices are held; call from SpuInit alongside spu_shadow_reset and spu_sample_map_reset.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_send, s_prev_reverb_low, s_prev_reverb_high, s_prev_noise_low, s_prev_noise_high, s_noise_clock, s_noise_clock_changed, s_modulated, s_held
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void spu_voice_flush_reset(void)
{
    int voice;

    for (voice = 0; voice < SPU_VOICE_COUNT; voice++)
        s_send[voice] = 0xFFu;
    s_prev_reverb_low = 0;
    s_prev_reverb_high = 0;
    s_prev_noise_low = 0;
    s_prev_noise_high = 0;
    s_noise_clock = 0;
    s_noise_clock_changed = 0;
    s_modulated = 0;
    s_held = 0;
}
