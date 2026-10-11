/*----------------------
 | gpu_tag.h
 | Description: Decodes PS1-style packet tag words whose 24-bit address field holds the low bits of a platform RAM pointer.
 | Author: suinevere
 | Dependencies: stdint.h, lom_platform_memory.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_GPU_TAG_H
#define LOM_GPU_TAG_H

#include <stdint.h>

#include "lom_platform_memory.h"

/*----------------------
 | GPU_TAG_END
 | Description: 24-bit address value that terminates an ordering-table chain.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GPU_TAG_END 0x00FFFFFFu

/*----------------------
 | gpu_tag_addr
 | Description: Returns the raw 24-bit next-packet field of a tag word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: tag -- packet tag word
 | Returns: 24-bit address field
 ----------------------*/
static inline uint32_t gpu_tag_addr(uint32_t tag)
{
    return tag & 0x00FFFFFFu;
}

/*----------------------
 | gpu_tag_len
 | Description: Returns the packet word count stored in a tag word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: tag -- packet tag word
 | Returns: word count
 ----------------------*/
static inline uint32_t gpu_tag_len(uint32_t tag)
{
    return tag >> 24;
}

/*----------------------
 | gpu_tag_pointer
 | Description: Expands a 24-bit tag address to a full pointer value: Dreamcast RAM 0x8Cxxxxxx, or Saturn High RAM 0x060xxxxx / Low RAM 0x002xxxxx.
 | Author: suinevere
 | Dependencies: lom_platform_memory.h
 | Globals: N/A
 | Params: addr -- 24-bit address field, compared against GPU_TAG_END before calling
 | Returns: full pointer value
 ----------------------*/
static inline uint32_t gpu_tag_pointer(uint32_t addr)
{
#if defined(LOM_DREAMCAST)
    return GPU_TAG_RAM_BASE | addr;
#else
    return addr < 0x00200000u ? (GPU_TAG_RAM_BASE | addr) : addr;
#endif
}

#endif
