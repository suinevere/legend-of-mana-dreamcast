/*----------------------
 | scene_sweep.h
 | Description: Test-only FIELD scene sweep: a pure per-vblank state machine that jumps to each scene in a range, waits for it to arrive and settle, and reports what happened.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef SCENE_SWEEP_H
#define SCENE_SWEEP_H

/*----------------------
 | SCENE_SWEEP_FIRST / SCENE_SWEEP_LAST
 | Description: The scene ids on the US disc (scenes 0-929 exist; none from 930 up).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SCENE_SWEEP_FIRST 0
#define SCENE_SWEEP_LAST 929

/*----------------------
 | SCENE_SWEEP_ARRIVE_VBLANKS
 | Description: Vblanks a jumped-to scene has to arrive before the sweep calls it stuck.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SCENE_SWEEP_ARRIVE_VBLANKS 1200

/*----------------------
 | SCENE_SWEEP_SETTLE_VBLANKS
 | Description: Vblanks an arrived scene must stay put before the sweep calls it ok.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SCENE_SWEEP_SETTLE_VBLANKS 240

/*----------------------
 | SCENE_SWEEP_IDLE / SCENE_SWEEP_JUMP / SCENE_SWEEP_ARRIVE / SCENE_SWEEP_SETTLE
 | Description: SceneSweep.phase values.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SCENE_SWEEP_IDLE 0
#define SCENE_SWEEP_JUMP 1
#define SCENE_SWEEP_ARRIVE 2
#define SCENE_SWEEP_SETTLE 3

/*----------------------
 | SWEEP_EVENT_*
 | Description: SweepEvent.kind values: nothing, jump to scene, scene arrived, scene settled ok, scene stuck (at reached), sweep done.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SWEEP_EVENT_NONE 0
#define SWEEP_EVENT_JUMP 1
#define SWEEP_EVENT_ARRIVE 2
#define SWEEP_EVENT_OK 3
#define SWEEP_EVENT_STUCK 4
#define SWEEP_EVENT_DONE 5

/*----------------------
 | SceneSweep
 | Description: Sweep state: phase, next scene to jump to, last scene, current target and vblanks left in the phase.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct SceneSweep
{
    int phase;
    int next;
    int last;
    int target;
    int wait;
} SceneSweep;

/*----------------------
 | SweepEvent
 | Description: What one sweep step asks the caller to do or log: kind, the scene it concerns, and for STUCK the scene actually reached.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
typedef struct SweepEvent
{
    int kind;
    int scene;
    int reached;
} SweepEvent;

/*----------------------
 | scene_sweep_start
 | Description: Starts a sweep over first..last clamped to the disc's scenes; an empty range leaves it idle.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: s -- sweep, first -- first scene, last -- last scene
 | Returns: N/A
 ----------------------*/
void scene_sweep_start(SceneSweep *s, int first, int last);

/*----------------------
 | scene_sweep_step
 | Description: Advances the sweep by one vblank given the current scene and whether a scene change is pending.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: s -- sweep, scene_id -- FIELD's current scene, transition_busy -- nonzero while a scene change is requested
 | Returns: the event to act on or log
 ----------------------*/
SweepEvent scene_sweep_step(SceneSweep *s, int scene_id, int transition_busy);

/*----------------------
 | scene_sweep_parse_range
 | Description: Parses "<first> <last>" from a block-padded VMU file, clamping to the disc's scenes.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: text -- file bytes, len -- byte count, first/last -- out range, untouched on failure
 | Returns: 0, or -1 for a missing, negative or empty range
 ----------------------*/
int scene_sweep_parse_range(const char *text, unsigned len, int *first, int *last);

/*----------------------
 | g_dc_scene_sweep
 | Description: Test-image-only: the sweep FIELD's watch poll steps; idle until scene_sweep_begin (scene_sweep_kos.c).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
extern SceneSweep g_dc_scene_sweep;

#endif
