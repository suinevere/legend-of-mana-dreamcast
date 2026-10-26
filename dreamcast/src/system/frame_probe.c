/*----------------------
 | frame_probe.c
 | Description: Counts PowerVR scenes and primitives per probe period and logs "LOM: probe vbl ..." every 60 vblanks and the GPU census every 600.
 | Author: suinevere
 | Dependencies: frame_probe.h, vsync_pacer.h, dc_system.h, gpu_census.h, palette_bank.h, texture_cache.h, libgpu_dc.h, perf_hud.h, pvr_backend.h
 | Globals: s_scenes, s_prims, s_period_start, s_census_next, s_started, s_last_wait_us, s_last_build_us, s_last_upload_us, s_scene_fill, s_fill_sum, s_fill_max
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
 | s_drawn
 | Description: Closed scenes that drew something since start-up.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static volatile unsigned s_drawn;

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
 | s_scene_fill / s_fill_sum / s_fill_max
 | Description: The open scene's area per PROBE_AREA_* kind, those areas summed over the probe period's scenes, and the period's largest scene total, in PowerVR pixels.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static float s_scene_fill[3];
static float s_fill_sum[3];
static float s_fill_max;

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
    float total = s_scene_fill[0] + s_scene_fill[1] + s_scene_fill[2];
    int k;

    for (k = 0; k < 3; k++)
    {
        s_fill_sum[k] += s_scene_fill[k];
        s_scene_fill[k] = 0.0f;
    }
    if (total > s_fill_max)
    {
        s_fill_max = total;
    }
    s_scenes++;
}

void frame_probe_drawn(void)
{
    s_drawn++;
}

unsigned frame_probe_drawn_total(void)
{
    return s_drawn;
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

void frame_probe_area(int kind, float pixels)
{
    s_scene_fill[kind] += pixels;
}

/*----------------------
 | fill_k
 | Description: The probe period's average area per scene for one PROBE_AREA_* kind, in thousands of PowerVR pixels.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_fill_sum, s_scenes
 | Params: kind -- PROBE_AREA_*
 | Returns: kilopixels per scene
 ----------------------*/
static unsigned fill_k(int kind)
{
    return s_scenes != 0 ? (unsigned)(s_fill_sum[kind] / (float)s_scenes / 1000.0f) : 0u;
}

/*----------------------
 | frame_probe_vblank
 | Description: Every 60 vblanks logs vblanks per scene to two decimals with prims, banks, expansions, flushes, subtractive uses, texture builds and the period's build, upload, VSync-wait, ordering-table and PowerVR-wait milliseconds and the longest render; every 600 logs the GPU census. In the debug build (LOM_DEBUG_MENU) each probe line also goes to the VMU readout, which the shipping build leaves to the pet screen.
 | Author: suinevere
 | Dependencies: vsync_count, dc_log, palette_bank_in_use, texture_cache_expansions, texture_cache_flushes, texture_cache_builds, texture_cache_build_us, texture_cache_upload_us, vsync_wait_us, gpu_census_flag_count, gpu_census_report, perf_hud_report, dc_otag_us, pvr_backend_wait_us, pvr_backend_take_render_max_us, pvr_backend_take_flip_expired
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
    unsigned flip_expired;

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
    flip_expired = pvr_backend_take_flip_expired();
    hundredths = s_scenes != 0 ? (elapsed * 100u + s_scenes / 2u) / s_scenes : elapsed * 100u;
    dc_log("probe vbl %u.%02u prims %u banks %u expand %u flush %u sub %u builds %u build_ms %u upload_ms %u wait_ms %u otag_ms %u pvr_ms %u render_max_ms %u flip_expired %u fill_k %u/%u/%u fill_max_k %u",
           hundredths / 100u, hundredths % 100u, s_prims, palette_bank_in_use(), texture_cache_expansions(), texture_cache_flushes(),
           gpu_census_flag_count(CENSUS_SUBTRACT), texture_cache_builds(), (unsigned)((texture_cache_build_us() - s_last_build_us) / 1000u),
           (unsigned)((texture_cache_upload_us() - s_last_upload_us) / 1000u), (unsigned)((vsync_wait_us() - s_last_wait_us) / 1000u),
           (unsigned)((dc_otag_us() - s_last_otag_us) / 1000u), (unsigned)((pvr_backend_wait_us() - s_last_pvr_us) / 1000u), render_ms, flip_expired, fill_k(0), fill_k(1), fill_k(2), (unsigned)(s_fill_max / 1000.0f));
#if defined(LOM_DEBUG_MENU)
    perf_hud_report(hundredths, (unsigned)((vsync_wait_us() - s_last_wait_us) / 1000u),
                    (unsigned)((texture_cache_build_us() - s_last_build_us + texture_cache_upload_us() - s_last_upload_us) / 1000u),
                    (unsigned)((dc_otag_us() - s_last_otag_us) / 1000u), (unsigned)((pvr_backend_wait_us() - s_last_pvr_us) / 1000u), render_ms, flip_expired);
#endif
    s_last_otag_us = dc_otag_us();
    s_last_pvr_us = pvr_backend_wait_us();
    s_last_build_us = texture_cache_build_us();
    s_last_upload_us = texture_cache_upload_us();
    s_last_wait_us = vsync_wait_us();
    s_scenes = 0;
    s_fill_sum[0] = s_fill_sum[1] = s_fill_sum[2] = 0.0f;
    s_fill_max = 0.0f;
    s_prims = 0;
    s_period_start = now;
    if ((int)(now - s_census_next) >= 0)
    {
        s_census_next = now + CENSUS_PERIOD;
        gpu_census_report();
    }
}
