/*----------------------
 | ot_walk.h
 | Description: Counts primitives in a PS1 ordering-table packet chain without drawing them.
 | Author: suinevere
 | Dependencies: lom_prefix.h (u_long), stdint.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_OT_WALK_H
#define LOM_OT_WALK_H

#include <stdint.h>

unsigned ot_walk_count(const u_long *ot_head);

#ifdef LOM_HOST_TEST

/*----------------------
 | ot_walk_set_base
 | Description: Host-test-only hook: treats a tag's 24-bit address field as an offset from base instead of Dreamcast RAM, so a chain can be built from host pointers.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: base -- host address the 24-bit field 0 corresponds to
 | Returns: N/A
 ----------------------*/
void ot_walk_set_base(uintptr_t base);

#endif

#endif
