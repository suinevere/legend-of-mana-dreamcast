;----------------------
; | page_ie.s
; | Description: Enters the BIOS page write at ROM 0xE024 with IE bit 7 set, which the Maple path never does, so the run must stop there.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ACC	EQU	$100
EXT	EQU	$10D
IE	EQU	$108

	.org $0000
	JMPF start

	.org $0200
start:
	SET1 IE, 7
	MOV #$01, $7C
	MOV #$01, $7D
	MOV #$C8, $7E
	MOV #$00, $7F
	MOV #$03, ACC
	PUSH ACC
	MOV #$01, ACC
	PUSH ACC
	NOT1 EXT, 0
	JMPF $E024
