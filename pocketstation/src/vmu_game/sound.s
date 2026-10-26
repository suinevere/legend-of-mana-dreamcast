;----------------------
; | sound.s
; | Description: sound.c in LC86K assembly: the sound end's stop with its interrupt, the sound state's start-up, the sound end handler and starting a sound.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/sound.c, pocketstation/build/vmu_game/rrl_ram.inc, pocketstation/src/vmu_rt/{rt_sound.s,rt_input.s}, hw.s (g_timer_irq), misc.s (g_sound_off), system.s (g_set_irq_handler)
; | Globals: snd_mode, snd_playing, snd_next, sound_req
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_sound_stop
; | Description: game_sound_stop: stops the sound end and its interrupt, clears the playing and queued sounds, and switches the buzzer off only when nothing was playing, as the C does.
; | Author: suinevere
; | Dependencies: rt_sound_stop, g_timer_irq, g_sound_off, rrl_ram.inc (HW_SND_PLAYING, HW_SND_NEXT)
; | Globals: snd_playing, snd_next
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, PSW; four bytes of stack.
;----------------------
g_sound_stop:
	CALLF rt_sound_stop
	MOV #$02, B
	MOV #$00, ACC
	CALLF g_timer_irq
	LD HW_SND_PLAYING
	MOV #$FF, HW_SND_PLAYING
	MOV #$FF, HW_SND_NEXT
	BNE #$FF, g_sound_stop_ret
	JMPF g_sound_off
g_sound_stop_ret:
	RET

;----------------------
; | g_sound_init
; | Description: game_sound_init: snd_mode from the system flags' bits 2-3, no sound playing, queued or requested, the buzzer off, the sound end and its interrupt stopped, and handler 3 on line 13.
; | Author: suinevere
; | Dependencies: rt_sys_flags, g_sound_off, rt_sound_stop, g_timer_irq, g_set_irq_handler, rrl_ram.inc (HW_SND_MODE, HW_SND_PLAYING, HW_SND_NEXT, HW_SOUND_REQ)
; | Globals: snd_mode, snd_playing, snd_next, sound_req, fiq_driver
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, VRMAD, PSW; four bytes of stack.
;----------------------
g_sound_init:
	CALLF rt_sys_flags
	AND #$0C
	ROR
	ROR
	ST HW_SND_MODE
	MOV #$FF, HW_SND_PLAYING
	MOV #$FF, HW_SND_NEXT
	MOV #$FF, HW_SOUND_REQ
	CALLF g_sound_off
	CALLF rt_sound_stop
	MOV #$02, B
	MOV #$00, ACC
	CALLF g_timer_irq
	MOV #$03, B
	MOV #$0D, ACC
	JMPF g_set_irq_handler

;----------------------
; | ROM_SOUNDS
; | Description: File offset of sound.c's ROM_SOUNDS: a card address per sound id, each sound ending where the next begins.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/sound.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_SOUNDS	EQU	$A7C8

;----------------------
; | g_sound_load
; | Description: Sets snd_start to sound id's file offset and snd_len to its length in samples, from ROM_SOUNDS read with LDC from app_tables.
; | Author: suinevere
; | Dependencies: app_tables, ROM_SOUNDS, rrl_ram.inc (HW_SND_START, HW_SND_LEN), g_tr_x4
; | Globals: snd_start, snd_len
; | Params: ACC -- id
; | Returns: N/A. Destroys ACC, C, TRH, TRL, PSW.
;----------------------
g_sound_load:
	ST TRL
	MOV #$00, TRH
	CALLF g_tr_x4
	LD TRL
	ADD #<(app_tables+ROM_SOUNDS)
	ST TRL
	LD TRH
	ADDC #>(app_tables+ROM_SOUNDS)
	ST TRH
	MOV #$00, ACC
	LDC
	ST HW_SND_START
	MOV #$01, ACC
	LDC
	ST HW_SND_START+1
	MOV #$04, ACC
	LDC
	ST C
	MOV #$05, ACC
	LDC
	ST TRH
	LD C
	SUB HW_SND_START
	ST HW_SND_LEN
	LD TRH
	SUBC HW_SND_START+1
	ST HW_SND_LEN+1
	MOV #$00, HW_SND_LEN+2
	MOV #$00, HW_SND_LEN+3
	RET

;----------------------
; | g_sound_len_less
; | Description: TRH:TRL:B, rt_sound_play's 24-bit sample count, = snd_len - ACC.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_SND_LEN)
; | Globals: snd_len
; | Params: ACC -- 0 or 1
; | Returns: TRH:TRL:B -- the count, B lowest. Destroys ACC, C, PSW.
;----------------------
g_sound_len_less:
	ST C
	LD HW_SND_LEN
	SUB C
	ST B
	LD HW_SND_LEN+1
	SUBC #$00
	ST TRL
	LD HW_SND_LEN+2
	SUBC #$00
	ST TRH
	RET

;----------------------
; | g_sound_fiq
; | Description: game_sound_fiq: the sound end. Plays the queued sound at the slow rate the C hard-codes for a chained sound, or with none queued stops the sound, its line and the sound hardware.
; | Author: suinevere
; | Dependencies: g_sound_load, g_sound_len_less, g_sound_off, rt_sound_stop, rt_irq_disable, rt_sound_play, rt.inc (RT_LINE_SOUND, RT_SOUND_SLOW), rrl_ram.inc (HW_SND_NEXT, HW_SND_PLAYING, HW_SND_POS)
; | Globals: snd_next, snd_playing, snd_start, snd_len, snd_pos
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, PSW.
;----------------------
g_sound_fiq:
	LD HW_SND_NEXT
	BNE #$FF, g_sound_fiq_play
	MOV #$FF, HW_SND_PLAYING
	MOV #$FF, HW_SND_NEXT
	CALLF rt_sound_stop
	MOV #<RT_LINE_SOUND, TRL
	MOV #>RT_LINE_SOUND, TRH
	CALLF rt_irq_disable
	JMPF g_sound_off
g_sound_fiq_play:
	ST HW_SND_PLAYING
	MOV #$FF, HW_SND_NEXT
	LD HW_SND_PLAYING
	CALLF g_sound_load
	MOV #$01, HW_SND_POS
	MOV #$00, HW_SND_POS+1
	MOV #$00, HW_SND_POS+2
	MOV #$00, HW_SND_POS+3
	MOV #$01, ACC
	CALLF g_sound_len_less
	MOV #RT_SOUND_SLOW, C
	JMPF rt_sound_play

;----------------------
; | g_sound_start
; | Description: game_sound_start: queues sound id after the playing one, cutting it short, or starts it: snd_* set, the sound hardware on, Timer 2's line on and the end timed at counts (500 slow, 71 fast).
; | Author: suinevere
; | Dependencies: g_sound_load, g_sound_len_less, g_sound_on, g_timer_irq, rt_sound_play, rrl_ram.inc (HW_SND_MODE, HW_SND_POS, HW_SND_LEN, HW_SND_NEXT, HW_SND_PLAYING)
; | Globals: snd_mode, snd_pos, snd_len, snd_next, snd_playing, snd_start
; | Params: ACC -- id; TRH:TRL -- counts, VMU_SOUND_SLOW (500) or VMU_SOUND_FAST (71)
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, PSW; four bytes of stack.
;----------------------
g_sound_start:
	ST B
	LD HW_SND_MODE
	BNE #$02, g_sound_start_on
	RET
g_sound_start_on:
	MOV #$00, C
	LD TRL
	BNE #$F4, g_sound_start_fast
	LD TRH
	BE #$01, g_sound_start_rate
g_sound_start_fast:
	MOV #$01, C
g_sound_start_rate:
	PUSH C
	PUSH B
	LD HW_SND_LEN
	ST HW_SND_POS
	LD HW_SND_LEN+1
	ST HW_SND_POS+1
	LD HW_SND_LEN+2
	ST HW_SND_POS+2
	LD HW_SND_LEN+3
	ST HW_SND_POS+3
	LD B
	ST HW_SND_NEXT
	LD HW_SND_PLAYING
	BE #$FF, g_sound_start_new
	POP B
	POP C
	MOV #$00, B
	MOV #$00, TRL
	MOV #$00, TRH
	JMPF rt_sound_play
g_sound_start_new:
	MOV #$FF, HW_SND_NEXT
	POP ACC
	ST HW_SND_PLAYING
	CALLF g_sound_load
	MOV #$00, HW_SND_POS
	MOV #$00, HW_SND_POS+1
	MOV #$00, HW_SND_POS+2
	MOV #$00, HW_SND_POS+3
	CALLF g_sound_on
	MOV #$02, B
	MOV #$01, ACC
	CALLF g_timer_irq
	MOV #$00, ACC
	CALLF g_sound_len_less
	POP C
	JMPF rt_sound_play

;----------------------
; | g_sound_begin
; | Description: game_sound_begin: stops the sound when docked or muted, the system flags read only when undocked; otherwise starts sound id at the slow rate.
; | Author: suinevere
; | Dependencies: rt_sys_flags, g_sound_stop, g_sound_start, rrl_ram.inc (HW_DOCKED)
; | Globals: docked and what g_sound_stop and g_sound_start write
; | Params: ACC -- id
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, PSW.
;----------------------
g_sound_begin:
	PUSH ACC
	LD HW_DOCKED
	BNZ g_sound_begin_stop
	CALLF rt_sys_flags
	AND #$01
	BNZ g_sound_begin_stop
	POP ACC
	MOV #<500, TRL
	MOV #>500, TRH
	JMPF g_sound_start
g_sound_begin_stop:
	POP ACC
	JMPF g_sound_stop

;----------------------
; | g_sound_play_waiting
; | Description: game_sound_play_waiting: plays the requested sound at the fast rate and idles until it ends or the VMU docks, unless docked or muted; restores frame ticks when it ends.
; | Author: suinevere
; | Dependencies: rt_sys_flags, g_sound_stop, rt_tick_mode, g_sound_start, rt_idle, rrl_ram.inc (HW_SOUND_REQ, HW_DOCKED, HW_SND_PLAYING)
; | Globals: sound_req, docked, snd_playing and what the callees and the handler write
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; one byte of stack on top of its callees'.
;----------------------
g_sound_play_waiting:
	LD HW_SOUND_REQ
	MOV #$FF, HW_SOUND_REQ
	BNE #$FF, g_sound_play_waiting_req
	RET
g_sound_play_waiting_req:
	PUSH ACC
	LD HW_DOCKED
	BNZ g_sound_play_waiting_off
	CALLF rt_sys_flags
	AND #$01
	BZ g_sound_play_waiting_play
g_sound_play_waiting_off:
	POP ACC
	JMPF g_sound_stop
g_sound_play_waiting_play:
	LD HW_SND_PLAYING
	BE #$FF, g_sound_play_waiting_fast
	CALLF g_sound_stop
g_sound_play_waiting_fast:
	MOV #$02, ACC
	CALLF rt_tick_mode
	POP ACC
	MOV #71, TRL
	MOV #$00, TRH
	CALLF g_sound_start
g_sound_play_waiting_wait:
	LD HW_DOCKED
	BNZ g_sound_play_waiting_docked
	LD HW_SND_PLAYING
	BNE #$FF, g_sound_play_waiting_idle
	MOV #$01, ACC
	JMPF rt_tick_mode
g_sound_play_waiting_idle:
	CALLF rt_idle
	BR g_sound_play_waiting_wait
g_sound_play_waiting_docked:
	JMPF g_sound_stop
