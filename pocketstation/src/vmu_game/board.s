;----------------------
; | board.s
; | Description: board.c in LC86K assembly: the bonus and meter cells, the board list's insert and gathering, a set's next argument and the walk's next step. Also entering the board, the meter's update, the random cell choices, a finished step's mode and the choice cells.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/board.c, pocketstation/build/vmu_game/rrl_ram.inc, game.inc
; | Globals: bonus_done, scene_id, meter, meter_shown, meter_max, meter_delay, step_result, lap_mark, board_list, walk_behind, walk_left, walk_back, steps, odds_set, mode, mode_done, mode_phase, mode_wait, walk_busy, walk_frame, walk_kind, cell_saved, facing_saved, sel_c, sel_d, sel_e, scratch_kinds, sound_req, cell_arg, cell_type, seed
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_cell_bonus
; | Description: cell_bonus: marks the bonus done, sets scene_id 0x0D and returns 11.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_BONUS_DONE, HW_SCENE_ID)
; | Globals: bonus_done, scene_id
; | Params: N/A
; | Returns: ACC -- 11. Destroys nothing else.
;----------------------
g_cell_bonus:
	MOV #$01, HW_BONUS_DONE
	MOV #$0D, HW_SCENE_ID
	MOV #$0B, ACC
	RET

;----------------------
; | g_cell_meter
; | Description: cell_meter: with an argument, raises the meter one step towards meter_max; without one, unless the step was a back step (step_result 2), grows meter_max by one up to 9 when the meter is full, fills the meter to it and clears lap_mark.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_METER, HW_METER_SHOWN, HW_METER_MAX, HW_METER_DELAY, HW_STEP_RESULT, HW_LAP_MARK)
; | Globals: meter, meter_shown, meter_max, meter_delay, step_result, lap_mark
; | Params: ACC -- arg
; | Returns: N/A. Destroys ACC.
;----------------------
g_cell_meter:
	BZ g_cell_meter_fill
	LD HW_METER_SHOWN
	XOR HW_METER_MAX
	BZ g_cell_meter_ret
	LD HW_METER_SHOWN
	INC ACC
	ST HW_METER
	MOV #$00, HW_METER_DELAY
	RET
g_cell_meter_fill:
	LD HW_STEP_RESULT
	XOR #$02
	BZ g_cell_meter_ret
	LD HW_METER_SHOWN
	XOR HW_METER_MAX
	BNZ g_cell_meter_set
	LD HW_METER_MAX
	XOR #$09
	BZ g_cell_meter_set
	INC HW_METER_MAX
g_cell_meter_set:
	LD HW_METER_MAX
	ST HW_METER
	MOV #$00, HW_METER_DELAY
	MOV #$00, HW_LAP_MARK
g_cell_meter_ret:
	RET

;----------------------
; | g_board_insert
; | Description: board_insert: inserts a (y, x, cell) entry into the work RAM list kept in y order, moving each later entry whose signed y is above y up a place on the way down, and returns the new count.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: board_list (the list the callers pass)
; | Params: TRH:TRL -- list, a vmu_ram offset in work RAM (0x100-0x2FF); ACC -- count, 1 or more; ARG_BLOCK+0..1 -- x; ARG_BLOCK+2..3 -- y, both int16; B -- cell
; | Returns: ACC -- count + 1. Destroys C, TRL, TRH, R0-R3, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear; three bytes of stack.
;----------------------
g_board_insert:
	ST $03
	DEC TRH
	ST C
	ADD C
	ADD C
	SUB #$03
	ADD TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	SET1 VSEL, 4
	MOV #>(ARG_BLOCK+2), VRMAD2
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD VTRBF
	ST $00
	LD VTRBF
	XOR #$80
	ST $01
	LD $03
	ST C
g_board_insert_scan:
	LD TRL
	ST VRMAD1
	LD TRH
	ST VRMAD2
	LD VTRBF
	ST $02
	BN ACC, 7, g_board_insert_plus
	LD $00
	SUB $02
	LD $01
	SUBC #$7F
	BR g_board_insert_test
g_board_insert_plus:
	LD $00
	SUB $02
	LD $01
	SUBC #$80
g_board_insert_test:
	BN PSW, 7, g_board_insert_place
	LD VTRBF
	PUSH ACC
	LD VTRBF
	XCH $02
	ST VTRBF
	POP ACC
	ST VTRBF
	LD $02
	ST VTRBF
	LD TRL
	SUB #$03
	ST TRL
	LD TRH
	SUBC #$00
	ST TRH
	DBNZ C, g_board_insert_scan
g_board_insert_place:
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST C
	LD TRL
	ADD #$03
	ST VRMAD1
	LD TRH
	ADDC #$00
	ST VRMAD2
	LD $00
	ST VTRBF
	LD C
	ST VTRBF
	LD B
	ST VTRBF
	CLR1 VSEL, 4
	LD $03
	INC ACC
	RET

;----------------------
; | g_next_arg
; | Description: next_arg: steps arg on once per bit shifted out of *set until a set bit comes out, and returns it; *set must hold a set bit.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ARG_BLOCK+0..3 -- *set, 32-bit; ACC -- arg
; | Returns: ACC -- the next argument; ARG_BLOCK+0..3 -- *set, shifted past it. Destroys C, R0-R3, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_next_arg:
	ST C
	SET1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST $00
	LD VTRBF
	ST $01
	LD VTRBF
	ST $02
	LD VTRBF
	ST $03
g_next_arg_step:
	INC C
	CLR1 PSW, 7
	LD $03
	RORC
	ST $03
	LD $02
	RORC
	ST $02
	LD $01
	RORC
	ST $01
	LD $00
	RORC
	ST $00
	BN PSW, 7, g_next_arg_step
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD $00
	ST VTRBF
	LD $01
	ST VTRBF
	LD $02
	ST VTRBF
	LD $03
	ST VTRBF
	CLR1 VSEL, 4
	LD C
	RET

;----------------------
; | g_walk_next
; | Description: walk_next: one step of the walk. Walking back it moves walk_behind on, ending on steps; behind it moves walk_behind in; ahead it counts steps on, ending at the odds set's last step, and at each 32-step lap clears lap_mark and raises the meter a step.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_WALK_LEFT, HW_WALK_BACK, HW_WALK_BEHIND, HW_STEPS, HW_STEP_RESULT, HW_LAP_MARK, HW_MODE_DONE, HW_METER_MAX, HW_METER_SHOWN, HW_METER, HW_METER_DELAY, HW_ODDS_SET)
; | Globals: walk_left, walk_back, walk_behind, steps, step_result, lap_mark, mode_done, meter_max, meter_shown, meter, meter_delay, odds_set
; | Params: N/A
; | Returns: ACC -- 0 when the walk back ends where it is, else 1. Destroys C.
;----------------------
g_walk_next:
	DEC HW_WALK_LEFT
	LD HW_WALK_BACK
	BZ g_walk_next_forward
	MOV #$00, HW_STEP_RESULT
	LD HW_WALK_BEHIND
	XOR HW_STEPS
	BNZ g_walk_next_back
	MOV #$00, HW_WALK_LEFT
	RET
g_walk_next_back:
	LD HW_WALK_BEHIND
	INC ACC
	ST HW_WALK_BEHIND
	BZ g_walk_next_one
	XOR HW_STEPS
	BNZ g_walk_next_one
	MOV #$00, HW_WALK_LEFT
	BR g_walk_next_one
g_walk_next_forward:
	LD HW_WALK_BEHIND
	BZ g_walk_next_ahead
	DEC HW_WALK_BEHIND
	MOV #$02, HW_STEP_RESULT
	BR g_walk_next_one
g_walk_next_ahead:
	LD HW_STEPS
	AND #$1F
	BNZ g_walk_next_step
	MOV #$00, HW_LAP_MARK
	LD HW_MODE_DONE
	BNZ g_walk_next_step
	LD HW_METER_MAX
	XOR HW_METER_SHOWN
	BZ g_walk_next_step
	LD HW_METER_SHOWN
	INC ACC
	ST HW_METER
	MOV #$00, HW_METER_DELAY
g_walk_next_step:
	LD HW_STEPS
	INC HW_STEPS
	ST C
	AND #$1F
	XOR #$1F
	BNZ g_walk_next_on
	LD C
	ROL
	ROL
	ROL
	AND #$07
	XOR HW_ODDS_SET
	BNZ g_walk_next_on
	MOV #$03, HW_STEP_RESULT
	MOV #$00, HW_WALK_LEFT
	BR g_walk_next_one
g_walk_next_on:
	MOV #$01, HW_STEP_RESULT
g_walk_next_one:
	MOV #$01, ACC
	RET

;----------------------
; | g_board_gather
; | Description: game_board_gather: moves one ring cell along dir and one isometric step, lists the cell by y when on screen, then follows each new exit until depth runs out.
; | Author: suinevere
; | Dependencies: g_gather_core, game.inc (ARG_BLOCK)
; | Globals: board_list (the list passed)
; | Params: ARG_BLOCK+0..1 -- x; ARG_BLOCK+2..3 -- y, both int16; ACC -- cell; B -- dir (1, 2, 4, 8; 0x80 steps back a cell); C -- depth; ARG_BLOCK+4 -- count; ARG_BLOCK+5..6 -- list, a vmu_ram offset in work RAM
; | Returns: ACC -- the new count. Destroys B, C, TRL, TRH, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear and x, y moved by dir's step.
;----------------------
g_board_gather:
	ST $00
	LD B
	ST $01
	LD C
	ST $02
	CLR1 VSEL, 4
	MOV #>(ARG_BLOCK+4), VRMAD2
	MOV #<(ARG_BLOCK+4), VRMAD1
	LD VTRBF
	ST $03
	CALLF g_gather_core
	LD $03
	RET

;----------------------
; | g_board_branch
; | Description: board_branch: restarts the walk from the junction cells along each exit set in exits.
; | Author: suinevere
; | Dependencies: g_branch_core, game.inc (ARG_BLOCK)
; | Globals: board_list (the list passed)
; | Params: ARG_BLOCK+0..1 -- x; ARG_BLOCK+2..3 -- y; ACC -- exits; B -- depth; ARG_BLOCK+4 -- count; ARG_BLOCK+5..6 -- list
; | Returns: ACC -- the new count. Destroys B, C, TRL, TRH, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_board_branch:
	ST C
	LD B
	ST $02
	CLR1 VSEL, 4
	MOV #>(ARG_BLOCK+4), VRMAD2
	MOV #<(ARG_BLOCK+4), VRMAD1
	LD VTRBF
	ST $03
	LD C
	CALLF g_branch_core
	LD $03
	RET

;----------------------
; | g_gather_core
; | Description: game_board_gather on registers, recursing through itself and g_branch_core; x and y stay in ARG_BLOCK+0..3, moved by dir's step, which each caller undoes after a child call.
; | Author: suinevere
; | Dependencies: g_gather_move, g_board_insert, g_nibble, g_branch_core, game.inc (ARG_BLOCK)
; | Globals: board_list (the list at ARG_BLOCK+5..6)
; | Params: R0 -- cell; R1 -- dir; R2 -- depth; R3 -- count; ARG_BLOCK+0..3 -- x, y; ARG_BLOCK+5..6 -- list; VRMAD2 -- ARG_BLOCK's page
; | Returns: R3 -- the new count. Destroys ACC, B, C, TRL, TRH, R0-R2, VRMAD, PSW; five bytes of stack a level, as the C's recursion nests.
;----------------------
g_gather_core:
	LD $01
	CALLF g_gather_move
	LD $00
	ADD #$01
	BN $01, 7, g_gather_core_cell
	ADD #$1E
g_gather_core_cell:
	AND #$1F
	ST $00
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ADD #$1F
	ST C
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	BNZ g_gather_core_off
	LD C
	SUB #$3F
	BN PSW, 7, g_gather_core_off
	INC VRMAD1
	LD VTRBF
	ADD #$1F
	ST C
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	BNZ g_gather_core_off
	LD C
	SUB #$3F
	BN PSW, 7, g_gather_core_off
	PUSH $00
	PUSH $01
	PUSH $02
	MOV #<(ARG_BLOCK+5), VRMAD1
	LD VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
	LD $00
	ST B
	LD $03
	CALLF g_board_insert
	ST $03
	POP $02
	POP $01
	POP $00
	BR g_gather_core_depth
g_gather_core_off:
	LD $00
	BE #$08, g_gather_core_junction
	BNE #$18, g_gather_core_depth
g_gather_core_junction:
	LD $01
	XOR #$FF
	AND #$0F
	JMPF g_branch_core
g_gather_core_depth:
	LD $02
	BNZ g_gather_core_go
	RET
g_gather_core_go:
	DEC $02
	LD $00
	CALLF g_nibble
	BE #$0F, g_gather_core_junction
	ST C
	LD $01
	XOR #$FF
	AND C
	ST C
	LD $01
	AND #$80
	OR C
	PUSH $00
	PUSH $02
	PUSH ACC
	BN ACC, 0, g_gather_core_c2
	AND #$80
	OR #$08
	ST $01
	CALLF g_gather_core
	MOV #$01, ACC
	CALLF g_gather_move
g_gather_core_c2:
	POP B
	POP $02
	POP $00
	PUSH $00
	PUSH $02
	PUSH B
	BN B, 1, g_gather_core_c3
	LD B
	AND #$80
	OR #$04
	ST $01
	CALLF g_gather_core
	MOV #$02, ACC
	CALLF g_gather_move
g_gather_core_c3:
	POP B
	POP $02
	POP $00
	PUSH $00
	PUSH $02
	PUSH B
	BN B, 2, g_gather_core_c4
	LD B
	AND #$80
	OR #$02
	ST $01
	CALLF g_gather_core
	MOV #$04, ACC
	CALLF g_gather_move
g_gather_core_c4:
	POP B
	POP $02
	POP $00
	BN B, 3, g_gather_core_done
	LD B
	AND #$80
	OR #$01
	ST $01
	CALLF g_gather_core
	MOV #$08, ACC
	CALLF g_gather_move
g_gather_core_done:
	RET

;----------------------
; | g_branch_core
; | Description: board_branch on registers: gathers from cell 8 along 0x82 and 4, then from 0x18 along 0x81 and 8, for exits 4, 2, 8 and 1. Each child's step is undone.
; | Author: suinevere
; | Dependencies: g_gather_core, g_gather_move
; | Globals: board_list
; | Params: ACC -- exits; R2 -- depth; R3 -- count; ARG_BLOCK+0..3 -- x, y; ARG_BLOCK+5..6 -- list
; | Returns: R3 -- the new count. Destroys ACC, B, C, TRL, TRH, R0-R2, VRMAD, PSW; four bytes of stack a level.
;----------------------
g_branch_core:
	PUSH $02
	PUSH ACC
	BN ACC, 2, g_branch_core_2
	MOV #$08, $00
	MOV #$82, $01
	CALLF g_gather_core
	MOV #$04, ACC
	CALLF g_gather_move
g_branch_core_2:
	POP C
	POP $02
	PUSH $02
	PUSH C
	BN C, 1, g_branch_core_8
	MOV #$08, $00
	MOV #$04, $01
	CALLF g_gather_core
	MOV #$02, ACC
	CALLF g_gather_move
g_branch_core_8:
	POP C
	POP $02
	PUSH $02
	PUSH C
	BN C, 3, g_branch_core_1
	MOV #$18, $00
	MOV #$81, $01
	CALLF g_gather_core
	MOV #$08, ACC
	CALLF g_gather_move
g_branch_core_1:
	POP C
	POP $02
	BN C, 0, g_branch_core_done
	MOV #$18, $00
	MOV #$08, $01
	CALLF g_gather_core
	MOV #$01, ACC
	CALLF g_gather_move
g_branch_core_done:
	RET

;----------------------
; | g_gather_move
; | Description: The step game_board_gather takes along dir: x at ARG_BLOCK+0..1 moves 16 (dir bits 0 or 2) or -16. Y at ARG_BLOCK+2..3 moves 8 (dir bits 0 or 1) or -8.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: ARG_BLOCK+0..3
; | Params: ACC -- dir
; | Returns: N/A. Destroys ACC, C, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_gather_move:
	ST C
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	AND #$05
	BZ g_gather_move_xl
	LD VTRBF
	ADD #$10
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	BR g_gather_move_y
g_gather_move_xl:
	LD VTRBF
	SUB #$10
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	SUBC #$00
g_gather_move_y:
	ST VTRBF
	INC VRMAD1
	LD C
	AND #$03
	BZ g_gather_move_yl
	LD VTRBF
	ADD #$08
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	RET
g_gather_move_yl:
	LD VTRBF
	SUB #$08
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	SUBC #$00
	ST VTRBF
	RET

;----------------------
; | g_board_enter
; | Description: game_board_enter: saves the state block unless the meter is empty, then starts the board walk. It sets the phase, wait and walk fields, the saved cell and facing, scene 0x5E and clears the flags.
; | Author: suinevere
; | Dependencies: g_save, rrl_ram.inc (HW_METER, HW_MODE_PHASE, HW_MODE_WAIT, HW_WALK_BUSY, HW_WALK_FRAME, HW_WALK_KIND, HW_CELL_SAVED, HW_SEL_C, HW_FACING_SAVED, HW_SEL_D, HW_SEL_E, HW_SCENE_ID, HW_MODE_DONE, HW_SCRATCH_KINDS)
; | Globals: meter, mode_phase, mode_wait, walk_busy, walk_frame, walk_kind, cell_saved, sel_c, facing_saved, sel_d, sel_e, scene_id, mode_done, scratch_kinds
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0, R1, FPR, PSW (g_save); six bytes of stack.
;----------------------
g_board_enter:
	LD HW_METER
	BZ g_board_enter_set
	CALLF g_save
g_board_enter_set:
	MOV #$00, HW_MODE_PHASE
	MOV #$05, HW_MODE_WAIT
	MOV #$01, HW_WALK_BUSY
	MOV #$00, HW_WALK_FRAME
	MOV #$02, HW_WALK_KIND
	LD HW_SEL_C
	ST HW_CELL_SAVED
	LD HW_SEL_D
	ST HW_FACING_SAVED
	MOV #$00, HW_SEL_E
	MOV #$5E, HW_SCENE_ID
	MOV #$00, HW_MODE_DONE
	MOV #$00, HW_SCRATCH_KINDS
	RET

;----------------------
; | g_meter_update
; | Description: game_meter_update: counts meter_delay up to 40 (empty meter) or 30. Once there it shows the meter, going to mode 12 when empty, else asking for sound 1 unless the meter fell.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_METER, HW_METER_DELAY, HW_METER_SHOWN, HW_MODE, HW_SOUND_REQ)
; | Globals: meter, meter_delay, meter_shown, mode, sound_req
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, PSW.
;----------------------
g_meter_update:
	LD HW_METER
	BNZ g_meter_update_30
	MOV #$28, ACC
	BR g_meter_update_cmp
g_meter_update_30:
	MOV #$1E, ACC
g_meter_update_cmp:
	BE HW_METER_DELAY, g_meter_update_show
	INC HW_METER_DELAY
	RET
g_meter_update_show:
	LD HW_METER_SHOWN
	ST B
	LD HW_METER
	ST HW_METER_SHOWN
	BNZ g_meter_update_rise
	MOV #$0C, HW_MODE
	RET
g_meter_update_rise:
	SUB B
	BP PSW, 7, g_meter_update_ret
	MOV #$01, HW_SOUND_REQ
g_meter_update_ret:
	RET

;----------------------
; | g_cell_random_scene
; | Description: cell_random_scene: picks scene_id as a random value below 0xD0 divided by 16, 0-12, and returns cell kind 11.
; | Author: suinevere
; | Dependencies: g_random, rrl_ram.inc (HW_SCENE_ID)
; | Globals: scene_id, seed
; | Params: N/A
; | Returns: ACC -- 11. Destroys B, C, TRH, TRL, PSW; four bytes of stack.
;----------------------
g_cell_random_scene:
	MOV #$D0, TRL
	MOV #$00, TRH
	CALLF g_random
	LD TRL
	ROR
	ROR
	ROR
	ROR
	AND #$0F
	ST HW_SCENE_ID
	MOV #$0B, ACC
	RET

;----------------------
; | g_board_takes
; | Description: board_takes: 1 when left is at most free. Otherwise 1 at random, with odds 70, 50 or 30 in 100 as left exceeds free by under 3, under 7 or more.
; | Author: suinevere
; | Dependencies: g_random
; | Globals: seed
; | Params: ACC -- left; B -- free
; | Returns: ACC -- 1 or 0. Destroys B, C, TRH, TRL, PSW; five bytes of stack.
;----------------------
g_board_takes:
	SUB B
	BP PSW, 7, g_board_takes_one
	BZ g_board_takes_one
	ST C
	SUB #$07
	BN PSW, 7, g_board_takes_30
	LD C
	SUB #$03
	BN PSW, 7, g_board_takes_50
	MOV #$46, ACC
	BR g_board_takes_roll
g_board_takes_50:
	MOV #$32, ACC
	BR g_board_takes_roll
g_board_takes_30:
	MOV #$1E, ACC
g_board_takes_roll:
	PUSH ACC
	MOV #$64, TRL
	MOV #$00, TRH
	CALLF g_random
	POP B
	LD TRL
	SUB B
	MOV #$00, ACC
	ADDC #$00
	RET
g_board_takes_one:
	MOV #$01, ACC
	RET

;----------------------
; | g_board_free_cell
; | Description: board_free_cell: the nth free cell (cell_arg 0xFF, and cell_type 0xFF when both) stepping by step from 0 and jumping the twin cell 0x18, n random below free.
; | Author: suinevere
; | Dependencies: g_strip_roll, rrl_ram.inc (HWW_CELL_ARG, HWW_CELL_TYPE)
; | Globals: cell_arg, cell_type, seed
; | Params: ACC -- free, the number of free cells, at least 1; B -- step; C -- both, nonzero to need cell_type free too
; | Returns: ACC -- the cell. Destroys B, C, TRH, TRL, VRMAD, PSW; clears VSEL bit 4 (work RAM auto-increment); eight bytes of stack.
;----------------------
g_board_free_cell:
	PUSH B
	PUSH C
	CALLF g_strip_roll
	ST TRL
	POP C
	POP B
	CLR1 VSEL, 4
	MOV #$00, TRH
g_board_free_cell_loop:
	LD TRH
	ADD #<HWW_CELL_ARG
	ST VRMAD1
	MOV #>HWW_CELL_ARG, ACC
	ADDC #$00
	ST VRMAD2
	LD VTRBF
	BNE #$FF, g_board_free_cell_next
	LD C
	BZ g_board_free_cell_hit
	LD VRMAD1
	ADD #<(HWW_CELL_TYPE-HWW_CELL_ARG)
	ST VRMAD1
	LD VRMAD2
	ADDC #>(HWW_CELL_TYPE-HWW_CELL_ARG)
	ST VRMAD2
	LD VTRBF
	BNE #$FF, g_board_free_cell_next
g_board_free_cell_hit:
	LD TRL
	BZ g_board_free_cell_found
	DEC TRL
g_board_free_cell_next:
	LD TRH
	ADD B
	ST TRH
	BNE #$18, g_board_free_cell_loop
	ADD B
	ST TRH
	BR g_board_free_cell_loop
g_board_free_cell_found:
	LD TRH
	RET

;----------------------
; | g_cell_event
; | Description: game_cell_event: the mode a finished step leads to, from step_result and the cell under sel_c, setting scene_id, mode_done or the meter on the way, as the C does.
; | Author: suinevere
; | Dependencies: g_cell_bonus, g_cell_random_scene, g_cell_meter, rrl_ram.inc (HW_SEL_E, HW_STEP_RESULT, HW_ODDS_SET, HW_SEL_C, HWW_CELL_ARG, HWW_CELL_TYPE, HW_SCENE_ID, HW_BONUS_DONE, HW_STEPS, HW_MODE_DONE, HW_MODE)
; | Globals: sel_e, step_result, odds_set, steps, sel_c, cell_arg, cell_type, bonus_done, scene_id, mode_done, mode, meter, meter_shown, meter_max, meter_delay, lap_mark, seed
; | Params: N/A
; | Returns: ACC -- the mode set: 3, 8, 9, 10, 11 or 15. Destroys B, C, TRH, TRL, VRMAD, PSW; six bytes of stack.
;----------------------
g_cell_event:
	MOV #$00, HW_SEL_E
	LD HW_STEP_RESULT
	BNE #$03, g_cell_event_step
	LD HW_ODDS_SET
	BNE #$02, g_cell_event_8
	CALLF g_cell_bonus
	JMPF g_cell_event_set
g_cell_event_8:
	MOV #$08, ACC
	JMPF g_cell_event_set
g_cell_event_step:
	BNZ g_cell_event_cell
	MOV #$03, ACC
	JMPF g_cell_event_set
g_cell_event_cell:
	LD HW_SEL_C
	ADD #<HWW_CELL_ARG
	ST VRMAD1
	MOV #>HWW_CELL_ARG, ACC
	ADDC #$00
	ST VRMAD2
	LD VTRBF
	ST C
	LD HW_SEL_C
	ADD #<HWW_CELL_TYPE
	ST VRMAD1
	MOV #>HWW_CELL_TYPE, ACC
	ADDC #$00
	ST VRMAD2
	LD VTRBF
	ST B
	SUB #$02
	BP PSW, 7, g_cell_event_type
	SUB #$03
	BN PSW, 7, g_cell_event_type
	LD B
	SUB #$02
	ST HW_SCENE_ID
	MOV #$09, ACC
	BR g_cell_event_set
g_cell_event_type:
	LD B
	BNE #$10, g_cell_event_choice
	LD HW_BONUS_DONE
	BNZ g_cell_event_random
	LD HW_ODDS_SET
	ST B
	MOV #$00, ACC
	MOV #$20, C
	MUL
	ST TRH
	LD C
	ADD #$20
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	LD TRL
	SUB HW_STEPS
	ST TRL
	LD TRH
	SUBC #$00
	BNZ g_cell_event_random
	LD TRL
	SUB #$20
	BN PSW, 7, g_cell_event_random
	CALLF g_cell_bonus
	BR g_cell_event_set
g_cell_event_random:
	CALLF g_cell_random_scene
	BR g_cell_event_set
g_cell_event_choice:
	BE #$FF, g_cell_event_arg
	SUB #$05
	ST HW_MODE_DONE
	MOV #$0F, ACC
	BR g_cell_event_set
g_cell_event_arg:
	LD C
	SUB #$07
	BN PSW, 7, g_cell_event_scene
	LD C
	CALLF g_cell_meter
	MOV #$03, ACC
	BR g_cell_event_set
g_cell_event_scene:
	LD C
	BE #$FF, g_cell_event_random
	ST HW_SCENE_ID
	MOV #$0A, ACC
g_cell_event_set:
	ST HW_MODE
	RET

;----------------------
; | g_board_choice
; | Description: board_choice: runs g_board_choice_core with the choices count taken from and put back to ARG_BLOCK.
; | Author: suinevere
; | Dependencies: g_board_choice_core, game.inc (ARG_BLOCK)
; | Globals: board_arg_kinds, cell_type, anim, seed
; | Params: ACC -- cell; B -- arg, 7 or more; C -- kinds; ARG_BLOCK+0 -- choices
; | Returns: ACC -- the kinds still open; ARG_BLOCK+0 -- choices, counted up. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear; nine bytes of stack.
;----------------------
g_board_choice:
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH ACC
	LD VTRBF
	ST $03
	POP ACC
	CALLF g_board_choice_core
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH ACC
	LD $03
	ST VTRBF
	POP ACC
	RET

;----------------------
; | g_board_choice_core
; | Description: May make cell a choice cell: with odds n/(n+30), n the kinds open among arg's board_arg_kinds bits, a random open kind takes choice slot R3 + 5 and its anim.
; | Author: suinevere
; | Dependencies: g_random, g_anim_start, rrl_ram.inc (HWW_BOARD_ARG_KINDS, HWW_CELL_TYPE)
; | Globals: board_arg_kinds, cell_type, anim, seed
; | Params: ACC -- cell; B -- arg, 7 or more; C -- kinds; R3 -- choices made
; | Returns: ACC -- the kinds still open; R3 -- choices, counted up. Destroys B, C, TRH, TRL, R0-R2, VRMAD, PSW; leaves VSEL bit 4 clear when it starts an anim.
;----------------------
g_board_choice_core:
	ST $00
	LD C
	ST $01
	LD B
	ADD #<(HWW_BOARD_ARG_KINDS-7)
	ST VRMAD1
	MOV #>(HWW_BOARD_ARG_KINDS-7), ACC
	ADDC #$00
	ST VRMAD2
	LD VTRBF
	BZ g_board_choice_kinds
	AND $01
	ST $02
	LD $03
	BE #$05, g_board_choice_kinds
	LD $02
	BNZ g_board_choice_open
g_board_choice_kinds:
	LD $01
	RET
g_board_choice_open:
	ST TRL
	MOV #$00, C
	MOV #$08, TRH
g_board_choice_count:
	BN TRL, 0, g_board_choice_count_next
	INC C
g_board_choice_count_next:
	LD TRL
	ROR
	ST TRL
	DBNZ TRH, g_board_choice_count
	PUSH C
	LD C
	ADD #$1E
	ROL
	ROL
	ROL
	ROL
	ST B
	AND #$F0
	ST TRL
	LD B
	AND #$0F
	ST TRH
	CALLF g_random
	LD TRL
	ROR
	ROR
	ROR
	ROR
	AND #$0F
	ST C
	LD TRH
	ROL
	ROL
	ROL
	ROL
	AND #$F0
	OR C
	ST C
	POP B
	SUB B
	BN PSW, 7, g_board_choice_kinds
	MOV #$00, B
	MOV #$01, TRH
g_board_choice_find:
	LD $02
	AND TRH
	BZ g_board_choice_find_next
	LD C
	BZ g_board_choice_found
	DEC C
g_board_choice_find_next:
	INC B
	LD TRH
	ADD TRH
	ST TRH
	BR g_board_choice_find
g_board_choice_found:
	LD $00
	ADD #<HWW_CELL_TYPE
	ST VRMAD1
	MOV #>HWW_CELL_TYPE, ACC
	ADDC #$00
	ST VRMAD2
	LD $03
	ADD #$05
	ST VTRBF
	PUSH TRH
	MOV #$00, C
	CALLF g_anim_start
	INC $03
	POP ACC
	XOR #$FF
	AND $01
	RET

;----------------------
; | BF_ARG / BF_FREE / BF_LEFT / BF_PLACED / BF_KINDS / BF_CHOICES / BF_TYPE / BF_ODDS / BF_CELL / BOARD_TWIN / ANIM_SCENE_CELL
; | Description: g_board_fill's locals, at these offsets in ARG_BLOCK after the four bytes next_arg's set takes; board.c's twin cell and the cell scene's anim id.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/board.c, game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
BF_ARG	EQU	4
BF_FREE	EQU	5
BF_LEFT	EQU	6
BF_PLACED	EQU	7
BF_KINDS	EQU	8
BF_CHOICES	EQU	9
BF_TYPE	EQU	10
BF_ODDS	EQU	11
BF_CELL	EQU	12
BOARD_TWIN	EQU	$18
ANIM_SCENE_CELL	EQU	$16

;----------------------
; | BFV
; | Description: Points VRMAD at g_board_fill's local at ARG_BLOCK + k, with VSEL bit 4 clear.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: %k -- the offset
; | Returns: N/A
;----------------------
%macro BFV %k
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<(ARG_BLOCK+%k), VRMAD1
%end

;----------------------
; | g_bf_cell
; | Description: Points VRMAD at board cell ACC's byte in cell_arg, or with C = $20 in cell_type, with VSEL bit 4 clear.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_CELL_ARG)
; | Globals: N/A
; | Params: ACC -- the cell, 0-31; C -- 0 for cell_arg, $20 for cell_type
; | Returns: N/A. Destroys ACC, PSW.
;----------------------
g_bf_cell:
	ADD C
	ADD #<HWW_CELL_ARG
	ST VRMAD1
	MOV #>HWW_CELL_ARG, VRMAD2
	CLR1 VSEL, 4
	RET

;----------------------
; | g_board_fill
; | Description: game_board_fill: clears the board's cells, places board_steps args on every fourth free cell, the rest on even cells by board_takes with their choices, then cell scenes 2-4 by odds, copies cell 8 to the twin and returns the kinds still open.
; | Author: suinevere
; | Dependencies: g_next_arg, g_board_free_cell, g_board_takes, g_board_choice_core, g_random, g_anim_start, g_bf_cell, BFV, game.inc (ARG_BLOCK), rrl_ram.inc (HWW_CELL_ARG, HWW_BOARD_STEPS, HWW_BOARD_ARGS, HWW_BOARD_ARG_SET, HW_SEL_A, HW_ODDS_SET)
; | Globals: seed, cell_arg, cell_type, board_steps, board_args, board_arg_set, sel_a, odds_set, anim slots
; | Params: N/A
; | Returns: ACC -- the kinds still open. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; keeps ARG_BLOCK; leaves VSEL bit 4 clear; thirteen bytes of stack for ARG_BLOCK on top of its callees'.
;----------------------
g_board_fill:
	SET1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	MOV #13, B
g_board_fill_save:
	PUSH VTRBF
	DBNZ B, g_board_fill_save
	MOV #<HWW_CELL_ARG, VRMAD1
	MOV #>HWW_CELL_ARG, VRMAD2
	MOV #64, B
	MOV #$FF, ACC
g_board_fill_clear:
	ST VTRBF
	DBNZ B, g_board_fill_clear
	MOV #$00, C
	LD HW_SEL_A
	BZ g_board_fill_home
	MOV #$10, ACC
g_board_fill_home:
	CALLF g_bf_cell
	MOV #$00, VTRBF
	BFV BF_KINDS
	MOV #$FF, VTRBF
	BFV BF_CHOICES
	MOV #$00, VTRBF
	MOV #<HWW_BOARD_STEPS, VRMAD1
	MOV #>HWW_BOARD_STEPS, VRMAD2
	LD VTRBF
	BFV BF_PLACED
	ST VTRBF
	BFV BF_LEFT
	ST VTRBF
	BZ g_board_fill_rest
	CALLF g_bf_set
	BFV BF_FREE
	MOV #$06, VTRBF
	BFV BF_ARG
	MOV #$00, VTRBF
g_board_fill_step:
	BFV BF_ARG
	LD VTRBF
	CALLF g_next_arg
	BFV BF_ARG
	ST VTRBF
	BFV BF_FREE
	LD VTRBF
	MOV #$04, B
	MOV #$00, C
	CALLF g_board_free_cell
	PUSH ACC
	BFV BF_ARG
	LD VTRBF
	ST B
	POP ACC
	MOV #$00, C
	CALLF g_bf_cell
	LD B
	ST VTRBF
	BFV BF_FREE
	LD VTRBF
	DEC ACC
	ST VTRBF
	BFV BF_LEFT
	LD VTRBF
	DEC ACC
	ST VTRBF
	BNZ g_board_fill_step
g_board_fill_rest:
	BFV BF_PLACED
	LD VTRBF
	ST B
	MOV #<HWW_BOARD_ARGS, VRMAD1
	MOV #>HWW_BOARD_ARGS, VRMAD2
	LD VTRBF
	SUB B
	BFV BF_LEFT
	ST VTRBF
	BNZ g_bf_far0
	JMPF g_board_fill_types
g_bf_far0:
	CALLF g_bf_set
	CALLF g_bf_set6
	MOV #14, ACC
	SUB B
	BFV BF_FREE
	ST VTRBF
	BFV BF_ARG
	MOV #$06, VTRBF
g_board_fill_more:
	BFV BF_ARG
	LD VTRBF
	CALLF g_next_arg
	BFV BF_ARG
	ST VTRBF
	BFV BF_FREE
	LD VTRBF
	ST B
	BFV BF_LEFT
	LD VTRBF
	CALLF g_board_takes
	BNZ g_bf_far1
	JMPF g_board_fill_skip
g_bf_far1:
	BFV BF_FREE
	LD VTRBF
	MOV #$02, B
	MOV #$00, C
	CALLF g_board_free_cell
	BFV BF_CELL
	ST VTRBF
	BFV BF_ARG
	LD VTRBF
	ST B
	BFV BF_CELL
	LD VTRBF
	MOV #$00, C
	CALLF g_bf_cell
	LD B
	ST VTRBF
	BFV BF_PLACED
	LD VTRBF
	INC ACC
	ST VTRBF
	BFV BF_CHOICES
	LD VTRBF
	ST $03
	BFV BF_KINDS
	LD VTRBF
	ST C
	BFV BF_ARG
	LD VTRBF
	ST B
	BFV BF_CELL
	LD VTRBF
	CALLF g_board_choice_core
	BFV BF_KINDS
	ST VTRBF
	BFV BF_CHOICES
	LD $03
	ST VTRBF
	BFV BF_FREE
	LD VTRBF
	DEC ACC
	ST VTRBF
	BZ g_board_fill_types
g_board_fill_skip:
	BFV BF_LEFT
	LD VTRBF
	DEC ACC
	ST VTRBF
	BZ g_bf_far2
	JMPF g_board_fill_more
g_bf_far2:
g_board_fill_types:
	LD HW_ODDS_SET
	BZ g_board_fill_odds20
	BE #$01, g_board_fill_odds30
	MOV #35, ACC
	BR g_board_fill_odds
g_board_fill_odds20:
	MOV #20, ACC
	BR g_board_fill_odds
g_board_fill_odds30:
	MOV #30, ACC
g_board_fill_odds:
	BFV BF_ODDS
	ST VTRBF
	BFV BF_PLACED
	LD VTRBF
	ST B
	MOV #30, ACC
	SUB B
	BFV BF_FREE
	ST VTRBF
	BFV BF_TYPE
	MOV #$02, VTRBF
g_board_fill_type:
	MOV #$40, TRL
	MOV #$06, TRH
	CALLF g_random
	LD TRL
	ROR
	ROR
	ROR
	ROR
	AND #$0F
	ST C
	LD TRH
	ROL
	ROL
	ROL
	ROL
	AND #$F0
	OR C
	ST C
	BFV BF_ODDS
	LD VTRBF
	ST B
	BFV BF_TYPE
	LD VTRBF
	BNE #$03, g_board_fill_cut
	MOV #80, B
g_board_fill_cut:
	LD C
	SUB B
	BN PSW, 7, g_board_fill_next
	BFV BF_FREE
	LD VTRBF
	MOV #$01, B
	MOV #$01, C
	CALLF g_board_free_cell
	PUSH ACC
	BFV BF_TYPE
	LD VTRBF
	ST B
	POP ACC
	MOV #$20, C
	CALLF g_bf_cell
	LD B
	ST VTRBF
	BFV BF_TYPE
	LD VTRBF
	MOV #ANIM_SCENE_CELL, B
	MOV #$00, C
	CALLF g_anim_start
	BFV BF_FREE
	LD VTRBF
	DEC ACC
	ST VTRBF
g_board_fill_next:
	BFV BF_TYPE
	LD VTRBF
	INC ACC
	ST VTRBF
	BE #$05, g_bf_far3
	JMPF g_board_fill_type
g_bf_far3:
	MOV #$00, C
	MOV #$08, ACC
	CALLF g_bf_cell
	LD VTRBF
	ST B
	MOV #BOARD_TWIN, ACC
	CALLF g_bf_cell
	LD B
	ST VTRBF
	MOV #$20, C
	MOV #$08, ACC
	CALLF g_bf_cell
	LD VTRBF
	ST B
	MOV #BOARD_TWIN, ACC
	CALLF g_bf_cell
	LD B
	ST VTRBF
	BFV BF_KINDS
	LD VTRBF
	ST C
	MOV #<(ARG_BLOCK+12), VRMAD1
	MOV #13, B
g_board_fill_back:
	POP VTRBF
	DEC VRMAD1
	DBNZ B, g_board_fill_back
	LD C
	RET

;----------------------
; | g_bf_set
; | Description: Copies board_arg_set to ARG_BLOCK+0..3, next_arg's set, and leaves board_steps' local count in B.
; | Author: suinevere
; | Dependencies: BFV, rrl_ram.inc (HWW_BOARD_ARG_SET)
; | Globals: board_arg_set
; | Params: N/A
; | Returns: B -- BF_PLACED's value. Destroys ACC, C, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_bf_set:
	MOV #$00, C
g_bf_set_byte:
	LD C
	ADD #<HWW_BOARD_ARG_SET
	ST VRMAD1
	MOV #>HWW_BOARD_ARG_SET, VRMAD2
	CLR1 VSEL, 4
	LD VTRBF
	PUSH ACC
	LD C
	ADD #<ARG_BLOCK
	ST VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	POP ACC
	ST VTRBF
	INC C
	LD C
	BNE #$04, g_bf_set_byte
	BFV BF_PLACED
	LD VTRBF
	ST B
	RET

;----------------------
; | g_bf_set6
; | Description: Shifts the 32-bit set at ARG_BLOCK+0..3 right by six.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A. Keeps B; destroys ACC, C, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_bf_set6:
	SET1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST $00
	LD VTRBF
	ST $01
	LD VTRBF
	ST $02
	LD VTRBF
	ST $03
	MOV #$06, C
g_bf_set6_bit:
	CLR1 PSW, 7
	LD $03
	RORC
	ST $03
	LD $02
	RORC
	ST $02
	LD $01
	RORC
	ST $01
	LD $00
	RORC
	ST $00
	DEC C
	LD C
	BNZ g_bf_set6_bit
	MOV #<ARG_BLOCK, VRMAD1
	LD $00
	ST VTRBF
	LD $01
	ST VTRBF
	LD $02
	ST VTRBF
	LD $03
	ST VTRBF
	CLR1 VSEL, 4
	RET
