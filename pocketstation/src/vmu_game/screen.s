;----------------------
; | screen.s
; | Description: screen.c's leaf in LC86K assembly: the value the markers' draw leaves behind.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/screen.c, pocketstation/build/vmu_game/rrl_ram.inc, game.inc (ARG_BLOCK)
; | Globals: marker
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_marker_base
; | Description: marker_base: what the markers' draw leaves in the ARM's r5, before when no slot was stepped, 0xFF when the last slot stepped is free, else that slot's sprite flags.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK), rrl_ram.inc (HWW_MARKER)
; | Globals: marker
; | Params: B -- before, a sprite's flags byte; ARG_BLOCK+0..11 -- s, the last slot's SpriteInfo (flags at +6); ACC -- last, 0xFF for none
; | Returns: ACC -- the value. Destroys VRMAD, PSW.
;----------------------
g_marker_base:
	INC ACC
	BZ g_marker_base_before
	DEC ACC
	ROL
	ROL
	ADD #<HWW_MARKER
	ST VRMAD1
	MOV #>HWW_MARKER, VRMAD2
	LD VTRBF
	INC ACC
	BZ g_marker_base_free
	MOV #<(ARG_BLOCK+6), VRMAD1
	MOV #>(ARG_BLOCK+6), VRMAD2
	LD VTRBF
	RET
g_marker_base_free:
	MOV #$FF, ACC
	RET
g_marker_base_before:
	LD B
	RET

;----------------------
; | g_draw
; | Description: draw: game_sprite_draw of the SpriteInfo in ARG_BLOCK, moving its y and bitmap into the draw's slots.
; | Author: suinevere
; | Dependencies: g_sprite_draw, game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: ARG_BLOCK+0..11 -- s, a SpriteInfo (x, y, width, height, flags, a pad byte, bitmap)
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW and ARG_BLOCK+0..11; leaves VSEL bit 4 clear.
;----------------------
g_draw:
	SET1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST TRL
	LD VTRBF
	ST TRH
	LD VTRBF
	ST $00
	LD VTRBF
	ST $01
	LD VTRBF
	ST $02
	LD VTRBF
	ST B
	LD VTRBF
	ST C
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+8), VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+2), VRMAD1
	ST VTRBF
	MOV #<(ARG_BLOCK+9), VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+3), VRMAD1
	ST VTRBF
	MOV #<(ARG_BLOCK+10), VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+4), VRMAD1
	ST VTRBF
	MOV #<(ARG_BLOCK+11), VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+5), VRMAD1
	ST VTRBF
	MOV #<ARG_BLOCK, VRMAD1
	LD $00
	ST VTRBF
	INC VRMAD1
	LD $01
	ST VTRBF
	LD $02
	JMPF g_sprite_draw


;----------------------
; | g_price_draw
; | Description: price_draw: sprite 0x37's third frame at x, y, then the digit 1 just right of it.
; | Author: suinevere
; | Dependencies: g_info_id, g_put_keep, g_bitmap_add, g_draw, SPRITE_DIGITS, game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y
; | Returns: N/A. Keeps ARG_BLOCK+2..12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; eleven bytes of stack on top of g_put_keep's.
;----------------------
g_price_draw:
	SET1 VSEL, 4
	MOV #<(ARG_BLOCK+2), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #11, B
g_price_draw_save:
	PUSH VTRBF
	DBNZ B, g_price_draw_save
	CLR1 VSEL, 4
	MOV #$37, ACC
	MOV #$00, B
	CALLF g_info_id
	MOV #$08, ACC
	MOV #$00, B
	CALLF g_bitmap_add
	CALLF g_put_keep
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
	MOV #<(ARG_BLOCK+4), VRMAD1
	LD VTRBF
	ADD TRL
	ST TRL
	MOV #$00, ACC
	ADDC TRH
	ST TRH
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD VTRBF
	ST B
	INC VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+1), VRMAD1
	ST VTRBF
	DEC VRMAD1
	LD B
	ST VTRBF
	MOV #SPRITE_DIGITS, ACC
	MOV #$00, B
	CALLF g_info_id
	MOV #$04, ACC
	MOV #$00, B
	CALLF g_bitmap_add
	CALLF g_draw
	MOV #<(ARG_BLOCK+12), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #11, B
g_price_draw_back:
	POP VTRBF
	DEC VRMAD1
	DBNZ B, g_price_draw_back
	RET

;----------------------
; | g_markers_draw
; | Description: game_markers_draw: steps every marker slot in use and draws its frame when it has a bitmap; *last becomes the last slot stepped or 0xFF; returns the last SpriteInfo, zero for none.
; | Author: suinevere
; | Dependencies: g_marker_frame, g_put_keep, game.inc (ARG_BLOCK), rrl_ram.inc (HWW_MARKER)
; | Globals: marker and what g_sprite_draw writes
; | Params: ARG_BLOCK+0 -- last
; | Returns: ARG_BLOCK+0 -- last; ARG_BLOCK+1..12 -- the SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; two bytes of stack on top of g_put_keep's.
;----------------------
g_markers_draw:
	SET1 VSEL, 4
	MOV #<(ARG_BLOCK+1), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #12, B
g_markers_draw_zero:
	MOV #$00, VTRBF
	DBNZ B, g_markers_draw_zero
	CLR1 VSEL, 4
	MOV #$FF, B
	MOV #$00, C
g_markers_draw_slot:
	LD C
	ROL
	ROL
	ADD #<HWW_MARKER
	ST VRMAD1
	MOV #>HWW_MARKER, VRMAD2
	LD VTRBF
	BE #$FF, g_markers_draw_next
	PUSH C
	LD C
	CALLF g_marker_frame
	MOV #<(ARG_BLOCK+8), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	INC VRMAD1
	OR VTRBF
	INC VRMAD1
	OR VTRBF
	INC VRMAD1
	OR VTRBF
	BZ g_markers_draw_keep
	CALLF g_put_keep
g_markers_draw_keep:
	MOV #<(ARG_BLOCK+11), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #12, B
g_markers_draw_shift:
	LD VTRBF
	INC VRMAD1
	ST VTRBF
	DEC VRMAD1
	DEC VRMAD1
	DBNZ B, g_markers_draw_shift
	POP C
	LD C
	ST B
g_markers_draw_next:
	INC C
	LD C
	BNE #$04, g_markers_draw_slot
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD B
	ST VTRBF
	RET

;----------------------
; | g_info_at
; | Description: game_sprite_info at x C and y B, both 0-127, for sprite id ACC.
; | Author: suinevere
; | Dependencies: g_info_id, game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: C -- x; B -- y; ACC -- id, below 256
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_info_at:
	PUSH ACC
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD B
	ST VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	LD C
	ST TRL
	MOV #$00, TRH
	POP ACC
	MOV #$00, B
	JMPF g_info_id

;----------------------
; | g_mirror_s
; | Description: Sets SPRITE_MIRROR in the flags of the SpriteInfo at ARG_BLOCK.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A. Destroys ACC, VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_mirror_s:
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+6), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	OR #$04
	ST VTRBF
	RET

;----------------------
; | g_menu_a_draw
; | Description: game_menu_a_draw: steps the A menu and draws it, with its arrow pointing at the chosen side when not blinking; returns the last SpriteInfo.
; | Author: suinevere
; | Dependencies: g_menu_pick_a, g_info_at, g_put_keep, g_mirror_s, rrl_ram.inc (HW_MODE_PHASE, HW_SEL_A)
; | Globals: mode_phase, sel_a and what the callees write
; | Params: N/A
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_menu_a_draw:
	CALLF g_menu_pick_a
	MOV #$00, C
	MOV #$00, B
	MOV #$43, ACC
	CALLF g_info_at
	CALLF g_put_keep
	LD HW_MODE_PHASE
	BZ g_menu_a_draw_arrow
	RET
g_menu_a_draw_arrow:
	MOV #$02, C
	LD HW_SEL_A
	BZ g_menu_a_draw_side
	MOV #$16, C
g_menu_a_draw_side:
	MOV #$10, B
	MOV #$34, ACC
	CALLF g_info_at
	LD HW_SEL_A
	BZ g_menu_a_draw_put
	CALLF g_mirror_s
g_menu_a_draw_put:
	JMPF g_put_keep

;----------------------
; | g_menu_row_draw
; | Description: game_menu_row_draw: steps the row menu and draws it, with its arrow at the chosen row when not blinking; returns the last SpriteInfo.
; | Author: suinevere
; | Dependencies: g_menu_pick_row, g_info_at, g_put_keep, rrl_ram.inc (HW_MODE_PHASE, HW_MODE_DONE)
; | Globals: mode_phase, mode_done and what the callees write
; | Params: N/A
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_menu_row_draw:
	CALLF g_menu_pick_row
	MOV #$00, C
	MOV #$00, B
	MOV #$44, ACC
	CALLF g_info_at
	CALLF g_put_keep
	LD HW_MODE_PHASE
	BZ g_menu_row_draw_arrow
	RET
g_menu_row_draw_arrow:
	LD HW_MODE_DONE
	ST C
	MOV #10, B
	MOV #$00, ACC
	MUL
	LD C
	ADD #$02
	ST B
	MOV #$00, C
	MOV #$33, ACC
	CALLF g_info_at
	JMPF g_put_keep

;----------------------
; | g_bag_arrows_draw
; | Description: bag_arrows_draw: unless one item is offered or the arrows blink off, draws sprite 0x33 mirrored at the left and plain at the right of row 12.
; | Author: suinevere
; | Dependencies: g_info_at, g_mirror_s, g_draw, game.inc (ARG_BLOCK), rrl_ram.inc (HW_MODE_PHASE)
; | Globals: mode_phase and what g_sprite_draw writes
; | Params: ACC -- offered
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; thirteen bytes of stack on top of g_draw's.
;----------------------
g_bag_arrows_draw:
	BE #$01, g_bag_arrows_draw_ret
	LD HW_MODE_PHASE
	BNZ g_bag_arrows_draw_ret
	SET1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #13, B
g_bag_arrows_draw_save:
	PUSH VTRBF
	DBNZ B, g_bag_arrows_draw_save
	CLR1 VSEL, 4
	MOV #$00, C
	MOV #$0C, B
	MOV #$33, ACC
	CALLF g_info_at
	CALLF g_mirror_s
	CALLF g_draw
	MOV #$18, C
	MOV #$0C, B
	MOV #$33, ACC
	CALLF g_info_at
	CALLF g_draw
	MOV #<(ARG_BLOCK+12), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #13, B
g_bag_arrows_draw_back:
	POP VTRBF
	DEC VRMAD1
	DBNZ B, g_bag_arrows_draw_back
g_bag_arrows_draw_ret:
	RET

;----------------------
; | g_keep13 / g_keep9 / g_keep5 / g_keep1 / g_keep
; | Description: Runs the routine body at R0:R1 with ARG_BLOCK's last R2 (13, 9, 5 or 1) bytes saved around it, for a routine that owns only the slots before them.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: R0, R1 -- the body's address, low byte first; R2 -- how many bytes (g_keep only); ACC, B, TRH, TRL pass through to the body
; | Returns: the body's ACC, B, C, TRH, TRL. Keeps the last R2 bytes of ARG_BLOCK; destroys C on the way in, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear; R2 + 3 bytes of stack under the body.
;----------------------
g_keep13:
	MOV #13, $02
	BR g_keep
g_keep9:
	MOV #9, $02
	BR g_keep
g_keep5:
	MOV #5, $02
	BR g_keep
g_keep1:
	MOV #1, $02
g_keep:
	ST $03
	LD $02
	ST C
	XOR #$FF
	ADD #<(ARG_BLOCK+14)
	ST VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
g_keep_save:
	PUSH VTRBF
	DBNZ C, g_keep_save
	CLR1 VSEL, 4
	PUSH $02
	MOV #<g_keep_back, ACC
	PUSH ACC
	MOV #>g_keep_back, ACC
	PUSH ACC
	PUSH $00
	PUSH $01
	LD $03
	RET
g_keep_back:
	ST $03
	POP $02
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+12), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
g_keep_pop:
	POP VTRBF
	DEC VRMAD1
	DBNZ $02, g_keep_pop
	LD $03
	RET

;----------------------
; | g_xy
; | Description: Sets the x and y most draws take from signed bytes: TRH:TRL from C, ARG_BLOCK+0..1 from B.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: C -- x, signed; B -- y, signed
; | Returns: TRH:TRL -- x; ARG_BLOCK+0..1 -- y. Keeps ACC, B, C; destroys VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_xy:
	PUSH ACC
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD B
	ST VTRBF
	INC VRMAD1
	MOV #$00, ACC
	BN B, 7, g_xy_x
	MOV #$FF, ACC
g_xy_x:
	ST VTRBF
	LD C
	ST TRL
	MOV #$00, TRH
	BN C, 7, g_xy_ret
	MOV #$FF, TRH
g_xy_ret:
	POP ACC
	RET

;----------------------
; | g_pos_strip
; | Description: Sets the reel draws' pos and strip slots: ARG_BLOCK+0..1 from B, C and ARG_BLOCK+2..3 from R0, R1.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: B, C -- pos, low byte first; R0, R1 -- strip, a vmu_ram offset in work RAM, low byte first
; | Returns: ARG_BLOCK+0..3 -- pos and strip. Destroys ACC, VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_pos_strip:
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	LD B
	ST VTRBF
	LD C
	ST VTRBF
	LD $00
	ST VTRBF
	LD $01
	ST VTRBF
	CLR1 VSEL, 4
	RET

;----------------------
; | g_win_a / g_win_b
; | Description: game_reel_window_draw at y of reel A (reel_pos_a, reel_len, strip A) or reel B (reel_pos_b, reel_len_b, strip B).
; | Author: suinevere
; | Dependencies: g_pos_strip, g_reel_window_draw, rrl_ram.inc (HW_REEL_POS_A, HW_REEL_POS_B, HW_REEL_LEN, HW_REEL_LEN_B, HWW_STRIP_A, HWW_STRIP_B)
; | Globals: reel_pos_a, reel_pos_b, reel_len, reel_len_b and what g_reel_window_draw writes
; | Params: TRH:TRL -- y
; | Returns: ARG_BLOCK+4..7 -- the last bitmap or 0. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW and ARG_BLOCK+0..3; leaves VSEL bit 4 clear.
;----------------------
g_win_a:
	LD HW_REEL_POS_A
	ST B
	LD HW_REEL_POS_A+1
	ST C
	MOV #<(HWW_STRIP_A+$100), $00
	MOV #>(HWW_STRIP_A+$100), $01
	CALLF g_pos_strip
	LD HW_REEL_LEN
	JMPF g_reel_window_draw
g_win_b:
	LD HW_REEL_POS_B
	ST B
	LD HW_REEL_POS_B+1
	ST C
	MOV #<(HWW_STRIP_B+$100), $00
	MOV #>(HWW_STRIP_B+$100), $01
	CALLF g_pos_strip
	LD HW_REEL_LEN_B
	JMPF g_reel_window_draw

;----------------------
; | g_reel_window_show
; | Description: game_reel_window_show: unless reel_phase is 8, sets *y to reel_slide - 12 in phases 0 and 7, else 0, and draws reel A's window there.
; | Author: suinevere
; | Dependencies: g_keep5, g_win_a, game.inc (ARG_BLOCK), rrl_ram.inc (HW_REEL_PHASE, HW_REEL_SLIDE)
; | Globals: reel_phase, reel_slide and what g_reel_window_draw reads and writes
; | Params: ARG_BLOCK+0..3 -- *y, 32-bit
; | Returns: ARG_BLOCK+0..3 -- *y, untouched in phase 8; ARG_BLOCK+4..7 -- the window's last symbol, 0 in phase 8. Keeps ARG_BLOCK+8..12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; g_keep5's eight bytes of stack on top of the callees'.
;----------------------
g_reel_window_show:
	MOV #<g_reel_window_show_body, $00
	MOV #>g_reel_window_show_body, $01
	JMPF g_keep5
g_reel_window_show_body:
	LD HW_REEL_PHASE
	BNE #$08, g_reel_window_show_draw
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+4), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
	RET
g_reel_window_show_draw:
	MOV #$00, B
	MOV #$00, C
	BZ g_reel_window_show_slide
	BNE #$07, g_reel_window_show_go
g_reel_window_show_slide:
	LD HW_REEL_SLIDE
	SUB #12
	ST B
	MOV #$00, ACC
	SUBC #$00
	ST C
g_reel_window_show_go:
	PUSH B
	PUSH C
	LD B
	ST TRL
	LD C
	ST TRH
	CALLF g_win_a
	POP C
	POP B
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	LD B
	ST VTRBF
	LD C
	ST VTRBF
	ST VTRBF
	ST VTRBF
	CLR1 VSEL, 4
	RET

;----------------------
; | g_reel_screen_draw
; | Description: game_reel_screen_draw: the reel screen's panel, scratch_kinds, scratch_first and scratch_count as cleared digits nine pixels apart, then reel A's window at the top.
; | Author: suinevere
; | Dependencies: g_keep9, g_info_at, g_draw, g_rs_digit, g_win_a, g_r_down, rrl_ram.inc (HW_SCRATCH_KINDS, HW_SCRATCH_FIRST, HW_SCRATCH_COUNT)
; | Globals: scratch_kinds, scratch_first, scratch_count and what the draws read and write
; | Params: N/A
; | Returns: ARG_BLOCK+0..3 -- the window's last symbol. Keeps ARG_BLOCK+4..12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; g_keep9's twelve bytes of stack on top of the callees'.
;----------------------
g_reel_screen_draw:
	MOV #<g_reel_screen_draw_body, $00
	MOV #>g_reel_screen_draw_body, $01
	JMPF g_keep9
g_reel_screen_draw_body:
	MOV #$00, C
	MOV #$00, B
	MOV #$4A, ACC
	CALLF g_info_at
	CALLF g_draw
	LD HW_SCRATCH_KINDS
	MOV #5, C
	CALLF g_rs_digit
	LD HW_SCRATCH_FIRST
	MOV #14, C
	CALLF g_rs_digit
	LD HW_SCRATCH_COUNT
	MOV #23, C
	CALLF g_rs_digit
	MOV #$00, TRL
	MOV #$00, TRH
	CALLF g_win_a
	JMPF g_r_down

;----------------------
; | g_rs_digit
; | Description: One of the reel screen's counts: the digits sprite at x C, y 0x16 moved on to the count's digit and drawn cleared.
; | Author: suinevere
; | Dependencies: g_digit_info, g_clear_draw
; | Globals: what g_sprite_draw writes
; | Params: ACC -- the count; C -- x
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW and ARG_BLOCK+0..11; leaves VSEL bit 4 clear.
;----------------------
g_rs_digit:
	MOV #$16, B
	CALLF g_digit_info
	JMPF g_clear_draw

;----------------------
; | g_anim_xy
; | Description: game_anim_draw of slot ACC at signed byte x C and y B.
; | Author: suinevere
; | Dependencies: g_xy, g_anim_draw
; | Globals: anim and what g_sprite_draw writes
; | Params: ACC -- slot; C -- x; B -- y
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_anim_xy:
	CALLF g_xy
	JMPF g_anim_draw

;----------------------
; | g_reel_anim_screen_draw
; | Description: game_reel_anim_screen_draw: steps the animations, draws the characters by reel_phase with the price tag at one or two kinds held, then window A outside phases 2-3 and window B from phase 2.
; | Author: suinevere
; | Dependencies: g_keep9, g_anims_tick, g_anim_xy, g_xy, g_price_draw, g_win_a, g_win_b, g_r_down, rrl_ram.inc (HW_BAG_KIND_COUNT, HW_REEL_PHASE)
; | Globals: bag_kind_count, reel_phase, anim and what the draws read and write
; | Params: N/A
; | Returns: ARG_BLOCK+0..3 -- the last window's last symbol. Keeps ARG_BLOCK+4..12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; g_keep9's twelve bytes of stack on top of the callees'.
;----------------------
g_reel_anim_screen_draw:
	MOV #<g_reel_anim_screen_draw_body, $00
	MOV #>g_reel_anim_screen_draw_body, $01
	JMPF g_keep9
g_reel_anim_screen_draw_body:
	LD HW_BAG_KIND_COUNT
	PUSH ACC
	CALLF g_anims_tick
	POP B
	LD HW_REEL_PHASE
	SUB #$02
	BN PSW, 7, g_reel_anim_screen_draw_late
	LD B
	BE #$01, g_reel_anim_screen_draw_tag0
	MOV #$00, C
	MOV #$04, B
	MOV #$00, ACC
	CALLF g_anim_xy
	BR g_reel_anim_screen_draw_windows
g_reel_anim_screen_draw_tag0:
	MOV #$FB, C
	MOV #$04, B
	MOV #$00, ACC
	CALLF g_anim_xy
	MOV #$16, C
	MOV #$10, B
	CALLF g_xy
	CALLF g_price_draw
	BR g_reel_anim_screen_draw_windows
g_reel_anim_screen_draw_late:
	SUB #$02
	BN PSW, 7, g_reel_anim_screen_draw_both
	LD B
	BE #$02, g_reel_anim_screen_draw_tag1
	MOV #$00, C
	MOV #$FC, B
	MOV #$01, ACC
	CALLF g_anim_xy
	BR g_reel_anim_screen_draw_windows
g_reel_anim_screen_draw_tag1:
	MOV #$05, C
	MOV #$FC, B
	MOV #$01, ACC
	CALLF g_anim_xy
	MOV #$00, C
	MOV #$08, B
	CALLF g_xy
	CALLF g_price_draw
	BR g_reel_anim_screen_draw_windows
g_reel_anim_screen_draw_both:
	MOV #$F6, C
	MOV #$00, B
	MOV #$01, ACC
	CALLF g_anim_xy
	MOV #$0A, C
	MOV #$00, B
	MOV #$00, ACC
	CALLF g_anim_xy
g_reel_anim_screen_draw_windows:
	LD HW_REEL_PHASE
	BE #$02, g_reel_anim_screen_draw_b
	BE #$03, g_reel_anim_screen_draw_b
	MOV #$00, TRL
	MOV #$00, TRH
	CALLF g_win_a
g_reel_anim_screen_draw_b:
	LD HW_REEL_PHASE
	SUB #$02
	BP PSW, 7, g_reel_anim_screen_draw_ret
	MOV #$16, TRL
	MOV #$00, TRH
	CALLF g_win_b
g_reel_anim_screen_draw_ret:
	JMPF g_r_down

;----------------------
; | g_item_icon_draw
; | Description: item_icon_draw: the icon of item's 0x02008094 entry at x, 0, its low byte the icon and its high byte one less than the marks.
; | Author: suinevere
; | Dependencies: g_keep13, g_rom_word_8094, g_icon_draw, game.inc (ARG_BLOCK)
; | Globals: what g_icon_draw reads and writes
; | Params: TRH:TRL -- x; ACC -- item
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_item_icon_draw:
	MOV #<g_item_icon_draw_body, $00
	MOV #>g_item_icon_draw_body, $01
	JMPF g_keep13
g_item_icon_draw_body:
	PUSH TRL
	PUSH TRH
	CALLF g_rom_word_8094
	LD TRH
	INC ACC
	ST B
	LD TRL
	POP TRH
	POP TRL
	PUSH ACC
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	CLR1 VSEL, 4
	MOV #$00, VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	POP ACC
	JMPF g_icon_draw

;----------------------
; | g_pair_number_draw
; | Description: pair_number_draw: n right-aligned for five digits from x at y, then the digits sprite's info at that x.
; | Author: suinevere
; | Dependencies: g_keep13, g_digits_x_vr, g_number_draw, g_info_id, SPRITE_DIGITS, game.inc (ARG_BLOCK)
; | Globals: what g_number_draw writes
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ARG_BLOCK+2..5 -- n
; | Returns: ACC -- the digits sprite's flags. Keeps ARG_BLOCK; destroys B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_pair_number_draw:
	MOV #<g_pair_number_draw_body, $00
	MOV #>g_pair_number_draw_body, $01
	JMPF g_keep13
g_pair_number_draw_body:
	MOV #<(ARG_BLOCK+2), VRMAD1
	CALLF g_digits_x_vr
	PUSH TRL
	PUSH TRH
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	PUSH ACC
	INC VRMAD1
	LD VTRBF
	PUSH ACC
	CALLF g_number_draw
	MOV #<(ARG_BLOCK+1), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	POP TRH
	POP TRL
	MOV #SPRITE_DIGITS, ACC
	MOV #$00, B
	CALLF g_info_id
	MOV #<(ARG_BLOCK+6), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	RET

;----------------------
; | g_reel_race_screen
; | Description: game_reel_race_screen: steps the race, then draws the lap picture's halves at half scratch_wait (mode_done 3), the finish picture (4) or the three reels.
; | Author: suinevere
; | Dependencies: g_keep13, g_reel_race_step, g_race_reel, g_rrs_wide, g_info_at, g_draw, rrl_ram.inc (HW_MODE_DONE, HW_SCRATCH_WAIT, HW_REEL_POS_A, HW_REEL_POS_B, HW_REEL_POS_C, HWW_STRIP_A, HWW_STRIP_B, HWW_STRIP_C)
; | Globals: mode_done, scratch_wait (read as a signed halfword), reel_pos_a, reel_pos_b, reel_pos_c and what the step and draws read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_reel_race_screen:
	MOV #<g_reel_race_screen_body, $00
	MOV #>g_reel_race_screen_body, $01
	JMPF g_keep13
g_reel_race_screen_body:
	CALLF g_reel_race_step
	LD HW_MODE_DONE
	BE #$03, g_reel_race_screen_lap
	BE #$04, g_reel_race_screen_finish
	MOV #$00, TRL
	MOV #$00, TRH
	LD HW_REEL_POS_A
	ST B
	LD HW_REEL_POS_A+1
	ST C
	MOV #<(HWW_STRIP_A+$100), $00
	MOV #>(HWW_STRIP_A+$100), $01
	CALLF g_race_reel
	MOV #$0B, TRL
	MOV #$00, TRH
	LD HW_REEL_POS_B
	ST B
	LD HW_REEL_POS_B+1
	ST C
	MOV #<(HWW_STRIP_B+$100), $00
	MOV #>(HWW_STRIP_B+$100), $01
	CALLF g_race_reel
	MOV #$16, TRL
	MOV #$00, TRH
	LD HW_REEL_POS_C
	ST B
	LD HW_REEL_POS_C+1
	ST C
	MOV #<(HWW_STRIP_C+$100), $00
	MOV #>(HWW_STRIP_C+$100), $01
	JMPF g_race_reel
g_reel_race_screen_finish:
	MOV #$00, C
	MOV #$00, B
	MOV #$57, ACC
	CALLF g_info_at
	JMPF g_draw
g_reel_race_screen_lap:
	LD HW_SCRATCH_WAIT+1
	CLR1 PSW, 7
	BN ACC, 7, g_reel_race_screen_half
	SET1 PSW, 7
g_reel_race_screen_half:
	RORC
	ST TRH
	LD HW_SCRATCH_WAIT
	RORC
	ST TRL
	PUSH TRL
	PUSH TRH
	MOV #$00, ACC
	CALLF g_rrs_wide
	POP TRH
	POP TRL
	LD TRL
	ADD #16
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	MOV #$01, ACC
	JMPF g_rrs_wide

;----------------------
; | g_race_reel
; | Description: game_reel_draw at y of a reel whose pos is in B, C and strip in R0, R1, with reel_len.
; | Author: suinevere
; | Dependencies: g_pos_strip, g_reel_draw, rrl_ram.inc (HW_REEL_LEN)
; | Globals: reel_len and what g_reel_draw writes
; | Params: TRH:TRL -- y; B, C -- pos; R0, R1 -- strip
; | Returns: ARG_BLOCK+4..7 -- the last bitmap. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW and ARG_BLOCK+0..3; leaves VSEL bit 4 clear.
;----------------------
g_race_reel:
	CALLF g_pos_strip
	LD HW_REEL_LEN
	JMPF g_reel_draw

;----------------------
; | g_rrs_wide
; | Description: game_wide_draw at x, 0 of wide sprite scene_id * 2 + ACC, the race's lap picture half.
; | Author: suinevere
; | Dependencies: g_wide_draw, game.inc (ARG_BLOCK), rrl_ram.inc (HW_SCENE_ID)
; | Globals: scene_id and what g_wide_draw writes
; | Params: TRH:TRL -- x; ACC -- 0 for the left half, 1 for the right
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_rrs_wide:
	ST C
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	LD HW_SCENE_ID
	ROL
	AND #$01
	ST B
	LD HW_SCENE_ID
	ADD ACC
	OR C
	ST VTRBF
	LD B
	ST VTRBF
	CLR1 VSEL, 4
	JMPF g_wide_draw

;----------------------
; | g_r_down
; | Description: Moves a reel window's result from ARG_BLOCK+4..7 down to ARG_BLOCK+0..3, the result slot of a routine with no arguments.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ARG_BLOCK+4..7 -- the result
; | Returns: ARG_BLOCK+0..3 -- the result. Destroys VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_r_down:
	SET1 VSEL, 4
	MOV #<(ARG_BLOCK+4), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+3), VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	RET

;----------------------
; | g_draw_at
; | Description: draw(game_sprite_info(x, y, id)) for x C and y B, both 0-127, and id ACC below 256.
; | Author: suinevere
; | Dependencies: g_info_at, g_draw
; | Globals: what g_sprite_draw writes
; | Params: C -- x; B -- y; ACC -- id
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW and ARG_BLOCK+0..11; leaves VSEL bit 4 clear.
;----------------------
g_draw_at:
	CALLF g_info_at
	JMPF g_draw

;----------------------
; | g_yn
; | Description: Sets ARG_BLOCK+0..1 to y B and ARG_BLOCK+2..5 to n C:ACC, the y and n most number draws take.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: B -- y, 0-127; ACC -- n's low byte; C -- its high byte
; | Returns: ARG_BLOCK+0..5 -- y and n. Destroys ACC, VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_yn:
	PUSH ACC
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	LD B
	ST VTRBF
	MOV #$00, VTRBF
	POP ACC
	ST VTRBF
	LD C
	ST VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
	RET

;----------------------
; | g_board_walk_screen / g_board_auto_screen / g_board_meter
; | Description: game_board_walk_screen and game_board_auto_screen: one frame of a walk by the buttons or on its own, then the board and the meter at y 0 (g_board_meter).
; | Author: suinevere
; | Dependencies: g_keep13, g_walk_by_buttons, g_walk_pending, g_board_draw, g_meter_draw
; | Globals: what the walk, the board and the meter read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK (g_board_meter: ARG_BLOCK+0..11 -- the meter's SpriteInfo); destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_board_walk_screen:
	MOV #<g_board_walk_screen_body, $00
	MOV #>g_board_walk_screen_body, $01
	JMPF g_keep13
g_board_walk_screen_body:
	CALLF g_walk_by_buttons
g_board_meter:
	CALLF g_board_draw
	MOV #$00, TRL
	MOV #$00, TRH
	JMPF g_meter_draw
g_board_auto_screen:
	MOV #<g_board_auto_screen_body, $00
	MOV #>g_board_auto_screen_body, $01
	JMPF g_keep13
g_board_auto_screen_body:
	CALLF g_walk_pending
	JMPF g_board_meter

;----------------------
; | g_board_wait_screen
; | Description: game_board_wait_screen: the board and meter while mode_wait counts up to 70, with the banner for the first 45 frames.
; | Author: suinevere
; | Dependencies: g_keep13, g_board_meter, g_draw_at, rrl_ram.inc (HW_MODE_WAIT)
; | Globals: mode_wait and what the draws read and write
; | Params: N/A
; | Returns: ACC -- 0 once 70 frames have passed, else 1. Keeps ARG_BLOCK; destroys B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_board_wait_screen:
	MOV #<g_board_wait_screen_body, $00
	MOV #>g_board_wait_screen_body, $01
	JMPF g_keep13
g_board_wait_screen_body:
	CALLF g_board_meter
	LD HW_MODE_WAIT
	ST C
	INC ACC
	ST HW_MODE_WAIT
	LD C
	SUB #69
	BN PSW, 7, g_board_wait_screen_over
	LD C
	SUB #44
	BN PSW, 7, g_board_wait_screen_on
	MOV #$00, C
	MOV #10, B
	MOV #$45, ACC
	CALLF g_draw_at
g_board_wait_screen_on:
	MOV #$01, ACC
	RET
g_board_wait_screen_over:
	MOV #$00, ACC
	RET

;----------------------
; | g_board_choice_draw
; | Description: game_board_choice_draw: steps the yes/no choice, draws the board and, while shown, the pointer by slot 0's argument: bit 1 mirrors it to the right, bit 0 raises it to the second row.
; | Author: suinevere
; | Dependencies: g_keep13, g_menu_pick_b, g_board_draw, g_info_at, g_mirror_s, g_bitmap_add, g_draw, rrl_ram.inc (HW_MODE_PHASE, HWW_ANIM)
; | Globals: mode_phase, anim and what the draws read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_board_choice_draw:
	MOV #<g_board_choice_draw_body, $00
	MOV #>g_board_choice_draw_body, $01
	JMPF g_keep13
g_board_choice_draw_body:
	CALLF g_menu_pick_b
	CALLF g_board_draw
	LD HW_MODE_PHASE
	BNZ g_board_choice_draw_ret
	CLR1 VSEL, 4
	MOV #<(HWW_ANIM+1), VRMAD1
	MOV #>(HWW_ANIM+1), VRMAD2
	LD VTRBF
	PUSH ACC
	MOV #$00, C
	BN ACC, 1, g_board_choice_draw_y
	MOV #$18, C
g_board_choice_draw_y:
	MOV #$18, B
	BN ACC, 0, g_board_choice_draw_info
	MOV #$0A, B
g_board_choice_draw_info:
	MOV #$34, ACC
	CALLF g_info_at
	POP ACC
	PUSH ACC
	BN ACC, 1, g_board_choice_draw_raise
	CALLF g_mirror_s
g_board_choice_draw_raise:
	POP ACC
	BN ACC, 0, g_board_choice_draw_put
	MOV #$10, ACC
	MOV #$00, B
	CALLF g_bitmap_add
g_board_choice_draw_put:
	JMPF g_draw
g_board_choice_draw_ret:
	RET

;----------------------
; | g_board_reel_screen
; | Description: game_board_reel_screen: spins reel A, steps the walk, draws the board, the reel window and the meter at y 0.
; | Author: suinevere
; | Dependencies: g_keep1, g_reel_spin_step, g_walk_step, g_board_draw, g_reel_window_show, g_meter_draw, game.inc (ARG_BLOCK)
; | Globals: what the steps and draws read and write
; | Params: N/A
; | Returns: ARG_BLOCK+0..11 -- the meter's SpriteInfo. Keeps ARG_BLOCK+12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; g_keep1's four bytes of stack on top of the callees'.
;----------------------
g_board_reel_screen:
	MOV #<g_board_reel_screen_body, $00
	MOV #>g_board_reel_screen_body, $01
	JMPF g_keep1
g_board_reel_screen_body:
	CALLF g_reel_spin_step
	CALLF g_walk_step
	CALLF g_board_draw
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
	CALLF g_reel_window_show
	MOV #$00, TRL
	MOV #$00, TRH
	JMPF g_meter_draw

;----------------------
; | g_menu_two_screen
; | Description: game_menu_two_screen: steps the two-row menu and draws its panel, the items held out of 50 and, while shown, the cursor on row 0 or 1.
; | Author: suinevere
; | Dependencies: g_keep13, g_menu_pick_two, g_draw_at, g_yn, g_count_of_draw, rrl_ram.inc (HWW_BAG_COUNT, HW_MODE_PHASE, HW_MODE_DONE)
; | Globals: bag_count, mode_phase, mode_done and what the menu and draws read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_menu_two_screen:
	MOV #<g_menu_two_screen_body, $00
	MOV #>g_menu_two_screen_body, $01
	JMPF g_keep13
g_menu_two_screen_body:
	CALLF g_menu_pick_two
	MOV #$00, C
	MOV #$00, B
	MOV #$42, ACC
	CALLF g_draw_at
	CLR1 VSEL, 4
	MOV #<HWW_BAG_COUNT, VRMAD1
	MOV #>HWW_BAG_COUNT, VRMAD2
	LD VTRBF
	MOV #$1A, B
	MOV #$00, C
	CALLF g_yn
	MOV #$06, TRL
	MOV #$00, TRH
	MOV #$32, ACC
	CALLF g_count_of_draw
	LD HW_MODE_PHASE
	BNZ g_menu_two_screen_ret
	MOV #$00, C
	MOV #$03, B
	LD HW_MODE_DONE
	BZ g_menu_two_screen_cursor
	MOV #$0E, B
g_menu_two_screen_cursor:
	MOV #$33, ACC
	JMPF g_draw_at
g_menu_two_screen_ret:
	RET

;----------------------
; | g_reel_game_screen
; | Description: game_reel_game_screen: steps the reel game and draws the spinning reel screen (mode_done 2), the consolation item (5) or the markers.
; | Author: suinevere
; | Dependencies: g_keep13, g_reel_game_step, g_reel_spin_step, g_reel_screen_draw, g_item_icon_draw, g_markers_draw, rrl_ram.inc (HW_MODE_DONE, HW_SCRATCH_WAIT)
; | Globals: mode_done, scratch_wait and what the steps and draws read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_reel_game_screen:
	MOV #<g_reel_game_screen_body, $00
	MOV #>g_reel_game_screen_body, $01
	JMPF g_keep13
g_reel_game_screen_body:
	CALLF g_reel_game_step
	LD HW_MODE_DONE
	BNE #$02, g_reel_game_screen_item
	CALLF g_reel_spin_step
	JMPF g_reel_screen_draw
g_reel_game_screen_item:
	BNE #$05, g_reel_game_screen_markers
	MOV #$00, TRL
	MOV #$00, TRH
	LD HW_SCRATCH_WAIT
	JMPF g_item_icon_draw
g_reel_game_screen_markers:
	JMPF g_markers_draw

;----------------------
; | g_scene_screen_draw
; | Description: game_scene_screen_draw: steps the scene, draws the markers until its result shows, then reel A from its start with the price tag while nothing is counted (scene 2) or the trade sliding in by half scratch_first.
; | Author: suinevere
; | Dependencies: g_keep13, g_scene_step, g_markers_draw, g_pos_strip, g_reel_window_draw, g_xy, g_price_draw, g_item_icon_draw, g_info_id, g_bitmap_add, g_draw, rrl_ram.inc (HW_MODE_DONE, HW_SCENE_ID, HW_REEL_LEN, HW_SCRATCH_COUNT, HW_SCRATCH_FIRST, HW_SCRATCH_WAIT, HW_SCRATCH_KINDS, HWW_STRIP_A)
; | Globals: mode_done, scene_id, reel_len, scratch_count, scratch_first, scratch_wait, scratch_kinds and what the step and draws read and write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_scene_screen_draw:
	MOV #<g_scene_screen_draw_body, $00
	MOV #>g_scene_screen_draw_body, $01
	JMPF g_keep13
g_scene_screen_draw_body:
	CALLF g_scene_step
	LD HW_MODE_DONE
	BE #$01, g_scene_screen_draw_shown
	JMPF g_markers_draw
g_scene_screen_draw_shown:
	LD HW_SCENE_ID
	BNE #$02, g_scene_screen_draw_trade
	MOV #$00, TRL
	MOV #$00, TRH
	MOV #$00, B
	MOV #$00, C
	MOV #<(HWW_STRIP_A+$100), $00
	MOV #>(HWW_STRIP_A+$100), $01
	CALLF g_pos_strip
	LD HW_REEL_LEN
	CALLF g_reel_window_draw
	LD HW_SCRATCH_COUNT
	BNZ g_scene_screen_draw_ret
	MOV #$0B, C
	MOV #$11, B
	CALLF g_xy
	JMPF g_price_draw
g_scene_screen_draw_trade:
	LD HW_SCRATCH_WAIT
	BE #$FF, g_scene_screen_draw_new
	MOV #$00, B
	MOV #$00, ACC
	CALLF g_slide_x
	LD HW_SCRATCH_WAIT
	CALLF g_item_icon_draw
	MOV #$09, B
	MOV #$16, ACC
	CALLF g_slide_x
	MOV #$4D, ACC
	MOV #$00, B
	CALLF g_info_id
	LD HW_SCRATCH_COUNT
	SUB #11
	BN PSW, 7, g_scene_screen_draw_arrow
	MOV #$1C, ACC
	MOV #$00, B
	CALLF g_bitmap_add
g_scene_screen_draw_arrow:
	CALLF g_draw
g_scene_screen_draw_new:
	MOV #$00, B
	MOV #$34, ACC
	CALLF g_slide_x
	LD HW_SCRATCH_KINDS
	JMPF g_item_icon_draw
g_scene_screen_draw_ret:
	RET

;----------------------
; | g_slide_x
; | Description: The scene screen's slide: x ACC less half scratch_first, set up with y B for a draw.
; | Author: suinevere
; | Dependencies: g_xy, rrl_ram.inc (HW_SCRATCH_FIRST)
; | Globals: scratch_first
; | Params: ACC -- x before the slide; B -- y, signed
; | Returns: TRH:TRL -- x; ARG_BLOCK+0..1 -- y. Destroys ACC, C, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_slide_x:
	ST C
	LD HW_SCRATCH_FIRST
	CLR1 PSW, 7
	RORC
	XOR #$FF
	INC ACC
	ADD C
	ST C
	JMPF g_xy

;----------------------
; | g_n
; | Description: Sets ARG_BLOCK+2..5 to n C:ACC, the n the number draws take after their y.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- n's low byte; C -- its high byte
; | Returns: ARG_BLOCK+2..5 -- n. Destroys ACC, VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_n:
	PUSH ACC
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+2), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	POP ACC
	ST VTRBF
	LD C
	ST VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
	RET

;----------------------
; | g_bag_count
; | Description: Reads bag_count from work RAM.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_BAG_COUNT)
; | Globals: bag_count
; | Params: N/A
; | Returns: ACC -- bag_count. Destroys VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_bag_count:
	CLR1 VSEL, 4
	MOV #<HWW_BAG_COUNT, VRMAD1
	MOV #>HWW_BAG_COUNT, VRMAD2
	LD VTRBF
	RET

;----------------------
; | g_bag_item
; | Description: Reads bag_items[ACC] from work RAM.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_BAG_ITEMS)
; | Globals: bag_items
; | Params: ACC -- the index
; | Returns: ACC -- the item. Destroys VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_bag_item:
	CLR1 VSEL, 4
	ADD #<HWW_BAG_ITEMS
	ST VRMAD1
	MOV #>HWW_BAG_ITEMS, ACC
	ADDC #$00
	ST VRMAD2
	LD VTRBF
	RET

;----------------------
; | g_digit_info
; | Description: game_sprite_info of the digits at x C, y B, moved on to digit ACC's bitmap.
; | Author: suinevere
; | Dependencies: g_info_at, g_bitmap_add, SPRITE_DIGITS
; | Globals: N/A
; | Params: ACC -- the digit or count; C -- x; B -- y
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_digit_info:
	PUSH ACC
	MOV #SPRITE_DIGITS, ACC
	CALLF g_info_at
	POP ACC
	ROL
	ROL
	ST C
	AND #$03
	ST B
	LD C
	AND #$FC
	JMPF g_bitmap_add

;----------------------
; | g_clear_draw
; | Description: Draws the SpriteInfo at ARG_BLOCK with SPRITE_CLEAR set.
; | Author: suinevere
; | Dependencies: g_draw, sprite.s (SPRITE_CLEAR), game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: ARG_BLOCK+0..11 -- s
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW and ARG_BLOCK+0..11; leaves VSEL bit 4 clear.
;----------------------
g_clear_draw:
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+6), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	OR #SPRITE_CLEAR
	ST VTRBF
	JMPF g_draw

;----------------------
; | g_tag_panel
; | Description: The payout panel and the price tag's third frame at 4, y, leaving the x just right of the tag and the tag's y set up for a number draw.
; | Author: suinevere
; | Dependencies: g_draw_at, g_info_at, g_bitmap_add, g_put_keep, game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: B -- the tag's y, 0-127
; | Returns: TRH:TRL -- the tag's x plus its width; ARG_BLOCK+0..1 -- its y. Destroys ACC, B, C, R0-R3, VRMAD, FPR, PSW and ARG_BLOCK+2..11; leaves VSEL bit 4 clear.
;----------------------
g_tag_panel:
	PUSH B
	MOV #$00, C
	MOV #$00, B
	MOV #$4C, ACC
	CALLF g_draw_at
	POP B
	MOV #$04, C
	MOV #$37, ACC
	CALLF g_info_at
	MOV #$08, ACC
	MOV #$00, B
	CALLF g_bitmap_add
	CALLF g_put_keep
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
	MOV #<(ARG_BLOCK+4), VRMAD1
	LD VTRBF
	ADD TRL
	ST TRL
	MOV #$00, ACC
	ADDC TRH
	ST TRH
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD VTRBF
	ST B
	INC VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+1), VRMAD1
	ST VTRBF
	DEC VRMAD1
	LD B
	ST VTRBF
	RET

;----------------------
; | g_two_anims
; | Description: The two characters at -10, 7 (slot 1) and 10, 7 (slot 0).
; | Author: suinevere
; | Dependencies: g_anim_xy
; | Globals: anim and what g_sprite_draw writes
; | Params: N/A
; | Returns: ARG_BLOCK+0..11 -- slot 0's SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_two_anims:
	MOV #$F6, C
	MOV #$07, B
	MOV #$01, ACC
	CALLF g_anim_xy
	MOV #$0A, C
	MOV #$07, B
	MOV #$00, ACC
	JMPF g_anim_xy

;----------------------
; | g_bag_screen_draw
; | Description: game_bag_screen_draw: steps the reveal and draws by mode_done: the cursor's item from scene 8, the lap price tag, reel A's symbols sliding in, else the board and meter with the bag panel lowered over blank rows.
; | Author: suinevere
; | Dependencies: g_keep13, g_bag_reveal, g_bag_item, g_item_icon_draw, g_tag_panel, g_n, g_pair_number_draw, g_anims_tick, g_sprite_draw, g_anim_draw, g_board_draw, g_meter_draw, g_run_fill, g_xy, g_info_id, g_bitmap_add, g_draw, game.inc (ARG_BLOCK), rrl_ram.inc (HW_MODE_DONE, HW_SCENE_ID, HW_BAG_CURSOR, HW_LAP_POINTS, HW_SCRATCH_FIRST, HW_SCRATCH_COUNT, HW_SCRATCH_KINDS, HW_REEL_LEN, HW_WALK_FRAME, HW_MODE_PHASE, HWW_STRIP_A)
; | Globals: mode_done, scene_id, bag_cursor, bag_items, lap_points, scratch_first, scratch_count, scratch_kinds, reel_len, strip_a, walk_frame, mode_phase, the frame buffer and what the draws write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_bag_screen_draw:
	MOV #<g_bag_screen_draw_body, $00
	MOV #>g_bag_screen_draw_body, $01
	JMPF g_keep13
g_bag_screen_draw_body:
	CALLF g_bag_reveal
	LD HW_MODE_DONE
	BNE #$01, g_bag_screen_draw_tag
	LD HW_SCENE_ID
	SUB #8
	BN PSW, 7, g_bag_screen_draw_cursor
	JMPF g_bag_screen_draw_ret
g_bag_screen_draw_cursor:
	LD HW_BAG_CURSOR
	DEC ACC
	CALLF g_bag_item
	MOV #$00, TRL
	MOV #$00, TRH
	JMPF g_item_icon_draw
g_bag_screen_draw_tag:
	BNE #$02, g_bag_screen_draw_reel
	MOV #$14, B
	CALLF g_tag_panel
	LD HW_LAP_POINTS+1
	ST C
	LD HW_LAP_POINTS
	CALLF g_n
	JMPF g_pair_number_draw
g_bag_screen_draw_reel:
	BE #$03, g_bag_screen_draw_symbols
	JMPF g_bag_screen_draw_board
g_bag_screen_draw_symbols:
	CALLF g_anims_tick
	LD HW_SCRATCH_COUNT
	CLR1 PSW, 7
	BN ACC, 7, g_bag_screen_draw_half
	SET1 PSW, 7
g_bag_screen_draw_half:
	RORC
	ST B
	LD HW_SCRATCH_FIRST
	RORC
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+7), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	ST VTRBF
	LD B
	ST VTRBF
	LD HW_REEL_LEN
	ST VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
g_bag_screen_draw_symbol:
	MOV #<(ARG_BLOCK+8), VRMAD1
	LD VTRBF
	BN ACC, 7, g_bag_screen_draw_right
	JMPF g_bag_screen_draw_go
g_bag_screen_draw_right:
	BZ g_bag_screen_draw_low
	JMPF g_bag_screen_draw_anim
g_bag_screen_draw_low:
	DEC VRMAD1
	LD VTRBF
	SUB #32
	BP PSW, 7, g_bag_screen_draw_go
	JMPF g_bag_screen_draw_anim
g_bag_screen_draw_go:
	MOV #<(ARG_BLOCK+11), VRMAD1
	LD VTRBF
	ADD #<HWW_STRIP_A
	ST VRMAD1
	MOV #>HWW_STRIP_A, ACC
	ADDC #$00
	ST VRMAD2
	LD VTRBF
	ST C
	MOV #$00, ACC
	MOV #12, B
	MUL
	ST B
	LD C
	ADD #$BC
	ST C
	LD B
	ADDC #$7D
	ST B
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	LD C
	ST VTRBF
	LD B
	ST VTRBF
	MOV #$00, VTRBF
	MOV #$02, VTRBF
	LD VTRBF
	LD VTRBF
	ST TRL
	LD VTRBF
	ST TRH
	CLR1 VSEL, 4
	MOV #$08, ACC
	MOV #11, B
	MOV #$00, C
	CALLF g_sprite_draw
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+7), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ADD #8
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	MOV #<(ARG_BLOCK+11), VRMAD1
	LD VTRBF
	INC ACC
	ST VTRBF
	MOV #<(ARG_BLOCK+9), VRMAD1
	LD VTRBF
	SUB #1
	ST VTRBF
	ST C
	INC VRMAD1
	LD VTRBF
	SUBC #$00
	ST VTRBF
	OR C
	BZ g_bag_screen_draw_anim
	JMPF g_bag_screen_draw_symbol
g_bag_screen_draw_anim:
	MOV #$00, TRL
	MOV #$00, TRH
	LD HW_SCRATCH_KINDS
	ST C
	MOV #7, ACC
	SUB C
	ST B
	MOV #$00, ACC
	SUBC #$00
	ST C
	MOV #<ARG_BLOCK, VRMAD1
	LD B
	ST VTRBF
	INC VRMAD1
	LD C
	ST VTRBF
	MOV #$00, ACC
	JMPF g_anim_draw
g_bag_screen_draw_board:
	CALLF g_board_draw
	LD HW_WALK_FRAME
	ST TRL
	MOV #$00, TRH
	CALLF g_meter_draw
	MOV #$00, $00
g_bag_screen_draw_clear:
	LD HW_WALK_FRAME
	CLR1 PSW, 7
	RORC
	ST B
	LD $00
	BE B, g_bag_screen_draw_panel
	MOV #$00, B
	CALLF g_run_fill
	INC $00
	BR g_bag_screen_draw_clear
g_bag_screen_draw_panel:
	MOV #$00, C
	LD HW_WALK_FRAME
	CLR1 PSW, 7
	RORC
	SUB #32
	ST B
	CALLF g_xy
	MOV #$4B, ACC
	MOV #$00, B
	CALLF g_info_id
	LD HW_MODE_PHASE
	ST C
	MOV #$00, ACC
	MOV #$70, B
	MUL
	ST B
	LD C
	CALLF g_bitmap_add
	JMPF g_draw
g_bag_screen_draw_ret:
	RET

;----------------------
; | g_reel_pair_screen
; | Description: game_reel_pair_screen: steps the two-reel game and draws it by mode_done, then gives slot 0 scratch_wait as its argument when the game ended this frame (mode 3 or 14).
; | Author: suinevere
; | Dependencies: g_keep13, g_reel_pair_step, g_anims_tick, g_markers_draw, g_yn, g_anim_draw, g_reel_pair_spin_step, g_reel_anim_screen_draw, g_two_anims, g_info_at, g_put_keep, g_cd_move, g_bitmap_add, g_draw, g_digit_info, g_anim_xy, g_tag_panel, g_n, g_pair_number_draw, g_draw_at, g_item_icon_draw, g_board_meter, rrl_ram.inc (HW_MODE_DONE, HW_MODE_PHASE, HW_MODE, HW_SCRATCH_KINDS, HW_SCRATCH_FIRST, HW_SCRATCH_LO, HW_SCRATCH_HI, HW_LAP_POINTS, HW_BAG_NTH_INDEX, HW_SCRATCH_WAIT, HWW_ANIM)
; | Globals: mode, mode_done, mode_phase, scratch_kinds, scratch_first, scratch_lo, scratch_hi, lap_points, bag_nth_index, scratch_wait, anim, marker and what the steps and draws write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_reel_pair_screen:
	MOV #<g_reel_pair_screen_body, $00
	MOV #>g_reel_pair_screen_body, $01
	JMPF g_keep13
g_reel_pair_screen_body:
	CALLF g_reel_pair_step
	LD HW_MODE_DONE
	BNZ g_reel_pair_screen_spin
	CALLF g_anims_tick
	CALLF g_markers_draw
	LD HW_MODE_PHASE
	ST C
	MOV #$F6, ACC
	SUB C
	ST TRL
	MOV #$FF, ACC
	SUBC #$00
	ST TRH
	MOV #$07, B
	CALLF g_yn
	MOV #$01, ACC
	CALLF g_anim_draw
	LD HW_MODE_PHASE
	ADD #10
	ST TRL
	MOV #$00, ACC
	ADDC #$00
	ST TRH
	MOV #$07, B
	CALLF g_yn
	MOV #$00, ACC
	CALLF g_anim_draw
	JMPF g_reel_pair_screen_end
g_reel_pair_screen_spin:
	BNE #$01, g_reel_pair_screen_drawn
	CALLF g_reel_pair_spin_step
	CALLF g_reel_anim_screen_draw
	JMPF g_reel_pair_screen_end
g_reel_pair_screen_drawn:
	BNE #$03, g_reel_pair_screen_won
	CALLF g_anims_tick
	CALLF g_two_anims
	MOV #$00, C
	MOV #$00, B
	MOV #$48, ACC
	CALLF g_info_at
	CALLF g_put_keep
	MOV #$10, ACC
	MOV #$00, B
	CALLF g_cd_move
	MOV #$38, ACC
	MOV #$00, B
	CALLF g_bitmap_add
	CALLF g_draw
	MOV #$18, C
	MOV #$01, B
	LD HW_SCRATCH_KINDS
	CALLF g_digit_info
	CALLF g_draw
	MOV #$05, C
	MOV #$01, B
	LD HW_SCRATCH_FIRST
	CALLF g_digit_info
	CALLF g_draw
	JMPF g_reel_pair_screen_end
g_reel_pair_screen_won:
	BNE #$04, g_reel_pair_screen_pay
	CALLF g_anims_tick
	CALLF g_markers_draw
	MOV #$00, C
	MOV #$07, B
	MOV #$00, ACC
	CALLF g_anim_xy
	JMPF g_reel_pair_screen_end
g_reel_pair_screen_pay:
	BNE #$06, g_reel_pair_screen_win
	MOV #$0C, B
	CALLF g_tag_panel
	LD HW_SCRATCH_HI
	ST C
	LD HW_SCRATCH_LO
	CALLF g_n
	CALLF g_pair_number_draw
	MOV #10, TRL
	MOV #$00, TRH
	MOV #$18, B
	LD HW_LAP_POINTS+1
	ST C
	LD HW_LAP_POINTS
	CALLF g_yn
	CALLF g_pair_number_draw
	JMPF g_reel_pair_screen_end
g_reel_pair_screen_win:
	BNE #$07, g_reel_pair_screen_loss
	CALLF g_anims_tick
	CALLF g_two_anims
	CALLF g_markers_draw
	JMPF g_reel_pair_screen_end
g_reel_pair_screen_loss:
	BNE #$08, g_reel_pair_screen_prize
	CALLF g_anims_tick
	MOV #$00, C
	MOV #$00, B
	MOV #$49, ACC
	CALLF g_draw_at
	MOV #$00, C
	MOV #$07, B
	MOV #$00, ACC
	CALLF g_anim_xy
	CALLF g_markers_draw
	JMPF g_reel_pair_screen_end
g_reel_pair_screen_prize:
	BNE #$09, g_reel_pair_screen_rest
	MOV #$00, TRL
	MOV #$00, TRH
	LD HW_BAG_NTH_INDEX
	CALLF g_item_icon_draw
	JMPF g_reel_pair_screen_end
g_reel_pair_screen_rest:
	SUB #10
	BP PSW, 7, g_reel_pair_screen_markers
	CALLF g_board_meter
	JMPF g_reel_pair_screen_end
g_reel_pair_screen_markers:
	CALLF g_markers_draw
g_reel_pair_screen_end:
	LD HW_MODE
	BE #$03, g_reel_pair_screen_arg
	BNE #$0E, g_reel_pair_screen_ret
g_reel_pair_screen_arg:
	LD HW_SCRATCH_WAIT
	CLR1 VSEL, 4
	MOV #<(HWW_ANIM+1), VRMAD1
	MOV #>(HWW_ANIM+1), VRMAD2
	ST VTRBF
g_reel_pair_screen_ret:
	RET

;----------------------
; | g_bag_menu_screen
; | Description: game_bag_menu_screen: steps the trade menu and draws by mode_done: the chosen item with its number, the trade question with its cursor, the give-up question with its lit answer, else the kind, its count and the total; the arrows while more than one is on offer.
; | Author: suinevere
; | Dependencies: g_keep13, g_bag_menu_step, g_bag_item, g_item_icon_draw, g_yn, g_small_number_draw, g_draw_at, g_bag_arrows_draw, g_info_at, g_bitmap_add, g_draw, g_icon_draw, g_bag_count, g_digit_info, g_clear_draw, rrl_ram.inc (HW_MODE_DONE, HW_MODE_PHASE, HW_BAG_NTH_INDEX, HW_SCRATCH_COUNT, HW_SCRATCH_LO, HW_SCRATCH_FIRST, HW_SCRATCH_WAIT, HW_SCENE_ID, HW_BAG_KIND_COUNT)
; | Globals: mode_done, mode_phase, bag_items, bag_nth_index, bag_count, bag_kind_count, scratch_first, scratch_lo, scratch_count, scratch_wait, scene_id and what the step and draws write
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep13's sixteen bytes of stack on top of the callees'.
;----------------------
g_bag_menu_screen:
	MOV #<g_bag_menu_screen_body, $00
	MOV #>g_bag_menu_screen_body, $01
	JMPF g_keep13
g_bag_menu_screen_body:
	CALLF g_bag_menu_step
	LD HW_MODE_DONE
	BNE #$01, g_bag_menu_screen_trade
	LD HW_BAG_NTH_INDEX
	CALLF g_bag_item
	MOV #$00, TRL
	MOV #$00, TRH
	CALLF g_item_icon_draw
	LD HW_SCRATCH_COUNT
	ADD #1
	PUSH ACC
	MOV #$00, ACC
	ADDC #$00
	ST C
	POP ACC
	MOV #$1A, B
	CALLF g_yn
	MOV #$00, TRL
	MOV #$00, TRH
	CALLF g_small_number_draw
	MOV #$00, C
	MOV #$00, B
	MOV #$47, ACC
	CALLF g_draw_at
	LD HW_SCRATCH_LO
	JMPF g_bag_arrows_draw
g_bag_menu_screen_trade:
	BNE #$02, g_bag_menu_screen_give
	MOV #$00, C
	MOV #$00, B
	MOV #$46, ACC
	CALLF g_draw_at
	LD HW_MODE_PHASE
	BZ g_bag_menu_screen_row_pick
	JMPF g_bag_menu_screen_ret
g_bag_menu_screen_row_pick:
	LD HW_SCENE_ID
	ST C
	MOV #$00, B
	BZ g_bag_menu_screen_row
	MOV #$07, B
g_bag_menu_screen_row:
	MOV #$33, ACC
	JMPF g_draw_at
g_bag_menu_screen_give:
	BE #$03, g_bag_menu_screen_ask
	JMPF g_bag_menu_screen_kind
g_bag_menu_screen_ask:
	MOV #$00, C
	MOV #$00, B
	MOV #$4D, ACC
	CALLF g_info_at
	LD HW_SCENE_ID
	BNZ g_bag_menu_screen_yes
	LD HW_SCRATCH_WAIT
	SUB #11
	BN PSW, 7, g_bag_menu_screen_yes
	MOV #$1C, ACC
	MOV #$00, B
	CALLF g_bitmap_add
g_bag_menu_screen_yes:
	CALLF g_draw
	MOV #$00, C
	MOV #$00, B
	MOV #$4E, ACC
	CALLF g_info_at
	LD HW_SCENE_ID
	BNE #$01, g_bag_menu_screen_no
	LD HW_SCRATCH_WAIT
	SUB #11
	BN PSW, 7, g_bag_menu_screen_no
	MOV #$18, ACC
	MOV #$00, B
	CALLF g_bitmap_add
g_bag_menu_screen_no:
	CALLF g_draw
	LD HW_MODE_PHASE
	BNZ g_bag_menu_screen_ret
	MOV #$00, C
	MOV #$04, B
	LD HW_SCENE_ID
	BZ g_bag_menu_screen_cursor
	MOV #$15, B
g_bag_menu_screen_cursor:
	MOV #$33, ACC
	JMPF g_draw_at
g_bag_menu_screen_kind:
	MOV #$00, TRL
	MOV #$00, TRH
	MOV #$00, B
	MOV #$00, C
	CALLF g_yn
	LD HW_SCRATCH_FIRST
	MOV #$00, B
	CALLF g_icon_draw
	MOV #$00, TRL
	MOV #$00, TRH
	MOV #$1A, B
	MOV #$00, C
	LD HW_SCRATCH_LO
	CALLF g_yn
	CALLF g_small_number_draw
	MOV #$1B, C
	CALLF g_bag_count
	SUB #10
	BP PSW, 7, g_bag_menu_screen_slash
	MOV #$17, C
g_bag_menu_screen_slash:
	MOV #$1A, B
	MOV #10, ACC
	CALLF g_digit_info
	CALLF g_clear_draw
	MOV #$18, TRL
	MOV #$00, TRH
	MOV #$1A, B
	MOV #$00, C
	CALLF g_bag_count
	CALLF g_yn
	CALLF g_small_number_draw
	LD HW_BAG_KIND_COUNT
	JMPF g_bag_arrows_draw
g_bag_menu_screen_ret:
	RET
