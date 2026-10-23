/*----------------------
 | perf_hud.h
 | Description: Once-a-second speed readout on the first VMU's screen, so real-hardware frame pacing, idle time, texture, movie decode and CD stalls can be read without a serial cable.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PERF_HUD_H
#define LOM_PERF_HUD_H

/*----------------------
 | perf_hud_report
 | Description: Hands one probe period's figures, with the movie decode time and CD stream stalls since the last report, to the HUD thread to draw; skipped while the last is undrawn.
 | Author: suinevere
 | Dependencies: libpress_dc_stats, cd_stream_stalls, hud_thread, thd_create_ex, sem_signal
 | Globals: N/A
 | Params: vbl_hundredths -- vblanks per drawn frame times 100, idle_ms -- time spent waiting for vblank, texture_ms -- texture build plus upload time,
 |         otag_ms -- time walking ordering tables and submitting primitives, pvr_ms -- time waiting for the PowerVR to finish a scene, render_ms -- longest PowerVR render
 | Returns: N/A
 ----------------------*/
void perf_hud_report(unsigned vbl_hundredths, unsigned idle_ms, unsigned texture_ms, unsigned otag_ms, unsigned pvr_ms, unsigned render_ms);

#endif
