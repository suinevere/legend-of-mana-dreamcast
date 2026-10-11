/*----------------------
 | main_globals_dc.c
 | Description: Dreamcast definitions of the PS1 fixed-address buffers lom_prefix.h redirects to, typed main-executable globals with their PS1 initial values, and the libc references real overlays resolve from main's ELF.
 | Author: suinevere
 | Dependencies: lom_prefix.h, lom_platform_memory.h, main/main.h, main/game_state.h, main/audio/game_audio.h, main/field_runtime.h, main/audio/akao.h, common/saved_game.h, overlays/field/field_text.h, stdlib.h, saturn/src/boot/main_globals.c
 | Globals: the lom_* fixed-address buffers, the g_* main and resident-FIELD state listed below, g_overlay_exit, g_overlay_abort, g_overlay_abs
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdlib.h>

#include "main/main.h"
#include "main/game_state.h"
#include "main/audio/game_audio.h"
#include "main/field_runtime.h"
#include "main/audio/akao.h"
#include "common/saved_game.h"
#include "overlays/field/field_text.h"

/*----------------------
 | lom_resident_state / lom_load_buffer
 | Description: PS1 fixed-address scratch state and FIELD's CD load buffer, sized as the Saturn's but aligned for the SH4 instead of pinned to LWRAM.
 | Author: suinevere
 | Dependencies: lom_prefix.h, lom_platform_memory.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 lom_resident_state[0x1000] __attribute__((aligned(32)));
u8 lom_load_buffer[LOM_LOAD_BUFFER_SIZE] __attribute__((aligned(32)));

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
 | lom_secondary_overlay
 | Description: SECONDARY_OVERLAY_ADDRESS's storage, sized so main.c's GNAME_RENDER_ADDRESS (+0x20000) and WSEL_RENDER_ADDRESS (+0x30000) hold their render buffers.
 | Author: suinevere
 | Dependencies: lom_platform_memory.h, src/main/main.c (GNAME_RENDER_ADDRESS, WSEL_RENDER_ADDRESS)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
u8 lom_secondary_overlay[LOM_SECONDARY_OVERLAY_SIZE] __attribute__((aligned(32)));

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
 | g_pad_input / g_pad_input_inject / g_frame_counter / g_saved_game_ctx
 | Description: FIELD's delivered pad buttons, frame counter and saved-game pointer that GNAME reads, zero like FIELD's freshly loaded databin.
 | Author: suinevere
 | Dependencies: main/main.h, common/saved_game.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
s32 g_pad_input;
s32 g_pad_input_inject;
s32 g_frame_counter;
SavedGameLayout *g_saved_game_ctx;

/*----------------------
 | g_field_buffered_input / g_field_modal_state / g_field_primary_held_buttons / g_field_primary_repeat_delay / g_field_secondary_held_buttons / g_field_secondary_repeat_delay
 | Description: field_update_input_repeat's held-button and repeat-delay state, which GNAME's frame loop drives.
 | Author: suinevere
 | Dependencies: src/overlays/field/ui/field_modal_runtime.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
s32 g_field_buffered_input;
s32 g_field_modal_state;
s32 g_field_primary_held_buttons;
s32 g_field_primary_repeat_delay;
s32 g_field_secondary_held_buttons;
s32 g_field_secondary_repeat_delay;

/*----------------------
 | g_field_instrument_bank / g_field_song_handles / g_field_song_volume
 | Description: FIELD's resident instrument bank pointer, song handles and song volume, which its entry music and sound effects use.
 | Author: suinevere
 | Dependencies: src/overlays/field/runtime/field_audio_runtime.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
AkaoHeader *g_field_instrument_bank;
s32 g_field_song_handles[2];
s32 g_field_song_volume;

/*----------------------
 | g_field_text_macros
 | Description: FIELD's sixteen text macro slots (PS1 0x80122B80), which the text renderer expands.
 | Author: suinevere
 | Dependencies: overlays/field/field_text.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
FieldTextMacro g_field_text_macros[16];

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
