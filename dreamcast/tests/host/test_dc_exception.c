/*----------------------
 | test_dc_exception.c
 | Description: Host checks for dc_exception_format_halt's text and truncation, and dc_exception_format_registers' layout.
 | Author: suinevere
 | Dependencies: dc_exception_format.c
 | Globals: N/A
 | Params: N/A
 | Returns: 0 on success
 ----------------------*/
#include <stdio.h>
#include <string.h>
#include "dc_exception_format.h"

int main(void)
{
    char out[64];
    char tiny[8];
    dc_exception_format_halt(out, sizeof out, "ADDR READ", 0x8C0123A4ul, 0x8C010000ul, 0x00000003ul);
    if (strcmp(out, "ADDR READ PC=8C0123A4 PR=8C010000 ADDR=00000003") != 0) {
        printf("FAIL format: %s\n", out);
        return 1;
    }
    dc_exception_format_halt(tiny, sizeof tiny, "ILLEGAL", 1, 2, 3);
    if (strlen(tiny) != sizeof tiny - 1 || strncmp(tiny, "ILLEGAL", 7) != 0) {
        printf("FAIL truncate: %s\n", tiny);
        return 1;
    }
    {
        unsigned long r[16];
        char regs[256];
        int i;

        for (i = 0; i < 16; i++)
            r[i] = 0x8C000000ul + (unsigned long)i;
        dc_exception_format_registers(regs, sizeof regs, r);
        if (strncmp(regs, "R0 8C000000 R1 8C000001 R2 8C000002\nR3 8C000003", 47) != 0 || strstr(regs, "R15 8C00000F") == NULL) {
            printf("FAIL registers: %s\n", regs);
            return 1;
        }
        dc_exception_format_registers(tiny, sizeof tiny, r);
        if (strlen(tiny) != sizeof tiny - 1) {
            printf("FAIL registers truncate: %s\n", tiny);
            return 1;
        }
    }
    printf("OK\n");
    return 0;
}
