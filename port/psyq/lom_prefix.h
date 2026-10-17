/*----------------------
 | lom_prefix.h
 | Description: Forced-include prefix that maps the decomp's fixed PS1 addresses to platform buffers and makes packet macros endian-correct; each backend's lom_platform_memory.h supplies its own memory map.
 | Author: suinevere
 | Dependencies: gte_saturn.h, lom_platform_memory.h, spu_shadow.h (Dreamcast), common.h, libgpu.h, common/gpu_packet.h
 | Globals: lom_resident_state, lom_load_buffer, lom_sound_bank, lom_font_buffer, lom_render_arena, lom_primary_upper, lom_scratchpad, lom_text_cache, lom_trap_buffer
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PREFIX_H
#define LOM_PREFIX_H

#define MEMORY_MAP_H
#define SDK_MEMORY_H
#define INCLUDE_ASM_H
#define SDK_INLINE_C_BRIDGE_H
#include "gte_saturn.h"

#include "version.h"
#include "lom_platform_memory.h"

extern unsigned char lom_resident_state[];
extern unsigned char lom_load_buffer[];
extern unsigned char lom_sound_bank[];

/*----------------------
 | LOM_LOAD_BUFFER_SIZE
 | Description: Byte size of lom_load_buffer, matching the PS1 0x5A000-byte load region at 0x80180000.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOM_LOAD_BUFFER_SIZE 0x5A000

/*----------------------
 | RESIDENT_STATE_ADDRESS and derived blocks
 | Description: The PS1 resident state block at 0x801ED000, redirected to a platform buffer of the same layout.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_resident_state
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RESIDENT_STATE_ADDRESS ((unsigned int)lom_resident_state)
#define FIELD_WORKSPACE_ADDRESS RESIDENT_STATE_ADDRESS
#define MAP_BOUNDS_ADDRESS (RESIDENT_STATE_ADDRESS + 0x400)
#define TEXT_CONFIG_ADDRESS (RESIDENT_STATE_ADDRESS + 0x408)
#define SCENE_STATE_ADDRESS (RESIDENT_STATE_ADDRESS + 0x480)
#define MOVIE_STATE_ADDRESS (RESIDENT_STATE_ADDRESS + 0x500)
#define CONTROLLER_STATE_ADDRESS (RESIDENT_STATE_ADDRESS + 0x600)
#define CD_SYSTEM_ADDRESS (RESIDENT_STATE_ADDRESS + 0x800)
#define CD_RESOURCE_ENTRIES_ADDRESS (RESIDENT_STATE_ADDRESS + 0x998)

/*----------------------
 | LOAD_BUFFER_ADDRESS / LOAD_BUFFER_AT
 | Description: The PS1 CD staging buffer at 0x80180000, redirected to a platform buffer (on the Saturn it shares LWRAM with resident overlay data until a CD read fills it).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_load_buffer
 | Params: offset -- byte offset into the buffer
 | Returns: N/A
 ----------------------*/
#define LOAD_BUFFER_ADDRESS ((unsigned int)lom_load_buffer)
#define LOAD_BUFFER_AT(offset) (LOAD_BUFFER_ADDRESS + (offset))

/*----------------------
 | SOUND_BANK_ADDRESS
 | Description: The PS1 AKAO bank area at 0x8013C000, redirected to a platform buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_sound_bank
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SOUND_BANK_ADDRESS ((unsigned int)lom_sound_bank)

extern unsigned char lom_font_buffer[];
extern unsigned char lom_render_arena[];
extern unsigned char lom_primary_upper[];
extern unsigned char lom_scratchpad[];
extern unsigned char lom_text_cache[];
extern unsigned char lom_trap_buffer[];

/*----------------------
 | FONT_ADDRESS / FONT_AT
 | Description: The PS1 field font block at 0x801E1200 (0xBE00 bytes), redirected to a platform buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_font_buffer
 | Params: offset -- byte offset
 | Returns: N/A
 ----------------------*/
#define FONT_ADDRESS ((unsigned int)lom_font_buffer)
#define FONT_AT(offset) (FONT_ADDRESS + (offset))

/*----------------------
 | PRIMARY_OVERLAY_UPPER_ADDRESS
 | Description: The PS1 page at 0x80100000, resolved per link: the render arena for main and CHECKPS, FIELD's own slot page in FIELD.
 | Author: suinevere
 | Dependencies: saturn.linker, tools/overlay_link.py, main_globals_dc.c
 | Globals: lom_primary_upper
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PRIMARY_OVERLAY_UPPER_ADDRESS ((unsigned int)lom_primary_upper)

/*----------------------
 | SCRATCHPAD_ADDRESS / SCRATCHPAD_SIZE / SCRATCHPAD_AT
 | Description: The PS1 1 KB scratchpad at 0x1F800000, redirected to a platform buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_scratchpad
 | Params: offset -- byte offset
 | Returns: N/A
 ----------------------*/
#define SCRATCHPAD_ADDRESS ((unsigned int)lom_scratchpad)
#define SCRATCHPAD_SIZE 0x400
#define SCRATCHPAD_AT(offset) (SCRATCHPAD_ADDRESS + (offset))

/*----------------------
 | TEXT_CACHE_ADDRESS
 | Description: The PS1 text cache at 0x801DE000 (0x3000 bytes), redirected to a platform buffer.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_text_cache
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TEXT_CACHE_ADDRESS ((unsigned int)lom_text_cache)

/*----------------------
 | SPU registers
 | Description: On the Dreamcast, spu_shadow.h defines SPU_REGISTER_BASE first, so AKAO's spu_registers.h macros write the RAM shadow the voice flush reads.
 | Author: suinevere
 | Dependencies: ../sound/spu_shadow.h, include/main/audio/spu_registers.h
 | Globals: lom_spu_shadow
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifdef LOM_DREAMCAST
#include "../sound/spu_shadow.h"
#endif

#include "common.h"

#ifndef LOM_SKIP_PREFIX_BCOPY
extern void *bcopy(const unsigned char *src, unsigned char *dst, int count);
#endif

#include <libgpu.h>
#include "common/gpu_packet.h"

#undef setlen
#undef setaddr
#undef getaddr
#undef addPrim
#undef addPrims
#undef SET_BGR0_PACKED

/*----------------------
 | setlen
 | Description: Stores a packet word count in the top byte of the tag word, matching the PS1 numeric tag layout.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: prim -- packet, length -- word count
 | Returns: N/A
 ----------------------*/
#define setlen(prim, length) \
    (((u_long *)(prim))[0] = (((u_long *)(prim))[0] & 0x00FFFFFFu) | ((u_long)(u_char)(length) << 24))

/*----------------------
 | setaddr
 | Description: Stores the low 24 bits of the next-packet address in the tag word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: prim -- packet, address -- next packet
 | Returns: N/A
 ----------------------*/
#define setaddr(prim, address) \
    (((u_long *)(prim))[0] = (((u_long *)(prim))[0] & 0xFF000000u) | ((u_long)(address) & 0x00FFFFFFu))

/*----------------------
 | getaddr
 | Description: Reads the raw 24-bit next-packet field of a tag word.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: prim -- packet
 | Returns: raw 24-bit address field
 ----------------------*/
#define getaddr(prim) (((u_long *)(prim))[0] & 0x00FFFFFFu)

/*----------------------
 | addPrim / addPrims
 | Description: Links a packet (or packet run) in front of an ordering-table entry's chain.
 | Author: suinevere
 | Dependencies: setaddr, getaddr
 | Globals: N/A
 | Params: ordering_table -- OT entry, prim/first/last -- packets
 | Returns: N/A
 ----------------------*/
#define addPrim(ordering_table, prim) \
    (setaddr((prim), getaddr(ordering_table)), setaddr((ordering_table), (prim)))
#define addPrims(ordering_table, first, last) \
    (setaddr((last), getaddr(ordering_table)), setaddr((ordering_table), (first)))

/*----------------------
 | SET_BGR0_PACKED
 | Description: Writes a little-endian packed colour word byte by byte so r0/g0/b0/code land where the PS1 put them.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: p -- packet, _word -- PS1 colour word (r low byte)
 | Returns: N/A
 ----------------------*/
#define SET_BGR0_PACKED(p, _word)                          \
    (((u8 *)(p))[4] = (u8)((u32)(_word)),                  \
     ((u8 *)(p))[5] = (u8)((u32)(_word) >> 8),             \
     ((u8 *)(p))[6] = (u8)((u32)(_word) >> 16),            \
     ((u8 *)(p))[7] = (u8)((u32)(_word) >> 24))

#endif
