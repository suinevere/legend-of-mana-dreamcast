;----------------------
; | halt.s
; | Description: Reset vector jumps to 0x200, which loads ACC with 0x5A then sets the HALT bit of PCON, for vmu_harness's halting smoke test.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c, pocketstation/harness/lc86k/cpu.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ACC	EQU	$100
PCON	EQU	$107

	.org $0000
	JMPF code_start

	.org $0200
code_start:
	MOV #$5A, ACC
	SET1 PCON, 0
