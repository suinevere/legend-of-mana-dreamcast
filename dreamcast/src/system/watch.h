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

#endif
