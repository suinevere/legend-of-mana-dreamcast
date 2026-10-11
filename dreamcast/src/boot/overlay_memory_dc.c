/*----------------------
 | overlay_memory_dc.c
 | Description: Dreamcast replacement for overlay_memory.c: overlays load into their own arena regions and per-state workspaces come from the render arena, as on the Saturn.
 | Author: suinevere
 | Dependencies: lom_prefix.h, main/overlay_memory.h, overlays/title/internal/title_internal.h, overlay_regions.h, lom_platform_memory.h
 | Globals: lom_render_arena
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include "main/overlay_memory.h"
#include "overlays/title/internal/title_internal.h"
#include "overlay_regions.h"

_Static_assert(2 * sizeof(TitleMenuContext) <= LOM_RENDER_ARENA_SIZE, "render arena too small for TITLE");

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
