/*----------------------
 | memory.h
 | Description: Shim for decomp files that include <memory.h>, which lom_prefix.h's SDK_MEMORY_H guard hides; declares memcpy and memset without prototypes so files that prototype them their own way still compile.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PSYQ_MEMORY_H
#define LOM_PSYQ_MEMORY_H

extern void *memcpy();
extern void *memset();

#endif
