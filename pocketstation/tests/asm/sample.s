;----------------------
; | sample.s
; | Description: Each frame logs port 3 to RT_LOG and presents through RT_PRESENT_PORT, so a script's samples show up as the buttons the program read.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_rt/rt.inc, pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_rt/rt.inc"
P3	EQU	$14C

	.org $0000
	JMPF start

	.org $0200
start:
	LD P3
	ST RT_LOG
	MOV #$00, RT_PRESENT_PORT
	BR start
