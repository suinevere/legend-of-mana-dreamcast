/*----------------------
 | field_watch_unit.c
 | Description: Test-image-only FIELD vector table entry that registers scene id, map id, dialog0 and battle-group state with the watcher and installs main's watch-poll hook.
 | Author: suinevere
 | Dependencies: watch.h, dc_system.h, main/main.h, overlays/field/internal/field_records.h
 ----------------------*/
#ifdef LOM_PAD_SCRIPT

#include "watch.h"
#include "dc_system.h"
#include "main/main.h"
#include "overlays/field/internal/field_records.h"

/*----------------------
 | g_field_map_id
 | Description: FIELD's current map id, redeclared here since it has no shared header (see field_scene_load.c).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern u16 g_field_map_id;

/*----------------------
 | g_field_text_window0_flags
 | Description: FIELD's text window 0 state word, redeclared here since it has no shared header (see field_text.c).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern s32 g_field_text_window0_flags;

/*----------------------
 | g_field_runtime
 | Description: Pointer at the field runtime context, redeclared here as every FIELD .c file does (see field_record_lookup_ops.c).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern FieldRuntimeContext *g_field_runtime;

/*----------------------
 | s_scene_id / s_map_id / s_dialog0 / s_battle
 | Description: Shadow int32 copies field_watch_poll refreshes each vblank, since watch_poll wants fixed-width addresses and battle is a bitfield.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int32_t s_scene_id;
static int32_t s_map_id;
static int32_t s_dialog0;
static int32_t s_battle;

/*----------------------
 | s_field_watch_entries
 | Description: The four watched values field_watch_install hands to watch_init.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_scene_id, s_map_id, s_dialog0, s_battle
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static WatchEntry s_field_watch_entries[4] = {
    { "scene", &s_scene_id, 0, 0 },
    { "map", &s_map_id, 0, 0 },
    { "dialog0", &s_dialog0, 0, 0 },
    { "battle", &s_battle, 0, 0 },
};

/*----------------------
 | field_watch_print
 | Description: Forwards a watch line to dc_log, dropping its "LOM: " prefix since dc_log adds one of its own.
 | Author: suinevere
 | Dependencies: dc_log
 | Globals: N/A
 | Params: line -- full "LOM: field <name> <value>" line from watch_poll
 | Returns: N/A
 ----------------------*/
static void field_watch_print(const char *line)
{
    dc_log("%s", line + 5);
}

/*----------------------
 | field_watch_poll
 | Description: Refreshes the shadow copies from live FIELD state and runs one watch_poll pass.
 | Author: suinevere
 | Dependencies: watch_poll
 | Globals: s_scene_id, s_map_id, s_dialog0, s_battle, g_field_scene_id, g_field_map_id, g_field_text_window0_flags, g_field_runtime
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void field_watch_poll(void)
{
    s_scene_id = (int32_t)g_field_scene_id;
    s_map_id = (int32_t)g_field_map_id;
    s_dialog0 = (int32_t)g_field_text_window0_flags;
    s_battle = (int32_t)g_field_runtime->state.bits.group_active;
    watch_poll();
}

/*----------------------
 | field_watch_install
 | Description: Registers FIELD's scene, map, dialog0 and battle state with the watcher and sets main's g_dc_watch_poll hook; test image only.
 | Author: suinevere
 | Dependencies: watch_init
 | Globals: s_field_watch_entries, g_dc_watch_poll
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void field_watch_install(void)
{
    watch_init(s_field_watch_entries, 4, field_watch_print);
    g_dc_watch_poll = field_watch_poll;
}

#endif
