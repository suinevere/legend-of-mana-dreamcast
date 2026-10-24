;----------------------
; | timer.s
; | Description: Starts Timer 0 low with the slowest prescale (65,536 cycles to overflow) and halts; its interrupt stops the timer, sets ACC to 0x77 and halts for good.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ACC	EQU	$100
PCON	EQU	$107
IE	EQU	$108
T0CNT	EQU	$110
T0PRR	EQU	$111
T0LR	EQU	$113
BTCR	EQU	$17F

	.org $0000
	JMPF start

	.org $0013
	JMPF t0l_int

	.org $0200
start:
	MOV #$00, BTCR
	MOV #$00, T0PRR
	MOV #$00, T0LR
	MOV #$41, T0CNT
	MOV #$80, IE
	SET1 PCON, 0

;----------------------
; | t0l_int
; | Description: Timer 0 low overflow: stops Timer 0, sets ACC to 0x77 and halts.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
t0l_int:
	MOV #$00, T0CNT
	MOV #$77, ACC
	SET1 PCON, 0
