/*----------------------
 | lom_platform_memory.h
 | Description: The Dreamcast memory map behind lom_prefix.h: packet tags cover all 16 MB of main RAM, and regions of states D1 never reaches point at the trap buffer.
 | Author: suinevere
 | Dependencies: main_globals_dc.c
 | Globals: lom_trap_buffer, lom_secondary_overlay
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PLATFORM_MEMORY_H
#define LOM_PLATFORM_MEMORY_H

/*----------------------
 | GPU_TAG_RAM_BASE
 | Description: Dreamcast main RAM base (P1, cached) that gpu_tag_pointer ORs onto every 24-bit tag address.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define GPU_TAG_RAM_BASE 0x8C000000u

/*----------------------
 | LOM_RENDER_ARENA_SIZE
 | Description: Bytes in lom_render_arena, matching the Saturn arena and covering TITLE's double menu context (0x1FB10).
 | Author: suinevere
 | Dependencies: main_globals_dc.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOM_RENDER_ARENA_SIZE 0x20000

/*----------------------
 | LOM_SECONDARY_OVERLAY_SIZE
 | Description: Bytes in lom_secondary_overlay: the PS1 secondary region up to the end of WSEL's two 0x80CC-byte render buffers at +0x30000.
 | Author: suinevere
 | Dependencies: main_globals_dc.c, common/render_context.h, overlays/wsel/wsel.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOM_SECONDARY_OVERLAY_SIZE 0x401A0

/*----------------------
 | SECONDARY_OVERLAY_ADDRESS / SECONDARY_OVERLAY_AT
 | Description: The PS1 secondary overlay region at 0x80140000; overlays load at their own arena addresses, so it only holds GNAME's and WSEL's render buffers.
 | Author: suinevere
 | Dependencies: main_globals_dc.c
 | Globals: lom_secondary_overlay
 | Params: offset -- byte offset
 | Returns: N/A
 ----------------------*/
extern unsigned char lom_secondary_overlay[];
#define SECONDARY_OVERLAY_ADDRESS ((unsigned int)lom_secondary_overlay)
#define SECONDARY_OVERLAY_AT(offset) (SECONDARY_OVERLAY_ADDRESS + (offset))

/*----------------------
 | FIELD_CD_BUFFER_ADDRESS / FIELD_ACTOR_HEAP_ADDRESS
 | Description: FIELD's CD read buffer and actor heap at PS1 0x80140000 and 0x80158000; FIELD is past D1, so both trap.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_trap_buffer
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FIELD_CD_BUFFER_ADDRESS ((unsigned int)lom_trap_buffer)
#define FIELD_ACTOR_HEAP_ADDRESS ((unsigned int)lom_trap_buffer)

/*----------------------
 | Unported-state addresses
 | Description: Fixed PS1 regions only used by states the Dreamcast build halts before reaching; they point at a small trap buffer, as on the Saturn.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_trap_buffer
 | Params: offset -- byte offset
 | Returns: N/A
 ----------------------*/
#define AKAO_PRIMARY_SONG_ADDRESS ((unsigned int)lom_trap_buffer)
#define CD_STREAM_STAGING_ADDRESS ((unsigned int)lom_trap_buffer)
#define CD_STREAM_STAGING_AT(offset) (CD_STREAM_STAGING_ADDRESS + (offset))
#define CD_STREAM_BUFFER_ADDRESS ((unsigned int)lom_trap_buffer)
#define CD_STREAM_BUFFER_AT(offset) (CD_STREAM_BUFFER_ADDRESS + (offset))
#define COLLISION_LISTS_ADDRESS ((unsigned int)lom_trap_buffer)
#define COLLISION_LISTS_AT(offset) (COLLISION_LISTS_ADDRESS + (offset))
#define BIOS_STACK_ADDRESS 0

#endif
