/*----------------------
 | test_save_pack.c
 | Description: Host test for save_pack.c: round trips of empty, zero, random, repetitive and save-shaped data, the size of a mostly empty save, the output bound, and refusal of truncated and out-of-range streams.
 | Author: suinevere
 | Dependencies: save_pack.c
 ----------------------*/
#include <string.h>
#include "save_pack.h"

int printf(const char *format, ...);
int puts(const char *text);

#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); return 1; } } while (0)

static unsigned char s_in[0x1E000];
static unsigned char s_packed[0x1E000 + 0x1E000 / 8 + 8];
static unsigned char s_out[0x1E000];
static unsigned int s_seed = 12345;

static unsigned int next_rand(void)
{
    s_seed = s_seed * 1103515245u + 12345u;
    return s_seed >> 16;
}

static int round_trip(unsigned int size)
{
    int packed = save_pack(s_in, size, s_packed, save_pack_bound(size));

    if (packed < 0 || (unsigned int)packed > save_pack_bound(size))
    {
        return -1;
    }
    memset(s_out, 0xEE, sizeof s_out);
    if (save_unpack(s_packed, (unsigned int)packed, s_out, size) != 0 || memcmp(s_in, s_out, size) != 0)
    {
        return -1;
    }
    return packed;
}

int main(void)
{
    unsigned int i;
    int packed;

    CHECK(round_trip(0) == 0);

    memset(s_in, 0, 0x4000);
    packed = round_trip(0x4000);
    CHECK(packed > 0 && packed < 128);

    for (i = 0; i < sizeof s_in; i++)
    {
        s_in[i] = (unsigned char)next_rand();
    }
    packed = round_trip(sizeof s_in);
    CHECK(packed > 0 && (unsigned int)packed <= save_pack_bound(sizeof s_in));
    CHECK(round_trip(1) == 2);
    CHECK(round_trip(7) == 8);

    for (i = 0; i < 0x4000; i++)
    {
        s_in[i] = (unsigned char)("LEGEND OF MANA  "[i % 16] + (i / 997));
    }
    CHECK(round_trip(0x4000) > 0);

    memset(s_in, 0, 0x4000);
    memcpy(s_in, "SC\x11\x01", 4);
    for (i = 0x200; i < 0x400; i++)
    {
        s_in[i] = (unsigned char)next_rand();
    }
    for (i = 0x600; i < 0x3400; i += 7)
    {
        s_in[i] = (unsigned char)(next_rand() & 3);
    }
    packed = round_trip(0x4000);
    CHECK(packed > 0 && packed < 0x4000 / 4);
    CHECK(save_unpack(s_packed, (unsigned int)packed - 1, s_out, 0x4000) == -1);
    CHECK(save_pack(s_in, 0x4000, s_packed, 16) == -1);

    {
        static const unsigned char before_start[] = {0x00, 0x00, 0x00};
        static const unsigned char too_long[] = {0x01, 'A', 0x00, 0x0F, 0xFF};
        static const unsigned char no_flags[] = {0};

        CHECK(save_unpack(before_start, sizeof before_start, s_out, 4) == -1);
        CHECK(save_unpack(too_long, sizeof too_long, s_out, 20) == -1);
        CHECK(save_unpack(no_flags, 0, s_out, 1) == -1);
    }

    puts("OK");
    return 0;
}
