;----------------------
; | rom_call.s
; | Description: Calls the BIOS SLEEP_EX entry at ROM 0x0140, which the harness does not model, so the run must stop naming it.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
EXT	EQU	$10D

	.org $0000
	JMPF start

	.org $0200
start:
	NOT1 EXT, 0
	JMPF $0140
