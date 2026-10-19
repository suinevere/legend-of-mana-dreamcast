/*----------------------
 | scene_sweep_kos.c
 | Description: Test-image-only: owns the FIELD scene sweep and starts it over the range the host wrote to the VMU's LOMSWEEP file.
 | Author: suinevere
 | Dependencies: kos.h, dc/vmufs.h, scene_sweep.h, pad_script.h, dc_system.h
 | Globals: g_dc_scene_sweep
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdlib.h>
#include <kos.h>
#include <dc/vmufs.h>

#include "scene_sweep.h"
#include "pad_script.h"
#include "dc_system.h"

/*----------------------
 | SWEEP_FILE
 | Description: VMU file name holding "<first> <last>"; written by dreamcast/tools/scene_sweep.py.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define SWEEP_FILE "LOMSWEEP"

/*----------------------
 | g_dc_scene_sweep
 | Description: The sweep FIELD's watch poll steps; zeroed, so idle until scene_sweep_begin.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
SceneSweep g_dc_scene_sweep;

/*----------------------
 | scene_sweep_begin
 | Description: Reads LOMSWEEP from the first memory card, falling back to every scene when it is missing or invalid, logs the range and starts the sweep.
 | Author: suinevere
 | Dependencies: maple_enum_type, vmufs_read, scene_sweep_parse_range, scene_sweep_start, dc_log
 | Globals: g_dc_scene_sweep
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void scene_sweep_begin(void)
{
    maple_device_t *dev = maple_enum_type(0, MAPLE_FUNC_MEMCARD);
    void *data = NULL;
    int size = 0;
    int first = SCENE_SWEEP_FIRST;
    int last = SCENE_SWEEP_LAST;

    if (dev != NULL && vmufs_read(dev, SWEEP_FILE, &data, &size) >= 0 && data != NULL)
    {
        if (scene_sweep_parse_range((const char *)data, (unsigned)size, &first, &last) != 0)
        {
            dc_log("sweep: bad %s", SWEEP_FILE);
        }
        free(data);
    }
    else
    {
        dc_log("sweep: no %s", SWEEP_FILE);
    }
    dc_log("sweep range %d %d", first, last);
    scene_sweep_start(&g_dc_scene_sweep, first, last);
}
