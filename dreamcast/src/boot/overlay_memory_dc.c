/*----------------------
 | overlay_memory_dc.c
 | Description: Dreamcast replacement for overlay_memory.c: overlays load into their own arena regions and per-state workspaces come from the render arena, as on the Saturn.
 | Author: suinevere
 | Dependencies: lom_prefix.h, main/overlay_memory.h, overlays/title/internal/title_internal.h, overlays/wsel/wsel.h, overlay_regions.h, common/render_context.h, lom_platform_memory.h
 | Globals: lom_render_arena
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "main/overlay_memory.h"
#include "overlays/title/internal/title_internal.h"
#include "overlays/wsel/wsel.h"
#include "overlay_regions.h"
#include "common/render_context.h"

/*----------------------
 | title_menu_context_fits
 | Description: Compile-time check that TITLE's double-buffered menu context, placed at the render arena, fits inside it.
 | Author: suinevere
 | Dependencies: TitleMenuContext, LOM_RENDER_ARENA_SIZE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
_Static_assert(2 * sizeof(TitleMenuContext) <= LOM_RENDER_ARENA_SIZE, "render arena too small for TITLE");

/*----------------------
 | gname_render_contexts_fit
 | Description: Compile-time check that GNAME's two render contexts at main.c's GNAME_RENDER_ADDRESS (+0x20000) end inside lom_secondary_overlay.
 | Author: suinevere
 | Dependencies: RenderContext, LOM_SECONDARY_OVERLAY_SIZE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
_Static_assert(0x20000 + 2 * sizeof(RenderContext) <= LOM_SECONDARY_OVERLAY_SIZE, "secondary overlay too small for GNAME's render contexts");

/*----------------------
 | wsel_render_buffers_fit
 | Description: Compile-time check that WSEL's two render buffers at main.c's WSEL_RENDER_ADDRESS (+0x30000) end inside lom_secondary_overlay.
 | Author: suinevere
 | Dependencies: WselRenderBuffer, LOM_SECONDARY_OVERLAY_SIZE
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
_Static_assert(0x30000 + 2 * sizeof(WselRenderBuffer) <= LOM_SECONDARY_OVERLAY_SIZE, "secondary overlay too small for WSEL's render buffers");

/*----------------------
 | get_overlay_load_base
 | Description: The PS1 overlay load base; on the Dreamcast, the start of the overlay arena.
 | Author: suinevere
 | Dependencies: overlay_regions.h
 | Globals: N/A
 | Params: N/A
 | Returns: arena address
 ----------------------*/
void *get_overlay_load_base(void)
{
    return (void *)OVERLAY_ARENA_BASE;
}

/*----------------------
 | get_field_render_buffers
 | Description: FIELD's render halves; FIELD is past D1, so this hands out the render arena.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_render_arena
 | Params: N/A
 | Returns: arena address
 ----------------------*/
void *get_field_render_buffers(void)
{
    return lom_render_arena;
}

/*----------------------
 | get_world_map_overlay_end
 | Description: World map end marker; the world map is not ported, so this hands out the render arena.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_render_arena
 | Params: N/A
 | Returns: arena address
 ----------------------*/
void *get_world_map_overlay_end(void)
{
    return lom_render_arena;
}

/*----------------------
 | get_title_menu_buffers
 | Description: TITLE's double-buffered menu context, placed in the render arena.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: lom_render_arena
 | Params: N/A
 | Returns: arena address
 ----------------------*/
void *get_title_menu_buffers(void)
{
    return lom_render_arena;
}
