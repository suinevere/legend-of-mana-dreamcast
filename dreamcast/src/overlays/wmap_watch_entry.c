/*----------------------
 | wmap_watch_entry.c
 | Description: Always-compiled WMAP overlay entry main's loader calls after WMAP finishes loading; under LOM_PAD_SCRIPT it installs the landfx hook, which plays a land placement effect the way confirming a cell does, and watches WMAP's event flag.
 | Author: suinevere
 | Dependencies: watch.h, dc_system.h, src/overlays/wmap/effects/wmap_land_effect_loader.c, src/overlays/wmap/map/wmap_land_transition.c
 | Globals: s_landfx_request, s_event_active, s_wmap_watch_entries
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifdef LOM_PAD_SCRIPT
#include <stdint.h>
#include "watch.h"
#include "dc_system.h"

typedef int (*WmapSequenceCallback)(int initialize);
void func_800591A8(int selection);
void wmap_place_land(int x, int y, int index);
void wmap_install_callback(WmapSequenceCallback callback);
int wmap_begin_land_placement(int initialize);
extern int32_t g_wmap_focus_cell_x;
extern int32_t g_wmap_focus_cell_y;
extern int32_t g_wmap_event_active;
extern int32_t g_wmap_input_locked;
extern int32_t g_wmap_placement_preview_locked;

/*----------------------
 | WMAP_LANDFX_SHARED_LAND
 | Description: The selection whose placement installs land 16, as wmap_land_transition.c's confirm path does.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define WMAP_LANDFX_SHARED_LAND 22

/*----------------------
 | s_landfx_request / s_event_active / s_wmap_watch_entries
 | Description: The effect a landfx step asked for (-1 for none), WMAP's event flag as the watcher samples it, and the watcher's table, which prints "field landfx_event <n>".
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_landfx_request = -1;
static int32_t s_event_active;
static WatchEntry s_wmap_watch_entries[1] = {
    { "landfx_event", &s_event_active, 0, 0 },
};

/*----------------------
 | wmap_watch_print
 | Description: Forwards a watch line to dc_log, dropping its "LOM: " prefix since dc_log adds one of its own.
 | Author: suinevere
 | Dependencies: dc_log
 | Globals: N/A
 | Params: line -- full "LOM: field <name> <value>" line from watch_poll
 | Returns: N/A
 ----------------------*/
static void wmap_watch_print(const char *line)
{
    dc_log("%s", line + 5);
}

/*----------------------
 | wmap_landfx
 | Description: g_dc_wmap_landfx: queues one land placement effect for the next idle poll.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: s_landfx_request
 | Params: land -- effect selection
 | Returns: N/A
 ----------------------*/
static void wmap_landfx(int land)
{
    s_landfx_request = land;
}

/*----------------------
 | wmap_watch_poll
 | Description: Samples the event flag for the watcher and, once no event runs, starts a queued effect at the focused cell with the confirm path's own calls.
 | Author: suinevere
 | Dependencies: watch_poll, func_800591A8, wmap_place_land, wmap_install_callback, wmap_begin_land_placement, dc_log
 | Globals: s_landfx_request, s_event_active, g_wmap_event_active, g_wmap_input_locked, g_wmap_placement_preview_locked, g_wmap_focus_cell_x, g_wmap_focus_cell_y
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void wmap_watch_poll(void)
{
    int land;

    s_event_active = g_wmap_event_active;
    watch_poll();
    if (s_landfx_request < 0 || g_wmap_event_active != 0)
    {
        return;
    }
    land = s_landfx_request;
    s_landfx_request = -1;
    dc_log("landfx %d", land);
    g_wmap_input_locked = 1;
    g_wmap_placement_preview_locked = 1;
    g_wmap_event_active = 1;
    func_800591A8(land);
    wmap_place_land(g_wmap_focus_cell_x, g_wmap_focus_cell_y, land == WMAP_LANDFX_SHARED_LAND ? 16 : land);
    wmap_install_callback(wmap_begin_land_placement);
}
#endif

/*----------------------
 | wmap_watch_entry
 | Description: WMAP's Dreamcast-only extra vector slot; installs the landfx hook and event watcher in the test image, else does nothing.
 | Author: suinevere
 | Dependencies: watch_init
 | Globals: s_wmap_watch_entries, g_dc_watch_poll, g_dc_wmap_landfx
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void wmap_watch_entry(void)
{
#ifdef LOM_PAD_SCRIPT
    watch_init(s_wmap_watch_entries, 1, wmap_watch_print);
    g_dc_watch_poll = wmap_watch_poll;
    g_dc_wmap_landfx = wmap_landfx;
#endif
}
