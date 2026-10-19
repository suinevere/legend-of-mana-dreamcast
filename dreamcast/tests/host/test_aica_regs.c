/*----------------------
 | test_aica_regs.c
 | Description: Host tests for aica_regs.c: a voice setup packs into the AICA channel register words (including the send and PCMS bit), the EG readback maps to a PS1 envelope level, and aica_reverb.c's COEF table.
 | Author: suinevere
 | Dependencies: aica_regs.c, aica_regs.h, aica_reverb.c, aica_reverb.h, spu_backend.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "aica_regs.h"
#include "aica_reverb.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static void test_looped_words(void)
{
    static const unsigned short expected[AICA_CHANNEL_WORDS] = {0x0200 | 0x12, 0x3456, 28, 1000, 0x1234, 0x3C1F, 0x0800, 0, 0, 0x0F12, 0x0320};
    SpuVoiceSetup setup;
    unsigned short words[AICA_CHANNEL_WORDS];

    memset(&setup, 0, sizeof(setup));
    setup.span.pcm_offset = 0x123456;
    setup.span.frames = 1000;
    setup.span.loop_start = 28;
    setup.span.loops = 1;
    setup.pitch = 0x0800;
    setup.tl = 3;
    setup.dipan = 0x12;
    setup.eg10 = 0x1234;
    setup.eg14 = 0x3C1F;
    aica_channel_words(&setup, words);
    CHECK(memcmp(words, expected, sizeof(words)) == 0);
    setup.span.loops = 0;
    aica_channel_words(&setup, words);
    CHECK(words[0] == 0x12);
}

static void test_send_and_pcm8_words(void)
{
    SpuVoiceSetup setup;
    unsigned short words[AICA_CHANNEL_WORDS];

    memset(&setup, 0, sizeof(setup));
    setup.span.pcm_offset = 0x123456;
    setup.span.frames = 1000;
    setup.span.loop_start = 28;
    setup.span.loops = 1;
    setup.isel = 2;
    setup.imxl = 15;
    aica_channel_words(&setup, words);
    CHECK(words[8] == 0x00F2);
    CHECK(words[0] == (unsigned short)(0x0200 | 0x12));
    setup.pcm8 = 1;
    aica_channel_words(&setup, words);
    CHECK((words[0] & 0x0080) == 0x0080);
}

static void test_reverb_coefs(void)
{
    unsigned short out[8];
    static const unsigned short expected_on[8] = {0xC000, 0xC000, 0xC000, 0xC000, 0x4000, 0x2960, 0x2000, 0x14B0};
    static const unsigned short expected_off[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    aica_reverb_coefs(1, 0x4000, 0x2000, out);
    CHECK(memcmp(out, expected_on, sizeof(out)) == 0);
    aica_reverb_coefs(0, 0x4000, 0x2000, out);
    CHECK(memcmp(out, expected_off, sizeof(out)) == 0);
}

static void test_envelope_level(void)
{
    CHECK(aica_envelope_level(0x1FFF, 1) == 0);
    CHECK(aica_envelope_level(0x3C0, 1) == 0);
    CHECK(aica_envelope_level(0, 1) == 0x7FFF);
    CHECK(aica_envelope_level(0x3BF, 1) == 1);
    CHECK(aica_envelope_level(0xE000 | 0x100, 1) == aica_envelope_level(0x100, 1));
    CHECK(aica_envelope_level(0, 0) == 0);
}

int main(void)
{
    test_looped_words();
    test_send_and_pcm8_words();
    test_envelope_level();
    test_reverb_coefs();
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
