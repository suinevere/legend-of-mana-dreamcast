/*----------------------
 | main_globals_dc.c
 | Description: Dreamcast definitions of the PS1 fixed-address buffers lom_prefix.h redirects to, typed main-executable globals with their PS1 initial values, and the libc references real overlays resolve from main's ELF.
 |              Holds only the globals whose PS1 address is below 0x8004FC70 (main's range per shared_symbol_addrs.txt); FIELD's own globals live in src/overlays/field_globals_dc.c since Task 6 gave FIELD its own overlay.
 | Author: suinevere
 | Dependencies: lom_prefix.h, lom_platform_memory.h, main/main.h, main/game_state.h, main/audio/game_audio.h, main/field_runtime.h, stdlib.h, saturn/src/boot/main_globals.c
 | Globals: the lom_* fixed-address buffers, the g_* main state listed below, g_overlay_exit, g_overlay_abort, g_overlay_abs
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdlib.h>

#include "main/main.h"
#include "main/game_state.h"
#include "main/audio/game_audio.h"
#include "main/field_runtime.h"

/*----------------------
 | lom_resident_state / lom_load_buffer
 | Description: PS1 fixed-address scratch state and FIELD's CD load buffer, aligned for the SH4; the buffer holds the largest music file, since nothing past it on the Dreamcast is scratch the way the PS1's CD stream area is.
 | Author: suinevere
 | Dependencies: lom_prefix.h, lom_platform_memory.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 lom_resident_state[0x1000] __attribute__((aligned(32)));
u8 lom_load_buffer[LOM_DC_LOAD_BUFFER_SIZE] __attribute__((aligned(32)));

/*----------------------
 | lom_sound_bank
 | Description: The whole PS1 AKAO bank area (0x8013C000-0x80140000); TITLE bcopies its sound-effect bank here.
 | Author: suinevere
 | Dependencies: src/overlays/title/title.c (load_title_audio_bank)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 lom_sound_bank[0x4000] __attribute__((aligned(32)));

/*----------------------
 | lom_font_buffer / lom_scratchpad / lom_text_cache / lom_trap_buffer / lom_render_arena
 | Description: The remaining PS1 fixed-address buffers lom_prefix.h redirects to, sized as the Saturn's and 32-byte aligned for the SH4.
 | Author: suinevere
 | Dependencies: lom_prefix.h, lom_platform_memory.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 lom_font_buffer[0xBE00] __attribute__((aligned(32)));
u8 lom_scratchpad[0x400] __attribute__((aligned(32)));
u8 lom_text_cache[0x3000] __attribute__((aligned(32)));
u8 lom_trap_buffer[0x100] __attribute__((aligned(32)));
u8 lom_render_arena[LOM_RENDER_ARENA_SIZE] __attribute__((aligned(32)));

/*----------------------
 | lom_field_cd_buffer / lom_field_actor_heap
 | Description: FIELD's own CD read buffer and actor heap, sized as the Saturn's; real storage once FIELD's actors load resources for real, instead of aliasing the tiny trap buffer.
 | Author: suinevere
 | Dependencies: lom_prefix.h, lom_platform_memory.h, saturn/src/boot/overlay_loader.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 lom_field_cd_buffer[0x19000] __attribute__((aligned(32)));
u8 lom_field_actor_heap[0x20000] __attribute__((aligned(32)));

/*----------------------
 | lom_cd_stream_staging / lom_cd_stream_buffer / lom_collision_lists
 | Description: FIELD's CD stream staging area, stream sector/bank buffer and collision node lists, each its own real buffer sized from the PS1 fixed-address gaps in include/common/memory_map.h, instead of aliasing the tiny trap buffer the three used to overrun together.
 | Author: suinevere
 | Dependencies: lom_prefix.h, lom_platform_memory.h, include/common/memory_map.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 lom_cd_stream_staging[0x2000] __attribute__((aligned(32)));
u8 lom_cd_stream_buffer[0x2000] __attribute__((aligned(32)));
u8 lom_collision_lists[0x200] __attribute__((aligned(32)));

/*----------------------
 | lom_secondary_overlay
 | Description: SECONDARY_OVERLAY_ADDRESS's storage, sized so main.c's GNAME_RENDER_ADDRESS (+0x20000) and WSEL_RENDER_ADDRESS (+0x30000) hold their render buffers and MOVIE's full-screen arena fits.
 | Author: suinevere
 | Dependencies: lom_platform_memory.h, src/main/main.c (GNAME_RENDER_ADDRESS, WSEL_RENDER_ADDRESS)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 lom_secondary_overlay[LOM_SECONDARY_OVERLAY_SIZE] __attribute__((aligned(32)));

/*----------------------
 | g_field_movie_frame_width / g_field_movie_frame_height
 | Description: FIELD's PS1 0x80140000/0x80140002 halfwords: the size header of the movie still FIELD reads into FIELD_MOVIE_BUFFER, which is lom_secondary_overlay here.
 | Author: suinevere
 | Dependencies: lom_secondary_overlay, src/overlays/field/actors/field_animation.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
__asm__(".global _g_field_movie_frame_width\n"
        ".set _g_field_movie_frame_width, _lom_secondary_overlay\n"
        ".global _g_field_movie_frame_height\n"
        ".set _g_field_movie_frame_height, _lom_secondary_overlay + 2\n");

/*----------------------
 | lom_primary_upper
 | Description: PRIMARY_OVERLAY_UPPER_ADDRESS for main, CHECKPS and TITLE, aliased to the render arena as saturn.linker does.
 | Author: suinevere
 | Dependencies: lom_render_arena
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern u8 lom_primary_upper[LOM_RENDER_ARENA_SIZE] __attribute__((alias("lom_render_arena")));

/*----------------------
 | g_game_state / g_previous_game_state / g_field_spawn_id / D_8003EC8C
 | Description: Core game-state globals from the PS1 executable's data segment, zero-initialised like the original.
 | Author: suinevere
 | Dependencies: main/game_state.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u32 g_game_state;
u32 g_previous_game_state;
u32 g_field_spawn_id;
s32 D_8003EC8C;

/*----------------------
 | g_pending_game_state
 | Description: Next top-level game state FIELD requests; real storage so it stops aliasing lom_trap_buffer, whose other tenants (CD stream staging, AKAO, collision lists) were stomping it mid-scene.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
s32 g_pending_game_state;

/*----------------------
 | g_field_scene_data_size / g_field_action_context
 | Description: Scene-load byte count and the actor input/effect scratch word; real storage so they stop aliasing lom_trap_buffer. Both PS1 addresses (0x800473F0, 0x800473F8) sit below 0x8004FC70, main's range.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
s32 g_field_scene_data_size;
s32 g_field_action_context;

/*----------------------
 | g_field_scene_request_pending
 | Description: Set while a scene change is queued but not yet applied; real storage so it stops aliasing lom_trap_buffer. PS1 address 0x801158A4 is past 0x8004FC70 (FIELD's own range), but field_runtime.c's run_field_scene (linked into main, not the overlay) reads and writes it directly, so it must resolve at main's own link; defined here instead of field_globals_dc.c for that reason.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
s32 g_field_scene_request_pending;

/*----------------------
 | g_field_player_map_position
 | Description: Party's map (x, z) in whole units, for trigger-region tests; real storage so it stops aliasing lom_trap_buffer. PS1 address 0x80042FC8 sits below 0x8004FC70, main's range. Matches FieldMapPoint's layout (field_interaction_start.c) without sharing its private typedef.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
struct
{
    u16 x;
    u16 z;
} g_field_player_map_position;

/*----------------------
 | g_field_scene_id
 | Description: Scene of the next field entry; main.h declares it u16 off LOM_BIG_ENDIAN; FIELD's whole-word writes are past D1.
 | Author: suinevere
 | Dependencies: main/main.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u16 g_field_scene_id;

/*----------------------
 | g_field_sound_bank_id / g_field_audio_timer / g_save_compatibility_tag / g_field_music_id / g_field_object_id / D_80042FD0 / g_field_secondary_music_id / g_music_track_index / g_script_pair_value_49 / g_playtime_vsync_origin
 | Description: FIELD's audio and save-compatibility state from the PS1 executable's data segment, zero-initialised like the original.
 | Author: suinevere
 | Dependencies: main/audio/game_audio.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
s32 g_field_sound_bank_id;
s32 g_field_audio_timer;
s32 g_save_compatibility_tag;
s32 g_field_music_id;
s32 g_field_object_id;
s32 D_80042FD0;
s32 g_field_secondary_music_id;
u16 g_music_track_index;
s32 g_script_pair_value_49;
s32 g_playtime_vsync_origin;

/*----------------------
 | g_resident_song_buffer
 | Description: The PS1 resident song area (0x8003ECA0-0x80042CA0); TITLE's load_title_seq bcopies its 0x1768-byte sequence here.
 | Author: suinevere
 | Dependencies: src/overlays/title/title.c (load_title_seq)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 g_resident_song_buffer[0x4000] __attribute__((aligned(4)));

/*----------------------
 | g_field_draw_count / g_field_force_two_primitives / g_field_primitive_cursor / g_field_current_render_half
 | Description: field_runtime.c's frame-loop state, zero like the PS1 executable's.
 | Author: suinevere
 | Dependencies: src/main/field_runtime.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
s32 g_field_draw_count;
s32 g_field_force_two_primitives;
void *g_field_primitive_cursor;
FieldRenderHalf *g_field_current_render_half;

/*----------------------
 | g_text_clut_base / g_text_cursor_x / g_text_cursor_y
 | Description: field_init_text_renderer's CLUT id and glyph cursor, zero like the PS1 executable's.
 | Author: suinevere
 | Dependencies: src/main/field_runtime.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
s32 g_text_clut_base;
s32 g_text_cursor_x;
s32 g_text_cursor_y;

/*----------------------
 | g_overlay_exit
 | Description: References libc's exit so main links it and real overlays such as CHECKPS resolve it from main's ELF.
 | Author: suinevere
 | Dependencies: libc exit
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void (*const g_overlay_exit)(int) = exit;

/*----------------------
 | g_overlay_abort
 | Description: References libc's abort so main links it and the CHECKPS and FIELD overlays resolve it from main's ELF.
 | Author: suinevere
 | Dependencies: libc abort
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void (*const g_overlay_abort)(void) = abort;

/*----------------------
 | g_overlay_abs
 | Description: References libc's abs so main links it and the FIELD overlay resolves it from main's ELF.
 | Author: suinevere
 | Dependencies: libc abs
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
int (*const g_overlay_abs)(int) = abs;

/*----------------------
 | g_music_track_table
 | Description: Music track per area, copied byte for byte from the PS1 executable's initialised data at 0x800351A0.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 g_music_track_table[0x8C] = {
    0x0a, 0x0b, 0x01, 0x00, 0x0c, 0x02, 0x00, 0x03, 0x07, 0x05, 0x06, 0x13, 0x09, 0x08, 0x00, 0x09,
    0x0e, 0x09, 0x05, 0x05, 0x00, 0x04, 0x00, 0x10, 0x11, 0x0f, 0x09, 0x0d, 0x00, 0x01, 0x05, 0x12,
    0x06, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0xff, 0xff, 0x09, 0x0a, 0xff, 0xff,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
};
