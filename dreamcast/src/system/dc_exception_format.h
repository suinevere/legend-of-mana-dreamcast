/*----------------------
 | dc_exception_format.h
 | Description: Declares the halt-text formatter for SH-4 exceptions, kept free of KallistiOS includes so it can be host-tested.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef DC_EXCEPTION_FORMAT_H
#define DC_EXCEPTION_FORMAT_H

/*----------------------
 | dc_exception_format_halt
 | Description: Writes "<name> PC=.. PR=.. ADDR=.." into out, truncated to fit.
 | Author: suinevere
 | Dependencies: snprintf
 | Globals: N/A
 | Params: out -- buffer, out_size -- its size, name -- exception name, pc/pr -- registers, address -- faulting address (TEA)
 | Returns: out
 ----------------------*/
char *dc_exception_format_halt(char *out, int out_size, const char *name, unsigned long pc, unsigned long pr, unsigned long address);

#endif
