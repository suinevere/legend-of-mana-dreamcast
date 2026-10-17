/*----------------------
 | field_watch_unit.c
 | Description: Test-image-only FIELD vector table entry that registers scene id, map id, dialog0, battle-group and requested-door state with the watcher and installs main's watch-poll, scene-jump and battle hooks.
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
 | field_script_op_85 / field_battle_start
 | Description: FIELD's door scene change and battle start, redeclared here since their prototypes live in FIELD's .c files.
 | Author: suinevere
 | Dependencies: field_script_ops.c, field_actor_lifecycle.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void field_script_op_85(s32 scene_id, s32 object_id, s32 audio, s32 spawn_id);
void field_battle_start(s32 group);

/*----------------------
 | FIELD_JUMP_KEEP_AUDIO
 | Description: op 0x85 audio argument keeping the current sound bank, music and secondary music (0xFE in each byte).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FIELD_JUMP_KEEP_AUDIO 0xFEFEFE

/*----------------------
 | FIELD_TRIGGER_SCRIPT_BIT
 | Description: Trigger region command bit that starts a script rather than a battle (FIELD_TRIGGER_SCRIPT).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FIELD_TRIGGER_SCRIPT_BIT 0x8000

/*----------------------
 | s_scene_id / s_map_id / s_dialog0 / s_battle / s_door
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
static int32_t s_door;

/*----------------------
 | s_field_watch_entries
 | Description: The five watched values field_watch_install hands to watch_init; door is the scene a requested transition targets, or -1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_scene_id, s_map_id, s_dialog0, s_battle
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static WatchEntry s_field_watch_entries[5] = {
    { "scene", &s_scene_id, 0, 0 },
    { "map", &s_map_id, 0, 0 },
    { "dialog0", &s_dialog0, 0, 0 },
    { "battle", &s_battle, 0, 0 },
    { "door", &s_door, 0, 0 },
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
    s_door = g_field_runtime->transition.bits.requested ? (int32_t)g_field_runtime->transition.fields.scene_id : -1;
    watch_poll();
}

/*----------------------
 | field_scene_jump
 | Description: Test-only pad step: requests a change to scene at spawn through op 0x85, exactly as a door script does, keeping the current audio.
 | Author: suinevere
 | Dependencies: field_script_op_85, dc_log
 | Globals: N/A
 | Params: scene -- target scene id, spawn -- spawn point in it
 | Returns: N/A
 ----------------------*/
static void field_scene_jump(int scene, int spawn)
{
    dc_log("jump scene %d spawn %d", scene, spawn);
    field_script_op_85(scene, 0, FIELD_JUMP_KEEP_AUDIO, spawn);
}

/*----------------------
 | field_battle
 | Description: Test-only pad step: starts the first monster group in the current scene's trigger table, as walking into its region would, unless the scene already started one; either way it forgets older watcher lines so a later battle-0 marker means this battle ended.
 | Author: suinevere
 | Dependencies: field_battle_start, watch_clear_seen, dc_log
 | Globals: g_field_runtime
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void field_battle(void)
{
    FieldTriggerTable *table = g_field_runtime->trigger_table;
    int i;

    if (table == NULL)
    {
        dc_log("battle: no trigger table");
        return;
    }
    if (g_field_runtime->state.bits.group_active)
    {
        dc_log("battle: group %d already active", (int)g_field_runtime->state.bits.trigger_group);
        watch_clear_seen();
        return;
    }
    for (i = 0; i < table->count; i++)
    {
        if (!(table->regions[i].command & FIELD_TRIGGER_SCRIPT_BIT))
        {
            dc_log("battle group %d", (int)table->regions[i].command);
            watch_clear_seen();
            field_battle_start(table->regions[i].command);
            return;
        }
    }
    dc_log("battle: no monster group");
}

/*----------------------
 | field_watch_install
 | Description: Registers FIELD's scene, map, dialog0, battle and door state with the watcher and sets main's watch-poll, scene-jump and battle hooks; test image only.
 | Author: suinevere
 | Dependencies: watch_init
 | Globals: s_field_watch_entries, g_dc_watch_poll, g_dc_field_scene_jump, g_dc_field_battle
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void field_watch_install(void)
{
    watch_init(s_field_watch_entries, 5, field_watch_print);
    g_dc_watch_poll = field_watch_poll;
    g_dc_field_scene_jump = field_scene_jump;
    g_dc_field_battle = field_battle;
}

#endif
