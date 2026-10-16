/*----------------------
 | field_globals_dc.c
 | Description: Dreamcast definitions of the FIELD-owned PS1 globals (address >= 0x8004FC70, main's range per shared_symbol_addrs.txt) moved out of main_globals_dc.c when FIELD became its own overlay.
 | Author: suinevere
 | Dependencies: main/main.h, main/audio/akao.h, common/saved_game.h, overlays/field/field_text.h
 | Globals: g_pad_input, g_pad_input_inject, g_frame_counter, g_saved_game_ctx, g_field_buffered_input, g_field_modal_state, g_field_primary_held_buttons, g_field_primary_repeat_delay, g_field_secondary_held_buttons, g_field_secondary_repeat_delay, g_field_instrument_bank, g_field_song_handles, g_field_song_volume, g_field_text_macros
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "main/main.h"
#include "main/audio/akao.h"
#include "common/saved_game.h"
#include "overlays/field/field_text.h"

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
