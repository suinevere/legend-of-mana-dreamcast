;----------------------
; | frame.s
; | Description: frame.c in LC86K assembly: each mode's frame, drawing its screen and leaving the mode when it changed, and the dispatch by mode.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/frame.c, pocketstation/build/vmu_game/rrl_ram.inc, game.inc (ARG_BLOCK)
; | Globals: mode, mode_wait, mode_done, btn_new, scene_id
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_leave_unless
; | Description: game_mode_leave_changed for a constant was: runs the current mode's leave handler unless the mode is ACC.
; | Author: suinevere
; | Dependencies: g_mode_leave, rrl_ram.inc (HW_MODE)
; | Globals: mode and what the leave handlers read and write
; | Params: ACC -- was, the mode the frame belongs to
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_leave_unless:
	ST B
	LD HW_MODE
	BE B, g_leave_unless_ret
	JMPF g_mode_leave
g_leave_unless_ret:
	RET

;----------------------
; | g_mode0_frame
; | Description: game_mode0_frame: the reel race's screen, then mode 0's leave check.
; | Author: suinevere
; | Dependencies: g_reel_race_screen, g_leave_unless
; | Globals: what the race screen and the leave handlers read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode0_frame:
	CALLF g_reel_race_screen
	MOV #$00, ACC
	JMPF g_leave_unless

;----------------------
; | g_mode15_frame
; | Description: mode15_frame: counts mode_wait down and, once it is done, a press puts scene_id's item in the bag; then the item's icon and the character, and mode 15's leave check.
; | Author: suinevere
; | Dependencies: g_keep13, g_bag_add, g_anims_tick, g_rom_word_8094, g_yn, g_icon_draw, g_anim_xy, g_leave_unless, rrl_ram.inc (HW_MODE_WAIT, HW_BTN_NEW, HW_SCENE_ID, HW_MODE_DONE)
; | Globals: mode_wait, btn_new, scene_id, mode_done, the bag and what the draws and leave handlers write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_mode15_frame:
	MOV #<g_mode15_frame_body, $00
	MOV #>g_mode15_frame_body, $01
	JMPF g_keep13
g_mode15_frame_body:
	LD HW_MODE_WAIT
	BZ g_mode15_frame_ready
	ST C
	DEC ACC
	ST HW_MODE_WAIT
	LD C
	BNE #$01, g_mode15_frame_tick
g_mode15_frame_ready:
	LD HW_BTN_NEW
	BN ACC, 0, g_mode15_frame_tick
	LD HW_SCENE_ID
	CALLF g_bag_add
g_mode15_frame_tick:
	CALLF g_anims_tick
	LD HW_SCENE_ID
	CALLF g_rom_word_8094
	LD TRH
	INC ACC
	PUSH ACC
	MOV #$00, B
	MOV #$00, C
	MOV #$00, ACC
	CALLF g_yn
	MOV #$05, TRL
	MOV #$00, TRH
	POP B
	MOV #$03, ACC
	CALLF g_icon_draw
	MOV #$FA, C
	MOV #$06, B
	LD HW_MODE_DONE
	ADD #5
	CALLF g_anim_xy
	MOV #15, ACC
	JMPF g_leave_unless

;----------------------
; | g_mode1_frame / g_mode9_frame / g_mode10_frame / g_mode11_frame / g_mode14_frame
; | Description: game_mode1_frame and the others: the mode's screen, then its leave check.
; | Author: suinevere
; | Dependencies: g_menu_two_screen, g_scene_screen_draw, g_reel_game_screen, g_reel_pair_screen, g_bag_menu_screen, g_leave_unless
; | Globals: what the screens and the leave handlers read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode1_frame:
	CALLF g_menu_two_screen
	MOV #1, ACC
	JMPF g_leave_unless
g_mode9_frame:
	CALLF g_scene_screen_draw
	MOV #9, ACC
	JMPF g_leave_unless
g_mode10_frame:
	CALLF g_reel_game_screen
	MOV #10, ACC
	JMPF g_leave_unless
g_mode11_frame:
	CALLF g_reel_pair_screen
	MOV #11, ACC
	JMPF g_leave_unless
g_mode14_frame:
	CALLF g_bag_menu_screen
	MOV #14, ACC
	JMPF g_leave_unless

;----------------------
; | g_mode3_frame
; | Description: game_mode3_frame: the walk; leaves a changed mode, and on a press with the meter settled steps back to the saved cell or, there and still, advances the mode, with sound 3 unless sel_b is 2.
; | Author: suinevere
; | Dependencies: g_keep13, g_board_walk_screen, g_mode_leave, g_sound_begin, g_was_5c4, g_was_byte, g_mode_advance, rrl_ram.inc (HW_MODE, HW_BTN_NEW, HW_METER_SHOWN, HW_METER, HW_CELL_SAVED, HW_SEL_C, HW_WALK_BUSY, HW_SEL_D, HW_FACING_SAVED, HW_WALK_FRAME, HW_SEL_B)
; | Globals: mode, btn_new, meter_shown, meter, cell_saved, sel_c, walk_busy, sel_d, facing_saved, walk_frame, sel_b and what the screen and the mode change write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_mode3_frame:
	MOV #<g_mode3_frame_body, $00
	MOV #>g_mode3_frame_body, $01
	JMPF g_keep13
g_mode3_frame_body:
	CALLF g_board_walk_screen
	LD HW_MODE
	BE #$03, g_mode3_frame_press
	JMPF g_mode_leave
g_mode3_frame_press:
	LD HW_BTN_NEW
	BN ACC, 0, g_mode3_frame_ret
	LD HW_METER_SHOWN
	BNE HW_METER, g_mode3_frame_ret
	LD HW_CELL_SAVED
	BNE HW_SEL_C, g_mode3_frame_back
	LD HW_WALK_BUSY
	BNZ g_mode3_frame_back
	LD HW_SEL_B
	BE #$02, g_mode3_frame_quiet
	MOV #$03, ACC
	CALLF g_sound_begin
	CALLF g_was_5c4
	JMPF g_mode_advance
g_mode3_frame_quiet:
	MOV #$00, ACC
	CALLF g_was_byte
	JMPF g_mode_advance
g_mode3_frame_back:
	LD HW_CELL_SAVED
	ST HW_SEL_C
	LD HW_FACING_SAVED
	ST HW_SEL_D
	MOV #$00, HW_WALK_BUSY
	MOV #$00, HW_WALK_FRAME
g_mode3_frame_ret:
	RET

;----------------------
; | g_mode5_frame
; | Description: game_mode5_frame: the board's wait screen, advancing the mode with mode_wait as was once it is over.
; | Author: suinevere
; | Dependencies: g_keep13, g_board_wait_screen, g_was_byte, g_mode_advance, rrl_ram.inc (HW_MODE_WAIT)
; | Globals: mode_wait and what the screen and the mode change write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_mode5_frame:
	MOV #<g_mode5_frame_body, $00
	MOV #>g_mode5_frame_body, $01
	JMPF g_keep13
g_mode5_frame_body:
	CALLF g_board_wait_screen
	BNZ g_mode5_frame_ret
	LD HW_MODE_WAIT
	CALLF g_was_byte
	JMPF g_mode_advance
g_mode5_frame_ret:
	RET

;----------------------
; | g_mode6_frame
; | Description: game_mode6_frame: the board's reel screen, advancing the mode with the meter's y as was once reel_phase is 8.
; | Author: suinevere
; | Dependencies: g_keep13, g_board_reel_screen, g_mode_advance, game.inc (ARG_BLOCK), rrl_ram.inc (HW_REEL_PHASE)
; | Globals: reel_phase and what the screen and the mode change write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_mode6_frame:
	MOV #<g_mode6_frame_body, $00
	MOV #>g_mode6_frame_body, $01
	JMPF g_keep13
g_mode6_frame_body:
	CALLF g_board_reel_screen
	LD HW_REEL_PHASE
	BNE #$08, g_mode6_frame_ret
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+2), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST B
	INC VRMAD1
	LD VTRBF
	ST C
	MOV #<ARG_BLOCK, VRMAD1
	SET1 VSEL, 4
	LD B
	ST VTRBF
	LD C
	ST VTRBF
	MOV #$00, ACC
	BN C, 7, g_mode6_frame_sign
	MOV #$FF, ACC
g_mode6_frame_sign:
	ST VTRBF
	ST VTRBF
	CLR1 VSEL, 4
	JMPF g_mode_advance
g_mode6_frame_ret:
	RET

;----------------------
; | g_mode12_frame
; | Description: mode12_frame: the markers; counts mode_wait down, then a press advances the mode with btn_new as was.
; | Author: suinevere
; | Dependencies: g_keep13, g_markers_draw, g_was_byte, g_mode_advance, rrl_ram.inc (HW_MODE_WAIT, HW_BTN_NEW)
; | Globals: mode_wait, btn_new, marker and what the mode change writes
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_mode12_frame:
	MOV #<g_mode12_frame_body, $00
	MOV #>g_mode12_frame_body, $01
	JMPF g_keep13
g_mode12_frame_body:
	CALLF g_markers_draw
	LD HW_MODE_WAIT
	BZ g_mode12_frame_press
	DEC ACC
	ST HW_MODE_WAIT
	RET
g_mode12_frame_press:
	LD HW_BTN_NEW
	BN ACC, 0, g_mode12_frame_ret
	CALLF g_was_byte
	JMPF g_mode_advance
g_mode12_frame_ret:
	RET

;----------------------
; | g_mode2_frame / g_mode4_frame / g_mode13_frame
; | Description: game_mode2_frame, game_mode4_frame and game_mode13_frame: the A menu, the board's choice or the row menu, then the press check.
; | Author: suinevere
; | Dependencies: g_keep13, g_menu_a_draw, g_board_choice_draw, g_menu_row_draw, g_mode_press
; | Globals: what the screens and the mode change read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_mode2_frame:
	MOV #<g_mode2_frame_body, $00
	MOV #>g_mode2_frame_body, $01
	JMPF g_keep13
g_mode2_frame_body:
	CALLF g_menu_a_draw
	JMPF g_mode_press
g_mode4_frame:
	CALLF g_board_choice_draw
	JMPF g_mode_press
g_mode13_frame:
	MOV #<g_mode13_frame_body, $00
	MOV #>g_mode13_frame_body, $01
	JMPF g_keep13
g_mode13_frame_body:
	CALLF g_menu_row_draw
	JMPF g_mode_press

;----------------------
; | g_mode8_frame
; | Description: game_mode8_frame: the bag screen; leaves a changed mode, else takes a press once the bag shows a result or its panel is fully down.
; | Author: suinevere
; | Dependencies: g_bag_screen_draw, g_mode_leave, g_mode_press, rrl_ram.inc (HW_MODE, HW_MODE_DONE, HW_WALK_FRAME)
; | Globals: mode, mode_done, walk_frame and what the screen and the mode change read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode8_frame:
	CALLF g_bag_screen_draw
	LD HW_MODE
	BE #$08, g_mode8_frame_press
	JMPF g_mode_leave
g_mode8_frame_press:
	LD HW_MODE_DONE
	BNZ g_mode8_frame_take
	LD HW_WALK_FRAME
	BNE #$40, g_mode8_frame_ret
g_mode8_frame_take:
	JMPF g_mode_press
g_mode8_frame_ret:
	RET

;----------------------
; | g_mode_frame
; | Description: game_mode_frame: runs the current mode's frame by mode & 15 through g_mode_frames.
; | Author: suinevere
; | Dependencies: g_mode_frames, rrl_ram.inc (HW_MODE)
; | Globals: mode and what the frames read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mode_frame:
	LD HW_MODE
	AND #$0F
	ST C
	ADD C
	ADD C
	ADD #<g_mode_frames
	ST TRL
	MOV #>g_mode_frames, ACC
	ADDC #$00
	ST TRH
	PUSH TRL
	PUSH TRH
	RET

;----------------------
; | g_mode_frames
; | Description: game_mode_frame's table: one JMPF per mode 0-15 to its frame, as frame.c's frame[] lists them.
; | Author: suinevere
; | Dependencies: the g_mode*_frame routines, g_board_auto_frame
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
g_mode_frames:
	JMPF g_mode0_frame
	JMPF g_mode1_frame
	JMPF g_mode2_frame
	JMPF g_mode3_frame
	JMPF g_mode4_frame
	JMPF g_mode5_frame
	JMPF g_mode6_frame
	JMPF g_board_auto_frame
	JMPF g_mode8_frame
	JMPF g_mode9_frame
	JMPF g_mode10_frame
	JMPF g_mode11_frame
	JMPF g_mode12_frame
	JMPF g_mode13_frame
	JMPF g_mode14_frame
	JMPF g_mode15_frame
