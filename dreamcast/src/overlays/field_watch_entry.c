/*----------------------
 | field_watch_entry.c
 | Description: Always-compiled FIELD overlay entry main's loader calls after FIELD finishes loading; registers FIELD's walking query with the pad shim, and installs the test image's watcher under LOM_PAD_SCRIPT,
 |              so the shipping FIELD link never needs field_watch_unit.o.
 | Author: suinevere
 | Dependencies: controller_dc.c (dc_pad_set_field_walking), field_text.c (field_text_get_status), src/overlays/field_watch_unit.c (field_watch_install, test image only)
 | Globals: g_field_modal_state, g_field_dialog_screen_mode
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifdef LOM_PAD_SCRIPT
void field_watch_install(void);
#endif

typedef int (*PadDcWalkingQuery)(void);
void dc_pad_set_field_walking(PadDcWalkingQuery walking);
int field_text_get_status(int slot);
extern int g_field_modal_state;
extern int g_field_dialog_screen_mode;

/*----------------------
 | FIELD_DC_TEXT_CLOSED
 | Description: field_text_get_status's answer for a closed text window.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define FIELD_DC_TEXT_CLOSED (-1)

/*----------------------
 | field_dc_walking
 | Description: FIELD's walking query: no modal screen such as a shop, no dialog screen and both text windows closed, the gate field_poll_leader_interaction uses without its battle checks.
 | Author: suinevere
 | Dependencies: field_text_get_status
 | Globals: g_field_modal_state, g_field_dialog_screen_mode
 | Params: N/A
 | Returns: nonzero while the player is free to walk
 ----------------------*/
static int field_dc_walking(void)
{
    return g_field_modal_state == 0 && g_field_dialog_screen_mode == 0 && field_text_get_status(0) == FIELD_DC_TEXT_CLOSED &&
           field_text_get_status(1) == FIELD_DC_TEXT_CLOSED;
}

/*----------------------
 | field_watch_entry
 | Description: FIELD's Dreamcast-only extra vector slot; calls field_watch_install when built for the test image, else is a no-op.
 | Author: suinevere
 | Dependencies: field_watch_install
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
void field_watch_entry(void)
{
    dc_pad_set_field_walking(field_dc_walking);
#ifdef LOM_PAD_SCRIPT
    field_watch_install();
#endif
}
