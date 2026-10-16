/*----------------------
 | field_watch_entry.c
 | Description: Always-compiled FIELD overlay entry main's loader calls after FIELD finishes loading; installs the test image's watcher under LOM_PAD_SCRIPT, otherwise does nothing, so the shipping FIELD link never needs field_watch_unit.o.
 | Author: suinevere
 | Dependencies: src/overlays/field_watch_unit.c (field_watch_install, test image only)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifdef LOM_PAD_SCRIPT
void field_watch_install(void);
#endif

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
#ifdef LOM_PAD_SCRIPT
    field_watch_install();
#endif
}
