/*----------------------
 | wsel_unit.c
 | Description: Compiles the decomp's unmodified WSEL overlay (wsel.c) into the Dreamcast WSEL overlay, passing wsel_main's buffers to wsel_init where the little-endian source relies on MIPS a0 still holding them.
 | Author: suinevere
 | Dependencies: src/overlays/wsel/wsel.c, lom_prefix.h (forced include)
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/

/*----------------------
 | WSEL_DC_SECOND
 | Description: Picks its second argument; wsel_init uses it to tell the argument-less call from the definition's parameter list.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: first -- ignored, second -- the argument kept
 | Returns: second
 ----------------------*/
#define WSEL_DC_SECOND(first, second, ...) second

/*----------------------
 | wsel_init
 | Description: Rewrites wsel.c's argument-less wsel_init() call and declaration to pass wsel_main's buffers; the declaration then warns that its parameter has no type.
 | Author: suinevere
 | Dependencies: WSEL_DC_SECOND
 | Globals: N/A
 | Params: ... -- the definition's parameter list, or nothing at the call and declaration
 | Returns: N/A
 ----------------------*/
#define wsel_init(...) wsel_init(WSEL_DC_SECOND(__VA_OPT__(,) __VA_ARGS__, buffers, ))

#include "overlays/wsel/wsel.c"
