/*----------------------
 | palette_bank.h
 | Description: LRU banks of PowerVR palette RAM holding converted PS1 CLUTs, refilled when the CLUT's VRAM generation moves and pinned per frame.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PALETTE_BANK_H
#define LOM_PALETTE_BANK_H

#include <stdint.h>

/*----------------------
 | PAL_OPAQUE, PAL_HALF, PAL_STP_ONLY, PAL_NONSTP_ONLY, PAL_COVER
 | Description: How a CLUT colour's bit 15 becomes alpha: always opaque, half where set, keep only set or only clear texels, or opaque white where set.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { PAL_OPAQUE = 0, PAL_HALF = 1, PAL_STP_ONLY = 2, PAL_NONSTP_ONLY = 3, PAL_COVER = 4 };

/*----------------------
 | STP_NONE, STP_ALL, STP_MIXED
 | Description: Whether a CLUT's non-zero entries have bit 15 clear, all set, or both.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
enum { STP_NONE = 0, STP_ALL = 1, STP_MIXED = 2 };

/*----------------------
 | PaletteOps
 | Description: Backend hook that writes one ARGB8888 entry of PowerVR palette RAM; host tests fake it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct { void (*set_entry)(unsigned index, uint32_t argb8888); } PaletteOps;

void palette_bank_init(const PaletteOps *ops);
int palette_bank_acquire(int clut_x, int clut_y, int depth, int variant, unsigned frame);
int palette_clut_stp(int clut_x, int clut_y, int depth);
uint32_t palette_convert(uint16_t ps1, int variant);
unsigned palette_bank_in_use(void);

#endif
