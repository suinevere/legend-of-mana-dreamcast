/*----------------------
 | test_dc_exception.c
 | Description: Host checks for dc_exception_format_halt's text and truncation.
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
    printf("OK\n");
    return 0;
}
