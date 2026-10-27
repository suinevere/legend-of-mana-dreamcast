/*----------------------
 | rumble_dc.c
 | Description: Drives the Jump Pack on each controller's Maple port: a continuous effect at the level the game asks for, sent when the level changes and again once a second in case the pack was plugged in since, and a stop at level 0.
 | Author: suinevere
 | Dependencies: dc/maple.h, dc/maple/purupuru.h, rumble_dc.h, pad_dc_map.h (PAD_DC_RUMBLE_MAX)
 | Globals: s_sent, s_age
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <dc/maple.h>
#include <dc/maple/purupuru.h>

#include "rumble_dc.h"
#include "pad_dc_map.h"

/*----------------------
 | RUMBLE_PORTS / RUMBLE_UNITS / RUMBLE_REFRESH / RUMBLE_STOP
 | Description: The two Maple ports the game's two controllers sit on, the expansion units a pad has, the frames between re-sends of a running effect, and the raw effect that stops the motor.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define RUMBLE_PORTS 2
#define RUMBLE_UNITS 2
#define RUMBLE_REFRESH 60
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
 | rumble_dc_effect
 | Description: The raw continuous effect for a level: no duration, the level as both intensity nibbles, motor 1.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: level -- 1 to PAD_DC_RUMBLE_MAX
 | Returns: the raw effect word
 ----------------------*/
static uint32 rumble_dc_effect(int level)
{
    return ((uint32)PURUPURU_EFFECT2_LINTENSITY(level) << 16) | ((uint32)PURUPURU_EFFECT1_INTENSITY(level) << 8) | PURUPURU_SPECIAL_MOTOR1;
}

/*----------------------
 | rumble_dc_update
 | Description: Once a frame per port: sends the level's effect, or a stop at 0, when it differs from the last sent or a running effect is RUMBLE_REFRESH frames old; a busy Maple bus leaves it for the next frame, and a missing pack forgets what was sent.
 | Author: suinevere
 | Dependencies: rumble_dc_pack, rumble_dc_effect, purupuru_rumble_raw
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
    if (level == s_sent[port] && (level == 0 || s_age[port] < RUMBLE_REFRESH))
    {
        return;
    }
    dev = rumble_dc_pack(port);
    if (dev == NULL)
    {
        s_sent[port] = -1;
        return;
    }
    if (purupuru_rumble_raw(dev, level == 0 ? RUMBLE_STOP : rumble_dc_effect(level)) == MAPLE_EOK)
    {
        s_sent[port] = level;
        s_age[port] = 0;
    }
}
