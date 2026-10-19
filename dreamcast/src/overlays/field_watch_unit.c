/*----------------------
 | field_watch_unit.c
 | Description: Test-image-only FIELD vector table entry that registers scene, map, dialog0, battle, door and modal state with the watcher and installs main's watch-poll, scene-jump, battle and open hooks.
 | Author: suinevere
 | Dependencies: watch.h, dc_system.h, main/main.h, overlays/field/internal/field_records.h
 ----------------------*/
#ifdef LOM_PAD_SCRIPT

#include "watch.h"
#include "scene_sweep.h"
#include "dc_system.h"
#include "main/main.h"
#include "overlays/field/internal/field_records.h"
#include "overlays/field/internal/field_scene_internal.h"
#include "overlays/movie/movie_state.h"
#include "overlays/shop/shop.h"

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
 | field_script_op_85 / field_battle_start / field_control_animation / field_begin_scene_fade_out
 | Description: FIELD's door scene change, battle start and animation control, redeclared here since their prototypes live in FIELD's .c files or clash with this file's.
 | Author: suinevere
 | Dependencies: field_script_ops.c, field_actor_lifecycle.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void field_script_op_85(s32 scene_id, s32 object_id, s32 audio, s32 spawn_id);
void field_battle_start(s32 group);
void field_control_animation(s32 list_kind, s32 index, s32 keyframe, s32 op);
void field_begin_scene_fade_out(void);

/*----------------------
 | field openers / g_field_modal_state / g_field_gover_load_countdown / D_800F19AC
 | Description: FIELD's own entry points for each secondary overlay, its modal-overlay state, the game-over load countdown, and the GOSUB golem-parts screen sequence field_menu_choose_golem_parts opens.
 | Author: suinevere
 | Dependencies: src/overlays/field/ui/field_modal_runtime.c, src/overlays/field/ui/field_modal_stream_start.c, src/overlays/field/ui/field_menu_ops.c
 | Globals: g_field_modal_state, D_800F19AC
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void field_open_shop_mode_1(s32 entry_count, ShopEntry *entries, void *item_records, s32 shop_options);
void field_run_golem(void);
void field_run_zukan(s32 context);
void field_open_gosub_screen_sequence(void *screen_sequence);
void field_open_carda(s32 mode);
void field_open_niki(s32 mode);
void field_open_addhero(s32 mode);
extern s32 g_field_modal_state;
extern s32 g_field_gover_load_countdown;
void field_script_op_88(s32 image_resource_index, s32 music_resource_index, s32 audio_clip_index);

/*----------------------
 | FIELD_GAMEOVER_IMAGE / FIELD_GAMEOVER_NO_SOUND
 | Description: The game-over image operand scene 16 passes to op 0x88 (GOVER adds its own resource base, giving the 320x240 picture 4161), and the operand value op 0x88 maps to no music or clip.
 | Author: suinevere
 | Dependencies: tools/scenes/event_scripts.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FIELD_GAMEOVER_IMAGE 69
#define FIELD_GAMEOVER_NO_SOUND 0xFF
extern u8 D_800F19AC[];

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
 | FIELD_MOVIE_STEP_STREAM_STATE
 | Description: The movie handler step in FieldAnim::flags.b.state once movie_init has run (FIELD_MOVIE_STEP_STREAM in field_animation.c), after which MovieState::end_state is the movie's own.
 | Author: suinevere
 | Dependencies: src/overlays/field/actors/field_animation.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FIELD_MOVIE_STEP_STREAM_STATE 2

/*----------------------
 | FIELD_WATCH_TILE_ANIMS / FIELD_WATCH_ANIM_RESTART
 | Description: FIELD_LIST_TILE_ANIMS and FIELD_ANIM_OP_RESTART, restated since field_calls.h clashes with this file's own FIELD prototypes.
 | Author: suinevere
 | Dependencies: src/overlays/field/internal/field_calls.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FIELD_WATCH_TILE_ANIMS 0
#define FIELD_WATCH_ANIM_RESTART 2

/*----------------------
 | FIELD_WATCH_EMBEDDED_MOVIES
 | Description: Movie numbers below this play inside the map (FIELD_SCENE_MOVIE_COUNT); the rest are full-screen and start from the scene fade-out.
 | Author: suinevere
 | Dependencies: src/overlays/field/actors/field_animation.c
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FIELD_WATCH_EMBEDDED_MOVIES 2

/*----------------------
 | s_scene_id / s_map_id / s_dialog0 / s_battle / s_door / s_modal / s_open / s_gover
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
static int32_t s_movie_end = -1;
static int32_t s_door;
static int32_t s_modal;
static int32_t s_open;
static int32_t s_gover;

/*----------------------
 | s_open_request / s_opening
 | Description: The OPEN_TARGETS index the last open step asked for (-1 for none), and whether an opener is running, since overlays call VSync and so re-enter field_watch_poll.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_open_request = -1;
static int s_opening;

/*----------------------
 | FIELD_OPEN_NAMES
 | Description: Overlay names by OPEN_TARGETS index, matching dreamcast/tools/gen_pad_script.py, for the open log lines.
 | Author: suinevere
 | Dependencies: dreamcast/tools/gen_pad_script.py
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
/*----------------------
 | s_open_shop_entries / FIELD_OPEN_SHOP_OPTIONS
 | Description: A three-item buying list for "open shop", since a new game has nothing to sell and shop mode 0 would show FIELD's empty notice instead, and the options value FIELD's own shop scripts pass.
 | Author: suinevere
 | Dependencies: overlays/shop/shop.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static ShopEntry s_open_shop_entries[] = { { 1, 0, 10 }, { 2, 0, 20 }, { 3, 0, 30 } };
#define FIELD_OPEN_SHOP_OPTIONS 2

static const char *const FIELD_OPEN_NAMES[] = { "shop", "golem", "zukan", "gosub", "carda", "niki", "addhero" };

/*----------------------
 | s_field_watch_entries
 | Description: The nine watched values field_watch_install hands to watch_init; movie_end is MovieState::end_state for a fieldmovie step's movie, -1 before one streams; modal is FIELD's modal-overlay state, open is 1 while an open step's opener runs, gover is 1 once a game over starts loading; door is the scene a requested transition targets, or -1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_scene_id, s_map_id, s_dialog0, s_battle
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static WatchEntry s_field_watch_entries[9] = {
    { "scene", &s_scene_id, 0, 0 },
    { "map", &s_map_id, 0, 0 },
    { "dialog0", &s_dialog0, 0, 0 },
    { "battle", &s_battle, 0, 0 },
    { "door", &s_door, 0, 0 },
    { "modal", &s_modal, 0, 0 },
    { "open", &s_open, 0, 0 },
    { "gover", &s_gover, 0, 0 },
    { "movie_end", &s_movie_end, 0, 0 },
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
 | field_run_open_request
 | Description: Runs a pending open step's FIELD opener once, outside any opener already running; blocking openers return here when their overlay closes.
 | Author: suinevere
 | Dependencies: field openers, watch_clear_seen, dc_log
 | Globals: s_open_request, s_opening, FIELD_OPEN_NAMES, D_800F19AC, s_open_shop_entries
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void field_run_open_request(void)
{
    int target = s_open_request;

    if (target < 0 || s_opening)
    {
        return;
    }
    s_open_request = -1;
    s_opening = 1;
    watch_clear_seen();
    dc_log("open %s", FIELD_OPEN_NAMES[target]);
    switch (target)
    {
    case 0:
        field_open_shop_mode_1(3, s_open_shop_entries, NULL, FIELD_OPEN_SHOP_OPTIONS);
        break;
    case 1:
        field_run_golem();
        break;
    case 2:
        field_run_zukan(0);
        break;
    case 3:
        field_open_gosub_screen_sequence(D_800F19AC);
        break;
    case 4:
        field_open_carda(0);
        break;
    case 5:
        field_open_niki(0);
        break;
    default:
        field_open_addhero(0);
        break;
    }
    dc_log("open %s returned", FIELD_OPEN_NAMES[target]);
    s_opening = 0;
}

/*----------------------
 | field_open
 | Description: Test-only pad step: records which secondary overlay to open; field_watch_poll runs it so the opener starts from FIELD's frame, not the pad interpreter.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_open_request, FIELD_OPEN_NAMES
 | Params: target -- OPEN_TARGETS index
 | Returns: N/A
 ----------------------*/
static void field_open(int target)
{
    if (target >= 0 && target < (int)(sizeof(FIELD_OPEN_NAMES) / sizeof(FIELD_OPEN_NAMES[0])))
    {
        s_open_request = target;
    }
}

/*----------------------
 | s_movie_anim / s_movie_scene
 | Description: The tile animation a fieldmovie step started and the scene it belongs to, so a later scene change drops it.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static FieldAnim *s_movie_anim;
static FieldScene *s_movie_scene;

/*----------------------
 | field_movie_watch
 | Description: Refreshes s_movie_end from MovieState::end_state while the fieldmovie step's animation is past movie_init, keeping its last value afterwards.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_movie_anim, s_movie_scene, s_movie_end, g_field_scene
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void field_movie_watch(void)
{
    if (s_movie_anim == NULL || s_movie_scene != g_field_scene.scene)
    {
        s_movie_anim = NULL;
        return;
    }
    if (s_movie_anim->flags.b.state == FIELD_MOVIE_STEP_STREAM_STATE)
    {
        s_movie_end = (int32_t)MOVIE_STATE->end_state;
    }
}

/*----------------------
 | field_sweep_poll
 | Description: Steps the scene sweep once per vblank against FIELD's scene and pending transition, jumping and logging as it asks.
 | Author: suinevere
 | Dependencies: scene_sweep_step, field_scene_jump, dc_log
 | Globals: g_dc_scene_sweep, g_field_scene_id, g_field_runtime
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void field_scene_jump(int scene, int spawn);

static void field_sweep_poll(void)
{
    SweepEvent e = scene_sweep_step(&g_dc_scene_sweep, (int)(g_field_scene_id & 0xFFFF),
                                    g_field_runtime->transition.bits.requested ? 1 : 0);

    switch (e.kind)
    {
    case SWEEP_EVENT_JUMP:
        field_scene_jump(e.scene, 0);
        break;
    case SWEEP_EVENT_ARRIVE:
        dc_log("sweep arrive %d", e.scene);
        break;
    case SWEEP_EVENT_OK:
        dc_log("sweep ok %d", e.scene);
        break;
    case SWEEP_EVENT_STUCK:
        dc_log("sweep stuck %d at %d", e.scene, e.reached);
        break;
    case SWEEP_EVENT_DONE:
        dc_log("sweep done");
        break;
    default:
        break;
    }
}

/*----------------------
 | field_watch_poll
 | Description: Refreshes the shadow copies from live FIELD state and runs one watch_poll pass.
 | Author: suinevere
 | Dependencies: watch_poll, field_movie_watch, field_run_open_request
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
    s_modal = (int32_t)g_field_modal_state;
    s_open = s_opening;
    s_gover = g_field_gover_load_countdown != 0;
    field_movie_watch();
    watch_poll();
    field_run_open_request();
    field_sweep_poll();
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
 | field_gameover
 | Description: Test-only pad step: runs op 0x88 as a party wipe's script does, with sound off since D6 has not landed.
 | Author: suinevere
 | Dependencies: field_script_op_88, dc_log
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void field_gameover(void)
{
    dc_log("gameover");
    field_script_op_88(FIELD_GAMEOVER_IMAGE, FIELD_GAMEOVER_NO_SOUND, FIELD_GAMEOVER_NO_SOUND);
}

/*----------------------
 | field_movie
 | Description: Test-only pad step: restarts the current scene's first embedded movie animation as a scene script's op does, or for a full-screen one fades the scene out as the script's fade op does, after which FIELD starts tile animation 0.
 | Author: suinevere
 | Dependencies: field_control_animation, field_begin_scene_fade_out, dc_log
 | Globals: s_movie_anim, s_movie_scene, s_movie_end, g_field_scene
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void field_movie(void)
{
    FieldAnim *anim;
    s32 index;

    index = 0;
    for (anim = g_field_scene.scene->anims; anim != NULL; anim = anim->next)
    {
        if ((anim->def->flags.b.kind_flags & FIELD_ANIM_KIND_MASK) == FIELD_TILE_ANIM_MOVIE)
        {
            dc_log("fieldmovie anim %d movie %d", (int)index, (int)anim->def->head.b.unk1);
            s_movie_anim = anim;
            s_movie_scene = g_field_scene.scene;
            s_movie_end = -1;
            if (anim->def->head.b.unk1 < FIELD_WATCH_EMBEDDED_MOVIES)
            {
                field_control_animation(FIELD_WATCH_TILE_ANIMS, index, 0, FIELD_WATCH_ANIM_RESTART);
            }
            else
            {
                field_begin_scene_fade_out();
            }
            return;
        }
        index++;
    }
    dc_log("fieldmovie: no movie animation");
}

/*----------------------
 | field_watch_install
 | Description: Registers FIELD's scene, map, dialog0, battle and door state with the watcher and sets main's watch-poll, scene-jump and battle hooks; test image only.
 | Author: suinevere
 | Dependencies: watch_init
 | Globals: s_field_watch_entries, g_dc_watch_poll, g_dc_field_scene_jump, g_dc_field_battle, g_dc_field_open, g_dc_field_gameover, g_dc_field_movie
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void field_watch_install(void)
{
    watch_init(s_field_watch_entries, 9, field_watch_print);
    g_dc_watch_poll = field_watch_poll;
    g_dc_field_scene_jump = field_scene_jump;
    g_dc_field_battle = field_battle;
    g_dc_field_open = field_open;
    g_dc_field_gameover = field_gameover;
    g_dc_field_movie = field_movie;
}

#endif
