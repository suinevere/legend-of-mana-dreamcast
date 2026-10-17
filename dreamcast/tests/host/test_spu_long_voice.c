/*----------------------
 | test_spu_long_voice.c
 | Description: Host tests for spu_long_voice.c: a ring-streamed voice sounds exactly the PS1's block-by-block decode across ring wraps, loop jumps and late ticks, and ends like the PS1.
 | Author: suinevere
 | Dependencies: spu_long_voice.c, spu_sample_map.c, spu_backend.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "spu_backend.h"
#include "spu_long_voice.h"
#include "spu_sample_map.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

#define MAX_OUT 320000u

static short s_image[0x200000 / 2];
static unsigned s_pos[SPU_VOICE_COUNT];
static int s_key_on[SPU_VOICE_COUNT];
static int s_key_off[SPU_VOICE_COUNT];
static SpuVoiceSetup s_setup[SPU_VOICE_COUNT];
static short s_out[2][MAX_OUT];
static unsigned s_out_len[2];
static short s_ref[MAX_OUT];
static unsigned char s_adpcm[3000 * 16];

static void fake_upload(unsigned aica_offset, const short *pcm, unsigned bytes)
{
    memcpy((unsigned char *)s_image + aica_offset, pcm, bytes);
}

static void fake_key_on(int voice, const SpuVoiceSetup *setup)
{
    s_key_on[voice]++;
    s_setup[voice] = *setup;
}

static void fake_key_off(int voice)
{
    s_key_off[voice]++;
}

static unsigned fake_position(int voice)
{
    return s_pos[voice];
}

static const SpuBackend k_backend = {fake_upload, fake_key_on, fake_key_off, NULL, NULL, NULL, NULL, fake_position, NULL, NULL};

static void sink(unsigned aica_offset, const short *pcm, unsigned bytes, void *ctx)
{
    (void)aica_offset;
    (void)pcm;
    (void)bytes;
    (void)ctx;
}

static void build_sample(unsigned spu_addr, unsigned blocks, unsigned loop_flag_block, unsigned char end_flags)
{
    unsigned b;
    int i;

    for (b = 0; b < blocks; b++)
    {
        unsigned char *p = s_adpcm + b * 16;

        p[0] = 0x1A;
        p[1] = (unsigned char)((b == loop_flag_block ? 0x04 : 0x00) | (b == blocks - 1 ? end_flags : 0x00));
        for (i = 2; i < 16; i++)
        {
            int lo = (int)((b + (unsigned)(2 * (i - 2))) % 16u);
            int hi = (int)((b + (unsigned)(2 * (i - 2) + 1)) % 16u);

            p[i] = (unsigned char)(lo | (hi << 4));
        }
    }
    spu_sample_map_reset();
    spu_sample_map_write(spu_addr, s_adpcm, blocks * 16, sink, NULL);
}

static unsigned reference(unsigned start_block, unsigned repeat_block, unsigned frames)
{
    const unsigned char *ram = spu_sample_map_ram();
    unsigned block = start_block;
    unsigned made = 0;
    int old = 0;
    int older = 0;
    short buf[SPU_BLOCK_FRAMES];

    while (made < frames)
    {
        unsigned char flags = ram[block * 16 + 1];
        unsigned i;

        if (flags & 0x04)
            repeat_block = block;
        spu_adpcm_decode_block(ram + block * 16, &old, &older, buf);
        for (i = 0; i < SPU_BLOCK_FRAMES && made < frames; i++)
            s_ref[made++] = buf[i];
        if (flags & 0x01)
        {
            if (!(flags & 0x02))
                return made;
            block = repeat_block;
        }
        else
        {
            block++;
        }
    }
    return made;
}

static void play(int slot, int voice, unsigned n)
{
    unsigned ring = (SPU_LONG_RING_RAM + (unsigned)voice * SPU_LONG_RING_STRIDE) / 2u;
    unsigned i;

    for (i = 0; i < n; i++)
    {
        if (s_out_len[slot] < MAX_OUT)
            s_out[slot][s_out_len[slot]++] = s_image[ring + s_pos[voice]];
        s_pos[voice] = (s_pos[voice] + 1u) % SPU_LONG_RING_FRAMES;
    }
}

static void reset_fakes(void)
{
    memset(s_pos, 0, sizeof(s_pos));
    memset(s_key_on, 0, sizeof(s_key_on));
    memset(s_key_off, 0, sizeof(s_key_off));
    memset(s_out_len, 0, sizeof(s_out_len));
}

static void start(int voice, unsigned start_addr8, unsigned repeat_addr8)
{
    SpuVoiceSetup setup;

    memset(&setup, 0, sizeof(setup));
    CHECK(spu_sample_map_locate(start_addr8, repeat_addr8, &setup.span) == 0);
    CHECK(setup.span.streamed == 1);
    spu_long_voice_start(&k_backend, voice, &setup);
}

static int matches(int slot, unsigned count)
{
    return s_out_len[slot] >= count && memcmp(s_out[slot], s_ref, count * sizeof(short)) == 0;
}

static void test_one_shot(void)
{
    unsigned made;
    unsigned guard = 0;
    unsigned i;

    reset_fakes();
    build_sample(0x10000, 3000, 9999, 0x01);
    made = reference(0x10000 / 16, 0x10000 / 16, MAX_OUT);
    CHECK(made == 84000);
    start(5, 0x10000 / 8, 0);
    CHECK(s_key_on[5] == 1);
    CHECK(s_setup[5].span.pcm_offset == SPU_LONG_RING_RAM + 5u * SPU_LONG_RING_STRIDE);
    CHECK(s_setup[5].span.frames == SPU_LONG_RING_FRAMES);
    CHECK(s_setup[5].span.loops == 1 && s_setup[5].span.loop_start == 0 && s_setup[5].span.streamed == 0);
    while (!spu_long_voice_finished(5) && guard++ < 2000)
    {
        play(0, 5, 184);
        spu_long_voice_service(&k_backend);
    }
    CHECK(spu_long_voice_finished(5) == 1);
    CHECK(spu_long_voice_active(5) == 0);
    CHECK(s_key_off[5] == 1);
    CHECK(matches(0, made));
    for (i = made; i < s_out_len[0]; i++)
        CHECK(s_out[0][i] == 0);
}

static void run_loop_case(unsigned repeat_addr8, unsigned loop_flag_block)
{
    unsigned made;

    reset_fakes();
    build_sample(0x10000, 3000, loop_flag_block, 0x03);
    made = reference(0x10000 / 16, repeat_addr8 ? repeat_addr8 / 2 : 0x10000 / 16, 300000);
    CHECK(made == 300000);
    start(9, 0x10000 / 8, repeat_addr8);
    while (s_out_len[0] < 300000)
    {
        play(0, 9, 184);
        spu_long_voice_service(&k_backend);
    }
    CHECK(matches(0, 300000));
    CHECK(spu_long_voice_finished(9) == 0);
    CHECK(spu_long_voice_active(9) == 1);
}

static void test_loop_by_flag(void)
{
    run_loop_case(0, 1000);
}

static void test_loop_by_register(void)
{
    run_loop_case((0x10000 / 16 + 500) * 2, 9999);
}

static void test_fast_voice_never_underruns(void)
{
    unsigned ring = (SPU_LONG_RING_RAM + 2u * SPU_LONG_RING_STRIDE) / 2u;
    unsigned i;

    reset_fakes();
    build_sample(0x10000, 3000, 1000, 0x03);
    reference(0x10000 / 16, 0x10000 / 16, 100000);
    for (i = 0; i < SPU_LONG_RING_FRAMES; i++)
        s_image[ring + i] = 0x7FFF;
    start(2, 0x10000 / 8, 0);
    while (s_out_len[0] < 100000)
    {
        play(0, 2, 735);
        spu_long_voice_service(&k_backend);
    }
    CHECK(matches(0, 100000));
    for (i = 0; i < 100000; i++)
        if (s_out[0][i] == 0x7FFF)
        {
            CHECK(s_out[0][i] != 0x7FFF);
            break;
        }
}

static void test_late_tick(void)
{
    reset_fakes();
    build_sample(0x10000, 3000, 1000, 0x03);
    reference(0x10000 / 16, 0x10000 / 16, 20000);
    start(4, 0x10000 / 8, 0);
    play(0, 4, 3900);
    spu_long_voice_service(&k_backend);
    while (s_out_len[0] < 20000)
    {
        play(0, 4, 184);
        spu_long_voice_service(&k_backend);
    }
    CHECK(matches(0, 20000));
}

static void test_restart(void)
{
    reset_fakes();
    build_sample(0x10000, 3000, 1000, 0x03);
    start(6, 0x10000 / 8, 0);
    play(0, 6, 1000);
    spu_long_voice_service(&k_backend);
    spu_long_voice_stop(6);
    CHECK(spu_long_voice_active(6) == 0 && spu_long_voice_finished(6) == 0);
    build_sample(0x20000, 2500, 9999, 0x01);
    reference(0x20000 / 16, 0x20000 / 16, 10000);
    s_pos[6] = 0;
    s_out_len[1] = 0;
    start(6, 0x20000 / 8, 0);
    while (s_out_len[1] < 10000)
    {
        play(1, 6, 184);
        spu_long_voice_service(&k_backend);
    }
    CHECK(matches(1, 10000));
}

static void test_two_voices(void)
{
    unsigned made;

    reset_fakes();
    build_sample(0x10000, 3000, 1000, 0x03);
    made = reference(0x10000 / 16, 0x10000 / 16, 50000);
    CHECK(made == 50000);
    start(3, 0x10000 / 8, 0);
    play(0, 3, 500);
    start(17, 0x10000 / 8, 0);
    while (s_out_len[1] < 40000)
    {
        play(0, 3, 184);
        play(1, 17, 184);
        spu_long_voice_service(&k_backend);
    }
    CHECK(matches(0, 40500));
    CHECK(matches(1, 40000));
}

int main(void)
{
    test_one_shot();
    test_loop_by_flag();
    test_loop_by_register();
    test_fast_voice_never_underruns();
    test_late_tick();
    test_restart();
    test_two_voices();
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
