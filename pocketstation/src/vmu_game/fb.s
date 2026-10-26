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
; | Description: game_fb_clear: zeroes bytes 1-4 of all 32 frame buffer rows in XRAM, bank 0 then bank 1, two rows per 16-byte line through @R2.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: XRAM rows 0-31, XBNK
; | Params: N/A
; | Returns: N/A. Destroys ACC, C, R2, PSW; leaves XBNK at 1; four bytes of stack.
;----------------------
g_fb_clear:
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
; | Description: game_fb_put_rows: copies count table rows from src into frame buffer rows row on, each byte bit-reversed through g_bit_rot and inverted when asked.
; | Author: suinevere
; | Dependencies: rt_fb_row, g_bit_rot, g_tb_find, g_fb_src, game.inc (ARG_BLOCK)
; | Globals: XRAM rows, XBNK
; | Params: ACC -- row; B -- count (0 for 256, as the C's loop); ARG_BLOCK+0..3 -- src, a card address whose low 16 bits are the table offset; C -- invert, nonzero to invert
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R0-R3, VRMAD, FPR, PSW; leaves XBNK at the last row's bank and VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_fb_put_rows:
	ST $00
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

;----------------------
; | g_bit_rot
; | Description: Eight 256-aligned pages, so a lookup needs only TRH: page r holds every byte bit-reversed and rotated right r places. ANDing with 0xFF >> r keeps the shifted part, its complement the spill.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.cnop 0, $100
g_bit_rot:
	.byte $00, $80, $40, $C0, $20, $A0, $60, $E0, $10, $90, $50, $D0, $30, $B0, $70, $F0
	.byte $08, $88, $48, $C8, $28, $A8, $68, $E8, $18, $98, $58, $D8, $38, $B8, $78, $F8
	.byte $04, $84, $44, $C4, $24, $A4, $64, $E4, $14, $94, $54, $D4, $34, $B4, $74, $F4
	.byte $0C, $8C, $4C, $CC, $2C, $AC, $6C, $EC, $1C, $9C, $5C, $DC, $3C, $BC, $7C, $FC
	.byte $02, $82, $42, $C2, $22, $A2, $62, $E2, $12, $92, $52, $D2, $32, $B2, $72, $F2
	.byte $0A, $8A, $4A, $CA, $2A, $AA, $6A, $EA, $1A, $9A, $5A, $DA, $3A, $BA, $7A, $FA
	.byte $06, $86, $46, $C6, $26, $A6, $66, $E6, $16, $96, $56, $D6, $36, $B6, $76, $F6
	.byte $0E, $8E, $4E, $CE, $2E, $AE, $6E, $EE, $1E, $9E, $5E, $DE, $3E, $BE, $7E, $FE
	.byte $01, $81, $41, $C1, $21, $A1, $61, $E1, $11, $91, $51, $D1, $31, $B1, $71, $F1
	.byte $09, $89, $49, $C9, $29, $A9, $69, $E9, $19, $99, $59, $D9, $39, $B9, $79, $F9
	.byte $05, $85, $45, $C5, $25, $A5, $65, $E5, $15, $95, $55, $D5, $35, $B5, $75, $F5
	.byte $0D, $8D, $4D, $CD, $2D, $AD, $6D, $ED, $1D, $9D, $5D, $DD, $3D, $BD, $7D, $FD
	.byte $03, $83, $43, $C3, $23, $A3, $63, $E3, $13, $93, $53, $D3, $33, $B3, $73, $F3
	.byte $0B, $8B, $4B, $CB, $2B, $AB, $6B, $EB, $1B, $9B, $5B, $DB, $3B, $BB, $7B, $FB
	.byte $07, $87, $47, $C7, $27, $A7, $67, $E7, $17, $97, $57, $D7, $37, $B7, $77, $F7
	.byte $0F, $8F, $4F, $CF, $2F, $AF, $6F, $EF, $1F, $9F, $5F, $DF, $3F, $BF, $7F, $FF
	.byte $00, $40, $20, $60, $10, $50, $30, $70, $08, $48, $28, $68, $18, $58, $38, $78
	.byte $04, $44, $24, $64, $14, $54, $34, $74, $0C, $4C, $2C, $6C, $1C, $5C, $3C, $7C
	.byte $02, $42, $22, $62, $12, $52, $32, $72, $0A, $4A, $2A, $6A, $1A, $5A, $3A, $7A
	.byte $06, $46, $26, $66, $16, $56, $36, $76, $0E, $4E, $2E, $6E, $1E, $5E, $3E, $7E
	.byte $01, $41, $21, $61, $11, $51, $31, $71, $09, $49, $29, $69, $19, $59, $39, $79
	.byte $05, $45, $25, $65, $15, $55, $35, $75, $0D, $4D, $2D, $6D, $1D, $5D, $3D, $7D
	.byte $03, $43, $23, $63, $13, $53, $33, $73, $0B, $4B, $2B, $6B, $1B, $5B, $3B, $7B
	.byte $07, $47, $27, $67, $17, $57, $37, $77, $0F, $4F, $2F, $6F, $1F, $5F, $3F, $7F
	.byte $80, $C0, $A0, $E0, $90, $D0, $B0, $F0, $88, $C8, $A8, $E8, $98, $D8, $B8, $F8
	.byte $84, $C4, $A4, $E4, $94, $D4, $B4, $F4, $8C, $CC, $AC, $EC, $9C, $DC, $BC, $FC
	.byte $82, $C2, $A2, $E2, $92, $D2, $B2, $F2, $8A, $CA, $AA, $EA, $9A, $DA, $BA, $FA
	.byte $86, $C6, $A6, $E6, $96, $D6, $B6, $F6, $8E, $CE, $AE, $EE, $9E, $DE, $BE, $FE
	.byte $81, $C1, $A1, $E1, $91, $D1, $B1, $F1, $89, $C9, $A9, $E9, $99, $D9, $B9, $F9
	.byte $85, $C5, $A5, $E5, $95, $D5, $B5, $F5, $8D, $CD, $AD, $ED, $9D, $DD, $BD, $FD
	.byte $83, $C3, $A3, $E3, $93, $D3, $B3, $F3, $8B, $CB, $AB, $EB, $9B, $DB, $BB, $FB
	.byte $87, $C7, $A7, $E7, $97, $D7, $B7, $F7, $8F, $CF, $AF, $EF, $9F, $DF, $BF, $FF
	.byte $00, $20, $10, $30, $08, $28, $18, $38, $04, $24, $14, $34, $0C, $2C, $1C, $3C
	.byte $02, $22, $12, $32, $0A, $2A, $1A, $3A, $06, $26, $16, $36, $0E, $2E, $1E, $3E
	.byte $01, $21, $11, $31, $09, $29, $19, $39, $05, $25, $15, $35, $0D, $2D, $1D, $3D
	.byte $03, $23, $13, $33, $0B, $2B, $1B, $3B, $07, $27, $17, $37, $0F, $2F, $1F, $3F
	.byte $80, $A0, $90, $B0, $88, $A8, $98, $B8, $84, $A4, $94, $B4, $8C, $AC, $9C, $BC
	.byte $82, $A2, $92, $B2, $8A, $AA, $9A, $BA, $86, $A6, $96, $B6, $8E, $AE, $9E, $BE
	.byte $81, $A1, $91, $B1, $89, $A9, $99, $B9, $85, $A5, $95, $B5, $8D, $AD, $9D, $BD
	.byte $83, $A3, $93, $B3, $8B, $AB, $9B, $BB, $87, $A7, $97, $B7, $8F, $AF, $9F, $BF
	.byte $40, $60, $50, $70, $48, $68, $58, $78, $44, $64, $54, $74, $4C, $6C, $5C, $7C
	.byte $42, $62, $52, $72, $4A, $6A, $5A, $7A, $46, $66, $56, $76, $4E, $6E, $5E, $7E
	.byte $41, $61, $51, $71, $49, $69, $59, $79, $45, $65, $55, $75, $4D, $6D, $5D, $7D
	.byte $43, $63, $53, $73, $4B, $6B, $5B, $7B, $47, $67, $57, $77, $4F, $6F, $5F, $7F
	.byte $C0, $E0, $D0, $F0, $C8, $E8, $D8, $F8, $C4, $E4, $D4, $F4, $CC, $EC, $DC, $FC
	.byte $C2, $E2, $D2, $F2, $CA, $EA, $DA, $FA, $C6, $E6, $D6, $F6, $CE, $EE, $DE, $FE
	.byte $C1, $E1, $D1, $F1, $C9, $E9, $D9, $F9, $C5, $E5, $D5, $F5, $CD, $ED, $DD, $FD
	.byte $C3, $E3, $D3, $F3, $CB, $EB, $DB, $FB, $C7, $E7, $D7, $F7, $CF, $EF, $DF, $FF
	.byte $00, $10, $08, $18, $04, $14, $0C, $1C, $02, $12, $0A, $1A, $06, $16, $0E, $1E
	.byte $01, $11, $09, $19, $05, $15, $0D, $1D, $03, $13, $0B, $1B, $07, $17, $0F, $1F
	.byte $80, $90, $88, $98, $84, $94, $8C, $9C, $82, $92, $8A, $9A, $86, $96, $8E, $9E
	.byte $81, $91, $89, $99, $85, $95, $8D, $9D, $83, $93, $8B, $9B, $87, $97, $8F, $9F
	.byte $40, $50, $48, $58, $44, $54, $4C, $5C, $42, $52, $4A, $5A, $46, $56, $4E, $5E
	.byte $41, $51, $49, $59, $45, $55, $4D, $5D, $43, $53, $4B, $5B, $47, $57, $4F, $5F
	.byte $C0, $D0, $C8, $D8, $C4, $D4, $CC, $DC, $C2, $D2, $CA, $DA, $C6, $D6, $CE, $DE
	.byte $C1, $D1, $C9, $D9, $C5, $D5, $CD, $DD, $C3, $D3, $CB, $DB, $C7, $D7, $CF, $DF
	.byte $20, $30, $28, $38, $24, $34, $2C, $3C, $22, $32, $2A, $3A, $26, $36, $2E, $3E
	.byte $21, $31, $29, $39, $25, $35, $2D, $3D, $23, $33, $2B, $3B, $27, $37, $2F, $3F
	.byte $A0, $B0, $A8, $B8, $A4, $B4, $AC, $BC, $A2, $B2, $AA, $BA, $A6, $B6, $AE, $BE
	.byte $A1, $B1, $A9, $B9, $A5, $B5, $AD, $BD, $A3, $B3, $AB, $BB, $A7, $B7, $AF, $BF
	.byte $60, $70, $68, $78, $64, $74, $6C, $7C, $62, $72, $6A, $7A, $66, $76, $6E, $7E
	.byte $61, $71, $69, $79, $65, $75, $6D, $7D, $63, $73, $6B, $7B, $67, $77, $6F, $7F
	.byte $E0, $F0, $E8, $F8, $E4, $F4, $EC, $FC, $E2, $F2, $EA, $FA, $E6, $F6, $EE, $FE
	.byte $E1, $F1, $E9, $F9, $E5, $F5, $ED, $FD, $E3, $F3, $EB, $FB, $E7, $F7, $EF, $FF
	.byte $00, $08, $04, $0C, $02, $0A, $06, $0E, $01, $09, $05, $0D, $03, $0B, $07, $0F
	.byte $80, $88, $84, $8C, $82, $8A, $86, $8E, $81, $89, $85, $8D, $83, $8B, $87, $8F
	.byte $40, $48, $44, $4C, $42, $4A, $46, $4E, $41, $49, $45, $4D, $43, $4B, $47, $4F
	.byte $C0, $C8, $C4, $CC, $C2, $CA, $C6, $CE, $C1, $C9, $C5, $CD, $C3, $CB, $C7, $CF
	.byte $20, $28, $24, $2C, $22, $2A, $26, $2E, $21, $29, $25, $2D, $23, $2B, $27, $2F
	.byte $A0, $A8, $A4, $AC, $A2, $AA, $A6, $AE, $A1, $A9, $A5, $AD, $A3, $AB, $A7, $AF
	.byte $60, $68, $64, $6C, $62, $6A, $66, $6E, $61, $69, $65, $6D, $63, $6B, $67, $6F
	.byte $E0, $E8, $E4, $EC, $E2, $EA, $E6, $EE, $E1, $E9, $E5, $ED, $E3, $EB, $E7, $EF
	.byte $10, $18, $14, $1C, $12, $1A, $16, $1E, $11, $19, $15, $1D, $13, $1B, $17, $1F
	.byte $90, $98, $94, $9C, $92, $9A, $96, $9E, $91, $99, $95, $9D, $93, $9B, $97, $9F
	.byte $50, $58, $54, $5C, $52, $5A, $56, $5E, $51, $59, $55, $5D, $53, $5B, $57, $5F
	.byte $D0, $D8, $D4, $DC, $D2, $DA, $D6, $DE, $D1, $D9, $D5, $DD, $D3, $DB, $D7, $DF
	.byte $30, $38, $34, $3C, $32, $3A, $36, $3E, $31, $39, $35, $3D, $33, $3B, $37, $3F
	.byte $B0, $B8, $B4, $BC, $B2, $BA, $B6, $BE, $B1, $B9, $B5, $BD, $B3, $BB, $B7, $BF
	.byte $70, $78, $74, $7C, $72, $7A, $76, $7E, $71, $79, $75, $7D, $73, $7B, $77, $7F
	.byte $F0, $F8, $F4, $FC, $F2, $FA, $F6, $FE, $F1, $F9, $F5, $FD, $F3, $FB, $F7, $FF
	.byte $00, $04, $02, $06, $01, $05, $03, $07, $80, $84, $82, $86, $81, $85, $83, $87
	.byte $40, $44, $42, $46, $41, $45, $43, $47, $C0, $C4, $C2, $C6, $C1, $C5, $C3, $C7
	.byte $20, $24, $22, $26, $21, $25, $23, $27, $A0, $A4, $A2, $A6, $A1, $A5, $A3, $A7
	.byte $60, $64, $62, $66, $61, $65, $63, $67, $E0, $E4, $E2, $E6, $E1, $E5, $E3, $E7
	.byte $10, $14, $12, $16, $11, $15, $13, $17, $90, $94, $92, $96, $91, $95, $93, $97
	.byte $50, $54, $52, $56, $51, $55, $53, $57, $D0, $D4, $D2, $D6, $D1, $D5, $D3, $D7
	.byte $30, $34, $32, $36, $31, $35, $33, $37, $B0, $B4, $B2, $B6, $B1, $B5, $B3, $B7
	.byte $70, $74, $72, $76, $71, $75, $73, $77, $F0, $F4, $F2, $F6, $F1, $F5, $F3, $F7
	.byte $08, $0C, $0A, $0E, $09, $0D, $0B, $0F, $88, $8C, $8A, $8E, $89, $8D, $8B, $8F
	.byte $48, $4C, $4A, $4E, $49, $4D, $4B, $4F, $C8, $CC, $CA, $CE, $C9, $CD, $CB, $CF
	.byte $28, $2C, $2A, $2E, $29, $2D, $2B, $2F, $A8, $AC, $AA, $AE, $A9, $AD, $AB, $AF
	.byte $68, $6C, $6A, $6E, $69, $6D, $6B, $6F, $E8, $EC, $EA, $EE, $E9, $ED, $EB, $EF
	.byte $18, $1C, $1A, $1E, $19, $1D, $1B, $1F, $98, $9C, $9A, $9E, $99, $9D, $9B, $9F
	.byte $58, $5C, $5A, $5E, $59, $5D, $5B, $5F, $D8, $DC, $DA, $DE, $D9, $DD, $DB, $DF
	.byte $38, $3C, $3A, $3E, $39, $3D, $3B, $3F, $B8, $BC, $BA, $BE, $B9, $BD, $BB, $BF
	.byte $78, $7C, $7A, $7E, $79, $7D, $7B, $7F, $F8, $FC, $FA, $FE, $F9, $FD, $FB, $FF
	.byte $00, $02, $01, $03, $80, $82, $81, $83, $40, $42, $41, $43, $C0, $C2, $C1, $C3
	.byte $20, $22, $21, $23, $A0, $A2, $A1, $A3, $60, $62, $61, $63, $E0, $E2, $E1, $E3
	.byte $10, $12, $11, $13, $90, $92, $91, $93, $50, $52, $51, $53, $D0, $D2, $D1, $D3
	.byte $30, $32, $31, $33, $B0, $B2, $B1, $B3, $70, $72, $71, $73, $F0, $F2, $F1, $F3
	.byte $08, $0A, $09, $0B, $88, $8A, $89, $8B, $48, $4A, $49, $4B, $C8, $CA, $C9, $CB
	.byte $28, $2A, $29, $2B, $A8, $AA, $A9, $AB, $68, $6A, $69, $6B, $E8, $EA, $E9, $EB
	.byte $18, $1A, $19, $1B, $98, $9A, $99, $9B, $58, $5A, $59, $5B, $D8, $DA, $D9, $DB
	.byte $38, $3A, $39, $3B, $B8, $BA, $B9, $BB, $78, $7A, $79, $7B, $F8, $FA, $F9, $FB
	.byte $04, $06, $05, $07, $84, $86, $85, $87, $44, $46, $45, $47, $C4, $C6, $C5, $C7
	.byte $24, $26, $25, $27, $A4, $A6, $A5, $A7, $64, $66, $65, $67, $E4, $E6, $E5, $E7
	.byte $14, $16, $15, $17, $94, $96, $95, $97, $54, $56, $55, $57, $D4, $D6, $D5, $D7
	.byte $34, $36, $35, $37, $B4, $B6, $B5, $B7, $74, $76, $75, $77, $F4, $F6, $F5, $F7
	.byte $0C, $0E, $0D, $0F, $8C, $8E, $8D, $8F, $4C, $4E, $4D, $4F, $CC, $CE, $CD, $CF
	.byte $2C, $2E, $2D, $2F, $AC, $AE, $AD, $AF, $6C, $6E, $6D, $6F, $EC, $EE, $ED, $EF
	.byte $1C, $1E, $1D, $1F, $9C, $9E, $9D, $9F, $5C, $5E, $5D, $5F, $DC, $DE, $DD, $DF
	.byte $3C, $3E, $3D, $3F, $BC, $BE, $BD, $BF, $7C, $7E, $7D, $7F, $FC, $FE, $FD, $FF
	.byte $00, $01, $80, $81, $40, $41, $C0, $C1, $20, $21, $A0, $A1, $60, $61, $E0, $E1
	.byte $10, $11, $90, $91, $50, $51, $D0, $D1, $30, $31, $B0, $B1, $70, $71, $F0, $F1
	.byte $08, $09, $88, $89, $48, $49, $C8, $C9, $28, $29, $A8, $A9, $68, $69, $E8, $E9
	.byte $18, $19, $98, $99, $58, $59, $D8, $D9, $38, $39, $B8, $B9, $78, $79, $F8, $F9
	.byte $04, $05, $84, $85, $44, $45, $C4, $C5, $24, $25, $A4, $A5, $64, $65, $E4, $E5
	.byte $14, $15, $94, $95, $54, $55, $D4, $D5, $34, $35, $B4, $B5, $74, $75, $F4, $F5
	.byte $0C, $0D, $8C, $8D, $4C, $4D, $CC, $CD, $2C, $2D, $AC, $AD, $6C, $6D, $EC, $ED
	.byte $1C, $1D, $9C, $9D, $5C, $5D, $DC, $DD, $3C, $3D, $BC, $BD, $7C, $7D, $FC, $FD
	.byte $02, $03, $82, $83, $42, $43, $C2, $C3, $22, $23, $A2, $A3, $62, $63, $E2, $E3
	.byte $12, $13, $92, $93, $52, $53, $D2, $D3, $32, $33, $B2, $B3, $72, $73, $F2, $F3
	.byte $0A, $0B, $8A, $8B, $4A, $4B, $CA, $CB, $2A, $2B, $AA, $AB, $6A, $6B, $EA, $EB
	.byte $1A, $1B, $9A, $9B, $5A, $5B, $DA, $DB, $3A, $3B, $BA, $BB, $7A, $7B, $FA, $FB
	.byte $06, $07, $86, $87, $46, $47, $C6, $C7, $26, $27, $A6, $A7, $66, $67, $E6, $E7
	.byte $16, $17, $96, $97, $56, $57, $D6, $D7, $36, $37, $B6, $B7, $76, $77, $F6, $F7
	.byte $0E, $0F, $8E, $8F, $4E, $4F, $CE, $CF, $2E, $2F, $AE, $AF, $6E, $6F, $EE, $EF
	.byte $1E, $1F, $9E, $9F, $5E, $5F, $DE, $DF, $3E, $3F, $BE, $BF, $7E, $7F, $FE, $FF
