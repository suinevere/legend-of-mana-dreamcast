;----------------------
; | fb.s
; | Description: fb.c in LC86K assembly: the frame present, the clear and the row copy of the game's 32x32 area, the state block's load, and the bit-reversal pages the drawing reads.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/fb.c, pocketstation/build/vmu_game/rrl_ram.inc, game.inc, pocketstation/src/vmu_rt/rt_file.s (rt_save_read), pocketstation/src/vmu_rt/rt_lcd.s (rt_fb_row)
; | Globals: the state block (points to save_tail), XRAM rows 0-31
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_fb_present
; | Description: game_fb_present: the game draws straight into XRAM and game_wait_ticks marks the frame, so it only returns.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_fb_present:
	RET

;----------------------
; | g_state_reset
; | Description: game_state_reset: reads the 128-byte state block at points (vmu_ram 0x25B) from the pet save's sector 6, retrying until rt_save_read reports success.
; | Author: suinevere
; | Dependencies: rt_save_read, rrl_ram.inc (HWW_POINTS)
; | Globals: the state block (points to save_tail)
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0, FPR, PSW; two bytes of stack.
;----------------------
g_state_reset:
	MOV #$06, ACC
	MOV #<(HWW_POINTS+$100), TRL
	MOV #>(HWW_POINTS+$100), TRH
	CALLF rt_save_read
	BNZ g_state_reset
	RET

;----------------------
; | g_fb_clear
; | Description: game_fb_clear: zeroes bytes 1-4 of all 32 frame buffer rows in XRAM, bank 0 then bank 1, two rows per 16-byte line through @R2; nothing while draw_skip is set.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: XRAM rows 0-31, XBNK
; | Params: N/A
; | Returns: N/A. Destroys ACC, C, R2, PSW; leaves XBNK at 1; four bytes of stack.
;----------------------
g_fb_clear:
	LD HW_DRAW_SKIP
	BZ g_fb_clear_go
	RET
g_fb_clear_go:
	MOV #$00, XBNK
	CALLF g_fb_clear_bank
	MOV #$01, XBNK
g_fb_clear_bank:
	MOV #$81, $02
	MOV #$08, C
g_fb_clear_pair:
	MOV #$00, @R2
	INC $02
	MOV #$00, @R2
	INC $02
	MOV #$00, @R2
	INC $02
	MOV #$00, @R2
	LD $02
	ADD #$03
	ST $02
	MOV #$00, @R2
	INC $02
	MOV #$00, @R2
	INC $02
	MOV #$00, @R2
	INC $02
	MOV #$00, @R2
	LD $02
	ADD #$07
	ST $02
	DBNZ C, g_fb_clear_pair
	RET

;----------------------
; | g_fb_put_rows
; | Description: game_fb_put_rows: copies count table rows from src into frame buffer rows row on, each byte bit-reversed through g_bit_rot and inverted when asked; nothing while draw_skip is set.
; | Author: suinevere
; | Dependencies: rt_fb_row, g_bit_rot, g_tb_find, g_fb_src, game.inc (ARG_BLOCK)
; | Globals: XRAM rows, XBNK
; | Params: ACC -- row; B -- count (0 for 256, as the C's loop); ARG_BLOCK+0..3 -- src, a card address whose low 16 bits are the table offset; C -- invert, nonzero to invert
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R0-R3, VRMAD, FPR, PSW; leaves XBNK at the last row's bank and VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_fb_put_rows:
	ST $00
	LD HW_DRAW_SKIP
	BZ g_fb_put_rows_go
	RET
g_fb_put_rows_go:
	LD B
	ST $01
	LD C
	BZ g_fb_put_rows_plain
	MOV #$FF, ACC
g_fb_put_rows_plain:
	ST $03
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
	MOV #$01, VRMAD1
	LD $01
	SUB #$40
	BN PSW, 7, g_fb_put_rows_row
	LD $01
	BZ g_fb_put_rows_row
	ROL
	ROL
	AND #$FC
	ST B
	CALLF g_tb_find
	BN PSW, 7, g_fb_put_rows_row
	MOV #$00, VRMAD1
g_fb_put_rows_row:
	LD $00
	CALLF rt_fb_row
	LD B
	ST XBNK
	LD C
	INC ACC
	ST $02
	MOV #$00, B
g_fb_put_rows_byte:
	CALLF g_fb_src
	PUSH TRL
	PUSH TRH
	MOV #<g_bit_rot, TRL
	MOV #>g_bit_rot, TRH
	LDC
	XOR $03
	ST @R2
	POP TRH
	POP TRL
	INC $02
	INC B
	LD B
	BNE #$04, g_fb_put_rows_byte
	LD TRL
	ADD #$04
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	INC $00
	DBNZ $01, g_fb_put_rows_row
	RET

;----------------------
; | g_fb_src
; | Description: g_fb_put_rows' source byte B of the row at TRH:TRL: by LDC from app_tb_pages, or through rt_rom8 from the pet save when VRMAD1 is nonzero.
; | Author: suinevere
; | Dependencies: rt_rom8
; | Globals: N/A
; | Params: B -- the byte, 0-3; TRH:TRL -- the row's GAME address or file offset; VRMAD1 -- 0 for the GAME, else the save
; | Returns: ACC -- the byte. Keeps TRH, TRL; destroys FPR and PSW's flags.
;----------------------
g_fb_src:
	LD VRMAD1
	BNZ g_fb_src_save
	LD B
	LDC
	RET
g_fb_src_save:
	PUSH TRL
	PUSH TRH
	LD B
	ADD TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	CALLF rt_rom8
	POP TRH
	POP TRL
	RET
