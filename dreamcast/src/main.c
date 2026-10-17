/*----------------------
 | main.c
 | Description: Legend of Mana Dreamcast entry point: brings up KallistiOS, the PowerVR, the exception trap, vblank pacing and the overlay arena, then runs the decomp's real game loop.
 | Author: suinevere
 | Dependencies: kos.h, dc_system.h, pvr_submit.h, dc_exception.h, vblank_dc.h, overlay_loader_dc.h, libgpu_dc.h, state_report_dc.h, src/main/main.c (main_game_loop)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <kos.h>
#include "dc_system.h"
#include "pvr_submit.h"
#include "dc_exception.h"
#include "vblank_dc.h"
#include "overlay_loader_dc.h"
#include "libgpu_dc.h"
#include "state_report_dc.h"
#include "card_store_kos.h"
#include "akao_tick_dc.h"

#ifdef LOM_PAD_SCRIPT
#include "watch.h"
#endif

KOS_INIT_FLAGS(INIT_DEFAULT);

void main_game_loop(void);

#ifdef LOM_PAD_SCRIPT
/*----------------------
 | g_dc_watch_poll / g_dc_field_scene_jump / g_dc_field_battle / g_dc_field_open / g_dc_field_gameover
 | Description: Test-image-only pad-script hooks declared in watch.h; NULL until field_watch_install sets them.
 | Author: suinevere
 | Dependencies: watch.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void (*g_dc_watch_poll)(void) = NULL;
void (*g_dc_field_scene_jump)(int scene, int spawn) = NULL;
void (*g_dc_field_battle)(void) = NULL;
void (*g_dc_field_open)(int target) = NULL;
void (*g_dc_field_gameover)(void) = NULL;
#endif

/*----------------------
 | main
 | Description: Initialises the Dreamcast platform and runs main_game_loop, which never returns on the PS1.
 | Author: suinevere
 | Dependencies: dc_system_init, pvr_submit_init, dc_exception_install, vblank_dc_install, overlay_loader_init, card_store_kos_init, akao_tick_dc_init, dc_set_vsync_hook, state_report_poll, dc_log, main_game_loop, dc_halt
 | Globals: N/A
 | Params: N/A
 | Returns: never
 ----------------------*/
int main(void)
{
    dc_system_init();
    pvr_submit_init();
    dc_exception_install();
    vblank_dc_install();
    overlay_loader_init();
    card_store_kos_init();
    akao_tick_dc_init();
    dc_set_vsync_hook(state_report_poll);
    dc_log("boot");
    main_game_loop();
    dc_halt("main_game_loop returned");
}
