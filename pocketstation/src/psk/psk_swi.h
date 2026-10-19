/*----------------------
 | psk_swi.h
 | Description: The kernel-call interface decompiled PocketStation C uses in place of an SWI instruction.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef PSK_SWI_H
#define PSK_SWI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | psk_swi
 | Description: Performs kernel call number with r0-r2; every other register is left unchanged, as psx-spx specifies.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: number -- SWI number; r0, r1, r2 -- arguments
 | Returns: the kernel's r0
 ----------------------*/
uint32_t psk_swi(uint32_t number, uint32_t r0, uint32_t r1, uint32_t r2);

#ifdef __cplusplus
}
#endif

#endif
