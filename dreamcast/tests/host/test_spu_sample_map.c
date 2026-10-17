/*----------------------
 | test_spu_sample_map.c
 | Description: Host tests for spu_sample_map.c: PS1 ADPCM decode into the AICA image, sample bounds, loops by flag and register, streamed long samples and chunked uploads.
 | Author: suinevere
 | Dependencies: spu_sample_map.c, spu_sample_map.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "spu_sample_map.h"

static int failures;
static short s_aica[SPU_IMAGE_BYTES / 2];
static short s_other[SPU_IMAGE_BYTES / 2];
static unsigned char s_data[2400 * 16];

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static void sink(unsigned aica_offset, const short *pcm, unsigned bytes, void *ctx)
{
    short *image = ctx ? (short *)ctx : s_aica;

    memcpy((unsigned char *)image + aica_offset, pcm, bytes);
}

static void block(unsigned char *out, unsigned char shift_filter, unsigned char flags, int nibble)
{
    int i;

    out[0] = shift_filter;
    out[1] = flags;
    for (i = 2; i < 16; i++)
        out[i] = (unsigned char)((nibble & 0xF) | ((nibble & 0xF) << 4));
}

static void write_at(unsigned spu_addr, const unsigned char *data, unsigned size)
{
    spu_sample_map_write(spu_addr, data, size, sink, NULL);
}

static void test_one_block(void)
{
    SpuSampleSpan span;
    unsigned char b[16];
    int i;

    spu_sample_map_reset();
    block(b, 0x0C, 0x01, 1);
    write_at(0x1010, b, 16);
    CHECK(SPU_PCM_OFFSET(0x1010) == 0x3838);
    for (i = 0; i < 28; i++)
        CHECK(s_aica[0x3838 / 2 + i] == 1);
    CHECK(spu_sample_map_locate(0x1010 / 8, 0, &span) == 0);
    CHECK(span.pcm_offset == 0x3838);
    CHECK(span.frames == 28);
    CHECK(span.loops == 0);
    CHECK(span.streamed == 0);
    CHECK(spu_sample_map_ram()[0x1010] == 0x0C);
    CHECK(spu_sample_map_ram()[0x1011] == 0x01);
}

static void test_filter_history(void)
{
    unsigned char b[32];
    int old = 0;
    int older = 0;
    int i;

    spu_sample_map_reset();
    block(b, 0x1C, 0x00, 1);
    block(b + 16, 0x1C, 0x01, 1);
    write_at(0x2000, b, 32);
    for (i = 0; i < 56; i++)
    {
        int value = ((1 << 12) >> 12) + ((old * 60 + older * 0 + 32) >> 6);

        if (value > 32767)
            value = 32767;
        CHECK(s_aica[SPU_PCM_OFFSET(0x2000) / 2 + i] == value);
        older = old;
        old = value;
    }
}

static void test_shift_above_12(void)
{
    unsigned char b[16];

    spu_sample_map_reset();
    block(b, 0x0F, 0x01, 1);
    write_at(0x2000, b, 16);
    CHECK(s_aica[SPU_PCM_OFFSET(0x2000) / 2] == 8);
}

static void test_loop_by_flag(void)
{
    SpuSampleSpan span;
    unsigned char b[48];

    spu_sample_map_reset();
    block(b, 0x0C, 0x04, 1);
    block(b + 16, 0x0C, 0x00, 1);
    block(b + 32, 0x0C, 0x03, 1);
    write_at(0x3000, b, 48);
    CHECK(spu_sample_map_locate(0x3000 / 8, 0, &span) == 0);
    CHECK(span.loops == 1);
    CHECK(span.loop_start == 0);
    CHECK(span.frames == 84);

    spu_sample_map_reset();
    block(b, 0x0C, 0x00, 1);
    block(b + 16, 0x0C, 0x04, 1);
    write_at(0x3000, b, 48);
    CHECK(spu_sample_map_locate(0x3000 / 8, 0, &span) == 0);
    CHECK(span.loop_start == 28);
    CHECK(span.loop_block == 0x3000 / 16 + 1);
}

static void test_loop_by_register(void)
{
    SpuSampleSpan span;
    unsigned char b[48];

    spu_sample_map_reset();
    block(b, 0x0C, 0x00, 1);
    block(b + 16, 0x0C, 0x00, 1);
    block(b + 32, 0x0C, 0x03, 1);
    write_at(0x3000, b, 48);
    CHECK(spu_sample_map_locate(0x3000 / 8, 0x3010 / 8, &span) == 0);
    CHECK(span.loop_start == 28);
    CHECK(spu_sample_map_locate(0x3000 / 8, 0x5000 / 8, &span) == 0);
    CHECK(span.loop_start == 0);
}

static void test_history_resets_per_sample(void)
{
    unsigned char b[32];

    spu_sample_map_reset();
    block(b, 0x0C, 0x01, 5);
    block(b + 16, 0x1C, 0x01, 1);
    write_at(0x4000, b, 32);
    CHECK(s_aica[SPU_PCM_OFFSET(0x4010) / 2] == 1);
}

static void test_long_sample(void)
{
    SpuSampleSpan span;
    unsigned i;

    spu_sample_map_reset();
    for (i = 0; i < 2400; i++)
        block(s_data + 16 * i, 0x0C, i == 2000 ? 0x04 : (i == 2399 ? 0x03 : 0x00), 1);
    write_at(0x10000, s_data, 2400 * 16);
    CHECK(spu_sample_map_locate(0x10000 / 8, 0, &span) == 0);
    CHECK(span.frames == 67200);
    CHECK(span.streamed == 1);
    CHECK(span.loop_start == 56000);
    CHECK(span.start_block == 0x10000 / 16);
    CHECK(span.loop_block == 0x10000 / 16 + 2000);
    CHECK(s_aica[SPU_PCM_OFFSET(0x10000) / 2 + 67199] == 1);
}

static void test_streamed_boundary(void)
{
    SpuSampleSpan span;
    unsigned i;

    spu_sample_map_reset();
    for (i = 0; i < 2340; i++)
        block(s_data + 16 * i, 0x0C, i == 2339 ? 0x01 : 0x00, 1);
    write_at(0x10000, s_data, 2340 * 16);
    CHECK(spu_sample_map_locate(0x10000 / 8, 0, &span) == 0);
    CHECK(span.frames == 65520);
    CHECK(span.streamed == 0);

    spu_sample_map_reset();
    for (i = 0; i < 2341; i++)
        block(s_data + 16 * i, 0x0C, i == 2340 ? 0x01 : 0x00, 1);
    write_at(0x10000, s_data, 2341 * 16);
    CHECK(spu_sample_map_locate(0x10000 / 8, 0, &span) == 0);
    CHECK(span.frames == 65548);
    CHECK(span.streamed == 1);
}

static void test_start_mid_sample(void)
{
    SpuSampleSpan span;
    unsigned char b[48];

    spu_sample_map_reset();
    block(b, 0x0C, 0x00, 1);
    block(b + 16, 0x0C, 0x00, 1);
    block(b + 32, 0x0C, 0x01, 1);
    write_at(0x3000, b, 48);
    CHECK(spu_sample_map_locate(0x3010 / 8, 0, &span) == 0);
    CHECK(span.pcm_offset == SPU_PCM_OFFSET(0x3000) + 56);
    CHECK(span.frames == 56);
}

static void test_never_written(void)
{
    SpuSampleSpan span;

    spu_sample_map_reset();
    CHECK(spu_sample_map_locate(0x7000, 0, &span) == -1);
}

static void test_write_past_end(void)
{
    unsigned char b[64];

    spu_sample_map_reset();
    memset(b, 0, sizeof(b));
    CHECK(spu_sample_map_write(0x7FFF0, b, 64, sink, NULL) == 16);
}

static void test_chunked_write(void)
{
    unsigned char b[48];
    SpuSampleSpan span;

    block(b, 0x1C, 0x00, 3);
    block(b + 16, 0x2C, 0x00, -2);
    block(b + 32, 0x1C, 0x01, 7);
    spu_sample_map_reset();
    spu_sample_map_write(0x6000, b, 48, sink, s_other);
    spu_sample_map_reset();
    write_at(0x6000, b, 32);
    write_at(0x6020, b + 32, 16);
    CHECK(memcmp(&s_aica[SPU_PCM_OFFSET(0x6000) / 2], &s_other[SPU_PCM_OFFSET(0x6000) / 2], 84 * 2) == 0);
    CHECK(spu_sample_map_locate(0x6000 / 8, 0, &span) == 0);
    CHECK(span.frames == 84);
}

int main(void)
{
    test_one_block();
    test_filter_history();
    test_shift_above_12();
    test_loop_by_flag();
    test_loop_by_register();
    test_history_resets_per_sample();
    test_long_sample();
    test_streamed_boundary();
    test_start_mid_sample();
    test_never_written();
    test_write_past_end();
    test_chunked_write();
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
