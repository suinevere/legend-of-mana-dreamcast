;----------------------
; | beep.s
; | Description: beep.c in LC86K assembly: the quit beep's tone and the quit sound around it.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/beep.c, misc.s (g_sound_on, g_sound_off), pocketstation/src/vmu_rt/rt_sound.s (rt_buzz), pocketstation/src/vmu_rt/rt_events.s
; | Globals: docked, snd_mode
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_quit_beep
; | Description: game_quit_beep: buzzes at tone 192 (beep.c's BEEP_TONE) shifted right by shift, tail-calling rt_buzz.
; | Author: suinevere
; | Dependencies: rt_buzz
; | Globals: N/A
; | Params: ACC -- shift
; | Returns: N/A. Destroys ACC, B, C, PSW (rt_buzz).
;----------------------
g_quit_beep:
	ST C
	MOV #$C0, ACC
	XCH C
	BZ g_quit_beep_go
	XCH C
g_quit_beep_shift:
	ROR
	AND #$7F
	DBNZ C, g_quit_beep_shift
	JMPF rt_buzz
g_quit_beep_go:
	XCH C
	JMPF rt_buzz

;----------------------
; | g_quit_sound
; | Description: game_quit_sound: unless docked, muted or in sound mode 2, disables lines 0x35FF and beeps once (tone 192, or 48 with snd_mode set) between buzzer on and off. Then it restores the lines.
; | Author: suinevere
; | Dependencies: rt_sys_flags, rt_irq_mask, rt_irq_disable, g_sound_on, g_quit_beep, g_sound_off, rt_irq_enable, rrl_ram.inc (HW_DOCKED, HW_SND_MODE)
; | Globals: docked, snd_mode
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, PSW; five bytes of stack.
;----------------------
g_quit_sound:
	LD HW_DOCKED
	BNZ g_quit_sound_ret
	CALLF rt_sys_flags
	BP ACC, 0, g_quit_sound_ret
	LD HW_SND_MODE
	BE #$02, g_quit_sound_ret
	BZ g_quit_sound_shift
	MOV #$02, ACC
g_quit_sound_shift:
	ST B
	CALLF rt_irq_mask
	PUSH TRL
	PUSH TRH
	PUSH B
	MOV #$FF, TRL
	MOV #$35, TRH
	CALLF rt_irq_disable
	CALLF g_sound_on
	POP ACC
	CALLF g_quit_beep
	CALLF g_sound_off
	POP TRH
	POP TRL
	JMPF rt_irq_enable
g_quit_sound_ret:
	RET
