/*----------------------
 | checkps_probe.c
 | Description: A stand-in CHECKPS overlay for the link test: it calls into lom.elf, reads a CHECKPS data symbol and keeps a counter in .bss.
 | Author: suinevere
 | Dependencies: dc_system.h, gen/checkps_symbols.ld
 | Globals: s_calls, g_checkps_state
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "dc_system.h"

extern unsigned char g_checkps_state[];

static int s_calls;

int run_checkps(void)
{
    s_calls++;
    dc_log("checkps probe %d", s_calls);
    return g_checkps_state[0] + s_calls;
}
