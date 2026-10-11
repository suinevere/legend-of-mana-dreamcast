/*----------------------
 | state_report_dc.c
 | Description: Serial progress report for the Dreamcast boot: logs every g_game_state change, once per 60 vblanks the primitives drawn, and drives the frame probe; under LOM_PAD_SCRIPT also the pad script.
 | Author: suinevere
 | Dependencies: lom_prefix.h, main/game_state.h, dc_system.h, vsync_pacer.h, libgpu_dc.h, frame_probe.h, pad_script.h (LOM_PAD_SCRIPT)
 | Globals: g_game_state, s_reported_state, s_next_report
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "main/game_state.h"

#include "state_report_dc.h"
#include "dc_system.h"
#include "vsync_pacer.h"
#include "libgpu_dc.h"
#include "frame_probe.h"

#ifdef LOM_PAD_SCRIPT
#include "pad_script.h"
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

/*----------------------
 | state_report_poll
 | Description: Logs "state <n>" on a g_game_state change and "frame <count> prims <n>" each period, ticks the frame probe and, under LOM_PAD_SCRIPT, advances the pad script.
 | Author: suinevere
 | Dependencies: dc_log, vsync_count, dc_frame_primitives, frame_probe_vblank, pad_script_poll (LOM_PAD_SCRIPT)
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
    frame_probe_vblank();
#ifdef LOM_PAD_SCRIPT
    pad_script_poll(now, (int)g_game_state);
#endif
}
