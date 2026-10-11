/*----------------------
 | frame_probe.c
 | Description: Counts PowerVR scenes and primitives per probe period and logs "LOM: probe vbl ..." every 60 vblanks and the GPU census every 600.
 | Author: suinevere
 | Dependencies: frame_probe.h, vsync_pacer.h, dc_system.h, gpu_census.h, palette_bank.h, texture_cache.h
 | Globals: s_scenes, s_prims, s_period_start, s_census_next, s_started
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "frame_probe.h"
#include "vsync_pacer.h"
#include "dc_system.h"
#include "gpu_census.h"
#include "palette_bank.h"
#include "texture_cache.h"

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
 | Description: Every 60 vblanks logs vblanks per scene to two decimals with prims, banks, expansions, flushes and subtractive uses; every 600 logs the GPU census.
 | Author: suinevere
 | Dependencies: vsync_count, dc_log, palette_bank_in_use, texture_cache_expansions, texture_cache_flushes, gpu_census_flag_count, gpu_census_report
 | Globals: s_scenes, s_prims, s_period_start, s_census_next, s_started
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void frame_probe_vblank(void)
{
    unsigned now = vsync_count();
    unsigned elapsed;
    unsigned hundredths;

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
    hundredths = s_scenes != 0 ? (elapsed * 100u + s_scenes / 2u) / s_scenes : elapsed * 100u;
    dc_log("probe vbl %u.%02u prims %u banks %u expand %u flush %u sub %u", hundredths / 100u, hundredths % 100u,
           s_prims, palette_bank_in_use(), texture_cache_expansions(), texture_cache_flushes(),
           gpu_census_flag_count(CENSUS_SUBTRACT));
    s_scenes = 0;
    s_prims = 0;
    s_period_start = now;
    if ((int)(now - s_census_next) >= 0)
    {
        s_census_next = now + CENSUS_PERIOD;
        gpu_census_report();
    }
}
