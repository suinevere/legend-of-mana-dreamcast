;----------------------
; | rt_input.s
; | Description: The runtime's input: the buttons on their lines, Mode's quit request, a Sleep the game never sees, and the dock line following port 7, as vmu_host.c models them.
; | Author: suinevere
; | Dependencies: sfr.inc, rt.inc, rt_events.s (RTV_*, rt_ev_sync, rt_ev_restart, rt_isr_out), pocketstation/src/vmu/vmu_host.c
; | Globals: rt_input (bank 1 0xF9-0xFB), RTV_LEVELS, RTV_MASK, RTV_LATCH
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | RTI_HELD / RTI_IGNORE / RTI_FLAGS
; | Description: rt_input bytes: the port 3 lines held at the last poll (active-high), the held lines read as released until let go, and the flag bits.
; | Author: suinevere
; | Dependencies: pocketstation/src/game/ram_map.txt (rt_input)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RTI_HELD	EQU	$F9
RTI_IGNORE	EQU	$FA
RTI_FLAGS	EQU	$FB

;----------------------
; | RTI_F_SLEEP / RTI_F_SLEPT / RTI_F_QUIT / RTI_F_LCD
; | Description: RTI_FLAGS bit numbers: Sleep was pressed, rt_idle has stopped the schedule and blanked the LCD, Mode's press is unread, and the LCD is meant to be on.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RTI_F_SLEEP	EQU	0
RTI_F_SLEPT	EQU	1
RTI_F_QUIT	EQU	2
RTI_F_LCD	EQU	3

;----------------------
; | rt_input_init
; | Description: Sets BTCR to 0x41 (base timer running, interrupt 0 every half second, interrupt 1 off): the BIOS clock needs that tick, and Sleep's wake needs its poll to see Sleep released. Marks the LCD on, takes the buttons and dock as they are (levels only, no line enabled yet) and enables the port 3 interrupt.
; | Author: suinevere
; | Dependencies: rt_in_poll_full, rt_in_dock
; | Globals: rt_input, RTV_LEVELS, BTCR
; | Params: N/A (interrupts masked, before rt_events_init)
; | Returns: N/A. Destroys ACC, B, C.
;----------------------
rt_input_init:
	MOV #$41, BTCR
	MOV #$08, RTI_FLAGS
	CALLF rt_in_poll_full
	MOV #$05, P3INT
	JMPF rt_in_dock

;----------------------
; | rt_buttons
; | Description: vmu_buttons: the buttons held as VMU_BTN_* bits (A as Fire; B, Mode and Sleep have none), less those held since a wake.
; | Author: suinevere
; | Dependencies: rt_in_poll
; | Globals: rt_input, RTV_LEVELS
; | Params: N/A
; | Returns: ACC -- VMU_BTN_* bits. Destroys B, C.
;----------------------
rt_buttons:
	PUSH IE
	CLR1 IE, 7
	CALLF rt_in_poll
	LD RTV_LEVELS
	AND #$1F
	POP IE
	RET

;----------------------
; | rt_sys_flags
; | Description: vmu_sys_flags: VMU_SYS_QUIT once after each Mode press, which this read clears; nothing else is set on the VMU.
; | Author: suinevere
; | Dependencies: rt_in_poll
; | Globals: rt_input
; | Params: N/A
; | Returns: ACC -- VMU_SYS_* bits. Destroys B, C.
;----------------------
rt_sys_flags:
	PUSH IE
	CLR1 IE, 7
	CALLF rt_in_poll
	MOV #$00, ACC
	BPC RTI_FLAGS, RTI_F_QUIT, rt_sys_flags_quit
	POP IE
	RET
rt_sys_flags_quit:
	MOV #$02, ACC
	POP IE
	RET

;----------------------
; | rt_in_poll
; | Description: rt_in_poll_full's read, except that it returns at once when port 3 is as the last poll left it, which a full read would leave unchanged.
; | Author: suinevere
; | Dependencies: rt_in_poll_full (its rt_in_poll_read)
; | Globals: rt_input, RTV_LEVELS, RTV_MASK, RTV_LATCH
; | Params: N/A (interrupts masked or inside an interrupt)
; | Returns: N/A. Destroys ACC, B, C.
;----------------------
rt_in_poll:
	LD P3
	XOR #$FF
	BNE RTI_HELD, rt_in_poll_read
	RET

;----------------------
; | rt_in_poll_full
; | Description: Reads port 3 into vmu_host_set_buttons' model: Mode raises the quit request, button levels follow the held lines and latch enabled lines on a press, Sleep toggles sleep, and the wake reads every held line as released until let go. Arms the released lines.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: rt_input, RTV_LEVELS, RTV_MASK, RTV_LATCH
; | Params: N/A (interrupts masked or inside an interrupt)
; | Returns: N/A. Destroys ACC, B, C.
;----------------------
rt_in_poll_full:
	LD P3
	XOR #$FF
rt_in_poll_read:
	ST C
	LD RTI_HELD
	XOR #$FF
	AND C
	ST B
	LD C
	ST RTI_HELD
	AND RTI_IGNORE
	ST RTI_IGNORE
	BN RTI_FLAGS, RTI_F_SLEEP, rt_in_poll_awake
	BN B, 7, rt_in_poll_arm
	CLR1 RTI_FLAGS, RTI_F_SLEEP
	LD RTI_HELD
	ST RTI_IGNORE
	MOV #$00, B
	BR rt_in_poll_levels
rt_in_poll_awake:
	LD RTI_IGNORE
	XOR #$FF
	AND B
	ST B
	BN B, 6, rt_in_poll_levels
	SET1 RTI_FLAGS, RTI_F_QUIT
rt_in_poll_levels:
	LD RTI_IGNORE
	XOR #$FF
	AND RTI_HELD
	ST C
	MOV #$00, ACC
	BN C, 0, rt_in_poll_down
	SET1 ACC, 4
rt_in_poll_down:
	BN C, 1, rt_in_poll_left
	SET1 ACC, 3
rt_in_poll_left:
	BN C, 2, rt_in_poll_right
	SET1 ACC, 2
rt_in_poll_right:
	BN C, 3, rt_in_poll_fire
	SET1 ACC, 1
rt_in_poll_fire:
	BN C, 4, rt_in_poll_map
	SET1 ACC, 0
rt_in_poll_map:
	ST C
	LD RTV_LEVELS
	XOR #$FF
	AND C
	AND RTV_MASK
	OR RTV_LATCH
	ST RTV_LATCH
	LD RTV_LEVELS
	AND #$E0
	OR C
	ST RTV_LEVELS
	BN B, 7, rt_in_poll_arm
	SET1 RTI_FLAGS, RTI_F_SLEEP
rt_in_poll_arm:
	LD RTI_HELD
	XOR #$FF
	ST P3
	RET

;----------------------
; | rt_in_dock
; | Description: Sets INT0 to fire on the other port 7 level, then makes the dock line's level follow port 7, a rising level latching the line if enabled.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RTV_LEVELS, RTV_MASK, RTV_LATCH
; | Params: N/A (interrupts masked or inside an interrupt)
; | Returns: N/A. Destroys ACC, C.
;----------------------
rt_in_dock:
	LD P7
	AND #$01
	ST C
	BZ rt_in_dock_high
	MOV #$05, I01CR
	BR rt_in_dock_again
rt_in_dock_high:
	MOV #$0D, I01CR
rt_in_dock_again:
	LD P7
	AND #$01
	BNE C, rt_in_dock
	LD C
	BZ rt_in_dock_low
	BP RTV_LEVELS+1, 2, rt_in_dock_ret
	SET1 RTV_LEVELS+1, 2
	BN RTV_MASK+1, 2, rt_in_dock_ret
	SET1 RTV_LATCH+1, 2
	RET
rt_in_dock_low:
	CLR1 RTV_LEVELS+1, 2
rt_in_dock_ret:
	RET

;----------------------
; | rt_in_sleep
; | Description: rt_idle's Sleep step: on Sleep it stops Timer 0 with the counts run so far taken in, so every countdown is kept, and blanks the LCD; on the wake it restores the LCD and restarts the schedule.
; | Author: suinevere
; | Dependencies: rt_ev_sync, rt_ev_restart
; | Globals: rt_input, rt_reserved
; | Params: N/A (interrupts masked)
; | Returns: ACC -- nonzero while asleep. Destroys B, C, TRL, TRH, R1.
;----------------------
rt_in_sleep:
	BP RTI_FLAGS, RTI_F_SLEEP, rt_in_sleep_on
	BN RTI_FLAGS, RTI_F_SLEPT, rt_in_sleep_awake
	CLR1 RTI_FLAGS, RTI_F_SLEPT
	MOV #$80, VCCR
	BP RTI_FLAGS, RTI_F_LCD, rt_in_sleep_restart
	MOV #$00, VCCR
rt_in_sleep_restart:
	CALLF rt_ev_restart
rt_in_sleep_awake:
	MOV #$00, ACC
	RET
rt_in_sleep_on:
	BP RTI_FLAGS, RTI_F_SLEPT, rt_in_sleep_halt
	SET1 RTI_FLAGS, RTI_F_SLEPT
	CALLF rt_ev_sync
	MOV #RT_T0_STOP, T0CNT
	MOV #$00, VCCR
rt_in_sleep_halt:
	MOV #$01, ACC
	RET

;----------------------
; | rt_p3_isr
; | Description: Port 3 interrupt, raised while an armed line is held: clears its flag and polls the buttons, which disarms the held lines.
; | Author: suinevere
; | Dependencies: rt_in_poll, rt_isr_out
; | Globals: rt_input, RTV_SCRATCH (its return address)
; | Params: N/A (the interrupted code is on RAM bank 1)
; | Returns: N/A (RETI through rt_isr_out). Keeps every register.
;----------------------
rt_p3_isr:
	POP RTV_SCRATCH+1
	POP RTV_SCRATCH
	PUSH RTV_SCRATCH
	PUSH RTV_SCRATCH+1
	PUSH PSW
	PUSH ACC
	MOV #$02, PSW
	PUSH B
	PUSH C
	CLR1 P3INT, 1
	CALLF rt_in_poll
	POP C
	POP B
	JMPF rt_isr_out

;----------------------
; | rt_bt_isr
; | Description: Base timer interrupt, every half second: polls the buttons (how a release is seen while rt_idle halts), then forwards to the BIOS clock, which clears the flags.
; | Author: suinevere
; | Dependencies: rt_in_poll, rt_bios_timer, rt_isr_out
; | Globals: rt_input, RTV_SCRATCH (its return address)
; | Params: N/A (the interrupted code is on RAM bank 1)
; | Returns: N/A (RETI through rt_isr_out). Keeps every register.
;----------------------
rt_bt_isr:
	POP RTV_SCRATCH+1
	POP RTV_SCRATCH
	PUSH RTV_SCRATCH
	PUSH RTV_SCRATCH+1
	PUSH PSW
	PUSH ACC
	MOV #$02, PSW
	PUSH B
	PUSH C
	CALLF rt_in_poll
	CALLF rt_bios_timer
	POP C
	POP B
	JMPF rt_isr_out

;----------------------
; | rt_int0_isr
; | Description: INT0, port 7's dock level changing: updates the dock line.
; | Author: suinevere
; | Dependencies: rt_in_dock, rt_isr_out
; | Globals: RTV_LEVELS, RTV_LATCH, RTV_SCRATCH (its return address)
; | Params: N/A (the interrupted code is on RAM bank 1)
; | Returns: N/A (RETI through rt_isr_out). Keeps every register.
;----------------------
rt_int0_isr:
	POP RTV_SCRATCH+1
	POP RTV_SCRATCH
	PUSH RTV_SCRATCH
	PUSH RTV_SCRATCH+1
	PUSH PSW
	PUSH ACC
	MOV #$02, PSW
	PUSH C
	CALLF rt_in_dock
	POP C
	JMPF rt_isr_out
