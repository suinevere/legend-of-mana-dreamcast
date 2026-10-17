/*----------------------
 | spu_shadow.h
 | Description: RAM shadow of the PS1 SPU register map (0x1F801C00-0x1F801DFF) that AKAO's register macros write instead of the hardware, with change tracking for the voice flush.
 | Author: suinevere
 | Dependencies: include/main/audio/spu_registers.h
 | Globals: lom_spu_shadow, spu_shadow_key_events
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_SPU_SHADOW_H
#define LOM_SPU_SHADOW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------
 | SPU_SHADOW_HALFWORDS
 | Description: Halfwords in the shadow, covering the 0x200-byte SPU register map.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_SHADOW_HALFWORDS 0x100

/*----------------------
 | SPU_SHADOW_KEY_FIRST
 | Description: Shadow index of the key-on low register; key on low/high and key off low/high follow it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_SHADOW_KEY_FIRST (0x188 / 2)

/*----------------------
 | SPU_SHADOW_KEY_COUNT
 | Description: Write-triggered key registers (on low, on high, off low, off high).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_SHADOW_KEY_COUNT 4

/*----------------------
 | lom_spu_shadow
 | Description: The shadow register map; index = (PS1 address - 0x1F801C00) / 2.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern volatile unsigned short lom_spu_shadow[SPU_SHADOW_HALFWORDS];

/*----------------------
 | spu_shadow_key_events
 | Description: Key on low/high and key off low/high latched by the last spu_shadow_take_dirty, for the flush to apply.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern unsigned short spu_shadow_key_events[SPU_SHADOW_KEY_COUNT];

/*----------------------
 | SPU_REGISTER_BASE
 | Description: Points every spu_registers.h macro at the shadow instead of the PS1 SPU.
 | Author: suinevere
 | Dependencies: lom_spu_shadow
 | Globals: lom_spu_shadow
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SPU_REGISTER_BASE ((uintptr_t)lom_spu_shadow)

/*----------------------
 | spu_shadow_reset
 | Description: Zeroes the shadow, its last-flushed copy and the latched key events.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_spu_shadow, spu_shadow_key_events
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void spu_shadow_reset(void);

/*----------------------
 | spu_shadow_take_dirty
 | Description: Marks every halfword changed since the last call, plus any non-zero key register, then latches and clears the key registers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_spu_shadow, spu_shadow_key_events
 | Params: out_bits -- receives one bit per halfword (bit i % 32 of word i / 32)
 | Returns: non-zero if any bit is set
 ----------------------*/
int spu_shadow_take_dirty(unsigned out_bits[8]);

#ifdef __cplusplus
}
#endif

#endif
