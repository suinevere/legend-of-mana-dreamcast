;----------------------
; | page_return.s
; | Description: Enters the BIOS page write at ROM 0xE024 with 0x1234 pushed as its return address, a ROM place the harness does not model, so the run must stop there.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ACC	EQU	$100
EXT	EQU	$10D

	.org $0000
	JMPF start

	.org $0200
start:
	MOV #$01, $7C
	MOV #$01, $7D
	MOV #$C8, $7E
	MOV #$00, $7F
	MOV #$34, ACC
	PUSH ACC
	MOV #$12, ACC
	PUSH ACC
	NOT1 EXT, 0
	JMPF $E024
