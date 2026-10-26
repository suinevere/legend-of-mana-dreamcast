;----------------------
; | mode.s
; | Description: mode.c in LC86K assembly: the mode leave steps, the pair game's end, the race's reset and countdown, the restart wait and the shake offset.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/mode.c, pocketstation/build/vmu_game/rrl_ram.inc
; | Globals: mode, mode_wait, mode_phase, mode_done, sel_a, sel_b, sel_c, sel_d, sel_e, scene_id, reel_pos_a, reel_pos_b, reel_pos_c
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_mode5_leave
; | Description: game_mode5_leave: clears mode_wait.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_MODE_WAIT)
; | Globals: mode_wait
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_mode5_leave:
	MOV #$00, HW_MODE_WAIT
	RET

;----------------------
; | g_mode7_leave
; | Description: game_mode7_leave: sets sel_e and mode_done to 1.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_SEL_E, HW_MODE_DONE)
; | Globals: sel_e, mode_done
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_mode7_leave:
	MOV #$01, HW_SEL_E
	MOV #$01, HW_MODE_DONE
	RET

;----------------------
; | g_pair_end
; | Description: pair_end: sets mode to 8 when scene_id is 13, else to 3.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_MODE, HW_SCENE_ID)
; | Globals: mode, scene_id
; | Params: N/A
; | Returns: N/A. Destroys ACC.
;----------------------
g_pair_end:
	MOV #$03, HW_MODE
	LD HW_SCENE_ID
	XOR #$0D
	BNZ g_pair_end_ret
	MOV #$08, HW_MODE
g_pair_end_ret:
	RET

;----------------------
; | g_race_reset
; | Description: race_reset: zeroes the three reel positions, sets mode_wait to 0xF0 and mode_done to 2.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_REEL_POS_A, HW_REEL_POS_B, HW_REEL_POS_C, HW_MODE_WAIT, HW_MODE_DONE)
; | Globals: reel_pos_a, reel_pos_b, reel_pos_c, mode_wait, mode_done
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_race_reset:
	MOV #$00, HW_REEL_POS_A
	MOV #$00, HW_REEL_POS_A+1
	MOV #$00, HW_REEL_POS_B
	MOV #$00, HW_REEL_POS_B+1
	MOV #$00, HW_REEL_POS_C
	MOV #$00, HW_REEL_POS_C+1
	MOV #$F0, HW_MODE_WAIT
	MOV #$02, HW_MODE_DONE
	RET

;----------------------
; | g_race_wait
; | Description: race_wait: counts mode_wait down by one and returns 1 when it reaches 0, else 0.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_MODE_WAIT)
; | Globals: mode_wait
; | Params: N/A
; | Returns: ACC -- 1 when the wait is over, else 0. Destroys nothing else.
;----------------------
g_race_wait:
	DEC HW_MODE_WAIT
	LD HW_MODE_WAIT
	BZ g_race_wait_done
	MOV #$00, ACC
	RET
g_race_wait_done:
	MOV #$01, ACC
	RET

;----------------------
; | g_restart_wait
; | Description: restart_wait: clears mode_phase and sets mode_wait to 12.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_MODE_PHASE, HW_MODE_WAIT)
; | Globals: mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_restart_wait:
	MOV #$00, HW_MODE_PHASE
	MOV #$0C, HW_MODE_WAIT
	RET

;----------------------
; | g_shake_offset
; | Description: shake_offset: the shake's offset for a wait count, 0, 2 or 3 from its low two bits (0, 2, 3, 2), negated when bit 2 is set.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- wait, of which only the low three bits count
; | Returns: ACC -- the offset as a signed byte. Destroys C.
;----------------------
g_shake_offset:
	ST C
	AND #$03
	BZ g_shake_offset_ret
	XOR #$02
	BZ g_shake_offset_3
	MOV #$02, ACC
	BR g_shake_offset_sign
g_shake_offset_3:
	MOV #$03, ACC
g_shake_offset_sign:
	BN C, 2, g_shake_offset_ret
	XOR #$FF
	INC ACC
g_shake_offset_ret:
	RET

;----------------------
; | g_mode1_leave
; | Description: game_mode1_leave: clears mode_done and restarts the wait, tail-calling g_restart_wait.
; | Author: suinevere
; | Dependencies: g_restart_wait, rrl_ram.inc (HW_MODE_DONE)
; | Globals: mode_done, mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_mode1_leave:
	MOV #$00, HW_MODE_DONE
	JMPF g_restart_wait

;----------------------
; | g_mode2_leave
; | Description: game_mode2_leave: sets sel_a to 1 and restarts the wait, tail-calling g_restart_wait.
; | Author: suinevere
; | Dependencies: g_restart_wait, rrl_ram.inc (HW_SEL_A)
; | Globals: sel_a, mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_mode2_leave:
	MOV #$01, HW_SEL_A
	JMPF g_restart_wait

;----------------------
; | g_mode4_leave
; | Description: game_mode4_leave: clears sel_b and restarts the wait, tail-calling g_restart_wait.
; | Author: suinevere
; | Dependencies: g_restart_wait, rrl_ram.inc (HW_SEL_B)
; | Globals: sel_b, mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_mode4_leave:
	MOV #$00, HW_SEL_B
	JMPF g_restart_wait

;----------------------
; | g_mode13_leave
; | Description: game_mode13_leave: sets sel_d and sel_c to 8 and 0 when sel_a is 0, else to 4 and 0x10, then sel_b to 2, clears mode_done and restarts the wait.
; | Author: suinevere
; | Dependencies: g_restart_wait, rrl_ram.inc (HW_SEL_A, HW_SEL_B, HW_SEL_C, HW_SEL_D, HW_MODE_DONE)
; | Globals: sel_a, sel_b, sel_c, sel_d, mode_done, mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys ACC.
;----------------------
g_mode13_leave:
	LD HW_SEL_A
	BNZ g_mode13_leave_one
	MOV #$08, HW_SEL_D
	MOV #$00, HW_SEL_C
	BR g_mode13_leave_end
g_mode13_leave_one:
	MOV #$04, HW_SEL_D
	MOV #$10, HW_SEL_C
g_mode13_leave_end:
	MOV #$02, HW_SEL_B
	MOV #$00, HW_MODE_DONE
	JMPF g_restart_wait
