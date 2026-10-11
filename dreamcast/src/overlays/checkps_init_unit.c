/*----------------------
 | checkps_init_unit.c
 | Description: Compiles the decomp's unmodified CHECKPS init.c into the Dreamcast CHECKPS overlay, where its private CheckPSRenderState is complete enough to size-check.
 | Author: suinevere
 | Dependencies: src/overlays/checkps/init.c, lom_prefix.h (forced include), lom_platform_memory.h (LOM_RENDER_ARENA_SIZE)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "overlays/checkps/init.c"

/*----------------------
 | checkps_render_state_fits
 | Description: Compile-time check that main.c's CHECKPS render state, placed at the render arena, fits inside it.
 | Author: suinevere
 | Dependencies: CheckPSRenderState, LOM_RENDER_ARENA_SIZE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
_Static_assert(sizeof(CheckPSRenderState) <= LOM_RENDER_ARENA_SIZE, "render arena too small for CHECKPS");
