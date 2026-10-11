/*----------------------
 | main.c
 | Description: Legend of Mana Dreamcast entry point: brings up KallistiOS, the exception trap, vblank pacing and the overlay arena, then runs the decomp's real game loop.
 | Author: suinevere
 | Dependencies: kos.h, dc_system.h, dc_exception.h, vblank_dc.h, overlay_loader_dc.h, libgpu_dc.h, state_report_dc.h, src/main/main.c (main_game_loop)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <kos.h>
#include "dc_system.h"
#include "dc_exception.h"
#include "vblank_dc.h"
#include "overlay_loader_dc.h"
#include "libgpu_dc.h"
#include "state_report_dc.h"

KOS_INIT_FLAGS(INIT_DEFAULT);

void main_game_loop(void);

/*----------------------
 | main
 | Description: Initialises the Dreamcast platform and runs main_game_loop, which never returns on the PS1.
 | Author: suinevere
 | Dependencies: dc_system_init, dc_exception_install, vblank_dc_install, overlay_loader_init, dc_set_vsync_hook, state_report_poll, dc_log, main_game_loop, dc_halt
 | Globals: N/A
 | Params: N/A
 | Returns: never
 ----------------------*/
int main(void)
{
    dc_system_init();
    dc_exception_install();
    vblank_dc_install();
    overlay_loader_init();
    dc_set_vsync_hook(state_report_poll);
    dc_log("boot");
    main_game_loop();
    dc_halt("main_game_loop returned");
}
