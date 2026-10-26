;----------------------
; | hud.s
; | Description: hud.c's routines in LC86K assembly.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/hud.c, screen.s (g_draw)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_sprite_put
; | Description: sprite_put: the same as draw, hud.c's copy of it.
; | Author: suinevere
; | Dependencies: g_draw
; | Globals: what g_sprite_draw writes
; | Params: ARG_BLOCK+0..11 -- s, a SpriteInfo
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW and ARG_BLOCK+0..11; leaves VSEL bit 4 clear.
;----------------------
g_sprite_put:
	JMPF g_draw

;----------------------
; | g_put_keep
; | Description: sprite_put of the SpriteInfo in ARG_BLOCK+0..11, leaving it there: saves the six bytes the draw takes over and puts them back.
; | Author: suinevere
; | Dependencies: g_draw, game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: ARG_BLOCK+0..11 -- s
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; six bytes of stack on top of g_draw's.
;----------------------
g_put_keep:
	SET1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	CLR1 VSEL, 4
	CALLF g_draw
	MOV #<(ARG_BLOCK+5), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	RET

;----------------------
; | g_info_id
; | Description: game_sprite_info at x TRH:TRL and the y in ARG_BLOCK+0..1 for sprite id B:ACC.
; | Author: suinevere
; | Dependencies: g_sprite_info, game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ACC -- the id's low byte; B -- its high byte
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_info_id:
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+2), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	ST VTRBF
	INC VRMAD1
	LD B
	ST VTRBF
	JMPF g_sprite_info

;----------------------
; | ANIM_SPRITES / WIDE_SPRITES / WIDE_COLUMN
; | Description: hud.c's first anim sprite id, first wide sprite id and the wide sprites' column width.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/hud.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ANIM_SPRITES	EQU	$1C
WIDE_SPRITES	EQU	$4F
WIDE_COLUMN	EQU	$20

;----------------------
; | g_anim_draw
; | Description: game_anim_draw: draws anim slot's sprite at x, y: frames past 1 step two for an odd arg, arg bit 1 mirrors, and the bitmap advances by the frame's size; returns the SpriteInfo.
; | Author: suinevere
; | Dependencies: g_info_id, g_put_keep, ANIM_SPRITES, game.inc (ARG_BLOCK), rrl_ram.inc (HWW_ANIM)
; | Globals: anim and what g_sprite_draw writes
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ACC -- slot, 0-9
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo, over y. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_anim_draw:
	CLR1 VSEL, 4
	ROL
	ROL
	AND #$FC
	ADD #<HWW_ANIM
	ST VRMAD1
	MOV #>HWW_ANIM, VRMAD2
	LD VTRBF
	ST $00
	INC VRMAD1
	LD VTRBF
	ST $01
	INC VRMAD1
	LD VTRBF
	ST $02
	MOV #$00, $03
	LD $00
	SUB #$08
	BP PSW, 7, g_anim_draw_plain
	LD $00
	BE #$16, g_anim_draw_plain
	LD $01
	AND #$01
	BZ g_anim_draw_info
	LD $02
	ADD #$02
	ST $02
	MOV #$00, ACC
	ADDC #$00
	ST $03
	BR g_anim_draw_info
g_anim_draw_plain:
	MOV #$00, $01
g_anim_draw_info:
	PUSH $01
	PUSH $02
	PUSH $03
	LD $00
	ADD #ANIM_SPRITES
	PUSH ACC
	MOV #$00, ACC
	ADDC #$00
	ST B
	POP ACC
	CALLF g_info_id
	POP $03
	POP $02
	POP $01
	MOV #<(ARG_BLOCK+6), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD $01
	AND #$02
	BZ g_anim_draw_size
	LD VTRBF
	OR #$04
	ST VTRBF
g_anim_draw_size:
	MOV #<(ARG_BLOCK+4), VRMAD1
	LD VTRBF
	ST C
	INC VRMAD1
	LD VTRBF
	ST B
	MOV #$00, ACC
	MUL
	ST TRH
	LD C
	ADD #31
	ST C
	LD TRH
	ADDC #$00
	ST TRH
	MOV #$02, B
g_anim_draw_div:
	CLR1 PSW, 7
	LD TRH
	RORC
	ST TRH
	LD C
	RORC
	ST C
	DBNZ B, g_anim_draw_div
	LD C
	AND #$F8
	ST C
	ST TRL
	LD $02
	ST B
	LD TRH
	MUL
	ST $00
	LD $03
	BZ g_anim_draw_add
	LD $00
	ADD TRL
	ST $00
	LD B
	ADDC TRH
	ST B
g_anim_draw_add:
	MOV #<(ARG_BLOCK+8), VRMAD1
	LD VTRBF
	ADD C
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC $00
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC B
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	JMPF g_put_keep

;----------------------
; | g_wide_draw
; | Description: game_wide_draw: draws wide sprite id at x, y as 32-pixel columns while x is left of 32; returns the SpriteInfo after the last column.
; | Author: suinevere
; | Dependencies: g_info_id, g_put_keep, WIDE_SPRITES, WIDE_COLUMN, game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ARG_BLOCK+2..3 -- id
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo, over y and id. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; one byte of stack on top of g_put_keep's.
;----------------------
g_wide_draw:
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+3), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST B
	DEC VRMAD1
	LD VTRBF
	ADD #WIDE_SPRITES
	PUSH ACC
	LD B
	ADDC #$00
	ST B
	POP ACC
	CALLF g_info_id
	MOV #<(ARG_BLOCK+4), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	PUSH ACC
	MOV #WIDE_COLUMN, VTRBF
g_wide_draw_column:
	MOV #<(ARG_BLOCK+1), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	BN ACC, 7, g_wide_draw_right
	BR g_wide_draw_put
g_wide_draw_right:
	BNZ g_wide_draw_done
	DEC VRMAD1
	LD VTRBF
	SUB #WIDE_COLUMN
	BN PSW, 7, g_wide_draw_done
g_wide_draw_put:
	CALLF g_put_keep
	MOV #<(ARG_BLOCK+8), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ADD #WIDE_COLUMN
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
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ADD #WIDE_COLUMN
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	POP ACC
	SUB #WIDE_COLUMN
	BP PSW, 7, g_wide_draw_ret
	PUSH ACC
	BR g_wide_draw_column
g_wide_draw_done:
	POP ACC
g_wide_draw_ret:
	RET


;----------------------
; | g_bitmap_add
; | Description: Adds the signed 16-bit B:ACC to the 32-bit bitmap of the SpriteInfo at ARG_BLOCK+8..11.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- the low byte; B -- the high byte
; | Returns: N/A. Destroys ACC, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_bitmap_add:
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+8), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	ADD VTRBF
	ST VTRBF
	INC VRMAD1
	LD B
	ADDC VTRBF
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	BP B, 7, g_bitmap_add_down
	ADDC #$00
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	RET
g_bitmap_add_down:
	ADDC #$FF
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$FF
	ST VTRBF
	RET

;----------------------
; | g_put_at
; | Description: g_put_keep with the SpriteInfo moved by dx, dy for the draw only.
; | Author: suinevere
; | Dependencies: g_draw, game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: ARG_BLOCK+0..11 -- s; ACC -- dx, signed; B -- dy, signed
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; six bytes of stack on top of g_draw's.
;----------------------
g_put_at:
	ST C
	SET1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ADD C
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	BP C, 7, g_put_at_left
	ADDC #$00
	BR g_put_at_x
g_put_at_left:
	ADDC #$FF
g_put_at_x:
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADD B
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	BP B, 7, g_put_at_up
	ADDC #$00
	BR g_put_at_y
g_put_at_up:
	ADDC #$FF
g_put_at_y:
	ST VTRBF
	CALLF g_draw
	MOV #<(ARG_BLOCK+5), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	RET

;----------------------
; | ICON_SPRITES / ICON_PLAIN_SPRITES / SPRITE_ICON_FULL / SPRITE_MARK / ROM_MARK_OFFSETS_OFF / SPRITE_DIGITS / SMALL_DIGIT_W
; | Description: hud.c's icon, plain icon, full-bag and mark sprite ids, the mark offsets' file offset, the digits sprite id and the small digits' width.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/hud.c, pocketstation/src/vmu/game.h, pocketstation/src/vmu/rrl_rom.h
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ICON_SPRITES	EQU	$39
ICON_PLAIN_SPRITES	EQU	$3D
SPRITE_ICON_FULL	EQU	$41
SPRITE_MARK	EQU	$65
ROM_MARK_OFFSETS_OFF	EQU	$A7A4
SPRITE_DIGITS	EQU	$35
SMALL_DIGIT_W	EQU	4

;----------------------
; | g_icon_draw
; | Description: game_icon_draw: draws item type's icon at x, y, then a blinking full-bag mark for 4 marks, the plain icon for none, or that many blinking marks at the table's offsets; returns the last SpriteInfo.
; | Author: suinevere
; | Dependencies: g_info_id, g_put_keep, g_put_at, g_bitmap_add, app_tables, ICON_*, SPRITE_ICON_FULL, SPRITE_MARK, ROM_MARK_OFFSETS_OFF, game.inc (ARG_BLOCK), rrl_ram.inc (HW_CYCLE16)
; | Globals: cycle16 and what g_sprite_draw writes
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ACC -- type; B -- marks
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo, over y. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; six bytes of stack on top of g_put_keep's.
;----------------------
g_icon_draw:
	PUSH B
	PUSH ACC
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH VTRBF
	INC VRMAD1
	PUSH VTRBF
	PUSH TRL
	PUSH TRH
	ADD #ICON_SPRITES
	MOV #$00, B
	CALLF g_info_id
	CALLF g_put_keep
	POP TRH
	POP TRL
	MOV #<(ARG_BLOCK+1), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	POP C
	POP B
	LD B
	BNE #$04, g_icon_draw_none
	MOV #SPRITE_ICON_FULL, ACC
	MOV #$00, B
	CALLF g_info_id
	MOV #$50, ACC
	CALLF g_icon_blink
	JMPF g_put_keep
g_icon_draw_none:
	BNZ g_icon_draw_marks
	LD C
	ADD #ICON_PLAIN_SPRITES
	MOV #$00, B
	CALLF g_info_id
	JMPF g_put_keep
g_icon_draw_marks:
	PUSH B
	MOV #SPRITE_MARK, ACC
	MOV #$00, B
	CALLF g_info_id
	MOV #$20, ACC
	CALLF g_icon_blink
	POP B
	MOV #$00, C
	LD B
	BNE #$02, g_icon_draw_three
	MOV #$02, C
g_icon_draw_three:
	BNE #$03, g_icon_draw_mark
	MOV #$06, C
g_icon_draw_mark:
	PUSH B
	PUSH C
	MOV #<(app_tables+ROM_MARK_OFFSETS_OFF), TRL
	MOV #>(app_tables+ROM_MARK_OFFSETS_OFF), TRH
	LD C
	INC ACC
	LDC
	ST B
	LD C
	LDC
	CALLF g_put_at
	POP C
	POP B
	INC C
	INC C
	DBNZ B, g_icon_draw_mark
	RET

;----------------------
; | g_icon_blink
; | Description: For g_icon_draw: adds ACC to the SpriteInfo's bitmap in the second half of each 16-frame cycle.
; | Author: suinevere
; | Dependencies: g_bitmap_add, rrl_ram.inc (HW_CYCLE16)
; | Globals: cycle16
; | Params: ACC -- the step; ARG_BLOCK+8..11 -- the bitmap
; | Returns: N/A. Destroys ACC, B, VRMAD, PSW.
;----------------------
g_icon_blink:
	ST B
	LD HW_CYCLE16
	SUB #$08
	BN PSW, 7, g_icon_blink_on
	RET
g_icon_blink_on:
	LD B
	MOV #$00, B
	JMPF g_bitmap_add

;----------------------
; | g_digits_base
; | Description: Sets the SpriteInfo's bitmap at ARG_BLOCK+8..11 to the digits sprite's bitmap plus 4 * B:ACC.
; | Author: suinevere
; | Dependencies: app_tables, ROM_SPRITES_OFF, SPRITE_DIGITS, g_bitmap_add, game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- the digit's low byte; B -- its high byte, 0-63
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_digits_base:
	ST C
	CLR1 PSW, 7
	ROLC
	ST C
	LD B
	ROLC
	ST B
	LD C
	CLR1 PSW, 7
	ROLC
	ST C
	LD B
	ROLC
	ST B
	PUSH B
	PUSH C
	MOV #<(app_tables+ROM_SPRITES_OFF+SPRITE_DIGITS*8+4), TRL
	MOV #>(app_tables+ROM_SPRITES_OFF+SPRITE_DIGITS*8+4), TRH
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+8), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #$00, C
g_digits_base_byte:
	LD C
	LDC
	ST VTRBF
	INC VRMAD1
	INC C
	LD C
	BNE #$04, g_digits_base_byte
	POP ACC
	POP B
	JMPF g_bitmap_add

;----------------------
; | g_places
; | Description: game_number_draw's NUMBER_PLACES: 10000, 1000, 100, 10, little-endian halfwords.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/hud.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
g_places:
	.byte $10, $27, $E8, $03, $64, $00, $0A, $00

;----------------------
; | g_number_draw
; | Description: game_number_draw: draws n in the large digits from x, y without leading zeros past the units; returns the last digit's bitmap.
; | Author: suinevere
; | Dependencies: g_info_id, g_put_keep, g_digits_base, g_place, SPRITE_DIGITS, game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ARG_BLOCK+2..5 -- n (each place's digit below 8192, as for every caller's n of at most 99999)
; | Returns: ARG_BLOCK+6..9 -- the last bitmap. Keeps ARG_BLOCK+10..12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; seven bytes of stack on top of g_put_keep's.
;----------------------
g_number_draw:
	SET1 VSEL, 4
	MOV #<(ARG_BLOCK+10), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #3, B
g_number_draw_save:
	PUSH VTRBF
	DBNZ B, g_number_draw_save
	CLR1 VSEL, 4
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+2), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST $00
	INC VRMAD1
	LD VTRBF
	ST $01
	INC VRMAD1
	LD VTRBF
	ST $02
	INC VRMAD1
	LD VTRBF
	ST $03
	PUSH TRL
	PUSH TRH
	MOV #$00, TRL
g_number_draw_lead:
	LD TRL
	CALLF g_place
	LD $00
	SUB B
	LD $01
	SUBC C
	LD $02
	SUBC #$00
	LD $03
	SUBC #$00
	BN PSW, 7, g_number_draw_info
	INC TRL
	LD TRL
	BNE #$04, g_number_draw_lead
g_number_draw_info:
	MOV #<(ARG_BLOCK+12), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD TRL
	ST VTRBF
	POP TRH
	POP TRL
	PUSH $00
	PUSH $01
	PUSH $02
	PUSH $03
	MOV #SPRITE_DIGITS, ACC
	MOV #$00, B
	CALLF g_info_id
	POP $03
	POP $02
	POP $01
	POP $00
g_number_draw_place:
	MOV #<(ARG_BLOCK+12), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	BNE #$04, g_number_draw_more
	JMPF g_number_draw_units
g_number_draw_more:
	CALLF g_place
	MOV #$00, TRL
	MOV #$00, TRH
g_number_draw_sub:
	LD $00
	SUB B
	PUSH ACC
	LD $01
	SUBC C
	PUSH ACC
	LD $02
	SUBC #$00
	PUSH ACC
	LD $03
	SUBC #$00
	BP PSW, 7, g_number_draw_digit
	ST $03
	POP $02
	POP $01
	POP $00
	INC TRL
	LD TRL
	BNZ g_number_draw_sub
	INC TRH
	BR g_number_draw_sub
g_number_draw_digit:
	POP ACC
	POP ACC
	POP ACC
	MOV #<(ARG_BLOCK+12), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	INC ACC
	ST VTRBF
	PUSH $00
	PUSH $01
	PUSH $02
	PUSH $03
	LD TRH
	ST B
	LD TRL
	CALLF g_digits_base
	CALLF g_put_keep
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST C
	MOV #<(ARG_BLOCK+4), VRMAD1
	LD VTRBF
	MOV #<ARG_BLOCK, VRMAD1
	ADD C
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	POP $03
	POP $02
	POP $01
	POP $00
	JMPF g_number_draw_place
g_number_draw_units:
	LD $01
	ST B
	LD $00
	CALLF g_digits_base
	CALLF g_put_keep
	MOV #<(ARG_BLOCK+8), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #$04, B
g_number_draw_copy:
	LD VTRBF
	DEC VRMAD1
	DEC VRMAD1
	ST VTRBF
	INC VRMAD1
	INC VRMAD1
	INC VRMAD1
	DBNZ B, g_number_draw_copy
	MOV #<(ARG_BLOCK+12), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #3, B
g_number_draw_back:
	POP VTRBF
	DEC VRMAD1
	DBNZ B, g_number_draw_back
	RET

;----------------------
; | g_place
; | Description: For g_number_draw: place ACC of g_places in C:B.
; | Author: suinevere
; | Dependencies: g_places
; | Globals: N/A
; | Params: ACC -- the place index, 0-3
; | Returns: B -- the place's low byte; C -- its high byte. Keeps TRH and TRL; destroys ACC, PSW.
;----------------------
g_place:
	PUSH TRL
	PUSH TRH
	ROL
	AND #$FE
	ST C
	MOV #<g_places, TRL
	MOV #>g_places, TRH
	LDC
	ST B
	LD C
	INC ACC
	LDC
	ST C
	POP TRH
	POP TRL
	RET

;----------------------
; | g_small_number_put
; | Description: small_number_put: draws n in the small digits from the SpriteInfo's x, the tens only from 10, and returns the SpriteInfo of the units.
; | Author: suinevere
; | Dependencies: g_put_keep, g_bitmap_add, SMALL_DIGIT_W, game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: ARG_BLOCK+0..11 -- s; TRH:TRL -- n, 0-639 (callers pass at most 256)
; | Returns: ARG_BLOCK+0..11 -- the units' SpriteInfo, over s. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; two bytes of stack on top of g_put_keep's.
;----------------------
g_small_number_put:
	LD TRH
	BNZ g_small_number_put_tens
	LD TRL
	SUB #10
	BN PSW, 7, g_small_number_put_tens
	LD TRL
	PUSH ACC
	BR g_small_number_put_units
g_small_number_put_tens:
	LD TRL
	ST C
	LD TRH
	MOV #10, B
	DIV
	PUSH B
	LD C
	PUSH ACC
	ROL
	ROL
	AND #$FC
	MOV #$00, B
	CALLF g_bitmap_add
	CALLF g_put_keep
	POP ACC
	ROL
	ROL
	AND #$FC
	XOR #$FF
	INC ACC
	MOV #$FF, B
	CALLF g_bitmap_add
g_small_number_put_units:
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ADD #SMALL_DIGIT_W
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	POP ACC
	ROL
	ROL
	AND #$FC
	MOV #$00, B
	CALLF g_bitmap_add
	JMPF g_put_keep

;----------------------
; | SPRITE_METER_LABEL / METER_DIGIT_X / DIGIT_BLANK
; | Description: hud.c's meter label sprite, the meter digit's x and the blank digit.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/hud.c, pocketstation/src/vmu/game.h
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
SPRITE_METER_LABEL	EQU	$38
METER_DIGIT_X	EQU	$1C
DIGIT_BLANK	EQU	10

;----------------------
; | g_meter_draw
; | Description: game_meter_draw: the meter label at y unless it blinks off while rising, then the meter digit below it, blank or old while it rises; returns the digit's SpriteInfo.
; | Author: suinevere
; | Dependencies: g_info_id, g_draw, g_put_keep, g_bitmap_add, SPRITE_*, sprite.s (SPRITE_CLEAR), METER_DIGIT_X, DIGIT_BLANK, game.inc (ARG_BLOCK), rrl_ram.inc (HW_METER_SHOWN, HW_METER, HW_METER_DELAY)
; | Globals: meter_shown, meter, meter_delay and what g_sprite_draw writes
; | Params: TRH:TRL -- y
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; three bytes of stack on top of g_put_keep's.
;----------------------
g_meter_draw:
	PUSH TRL
	PUSH TRH
	LD HW_METER_DELAY
	ST C
	MOV #$00, ACC
	MOV #20, B
	DIV
	LD B
	PUSH ACC
	LD HW_METER_SHOWN
	SUB HW_METER
	BN PSW, 7, g_meter_draw_label
	LD B
	SUB #10
	BP PSW, 7, g_meter_draw_digit
g_meter_draw_label:
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD TRL
	ST VTRBF
	INC VRMAD1
	LD TRH
	ST VTRBF
	MOV #$00, TRL
	MOV #$00, TRH
	MOV #SPRITE_METER_LABEL, ACC
	MOV #$00, B
	CALLF g_info_id
	CALLF g_draw
g_meter_draw_digit:
	POP C
	POP TRH
	POP TRL
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD TRL
	ADD #$18
	ST VTRBF
	INC VRMAD1
	LD TRH
	ADDC #$00
	ST VTRBF
	PUSH C
	MOV #METER_DIGIT_X, TRL
	MOV #$00, TRH
	MOV #SPRITE_DIGITS, ACC
	MOV #$00, B
	CALLF g_info_id
	POP C
	LD HW_METER
	SUB HW_METER_SHOWN
	BP PSW, 7, g_meter_draw_rising
	LD HW_METER_SHOWN
	BR g_meter_draw_add
g_meter_draw_rising:
	MOV #DIGIT_BLANK, ACC
	PUSH ACC
	LD C
	SUB #10
	POP ACC
	BP PSW, 7, g_meter_draw_add
	LD HW_METER_DELAY
	SUB #30
	LD HW_METER_SHOWN
	BP PSW, 7, g_meter_draw_add
	LD HW_METER
g_meter_draw_add:
	ROL
	ROL
	AND #$FC
	MOV #$00, B
	CALLF g_bitmap_add
	MOV #<(ARG_BLOCK+6), VRMAD1
	LD VTRBF
	OR #SPRITE_CLEAR
	ST VTRBF
	JMPF g_put_keep

;----------------------
; | SPRITE_SMALL_DIGITS / SMALL_SLASH
; | Description: hud.c's small digits sprite and the slash's place among them.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/hud.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
SPRITE_SMALL_DIGITS	EQU	$36
SMALL_SLASH	EQU	10

;----------------------
; | g_small_number_draw
; | Description: game_small_number_draw: n in the small digits, drawn cleared, from x, y.
; | Author: suinevere
; | Dependencies: g_keep1, g_info_id, g_small_number_put, SPRITE_SMALL_DIGITS, sprite.s (SPRITE_CLEAR), game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ARG_BLOCK+2..5 -- n, at most 256 (every caller passes a byte or a byte plus one)
; | Returns: ARG_BLOCK+0..11 -- the units' SpriteInfo, over y and n. Keeps ARG_BLOCK+12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep1's four bytes of stack on top of the callees'.
;----------------------
g_small_number_draw:
	MOV #<g_small_number_draw_body, $00
	MOV #>g_small_number_draw_body, $01
	JMPF g_keep1
g_small_number_draw_body:
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+2), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	PUSH ACC
	INC VRMAD1
	LD VTRBF
	PUSH ACC
	MOV #SPRITE_SMALL_DIGITS, ACC
	MOV #$00, B
	CALLF g_info_id
	MOV #<(ARG_BLOCK+6), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	OR #SPRITE_CLEAR
	ST VTRBF
	POP TRH
	POP TRL
	JMPF g_small_number_put

;----------------------
; | g_count_of_draw
; | Description: game_count_of_draw: count in the small digits from x, y, total two digits past its units (a lone digit below 10) and the slash between them.
; | Author: suinevere
; | Dependencies: g_keep1, g_info_id, g_small_number_put, g_put_keep, g_bitmap_add, g_cd_move, SPRITE_SMALL_DIGITS, SMALL_SLASH, game.inc (ARG_BLOCK)
; | Globals: what g_sprite_draw writes
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ARG_BLOCK+2..5 -- count, below 256 (the caller passes bag_count); ACC -- total
; | Returns: ARG_BLOCK+0..11 -- the slash's SpriteInfo, over y and count. Keeps ARG_BLOCK+12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; g_keep1's four bytes of stack on top of the callees'.
;----------------------
g_count_of_draw:
	MOV #<g_count_of_draw_body, $00
	MOV #>g_count_of_draw_body, $01
	JMPF g_keep1
g_count_of_draw_body:
	PUSH ACC
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+2), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	PUSH ACC
	MOV #SPRITE_SMALL_DIGITS, ACC
	MOV #$00, B
	CALLF g_info_id
	POP TRL
	MOV #$00, TRH
	MOV #<(ARG_BLOCK+8), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	SET1 VSEL, 4
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	CLR1 VSEL, 4
	CALLF g_small_number_put
	MOV #$08, ACC
	MOV #$00, B
	CALLF g_cd_move
	MOV #<(ARG_BLOCK+11), VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	POP ACC
	PUSH ACC
	SUB #10
	BN PSW, 7, g_count_of_draw_tens
	POP ACC
	PUSH ACC
	ROL
	ROL
	AND #$FC
	MOV #$00, B
	CALLF g_bitmap_add
	CALLF g_put_keep
	MOV #$FC, ACC
	BR g_count_of_draw_slash
g_count_of_draw_tens:
	POP ACC
	PUSH ACC
	ST TRL
	MOV #$00, TRH
	CALLF g_small_number_put
	MOV #$F8, ACC
g_count_of_draw_slash:
	MOV #$FF, B
	CALLF g_cd_move
	POP ACC
	ST C
	MOV #$00, ACC
	MOV #10, B
	DIV
	LD B
	ROL
	ROL
	AND #$FC
	XOR #$FF
	ADD #SMALL_SLASH * 4 + 1
	MOV #$00, B
	CALLF g_bitmap_add
	JMPF g_put_keep

;----------------------
; | g_cd_move
; | Description: Moves the SpriteInfo at ARG_BLOCK by B:ACC in x.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- dx's low byte; B -- its high byte
; | Returns: N/A. Destroys ACC, C, VRMAD, PSW; leaves VSEL bit 4 clear and VRMAD2 at ARG_BLOCK's page.
;----------------------
g_cd_move:
	ST C
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ADD C
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ADDC B
	ST VTRBF
	RET
