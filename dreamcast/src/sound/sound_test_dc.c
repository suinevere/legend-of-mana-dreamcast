/*----------------------
 | sound_test_dc.c
 | Description: Test-image only: plays a sine on the AICA stream channel for the pad script's tone step, a 1050 Hz sine sample longer than one AICA channel reaches on SPU voice 23 for its longvoice step, reverb, noise and pitch-modulation cases through the shim for its effects step, and any music resource's song or effect resource's banks for the sweep's music and sfxset steps, driven from the per-vblank pad-script poll.
 | Author: suinevere
 | Dependencies: aica_stream.h, akao_tick_dc.h, pad_script.h, dc_system.h, vsync_pacer.h, libspu.h, main/audio/spu_registers.h, main/audio/akao_cmd.h, main/cdrom.h, math.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <math.h>

#include <libspu.h>

#include "main/audio/akao_cmd.h"
#include "main/audio/spu_registers.h"
#include "main/cdrom.h"
#include "spu_shim.h"
#include "aica_stream.h"
#include "akao_tick_dc.h"
#include "dc_system.h"
#include "pad_script.h"
#include "vsync_pacer.h"

/*----------------------
 | TONE_RATE / TONE_AMPLITUDE / TONE_CHUNK / TONE_DRAIN_VBLANKS
 | Description: Stream rate, sine peak, frames generated per push, and vblanks the ring plays out after the last push.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TONE_RATE 44100u
#define TONE_AMPLITUDE 30000.0f
#define TONE_CHUNK 512u
#define TONE_DRAIN_VBLANKS 30

/*----------------------
 | s_active / s_phase / s_step / s_left / s_drain
 | Description: Whether a tone plays, its phase and phase step in radians, frames still to push, and vblanks drained since the last push.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_active;
static float s_phase;
static float s_step;
static unsigned s_left;
static int s_drain;

/*----------------------
 | LONG_VOICE / LONG_SPU_ADDR / LONG_BLOCKS / LONG_LOOP_BLOCK / LONG_PERIOD / LONG_AMPLITUDE
 | Description: The long-voice check: SPU voice 23, a free SPU address before any bank loads, 3,000 ADPCM blocks (84,000 frames) looping from block 1,500, and a 42-frame (1050 Hz) sine so the loop holds whole periods.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LONG_VOICE 23
#define LONG_SPU_ADDR 0x48000u
#define LONG_BLOCKS 3000u
#define LONG_LOOP_BLOCK 1500u
#define LONG_PERIOD 42.0f
#define LONG_AMPLITUDE 14000.0f

/*----------------------
 | s_long_adpcm / s_long_left / s_long_pending
 | Description: The encoded long sample, vblanks until the long voice is keyed off (0 when idle), and the seconds of a long voice waiting for AKAO's tick to run.
 | Author: suinevere
 | Dependencies: LONG_BLOCKS
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_long_adpcm[LONG_BLOCKS * 16u];
static int s_long_left;
static int s_long_pending;

/*----------------------
 | sound_test_tone
 | Description: Starts a mono sine of hz for seconds on the stream channel and logs "tone start <hz> at <vblank>".
 | Author: suinevere
 | Dependencies: aica_stream_start, vsync_count, dc_log
 | Globals: s_active, s_phase, s_step, s_left, s_drain
 | Params: hz -- frequency, seconds -- length
 | Returns: N/A
 ----------------------*/
void sound_test_tone(int hz, int seconds)
{
    if (s_active || aica_stream_start(TONE_RATE, 1) != 0)
    {
        dc_log("tone busy");
        return;
    }
    s_phase = 0.0f;
    s_step = 6.2831853f * (float)hz / (float)TONE_RATE;
    s_left = (unsigned)seconds * TONE_RATE;
    s_drain = 0;
    s_active = 1;
    dc_log("tone start %d at %u", hz, vsync_count());
}

/*----------------------
 | encode_sine
 | Description: Encodes a sine of period frames and peak amplitude as filter-0, shift-1 ADPCM, flagging loop_block as the loop start and the last block as end and repeat.
 | Author: suinevere
 | Dependencies: sinf, lroundf
 | Globals: N/A
 | Params: dst -- blocks * 16 bytes, blocks -- ADPCM blocks, period -- whole frames per cycle, amplitude -- peak, loop_block -- loop start block
 | Returns: N/A
 ----------------------*/
static void encode_sine(unsigned char *dst, unsigned blocks, float period, float amplitude, unsigned loop_block)
{
    unsigned block;
    unsigned i;

    for (block = 0; block < blocks; block++)
    {
        unsigned char *p = dst + block * 16u;

        p[0] = 0x01;
        p[1] = (unsigned char)(block == loop_block ? 0x04 : (block == blocks - 1u ? 0x03 : 0x00));
        for (i = 0; i < 28u; i++)
        {
            float x = amplitude * sinf(6.2831853f * (float)((block * 28u + i) % (unsigned)period) / period);
            int nibble = (int)lroundf(x / 2048.0f);

            if (nibble > 7)
                nibble = 7;
            if (nibble < -8)
                nibble = -8;
            if (i & 1u)
                p[2 + i / 2u] |= (unsigned char)((nibble & 0xF) << 4);
            else
                p[2 + i / 2u] = (unsigned char)(nibble & 0xF);
        }
    }
}

/*----------------------
 | set_voice
 | Description: Points an SPU voice at a sample through the register shadow with both volumes at volume, pitch 0x1000 and the long voice's full-sustain ADSR.
 | Author: suinevere
 | Dependencies: SPU_VOICE_REG
 | Globals: N/A
 | Params: voice -- SPU voice, spu_addr -- sample's SPU byte address, volume -- left and right volume
 | Returns: N/A
 ----------------------*/
static void set_voice(int voice, unsigned spu_addr, u16 volume)
{
    SPU_VOICE_REG(voice, volume_left) = volume;
    SPU_VOICE_REG(voice, volume_right) = volume;
    SPU_VOICE_REG(voice, pitch) = 0x1000;
    SPU_VOICE_REG(voice, start_addr) = (u16)(spu_addr / 8u);
    SPU_VOICE_REG(voice, repeat_addr) = 0;
    SPU_VOICE_REG(voice, adsr_low) = 0x000F;
    SPU_VOICE_REG(voice, adsr_high) = 0x0000;
}

/*----------------------
 | start_long_voice
 | Description: Uploads the long sample with SpuWrite and keys SPU voice 23 on it through the register shadow, the way AKAO keys a voice, for seconds; logs "longvoice start".
 | Author: suinevere
 | Dependencies: encode_sine, set_voice, SpuSetTransferStartAddr, SpuWrite, SPU_KEY_ON_HIGH, dc_log
 | Globals: s_long_adpcm, s_long_left
 | Params: seconds -- length
 | Returns: N/A
 ----------------------*/
static void start_long_voice(int seconds)
{
    encode_sine(s_long_adpcm, LONG_BLOCKS, LONG_PERIOD, LONG_AMPLITUDE, LONG_LOOP_BLOCK);
    SpuSetTransferStartAddr(LONG_SPU_ADDR);
    SpuWrite(s_long_adpcm, sizeof(s_long_adpcm));
    set_voice(LONG_VOICE, LONG_SPU_ADDR, 0x3FFF);
    SPU_KEY_ON_HIGH = (u16)(1u << (LONG_VOICE - 16));
    s_long_left = seconds * 60;
    dc_log("longvoice start");
}

/*----------------------
 | sound_test_longvoice
 | Description: Asks for the long voice; it starts on the first poll after AKAO's tick runs, since AKAO's SpuInit clears the sample map.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_long_pending
 | Params: seconds -- length
 | Returns: N/A
 ----------------------*/
void sound_test_longvoice(int seconds)
{
    s_long_pending = seconds;
}

/*----------------------
 | FX_CLICK_ADDR / FX_MOD_ADDR / FX_CARRIER_ADDR
 | Description: SPU addresses of the effects step's one-block click, its 5 Hz modulator sine and its 1,050 Hz carrier sine, clear of the long voice.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FX_CLICK_ADDR 0x60000u
#define FX_MOD_ADDR 0x60010u
#define FX_CARRIER_ADDR 0x61400u

/*----------------------
 | FX_REVERB_VOICE / FX_NOISE_VOICE / FX_MOD_VOICE / FX_CARRIER_VOICE
 | Description: SPU voices the effects step plays its reverb click, noise, modulator and pitch-modulated carrier on.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FX_REVERB_VOICE 22
#define FX_NOISE_VOICE 21
#define FX_MOD_VOICE 19
#define FX_CARRIER_VOICE 20

/*----------------------
 | FX_MOD_BLOCKS / FX_MOD_PERIOD / FX_MOD_AMPLITUDE / FX_CARRIER_BLOCKS / FX_CARRIER_PERIOD / FX_CARRIER_AMPLITUDE
 | Description: The modulator is one 8,820-frame (5 Hz) cycle in 315 blocks at peak 4,000; the carrier is 14 cycles of 42 frames (1,050 Hz) in 21 blocks at peak 14,000.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FX_MOD_BLOCKS 315u
#define FX_MOD_PERIOD 8820.0f
#define FX_MOD_AMPLITUDE 4000.0f
#define FX_CARRIER_BLOCKS 21u
#define FX_CARRIER_PERIOD 42.0f
#define FX_CARRIER_AMPLITUDE 14000.0f

/*----------------------
 | FX_NOISE_CLOCK / FX_REVERB_DEPTH / FX_VOLUME
 | Description: Noise clock 0x3F, which plays the noise table at 44.1 kHz; full reverb output depth; and the click and carrier voices' volume.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FX_NOISE_CLOCK 0x3F
#define FX_REVERB_DEPTH 0x7FFF
#define FX_VOLUME 0x3FFF

/*----------------------
 | FxPhase
 | Description: The effects sequence's phases in play order; FX_NOISE_GAP follows the noise voice's key-off and FX_UNMODULATED holds the carrier with its pitch-modulation bit clear.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef enum
{
    FX_IDLE,
    FX_REVERB_ON,
    FX_REVERB_OFF,
    FX_NOISE,
    FX_NOISE_GAP,
    FX_PITCHMOD,
    FX_UNMODULATED,
    FX_PHASE_COUNT
} FxPhase;

/*----------------------
 | s_fx_vblanks
 | Description: How long each phase holds, in vblanks: 3 s per reverb click, 1 s of noise and 0.3 s after it, 1.5 s modulated and 1 s unmodulated.
 | Author: suinevere
 | Dependencies: FxPhase
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static const int s_fx_vblanks[FX_PHASE_COUNT] = {0, 180, 180, 60, 18, 90, 60};

/*----------------------
 | s_fx_click / s_fx_mod / s_fx_carrier / s_fx_phase / s_fx_left / s_fx_pending
 | Description: The effects step's samples (the click is silence with 28,672 at sample 4, ending without a loop), its current phase, vblanks left in it, and whether a sequence waits for AKAO's tick to run.
 | Author: suinevere
 | Dependencies: FX_MOD_BLOCKS, FX_CARRIER_BLOCKS, FxPhase
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_fx_click[16] = {0x00, 0x01, 0x00, 0x00, 0x07};
static unsigned char s_fx_mod[FX_MOD_BLOCKS * 16u];
static unsigned char s_fx_carrier[FX_CARRIER_BLOCKS * 16u];
static FxPhase s_fx_phase;
static int s_fx_left;
static int s_fx_pending;

/*----------------------
 | voice_bit
 | Description: A voice 16-23's bit in the high halfword of an SPU voice bitmap.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: voice -- SPU voice 16-23
 | Returns: the bit
 ----------------------*/
static u16 voice_bit(int voice)
{
    return (u16)(1u << (voice - 16));
}

/*----------------------
 | effects_hold
 | Description: Re-applies the current phase's settings and effect bits, since AKAO's mask and clock updates can overwrite them.
 | Author: suinevere
 | Dependencies: voice_bit, SpuSetReverbModeDepth, SpuSetNoiseClock, SPU_REVERB_HIGH, SPU_NOISE_HIGH, SPU_PITCH_MOD_HIGH
 | Globals: s_fx_phase
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void effects_hold(void)
{
    if (s_fx_phase == FX_REVERB_ON || s_fx_phase == FX_REVERB_OFF)
        SpuSetReverbModeDepth(FX_REVERB_DEPTH, FX_REVERB_DEPTH);
    if (s_fx_phase == FX_REVERB_ON)
        SPU_REVERB_HIGH |= voice_bit(FX_REVERB_VOICE);
    if (s_fx_phase == FX_NOISE || s_fx_phase == FX_NOISE_GAP)
    {
        SpuSetNoiseClock(FX_NOISE_CLOCK);
        SPU_NOISE_HIGH |= voice_bit(FX_NOISE_VOICE);
    }
    if (s_fx_phase == FX_PITCHMOD)
        SPU_PITCH_MOD_HIGH |= voice_bit(FX_CARRIER_VOICE);
}

/*----------------------
 | effects_enter
 | Description: Starts a phase: uploads the samples and keys the click with its reverb bit set, keys it again with the bit clear, keys the noise voice and later off, keys the modulator and carrier, clears the carrier's bit, or keys both off; logs "effects <phase>" and "effects end".
 | Author: suinevere
 | Dependencies: effects_hold, set_voice, voice_bit, encode_sine, SpuSetTransferStartAddr, SpuWrite, SPU_KEY_ON_HIGH, SPU_KEY_OFF_HIGH, SPU_REVERB_HIGH, SPU_NOISE_HIGH, SPU_PITCH_MOD_HIGH, dc_log
 | Globals: s_fx_click, s_fx_mod, s_fx_carrier, s_fx_phase, s_fx_left
 | Params: phase -- phase to start
 | Returns: N/A
 ----------------------*/
static void effects_enter(FxPhase phase)
{
    s_fx_phase = phase;
    s_fx_left = s_fx_vblanks[phase];
    switch (phase)
    {
    case FX_REVERB_ON:
        encode_sine(s_fx_mod, FX_MOD_BLOCKS, FX_MOD_PERIOD, FX_MOD_AMPLITUDE, 0);
        encode_sine(s_fx_carrier, FX_CARRIER_BLOCKS, FX_CARRIER_PERIOD, FX_CARRIER_AMPLITUDE, 0);
        SpuSetTransferStartAddr(FX_CLICK_ADDR);
        SpuWrite(s_fx_click, sizeof(s_fx_click));
        SpuSetTransferStartAddr(FX_MOD_ADDR);
        SpuWrite(s_fx_mod, sizeof(s_fx_mod));
        SpuSetTransferStartAddr(FX_CARRIER_ADDR);
        SpuWrite(s_fx_carrier, sizeof(s_fx_carrier));
        effects_hold();
        set_voice(FX_REVERB_VOICE, FX_CLICK_ADDR, FX_VOLUME);
        SPU_KEY_ON_HIGH = voice_bit(FX_REVERB_VOICE);
        dc_log("effects reverb-on");
        break;
    case FX_REVERB_OFF:
        SPU_REVERB_HIGH &= (u16)~voice_bit(FX_REVERB_VOICE);
        effects_hold();
        set_voice(FX_REVERB_VOICE, FX_CLICK_ADDR, FX_VOLUME);
        SPU_KEY_ON_HIGH = voice_bit(FX_REVERB_VOICE);
        dc_log("effects reverb-off");
        break;
    case FX_NOISE:
        effects_hold();
        set_voice(FX_NOISE_VOICE, FX_CLICK_ADDR, FX_VOLUME);
        SPU_KEY_ON_HIGH = voice_bit(FX_NOISE_VOICE);
        dc_log("effects noise");
        break;
    case FX_NOISE_GAP:
        effects_hold();
        SPU_KEY_OFF_HIGH = voice_bit(FX_NOISE_VOICE);
        break;
    case FX_PITCHMOD:
        SPU_NOISE_HIGH &= (u16)~voice_bit(FX_NOISE_VOICE);
        effects_hold();
        set_voice(FX_MOD_VOICE, FX_MOD_ADDR, 0);
        set_voice(FX_CARRIER_VOICE, FX_CARRIER_ADDR, FX_VOLUME);
        SPU_KEY_ON_HIGH = (u16)(voice_bit(FX_MOD_VOICE) | voice_bit(FX_CARRIER_VOICE));
        dc_log("effects pitchmod");
        break;
    case FX_UNMODULATED:
        SPU_PITCH_MOD_HIGH &= (u16)~voice_bit(FX_CARRIER_VOICE);
        break;
    case FX_IDLE:
    default:
        SPU_KEY_OFF_HIGH = (u16)(voice_bit(FX_MOD_VOICE) | voice_bit(FX_CARRIER_VOICE));
        dc_log("effects end");
        break;
    }
}

/*----------------------
 | effects_poll
 | Description: Once per vblank: starts a requested sequence once AKAO's tick runs, holds the current phase's settings, and moves to the next phase when its time is up.
 | Author: suinevere
 | Dependencies: effects_enter, effects_hold, akao_tick_dc_count
 | Globals: s_fx_phase, s_fx_left, s_fx_pending
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void effects_poll(void)
{
    if (s_fx_pending && akao_tick_dc_count() > 0)
    {
        s_fx_pending = 0;
        effects_enter(FX_REVERB_ON);
        return;
    }
    if (s_fx_phase == FX_IDLE)
        return;
    effects_hold();
    if (--s_fx_left == 0)
        effects_enter(s_fx_phase + 1 < FX_PHASE_COUNT ? (FxPhase)(s_fx_phase + 1) : FX_IDLE);
}

/*----------------------
 | sound_test_effects
 | Description: Asks for the effects sequence; it starts on the first poll after AKAO's tick runs, like the long voice.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_fx_pending
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void sound_test_effects(void)
{
    s_fx_pending = 1;
}

/*----------------------
 | SONG_BYTES / SFX_SET_FIRST / SFX_SET_LAST / WEAPON_SFX_FIRST / WEAPON_SFX_LAST / BARE_BANK
 | Description: The sweep's private song area, the effect-table set and weapon-table resource ranges, and the one resource that is a bare bank.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SONG_BYTES 0x10000u
#define SFX_SET_FIRST 81
#define SFX_SET_LAST 118
#define WEAPON_SFX_FIRST 131
#define WEAPON_SFX_LAST 141
#define BARE_BANK 22

/*----------------------
 | s_song
 | Description: Song the sweep plays, copied out of the load buffer so the next read cannot overwrite it while AKAO reads it.
 | Author: suinevere
 | Dependencies: SONG_BYTES
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned char s_song[SONG_BYTES] __attribute__((aligned(4)));

/*----------------------
 | sound_test_music
 | Description: Stops the song, reads a music container, copies its first section as the song, uploads its last section as the bank and plays the song at full volume, the way field_load_song does; logs "music <resource> at <vblank>".
 | Author: suinevere
 | Dependencies: akao_stop_song, cdrom_queue_read, bcopy, akao_upload_bank_blocking, spu_shim_key_stats_take, akao_play_song, akao_set_song_volume, vsync_count, dc_log
 | Globals: s_song, lom_load_buffer
 | Params: resource -- CD resource of a music container
 | Returns: N/A
 ----------------------*/
void sound_test_music(int resource)
{
    const u32 *offsets = (const u32 *)lom_load_buffer;
    u32 count;
    u32 length;
    s32 handle;

    unsigned loudest_tl;

    akao_stop_song(0);
    cdrom_queue_read(resource, lom_load_buffer);
    count = offsets[0];
    length = count >= 2u ? offsets[2] - offsets[1] : 0u;
    if (length == 0u || length > SONG_BYTES)
    {
        dc_log("music %d bad song %u of %u sections", resource, (unsigned)length, (unsigned)count);
        return;
    }
    bcopy(lom_load_buffer + offsets[1], s_song, length);
    akao_upload_bank_blocking((AkaoBankHeader *)(lom_load_buffer + offsets[count]), 1);
    spu_shim_key_stats_take(&loudest_tl);
    handle = akao_play_song((AkaoHeader *)s_song);
    akao_set_song_volume(handle, AKAO_VOLUME_MAX);
    dc_log("music %d at %u", resource, vsync_count());
}

/*----------------------
 | sound_test_music_off
 | Description: Stops the song and logs "musicoff at <vblank> keyons <n> tl <lowest total level>" for the voices keyed since the song started.
 | Author: suinevere
 | Dependencies: spu_shim_key_stats_take, akao_stop_song, vsync_count, dc_log
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void sound_test_music_off(void)
{
    unsigned loudest_tl;
    unsigned key_ons = spu_shim_key_stats_take(&loudest_tl);

    akao_stop_song(0);
    dc_log("musicoff at %u keyons %u tl %u", vsync_count(), key_ons, loudest_tl);
}

/*----------------------
 | table_end
 | Description: End of an effect table, which its own last offset entry marks; the table's bank follows it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: table -- effect table
 | Returns: the byte after the table
 ----------------------*/
static unsigned char *table_end(unsigned char *table)
{
    const s32 *words = (const s32 *)table;

    return table + words[words[0]];
}

/*----------------------
 | sound_test_sfxset
 | Description: Reads an effect resource and uploads every bank in it with akao_upload_bank_blocking: each section's bank after its table for an effect-table set, the bank after a weapon table, a bare bank, or a container's last section; logs "sfxset <resource> banks <n>".
 | Author: suinevere
 | Dependencies: cdrom_queue_read, table_end, akao_upload_bank_blocking, dc_log
 | Globals: lom_load_buffer
 | Params: resource -- CD resource of an effect set
 | Returns: N/A
 ----------------------*/
void sound_test_sfxset(int resource)
{
    const u32 *offsets = (const u32 *)lom_load_buffer;
    unsigned banks = 0;
    u32 i;

    cdrom_queue_read(resource, lom_load_buffer);
    if (resource >= SFX_SET_FIRST && resource <= SFX_SET_LAST)
    {
        for (i = 1; i <= offsets[0]; i++, banks++)
            akao_upload_bank_blocking((AkaoBankHeader *)table_end(lom_load_buffer + offsets[i]), 1);
    }
    else
    {
        unsigned char *bank;

        if (resource >= WEAPON_SFX_FIRST && resource <= WEAPON_SFX_LAST)
            bank = table_end(lom_load_buffer);
        else if (resource == BARE_BANK)
            bank = lom_load_buffer;
        else
            bank = lom_load_buffer + offsets[offsets[0]];
        akao_upload_bank_blocking((AkaoBankHeader *)bank, 1);
        banks = 1;
    }
    dc_log("sfxset %d banks %u", resource, banks);
}

/*----------------------
 | sound_test_poll
 | Description: Once per vblank: starts a requested long voice once AKAO's tick runs, keys it off with "longvoice end" when its time is up, runs the effects sequence, tops the stream up with the next sine frames, and stops it and logs "tone end" once the last frames have played out.
 | Author: suinevere
 | Dependencies: start_long_voice, effects_poll, akao_tick_dc_count, SPU_KEY_OFF_HIGH, aica_stream_free, aica_stream_push, aica_stream_stop, sinf, dc_log
 | Globals: s_active, s_phase, s_step, s_left, s_drain, s_long_left, s_long_pending
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void sound_test_poll(void)
{
    static short chunk[TONE_CHUNK];
    unsigned room;

    if (s_long_pending > 0 && akao_tick_dc_count() > 0)
    {
        start_long_voice(s_long_pending);
        s_long_pending = 0;
    }
    if (s_long_left > 0 && --s_long_left == 0)
    {
        SPU_KEY_OFF_HIGH = (u16)(1u << (LONG_VOICE - 16));
        dc_log("longvoice end");
    }
    effects_poll();
    if (!s_active)
        return;
    if (s_left == 0)
    {
        if (++s_drain >= TONE_DRAIN_VBLANKS)
        {
            aica_stream_stop();
            s_active = 0;
            dc_log("tone end");
        }
        return;
    }
    room = aica_stream_free();
    while (room != 0 && s_left != 0)
    {
        unsigned count = room < TONE_CHUNK ? room : TONE_CHUNK;
        unsigned i;

        if (count > s_left)
            count = s_left;
        for (i = 0; i < count; i++)
        {
            chunk[i] = (short)(TONE_AMPLITUDE * sinf(s_phase));
            s_phase += s_step;
            if (s_phase > 6.2831853f)
                s_phase -= 6.2831853f;
        }
        aica_stream_push(chunk, count);
        s_left -= count;
        room -= count;
    }
}
