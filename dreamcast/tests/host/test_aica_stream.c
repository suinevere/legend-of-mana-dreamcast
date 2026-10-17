/*----------------------
 | test_aica_stream.c
 | Description: Host tests for aica_stream_ring.c: free space between the write and play positions, and splitting a push around the ring's wrap.
 | Author: suinevere
 | Dependencies: aica_stream_ring.c, aica_stream.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>

#include "aica_stream.h"

static int failures;

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static void test_free(void)
{
    CHECK(aica_ring_free(0, 0, 16384, 256) == 16384 - 256);
    CHECK(aica_ring_free(100, 100, 16384, 256) == 16384 - 256);
    CHECK(aica_ring_free(99, 100, 16384, 256) == 0);
    CHECK(aica_ring_free(100, 400, 16384, 256) == 44);
    CHECK(aica_ring_free(16000, 300, 16384, 256) == 428);
    CHECK(aica_ring_free(200, 300, 16384, 256) == 0);
}

static void test_split(void)
{
    unsigned first;
    unsigned second;

    second = aica_ring_split(16000, 1000, 16384, &first);
    CHECK(first == 384 && second == 616);
    second = aica_ring_split(10, 1000, 16384, &first);
    CHECK(first == 1000 && second == 0);
    second = aica_ring_split(16384 - 5, 5, 16384, &first);
    CHECK(first == 5 && second == 0);
}

int main(void)
{
    test_free();
    test_split();
    if (failures == 0)
        printf("OK\n");
    return failures != 0;
}
