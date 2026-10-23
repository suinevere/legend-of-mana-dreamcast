/*----------------------
 | test_libc_rand.c
 | Description: Host tests for the Psy-Q rand and srand: the PS1's exact sequence from its zero seed and from srand, and every value inside 0..RAND_MAX.
 | Author: suinevere
 | Dependencies: port/psyq/libc_rand.c, sdk/rand.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

int rand(void);
void srand(unsigned int seed);

static int s_fail;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); s_fail = 1; } } while (0)

static void test_sequence_from_zero_seed(void)
{
    CHECK(rand() == 0x0000);
    CHECK(rand() == 0x53DC);
    CHECK(rand() == 0x2704);
    CHECK(rand() == 0x5665);
    CHECK(rand() == 0x0DAA);
}

static void test_srand_restarts_the_sequence(void)
{
    srand(12345);
    CHECK(rand() == 0x53DC);
    CHECK(rand() == 0x2704);
    CHECK(rand() == 0x5665);
}

static void test_values_stay_within_rand_max(void)
{
    int i;
    int max = 0;

    srand(7);
    for (i = 0; i < 100000; i++)
    {
        int v = rand();

        CHECK(v >= 0 && v <= 32767);
        max = v > max ? v : max;
    }
    CHECK(max > 32000);
}

int main(void)
{
    test_sequence_from_zero_seed();
    test_srand_restarts_the_sequence();
    test_values_stay_within_rand_max();
    if (!s_fail)
    {
        printf("ok\n");
    }
    return s_fail;
}
