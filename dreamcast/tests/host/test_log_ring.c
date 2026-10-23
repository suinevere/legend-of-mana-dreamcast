/*----------------------
 | test_log_ring.c
 | Description: Host test for the queued serial log: bytes leave in order through a FIFO that drains, and a flush against a FIFO that never drains gives up after LOG_RING_STALL_US and drops later lines instead of hanging.
 | Author: suinevere
 | Dependencies: log_ring.h, stdio.h, string.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>
#include "log_ring.h"

static unsigned long long s_now;
static unsigned s_fifo_used;
static int s_fifo_stuck;
static char s_sent[65536];
static unsigned s_sent_len;
static int s_failures;

static unsigned fake_room(void)
{
    if (!s_fifo_stuck)
    {
        s_fifo_used = 0;
    }
    s_now += 10;
    return 16u - s_fifo_used;
}

static void fake_put(uint8_t c)
{
    s_fifo_used++;
    if (s_sent_len < sizeof s_sent)
    {
        s_sent[s_sent_len++] = (char)c;
    }
}

static unsigned long long fake_now(void)
{
    return s_now;
}

static int fake_lock(void)
{
    return 0;
}

static void fake_unlock(int old)
{
    (void)old;
}

static const LogPort s_port = { fake_room, fake_put, fake_now, fake_lock, fake_unlock };

static void check(int ok, const char *what)
{
    if (!ok)
    {
        printf("FAIL: %s\n", what);
        s_failures++;
    }
}

static void test_drains_in_order(void)
{
    int i;
    char line[64];
    char expect[65536];
    unsigned expect_len = 0;

    log_ring_reset();
    log_ring_bind(&s_port);
    s_fifo_stuck = 0;
    s_sent_len = 0;
    for (i = 0; i < 2000; i++)
    {
        int n = snprintf(line, sizeof line, "line %d\n", i);

        memcpy(expect + expect_len, line, (size_t)n);
        expect_len += (unsigned)n;
        log_ring_push(line, (unsigned)n);
    }
    log_ring_flush();
    check(!log_ring_dead(), "a draining port stays live");
    check(s_sent_len == expect_len && memcmp(s_sent, expect, expect_len) == 0, "every byte leaves in order");
}

static void test_stuck_port_gives_up(void)
{
    char line[200];
    unsigned long long start;
    int i;

    log_ring_reset();
    log_ring_bind(&s_port);
    s_fifo_stuck = 1;
    s_fifo_used = 0;
    s_sent_len = 0;
    memset(line, 'x', sizeof line);
    start = s_now;
    for (i = 0; i < 200; i++)
    {
        log_ring_push(line, sizeof line);
    }
    check(log_ring_dead(), "a port that never drains is declared dead once the ring fills");
    check(s_now - start < 4u * LOG_RING_STALL_US, "the full ring waits about one stall period, not forever");
    check(s_sent_len == 16u, "only the FIFO's own bytes were written");
    start = s_now;
    log_ring_push(line, sizeof line);
    log_ring_drain();
    log_ring_flush();
    check(s_now == start, "a dead port costs nothing afterwards");
}

int main(void)
{
    test_drains_in_order();
    test_stuck_port_gives_up();
    if (s_failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
