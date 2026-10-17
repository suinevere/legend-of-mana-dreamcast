/*----------------------
 | sound_test_dc.c
 | Description: Test-image only: plays a sine on the AICA stream channel for the pad script's tone step, a 1050 Hz sine sample longer than one AICA channel reaches on SPU voice 23 for its longvoice step, and any music resource's song or effect resource's banks for the sweep's music and sfxset steps, driven from the per-vblank pad-script poll.
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
 | encode_long_sample
 | Description: Encodes the 1050 Hz sine as filter-0, shift-1 ADPCM, flagging block LONG_LOOP_BLOCK as the loop start and the last block as end and repeat.
 | Author: suinevere
 | Dependencies: sinf
 | Globals: s_long_adpcm
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void encode_long_sample(void)
{
    unsigned block;
    unsigned i;

    for (block = 0; block < LONG_BLOCKS; block++)
    {
        unsigned char *p = s_long_adpcm + block * 16u;

        p[0] = 0x01;
        p[1] = (unsigned char)(block == LONG_LOOP_BLOCK ? 0x04 : (block == LONG_BLOCKS - 1u ? 0x03 : 0x00));
        for (i = 0; i < 28u; i++)
        {
            float x = LONG_AMPLITUDE * sinf(6.2831853f * (float)((block * 28u + i) % 42u) / LONG_PERIOD);
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
 | start_long_voice
 | Description: Uploads the long sample with SpuWrite and keys SPU voice 23 on it through the register shadow, the way AKAO keys a voice, for seconds; logs "longvoice start".
 | Author: suinevere
 | Dependencies: encode_long_sample, SpuSetTransferStartAddr, SpuWrite, SPU_VOICE_REG, SPU_KEY_ON_HIGH, dc_log
 | Globals: s_long_left
 | Params: seconds -- length
 | Returns: N/A
 ----------------------*/
static void start_long_voice(int seconds)
{
    encode_long_sample();
    SpuSetTransferStartAddr(LONG_SPU_ADDR);
    SpuWrite(s_long_adpcm, sizeof(s_long_adpcm));
    SPU_VOICE_REG(LONG_VOICE, volume_left) = 0x3FFF;
    SPU_VOICE_REG(LONG_VOICE, volume_right) = 0x3FFF;
    SPU_VOICE_REG(LONG_VOICE, pitch) = 0x1000;
    SPU_VOICE_REG(LONG_VOICE, start_addr) = (u16)(LONG_SPU_ADDR / 8u);
    SPU_VOICE_REG(LONG_VOICE, repeat_addr) = 0;
    SPU_VOICE_REG(LONG_VOICE, adsr_low) = 0x000F;
    SPU_VOICE_REG(LONG_VOICE, adsr_high) = 0x0000;
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
 | Description: Once per vblank: starts a requested long voice once AKAO's tick runs, keys it off with "longvoice end" when its time is up, tops the stream up with the next sine frames, and stops it and logs "tone end" once the last frames have played out.
 | Author: suinevere
 | Dependencies: start_long_voice, akao_tick_dc_count, SPU_KEY_OFF_HIGH, aica_stream_free, aica_stream_push, aica_stream_stop, sinf, dc_log
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
