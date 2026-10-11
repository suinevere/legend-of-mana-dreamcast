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
