/*----------------------
 | rumble_dc.c
 | Description: Drives the Jump Pack on each controller's Maple port: a continuous vibration at the level the game asks for, sent when the level changes and re-sent while it holds so the pack's auto-stop never ends it, and a stop at level 0.
 | Author: suinevere
 | Dependencies: dc/maple.h, dc/maple/purupuru.h, rumble_dc.h, pad_dc_map.h (PAD_DC_RUMBLE_MAX, pad_dc_rumble_due, pad_dc_rumble_effect), heap_probe.h
 | Globals: s_sent, s_age
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <dc/maple.h>
#include <dc/maple/purupuru.h>

#include "rumble_dc.h"
#include "pad_dc_map.h"
#include "heap_probe.h"

/*----------------------
 | RUMBLE_PORTS / RUMBLE_UNITS / RUMBLE_STOP
 | Description: The two Maple ports the game's two controllers sit on, the expansion units a pad has, and the raw effect that stops the motor.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RUMBLE_PORTS 2
#define RUMBLE_UNITS 2
#define RUMBLE_STOP 0u

/*----------------------
 | s_sent / s_age
 | Description: Each port's level last sent, -1 for none since boot or since its pack went away, and the frames since.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static int s_sent[RUMBLE_PORTS] = {-1, -1};
static int s_age[RUMBLE_PORTS];

/*----------------------
 | rumble_dc_pack
 | Description: The Jump Pack in one of a Maple port's expansion units, if any.
 | Author: suinevere
 | Dependencies: maple_enum_dev
 | Globals: N/A
 | Params: port -- Maple port
 | Returns: the device, or NULL
 ----------------------*/
static maple_device_t *rumble_dc_pack(int port)
{
    int unit;

    for (unit = 1; unit <= RUMBLE_UNITS; unit++)
    {
        maple_device_t *dev = maple_enum_dev(port, unit);

        if (dev != NULL && dev->valid && (dev->info.functions & MAPLE_FUNC_PURUPURU))
        {
            return dev;
        }
    }
    return NULL;
}

/*----------------------
 | rumble_dc_update
 | Description: Once a frame per port: sends the level's effect, or a stop at 0, when pad_dc_rumble_due says so; a busy Maple bus leaves it for the next frame, and a missing pack forgets what was sent.
 | Author: suinevere
 | Dependencies: rumble_dc_pack, pad_dc_rumble_due, pad_dc_rumble_effect, purupuru_rumble_raw, heap_probe
 | Globals: s_sent, s_age
 | Params: port -- 0 or 1, level -- 0 to PAD_DC_RUMBLE_MAX
 | Returns: N/A
 ----------------------*/
void rumble_dc_update(int port, int level)
{
    maple_device_t *dev;

    if (port < 0 || port >= RUMBLE_PORTS)
    {
        return;
    }
    if (level > PAD_DC_RUMBLE_MAX)
    {
        level = PAD_DC_RUMBLE_MAX;
    }
    s_age[port]++;
    if (!pad_dc_rumble_due(level, s_sent[port], s_age[port]))
    {
        return;
    }
    dev = rumble_dc_pack(port);
    if (dev == NULL)
    {
        s_sent[port] = -1;
        return;
    }
    heap_probe("rumble in");
    if (purupuru_rumble_raw(dev, level == 0 ? RUMBLE_STOP : pad_dc_rumble_effect(level)) == MAPLE_EOK)
    {
        s_sent[port] = level;
        s_age[port] = 0;
    }
    heap_probe("rumble out");
}
