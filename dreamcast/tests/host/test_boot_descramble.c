/*----------------------
 | test_boot_descramble.c
 | Description: Host checks for boot_descramble against known answers from the descrambler that reproduced lom.cdi's 1ST_READ.BIN as lom.elf's binary, and for its size limit.
 | Author: suinevere
 | Dependencies: boot_descramble.h
 ----------------------*/
#include <stdint.h>
#include <stdio.h>

#include "boot_descramble.h"

static int s_failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); s_failures++; } } while (0)

static uint8_t s_src[70000];
static uint8_t s_dst[70000];

static uint32_t crc32(const uint8_t *p, uint32_t n)
{
    uint32_t c = 0xFFFFFFFFu;
    uint32_t i;
    int k;

    for (i = 0; i < n; i++)
    {
        c ^= p[i];
        for (k = 0; k < 8; k++)
        {
            c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
        }
    }
    return ~c;
}

static void test_known_answer(uint32_t size, unsigned mul, unsigned add, uint32_t expect)
{
    uint32_t i;

    for (i = 0; i < size; i++)
    {
        s_src[i] = (uint8_t)(i * mul + add);
    }
    CHECK(boot_descramble(s_src, s_dst, size) == 0);
    CHECK(crc32(s_dst, size) == expect);
}

static void test_refuses_oversized_binaries(void)
{
    CHECK(boot_descramble(s_src, s_dst, BOOT_DESCRAMBLE_MAX_BYTES + 1u) != 0);
}

int main(void)
{
    test_known_answer(1000, 7, 3, 0x329DF104u);
    test_known_answer(70000, 13, 5, 0x53F05342u);
    test_refuses_oversized_binaries();
    if (s_failures == 0)
    {
        printf("OK\n");
    }
    return s_failures != 0;
}
