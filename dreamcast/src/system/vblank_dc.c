/*----------------------
 | vblank_dc.c
 | Description: Registers a KallistiOS vblank handler that ticks the shared PS1 VSync pacer and sweeps stuck Maple frames.
 | Author: suinevere
 | Dependencies: vblank_dc.h, dc/vblank.h, vsync_pacer.h, maple_sweep_dc.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdint.h>

#include <dc/vblank.h>

#include "vblank_dc.h"
#include "vsync_pacer.h"
#include "maple_sweep_dc.h"

/*----------------------
 | vblank_dc_handler
 | Description: KallistiOS vblank callback that ticks the VSync pacer and clears Maple frames a timeout left queued.
 | Author: suinevere
 | Dependencies: vsync_tick, maple_sweep_dc
 | Globals: N/A
 | Params: code -- ASIC event code, ignored; data -- user pointer, ignored
 | Returns: N/A
 ----------------------*/
static void vblank_dc_handler(uint32_t code, void *data)
{
    (void)code;
    (void)data;
    vsync_tick();
    maple_sweep_dc();
}

/*----------------------
 | vblank_dc_install
 | Description: Registers vblank_dc_handler with KallistiOS so vsync_tick runs every vblank.
 | Author: suinevere
 | Dependencies: vblank_handler_add, vblank_dc_handler
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void vblank_dc_install(void)
{
    vblank_handler_add(vblank_dc_handler, (void *)0);
}
