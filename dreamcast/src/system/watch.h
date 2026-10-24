/*----------------------
 | watch.h
 | Description: Change-printing watcher for a handful of signed 32-bit values: prints a line once at first poll and again on each change, and remembers recent lines for marker checks.
 | Author: suinevere
 | Dependencies: stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_WATCH_H
#define LOM_WATCH_H

#include <stdint.h>

/*----------------------
 | WatchEntry
 | Description: One watched value: its printed name, the address watch_poll samples, the last value printed and whether it has printed yet.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct
{
    const char *name;
    const volatile int32_t *value;
    int32_t last;
    uint8_t primed;
} WatchEntry;

/*----------------------
 | watch_init
 | Description: Starts watching entries with printer as the line sink; resets every entry to unprinted and clears the recent-line ring.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: entries -- watch table, count -- number of entries, printer -- callback receiving each full "LOM: field <name> <value>" line
 | Returns: N/A
 ----------------------*/
void watch_init(WatchEntry *entries, unsigned count, void (*printer)(const char *));

/*----------------------
 | watch_poll
 | Description: Samples every watched entry, printing its line on the first poll after watch_init and again whenever its value changes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void watch_poll(void);

/*----------------------
 | watch_seen
 | Description: Reports whether any of the last 32 printed lines contains text as a substring.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: text -- substring to search recent printed lines for
 | Returns: 1 if a recent line contains text, else 0
 ----------------------*/
int watch_seen(const char *text);

/*----------------------
 | watch_clear_seen
 | Description: Forgets every recently printed line, so a later watch_seen only finds lines printed after this call.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void watch_clear_seen(void);


/*----------------------
 | g_dc_watch_poll
 | Description: Main's per-vblank watcher hook, set by field_watch_install; NULL until installed, called from the test image's VSync handler.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_dc_watch_poll
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern void (*g_dc_watch_poll)(void);

/*----------------------
 | g_dc_field_scene_jump
 | Description: Main's pad-script hook for an immediate field scene jump, set by field_watch_install; NULL until installed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_dc_field_scene_jump
 | Params: scene -- target scene id, spawn -- spawn point in that scene
 | Returns: N/A
 ----------------------*/
extern void (*g_dc_field_scene_jump)(int scene, int spawn);

/*----------------------
 | g_dc_field_battle
 | Description: Main's pad-script hook to force-enter a battle, set by field_watch_install; NULL until installed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_dc_field_battle
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern void (*g_dc_field_battle)(void);

/*----------------------
 | g_dc_field_open
 | Description: Main's pad-script hook to open a secondary overlay through FIELD's own opener, set by field_watch_install; NULL until installed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_dc_field_open
 | Params: target -- index into gen_pad_script.OPEN_TARGETS
 | Returns: N/A
 ----------------------*/
extern void (*g_dc_field_open)(int target);

/*----------------------
 | g_dc_wmap_landfx
 | Description: Main's pad-script hook that plays one land placement effect on the world map, set by wmap_watch_entry in the test image; NULL until installed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_dc_wmap_landfx
 | Params: land -- land effect selection, as func_800591A8 takes it
 | Returns: N/A
 ----------------------*/
extern void (*g_dc_wmap_landfx)(int land);

/*----------------------
 | g_dc_field_gameover
 | Description: Main's pad-script hook that starts the game-over transition a party wipe's script would, set by field_watch_install; NULL until installed.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_dc_field_gameover
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern void (*g_dc_field_gameover)(void);

/*----------------------
 | g_dc_movie_probe
 | Description: The test image's decode-speed probe, defined by movie_probe_dc.c; NULL where that file is not linked.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_dc_movie_probe
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern void (*g_dc_movie_probe)(void);

/*----------------------
 | g_dc_field_movie
 | Description: Test-only pad step hook that starts the current scene's first movie animation; set by field_watch_install, NULL until FIELD loads.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_dc_field_movie
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern void (*g_dc_field_movie)(void);

#endif
