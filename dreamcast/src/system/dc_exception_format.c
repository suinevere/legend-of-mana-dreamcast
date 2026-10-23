/*----------------------
 | dc_exception_format.c
 | Description: Formats the halt line for an SH-4 exception without KallistiOS dependencies so the host can test it.
 | Author: suinevere
 | Dependencies: dc_exception_format.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include "dc_exception_format.h"

/*----------------------
 | dc_exception_format_halt
 | Description: Writes "<name> PC=.. PR=.. ADDR=.." into out, truncated to fit.
 | Author: suinevere
 | Dependencies: snprintf
 | Globals: N/A
 | Params: out -- buffer, out_size -- its size, name -- exception name, pc/pr -- registers, address -- faulting address (TEA)
 | Returns: out
 ----------------------*/
char *dc_exception_format_halt(char *out, int out_size, const char *name, unsigned long pc, unsigned long pr, unsigned long address)
{
    snprintf(out, (size_t)out_size, "%s PC=%08lX PR=%08lX ADDR=%08lX", name, pc, pr, address);
    return out;
}

char *dc_exception_format_registers(char *out, int out_size, const unsigned long r[16])
{
    int used = 0;
    int i;

    out[0] = 0;
    for (i = 0; i < 16 && used < out_size - 1; i++)
    {
        int n = snprintf(out + used, (size_t)(out_size - used), "%sR%d %08lX", i == 0 ? "" : (i % 3 == 0 ? "\n" : " "), i, r[i]);

        if (n < 0)
            break;
        used += n;
    }
    return out;
}
