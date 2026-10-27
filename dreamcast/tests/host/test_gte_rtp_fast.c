/*----------------------
 | test_gte_rtp_fast.c
 | Description: Fuzzes gte_rtp_fast against the generic gte_rtp over random register files, comparing every register and FLAG bit after each RTP step.
 | Author: suinevere
 | Dependencies: port/psyq/gte_soft.c (included for its static functions)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "gte_soft.c"

#include <stdio.h>

static unsigned s_rng = 0x12345678u;

static unsigned next(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

static int16_t pick16(void)
{
    switch (next() % 4)
    {
    case 0:
        return (int16_t)(next() & 0xFFFF);
    case 1:
        return (int16_t)((int)(next() % 0x2001) - 0x1000);
    case 2:
        return (int16_t)((next() & 1) ? 0x7FFF : -0x8000);
    default:
        return (int16_t)((int)(next() % 512) - 256);
    }
}

static int32_t pick_tr(void)
{
    switch (next() % 4)
    {
    case 0:
        return (int32_t)((int)(next() % 0x7FFFFFFF) - 0x3FFFFFFF);
    case 1:
        return (int32_t)((int)(next() % 0x20000) - 0x10000);
    case 2:
        return (next() & 1) ? 0x3FFFFFFF : -0x3FFFFFFF;
    default:
        return (int32_t)((int)(next() % 0x2000) - 0x1000);
    }
}

static void randomise(void)
{
    int i;
    int j;

    memset(&lom_gte, 0, sizeof(lom_gte));
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            lom_gte.v[i][j] = pick16();
            lom_gte.rt[i][j] = pick16();
        }
        lom_gte.tr[i] = pick_tr();
        lom_gte.sxy[i][0] = pick16();
        lom_gte.sxy[i][1] = pick16();
    }
    for (i = 0; i < 4; i++)
    {
        lom_gte.sz[i] = (uint16_t)next();
        lom_gte.ir[i] = pick16();
        lom_gte.mac[i] = (int32_t)next();
    }
    lom_gte.h = (uint16_t)((next() & 1) ? next() : 0x100 + next() % 0x400);
    lom_gte.ofx = (int32_t)next() >> (next() % 16);
    lom_gte.ofy = (int32_t)next() >> (next() % 16);
    lom_gte.dqa = pick16();
    lom_gte.dqb = (int32_t)next() >> (next() % 16);
    lom_gte.flag = next();
}

int main(void)
{
    unsigned trial;
    unsigned mismatches = 0;

    for (trial = 0; trial < 2000000u; trial++)
    {
        GteRegisters start;
        GteRegisters generic;
        int slot;
        int cue;

        randomise();
        if (!gte_tr_unchecked())
        {
            continue;
        }
        slot = (int)(next() % 3);
        cue = (int)(next() & 1);
        start = lom_gte;
        gte_rtp(slot, 1, 0, cue);
        generic = lom_gte;
        lom_gte = start;
        gte_rtp_fast(slot, cue);
        if (memcmp(&generic, &lom_gte, sizeof(lom_gte)) != 0)
        {
            if (mismatches < 5)
            {
                printf("FAIL trial %u slot %d cue %d flag %08X vs %08X\n", trial, slot, cue, (unsigned)generic.flag, (unsigned)lom_gte.flag);
            }
            mismatches++;
        }
    }
    if (mismatches == 0)
    {
        printf("ok\n");
    }
    return mismatches != 0;
}
