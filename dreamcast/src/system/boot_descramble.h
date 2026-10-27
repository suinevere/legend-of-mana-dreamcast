/*----------------------
 | boot_descramble.h
 | Description: Undoes the slice scrambling a CD-R's boot binary carries, as the BIOS does when it loads one.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_BOOT_DESCRAMBLE_H
#define LOM_BOOT_DESCRAMBLE_H

#include <stdint.h>

/*----------------------
 | BOOT_DESCRAMBLE_MAX_BYTES
 | Description: The largest binary boot_descramble takes; its biggest chunk is then 256 KB, 8192 slices.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define BOOT_DESCRAMBLE_MAX_BYTES 0x7FFFFu

int boot_descramble(const uint8_t *src, uint8_t *dst, uint32_t size);

#endif
