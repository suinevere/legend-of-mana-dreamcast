;----------------------
; | log.s
; | Description: Writes 1, 2 and 3 to the RT_LOG debug port, then halts with nothing left to wake it.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_rt/rt.inc, pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_rt/rt.inc"
PCON	EQU	$107

	.org $0000
	JMPF start

	.org $0200
start:
	MOV #1, RT_LOG
	MOV #2, RT_LOG
	MOV #3, RT_LOG
	SET1 PCON, 0
