;----------------------
; | late_tick.s
; | Description: Drives the harness's late-tick check without the runtime: sets the runtime's state byte to frame mode and its tick countdown to k, then four times writes RT_IDLE_MARK, waits with interrupts masked and reaches rt_idle_step with k in TRH:TRL:B:C, each round a little over a tick period long, so every tick after the first is delivered after its due time although its idle began before it.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_rt/sfr.inc, pocketstation/src/vmu_rt/rt.inc, pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_rt/sfr.inc"
	.include "../../src/vmu_rt/rt.inc"

;----------------------
; | LT_STATE / LT_TICK / LT_ROUNDS / LT_OUTER / LT_INNER / LT_FIRST / LT_SECOND
; | Description: Bank 1 addresses of the runtime's state byte and tick countdown the harness reads, and this program's round counter and delay counters, with the delay lengths before and after each delivery in outer loops of 256.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_rt/rt_events.s (RTV_STATE, RTV_TICK)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
LT_STATE	EQU	$14
LT_TICK	EQU	$04
LT_ROUNDS	EQU	$30
LT_OUTER	EQU	$31
LT_INNER	EQU	$32
LT_FIRST	EQU	40
LT_SECOND	EQU	21

	.org $0000
	JMPF start

	.org $0200

;----------------------
; | start
; | Description: Masks interrupts, switches to the runtime's 1 MHz clock as rt_reset does, sets frame mode on clock 5 (state 0x05) and a tick countdown of 15,626, then runs the four rounds and halts with nothing left to wake it.
; | Author: suinevere
; | Dependencies: delay, rt_idle_step
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
start:
	CLR1 IE, 7
	MOV #$81, OCR
	MOV #$80, OCR
	MOV #$64, ACC
start_wait:
	DBNZ ACC, start_wait
	MOV #$92, OCR
	MOV #$05, LT_STATE
	MOV #$0A, LT_TICK
	MOV #$3D, LT_TICK+1
	MOV #$00, LT_TICK+2
	MOV #$00, LT_TICK+3
	MOV #4, LT_ROUNDS
start_round:
	MOV #$00, RT_IDLE_MARK
	MOV #LT_FIRST, ACC
	CALLF delay
	MOV #$00, TRH
	MOV #$00, TRL
	MOV #$3D, B
	MOV #$0A, C
	CALLF rt_idle_step
	MOV #LT_SECOND, ACC
	CALLF delay
	DBNZ LT_ROUNDS, start_round
	SET1 PCON, 0

;----------------------
; | rt_idle_step
; | Description: Stands in for the runtime's label of that name, where the harness takes a tick as delivered.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRH:TRL:B:C -- k
; | Returns: N/A
;----------------------
rt_idle_step:
	NOP
	RET

;----------------------
; | delay
; | Description: Spins ACC outer loops of 256 inner ones.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- outer loops
; | Returns: N/A
;----------------------
delay:
	ST LT_OUTER
delay_outer:
	MOV #$00, LT_INNER
delay_inner:
	DBNZ LT_INNER, delay_inner
	DBNZ LT_OUTER, delay_outer
	RET
