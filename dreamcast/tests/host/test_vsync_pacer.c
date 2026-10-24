/*----------------------
 | test_vsync_pacer.c
 | Description: Host test for the shared VSync pacer: VSync(0) always waits for the next vblank, and vsync_wait_after returns at once when a vblank has passed since the last wait but never runs faster than one return per vblank.
 | Author: suinevere
 | Dependencies: vsync_pacer.h, stdio.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include "vsync_pacer.h"

static unsigned s_clock;
static unsigned s_reads;
static unsigned s_tick_every;
static int s_failures;

static unsigned fake_clock(void)
{
    s_reads++;
    if (s_tick_every != 0 && s_reads % s_tick_every == 0)
    {
        s_clock++;
    }
    return s_clock;
}

#define CHECK(cond)                                                                                                                   \
    do                                                                                                                                \
    {                                                                                                                                 \
        if (!(cond))                                                                                                                  \
        {                                                                                                                             \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);                                                                     \
            s_failures++;                                                                                                             \
        }                                                                                                                             \
    } while (0)

static void test_mode0_waits_for_the_next_vblank(void)
{
    s_clock = 100;
    s_reads = 0;
    s_tick_every = 3;
    vsync_wait(0, fake_clock);
    s_clock += 5;
    vsync_wait(0, fake_clock);
    CHECK(s_clock == 107);
}

static void test_after_returns_at_once_when_a_vblank_passed(void)
{
    s_clock = 200;
    s_reads = 0;
    s_tick_every = 3;
    vsync_wait(0, fake_clock);
    s_tick_every = 0;
    s_clock += 4;
    s_reads = 0;
    vsync_wait_after(fake_clock);
    CHECK(s_clock == 205);
    CHECK(s_reads == 1);
}

static void test_after_never_returns_twice_in_one_vblank(void)
{
    s_clock = 300;
    s_reads = 0;
    s_tick_every = 3;
    vsync_wait_after(fake_clock);
    vsync_wait_after(fake_clock);
    vsync_wait_after(fake_clock);
    CHECK(s_clock >= 302);
}

int main(void)
{
    test_mode0_waits_for_the_next_vblank();
    test_after_returns_at_once_when_a_vblank_passed();
    test_after_never_returns_twice_in_one_vblank();
    if (s_failures != 0)
    {
        return 1;
    }
    printf("OK\n");
    return 0;
}
