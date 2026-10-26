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

;----------------------
; | g_mode14_leave
; | Description: game_mode14_leave: mode_done 3 for a full bag, else 0; scene 0, scratch_wait 20, the mode wait restarted, then the bag's kinds and first count refreshed.
; | Author: suinevere
; | Dependencies: g_bag_kinds, g_bag_count_first, game.inc (ARG_BLOCK), rrl_ram.inc (HW_BAG_FULL, HW_MODE_DONE, HW_SCENE_ID, HW_SCRATCH_WAIT, HW_MODE_PHASE, HW_MODE_WAIT)
; | Globals: bag_full, mode_done, scene_id, scratch_wait, mode_phase, mode_wait and what g_bag_kinds and g_bag_count_first write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear; four bytes of stack for ARG_BLOCK on top of its callees'.
;----------------------
g_mode14_leave:
	LD HW_BAG_FULL
	BZ g_mode14_leave_done
	MOV #$03, ACC
g_mode14_leave_done:
	ST HW_MODE_DONE
	MOV #$00, HW_SCENE_ID
	MOV #20, HW_SCRATCH_WAIT
	MOV #$00, HW_MODE_PHASE
	MOV #12, HW_MODE_WAIT
	SET1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	CLR1 VSEL, 4
	CALLF g_bag_kinds
	MOV #<(ARG_BLOCK+3), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	JMPF g_bag_count_first

;----------------------
; | g_mode3_leave
; | Description: game_mode3_leave: saves the walk's cell and facing; off the bonus board, enters board mode 8 once steps reach (odds_set + 1) * 32, else resets the lap and fills a new board.
; | Author: suinevere
; | Dependencies: g_board_enter, g_anims_clear, g_anim_start, g_random, g_rl_strip, g_board_fill, rrl_ram.inc (HW_SEL_A-HW_SEL_E, HW_CELL_SAVED, HW_FACING_SAVED, HW_WALK_BUSY, HW_STEPS, HW_ODDS_SET, HW_MODE, HW_MODE_DONE, HW_BONUS_DONE, HW_WALK_BEHIND, HW_METER*, HW_BAG_CURSOR, HW_LAP_POINTS, HW_LAP_MARK, HW_STRIP_C_LEN, HWW_BAG_COUNT, HWW_PET_KIND)
; | Globals: the fields above and what the callees write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode3_leave:
	LD HW_SEL_C
	ST HW_CELL_SAVED
	LD HW_SEL_D
	ST HW_FACING_SAVED
	MOV #$00, HW_SEL_E
	MOV #$00, HW_WALK_BUSY
	LD HW_SEL_B
	BE #$02, g_mode3_leave_lap
	LD HW_STEPS
	AND #$1F
	BNZ g_mode3_leave_ret
	LD HW_STEPS
	ROL
	ROL
	ROL
	AND #$07
	ST C
	LD HW_ODDS_SET
	INC ACC
	BZ g_mode3_leave_ret
	BNE C, g_mode3_leave_ret
	MOV #$08, HW_MODE
	JMPF g_board_enter
g_mode3_leave_ret:
	RET
g_mode3_leave_lap:
	LD HW_MODE_DONE
	ST HW_ODDS_SET
	MOV #$00, HW_BONUS_DONE
	MOV #$00, HW_STEPS
	MOV #$00, HW_WALK_BEHIND
	MOV #$03, HW_METER_MAX
	MOV #$03, HW_METER_SHOWN
	MOV #$03, HW_METER
	MOV #$00, HW_METER_DELAY
	CLR1 VSEL, 4
	MOV #<HWW_BAG_COUNT, VRMAD1
	MOV #>HWW_BAG_COUNT, VRMAD2
	LD VTRBF
	ST HW_BAG_CURSOR
	MOV #$00, HW_LAP_POINTS
	MOV #$00, HW_LAP_POINTS+1
	CALLF g_anims_clear
	MOV #<HWW_PET_KIND, VRMAD1
	MOV #>HWW_PET_KIND, VRMAD2
	LD VTRBF
	ADD #$08
	ST B
	MOV #$01, ACC
	SUB HW_SEL_A
	ROL
	AND #$FE
	ST C
	MOV #$00, ACC
	CALLF g_anim_start
	MOV #$00, HW_LAP_MARK
	MOV #$40, TRL
	MOV #$00, TRH
	CALLF g_random
	LD TRL
	ROR
	ROR
	ROR
	ROR
	AND #$0F
	ADD #$04
	ST TRL
	MOV #$00, TRH
	MOV #$20, B
	MOV #$00, ACC
	CALLF g_rl_strip
	LD TRL
	ST HW_STRIP_C_LEN
	JMPF g_board_fill

;----------------------
; | SCENE_SCROLL_END
; | Description: mode.c's SCENE_SCROLL_END, where scene 1's scroll stops.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/mode.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
SCENE_SCROLL_END	EQU	$68

;----------------------
; | g_scene_step
; | Description: game_scene_step: counts the mode wait down; scene 1 starts and scrolls, scene 2 starts, counts down and raises the lap with a sound; a ready fire button then sets mode 3.
; | Author: suinevere
; | Dependencies: g_reels_load_raised, g_sound_request, SCENE_SCROLL_END, rrl_ram.inc (HW_MODE_WAIT, HW_BTN_NEW, HW_SCENE_ID, HW_MODE_DONE, HW_SCRATCH_WAIT, HW_SCRATCH_FIRST, HW_SCRATCH_COUNT, HW_LAP_MARK, HW_MODE)
; | Globals: mode_wait, btn_new, scene_id, mode_done, scratch_wait, scratch_first, scratch_count, lap_mark, mode and what g_reels_load_raised writes
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_scene_step:
	LD HW_MODE_WAIT
	BZ g_scene_step_waited
	DEC ACC
	ST HW_MODE_WAIT
g_scene_step_waited:
	MOV #$00, B
	BNZ g_scene_step_scene
	LD HW_BTN_NEW
	AND #$01
	ST B
g_scene_step_scene:
	LD HW_SCENE_ID
	BNE #$01, g_scene_step_two
	LD HW_MODE_DONE
	BE #$01, g_scene_step_scroll
	LD B
	BZ g_scene_step_ret
	MOV #SCENE_SCROLL_END, C
	LD HW_SCRATCH_WAIT
	BE #$FF, g_scene_step_first
	MOV #$00, C
g_scene_step_first:
	LD C
	ST HW_SCRATCH_FIRST
	MOV #$01, HW_MODE_DONE
	MOV #20, HW_SCRATCH_COUNT
	RET
g_scene_step_scroll:
	LD HW_SCRATCH_FIRST
	BE #SCENE_SCROLL_END, g_scene_step_count
	INC HW_SCRATCH_FIRST
g_scene_step_count:
	LD HW_SCRATCH_COUNT
	DEC ACC
	BNZ g_scene_step_store
	MOV #20, ACC
g_scene_step_store:
	ST HW_SCRATCH_COUNT
	BR g_scene_step_ready
g_scene_step_two:
	BNE #$02, g_scene_step_ready
	LD HW_MODE_DONE
	BE #$01, g_scene_step_lap
	LD B
	BZ g_scene_step_ret
	MOV #40, HW_SCRATCH_COUNT
	MOV #$01, HW_MODE_DONE
	RET
g_scene_step_lap:
	LD HW_SCRATCH_COUNT
	BZ g_scene_step_fire
	DEC ACC
	ST HW_SCRATCH_COUNT
	BNZ g_scene_step_ret
	LD HW_LAP_MARK
	BE #$09, g_scene_step_mark
	INC ACC
g_scene_step_mark:
	ST HW_LAP_MARK
	CALLF g_reels_load_raised
	MOV #$01, ACC
	JMPF g_sound_request
g_scene_step_fire:
	LD HW_BTN_NEW
	AND #$01
	ST B
g_scene_step_ready:
	LD B
	BZ g_scene_step_ret
	MOV #$03, HW_MODE
g_scene_step_ret:
	RET

;----------------------
; | g_race_lap
; | Description: race_lap: scene lap, the race limit at minus twice sprite (lap * 2 + 0x50)'s width plus 0x20, the race position at 0x40 and mode_done 3.
; | Author: suinevere
; | Dependencies: app_tables, ROM_SPRITES_OFF, rrl_ram.inc (HW_SCENE_ID, HW_SCRATCH_FIRST, HW_SCRATCH_WAIT, HW_MODE_DONE), g_tr_x16
; | Globals: scene_id, scratch_first and the byte after it (RACE_LIMIT), scratch_wait and the byte after it (RACE_POS), mode_done
; | Params: ACC -- lap
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, PSW.
;----------------------
g_race_lap:
	ST HW_SCENE_ID
	ST TRL
	MOV #$00, TRH
	CALLF g_tr_x16
	LD TRL
	ADD #<(app_tables+ROM_SPRITES_OFF+$280)
	ST TRL
	LD TRH
	ADDC #>(app_tables+ROM_SPRITES_OFF+$280)
	ST TRH
	MOV #$02, ACC
	LDC
	ADD #$20
	ST C
	MOV #$00, ACC
	ADDC #$00
	ST B
	LD C
	ADD C
	ST C
	LD B
	ADDC B
	ST B
	MOV #$00, ACC
	SUB C
	ST HW_SCRATCH_FIRST
	MOV #$00, ACC
	SUBC B
	ST HW_SCRATCH_FIRST+1
	MOV #$40, HW_SCRATCH_WAIT
	MOV #$00, HW_SCRATCH_WAIT+1
	MOV #$03, HW_MODE_DONE
	RET

;----------------------
; | g_mode0_leave
; | Description: game_mode0_leave: mode 0, mode_done 0 and a 30-frame wait, with the three reels loaded.
; | Author: suinevere
; | Dependencies: g_reels_load, rrl_ram.inc (HW_MODE, HW_MODE_DONE, HW_MODE_WAIT)
; | Globals: mode, mode_done, mode_wait and what g_reels_load writes
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode0_leave:
	MOV #$00, HW_MODE
	MOV #$00, HW_MODE_DONE
	MOV #30, HW_MODE_WAIT
	JMPF g_reels_load

;----------------------
; | g_mode10_leave
; | Description: game_mode10_leave: mode_done 0, a 5-frame wait and marker 0 started as type 0x5B.
; | Author: suinevere
; | Dependencies: g_markers_clear, g_marker_start, rrl_ram.inc (HW_MODE_DONE, HW_MODE_WAIT)
; | Globals: mode_done, mode_wait, marker
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode10_leave:
	MOV #$00, HW_MODE_DONE
	MOV #$05, HW_MODE_WAIT
	CALLF g_markers_clear
	MOV #$00, ACC
	MOV #$5B, B
	MOV #$00, C
	JMPF g_marker_start

;----------------------
; | g_reel_marker
; | Description: reel_marker: sets mode_done and mode_wait and starts marker 0 as type.
; | Author: suinevere
; | Dependencies: g_markers_clear, g_marker_start, rrl_ram.inc (HW_MODE_DONE, HW_MODE_WAIT)
; | Globals: mode_done, mode_wait, marker
; | Params: ACC -- done; B -- wait; C -- type
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reel_marker:
	ST HW_MODE_DONE
	LD B
	ST HW_MODE_WAIT
	CALLF g_markers_clear
	LD C
	ST B
	MOV #$00, C
	MOV #$00, ACC
	JMPF g_marker_start

;----------------------
; | g_mode12_leave
; | Description: mode12_leave: a 5-frame wait and marker 0 started as type 0x61.
; | Author: suinevere
; | Dependencies: g_markers_clear, g_marker_start, rrl_ram.inc (HW_MODE_WAIT)
; | Globals: mode_wait, marker
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode12_leave:
	MOV #$05, HW_MODE_WAIT
	CALLF g_markers_clear
	MOV #$00, ACC
	MOV #$61, B
	MOV #$00, C
	JMPF g_marker_start

;----------------------
; | g_mode15_leave
; | Description: mode15_leave: scene_id becomes a how-2 item pick, then a 5-frame wait.
; | Author: suinevere
; | Dependencies: g_pick_item, rrl_ram.inc (HW_SCENE_ID, HW_MODE_WAIT)
; | Globals: scene_id, mode_wait and what g_pick_item reads
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode15_leave:
	MOV #$02, ACC
	CALLF g_pick_item
	LD TRL
	ST HW_SCENE_ID
	MOV #$05, HW_MODE_WAIT
	RET

;----------------------
; | g_reel_miss
; | Description: reel_miss: mode_done 5, a 5-frame wait and scratch_wait a how-0 item pick.
; | Author: suinevere
; | Dependencies: g_pick_item, rrl_ram.inc (HW_MODE_DONE, HW_MODE_WAIT, HW_SCRATCH_WAIT)
; | Globals: mode_done, mode_wait, scratch_wait and what g_pick_item reads
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reel_miss:
	MOV #$05, HW_MODE_DONE
	MOV #$05, HW_MODE_WAIT
	MOV #$00, ACC
	CALLF g_pick_item
	LD TRL
	ST HW_SCRATCH_WAIT
	RET

;----------------------
; | g_mode9_leave
; | Description: game_mode9_leave: scene 0 raises the meter, scene 1 trades a random bag item for a new pick, others raise reel A; then marker 0 starts as scene + 0x58.
; | Author: suinevere
; | Dependencies: g_random, g_bag_remove, g_pick_item, g_reels_load_raised, g_markers_clear, g_marker_start, rrl_ram.inc (HW_MODE_WAIT, HW_MODE_DONE, HW_SCENE_ID, HW_METER_MAX, HW_METER_SHOWN, HW_METER, HW_METER_DELAY, HW_SCRATCH_WAIT, HW_SCRATCH_KINDS, HWW_BAG_COUNT, HWW_BAG_ITEMS), g_tr_x16
; | Globals: the fields above, seed, bag_items, marker and what the callees write
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode9_leave:
	MOV #$05, HW_MODE_WAIT
	MOV #$00, HW_MODE_DONE
	LD HW_SCENE_ID
	BNZ g_mode9_leave_one
	LD HW_METER_MAX
	BE HW_METER_SHOWN, g_mode9_leave_top
	LD HW_METER_SHOWN
	INC ACC
	BR g_mode9_leave_meter
g_mode9_leave_top:
	BE #$09, g_mode9_leave_meter
	INC ACC
	ST HW_METER_MAX
g_mode9_leave_meter:
	ST HW_METER
	MOV #$00, HW_METER_DELAY
	JMPF g_mode9_leave_mark
g_mode9_leave_one:
	BE #$01, g_mode9_leave_bag
	CALLF g_reels_load_raised
	JMPF g_mode9_leave_mark
g_mode9_leave_bag:
	CALLF g_mode9_count
	BZ g_mode9_leave_empty
	CALLF g_tr_x16
	CALLF g_random
	LD TRL
	ROR
	ROR
	ROR
	ROR
	AND #$0F
	ST B
	LD TRH
	ROL
	ROL
	ROL
	ROL
	AND #$F0
	OR B
	PUSH ACC
	CALLF g_mode9_item
	LD VTRBF
	ST HW_SCRATCH_WAIT
	POP ACC
	MOV #$01, B
	CALLF g_bag_remove
	BR g_mode9_leave_pick
g_mode9_leave_empty:
	MOV #$FF, HW_SCRATCH_WAIT
g_mode9_leave_pick:
	MOV #$03, ACC
	CALLF g_pick_item
	LD TRL
	ST HW_SCRATCH_KINDS
	PUSH ACC
	CALLF g_mode9_count
	LD $00
	PUSH ACC
	CLR1 VSEL, 4
	MOV #<HWW_BAG_COUNT, VRMAD1
	MOV #>HWW_BAG_COUNT, VRMAD2
	LD VTRBF
	ADD #$01
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	POP ACC
	CALLF g_mode9_item
	POP ACC
	ST VTRBF
g_mode9_leave_mark:
	CALLF g_markers_clear
	LD HW_SCENE_ID
	ADD #$58
	ST B
	MOV #$00, ACC
	MOV #$00, C
	JMPF g_marker_start

;----------------------
; | g_mode9_count
; | Description: Reads bag_count for g_mode9_leave: its low byte in R0 and TRL, bits 8-15 in TRH, and whether any byte is set.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_BAG_COUNT)
; | Globals: bag_count
; | Params: N/A
; | Returns: ACC -- the four bytes ORed; R0, TRL -- byte 0; TRH -- byte 1. Destroys VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_mode9_count:
	CLR1 VSEL, 4
	MOV #<HWW_BAG_COUNT, VRMAD1
	MOV #>HWW_BAG_COUNT, VRMAD2
	LD VTRBF
	ST $00
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
	OR $00
	INC VRMAD1
	OR VTRBF
	INC VRMAD1
	OR VTRBF
	RET

;----------------------
; | g_mode9_item
; | Description: Points VRMAD at bag_items[ACC] for g_mode9_leave.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_BAG_ITEMS)
; | Globals: N/A
; | Params: ACC -- the index
; | Returns: N/A. Destroys ACC, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode9_item:
	CLR1 VSEL, 4
	ADD #<HWW_BAG_ITEMS
	ST VRMAD1
	MOV #>HWW_BAG_ITEMS, ACC
	ADDC #$00
	ST VRMAD2
	RET

;----------------------
; | g_mode11_leave
; | Description: game_mode11_leave: saves the walk; scene 13 starts the race intro, other scenes pick the bag kind by pet match, start the pet's anim and three markers, and sound 2.
; | Author: suinevere
; | Dependencies: g_anim_start, g_byte_is, g_pick_bag_nth, g_markers_clear, g_marker_start, g_sound_begin, rrl_ram.inc (HW_SEL_C, HW_SEL_D, HW_SEL_E, HW_CELL_SAVED, HW_FACING_SAVED, HW_BAG_KIND_COUNT, HW_SCENE_ID, HW_SCRATCH_COUNT, HW_MODE_DONE, HW_MODE_WAIT, HW_MODE_PHASE, HW_SCRATCH_LO, HW_SCRATCH_HI, HW_SCRATCH_WAIT, HWW_ANIM, HWW_PET_KIND)
; | Globals: the fields above, anim, marker and what the callees write
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode11_leave:
	MOV #$00, HW_SEL_E
	LD HW_SEL_C
	ST HW_CELL_SAVED
	LD HW_SEL_D
	ST HW_FACING_SAVED
	MOV #$00, HW_BAG_KIND_COUNT
	CLR1 VSEL, 4
	MOV #<(HWW_ANIM+1), VRMAD1
	MOV #>(HWW_ANIM+1), VRMAD2
	LD HW_SCENE_ID
	BNE #13, g_mode11_leave_pet
	MOV #$04, HW_SCRATCH_COUNT
	MOV #10, HW_MODE_DONE
	MOV #$20, HW_MODE_WAIT
	MOV #$40, HW_MODE_PHASE
	MOV #$00, HW_SCRATCH_LO
	MOV #$00, HW_SCRATCH_HI
	LD VTRBF
	ST HW_SCRATCH_WAIT
	MOV #$01, ACC
	MOV #$15, B
	MOV #$00, C
	JMPF g_anim_start
g_mode11_leave_pet:
	MOV #<HWW_PET_KIND, VRMAD1
	MOV #>HWW_PET_KIND, VRMAD2
	LD VTRBF
	ST C
	LD HW_SCENE_ID
	ST B
	LD C
	CALLF g_byte_is
	BZ g_mode11_leave_back
	MOV #$01, HW_BAG_KIND_COUNT
	BR g_mode11_leave_pick
g_mode11_leave_back:
	LD C
	ST B
	LD HW_SCENE_ID
	CALLF g_byte_is
	BZ g_mode11_leave_pick
	MOV #$02, HW_BAG_KIND_COUNT
g_mode11_leave_pick:
	CALLF g_pick_bag_nth
	MOV #$00, HW_MODE_DONE
	CLR1 VSEL, 4
	MOV #<(HWW_ANIM+1), VRMAD1
	MOV #>(HWW_ANIM+1), VRMAD2
	LD VTRBF
	ST HW_SCRATCH_WAIT
	MOV #$00, VTRBF
	LD HW_SCENE_ID
	ADD #$08
	ST B
	MOV #$02, C
	MOV #$01, ACC
	CALLF g_anim_start
	MOV #$28, HW_MODE_WAIT
	MOV #$14, HW_MODE_PHASE
	CALLF g_markers_clear
	MOV #$00, ACC
	MOV #$62, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$01, ACC
	MOV #$69, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$02, ACC
	MOV #$63, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$02, ACC
	JMPF g_sound_begin

;----------------------
; | g_event_shake
; | Description: game_event_shake: counts mode_wait down, shaking scratch_lo, or for type 11 animating anim 5 and the pet; type 11 ending starts the three race markers and sound 2.
; | Author: suinevere
; | Dependencies: g_shake_offset, g_markers_clear, g_marker_start, g_sound_begin, rrl_ram.inc (HW_MODE_WAIT, HW_MODE_DONE, HW_MODE_PHASE, HW_SCRATCH_LO, HW_SCRATCH_HI, HWW_ANIM)
; | Globals: mode_wait, mode_done, mode_phase, scratch_lo, scratch_hi, anim, marker and what g_sound_begin writes
; | Params: ACC -- type
; | Returns: ACC -- 0 when type 11's wait has run out, else 1. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_event_shake:
	BE #11, g_event_shake_pet
	LD HW_MODE_WAIT
	BNZ g_event_shake_lo
	MOV #11, HW_MODE_DONE
	MOV #$00, HW_SCRATCH_LO
	MOV #$50, HW_MODE_WAIT
	MOV #$01, ACC
	RET
g_event_shake_lo:
	DEC ACC
	ST HW_MODE_WAIT
	CALLF g_shake_offset
	ST HW_SCRATCH_LO
	MOV #$01, ACC
	RET
g_event_shake_pet:
	CLR1 VSEL, 4
	MOV #>HWW_ANIM, VRMAD2
	LD HW_MODE_WAIT
	BNZ g_event_shake_frame
	MOV #<(HWW_ANIM+1), VRMAD1
	MOV #$00, VTRBF
	MOV #<(HWW_ANIM+5), VRMAD1
	MOV #$02, VTRBF
	MOV #$28, HW_MODE_WAIT
	MOV #$14, HW_MODE_PHASE
	CALLF g_markers_clear
	MOV #$00, ACC
	MOV #$62, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$01, ACC
	MOV #$69, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$02, ACC
	MOV #$63, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$02, ACC
	CALLF g_sound_begin
	MOV #$00, ACC
	RET
g_event_shake_frame:
	DEC ACC
	ST HW_MODE_WAIT
	ST B
	ROR
	AND #$03
	ST C
	AND #$02
	BNZ g_event_shake_anim
	LD C
	XOR #$01
	ST C
g_event_shake_anim:
	MOV #<(HWW_ANIM+5), VRMAD1
	LD C
	ST VTRBF
	LD B
	AND #$F0
	BZ g_event_shake_hi
	LD B
	SUB #16
	CLR1 PSW, 7
	RORC
	ST HW_MODE_PHASE
	MOV #$01, ACC
	RET
g_event_shake_hi:
	LD B
	CALLF g_shake_offset
	ST HW_SCRATCH_HI
	MOV #$00, HW_MODE_PHASE
	MOV #$01, ACC
	RET

;----------------------
; | g_reel_race_step
; | Description: game_reel_race_step: one frame of the race scene: fire resets it outside the waits; spins the reels, waits between laps, moves the racer and starts the next lap until lap 4.
; | Author: suinevere
; | Dependencies: g_sound_begin, g_race_reset, g_reels_step, g_race_wait, g_race_lap, g_reels_load, rrl_ram.inc (HW_MODE_DONE, HW_BTN_NEW, HW_REEL_PHASE, HW_MODE, HW_MODE_WAIT, HW_SCRATCH_WAIT, HW_SCRATCH_FIRST, HW_SCENE_ID, HW_REEL_POS_A, HW_REEL_POS_B, HW_REEL_POS_C)
; | Globals: the fields above (scratch_wait and scratch_first with the bytes after them are RACE_POS and RACE_LIMIT) and what the callees write
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reel_race_step:
	LD HW_BTN_NEW
	AND #$01
	BZ g_reel_race_step_state
	LD HW_MODE_DONE
	BE #$02, g_reel_race_step_state
	BE #$05, g_reel_race_step_state
	MOV #$03, ACC
	CALLF g_sound_begin
	JMPF g_race_reset
g_reel_race_step_state:
	LD HW_MODE_DONE
	BNE #$01, g_reel_race_step_wait
	CALLF g_reels_step
	LD HW_REEL_PHASE
	BNE #$04, g_reel_race_step_ret
	JMPF g_race_reset
g_reel_race_step_wait:
	BE #$02, g_reel_race_step_hold
	BNE #$05, g_reel_race_step_3
g_reel_race_step_hold:
	LD HW_BTN_NEW
	AND #$01
	BZ g_reel_race_step_count
	MOV #$01, HW_MODE
	MOV #$03, ACC
	JMPF g_sound_begin
g_reel_race_step_count:
	CALLF g_race_wait
	BZ g_reel_race_step_ret
	LD HW_MODE_DONE
	BNE #$02, g_reel_race_step_rest
	MOV #$00, ACC
	JMPF g_race_lap
g_reel_race_step_rest:
	MOV #$1E, HW_MODE_WAIT
	MOV #$00, HW_MODE_DONE
g_reel_race_step_ret:
	RET
g_reel_race_step_3:
	BNE #$03, g_reel_race_step_4
	LD HW_SCRATCH_FIRST+1
	XOR #$80
	ST C
	LD HW_SCRATCH_WAIT+1
	XOR #$80
	ST B
	LD HW_SCRATCH_FIRST
	SUB HW_SCRATCH_WAIT
	LD C
	SUBC B
	MOV #$00, C
	BN PSW, 7, g_reel_race_step_move
	MOV #$01, C
g_reel_race_step_move:
	LD HW_SCRATCH_WAIT
	SUB #$01
	ST HW_SCRATCH_WAIT
	LD HW_SCRATCH_WAIT+1
	SUBC #$00
	ST HW_SCRATCH_WAIT+1
	LD C
	BNZ g_reel_race_step_ret
	LD HW_SCENE_ID
	INC ACC
	BE #$04, g_reel_race_step_last
	JMPF g_race_lap
g_reel_race_step_last:
	MOV #$78, HW_MODE_WAIT
	MOV #$04, HW_MODE_DONE
	RET
g_reel_race_step_4:
	BNE #$04, g_reel_race_step_other
	CALLF g_race_wait
	BZ g_reel_race_step_ret
	MOV #$00, HW_REEL_POS_A
	MOV #$00, HW_REEL_POS_A+1
	MOV #$00, HW_REEL_POS_B
	MOV #$00, HW_REEL_POS_B+1
	MOV #$00, HW_REEL_POS_C
	MOV #$00, HW_REEL_POS_C+1
	MOV #$F0, HW_MODE_WAIT
	MOV #$05, HW_MODE_DONE
	RET
g_reel_race_step_other:
	CALLF g_race_wait
	BZ g_reel_race_step_ret
	CALLF g_reels_load
	INC HW_MODE_DONE
	RET

;----------------------
; | g_mode_leave
; | Description: game_mode_leave: runs mode's leave routine from the sixteen-entry table, the mode's low four bits picking it.
; | Author: suinevere
; | Dependencies: g_mode_leaves
; | Globals: what the leave routines write
; | Params: ACC -- mode
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode_leave:
	AND #$0F
	ST C
	ADD C
	ADD C
	ADD #<g_mode_leaves
	ST TRL
	MOV #>g_mode_leaves, ACC
	ADDC #$00
	ST TRH
	PUSH TRL
	PUSH TRH
	RET

;----------------------
; | g_mode_leaves
; | Description: game_mode_leave's table: one JMPF per mode 0-15 to its leave routine, as mode.c's leave[] lists them.
; | Author: suinevere
; | Dependencies: the g_mode*_leave routines, g_reels_load_single, g_board_enter, g_mode12_leave, g_mode15_leave
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
g_mode_leaves:
	JMPF g_mode0_leave
	JMPF g_mode1_leave
	JMPF g_mode2_leave
	JMPF g_mode3_leave
	JMPF g_mode4_leave
	JMPF g_mode5_leave
	JMPF g_reels_load_single
	JMPF g_mode7_leave
	JMPF g_board_enter
	JMPF g_mode9_leave
	JMPF g_mode10_leave
	JMPF g_mode11_leave
	JMPF g_mode12_leave
	JMPF g_mode13_leave
	JMPF g_mode14_leave
	JMPF g_mode15_leave

;----------------------
; | g_ab_at
; | Description: Point VRMAD at ARG_BLOCK + ACC with VSEL bit 4 clear, for saving bytes a callee borrows: the caller pushes or pops VTRBF itself.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- the offset
; | Returns: N/A. Destroys ACC, VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_ab_at:
	CLR1 VSEL, 4
	ADD #<ARG_BLOCK
	ST VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	RET

;----------------------
; | g_reel_game_step
; | Description: game_reel_game_step: one frame of the slot game: starts the spin, picks or misses, and when the reel stops on a drawn symbol rolls between two prizes and a board event.
; | Author: suinevere
; | Dependencies: g_ab_at, g_strip_next_symbol, g_reel_miss, g_random, g_reel_marker, g_event_roll, g_reels_load_pick, g_bag_add, REEL_STRIPS, rrl_ram.inc (HW_MODE_DONE, HW_BTN_NEW, HW_REEL_PHASE, HW_REEL_LEN, HW_REEL_POS_A, HW_SCRATCH_KINDS, HW_SCRATCH_FIRST, HW_SCRATCH_COUNT, HW_MODE, HW_MODE_WAIT, HW_METER, HW_METER_SHOWN, HW_METER_DELAY, HW_WALK_LEFT, HW_WALK_BACK, HW_SCRATCH_WAIT)
; | Globals: the fields above and what the callees write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; three bytes of stack on top of its callees'.
;----------------------
g_reel_game_step:
	LD HW_MODE_DONE
	BNE #$02, g_reel_game_step_wait
	LD HW_REEL_PHASE
	BE #$08, g_reel_game_step_stop
	RET
g_reel_game_step_stop:
	MOV #$00, ACC
	CALLF g_ab_at
	PUSH VTRBF
	MOV #<REEL_STRIPS, VTRBF
	INC VRMAD1
	PUSH VTRBF
	MOV #>REEL_STRIPS, VTRBF
	LD HW_REEL_POS_A
	ST TRL
	LD HW_REEL_POS_A+1
	ST TRH
	LD HW_REEL_LEN
	CALLF g_strip_next_symbol
	ST B
	MOV #$01, ACC
	CALLF g_ab_at
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	LD B
	BE HW_SCRATCH_KINDS, g_reel_game_step_hit
	BE HW_SCRATCH_FIRST, g_reel_game_step_hit
	BE HW_SCRATCH_COUNT, g_reel_game_step_hit
	JMPF g_reel_miss
g_reel_game_step_hit:
	MOV #100, TRL
	MOV #$00, TRH
	CALLF g_random
	LD TRL
	SUB #20
	BN PSW, 7, g_reel_game_step_70
	MOV #$03, ACC
	MOV #$05, B
	MOV #$5D, C
	JMPF g_reel_marker
g_reel_game_step_70:
	LD TRL
	SUB #70
	BN PSW, 7, g_reel_game_step_event
	MOV #$04, ACC
	MOV #10, B
	MOV #$5E, C
	JMPF g_reel_marker
g_reel_game_step_event:
	CALLF g_event_roll
	MOV #11, HW_MODE
	RET
g_reel_game_step_wait:
	LD HW_MODE_WAIT
	BZ g_reel_game_step_state
	DEC HW_MODE_WAIT
	RET
g_reel_game_step_state:
	LD HW_MODE_DONE
	SUB #$06
	BP PSW, 7, g_reel_game_step_fire
	MOV #$03, HW_MODE
	RET
g_reel_game_step_fire:
	LD HW_BTN_NEW
	AND #$01
	BNZ g_reel_game_step_go
	RET
g_reel_game_step_go:
	LD HW_MODE_DONE
	BNZ g_reel_game_step_1
	MOV #100, TRL
	MOV #$00, TRH
	CALLF g_random
	LD TRL
	SUB #80
	BP PSW, 7, g_reel_game_step_spin
	JMPF g_reel_miss
g_reel_game_step_spin:
	MOV #$01, ACC
	MOV #$05, B
	MOV #$5C, C
	JMPF g_reel_marker
g_reel_game_step_1:
	BNE #$01, g_reel_game_step_3
	MOV #$02, HW_MODE_DONE
	MOV #$00, ACC
	CALLF g_ab_at
	PUSH VTRBF
	INC VRMAD1
	PUSH VTRBF
	INC VRMAD1
	PUSH VTRBF
	CALLF g_reels_load_pick
	MOV #$02, ACC
	CALLF g_ab_at
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	RET
g_reel_game_step_3:
	BNE #$03, g_reel_game_step_4
	LD HW_METER_SHOWN
	DEC ACC
	ST HW_METER
	MOV #$00, HW_METER_DELAY
	MOV #$03, HW_MODE
	RET
g_reel_game_step_4:
	BNE #$04, g_reel_game_step_bag
	MOV #$30, TRL
	MOV #$00, TRH
	CALLF g_random
	LD TRL
	ROR
	ROR
	ROR
	ROR
	AND #$0F
	ADD #$03
	ST HW_WALK_LEFT
	MOV #$01, HW_WALK_BACK
	MOV #$07, HW_MODE
	RET
g_reel_game_step_bag:
	LD HW_SCRATCH_WAIT
	JMPF g_bag_add

;----------------------
; | g_reel_pair_step
; | Description: game_reel_pair_step: one frame of the two-reel game: loads and spins the pair, then by how the reels compare pays out, offers an item, lowers the meter or shakes, waiting and needing fire between steps.
; | Author: suinevere
; | Dependencies: g_ab_at, g_marker_used, g_reels_load_pair, g_reels_next_two, g_markers_clear, g_marker_start, g_payout, g_random, g_pick_item, g_pair_end, g_bag_add, g_event_shake, rrl_ram.inc (HW_MODE_DONE, HW_BTN_NEW, HW_MODE_WAIT, HW_MODE_PHASE, HW_REEL_PHASE, HW_SCRATCH_KINDS, HW_SCRATCH_FIRST, HW_WAIT_HI, HW_SCENE_ID, HW_BAG_NTH_INDEX, HW_METER_DELAY, HW_METER_SHOWN, HW_METER, HWW_ANIM)
; | Globals: the fields above, anim, marker and what the callees write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; four bytes of stack on top of its callees'.
;----------------------
g_reel_pair_step:
	LD HW_MODE_DONE
	SUB #$03
	BP PSW, 7, g_reel_pair_step_go
	LD HW_MODE_DONE
	SUB #10
	BN PSW, 7, g_reel_pair_step_go
	LD HW_MODE_WAIT
	BZ g_reel_pair_step_fire
	DEC HW_MODE_WAIT
	RET
g_reel_pair_step_fire:
	LD HW_BTN_NEW
	AND #$01
	BNZ g_reel_pair_step_go
	RET
g_reel_pair_step_go:
	LD HW_MODE_DONE
	BNZ g_reel_pair_step_1
	LD HW_MODE_WAIT
	BZ g_reel_pair_step_phase
	DEC HW_MODE_WAIT
	BR g_reel_pair_step_used
g_reel_pair_step_phase:
	LD HW_MODE_PHASE
	BZ g_reel_pair_step_used
	SUB #$02
	ST HW_MODE_PHASE
g_reel_pair_step_used:
	MOV #$00, ACC
	CALLF g_marker_used
	BZ g_reel_pair_step_load
	RET
g_reel_pair_step_load:
	MOV #$00, ACC
	CALLF g_ab_at
	PUSH VTRBF
	CALLF g_reels_load_pair
	MOV #$00, ACC
	CALLF g_ab_at
	POP VTRBF
	MOV #$01, ACC
	JMPF g_reel_pair_step_done
g_reel_pair_step_1:
	BNE #$01, g_reel_pair_step_3
	LD HW_REEL_PHASE
	BE #$08, g_reel_pair_step_next
	RET
g_reel_pair_step_next:
	CALLF g_reels_next_two
	MOV #$03, ACC
	JMPF g_reel_pair_step_done
g_reel_pair_step_3:
	BNE #$03, g_reel_pair_step_4
	CALLF g_markers_clear
	LD HW_SCRATCH_KINDS
	BNE HW_SCRATCH_FIRST, g_reel_pair_step_less
	MOV #$00, ACC
	MOV #$60, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$07, ACC
	JMPF g_reel_pair_step_done
g_reel_pair_step_less:
	BN PSW, 7, g_reel_pair_step_more
	MOV #$00, ACC
	MOV #$64, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$01, ACC
	MOV #$65, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$02, ACC
	MOV #$6A, B
	MOV #$00, C
	CALLF g_marker_start
	CLR1 VSEL, 4
	MOV #<(HWW_ANIM+1), VRMAD1
	MOV #>(HWW_ANIM+1), VRMAD2
	MOV #$01, VTRBF
	MOV #$08, ACC
	JMPF g_reel_pair_step_done
g_reel_pair_step_more:
	MOV #$00, ACC
	MOV #$5F, B
	MOV #$00, C
	CALLF g_marker_start
	MOV #$04, ACC
	JMPF g_reel_pair_step_done
g_reel_pair_step_4:
	BNE #$04, g_reel_pair_step_5
	SET1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	CLR1 VSEL, 4
	CALLF g_payout
	MOV #$03, ACC
	CALLF g_ab_at
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	MOV #$40, HW_WAIT_HI
	MOV #$05, ACC
	BR g_reel_pair_step_done
g_reel_pair_step_5:
	BNE #$05, g_reel_pair_step_6
	MOV #$06, ACC
	BR g_reel_pair_step_done
g_reel_pair_step_6:
	BNE #$06, g_reel_pair_step_7
	LD HW_SCENE_ID
	BE #13, g_reel_pair_step_item
	MOV #100, TRL
	MOV #$00, TRH
	CALLF g_random
	LD TRL
	SUB #25
	BP PSW, 7, g_reel_pair_step_item
	JMPF g_pair_end
g_reel_pair_step_item:
	MOV #$01, ACC
	CALLF g_pick_item
	LD TRL
	ST HW_BAG_NTH_INDEX
	MOV #$09, ACC
	BR g_reel_pair_step_done
g_reel_pair_step_7:
	BNE #$07, g_reel_pair_step_8
	JMPF g_pair_end
g_reel_pair_step_8:
	BNE #$08, g_reel_pair_step_9
	MOV #$00, HW_METER_DELAY
	LD HW_METER_SHOWN
	DEC ACC
	ST C
	BZ g_reel_pair_step_meter
	LD HW_SCENE_ID
	BNE #13, g_reel_pair_step_meter
	DEC C
g_reel_pair_step_meter:
	LD C
	ST HW_METER
	JMPF g_pair_end
g_reel_pair_step_9:
	BNE #$09, g_reel_pair_step_shake
	LD HW_BAG_NTH_INDEX
	JMPF g_bag_add
g_reel_pair_step_shake:
	CALLF g_event_shake
	BZ g_reel_pair_step_zero
	RET
g_reel_pair_step_zero:
	MOV #$00, ACC
g_reel_pair_step_done:
	ST HW_MODE_DONE
	SUB #$03
	BP PSW, 7, g_reel_pair_step_ret
	MOV #$05, HW_MODE_WAIT
g_reel_pair_step_ret:
	RET

;----------------------
; | g_mode_leave_changed
; | Description: game_mode_leave_changed: runs the current mode's leave handler unless the mode equals was.
; | Author: suinevere
; | Dependencies: g_mode_leave, game.inc (ARG_BLOCK), rrl_ram.inc (HW_MODE)
; | Globals: mode and what the leave handlers read and write
; | Params: ARG_BLOCK+0..3 -- was, 32-bit
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode_leave_changed:
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+1), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	LD VTRBF
	OR VTRBF
	OR VTRBF
	CLR1 VSEL, 4
	BNZ g_mode_leave_changed_run
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST B
	LD HW_MODE
	BE B, g_mode_leave_changed_ret
g_mode_leave_changed_run:
	LD HW_MODE
	JMPF g_mode_leave
g_mode_leave_changed_ret:
	RET

;----------------------
; | g_mode_advance
; | Description: game_mode_advance: moves to the next mode and runs its leave handler unless it equals was (mode 3 compares with sel_b); mode 12 instead resets the state block and parks the mode at 0xFF.
; | Author: suinevere
; | Dependencies: g_state_reset, g_mode_leave_changed, game.inc (ARG_BLOCK), rrl_ram.inc (HW_MODE, HW_SEL_B)
; | Globals: mode, sel_b, the state block and what the leave handlers read and write
; | Params: ARG_BLOCK+0..3 -- was, 32-bit
; | Returns: N/A. Keeps ARG_BLOCK+4..12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW and, from mode 3, ARG_BLOCK+0..3; leaves VSEL bit 4 clear.
;----------------------
g_mode_advance:
	LD HW_MODE
	BNE #12, g_mode_advance_next
	CALLF g_state_reset
	MOV #$FF, ACC
	ST HW_MODE
	RET
g_mode_advance_next:
	BNE #$02, g_mode_advance_3
	MOV #13, ACC
	BR g_mode_advance_set
g_mode_advance_3:
	BNE #$03, g_mode_advance_8
	MOV #$06, B
	LD HW_SEL_B
	BNE #$02, g_mode_advance_was
	MOV #$04, B
g_mode_advance_was:
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	ST VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
	LD B
	BR g_mode_advance_set
g_mode_advance_8:
	BNE #$08, g_mode_advance_low
	MOV #$00, ACC
	BR g_mode_advance_set
g_mode_advance_low:
	SUB #7
	BN PSW, 7, g_mode_advance_rest
	LD HW_MODE
	INC ACC
	BR g_mode_advance_set
g_mode_advance_rest:
	MOV #$03, ACC
g_mode_advance_set:
	ST HW_MODE
	JMPF g_mode_leave_changed

;----------------------
; | g_was_5c4 / g_was_byte
; | Description: Sets the was slot ARG_BLOCK+0..3 to 0x5C4, which no mode matches, or to the byte in ACC.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- the byte (g_was_byte)
; | Returns: ARG_BLOCK+0..3 -- was. Destroys ACC, VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_was_5c4:
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+1), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #$05, VTRBF
	DEC VRMAD1
	MOV #$C4, VTRBF
	MOV #<(ARG_BLOCK+2), VRMAD1
	BR g_was_top
g_was_byte:
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	ST VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
g_was_top:
	SET1 VSEL, 4
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
	RET

;----------------------
; | g_mode_press
; | Description: game_mode_press: a press plays sound 3 and advances the mode, comparing with 0x5C4.
; | Author: suinevere
; | Dependencies: g_keep13, g_sound_begin, g_was_5c4, g_mode_advance, rrl_ram.inc (HW_BTN_NEW)
; | Globals: btn_new and what the sound and the mode change write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; eighteen bytes of stack on top of the mode change's.
;----------------------
g_mode_press:
	LD HW_BTN_NEW
	BN ACC, 0, g_mode_press_ret
	MOV #<g_mode_press_body, $00
	MOV #>g_mode_press_body, $01
	JMPF g_keep13
g_mode_press_body:
	MOV #$03, ACC
	CALLF g_sound_begin
	CALLF g_was_5c4
	JMPF g_mode_advance
g_mode_press_ret:
	RET

;----------------------
; | g_board_auto_frame
; | Description: game_board_auto_frame: one frame of an automatic walk and, once it has stopped, the leave handler of the mode its cell's event chose.
; | Author: suinevere
; | Dependencies: g_board_auto_screen, g_cell_event, g_mode_leave, rrl_ram.inc (HW_WALK_LEFT, HW_WALK_BUSY)
; | Globals: walk_left, walk_busy and what the screen, the event and the leave handlers read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_board_auto_frame:
	CALLF g_board_auto_screen
	LD HW_WALK_LEFT
	BNZ g_board_auto_frame_ret
	LD HW_WALK_BUSY
	BNZ g_board_auto_frame_ret
	CALLF g_cell_event
	JMPF g_mode_leave
g_board_auto_frame_ret:
	RET
