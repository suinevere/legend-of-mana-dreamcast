/*----------------------
 | title_debug_entry.c
 | Description: Always-compiled TITLE overlay entry main's loader calls after TITLE finishes loading; under LOM_DEBUG_MENU it shows the title menu's hidden third item, which starts FIELD from the debug saved-game template, as the GameShark code D0102672 0101 / 30102674 0001 does.
 | Author: suinevere
 | Dependencies: watch.h, src/overlays/title/title.c (g_title_menu_item_flags)
 | Globals: g_title_menu_item_flags, g_dc_watch_poll
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifdef LOM_DEBUG_MENU
#include "watch.h"

extern unsigned char g_title_menu_item_flags[];

/*----------------------
 | TITLE_DEBUG_ITEM
 | Description: Byte offsets in g_title_menu_item_flags: slot 1's two set bytes (Continue, ready once init_title_menu_state ran) and slot 2's enable byte (the debug start).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define TITLE_CONTINUE_FLAGS 2
#define TITLE_DEBUG_ITEM 4

/*----------------------
 | title_debug_poll
 | Description: Per-frame hook: once the title menu has set up its items, enables the hidden debug item.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: g_title_menu_item_flags
 | Params: N/A
 | Returns: N/A
 ----------------------*/
static void title_debug_poll(void)
{
    if (g_title_menu_item_flags[TITLE_CONTINUE_FLAGS] == 1 && g_title_menu_item_flags[TITLE_CONTINUE_FLAGS + 1] == 1)
    {
        g_title_menu_item_flags[TITLE_DEBUG_ITEM] = 1;
    }
}
#endif

/*----------------------
 | title_debug_entry
 | Description: TITLE's Dreamcast-only extra vector slot; installs title_debug_poll in the debug-menu build, else does nothing.
 | Author: suinevere
 | Dependencies: title_debug_poll
 | Globals: g_dc_watch_poll
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void title_debug_entry(void)
{
#ifdef LOM_DEBUG_MENU
    g_dc_watch_poll = title_debug_poll;
#endif
}
