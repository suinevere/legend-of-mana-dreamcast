/*----------------------
 | test_aica_convert.c
 | Description: Host tests for aica_convert.c: pitch to OCT/FNS, volume to TL/DIPAN and ADSR to AICA envelope words.
 | Author: suinevere
 | Dependencies: aica_convert.c, aica_envelope_table.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

#include "aica_convert.h"
#include "aica_envelope_table.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)
#define OCT(word) (((word) >> 11) & 0xF)
#define FNS(word) ((word) & 0x3FF)

static void test_pitch(void)
{
    unsigned short w;
    w = aica_pitch_word(0x1000, 44100);
    CHECK(OCT(w) == 0 && FNS(w) == 0);
    w = aica_pitch_word(0x2000, 44100);
    CHECK(OCT(w) == 1 && FNS(w) == 0);
    w = aica_pitch_word(0x1000, 22050);
    CHECK(OCT(w) == 0xF && FNS(w) == 0);
    w = aica_pitch_word(0x1800, 44100);
    CHECK(OCT(w) == 0 && FNS(w) == 512);
    w = aica_pitch_word(0x0FF0, 44100);
    CHECK(OCT(w) == 0xF && FNS(w) == 1016);
    w = aica_pitch_word(0x3FF0, 44100);
    CHECK(OCT(w) == 1 && FNS(w) == 1022);
    w = aica_pitch_word(0, 44100);
    CHECK(OCT(w) == 8 && FNS(w) == 0);
    CHECK((aica_pitch_word(0x1000, 44100) & 0x8400) == 0);
    w = aica_pitch_word(0x1000, 11025);
    CHECK(OCT(w) == 0xE && FNS(w) == 0);
    w = aica_pitch_word(0x3FFF, 44100);
    CHECK(OCT(w) == 2 && FNS(w) == 0);
}

static void test_level_pan(void)
{
    unsigned char tl, pan;
    int swept;

    aica_level_pan(0x3FFF, 0x3FFF, &tl, &pan, &swept);
    CHECK(tl == 0 && pan == 0 && swept == 0);
    aica_level_pan(0x1FFF, 0x1FFF, &tl, &pan, &swept);
    CHECK(tl == 16 && pan == 0);
    aica_level_pan(0x3FFF, 0x1FFF, &tl, &pan, &swept);
    CHECK(tl == 0 && pan == (0x10 | 2));
    aica_level_pan(0x1FFF, 0x3FFF, &tl, &pan, &swept);
    CHECK(tl == 0 && pan == 2);
    aica_level_pan(0x3FFF, 0, &tl, &pan, &swept);
    CHECK(tl == 0 && pan == 0x1F);
    aica_level_pan(0, 0x3FFF, &tl, &pan, &swept);
    CHECK(tl == 0 && pan == 0x0F);
    aica_level_pan(0x4001, 0x3FFF, &tl, &pan, &swept);
    CHECK(tl == 0 && pan == 0 && swept == 0);
    aica_level_pan(0x4001, 0x2000, &tl, &pan, &swept);
    CHECK(tl == 0 && pan == 0x12);
    aica_level_pan((short)(0x8000 | 0x10), 0x3FFF, &tl, &pan, &swept);
    CHECK(tl == 0 && pan == 0 && swept == 1);
    aica_level_pan(0, 0, &tl, &pan, &swept);
    CHECK(tl == 255);
    aica_level_pan(0x0001, 0x0001, &tl, &pan, &swept);
    CHECK(tl == 224);
}

static void test_envelope(void)
{
    unsigned short w1, w2;
    aica_envelope(0x00FF, 0x0000, &w1, &w2);
    CHECK((w1 & 0x1F) == aica_attack_linear[0]);
    CHECK(((w1 >> 6) & 0x1F) == aica_decrease_exponential[0xF << 2]);
    CHECK(((w1 >> 11) & 0x1F) == 0);
    CHECK((w1 & 0x20) == 0);
    CHECK((w2 & 0x1F) == aica_decrease_linear[0]);
    CHECK(((w2 >> 5) & 0x1F) == 0);
    CHECK(((w2 >> 10) & 0xF) == 0xF);
    CHECK((w2 & 0x4000) == 0);
    aica_envelope(0x8A30, 0xC5A3, &w1, &w2);
    CHECK((w1 & 0x1F) == aica_attack_exponential[0x0A]);
    CHECK(((w1 >> 6) & 0x1F) == aica_decrease_exponential[0x3 << 2]);
    CHECK(((w1 >> 11) & 0x1F) == aica_decrease_exponential[0x16]);
    CHECK((w2 & 0x1F) == aica_decrease_exponential[0x03 << 2]);
    CHECK(((w2 >> 5) & 0x1F) == aica_decay_level[0]);
}

int main(void)
{
    test_pitch();
    test_level_pan();
    test_envelope();
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
