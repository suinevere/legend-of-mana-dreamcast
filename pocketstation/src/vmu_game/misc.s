;----------------------
; | misc.s
; | Description: misc.c in LC86K assembly: the buzzer, the sound request, the state block's save, the digits' x and the mirrored direction. Also the nibble and byte tables' reads, the power-down and the dock handler.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/misc.c, pocketstation/build/vmu_game/rrl_ram.inc, game.inc, app_hi, pocketstation/src/vmu_rt/{rt_sound.s,rt_events.s,rt_file.s}
; | Globals: sound_req, the state block (points to save_tail)
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_sound_on
; | Description: game_sound_on: starts the buzzer at tone 96 (misc.c's SOUND_TONE), tail-calling rt_buzz.
; | Author: suinevere
; | Dependencies: rt_buzz
; | Globals: N/A
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, PSW (rt_buzz).
;----------------------
g_sound_on:
	MOV #$60, ACC
	JMPF rt_buzz

;----------------------
; | g_sound_off
; | Description: game_sound_off: silences the buzzer, tail-calling rt_buzz with tone 0.
; | Author: suinevere
; | Dependencies: rt_buzz
; | Globals: N/A
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, PSW (rt_buzz).
;----------------------
g_sound_off:
	MOV #$00, ACC
	JMPF rt_buzz

;----------------------
; | g_sound_request
; | Description: game_sound_request: stores the request in sound_req for the sound driver.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_SOUND_REQ)
; | Globals: sound_req
; | Params: ACC -- request
; | Returns: N/A. Destroys nothing.
;----------------------
g_sound_request:
	ST HW_SOUND_REQ
	RET

;----------------------
; | g_save
; | Description: game_save: masks the sound end line and writes the 128-byte state block at points (vmu_ram 0x25B) to the pet save's sector 6, retrying until rt_save_write succeeds, then puts the enabled lines back.
; | Author: suinevere
; | Dependencies: rt_irq_mask, rt_irq_disable, rt_save_write, rt_irq_enable, rrl_ram.inc (HWW_POINTS)
; | Globals: the state block (points to save_tail)
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0, R1, FPR, PSW; four bytes of stack.
;----------------------
g_save:
	CALLF rt_irq_mask
	PUSH TRL
	PUSH TRH
	MOV #$00, TRL
	MOV #$20, TRH
	CALLF rt_irq_disable
g_save_try:
	MOV #$06, ACC
	MOV #<(HWW_POINTS+$100), TRL
	MOV #>(HWW_POINTS+$100), TRH
	CALLF rt_save_write
	BNZ g_save_try
	POP TRH
	POP TRL
	JMPF rt_irq_enable

;----------------------
; | g_digits_x
; | Description: game_digits_x: where a five-digit number right-aligned from x starts, x for 10000 or more, then 4 pixels further in for each digit fewer, down to x + 16 for one digit. g_digits_x_vr reads n at ARG_BLOCK page offset VRMAD1 instead.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: TRH:TRL -- x, 16-bit (the callers keep an int16); ARG_BLOCK+0..3 -- n, 32-bit
; | Returns: TRH:TRL -- the x. Destroys ACC, B, C, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_digits_x:
	MOV #<ARG_BLOCK, VRMAD1
g_digits_x_vr:
	SET1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST C
	LD VTRBF
	ST B
	LD VTRBF
	OR VTRBF
	CLR1 VSEL, 4
	BNZ g_digits_x_ret
	LD C
	SUB #$10
	LD B
	SUBC #$27
	BN PSW, 7, g_digits_x_ret
	LD C
	SUB #$E8
	LD B
	SUBC #$03
	BN PSW, 7, g_digits_x_4
	LD C
	SUB #$64
	LD B
	SUBC #$00
	BN PSW, 7, g_digits_x_8
	LD C
	SUB #$0A
	LD B
	SUBC #$00
	BN PSW, 7, g_digits_x_12
	MOV #$10, ACC
	BR g_digits_x_add
g_digits_x_12:
	MOV #$0C, ACC
	BR g_digits_x_add
g_digits_x_8:
	MOV #$08, ACC
	BR g_digits_x_add
g_digits_x_4:
	MOV #$04, ACC
g_digits_x_add:
	ADD TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
g_digits_x_ret:
	RET

;----------------------
; | g_mirror_direction
; | Description: game_mirror_direction: the direction opposite the lowest of bits 0-3 set (1 to 8, 2 to 4, 4 to 2, 8 to 1), or bits unchanged when none is.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- bits, a direction mask
; | Returns: ACC -- the mirrored direction. Destroys nothing else.
;----------------------
g_mirror_direction:
	BN ACC, 0, g_mirror_direction_2
	MOV #$08, ACC
	RET
g_mirror_direction_2:
	BN ACC, 1, g_mirror_direction_4
	MOV #$04, ACC
	RET
g_mirror_direction_4:
	BN ACC, 2, g_mirror_direction_8
	MOV #$02, ACC
	RET
g_mirror_direction_8:
	BN ACC, 3, g_mirror_direction_ret
	MOV #$01, ACC
g_mirror_direction_ret:
	RET

;----------------------
; | ROM_NIBBLES
; | Description: Table offset of the nibble table at 0x02009DF0 (rrl_rom.h ROM_NIBBLES_9DF0): entry n is the low nibble of byte n / 2 when n is even, the high one when odd.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/rrl_rom.h
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_NIBBLES	EQU	$9DF0

;----------------------
; | g_nibble
; | Description: game_nibble: reads entry index of the nibble table, the C's word read and shift being one byte at index / 2 and its low or high half.
; | Author: suinevere
; | Dependencies: app_hi, ROM_NIBBLES
; | Globals: N/A
; | Params: ACC -- index
; | Returns: ACC -- the nibble. Destroys C, TRL, TRH.
;----------------------
g_nibble:
	ST C
	ROR
	AND #$7F
	MOV #<(app_hi+ROM_NIBBLES), TRL
	MOV #>(app_hi+ROM_NIBBLES), TRH
	LDC
	BN C, 0, g_nibble_low
	ROR
	ROR
	ROR
	ROR
g_nibble_low:
	AND #$0F
	RET

;----------------------
; | ROM_BYTES_9EFC
; | Description: The byte table's offset in RES1506.BIN that game_byte_is reads (misc.c's ROM_BYTES_9EFC).
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/misc.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_BYTES_9EFC	EQU	$9EFC

;----------------------
; | g_byte_is
; | Description: game_byte_is: 1 when entry index of the byte table equals value, else 0.
; | Author: suinevere
; | Dependencies: app_hi, ROM_BYTES_9EFC
; | Globals: N/A
; | Params: ACC -- index; B -- value (every caller passes a byte, mode.c's scene ids)
; | Returns: ACC -- 1 or 0. Destroys TRH, TRL, PSW.
;----------------------
g_byte_is:
	MOV #<(app_hi+ROM_BYTES_9EFC), TRL
	MOV #>(app_hi+ROM_BYTES_9EFC), TRH
	LDC
	SUB B
	BZ g_byte_is_yes
	MOV #$00, ACC
	RET
g_byte_is_yes:
	MOV #$01, ACC
	RET

;----------------------
; | g_power_down
; | Description: game_power_down: stops the ticks, the sound and the buzzer, disables every line, drops the handler, enables only the clock line and idles until no button line is up.
; | Author: suinevere
; | Dependencies: rt_tick_mode, rt_sound_stop, g_sound_off, rt_irq_disable, rt_set_handler, rt_irq_enable, rt_irq_levels, rt_idle
; | Globals: N/A
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, PSW.
;----------------------
g_power_down:
	MOV #$00, ACC
	CALLF rt_tick_mode
	CALLF rt_sound_stop
	CALLF g_sound_off
	MOV #$FF, TRL
	MOV #$3F, TRH
	CALLF rt_irq_disable
	MOV #$00, TRL
	MOV #$00, TRH
	CALLF rt_set_handler
	MOV #$00, TRL
	MOV #$02, TRH
	CALLF rt_irq_enable
g_power_down_wait:
	CALLF rt_irq_levels
	LD TRL
	AND #$1F
	BZ g_power_down_ret
	CALLF rt_idle
	BR g_power_down_wait
g_power_down_ret:
	RET

;----------------------
; | g_dock_irq
; | Description: game_dock_irq, the dock line's handler: marks the app docked, stops the sound end and its interrupt, and switches the buzzer off.
; | Author: suinevere
; | Dependencies: rt_sound_stop, g_timer_irq, g_sound_off, rrl_ram.inc (HW_DOCKED)
; | Globals: docked
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, PSW; four bytes of stack.
;----------------------
g_dock_irq:
	MOV #$01, HW_DOCKED
	CALLF rt_sound_stop
	MOV #$02, B
	MOV #$00, ACC
	CALLF g_timer_irq
	JMPF g_sound_off

;----------------------
; | g_standby
; | Description: game_standby: stops the sound and tick, turns the display off and sleeps in standby with the RTC line off until a button or the RTC wakes it, then restarts the frame tick.
; | Author: suinevere
; | Dependencies: g_sound_stop, g_sound_off, rt_tick_mode, rt_sound_stop, rt_lcd, rt_irq_disable, rt_power, rt_irq_enable, rt.inc (RT_TICK_OFF, RT_TICK_FRAME, RT_LINE_RTC, RT_POWER_STANDBY)
; | Globals: snd_*, sound hardware
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, PSW and what rt_power's standby does.
;----------------------
g_standby:
	CALLF g_sound_stop
	MOV #RT_TICK_OFF, ACC
	CALLF rt_tick_mode
	CALLF rt_sound_stop
	CALLF g_sound_off
	MOV #$00, ACC
	CALLF rt_lcd
	MOV #<RT_LINE_RTC, TRL
	MOV #>RT_LINE_RTC, TRH
	CALLF rt_irq_disable
	MOV #RT_POWER_STANDBY, ACC
	CALLF rt_power
	MOV #<RT_LINE_RTC, TRL
	MOV #>RT_LINE_RTC, TRH
	CALLF rt_irq_enable
	MOV #RT_TICK_FRAME, ACC
	JMPF rt_tick_mode

;----------------------
; | g_n_arg
; | Description: Puts the byte ACC in ARG_BLOCK+0..3 as a 32-bit argument.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- the value
; | Returns: N/A. Destroys VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_n_arg:
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	ST VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	RET

;----------------------
; | g_dock_screen
; | Description: game_dock_screen: undocked with the dock line down it returns; otherwise it silences the sound, blanks the screen, waits in standby ticks and quits for the dock.
; | Author: suinevere
; | Dependencies: rt_irq_levels, rt_sound_stop, g_timer_irq, g_sound_off, g_fb_clear, g_fb_present, g_ticks_start, g_n_arg, g_wait_ticks, g_dock_quit, rrl_ram.inc (HW_DOCKED)
; | Globals: docked and what the callees write
; | Params: N/A
; | Returns: N/A, and only when undocked with the dock line down. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW.
;----------------------
g_dock_screen:
	LD HW_DOCKED
	BNZ g_dock_screen_blank
	CALLF rt_irq_levels
	LD TRH
	AND #$04
	BNZ g_dock_screen_stop
	RET
g_dock_screen_stop:
	CALLF rt_sound_stop
	MOV #$02, B
	MOV #$00, ACC
	CALLF g_timer_irq
	CALLF g_sound_off
g_dock_screen_blank:
	CALLF g_fb_clear
	CALLF g_fb_present
	MOV #$03, ACC
	CALLF g_ticks_start
	MOV #$00, ACC
	CALLF g_n_arg
	MOV #$01, ACC
	CALLF g_wait_ticks
	MOV #$5A, ACC
	CALLF g_n_arg
	MOV #$01, ACC
	CALLF g_wait_ticks
	JMPF g_dock_quit
