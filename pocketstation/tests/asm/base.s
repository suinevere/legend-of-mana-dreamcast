;----------------------
; | base.s
; | Description: Runs the base timer (BIOS default BTCR, half-second interrupts) and Timer 1 low (an interrupt every 256 cycles), halting between them; after three base interrupts ACC holds the Timer 1 count.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ACC	EQU	$100
PCON	EQU	$107
IE	EQU	$108
T1CNT	EQU	$118
T1LR	EQU	$11B
BTCR	EQU	$17F

	.org $0000
	JMPF start

	.org $001B
	JMPF base_int

	.org $002B
	JMPF t1_int

	.org $0200
start:
	MOV #$00, $20
	MOV #$00, $21
	MOV #$00, T1LR
	MOV #$41, T1CNT
	MOV #$80, IE
wait:
	SET1 PCON, 0
	LD $20
	SUB #3
	BNZ wait
	MOV #$00, BTCR
	MOV #$00, T1CNT
	LD $21
	SET1 PCON, 0

;----------------------
; | base_int / t1_int
; | Description: Count base timer 0 and Timer 1 low overflows in RAM 0x20 and 0x21, clearing each overflow flag.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
base_int:
	INC $20
	CLR1 BTCR, 1
	RETI

t1_int:
	INC $21
	CLR1 T1CNT, 1
	RETI
