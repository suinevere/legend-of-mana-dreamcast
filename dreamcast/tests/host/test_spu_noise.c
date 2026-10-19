/*----------------------
 | test_spu_noise.c
 | Description: Host tests for spu_noise.c: the LFSR sequence, the rendered table's distinctness and statistics, the noise pitch formula and the noise voice span.
 | Author: suinevere
 | Dependencies: spu_noise.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "spu_noise.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static void test_sequence(void)
{
    static const unsigned expect[11] = {0x1u, 0x3u, 0x7u, 0xFu, 0x1Fu, 0x3Fu, 0x7Fu, 0xFFu, 0x1FFu, 0x3FFu, 0x7FFu};
    unsigned level = 0u;
    unsigned i;

    for (i = 0; i < 11u; i++)
    {
        level = spu_noise_next(level);
        CHECK(level == expect[i]);
    }
    level = spu_noise_next(level);
    CHECK(level == 0xFFEu);
}

static unsigned char s_seen[8192];

static void test_distinct(void)
{
    unsigned level = 0u;
    unsigned i;
    int duplicate = 0;

    memset(s_seen, 0, sizeof(s_seen));
    for (i = 0; i < SPU_NOISE_FRAMES; i++)
    {
        level = spu_noise_next(level);
        if (s_seen[level / 8u] & (unsigned char)(1u << (level % 8u)))
            duplicate = 1;
        s_seen[level / 8u] |= (unsigned char)(1u << (level % 8u));
    }
    CHECK(!duplicate);
}

static signed char s_table[SPU_NOISE_FRAMES];

static void test_render_stats(void)
{
    double sum = 0.0;
    double sum_sq = 0.0;
    unsigned i;
    double mean;
    double rms;

    spu_noise_render(s_table, SPU_NOISE_FRAMES);
    for (i = 0; i < SPU_NOISE_FRAMES; i++)
    {
        sum += (double)s_table[i];
        sum_sq += (double)s_table[i] * (double)s_table[i];
    }
    mean = sum / (double)SPU_NOISE_FRAMES;
    rms = sqrt(sum_sq / (double)SPU_NOISE_FRAMES);
    CHECK(mean > -2.0 && mean < 2.0);
    CHECK(rms > 70.0 && rms < 78.0);
}

static void test_pitch(void)
{
    CHECK(spu_noise_pitch(0x3Fu) == 0x1000u);
    CHECK(spu_noise_pitch(0x30u) == 0x200u);
    CHECK(spu_noise_pitch(0x20u) == 0x20u);
    CHECK(spu_noise_pitch(0u) == 1u);
}

static void test_span(void)
{
    SpuSampleSpan span;

    spu_noise_span(&span);
    CHECK(span.pcm_offset == SPU_NOISE_RAM);
    CHECK(span.frames == SPU_NOISE_FRAMES);
    CHECK(span.loop_start == 0u);
    CHECK(span.loops == 1);
    CHECK(span.streamed == 0);
}

int main(void)
{
    test_sequence();
    test_distinct();
    test_render_stats();
    test_pitch();
    test_span();
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
