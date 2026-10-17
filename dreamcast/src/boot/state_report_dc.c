/*----------------------
 | state_report_dc.c
 | Description: Serial progress report for the Dreamcast boot: logs every g_game_state change, field scene, spawn, pending-state and scene-request change, once per 60 vblanks the primitives drawn, and drives the frame probe. Under LOM_PAD_SCRIPT it also advances the pad script and watcher.
 | Author: suinevere
 | Dependencies: lom_prefix.h, main/game_state.h, dc_system.h, vsync_pacer.h, libgpu_dc.h, frame_probe.h, pad_script.h (LOM_PAD_SCRIPT), watch.h (LOM_PAD_SCRIPT)
 | Globals: g_game_state, g_field_scene_id, g_field_spawn_id, g_pending_game_state, g_field_scene_request_pending, s_reported_state, s_next_report, s_reported_field
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "main/game_state.h"

#include "state_report_dc.h"
#include "dc_system.h"
#include "vsync_pacer.h"
#include "libgpu_dc.h"
#include "frame_probe.h"
#include "akao_tick_dc.h"

#ifdef LOM_PAD_SCRIPT
#include "pad_script.h"
#include "watch.h"
#endif

/*----------------------
 | STATE_REPORT_PERIOD
 | Description: Vblanks between primitive-count lines; 60 is one NTSC second.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define STATE_REPORT_PERIOD 60u

/*----------------------
 | s_reported_state
 | Description: The g_game_state value last logged; starts outside every state so the first poll logs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static u32 s_reported_state = 0xFFFFFFFFu;

/*----------------------
 | s_next_report
 | Description: vsync_count value at which the next primitive-count line is due.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_next_report = STATE_REPORT_PERIOD;

extern s32 g_field_scene_id;
extern u32 g_field_spawn_id;
extern s32 g_pending_game_state;
extern s32 g_field_scene_request_pending;

/*----------------------
 | s_reported_field
 | Description: Field scene, spawn, pending state and scene-request values last logged; starts at impossible values so the first poll logs.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static s32 s_reported_field[4] = { -99, -99, -99, -99 };

/*----------------------
 | report_field_changes
 | Description: Logs "field scene <id> spawn <n>" (the id is the low 16 bits; the rest holds other state), "pending state <n>" and "scene request <n>" whenever those main globals change, so a stalled scene change shows how far it got.
 | Author: suinevere
 | Dependencies: dc_log
 | Globals: g_field_scene_id, g_field_spawn_id, g_pending_game_state, g_field_scene_request_pending, s_reported_field
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void report_field_changes(void)
{
    if ((g_field_scene_id & 0xFFFF) != s_reported_field[0] || (s32)g_field_spawn_id != s_reported_field[1])
    {
        s_reported_field[0] = g_field_scene_id & 0xFFFF;
        s_reported_field[1] = (s32)g_field_spawn_id;
        dc_log("field scene %d spawn %u", (int)(g_field_scene_id & 0xFFFF), (unsigned)g_field_spawn_id);
    }
    if (g_pending_game_state != s_reported_field[2])
    {
        s_reported_field[2] = g_pending_game_state;
        dc_log("pending state %d", (int)g_pending_game_state);
    }
    if (g_field_scene_request_pending != s_reported_field[3])
    {
        s_reported_field[3] = g_field_scene_request_pending;
        dc_log("scene request %d", (int)g_field_scene_request_pending);
    }
}

/*----------------------
 | state_report_poll
 | Description: Logs "state <n>" on a g_game_state change and "frame <count> prims <n>" and "akao ticks <n>" each period, ticks the frame probe and, under LOM_PAD_SCRIPT, advances the pad script and the watcher hook.
 | Author: suinevere
 | Dependencies: dc_log, vsync_count, dc_frame_primitives, akao_tick_dc_count, frame_probe_vblank, pad_script_poll (LOM_PAD_SCRIPT), g_dc_watch_poll (LOM_PAD_SCRIPT)
 | Globals: g_game_state, s_reported_state, s_next_report
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void state_report_poll(void)
{
    unsigned now = vsync_count();

    if (g_game_state != s_reported_state)
    {
        s_reported_state = g_game_state;
        dc_log("state %u", (unsigned)g_game_state);
    }
    report_field_changes();
    if ((int)(now - s_next_report) >= 0)
    {
        s_next_report = now + STATE_REPORT_PERIOD;
        dc_log("frame %u prims %u", now, dc_frame_primitives());
        dc_log("akao ticks %u", akao_tick_dc_count());
    }
    frame_probe_vblank();
#ifdef LOM_PAD_SCRIPT
    pad_script_poll(now, (int)g_game_state);
    if (g_dc_watch_poll != NULL)
    {
        g_dc_watch_poll();
    }
#endif
}
