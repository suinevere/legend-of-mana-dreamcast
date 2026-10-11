/*----------------------
 | spadstk.h
 | Description: Shim for decomp files that include <spadstk.h>; the real SDK header's SetSpadStack/ResetSpadStack are inline MIPS asm with no SH-2 equivalent, so this makes them no-ops around the same calls.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_PSYQ_SPADSTK_H
#define LOM_PSYQ_SPADSTK_H

/*----------------------
 | SetSpadStack
 | Description: No-op on Saturn; the PS1 macro moves SP into scratchpad RAM as a speed trick with no behavioural effect on the calls it brackets.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: save_slot -- PS1 save-slot address, unused
 | Returns: N/A
 ----------------------*/
#define SetSpadStack(save_slot) ((void)(save_slot))

/*----------------------
 | ResetSpadStack
 | Description: No-op on Saturn, pairing with SetSpadStack.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define ResetSpadStack() ((void)0)

#endif
