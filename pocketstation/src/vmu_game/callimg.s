;----------------------
; | callimg.s
; | Description: The --call test image for vmu_harness --call and call_compare.py: the runtime, every game routine ported so far and the tables the GAME keeps. The Makefile adds R2's pet save in callimg.bin.
; | Author: suinevere
; | Dependencies: callbase.inc, routines.inc, pocketstation/build/vmu_game/tables.s, pocketstation/src/vmu_game/Makefile
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "callbase.inc"
	.include "routines.inc"
	.cnop 0, $04
	.include "../../build/vmu_game/tables.s"
