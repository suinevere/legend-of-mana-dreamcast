;----------------------
; | rt_events.s
; | Description: The runtime's event core: vmu_host.c's tick, sound-end and RTC countdowns in timer counts, with its constants, event order and mode carry-over. Timer 0 only measures time; rt_idle takes one event per call, as vmu_idle does.
; | Author: suinevere
; | Dependencies: sfr.inc, rt.inc, pocketstation/src/vmu/vmu_host.c (the arithmetic it ports)
; | Globals: rt_reserved (bank 1 0x04-0x1F)
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | RTV_TICK / RTV_SOUND / RTV_RTC / RTV_BASE
; | Description: 32-bit little-endian counts in rt_reserved: the tick, sound-end and RTC countdowns from the last event rt_idle took, and the counts Timer 0 has run since that event.
; | Author: suinevere
; | Dependencies: pocketstation/src/game/ram_map.txt (rt_reserved)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RTV_TICK	EQU	$04
RTV_SOUND	EQU	$08
RTV_RTC	EQU	$0C
RTV_BASE	EQU	$10

;----------------------
; | RTV_STATE / RTV_T0L
; | Description: State bits: 0-1 tick mode, 2-3 clock (0 the launcher's 7, else the mode's), 4 in the handler, 5 in standby, 6-7 the clock at the last sync. RTV_T0L is rt_ev_start's copy of T0L while it waits for a period boundary.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RTV_STATE	EQU	$14
RTV_T0L	EQU	$15

;----------------------
; | RTV_MASK / RTV_LEVELS / RTV_LATCH / RTV_HANDLER / RTV_SCRATCH
; | Description: 16-bit little-endian enabled lines, request levels, latched lines and handler address. The scratch pair holds rt_ev_sync's counter read until rt_ev_resched, and the Timer 0 interrupt's return address.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RTV_MASK	EQU	$16
RTV_LEVELS	EQU	$18
RTV_LATCH	EQU	$1A
RTV_HANDLER	EQU	$1C
RTV_SCRATCH	EQU	$1E

;----------------------
; | RT_ST_INH / RT_ST_STANDBY
; | Description: RTV_STATE bit numbers: inside the handler, and in rt_power's standby, where rt_idle takes events without the handler until an enabled button or RTC line is latched.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_ST_INH	EQU	4
RT_ST_STANDBY	EQU	5

;----------------------
; | RT_T0_STOP / RT_T0_RUN
; | Description: T0CNT values for Timer 0 as one 16-bit counter with its overflow interrupt: stopped (which loads the counters from the reloads) and running.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_T0_STOP	EQU	$24
RT_T0_RUN	EQU	$E4

;----------------------
; | rt_events_init
; | Description: Sets vmu_host_init's schedule on cleared RAM: tick off, no sound, only the RTC line enabled, the first RTC edge a clock-7 half-period away. Starts Timer 0 and sets IE bit 7, keeping IE0/IE1 set.
; | Author: suinevere
; | Dependencies: rt_ev_restart
; | Globals: rt_reserved
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R1.
;----------------------
rt_events_init:
	MOV #$00, RTV_RTC
	MOV #$40, RTV_RTC+1
	MOV #$0F, RTV_RTC+2
	MOV #$00, RTV_RTC+3
	MOV #$02, RTV_MASK+1
	CALLF rt_ev_restart
	SET1 IE, 7
	RET

;----------------------
; | rt_tick_mode
; | Description: vmu_tick_mode: a running mode from off loads a full period, from another keeps the remaining counts, and sets the clock for the next RTC half-period and the pacing. Modes above 3 change nothing.
; | Author: suinevere
; | Dependencies: rt_ev_sync, rt_ev_tick_load, rt_ev_resched
; | Globals: rt_reserved
; | Params: ACC -- RT_TICK_*
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R1.
;----------------------
rt_tick_mode:
	PUSH IE
	CLR1 IE, 7
	PUSH ACC
	CALLF rt_ev_sync
	POP C
	LD C
	AND #$FC
	BNZ rt_tick_mode_out
	LD C
	BZ rt_tick_mode_set
	LD RTV_STATE
	AND #$03
	BNZ rt_tick_mode_clock
	LD RTV_STATE
	AND #$FC
	OR C
	ST RTV_STATE
	CALLF rt_ev_tick_load
rt_tick_mode_clock:
	LD C
	ROL
	ROL
	ST B
	LD RTV_STATE
	AND #$F3
	OR B
	ST RTV_STATE
rt_tick_mode_set:
	LD RTV_STATE
	AND #$FC
	OR C
	ST RTV_STATE
rt_tick_mode_out:
	CALLF rt_ev_resched
	POP IE
	RET

;----------------------
; | rt_sound_play
; | Description: vmu_sound_play: the sound end falls (samples + 1) * (counts_per_sample + 1) counts after the last event, 501 a sample slow and 72 fast. The product keeps 32 bits, as the host's uint32_t does.
; | Author: suinevere
; | Dependencies: rt_ev_sync, rt_ev_resched
; | Globals: rt_reserved
; | Params: TRH:TRL:B -- samples (24-bit, B low); C -- RT_SOUND_SLOW or nonzero for fast
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R1.
;----------------------
rt_sound_play:
	PUSH IE
	CLR1 IE, 7
	PUSH TRH
	PUSH TRL
	PUSH B
	PUSH C
	CALLF rt_ev_sync
	POP C
	POP B
	POP TRL
	POP TRH
	LD B
	ADD #$01
	ST RTV_SOUND
	LD TRL
	ADDC #$00
	ST RTV_SOUND+1
	LD TRH
	ADDC #$00
	ST RTV_SOUND+2
	MOV #$00, ACC
	ADDC #$00
	ST RTV_SOUND+3
	LD C
	PUSH ACC
	BNZ rt_sound_play_fast
	MOV #$F5, ACC
	BR rt_sound_play_mul
rt_sound_play_fast:
	MOV #$48, ACC
rt_sound_play_mul:
	PUSH ACC
	ST B
	LD RTV_SOUND+2
	ST C
	LD RTV_SOUND+3
	MUL
	ST TRH
	LD C
	ST TRL
	POP B
	LD RTV_SOUND
	ST C
	LD RTV_SOUND+1
	MUL
	PUSH ACC
	LD B
	ADD TRL
	ST B
	LD TRH
	ADDC #$00
	ST TRH
	POP TRL
	POP ACC
	BNZ rt_sound_play_store
	LD TRL
	ADD RTV_SOUND
	ST TRL
	LD B
	ADDC RTV_SOUND+1
	ST B
	LD TRH
	ADDC RTV_SOUND+2
	ST TRH
rt_sound_play_store:
	LD C
	ST RTV_SOUND
	LD TRL
	ST RTV_SOUND+1
	LD B
	ST RTV_SOUND+2
	LD TRH
	ST RTV_SOUND+3
	CALLF rt_ev_resched
	POP IE
	RET

;----------------------
; | rt_sound_stop
; | Description: vmu_sound_stop: cancels the sound end and acknowledges its line.
; | Author: suinevere
; | Dependencies: rt_ev_sync, rt_ev_resched
; | Globals: rt_reserved
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R1.
;----------------------
rt_sound_stop:
	PUSH IE
	CLR1 IE, 7
	CALLF rt_ev_sync
	MOV #$00, RTV_SOUND
	MOV #$00, RTV_SOUND+1
	MOV #$00, RTV_SOUND+2
	MOV #$00, RTV_SOUND+3
	CLR1 RTV_LEVELS+1, 5
	CLR1 RTV_LATCH+1, 5
	CALLF rt_ev_resched
	POP IE
	RET

;----------------------
; | rt_irq_enable / rt_irq_disable
; | Description: vmu_irq_enable and vmu_irq_disable: set the given lines in the mask, kept to the five lines, or clear them.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RTV_MASK
; | Params: TRH:TRL -- RT_LINE_* bits
; | Returns: N/A. Destroys ACC.
;----------------------
rt_irq_enable:
	PUSH IE
	CLR1 IE, 7
	LD TRL
	AND #$1F
	OR RTV_MASK
	ST RTV_MASK
	LD TRH
	AND #$27
	OR RTV_MASK+1
	ST RTV_MASK+1
	POP IE
	RET

rt_irq_disable:
	PUSH IE
	CLR1 IE, 7
	LD TRL
	XOR #$FF
	AND RTV_MASK
	ST RTV_MASK
	LD TRH
	XOR #$FF
	AND RTV_MASK+1
	ST RTV_MASK+1
	POP IE
	RET

;----------------------
; | rt_irq_ack
; | Description: vmu_irq_ack: clears the given lines' latches and the request levels of all but the level lines, buttons, RTC and dock.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RTV_LEVELS, RTV_LATCH
; | Params: TRH:TRL -- RT_LINE_* bits
; | Returns: N/A. Destroys ACC.
;----------------------
rt_irq_ack:
	PUSH IE
	CLR1 IE, 7
	LD TRL
	XOR #$FF
	OR #$1F
	AND RTV_LEVELS
	ST RTV_LEVELS
	LD TRH
	XOR #$FF
	OR #$06
	AND RTV_LEVELS+1
	ST RTV_LEVELS+1
	LD TRL
	XOR #$FF
	AND RTV_LATCH
	ST RTV_LATCH
	LD TRH
	XOR #$FF
	AND RTV_LATCH+1
	ST RTV_LATCH+1
	POP IE
	RET

;----------------------
; | rt_irq_pending / rt_irq_levels / rt_irq_mask
; | Description: vmu_irq_pending, vmu_irq_levels and vmu_irq_mask: the latched lines, the request levels and the enabled lines.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RTV_LATCH, RTV_LEVELS, RTV_MASK
; | Params: N/A
; | Returns: TRH:TRL -- RT_LINE_* bits. Destroys ACC; keeps B and C.
;----------------------
rt_irq_pending:
	PUSH IE
	CLR1 IE, 7
	LD RTV_LATCH
	ST TRL
	LD RTV_LATCH+1
	ST TRH
	POP IE
	RET

rt_irq_levels:
	PUSH IE
	CLR1 IE, 7
	LD RTV_LEVELS
	ST TRL
	LD RTV_LEVELS+1
	ST TRH
	POP IE
	RET

rt_irq_mask:
	PUSH IE
	CLR1 IE, 7
	LD RTV_MASK
	ST TRL
	LD RTV_MASK+1
	ST TRH
	POP IE
	RET

;----------------------
; | rt_set_handler
; | Description: vmu_set_handler: installs the routine rt_idle calls with interrupts masked; 0 acknowledges every line instead.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RTV_HANDLER
; | Params: TRH:TRL -- code address, or 0
; | Returns: N/A. Destroys ACC.
;----------------------
rt_set_handler:
	PUSH IE
	CLR1 IE, 7
	LD TRL
	ST RTV_HANDLER
	LD TRH
	ST RTV_HANDLER+1
	POP IE
	RET

;----------------------
; | rt_idle
; | Description: vmu_idle: takes exactly one event, halting until Timer 0 has run to it, or while asleep, polling the buttons at every wake, then polls the buttons and dock and calls the handler, interrupts masked, while an enabled line is latched. Like take(), each call first acknowledges the RTC, even for a sound end alone. With RT_ST_STANDBY set (rt_power) it calls no handler and takes events until an enabled button or RTC line is latched, then clears the bit.
; | Author: suinevere
; | Dependencies: rt_in_sleep, rt_in_poll, rt_in_dock, rt_ev_sync, rt_ev_kmin, rt_ev_resched, rt_ev_step, the interrupts through rt_isr_out (which wake the halt)
; | Globals: rt_reserved, RT_IDLE_MARK
; | Params: N/A (inside the handler it only takes the event; the handler acknowledges the other lines, the game's the sound line after its dispatch as system.c does)
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R1 and whatever the handler does; IE is restored.
;----------------------
rt_idle:
	PUSH IE
	CLR1 IE, 7
	MOV #$00, RT_IDLE_MARK
rt_idle_check:
	CALLF rt_in_sleep
	BNZ rt_idle_wait
	CALLF rt_ev_kmin
	LD RTV_BASE
	SUB C
	LD RTV_BASE+1
	SUBC B
	LD RTV_BASE+2
	SUBC TRL
	LD RTV_BASE+3
	SUBC TRH
	BN PSW, 7, rt_idle_step
rt_idle_wait:
	SET1 IE, 7
rt_idle_halt:
	SET1 PCON, 0
rt_idle_woke:
	CLR1 IE, 7
	CALLF rt_in_poll
	BR rt_idle_check
rt_idle_step:
	CALLF rt_in_poll
	CALLF rt_in_dock
	CALLF rt_ev_sync
	CALLF rt_ev_kmin
	CALLF rt_ev_step
	CALLF rt_ev_resched
	BP RTV_STATE, RT_ST_INH, rt_idle_out
	BP RTV_STATE, RT_ST_STANDBY, rt_idle_standby
rt_idle_deliver:
	LD RTV_LATCH
	AND RTV_MASK
	AND #$1F
	ST C
	LD RTV_LATCH+1
	AND RTV_MASK+1
	AND #$27
	OR C
	BZ rt_idle_out
	SET1 RTV_STATE, RT_ST_INH
	CLR1 RTV_LATCH+1, 1
	LD RTV_HANDLER
	OR RTV_HANDLER+1
	BZ rt_idle_none
	MOV #<rt_idle_back, ACC
	PUSH ACC
	MOV #>rt_idle_back, ACC
	PUSH ACC
	PUSH RTV_HANDLER
	PUSH RTV_HANDLER+1
	RET
rt_idle_none:
	LD RTV_LEVELS+1
	AND #$DE
	ST RTV_LEVELS+1
	LD RTV_LATCH
	AND #$E0
	ST RTV_LATCH
	LD RTV_LATCH+1
	AND #$D8
	ST RTV_LATCH+1
rt_idle_back:
	CLR1 RTV_STATE, RT_ST_INH
	BR rt_idle_deliver
rt_idle_standby:
	LD RTV_LATCH
	AND RTV_MASK
	AND #$1F
	ST C
	LD RTV_LATCH+1
	AND RTV_MASK+1
	AND #$02
	OR C
	BNZ rt_idle_woken
	JMPF rt_idle_check
rt_idle_woken:
	CLR1 RTV_STATE, RT_ST_STANDBY
rt_idle_out:
	POP IE
	RET

;----------------------
; | rt_t0h_isr / rt_isr_out
; | Description: Timer 0 overflow: adds the segment's counts to RTV_BASE and restarts Timer 0; it never steps an event. rt_isr_out, every interrupt's exit, returns past rt_idle's halt when it interrupted it, so the check runs again.
; | Author: suinevere
; | Dependencies: rt_ev_done, rt_idle_halt, rt_idle_woke
; | Globals: rt_reserved, RTV_SCRATCH (its return address)
; | Params: N/A (the interrupted code is on RAM bank 1, as all runtime code is; rt_isr_out expects the return address in RTV_SCRATCH and PSW, ACC pushed)
; | Returns: N/A (RETI). Keeps every register.
;----------------------
rt_t0h_isr:
	POP RTV_SCRATCH+1
	POP RTV_SCRATCH
	PUSH RTV_SCRATCH
	PUSH RTV_SCRATCH+1
	PUSH PSW
	PUSH ACC
	MOV #$02, PSW
	BPC T0CNT, 3, rt_t0h_isr_run
	BR rt_isr_out
rt_t0h_isr_run:
	PUSH B
	PUSH C
	PUSH TRL
	PUSH TRH
	PUSH $01
	CALLF rt_ev_done
	POP $01
	POP TRH
	POP TRL
	POP C
	POP B
rt_isr_out:
	LD RTV_SCRATCH
	BNE #<rt_idle_halt, rt_isr_ret
	LD RTV_SCRATCH+1
	BNE #>rt_idle_halt, rt_isr_ret
	POP RTV_SCRATCH
	POP RTV_SCRATCH+1
	POP ACC
	POP ACC
	MOV #<rt_idle_woke, ACC
	PUSH ACC
	MOV #>rt_idle_woke, ACC
	PUSH ACC
	PUSH RTV_SCRATCH+1
	PUSH RTV_SCRATCH
rt_isr_ret:
	POP ACC
	POP PSW
	RETI

;----------------------
; | rt_ev_done
; | Description: The segment Timer 0 was started for has run: adds its periods' counts to RTV_BASE and restarts Timer 0 for the next one, crediting the periods counted since the overflow.
; | Author: suinevere
; | Dependencies: rt_ev_kmin, rt_ev_seg, rt_ev_periods, rt_ev_credit, rt_ev_next, rt_ev_start
; | Globals: rt_reserved
; | Params: N/A (interrupts masked)
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R1.
;----------------------
rt_ev_done:
	CALLF rt_ev_kmin
	CALLF rt_ev_seg
	CALLF rt_ev_periods
	PUSH C
	PUSH B
	CALLF rt_ev_credit
	CALLF rt_ev_next
	POP TRH
	POP TRL
	JMPF rt_ev_start

;----------------------
; | rt_ev_sync
; | Description: Before a schedule change: finishes a segment whose overflow is pending, clearing its flag first as the ISR does so rt_ev_start does not count it again, then adds the counts Timer 0 has run into the current one to RTV_BASE. Keeps the counter and clock for rt_ev_resched.
; | Author: suinevere
; | Dependencies: rt_ev_done, rt_ev_kmin, rt_ev_seg, rt_ev_periods, rt_ev_count, rt_ev_credit
; | Globals: rt_reserved, RTV_SCRATCH (the counter read)
; | Params: N/A (interrupts masked)
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R1.
;----------------------
rt_ev_sync:
	BN T0CNT, 3, rt_ev_sync_part
	CLR1 T0CNT, 3
	CALLF rt_ev_done
rt_ev_sync_part:
	CALLF rt_ev_kmin
	CALLF rt_ev_seg
	CALLF rt_ev_periods
	CALLF rt_ev_count
	LD TRL
	ST RTV_SCRATCH
	ADD C
	ST C
	LD TRH
	ST RTV_SCRATCH+1
	ADDC B
	ST B
	CALLF rt_ev_credit
	LD RTV_STATE
	AND #$0C
	ROL
	ROL
	ROL
	ROL
	ST B
	LD RTV_STATE
	AND #$3F
	OR B
	ST RTV_STATE
	RET

;----------------------
; | rt_ev_resched / rt_ev_restart
; | Description: After a change: restarts Timer 0 for the segment toward the next event. rt_ev_resched credits the periods since rt_ev_sync's read, through an overflow since then (the reload's periods), converted from that clock's to the new one's; rt_ev_restart credits none and drops a pending overflow.
; | Author: suinevere
; | Dependencies: rt_ev_next, rt_ev_count, rt_ev_p, rt_ev_start
; | Globals: rt_reserved, RTV_SCRATCH
; | Params: N/A (interrupts masked)
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R1.
;----------------------
rt_ev_resched:
	CALLF rt_ev_next
	PUSH B
	PUSH C
	CALLF rt_ev_count
	PUSH TRL
	PUSH TRH
	LD TRL
	SUB RTV_SCRATCH
	ST TRL
	LD TRH
	SUBC RTV_SCRATCH+1
	BN T0CNT, 3, rt_ev_resched_hi
	ST TRH
	LD TRL
	SUB T0LR
	ST TRL
	LD TRH
	SUBC T0HR
	CLR1 T0CNT, 3
rt_ev_resched_hi:
	BZ rt_ev_resched_d
	MOV #$FF, TRL
rt_ev_resched_d:
	LD RTV_STATE
	ROL
	ROL
	AND #$03
	CALLF rt_ev_p
	ST B
	LD TRL
	ST C
	MOV #$00, ACC
	MUL
	PUSH ACC
	LD RTV_STATE
	ROR
	ROR
	AND #$03
	CALLF rt_ev_p
	ST B
	POP ACC
	DIV
	ST TRH
	POP B
	LD C
	POP C
	SUB C
	ST TRL
	LD TRH
	SUBC B
	ST TRH
	POP C
	POP B
	JMPF rt_ev_start

rt_ev_restart:
	CALLF rt_ev_next
	CLR1 T0CNT, 3
	CALLF rt_ev_count
	LD TRL
	XOR #$FF
	ADD #$01
	ST TRL
	LD TRH
	XOR #$FF
	ADDC #$00
	ST TRH
	JMPF rt_ev_start

;----------------------
; | rt_ev_next
; | Description: The next segment toward the nearest event, in Timer 0 periods.
; | Author: suinevere
; | Dependencies: rt_ev_kmin, rt_ev_seg, rt_ev_periods
; | Globals: rt_reserved
; | Params: N/A
; | Returns: B:C -- periods, 1 or more. Destroys ACC, TRL, TRH, R1.
;----------------------
rt_ev_next:
	CALLF rt_ev_kmin
	CALLF rt_ev_seg
	JMPF rt_ev_periods

;----------------------
; | rt_ev_kmin
; | Description: advance_to_event's k: the RTC countdown, the tick's if a mode runs and the sound's if one plays, whichever is smallest.
; | Author: suinevere
; | Dependencies: rt_ev_kmin_try
; | Globals: rt_reserved
; | Params: N/A
; | Returns: TRH:TRL:B:C -- k (C low). Destroys ACC, R1.
;----------------------
rt_ev_kmin:
	LD RTV_RTC
	ST C
	LD RTV_RTC+1
	ST B
	LD RTV_RTC+2
	ST TRL
	LD RTV_RTC+3
	ST TRH
	LD RTV_STATE
	AND #$03
	BZ rt_ev_kmin_sound
	MOV #RTV_TICK, $01
	CALLF rt_ev_kmin_try
rt_ev_kmin_sound:
	LD RTV_SOUND
	OR RTV_SOUND+1
	OR RTV_SOUND+2
	OR RTV_SOUND+3
	BZ rt_ev_kmin_ret
	MOV #RTV_SOUND, $01
	CALLF rt_ev_kmin_try
rt_ev_kmin_ret:
	RET

;----------------------
; | rt_ev_kmin_try
; | Description: Replaces k with the countdown at R1 if that is smaller.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: rt_reserved
; | Params: R1 -- countdown address; TRH:TRL:B:C -- k
; | Returns: TRH:TRL:B:C -- the smaller. Destroys ACC, R1.
;----------------------
rt_ev_kmin_try:
	LD @R1
	SUB C
	INC $01
	LD @R1
	SUBC B
	INC $01
	LD @R1
	SUBC TRL
	INC $01
	LD @R1
	SUBC TRH
	BN PSW, 7, rt_ev_kmin_try_ret
	LD @R1
	ST TRH
	DEC $01
	LD @R1
	ST TRL
	DEC $01
	LD @R1
	ST B
	DEC $01
	LD @R1
	ST C
rt_ev_kmin_try_ret:
	RET

;----------------------
; | rt_ev_step
; | Description: advance_to_event's firing for k counts: the RTC first (an edge at zero), then the tick (reload and latch), then the sound end (latch once). Takes k from RTV_BASE.
; | Author: suinevere
; | Dependencies: rt_ev_sub, rt_ev_rtc_edge, rt_ev_tick_load
; | Globals: rt_reserved
; | Params: TRH:TRL:B:C -- k
; | Returns: N/A. Keeps TRH, TRL, B, C; destroys ACC, R1.
;----------------------
rt_ev_step:
	MOV #RTV_RTC, $01
	CALLF rt_ev_sub
	BNZ rt_ev_step_tick
	CALLF rt_ev_rtc_edge
rt_ev_step_tick:
	LD RTV_STATE
	AND #$03
	BZ rt_ev_step_sound
	MOV #RTV_TICK, $01
	CALLF rt_ev_sub
	BNZ rt_ev_step_sound
	CALLF rt_ev_tick_load
	SET1 RTV_LATCH+1, 0
rt_ev_step_sound:
	LD RTV_SOUND
	OR RTV_SOUND+1
	OR RTV_SOUND+2
	OR RTV_SOUND+3
	BZ rt_ev_step_base
	MOV #RTV_SOUND, $01
	CALLF rt_ev_sub
	BNZ rt_ev_step_base
	SET1 RTV_LATCH+1, 5
rt_ev_step_base:
	MOV #RTV_BASE, $01
	JMPF rt_ev_sub

;----------------------
; | rt_ev_sub
; | Description: Subtracts k from the 32-bit count at R1.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: rt_reserved
; | Params: R1 -- count address; TRH:TRL:B:C -- k
; | Returns: ACC -- zero when the result is zero. Destroys R1.
;----------------------
rt_ev_sub:
	LD @R1
	SUB C
	ST @R1
	INC $01
	LD @R1
	SUBC B
	ST @R1
	INC $01
	LD @R1
	SUBC TRL
	ST @R1
	INC $01
	LD @R1
	SUBC TRH
	ST @R1
	DEC $01
	OR @R1
	DEC $01
	OR @R1
	DEC $01
	OR @R1
	RET

;----------------------
; | rt_ev_tick_load
; | Description: Loads the tick countdown with a full period of the running mode: TICK_RELOAD + 1 counts, 15,626 for frame and fast and 1,954 for standby.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RTV_TICK, RTV_STATE
; | Params: N/A
; | Returns: N/A. Destroys ACC.
;----------------------
rt_ev_tick_load:
	MOV #$00, RTV_TICK+2
	MOV #$00, RTV_TICK+3
	LD RTV_STATE
	AND #$03
	BE #$03, rt_ev_tick_load_standby
	MOV #$0A, RTV_TICK
	MOV #$3D, RTV_TICK+1
	RET
rt_ev_tick_load_standby:
	MOV #$A2, RTV_TICK
	MOV #$07, RTV_TICK+1
	RET

;----------------------
; | rt_ev_rtc_edge
; | Description: rtc_edge: the next half-period is the current clock's (clock_hz / 4 counts: 999,424 at 7, 253,952 at 5, 499,712 at 6, 31,744 at 2), and a rising edge requests the RTC line.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RTV_RTC, RTV_STATE, RTV_LEVELS, RTV_MASK, RTV_LATCH
; | Params: N/A
; | Returns: N/A. Destroys ACC.
;----------------------
rt_ev_rtc_edge:
	MOV #$00, RTV_RTC
	MOV #$00, RTV_RTC+3
	LD RTV_STATE
	AND #$0C
	BNZ rt_ev_rtc_edge_5
	MOV #$40, RTV_RTC+1
	MOV #$0F, RTV_RTC+2
	BR rt_ev_rtc_edge_level
rt_ev_rtc_edge_5:
	BNE #$04, rt_ev_rtc_edge_6
	MOV #$E0, RTV_RTC+1
	MOV #$03, RTV_RTC+2
	BR rt_ev_rtc_edge_level
rt_ev_rtc_edge_6:
	BNE #$08, rt_ev_rtc_edge_2
	MOV #$A0, RTV_RTC+1
	MOV #$07, RTV_RTC+2
	BR rt_ev_rtc_edge_level
rt_ev_rtc_edge_2:
	MOV #$7C, RTV_RTC+1
	MOV #$00, RTV_RTC+2
rt_ev_rtc_edge_level:
	BPC RTV_LEVELS+1, 1, rt_ev_rtc_edge_ret
	SET1 RTV_LEVELS+1, 1
	BN RTV_MASK+1, 1, rt_ev_rtc_edge_ret
	SET1 RTV_LATCH+1, 1
rt_ev_rtc_edge_ret:
	RET

;----------------------
; | rt_ev_seg
; | Description: The next segment: k less the counts already run, at most 0x0F0000 counts so its periods fit Timer 0's 16 bits. Once k is reached it is the most, to keep counting until rt_idle takes the event.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RTV_BASE
; | Params: TRH:TRL:B:C -- k
; | Returns: TRH:TRL:B:C -- segment counts (TRH 0). Destroys ACC.
;----------------------
rt_ev_seg:
	LD C
	SUB RTV_BASE
	ST C
	LD B
	SUBC RTV_BASE+1
	ST B
	LD TRL
	SUBC RTV_BASE+2
	ST TRL
	LD TRH
	SUBC RTV_BASE+3
	ST TRH
	BP PSW, 7, rt_ev_seg_cap
	BNZ rt_ev_seg_cap
	LD C
	OR B
	OR TRL
	BZ rt_ev_seg_cap
	LD TRL
	SUB #$0F
	BP PSW, 7, rt_ev_seg_ret
rt_ev_seg_cap:
	MOV #$00, C
	MOV #$00, B
	MOV #$0F, TRL
	MOV #$00, TRH
rt_ev_seg_ret:
	RET

;----------------------
; | rt_ev_periods
; | Description: Counts to Timer 0 periods, rounded up and at least 1: a period is 32 counts (16 on clock 2), within 0.06% at every clock (see rt_ev_p).
; | Author: suinevere
; | Dependencies: rt_ev_shr
; | Globals: RTV_STATE
; | Params: TRL:B:C -- counts, below 0x0F0000
; | Returns: B:C -- periods. Destroys ACC, TRL.
;----------------------
rt_ev_periods:
	LD RTV_STATE
	AND #$0C
	BE #$0C, rt_ev_periods_16
	LD C
	ADD #$1F
	ST C
	LD B
	ADDC #$00
	ST B
	LD TRL
	ADDC #$00
	ST TRL
	CALLF rt_ev_shr
	BR rt_ev_periods_shift
rt_ev_periods_16:
	LD C
	ADD #$0F
	ST C
	LD B
	ADDC #$00
	ST B
	LD TRL
	ADDC #$00
	ST TRL
rt_ev_periods_shift:
	CALLF rt_ev_shr
	CALLF rt_ev_shr
	CALLF rt_ev_shr
	CALLF rt_ev_shr
	LD C
	OR B
	BNZ rt_ev_periods_ret
	MOV #$01, C
rt_ev_periods_ret:
	RET

;----------------------
; | rt_ev_shr / rt_ev_shl
; | Description: Halves TRL:B:C, or doubles TRH:TRL:B:C.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRL:B:C, or TRH:TRL:B:C
; | Returns: the same registers. Destroys ACC.
;----------------------
rt_ev_shr:
	CLR1 PSW, 7
	LD TRL
	RORC
	ST TRL
	LD B
	RORC
	ST B
	LD C
	RORC
	ST C
	RET

rt_ev_shl:
	CLR1 PSW, 7
	LD C
	ROLC
	ST C
	LD B
	ROLC
	ST B
	LD TRL
	ROLC
	ST TRL
	LD TRH
	ROLC
	ST TRH
	RET

;----------------------
; | rt_ev_credit
; | Description: Adds whole Timer 0 periods to RTV_BASE as counts, 32 a period (16 on clock 2), so RTV_BASE follows the time that has run.
; | Author: suinevere
; | Dependencies: rt_ev_shl, rt_ev_addb
; | Globals: RTV_BASE, RTV_STATE
; | Params: B:C -- periods
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH.
;----------------------
rt_ev_credit:
	MOV #$00, TRL
	MOV #$00, TRH
	LD RTV_STATE
	AND #$0C
	BE #$0C, rt_ev_credit_16
	CALLF rt_ev_shl
rt_ev_credit_16:
	CALLF rt_ev_shl
	CALLF rt_ev_shl
	CALLF rt_ev_shl
	CALLF rt_ev_shl
	JMPF rt_ev_addb

;----------------------
; | rt_ev_addb
; | Description: Adds counts to RTV_BASE.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RTV_BASE
; | Params: TRH:TRL:B:C -- counts
; | Returns: N/A. Keeps TRH, TRL, B, C; destroys ACC.
;----------------------
rt_ev_addb:
	LD RTV_BASE
	ADD C
	ST RTV_BASE
	LD RTV_BASE+1
	ADDC B
	ST RTV_BASE+1
	LD RTV_BASE+2
	ADDC TRL
	ST RTV_BASE+2
	LD RTV_BASE+3
	ADDC TRH
	ST RTV_BASE+3
	RET

;----------------------
; | rt_ev_count
; | Description: Reads Timer 0's running 16-bit counter, again if the high byte moved during the read.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: TRH:TRL -- the counter. Keeps B and C; destroys ACC.
;----------------------
rt_ev_count:
	LD T0H
	ST TRH
	LD T0L
	ST TRL
	LD T0H
	BNE TRH, rt_ev_count
	RET

;----------------------
; | rt_ev_p
; | Description: A clock's Timer 0 period in 1 MHz cycles, chosen so a period is a power of two of that clock's counts, a count being 2 / clock_hz. 16 at 7 (31.98 counts), 63 at 5 (32.00), 32 at 6 (31.98), 252 at 2 (16.00).
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- clock index 0-3 (RTV_STATE bits 2-3)
; | Returns: ACC -- cycles
;----------------------
rt_ev_p:
	BNZ rt_ev_p_5
	MOV #16, ACC
	RET
rt_ev_p_5:
	BNE #$01, rt_ev_p_6
	MOV #63, ACC
	RET
rt_ev_p_6:
	BNE #$02, rt_ev_p_2
	MOV #32, ACC
	RET
rt_ev_p_2:
	MOV #252, ACC
	RET

;----------------------
; | rt_ev_start
; | Description: Restarts Timer 0 at the current clock's prescale to overflow after the given periods less those already run. An overflow not yet taken, at entry or before the boundary wait, adds the reload's periods to those run, so each overflow counts once (rt_ev_sync and the ISR clear the flag of the one rt_ev_done credits). A running timer is restarted just after a period boundary, so the stop loses no period; when that boundary is an overflow, the counter counts as 0x10000. When the periods run reach the segment, it credits the segment to RTV_BASE and starts the next with the rest run, so no count is lost.
; | Author: suinevere
; | Dependencies: rt_ev_p, rt_ev_credit, rt_ev_next
; | Globals: RTV_STATE, RTV_T0L, RTV_BASE
; | Params: B:C -- periods; TRH:TRL -- periods already run less the running counter
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R1.
;----------------------
rt_ev_start:
	BN T0CNT, 3, rt_ev_start_go
	LD TRL
	SUB T0LR
	ST TRL
	LD TRH
	SUBC T0HR
	ST TRH
	CLR1 T0CNT, 3
rt_ev_start_go:
	LD RTV_STATE
	ROR
	ROR
	AND #$03
	CALLF rt_ev_p
	XOR #$FF
	ADD #$01
	ST T0PRR
	LD TRL
	SUB C
	ST TRL
	LD TRH
	SUBC B
	ST TRH
	LD C
	XOR #$FF
	ADD #$01
	ST C
	LD B
	XOR #$FF
	ADDC #$00
	ST B
	LD T0L
	BN T0CNT, 7, rt_ev_start_load
	ST RTV_T0L
	BN T0CNT, 3, rt_ev_start_wait
	LD TRL
	SUB T0LR
	ST TRL
	LD TRH
	SUBC T0HR
	ST TRH
	CLR1 T0CNT, 3
rt_ev_start_wait:
	BP T0CNT, 3, rt_ev_start_wrap
	LD T0L
	BE RTV_T0L, rt_ev_start_wait
	BP T0CNT, 3, rt_ev_start_wrap
rt_ev_start_load:
	ADD TRL
	ST TRL
	ST T0LR
	LD T0H
	ADDC TRH
	BR rt_ev_start_high
rt_ev_start_wrap:
	LD TRL
	ST T0LR
	LD TRH
rt_ev_start_high:
	ST TRH
	ST T0HR
	MOV #RT_T0_STOP, T0CNT
	MOV #RT_T0_RUN, T0CNT
	LD C
	ST T0LR
	LD B
	ST T0HR
	LD TRH
	SUB #$10
	BN PSW, 7, rt_ev_start_ret
	LD C
	XOR #$FF
	ADD #$01
	ST C
	LD B
	XOR #$FF
	ADDC #$00
	ST B
	CALLF rt_ev_credit
	CALLF rt_ev_next
	MOV #$00, TRL
	MOV #$00, TRH
	JMPF rt_ev_start
rt_ev_start_ret:
	RET
