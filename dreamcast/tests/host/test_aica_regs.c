/*----------------------
 | test_aica_regs.c
 | Description: Host tests for aica_regs.c: a voice setup packs into the AICA channel register words, and the EG readback maps to a PS1 envelope level.
 | Author: suinevere
 | Dependencies: aica_regs.c, aica_regs.h, spu_backend.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "aica_regs.h"

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
    test_envelope_level();
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
