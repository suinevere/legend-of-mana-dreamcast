/*----------------------
 | lom_platform_memory.h
 | Description: The Dreamcast memory map behind lom_prefix.h: packet tags cover all 16 MB of main RAM, and regions of states D1 never reaches point at the trap buffer.
 | Author: suinevere
 | Dependencies: main_globals_dc.c
 | Globals: lom_trap_buffer, lom_secondary_overlay, lom_field_cd_buffer, lom_field_actor_heap, lom_cd_stream_staging, lom_cd_stream_buffer, lom_collision_lists
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
 | LOM_DC_LOAD_BUFFER_SIZE
 | Description: Bytes in the Dreamcast lom_load_buffer: room for the largest music file field_load_song reads whole (0x5EC9C bytes), past the PS1's 0x5A000-byte region whose reads spill into the scratch above it.
 | Author: suinevere
 | Dependencies: main_globals_dc.c, src/overlays/field/runtime/field_audio_runtime.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOM_DC_LOAD_BUFFER_SIZE 0x60000

/*----------------------
 | LOM_SECONDARY_OVERLAY_SIZE
 | Description: Bytes in lom_secondary_overlay: the PS1 secondary region up to the end of MOVIE's full-screen buffer arena at +0x66A00, past WSEL's render buffers at +0x30000.
 | Author: suinevere
 | Dependencies: main_globals_dc.c, common/render_context.h, overlays/wsel/wsel.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOM_SECONDARY_OVERLAY_SIZE 0x66A00

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
 | Description: FIELD's CD read buffer and actor heap, at PS1 0x80140000 and 0x80158000, each its own real buffer as on the Saturn, now that FIELD's actors reach them for real.
 | Author: suinevere
 | Dependencies: main_globals_dc.c, saturn/src/boot/lom_platform_memory.h
 | Globals: lom_field_cd_buffer, lom_field_actor_heap
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern unsigned char lom_field_cd_buffer[];
extern unsigned char lom_field_actor_heap[];
#define FIELD_CD_BUFFER_ADDRESS ((unsigned int)lom_field_cd_buffer)
#define FIELD_ACTOR_HEAP_ADDRESS ((unsigned int)lom_field_actor_heap)

/*----------------------
 | CD_STREAM_STAGING_ADDRESS / CD_STREAM_BUFFER_ADDRESS / COLLISION_LISTS_ADDRESS
 | Description: FIELD's CD stream staging area, stream sector/bank buffer and collision node lists, each its own real buffer sized from include/common/memory_map.h's PS1 layout (the gap to the next fixed address: CD_STREAM_STAGING_ADDRESS 0x801DA000 to CD_STREAM_BUFFER_ADDRESS 0x801DC000 is 0x2000; CD_STREAM_BUFFER_ADDRESS 0x801DC000 to TEXT_CACHE_ADDRESS 0x801DE000 is 0x2000; COLLISION_LISTS_ADDRESS 0x801E1000 to FONT_ADDRESS 0x801E1200 is 0x200), instead of aliasing the 256-byte trap buffer all three used to overrun together into lom_render_arena.
 | Author: suinevere
 | Dependencies: main_globals_dc.c, include/common/memory_map.h
 | Globals: lom_cd_stream_staging, lom_cd_stream_buffer, lom_collision_lists
 | Params: offset -- byte offset
 | Returns: N/A
 ----------------------*/
extern unsigned char lom_cd_stream_staging[];
extern unsigned char lom_cd_stream_buffer[];
extern unsigned char lom_collision_lists[];
#define CD_STREAM_STAGING_ADDRESS ((unsigned int)lom_cd_stream_staging)
#define CD_STREAM_STAGING_AT(offset) (CD_STREAM_STAGING_ADDRESS + (offset))
#define CD_STREAM_BUFFER_ADDRESS ((unsigned int)lom_cd_stream_buffer)
#define CD_STREAM_BUFFER_AT(offset) (CD_STREAM_BUFFER_ADDRESS + (offset))
#define COLLISION_LISTS_ADDRESS ((unsigned int)lom_collision_lists)
#define COLLISION_LISTS_AT(offset) (COLLISION_LISTS_ADDRESS + (offset))

/*----------------------
 | AKAO_PRIMARY_SONG_ADDRESS / BIOS_STACK_ADDRESS
 | Description: AKAO_PRIMARY_SONG_ADDRESS is the address of g_akao_seq_channel0 itself, as on the PS1, read by akao_cmd.c after its AKAO headers declare it. BIOS_STACK_ADDRESS is unused (D1 never boots through the BIOS stack).
 | Author: suinevere
 | Dependencies: src/main/audio/internal/akao_driver.h (g_akao_seq_channel0)
 | Globals: g_akao_seq_channel0
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define AKAO_PRIMARY_SONG_ADDRESS ((unsigned int)&g_akao_seq_channel0)
#define BIOS_STACK_ADDRESS 0

#endif
