/*----------------------
 | frame_probe.c
 | Description: Counts PowerVR scenes and primitives per probe period and logs "LOM: probe vbl ..." every 60 vblanks and the GPU census every 600.
 | Author: suinevere
 | Dependencies: frame_probe.h, vsync_pacer.h, dc_system.h, gpu_census.h, palette_bank.h, texture_cache.h, libgpu_dc.h, perf_hud.h, pvr_backend.h
 | Globals: s_scenes, s_prims, s_period_start, s_census_next, s_started, s_last_wait_us, s_last_build_us, s_last_upload_us
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "frame_probe.h"
#include "vsync_pacer.h"
#include "dc_system.h"
#include "gpu_census.h"
#include "palette_bank.h"
#include "texture_cache.h"
#include "libgpu_dc.h"
#include "perf_hud.h"
#include "pvr_backend.h"

/*----------------------
 | PROBE_PERIOD
 | Description: Vblanks between probe lines; 60 is one NTSC second.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define PROBE_PERIOD 60u

/*----------------------
 | CENSUS_PERIOD
 | Description: Vblanks between GPU census lines.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define CENSUS_PERIOD 600u

/*----------------------
 | s_scenes
 | Description: Scenes closed since the period started.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_scenes;

/*----------------------
 | s_prims
 | Description: Primitives submitted since the period started.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_prims;

/*----------------------
 | s_last_wait_us / s_last_build_us / s_last_upload_us / s_last_otag_us / s_last_pvr_us
 | Description: VSync wait, texture build, texture upload, ordering-table submit and PowerVR wait totals at the start of the period, so each probe line reports that period's share.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned long long s_last_wait_us;
static unsigned long long s_last_otag_us;
static unsigned long long s_last_pvr_us;
static unsigned long long s_last_build_us;
static unsigned long long s_last_upload_us;

/*----------------------
 | s_period_start
 | Description: vsync_count value when the current probe period began.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_period_start;

/*----------------------
 | s_census_next
 | Description: vsync_count value at which the next census line is due.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static unsigned s_census_next;

/*----------------------
 | s_started
 | Description: Set once the first vblank call has anchored both periods.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_started;

/*----------------------
 | frame_probe_scene
 | Description: Counts one PowerVR scene closed by pvr_submit_flip.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_scenes
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void frame_probe_scene(void)
{
    s_scenes++;
}

/*----------------------
 | frame_probe_prim
 | Description: Counts one primitive pvr_submit_prim wrote to the PowerVR.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_prims
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void frame_probe_prim(void)
{
    s_prims++;
}

/*----------------------
 | frame_probe_vblank
 | Description: Every 60 vblanks logs vblanks per scene to two decimals with prims, banks, expansions, flushes, subtractive uses, texture builds and the period's build, upload, VSync-wait, ordering-table and PowerVR-wait milliseconds and the longest render; every 600 logs the GPU census. Each probe line also goes to the VMU readout.
 | Author: suinevere
 | Dependencies: vsync_count, dc_log, palette_bank_in_use, texture_cache_expansions, texture_cache_flushes, texture_cache_builds, texture_cache_build_us, texture_cache_upload_us, vsync_wait_us, gpu_census_flag_count, gpu_census_report, perf_hud_report, dc_otag_us, pvr_backend_wait_us, pvr_backend_take_render_max_us
 | Globals: s_scenes, s_prims, s_period_start, s_census_next, s_started, s_last_wait_us, s_last_build_us, s_last_upload_us
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void frame_probe_vblank(void)
{
    unsigned now = vsync_count();
    unsigned elapsed;
    unsigned hundredths;
    unsigned render_ms;

    if (!s_started)
    {
        s_started = 1;
        s_period_start = now;
        s_census_next = now + CENSUS_PERIOD;
        return;
    }
    elapsed = now - s_period_start;
    if (elapsed < PROBE_PERIOD)
    {
        return;
    }
    render_ms = pvr_backend_take_render_max_us() / 1000u;
    hundredths = s_scenes != 0 ? (elapsed * 100u + s_scenes / 2u) / s_scenes : elapsed * 100u;
    dc_log("probe vbl %u.%02u prims %u banks %u expand %u flush %u sub %u builds %u build_ms %u upload_ms %u wait_ms %u otag_ms %u pvr_ms %u render_max_ms %u",
           hundredths / 100u, hundredths % 100u, s_prims, palette_bank_in_use(), texture_cache_expansions(), texture_cache_flushes(),
           gpu_census_flag_count(CENSUS_SUBTRACT), texture_cache_builds(), (unsigned)((texture_cache_build_us() - s_last_build_us) / 1000u),
           (unsigned)((texture_cache_upload_us() - s_last_upload_us) / 1000u), (unsigned)((vsync_wait_us() - s_last_wait_us) / 1000u),
           (unsigned)((dc_otag_us() - s_last_otag_us) / 1000u), (unsigned)((pvr_backend_wait_us() - s_last_pvr_us) / 1000u), render_ms);
    perf_hud_report(hundredths, (unsigned)((vsync_wait_us() - s_last_wait_us) / 1000u),
                    (unsigned)((texture_cache_build_us() - s_last_build_us + texture_cache_upload_us() - s_last_upload_us) / 1000u),
                    (unsigned)((dc_otag_us() - s_last_otag_us) / 1000u), (unsigned)((pvr_backend_wait_us() - s_last_pvr_us) / 1000u), render_ms);
    s_last_otag_us = dc_otag_us();
    s_last_pvr_us = pvr_backend_wait_us();
    s_last_build_us = texture_cache_build_us();
    s_last_upload_us = texture_cache_upload_us();
    s_last_wait_us = vsync_wait_us();
    s_scenes = 0;
    s_prims = 0;
    s_period_start = now;
    if ((int)(now - s_census_next) >= 0)
    {
        s_census_next = now + CENSUS_PERIOD;
        gpu_census_report();
    }
}
