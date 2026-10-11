/*----------------------
 | state_report_dc.c
 | Description: Serial progress report for the Dreamcast boot: logs every g_game_state change and, once per 60 vblanks, the primitives drawn in that second.
 | Author: suinevere
 | Dependencies: lom_prefix.h, main/game_state.h, dc_system.h, vsync_pacer.h, libgpu_dc.h
 | Globals: g_game_state, s_reported_state, s_next_report
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "main/game_state.h"

#include "state_report_dc.h"
#include "dc_system.h"
#include "vsync_pacer.h"
#include "libgpu_dc.h"

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

/*----------------------
 | state_report_poll
 | Description: Logs "state <n>" when g_game_state changed since the last poll, and "frame <count> prims <n>" once each period has passed.
 | Author: suinevere
 | Dependencies: dc_log, vsync_count, dc_frame_primitives
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
    if ((int)(now - s_next_report) >= 0)
    {
        s_next_report = now + STATE_REPORT_PERIOD;
        dc_log("frame %u prims %u", now, dc_frame_primitives());
    }
}
