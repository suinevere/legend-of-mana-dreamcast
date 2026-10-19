/*----------------------
 | test_spu_voice_flush.c
 | Description: Host tests for spu_voice_flush.c and libspu_shim.c: AKAO-style SPU register writes become the right backend calls, uploads decode and clip, envelope reads report real levels, noise voices play the noise table, and reverb sends, mode and depth reach the backend.
 | Author: suinevere
 | Dependencies: spu_voice_flush.c, libspu_shim.c, spu_long_voice.c, spu_sample_map.c, spu_noise.c, aica_convert.c, spu_shadow.c, include/main/audio/spu_registers.h, include/sdk/libspu.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "spu_shadow.h"

typedef unsigned short u16;
typedef signed short s16;
typedef unsigned char u8;

#include "main/audio/spu_registers.h"
#include <libspu.h>

#include "aica_convert.h"
#include "spu_backend.h"
#include "spu_long_voice.h"
#include "spu_noise.h"
#include "spu_shim.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

enum { OP_UPLOAD, OP_KEY_ON, OP_KEY_OFF, OP_PITCH, OP_LEVEL, OP_ENVELOPE, OP_CD, OP_SET_SEND, OP_REVERB_ENABLE, OP_REVERB_DEPTH, OP_REVERB_CLEAR };

typedef struct Event
{
    int op;
    int voice;
    unsigned a;
    unsigned b;
    SpuVoiceSetup setup;
} Event;

static Event s_events[256];
static int s_event_count;
static char s_logs[32][96];
static int s_log_count;
static int s_levels[SPU_VOICE_COUNT];
static unsigned s_pos[SPU_VOICE_COUNT];
static int s_samples[SPU_VOICE_COUNT];
static int s_callbacks;
static unsigned char s_adpcm[3000 * 16];

static void record(int op, int voice, unsigned a, unsigned b, const SpuVoiceSetup *setup)
{
    Event *e = &s_events[s_event_count < 255 ? s_event_count++ : 255];

    e->op = op;
    e->voice = voice;
    e->a = a;
    e->b = b;
    if (setup)
        e->setup = *setup;
}

static void fake_upload(unsigned aica_offset, const short *pcm, unsigned bytes)
{
    (void)pcm;
    record(OP_UPLOAD, -1, aica_offset, bytes, NULL);
}

static void fake_key_on(int voice, const SpuVoiceSetup *setup)
{
    record(OP_KEY_ON, voice, 0, 0, setup);
}

static void fake_key_off(int voice)
{
    record(OP_KEY_OFF, voice, 0, 0, NULL);
}

static void fake_pitch(int voice, unsigned short pitch)
{
    record(OP_PITCH, voice, pitch, 0, NULL);
}

static void fake_level(int voice, unsigned char tl, unsigned char dipan)
{
    record(OP_LEVEL, voice, tl, dipan, NULL);
}

static void fake_envelope_words(int voice, unsigned short eg10, unsigned short eg14)
{
    record(OP_ENVELOPE, voice, eg10, eg14, NULL);
}

static int fake_envelope(int voice)
{
    return s_levels[voice];
}

static unsigned fake_position(int voice)
{
    return s_pos[voice];
}

static void fake_cd(int left, int right)
{
    record(OP_CD, -1, (unsigned)left, (unsigned)right, NULL);
}

static void fake_log(const char *message)
{
    if (s_log_count < 32)
        snprintf(s_logs[s_log_count++], sizeof(s_logs[0]), "%s", message);
}

static void fake_set_send(int voice, unsigned char isel, unsigned char imxl)
{
    record(OP_SET_SEND, voice, isel, imxl, NULL);
}

static void fake_reverb_enable(int on)
{
    record(OP_REVERB_ENABLE, -1, (unsigned)on, 0, NULL);
}

static void fake_reverb_depth(int left, int right)
{
    record(OP_REVERB_DEPTH, -1, (unsigned)left, (unsigned)right, NULL);
}

static void fake_reverb_clear(void)
{
    record(OP_REVERB_CLEAR, -1, 0, 0, NULL);
}

static int fake_sample(int voice)
{
    return s_samples[voice];
}

static const SpuBackend k_backend = {fake_upload, fake_key_on, fake_key_off, fake_pitch, fake_level, fake_envelope_words,
                                     fake_envelope, fake_position, fake_cd, fake_log, fake_set_send, fake_reverb_enable,
                                     fake_reverb_depth, fake_reverb_clear, fake_sample};

static void transfer_done(void)
{
    s_callbacks++;
}

static int count_ops(int op, int voice)
{
    int n = 0;
    int i;

    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == op && (voice < 0 || s_events[i].voice == voice))
            n++;
    return n;
}

static int count_logs(const char *prefix)
{
    int n = 0;
    int i;

    for (i = 0; i < s_log_count; i++)
        if (strncmp(s_logs[i], prefix, strlen(prefix)) == 0)
            n++;
    return n;
}

static void clear_events(void)
{
    s_event_count = 0;
}

static void one_block(unsigned char *b, unsigned char flags)
{
    int i;

    b[0] = 0x0C;
    b[1] = flags;
    for (i = 2; i < 16; i++)
        b[i] = 0x11;
}

static void set_voice(int v, unsigned start, s16 left, s16 right)
{
    SPU_VOICE_REG(v, volume_left) = (u16)left;
    SPU_VOICE_REG(v, volume_right) = (u16)right;
    SPU_VOICE_REG(v, pitch) = 0x1000;
    SPU_VOICE_REG(v, start_addr) = (u16)(start / 8);
    SPU_VOICE_REG(v, adsr_low) = 0x00FF;
    SPU_VOICE_REG(v, adsr_high) = 0x0000;
}

static void key_on(int v)
{
    if (v < 16)
        SPU_KEY_ON_LOW = (u16)(1u << v);
    else
        SPU_KEY_ON_HIGH = (u16)(1u << (v - 16));
}

static void key_off(int v)
{
    if (v < 16)
        SPU_KEY_OFF_LOW = (u16)(1u << v);
    else
        SPU_KEY_OFF_HIGH = (u16)(1u << (v - 16));
}

static void set_noise(int v, int on)
{
    if (v < 16)
    {
        if (on)
            SPU_NOISE_LOW = (u16)(SPU_NOISE_LOW | (1u << v));
        else
            SPU_NOISE_LOW = (u16)(SPU_NOISE_LOW & ~(unsigned)(1u << v));
    }
    else
    {
        if (on)
            SPU_NOISE_HIGH = (u16)(SPU_NOISE_HIGH | (1u << (v - 16)));
        else
            SPU_NOISE_HIGH = (u16)(SPU_NOISE_HIGH & ~(unsigned)(1u << (v - 16)));
    }
}

static void set_pitch_mod(int v, int on)
{
    if (v < 16)
    {
        if (on)
            SPU_PITCH_MOD_LOW = (u16)(SPU_PITCH_MOD_LOW | (1u << v));
        else
            SPU_PITCH_MOD_LOW = (u16)(SPU_PITCH_MOD_LOW & ~(unsigned)(1u << v));
    }
    else
    {
        if (on)
            SPU_PITCH_MOD_HIGH = (u16)(SPU_PITCH_MOD_HIGH | (1u << (v - 16)));
        else
            SPU_PITCH_MOD_HIGH = (u16)(SPU_PITCH_MOD_HIGH & ~(unsigned)(1u << (v - 16)));
    }
}

static void test_nothing_written(void)
{
    spu_voice_flush();
    clear_events();
    spu_voice_flush();
    CHECK(s_event_count == 0);
}

static void test_upload(void)
{
    unsigned char b[16];

    one_block(b, 0x01);
    clear_events();
    SpuSetTransferCallback(transfer_done);
    SpuSetTransferStartAddr(0x1010);
    CHECK(SpuWrite(b, 16) == 16);
    CHECK(count_ops(OP_UPLOAD, -1) == 1);
    CHECK(s_events[0].a == 0x3838 && s_events[0].b == 56);
    CHECK(s_callbacks == 1);
}

static void test_key_on(void)
{
    set_voice(3, 0x1010, 0x3FFF, 0x3FFF);
    key_on(3);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, 3) == 1);
    CHECK(count_ops(OP_KEY_ON, -1) == 1);
    CHECK(s_events[0].setup.span.frames == 28);
    CHECK(s_events[0].setup.span.pcm_offset == 0x3838);
    CHECK(s_events[0].setup.pitch == 0);
    CHECK(s_events[0].setup.tl == 0);
}

static void test_pitch_only(void)
{
    SPU_VOICE_REG(3, pitch) = 0x2000;
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_PITCH, 3) == 1);
    CHECK(s_events[0].a == (1u << 11));
    CHECK(count_ops(OP_KEY_ON, -1) == 0);
}

static void test_off_then_on(void)
{
    key_off(3);
    key_on(3);
    clear_events();
    spu_voice_flush();
    CHECK(s_event_count == 2);
    CHECK(s_events[0].op == OP_KEY_OFF && s_events[0].voice == 3);
    CHECK(s_events[1].op == OP_KEY_ON && s_events[1].voice == 3);
}

static void test_unwritten_start(void)
{
    set_voice(20, 0x60000, 0x3FFF, 0x3FFF);
    key_on(20);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, -1) == 0);
    CHECK(count_logs("spu: key-on at unwritten") == 1);
    key_on(20);
    spu_voice_flush();
    CHECK(count_logs("spu: key-on at unwritten") == 1);
}

static void test_noise_key_on(void)
{
    int i;

    CHECK(SpuSetNoiseClock(0) == 0);
    set_voice(13, 0x1010, 0x3FFF, 0x3FFF);
    set_noise(13, 1);
    key_on(13);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, 13) == 1);
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_KEY_ON && s_events[i].voice == 13)
        {
            CHECK(s_events[i].setup.span.pcm_offset == SPU_NOISE_RAM);
            CHECK(s_events[i].setup.span.frames == SPU_NOISE_FRAMES);
            CHECK(s_events[i].setup.span.loop_start == 0);
            CHECK(s_events[i].setup.span.loops == 1);
            CHECK(s_events[i].setup.span.streamed == 0);
            CHECK(s_events[i].setup.pcm8 == 1);
            CHECK(s_events[i].setup.pitch == aica_pitch_word(spu_noise_pitch(0), 44100));
        }
    s_levels[13] = 1234;
}

static void test_noise_toggle_off_restarts_sample(void)
{
    int i;
    int saw_sample = 0;

    set_noise(13, 0);
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, 13) == 2);
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_KEY_ON && s_events[i].voice == 13 && s_events[i].setup.pcm8 == 0)
        {
            saw_sample = 1;
            CHECK(s_events[i].setup.span.frames == 28);
            CHECK(s_events[i].setup.span.pcm_offset == 0x3838);
        }
    CHECK(saw_sample == 1);
}

static void test_noise_clock_repitches_noise_voices_only(void)
{
    int i;
    unsigned short expect;

    set_voice(14, 0x1010, 0x3FFF, 0x3FFF);
    set_noise(14, 1);
    key_on(14);
    spu_voice_flush();
    clear_events();
    CHECK(SpuSetNoiseClock(0x20) == 0x20);
    spu_voice_flush();
    expect = aica_pitch_word(spu_noise_pitch(0x20), 44100);
    CHECK(count_ops(OP_PITCH, -1) == 1);
    CHECK(count_ops(OP_PITCH, 14) == 1);
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_PITCH && s_events[i].voice == 14)
            CHECK(s_events[i].a == expect);
}

static void test_noise_voice_pitch_write_ignored(void)
{
    SPU_VOICE_REG(14, pitch) = 0x1800;
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_PITCH, 14) == 0);
}

static void test_noise_toggle_key_off_same_tick(void)
{
    set_voice(11, 0x1010, 0x3FFF, 0x3FFF);
    key_on(11);
    spu_voice_flush();
    s_levels[11] = 1234;
    set_noise(11, 1);
    key_off(11);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_OFF, 11) == 1);
    CHECK(count_ops(OP_KEY_ON, 11) == 0);
}

static void test_noise_toggle_key_on_same_tick(void)
{
    s_levels[15] = 1234;
    set_voice(15, 0x1010, 0x3FFF, 0x3FFF);
    set_noise(15, 1);
    key_on(15);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, 15) == 1);
}

static void test_noise_toggle_after_release_no_rekey(void)
{
    set_voice(16, 0x1010, 0x3FFF, 0x3FFF);
    key_on(16);
    spu_voice_flush();
    key_off(16);
    spu_voice_flush();
    s_levels[16] = 1234;
    set_noise(16, 1);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, 16) == 0);
    CHECK(count_ops(OP_KEY_OFF, 16) == 0);
}

static void test_noise_toggle_after_natural_end_no_rekey(void)
{
    set_voice(17, 0x1010, 0x3FFF, 0x3FFF);
    key_on(17);
    spu_voice_flush();
    s_levels[17] = 0;
    set_noise(17, 1);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, 17) == 0);
}

static void test_pitch_modulation(void)
{
    int i;
    unsigned short expect;

    set_voice(5, 0x1010, 0x3FFF, 0x3FFF);
    spu_voice_flush();
    s_samples[4] = 0x4000;
    s_levels[4] = 0x7FFF;
    set_pitch_mod(5, 1);
    clear_events();
    spu_voice_flush();
    expect = aica_pitch_word(spu_pitch_mod(0x1000, s_samples[4], s_levels[4]), 44100);
    CHECK(expect == aica_pitch_word(0x17FF, 44100));
    CHECK(count_ops(OP_PITCH, 5) == 1);
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_PITCH && s_events[i].voice == 5)
            CHECK(s_events[i].a == expect);

    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_PITCH, 5) == 0);

    s_samples[4] = 0x2000;
    clear_events();
    spu_voice_flush();
    expect = aica_pitch_word(spu_pitch_mod(0x1000, s_samples[4], s_levels[4]), 44100);
    CHECK(count_ops(OP_PITCH, 5) == 1);
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_PITCH && s_events[i].voice == 5)
            CHECK(s_events[i].a == expect);

    set_pitch_mod(5, 0);
    clear_events();
    spu_voice_flush();
    expect = aica_pitch_word(0x1000, 44100);
    CHECK(count_ops(OP_PITCH, 5) == 1);
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_PITCH && s_events[i].voice == 5)
            CHECK(s_events[i].a == expect);

    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_PITCH, 5) == 0);

    set_pitch_mod(0, 1);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_PITCH, 0) == 0);
    set_pitch_mod(0, 0);
}

static void test_pitch_modulation_key_on_refreshes(void)
{
    int i;
    unsigned short expect;
    int saw;

    set_voice(6, 0x1010, 0x3FFF, 0x3FFF);
    spu_voice_flush();
    s_samples[5] = 0x4000;
    s_levels[5] = 0x7FFF;
    set_pitch_mod(6, 1);
    clear_events();
    spu_voice_flush();
    expect = aica_pitch_word(spu_pitch_mod(0x1000, s_samples[5], s_levels[5]), 44100);
    CHECK(count_ops(OP_PITCH, 6) == 1);

    key_on(6);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, 6) == 1);
    saw = 0;
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_PITCH && s_events[i].voice == 6 && s_events[i].a == expect)
            saw = 1;
    CHECK(saw == 1);

    set_pitch_mod(6, 0);
    clear_events();
    spu_voice_flush();
}

static void test_pitch_modulation_noise_toggle_keeps_noise_pitch(void)
{
    int i;
    unsigned short last_pitch = 0;
    unsigned short noise_pitch_expect;
    int have_last;

    set_voice(10, 0x1010, 0x3FFF, 0x3FFF);
    spu_voice_flush();
    s_samples[9] = 0x4000;
    s_levels[9] = 0x7FFF;
    set_pitch_mod(10, 1);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_PITCH, 10) == 1);

    s_levels[10] = 777;
    CHECK(SpuSetNoiseClock(0x10) == 0x10);
    set_noise(10, 1);
    clear_events();
    spu_voice_flush();

    noise_pitch_expect = aica_pitch_word(spu_noise_pitch(0x10), 44100);
    have_last = 0;
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].voice == 10 && (s_events[i].op == OP_PITCH || s_events[i].op == OP_KEY_ON))
        {
            last_pitch = s_events[i].op == OP_KEY_ON ? s_events[i].setup.pitch : (unsigned short)s_events[i].a;
            have_last = 1;
        }
    CHECK(have_last == 1);
    CHECK(last_pitch == noise_pitch_expect);

    set_noise(10, 0);
    set_pitch_mod(10, 0);
    clear_events();
    spu_voice_flush();
}

static void test_envelope_readback(void)
{
    short e = -1;

    s_levels[5] = 1234;
    SpuGetVoiceEnvelope(5, &e);
    CHECK(e == 1234);
}

static void test_cd_volume(void)
{
    SPU_CD_VOLUME_LEFT = 0x7FFF;
    SPU_CD_VOLUME_RIGHT = 0x7FFF;
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_CD, -1) == 1);
    CHECK(s_events[0].a == 0x7FFF && s_events[0].b == 0x7FFF);
}

static void test_sweep_volume(void)
{
    SPU_VOICE_REG(1, volume_left) = 0x8010;
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_LEVEL, 1) == 1);
    CHECK(s_events[0].a == 0);
    CHECK(count_logs("spu: volume sweep") == 1);
}

static void upload_long(unsigned spu_addr, unsigned char end_flags)
{
    unsigned b;

    for (b = 0; b < 3000; b++)
    {
        one_block(s_adpcm + b * 16, b == 2999 ? end_flags : 0x00);
        s_adpcm[b * 16] = 0x1A;
    }
    SpuSetTransferStartAddr(spu_addr);
    SpuWrite(s_adpcm, sizeof(s_adpcm));
}

static void test_streamed_key_on(void)
{
    upload_long(0x10000, 0x03);
    set_voice(7, 0x10000, 0x3FFF, 0x3FFF);
    key_on(7);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, 7) == 1);
    {
        int i;

        for (i = 0; i < s_event_count; i++)
            if (s_events[i].op == OP_KEY_ON)
            {
                CHECK(s_events[i].setup.span.pcm_offset == SPU_LONG_RING_RAM + 7u * SPU_LONG_RING_STRIDE);
                CHECK(s_events[i].setup.span.frames == SPU_LONG_RING_FRAMES);
                CHECK(s_events[i].setup.span.loops == 1);
            }
    }
    CHECK(spu_long_voice_active(7) == 1);
    s_pos[7] = 1000;
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_UPLOAD, -1) >= 1);
}

static void test_streamed_one_shot_ends_silent(void)
{
    short e = -1;
    int guard = 0;

    upload_long(0x10000, 0x01);
    s_pos[7] = 0;
    key_on(7);
    spu_voice_flush();
    s_levels[7] = 1234;
    while (!spu_long_voice_finished(7) && guard++ < 2000)
    {
        s_pos[7] = (s_pos[7] + 184) % SPU_LONG_RING_FRAMES;
        spu_voice_flush();
    }
    CHECK(spu_long_voice_finished(7) == 1);
    SpuGetVoiceEnvelope(7, &e);
    CHECK(e == 0);
}

static void test_reverb_key_on(void)
{
    int i;

    SPU_REVERB_LOW = (u16)(1u << 8);
    set_voice(8, 0x1010, 0x3FFF, 0x3FFF);
    key_on(8);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_KEY_ON, 8) == 1);
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_KEY_ON && s_events[i].voice == 8)
        {
            CHECK(s_events[i].setup.isel == 2);
            CHECK(s_events[i].setup.imxl == 15);
        }
}

static void test_reverb_clear_bit(void)
{
    SPU_REVERB_LOW = (u16)(SPU_REVERB_LOW & ~(1u << 8));
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_SET_SEND, 8) == 1);
    CHECK(s_events[0].a == 2 && s_events[0].b == 0);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_SET_SEND, -1) == 0);
}

static void test_reverb_pan_change(void)
{
    int i;

    SPU_REVERB_LOW = (u16)(SPU_REVERB_LOW | (1u << 9));
    set_voice(9, 0x1010, 0x3FFF, 0x3FFF);
    key_on(9);
    spu_voice_flush();
    clear_events();
    SPU_VOICE_REG(9, volume_left) = 0x3FFF;
    SPU_VOICE_REG(9, volume_right) = 0x0400;
    spu_voice_flush();
    CHECK(count_ops(OP_SET_SEND, 9) == 1);
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_SET_SEND && s_events[i].voice == 9)
            CHECK(s_events[i].a == 0 && s_events[i].b == 15);
}

static void test_reverb_mode_type(void)
{
    long mode;

    clear_events();
    CHECK(SpuSetReverbModeType(4 | SPU_REV_MODE_CLEAR_WA) == 0);
    CHECK(count_ops(OP_REVERB_CLEAR, -1) == 1);
    SpuGetReverbModeType(&mode);
    CHECK(mode == 4);
    clear_events();
    if (mode != 4)
        SpuSetReverbModeType(4 | SPU_REV_MODE_CLEAR_WA);
    CHECK(count_ops(OP_REVERB_CLEAR, -1) == 0);
}

static void test_reverb_mode_other_logs_once(void)
{
    clear_events();
    SpuSetReverbModeType(9);
    SpuSetReverbModeType(9);
    CHECK(count_logs("spu: reverb mode") == 1);
}

static void test_reverb_depth(void)
{
    clear_events();
    SpuSetReverbModeDepth((s16)0x3000, (s16)0x2000);
    CHECK(count_ops(OP_REVERB_DEPTH, -1) == 1);
    CHECK(s_events[0].a == 0x3000 && s_events[0].b == 0x2000);
}

static void test_upload_limit(void)
{
    unsigned char blocks[64 * 16];
    unsigned b;
    int i;

    for (b = 0; b < 64; b++)
        one_block(blocks + b * 16, 0x00);
    clear_events();
    SpuSetTransferStartAddr(0x78F00);
    SpuWrite(blocks, sizeof(blocks));
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_UPLOAD)
            CHECK(s_events[i].a + s_events[i].b <= SPU_PCM_LIMIT);
    CHECK(count_logs("spu: upload past 0x1A8000 dropped") == 1);
}

static void test_reset_clears_send_cache(void)
{
    int i;

    set_voice(12, 0x1010, 0x3FFF, 0x3FFF);
    SPU_REVERB_LOW = (u16)(SPU_REVERB_LOW | (1u << 12));
    key_on(12);
    spu_voice_flush();

    spu_shadow_reset();
    spu_sample_map_reset();
    spu_voice_flush_reset();

    set_voice(12, 0x1010, 0x3FFF, 0x3FFF);
    SPU_REVERB_LOW = (u16)(1u << 12);
    clear_events();
    spu_voice_flush();
    CHECK(count_ops(OP_SET_SEND, 12) == 1);
    for (i = 0; i < s_event_count; i++)
        if (s_events[i].op == OP_SET_SEND && s_events[i].voice == 12)
            CHECK(s_events[i].a == 2 && s_events[i].b == 15);
}

static void test_no_backend(void)
{
    short e = -1;

    spu_shim_set_backend(NULL);
    key_on(3);
    clear_events();
    spu_voice_flush();
    CHECK(s_event_count == 0);
    SpuGetVoiceEnvelope(5, &e);
    CHECK(e == 0);
}

int main(void)
{
    SpuInit();
    spu_shim_set_backend(&k_backend);
    test_nothing_written();
    test_upload();
    test_key_on();
    test_pitch_only();
    test_off_then_on();
    test_unwritten_start();
    test_noise_key_on();
    test_noise_toggle_off_restarts_sample();
    test_noise_clock_repitches_noise_voices_only();
    test_noise_voice_pitch_write_ignored();
    test_noise_toggle_key_off_same_tick();
    test_noise_toggle_key_on_same_tick();
    test_noise_toggle_after_release_no_rekey();
    test_noise_toggle_after_natural_end_no_rekey();
    test_pitch_modulation();
    test_pitch_modulation_key_on_refreshes();
    test_pitch_modulation_noise_toggle_keeps_noise_pitch();
    test_envelope_readback();
    test_cd_volume();
    test_sweep_volume();
    test_streamed_key_on();
    test_streamed_one_shot_ends_silent();
    test_reverb_key_on();
    test_reverb_clear_bit();
    test_reverb_pan_change();
    test_reverb_mode_type();
    test_reverb_mode_other_logs_once();
    test_reverb_depth();
    test_upload_limit();
    test_reset_clears_send_cache();
    test_no_backend();
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
