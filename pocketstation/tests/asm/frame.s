;----------------------
; | frame.s
; | Description: Presents two frames through RT_PRESENT_PORT, changing bank 1 RAM, WRAM and both XRAM halves between them, and logs a 0xFF data byte that must not present; then halts.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_rt/rt.inc, pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_rt/rt.inc"
PCON	EQU	$107
XBNK	EQU	$125
VRMAD1	EQU	$164
VRMAD2	EQU	$165
VTRBF	EQU	$166

	.org $0000
	JMPF start

	.org $0200
start:
	MOV #$AB, $10
	MOV #$05, VRMAD1
	MOV #$00, VRMAD2
	MOV #$CD, VTRBF
	MOV #$00, XBNK
	MOV #$EF, $180
	MOV #$00, RT_PRESENT_PORT
	MOV #$FF, RT_LOG
	MOV #$12, $11
	MOV #$01, XBNK
	MOV #$34, $185
	MOV #$00, RT_PRESENT_PORT
	SET1 PCON, 0
