;----------------------
; | press.s
; | Description: Halts once on Timer 0 without an idle mark, then marks idle 1 on RT_IDLE_MARK, arms every port 3 line and halts; the port 3 interrupt reads the held buttons into ACC and halts for good.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c, rt.inc
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_rt/rt.inc"
ACC	EQU	$100
PCON	EQU	$107
IE	EQU	$108
T0CNT	EQU	$110
T0PRR	EQU	$111
T0LR	EQU	$113
P3	EQU	$14C
P3INT	EQU	$14E
BTCR	EQU	$17F

	.org $0000
	JMPF start

	.org $0013
	JMPF t0l_int

	.org $004B
	JMPF p3_int

	.org $0200
start:
	MOV #$00, BTCR
	MOV #$FF, T0PRR
	MOV #$F0, T0LR
	MOV #$41, T0CNT
	MOV #$80, IE
	SET1 PCON, 0
	MOV #$00, RT_IDLE_MARK
	MOV #$FF, P3
	MOV #$05, P3INT
	SET1 PCON, 0

;----------------------
; | t0l_int
; | Description: Timer 0 low overflow: stops Timer 0 and returns past the first halt.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
t0l_int:
	MOV #$00, T0CNT
	RETI

;----------------------
; | p3_int
; | Description: Port 3 interrupt: latches the held lines into ACC and halts with the interrupt off.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
p3_int:
	LD P3
	MOV #$00, P3INT
	SET1 PCON, 0
