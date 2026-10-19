/*----------------------
 | scene_sweep.c
 | Description: Test-only FIELD scene sweep state machine and VMU range parser.
 | Author: suinevere
 | Dependencies: scene_sweep.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "scene_sweep.h"

/*----------------------
 | clamp_scene
 | Description: Clamps a scene id into the disc's range.
 | Author: suinevere
 | Dependencies: SCENE_SWEEP_FIRST, SCENE_SWEEP_LAST
 | Globals: N/A
 | Params: scene -- scene id
 | Returns: the clamped id
 ----------------------*/
static int clamp_scene(int scene)
{
    if (scene < SCENE_SWEEP_FIRST)
    {
        return SCENE_SWEEP_FIRST;
    }
    return scene > SCENE_SWEEP_LAST ? SCENE_SWEEP_LAST : scene;
}

/*----------------------
 | event
 | Description: Builds a SweepEvent.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: kind -- SWEEP_EVENT_*, scene -- scene concerned, reached -- scene reached for STUCK
 | Returns: the event
 ----------------------*/
static SweepEvent event(int kind, int scene, int reached)
{
    SweepEvent e;

    e.kind = kind;
    e.scene = scene;
    e.reached = reached;
    return e;
}

/*----------------------
 | scene_sweep_start
 | Description: Starts a sweep over first..last clamped to the disc's scenes; an empty range leaves it idle.
 | Author: suinevere
 | Dependencies: clamp_scene
 | Globals: N/A
 | Params: s -- sweep, first -- first scene, last -- last scene
 | Returns: N/A
 ----------------------*/
void scene_sweep_start(SceneSweep *s, int first, int last)
{
    s->next = clamp_scene(first);
    s->last = clamp_scene(last);
    s->target = -1;
    s->wait = 0;
    s->phase = s->next > s->last ? SCENE_SWEEP_IDLE : SCENE_SWEEP_JUMP;
}

/*----------------------
 | scene_sweep_step
 | Description: Advances the sweep by one vblank: jumps when no change is pending, counts arrival and settle time, and reports stuck scenes and scenes that left on their own.
 | Author: suinevere
 | Dependencies: event
 | Globals: N/A
 | Params: s -- sweep, scene_id -- FIELD's current scene, transition_busy -- nonzero while a scene change is requested
 | Returns: the event to act on or log
 ----------------------*/
SweepEvent scene_sweep_step(SceneSweep *s, int scene_id, int transition_busy)
{
    switch (s->phase)
    {
    case SCENE_SWEEP_JUMP:
        if (transition_busy)
        {
            return event(SWEEP_EVENT_NONE, 0, 0);
        }
        if (s->next > s->last)
        {
            s->phase = SCENE_SWEEP_IDLE;
            return event(SWEEP_EVENT_DONE, 0, 0);
        }
        s->target = s->next++;
        s->wait = SCENE_SWEEP_ARRIVE_VBLANKS;
        s->phase = SCENE_SWEEP_ARRIVE;
        return event(SWEEP_EVENT_JUMP, s->target, 0);
    case SCENE_SWEEP_ARRIVE:
        if (scene_id == s->target && !transition_busy)
        {
            s->wait = SCENE_SWEEP_SETTLE_VBLANKS;
            s->phase = SCENE_SWEEP_SETTLE;
            return event(SWEEP_EVENT_ARRIVE, s->target, 0);
        }
        if (--s->wait <= 0)
        {
            s->phase = SCENE_SWEEP_JUMP;
            return event(SWEEP_EVENT_STUCK, s->target, scene_id);
        }
        return event(SWEEP_EVENT_NONE, 0, 0);
    case SCENE_SWEEP_SETTLE:
        if (scene_id != s->target)
        {
            s->phase = SCENE_SWEEP_JUMP;
            return event(SWEEP_EVENT_STUCK, s->target, scene_id);
        }
        if (--s->wait <= 0)
        {
            s->phase = SCENE_SWEEP_JUMP;
            return event(SWEEP_EVENT_OK, s->target, 0);
        }
        return event(SWEEP_EVENT_NONE, 0, 0);
    default:
        return event(SWEEP_EVENT_NONE, 0, 0);
    }
}

/*----------------------
 | read_number
 | Description: Skips spaces and tabs, then reads a run of decimal digits.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: text -- bytes, len -- byte count, pos -- in/out cursor, out -- value read
 | Returns: 0, or -1 when no digits follow (a '-' counts as no digits)
 ----------------------*/
static int read_number(const char *text, unsigned len, unsigned *pos, int *out)
{
    int value = 0;
    int digits = 0;

    while (*pos < len && (text[*pos] == ' ' || text[*pos] == '\t'))
    {
        (*pos)++;
    }
    while (*pos < len && text[*pos] >= '0' && text[*pos] <= '9')
    {
        if (value < 100000)
        {
            value = value * 10 + (text[*pos] - '0');
        }
        digits++;
        (*pos)++;
    }
    if (digits == 0)
    {
        return -1;
    }
    *out = value;
    return 0;
}

/*----------------------
 | scene_sweep_parse_range
 | Description: Parses "<first> <last>" from a block-padded VMU file, clamping to the disc's scenes.
 | Author: suinevere
 | Dependencies: read_number, clamp_scene
 | Globals: N/A
 | Params: text -- file bytes, len -- byte count, first/last -- out range, untouched on failure
 | Returns: 0, or -1 for a missing, negative or empty range
 ----------------------*/
int scene_sweep_parse_range(const char *text, unsigned len, int *first, int *last)
{
    unsigned pos = 0;
    int a, b;

    if (read_number(text, len, &pos, &a) != 0 || read_number(text, len, &pos, &b) != 0)
    {
        return -1;
    }
    if (pos < len && text[pos] != '\n' && text[pos] != '\0' && text[pos] != ' ' && text[pos] != '\t' && text[pos] != '\r')
    {
        return -1;
    }
    a = clamp_scene(a);
    b = clamp_scene(b);
    if (a > b)
    {
        return -1;
    }
    *first = a;
    *last = b;
    return 0;
}
