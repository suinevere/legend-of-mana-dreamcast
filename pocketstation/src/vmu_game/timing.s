;----------------------
; | timing.s
; | Description: timing.c in LC86K assembly: the tick counters' step on a timer line and the tick's restart.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/timing.c, pocketstation/build/vmu_game/rrl_ram.inc, pocketstation/src/vmu_rt/rt_events.s (rt_tick_mode), hw.s (g_timer_irq), system.s (g_set_irq_handler)
; | Globals: ticks, irq_handler
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_timer_tick
; | Description: game_timer_tick: counts up the tick counter for a line, ticks[2] for the tick line 0x100, ticks[4] for the sound line 0x2000, else ticks[0].
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_TICKS)
; | Globals: ticks
; | Params: TRH:TRL -- line
; | Returns: N/A. Destroys ACC, VRMAD; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_timer_tick:
	MOV #<HWW_TICKS, VRMAD1
	LD TRL
	BNZ g_timer_tick_count
	LD TRH
	XOR #$01
	BNZ g_timer_tick_sound
	MOV #<(HWW_TICKS+2), VRMAD1
	BR g_timer_tick_count
g_timer_tick_sound:
	XOR #$21
	BNZ g_timer_tick_count
	MOV #<(HWW_TICKS+4), VRMAD1
g_timer_tick_count:
	MOV #>HWW_TICKS, VRMAD2
	CLR1 VSEL, 4
	INC VTRBF
	RET

;----------------------
; | g_ticks_start
; | Description: game_ticks_start: stops the tick and its interrupt, zeroes ticks[2] and ticks[3], puts handler 2 on line 8, switches the tick interrupt on and restarts the tick in mode.
; | Author: suinevere
; | Dependencies: rt_tick_mode, g_timer_irq, g_set_irq_handler, rrl_ram.inc (HWW_TICKS)
; | Globals: ticks, irq_handler
; | Params: ACC -- mode, a VMU_TICK_* running mode
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear; five bytes of stack.
;----------------------
g_ticks_start:
	PUSH ACC
	MOV #$00, ACC
	CALLF rt_tick_mode
	MOV #$01, B
	MOV #$00, ACC
	CALLF g_timer_irq
	MOV #<(HWW_TICKS+2), VRMAD1
	MOV #>(HWW_TICKS+2), VRMAD2
	SET1 VSEL, 4
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
	MOV #$02, B
	MOV #$08, ACC
	CALLF g_set_irq_handler
	MOV #$01, B
	MOV #$01, ACC
	CALLF g_timer_irq
	POP ACC
	JMPF rt_tick_mode
