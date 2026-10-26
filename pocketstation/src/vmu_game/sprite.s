;----------------------
; | sprite.s
; | Description: sprite.c in LC86K assembly: row unpacking and mirroring into blit_a and blit_b, copying rows stored ready to draw, the row plotter and the sprite blitter that draws into XRAM.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/sprite.c, game.inc (ARG_BLOCK), tables.s (g_bit_rot), pocketstation/build/vmu_game/rrl_ram.inc (HW_BLIT_A, HWW_BLIT_B), pocketstation/src/vmu_rt/rt_lcd.s (rt_fb_row), pocketstation/build/vmu_game/tables.s (app_tb_map, app_tb_pages, app_sprites, g_spr_*), pocketstation/src/vmu_rt/rt_file.s (rt_rom8, rt_tb_seek, rt_fl_block)
; | Globals: blit_a, blit_b, XRAM rows 0-31, XBNK
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | SPRITE_MASKED / SPRITE_CLEAR / SPRITE_MIRROR
; | Description: game.h's blit flags: a mask follows the image, unpacked rows clear their box first, and rows are mirrored.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/game.h
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
SPRITE_MASKED	EQU	$01
SPRITE_CLEAR	EQU	$02
SPRITE_MIRROR	EQU	$04

;----------------------
; | PLOT_UNPACKED
; | Description: The bit g_plot_core's flags carry for rows already unpacked into blit_a (and blit_b for a mask).
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
PLOT_UNPACKED	EQU	$80

;----------------------
; | UCOPY1 / UCOPY2 / UCOPY3 / UCOPY4
; | Description: g_unpack_core's row loops for widths that are whole bytes: each row is 1-4 source bytes copied as they are, zeros above, the table pointer moving on R2 bytes.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: blit_a or blit_b through %dst
; | Params: %dst -- @R1 (blit_a) or VTRBF (blit_b); %inc -- the pointer to step, $01 or VRMAD1
; | Returns: N/A
;----------------------
%macro UCOPY1 %dst, %inc
.row%:
	MOV #$00, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$00, ACC
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	LD TRL
	ADD $02
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	DBNZ $03, .row%
	RET
%end

%macro UCOPY2 %dst, %inc
.row%:
	MOV #$00, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$01, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$00, ACC
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	LD TRL
	ADD $02
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	DBNZ $03, .row%
	RET
%end

%macro UCOPY3 %dst, %inc
.row%:
	MOV #$00, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$01, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$02, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$00, ACC
	ST %dst
	INC %inc
	LD TRL
	ADD $02
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	DBNZ $03, .row%
	RET
%end

%macro UCOPY4 %dst, %inc
.row%:
	MOV #$00, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$01, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$02, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$03, ACC
	LDC
	ST %dst
	INC %inc
	LD TRL
	ADD $02
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	DBNZ $03, .row%
	RET
%end

;----------------------
; | UNPACK_OB1 / UNPACK_OB2 / UNPACK_OB2F / UNPACK_OB3 / UNPACK_OB4
; | Description: g_unpack_core's row loops for rows of 1-4 output bytes at a bit offset s, R0's bits 5-7: each byte a 16-bit source window MULed by 2^(8-s); OB2F adds a third source byte.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: blit_a or blit_b through %dst
; | Params: %dst -- @R1 (blit_a) or VTRBF (blit_b); %inc -- the pointer to step, $01 or VRMAD1; %keep -- the last byte's mask, VRMAD1 or $01
; | Returns: N/A
;----------------------
%macro UNPACK_OB1 %dst, %inc, %keep
.row%:
	MOV #$00, ACC
	LDC
	ST C
	MOV #$01, ACC
	LDC
	BP $00, 7, .t4%
	BP $00, 6, .t2%
	BN $00, 5, .s0%
	MOV #$80, B
	BR .mul%
.t2%:
	BP $00, 5, .t3%
	MOV #$40, B
	BR .mul%
.t3%:
	MOV #$20, B
	BR .mul%
.t4%:
	BP $00, 6, .t6%
	BP $00, 5, .t5%
	MOV #$10, B
	BR .mul%
.t5%:
	MOV #$08, B
	BR .mul%
.t6%:
	BP $00, 5, .t7%
	MOV #$04, B
	BR .mul%
.t7%:
	MOV #$02, B
.mul%:
	MUL
	BR .put%
.s0%:
	LD C
.put%:
	AND %keep
	ST %dst
	INC %inc
	MOV #$00, ACC
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	LD $00
	ADD $02
	ST C
	AND #$07
	ADDC TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	LD C
	AND #$E0
	ST $00
	DBNZ $03, .row%
	RET
%end

%macro UNPACK_OB2 %dst, %inc, %keep
.row%:
	MOV #$00, ACC
	LDC
	ST C
	MOV #$01, ACC
	LDC
	BP $00, 7, .t4%
	BP $00, 6, .t2%
	BN $00, 5, .s0%
	MOV #$80, B
	BR .mul%
.t2%:
	BP $00, 5, .t3%
	MOV #$40, B
	BR .mul%
.t3%:
	MOV #$20, B
	BR .mul%
.t4%:
	BP $00, 6, .t6%
	BP $00, 5, .t5%
	MOV #$10, B
	BR .mul%
.t5%:
	MOV #$08, B
	BR .mul%
.t6%:
	BP $00, 5, .t7%
	MOV #$04, B
	BR .mul%
.t7%:
	MOV #$02, B
.mul%:
	MUL
	ST %dst
	INC %inc
	LD B
	BR .put%
.s0%:
	XCH C
	ST %dst
	INC %inc
	LD C
.put%:
	AND %keep
	ST %dst
	INC %inc
	MOV #$00, ACC
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	LD $00
	ADD $02
	ST C
	AND #$07
	ADDC TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	LD C
	AND #$E0
	ST $00
	DBNZ $03, .row%
	RET
%end

%macro UNPACK_OB2F %dst, %inc, %keep
.row%:
	MOV #$00, ACC
	LDC
	ST C
	MOV #$01, ACC
	LDC
	BP $00, 7, .t4%
	BP $00, 6, .t2%
	BN $00, 5, .s0%
	MOV #$80, B
	BR .mul%
.t2%:
	BP $00, 5, .t3%
	MOV #$40, B
	BR .mul%
.t3%:
	MOV #$20, B
	BR .mul%
.t4%:
	BP $00, 6, .t6%
	BP $00, 5, .t5%
	MOV #$10, B
	BR .mul%
.t5%:
	MOV #$08, B
	BR .mul%
.t6%:
	BP $00, 5, .t7%
	MOV #$04, B
	BR .mul%
.t7%:
	MOV #$02, B
.mul%:
	PUSH B
	MUL
	ST %dst
	INC %inc
	POP ACC
	XCH B
	PUSH ACC
	MOV #$02, ACC
	LDC
	ST C
	MOV #$00, ACC
	MUL
	POP ACC
	OR C
	BR .put%
.s0%:
	XCH C
	ST %dst
	INC %inc
	LD C
.put%:
	AND %keep
	ST %dst
	INC %inc
	MOV #$00, ACC
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	LD $00
	ADD $02
	ST C
	AND #$07
	ADDC TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	LD C
	AND #$E0
	ST $00
	DBNZ $03, .more%
	RET
.more%:
	JMPF .row%
%end

%macro UNPACK_OB3 %dst, %inc, %keep
.row%:
	MOV #$00, ACC
	LDC
	ST C
	MOV #$01, ACC
	LDC
	BP $00, 7, .t4%
	BP $00, 6, .t2%
	BN $00, 5, .s0%
	MOV #$80, B
	BR .mul%
.t2%:
	BP $00, 5, .t3%
	MOV #$40, B
	BR .mul%
.t3%:
	MOV #$20, B
	BR .mul%
.t4%:
	BP $00, 6, .t6%
	BP $00, 5, .t5%
	MOV #$10, B
	BR .mul%
.t5%:
	MOV #$08, B
	BR .mul%
.t6%:
	BP $00, 5, .t7%
	MOV #$04, B
	BR .mul%
.t7%:
	MOV #$02, B
.mul%:
	PUSH B
	MUL
	ST %dst
	INC %inc
	POP ACC
	XCH B
	PUSH ACC
	MOV #$02, ACC
	LDC
	ST C
	MOV #$03, ACC
	LDC
	MUL
	XCH C
	POP B
	OR B
	ST %dst
	INC %inc
	LD C
	BR .put%
.s0%:
	XCH C
	ST %dst
	INC %inc
	LD C
	ST %dst
	INC %inc
	MOV #$02, ACC
	LDC
.put%:
	AND %keep
	ST %dst
	INC %inc
	MOV #$00, ACC
	ST %dst
	INC %inc
	LD $00
	ADD $02
	ST C
	AND #$07
	ADDC TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	LD C
	AND #$E0
	ST $00
	DBNZ $03, .more%
	RET
.more%:
	JMPF .row%
%end

%macro UNPACK_OB4 %dst, %inc, %keep
.row%:
	MOV #$00, ACC
	LDC
	ST C
	MOV #$01, ACC
	LDC
	BP $00, 7, .t4%
	BP $00, 6, .t2%
	BN $00, 5, .s0%
	MOV #$80, B
	BR .mul%
.t2%:
	BP $00, 5, .t3%
	MOV #$40, B
	BR .mul%
.t3%:
	MOV #$20, B
	BR .mul%
.t4%:
	BP $00, 6, .t6%
	BP $00, 5, .t5%
	MOV #$10, B
	BR .mul%
.t5%:
	MOV #$08, B
	BR .mul%
.t6%:
	BP $00, 5, .t7%
	MOV #$04, B
	BR .mul%
.t7%:
	MOV #$02, B
.mul%:
	PUSH B
	MUL
	ST %dst
	INC %inc
	MOV #$01, ACC
	LDC
	ST C
	MOV #$02, ACC
	LDC
	POP B
	PUSH B
	MUL
	ST %dst
	INC %inc
	MOV #$02, ACC
	LDC
	ST C
	MOV #$03, ACC
	LDC
	POP B
	PUSH B
	MUL
	ST %dst
	INC %inc
	MOV #$03, ACC
	LDC
	ST C
	MOV #$04, ACC
	LDC
	POP B
	MUL
	BR .put%
.s0%:
	XCH C
	ST %dst
	INC %inc
	LD C
	ST %dst
	INC %inc
	MOV #$02, ACC
	LDC
	ST %dst
	INC %inc
	MOV #$03, ACC
	LDC
.put%:
	AND %keep
	ST %dst
	INC %inc
	LD $00
	ADD $02
	ST C
	AND #$07
	ADDC TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	LD C
	AND #$E0
	ST $00
	DBNZ $03, .more%
	RET
.more%:
	JMPF .row%
%end

;----------------------
; | MLOAD / MINIT / MSTEP / MLAST / MNEXT
; | Description: The MUL mirror's steps: push a row's four bytes and start the window, then write bytes of the reversed row shifted right through MUL, the last one, and step on.
; | Author: suinevere
; | Dependencies: g_bit_rot through TRH:TRL
; | Globals: blit_a or blit_b through %dst
; | Params: %dst -- @R1 or VTRBF; %inc -- $01 or VRMAD1, already on the next row; %top -- the loop's label
; | Returns: N/A
;----------------------
%macro MLOAD %dst, %inc
	LD %dst
	PUSH ACC
	INC %inc
	LD %dst
	PUSH ACC
	INC %inc
	LD %dst
	PUSH ACC
	INC %inc
	LD %dst
	PUSH ACC
	LD %inc
	SUB #$03
	ST %inc
%end

%macro MINIT
	POP ACC
	LDC
	ST C
%end

%macro MSTEP %dst, %inc
	LD $02
	ST B
	POP ACC
	LDC
	ST $00
	MUL
	ST %dst
	INC %inc
	LD $00
	ST C
%end

%macro MLAST %dst, %inc
	LD $02
	ST B
	MOV #$00, ACC
	MUL
	ST %dst
	INC %inc
%end

%macro MNEXT %top
	DBNZ $03, %top
	RET
%end

;----------------------
; | PF_ORB / PE_ORB / PL_ORB
; | Description: One unpacked image byte from @R0 ORed into XRAM through @R2. PF ORs both parts and steps R2, PE only the spill (left edge), PL only the shifted part (right edge).
; | Author: suinevere
; | Dependencies: g_bit_rot page r through TRH:TRL
; | Globals: XRAM
; | Params: R1 -- 0xFF >> r; B -- its complement
; | Returns: N/A
;----------------------
%macro PF_ORB
	LD @R0
	LDC
	AND $01
	OR @R2
	ST @R2
	INC $02
	LD @R0
	LDC
	AND B
	OR @R2
	ST @R2
	INC $00
%end

%macro PE_ORB
	LD @R0
	LDC
	AND B
	OR @R2
	ST @R2
	INC $00
%end

%macro PL_ORB
	LD @R0
	LDC
	AND $01
	OR @R2
	ST @R2
%end

;----------------------
; | PF_MKB / PE_MKB / PL_MKB
; | Description: As PF_ORB and its edges, with the mask byte at VTRBF cleared out of XRAM before the image byte at @R0 is ORed in.
; | Author: suinevere
; | Dependencies: g_bit_rot page r through TRH:TRL
; | Globals: XRAM
; | Params: R1 -- 0xFF >> r; B -- its complement; VRMAD -- the mask byte, VSEL bit 4 clear
; | Returns: N/A
;----------------------
%macro PF_MKB
	LD VTRBF
	LDC
	AND $01
	XOR #$FF
	AND @R2
	ST @R2
	LD @R0
	LDC
	AND $01
	OR @R2
	ST @R2
	INC $02
	LD VTRBF
	LDC
	AND B
	XOR #$FF
	AND @R2
	ST @R2
	LD @R0
	LDC
	AND B
	OR @R2
	ST @R2
	INC $00
	INC VRMAD1
%end

%macro PE_MKB
	LD VTRBF
	LDC
	AND B
	XOR #$FF
	AND @R2
	ST @R2
	LD @R0
	LDC
	AND B
	OR @R2
	ST @R2
	INC $00
	INC VRMAD1
%end

%macro PL_MKB
	LD VTRBF
	LDC
	AND $01
	XOR #$FF
	AND @R2
	ST @R2
	LD @R0
	LDC
	AND $01
	OR @R2
	ST @R2
%end

;----------------------
; | PF_ORV / PE_ORV / PL_ORV / PF_ANV / PE_ANV / PL_ANV
; | Description: As PF_ORB and its edges for a table row staged in work RAM at VTRBF: OR it into XRAM, or clear it out of XRAM.
; | Author: suinevere
; | Dependencies: g_bit_rot page r through TRH:TRL
; | Globals: XRAM
; | Params: R1 -- 0xFF >> r; B -- its complement; VRMAD -- the staged byte, VSEL bit 4 clear
; | Returns: N/A
;----------------------
%macro PF_ORV
	LD VTRBF
	LDC
	AND $01
	OR @R2
	ST @R2
	INC $02
	LD VTRBF
	LDC
	AND B
	OR @R2
	ST @R2
	INC VRMAD1
%end

%macro PE_ORV
	LD VTRBF
	LDC
	AND B
	OR @R2
	ST @R2
	INC VRMAD1
%end

%macro PL_ORV
	LD VTRBF
	LDC
	AND $01
	OR @R2
	ST @R2
%end

%macro PF_ANV
	LD VTRBF
	LDC
	AND $01
	XOR #$FF
	AND @R2
	ST @R2
	INC $02
	LD VTRBF
	LDC
	AND B
	XOR #$FF
	AND @R2
	ST @R2
	INC VRMAD1
%end

%macro PE_ANV
	LD VTRBF
	LDC
	AND B
	XOR #$FF
	AND @R2
	ST @R2
	INC VRMAD1
%end

%macro PL_ANV
	LD VTRBF
	LDC
	AND $01
	XOR #$FF
	AND @R2
	ST @R2
%end

;----------------------
; | PH_OR0 / PH_MK0
; | Description: One table image byte at a whole-byte x, fetched from page VRMAD1 at R0 and bit-reversed through g_bit_rot page 0, ORed into XRAM. PH_MK0 first clears the mask byte at B:R1.
; | Author: suinevere
; | Dependencies: g_bit_rot, app_tb_pages, TRL holding 0
; | Globals: XRAM
; | Params: N/A
; | Returns: N/A
;----------------------
%macro PH_OR0
	LD VRMAD1
	ST TRH
	LD $00
	LDC
	MOV #>g_bit_rot, TRH
	LDC
	OR @R2
	ST @R2
	INC $02
	INC $00
%end

%macro PH_MK0
	LD B
	ST TRH
	LD $01
	LDC
	MOV #>g_bit_rot, TRH
	LDC
	XOR #$FF
	AND @R2
	ST @R2
	LD VRMAD1
	ST TRH
	LD $00
	LDC
	MOV #>g_bit_rot, TRH
	LDC
	OR @R2
	ST @R2
	INC $02
	INC $00
	INC $01
%end


;----------------------
; | g_unpack_rows
; | Description: unpack_rows: unpacks height rows of width bits, packed LSB first from table offset src, into blit_a or blit_b one word a row, mirrored when asked, from the GAME's pages.
; | Author: suinevere
; | Dependencies: g_tb_extent, g_tb_find, g_unpack_core, game.inc (ARG_BLOCK)
; | Globals: blit_a, blit_b
; | Params: TRH:TRL -- src, a card address 0x0200xxxx's low 16 bits; ACC -- width, 0-32; B -- height, 1-32; C -- mirror, nonzero to mirror; ARG_BLOCK+0 -- to_mask, nonzero for blit_b
; | Returns: ARG_BLOCK+1..4 -- the card address after the last word read. Destroys ACC, B, C, TRL, TRH, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_unpack_rows:
	ST $02
	LD C
	BZ g_unpack_rows_flat
	MOV #SPRITE_MIRROR, C
g_unpack_rows_flat:
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	BZ g_unpack_rows_go
	SET1 C, 0
g_unpack_rows_go:
	PUSH TRH
	LD $02
	CALLF g_tb_extent
	PUSH B
	ST B
	CALLF g_tb_find
	POP B
	POP ACC
	SUB TRH
	PUSH ACC
	LD $02
	CALLF g_unpack_core
	POP ACC
	ADD TRH
	ST TRH
g_unpack_rows_out:
	SET1 VSEL, 4
	MOV #>(ARG_BLOCK+1), VRMAD2
	MOV #<(ARG_BLOCK+1), VRMAD1
	LD TRL
	ST VTRBF
	LD TRH
	ST VTRBF
	MOV #$00, VTRBF
	MOV #$02, VTRBF
	CLR1 VSEL, 4
	RET

;----------------------
; | g_unpack_core
; | Description: Unpacks height rows of width bits from the table at TRH:TRL into blit_a, or blit_b for flag bit 0. Flag bit 2 mirrors them; bits above the width are zero.
; | Author: suinevere
; | Dependencies: g_unpack_c1..c4, g_unpack_d1..d4, g_unpack_a1..a4, g_unpack_a2f, g_unpack_b1..b4, g_unpack_b2f, g_mirror_pass, app_tb_pages (256-aligned)
; | Globals: blit_a, blit_b
; | Params: TRH:TRL -- src's address in app_tb_pages (g_tb_find), 4-aligned; ACC -- width, 0-32; B -- height, 1-32; C -- bit 0 to_mask, bit 2 mirror
; | Returns: TRH:TRL -- the address after the last word read, the rows' bits rounded up to whole words. Destroys ACC, B, C, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear; 12 bytes of stack.
;----------------------
g_unpack_core:
	ST $02
	PUSH B
	PUSH C
	LD B
	ST $03
	MOV #$00, $00
	CLR1 VSEL, 4
	LD $02
	BNZ g_far_4
	JMPF g_unpack_core_w0
g_far_4:
	AND #$07
	BNZ g_far_1
	JMPF g_unpack_core_whole
g_far_1:
	ST C
	MOV #$00, ACC
g_unpack_core_keep:
	SET1 PSW, 7
	ROLC
	DBNZ C, g_unpack_core_keep
	ST B
	LD $02
	ADD #$07
	ROR
	ROR
	ROR
	AND #$1F
	ST C
	CLR1 PSW, 2
	BNE #$02, g_unpack_core_rot
	LD $02
	OR #$08
	ST $01
	XOR #$FF
	INC ACC
	AND $01
	ST $01
	LD $02
	SUB $01
	SUB #$09
	CLR1 PSW, 2
	BP PSW, 7, g_unpack_core_rot
	SET1 PSW, 2
g_unpack_core_rot:
	LD $02
	ROR
	ROR
	ROR
	ST $02
	POP ACC
	PUSH ACC
	BN ACC, 0, g_far_0
	JMPF g_unpack_core_fb
g_far_0:
	LD B
	ST VRMAD1
	MOV #HW_BLIT_A, $01
	LD C
	BE #$01, g_unpack_core_a1
	BE #$02, g_unpack_core_a2
	BE #$03, g_unpack_core_a3
	CALLF g_unpack_a4
	JMPF g_unpack_core_done
g_unpack_core_a1:
	CALLF g_unpack_a1
	JMPF g_unpack_core_done
g_unpack_core_a2:
	BN PSW, 2, g_unpack_core_a2n
	CALLF g_unpack_a2f
	JMPF g_unpack_core_done
g_unpack_core_a2n:
	CALLF g_unpack_a2
	JMPF g_unpack_core_done
g_unpack_core_a3:
	CALLF g_unpack_a3
	JMPF g_unpack_core_done
g_unpack_core_w0:
	MOV #$00, B
	MOV #$01, C
	CLR1 PSW, 2
	BR g_unpack_core_rot
g_unpack_core_whole:
	LD $02
	ROR
	ROR
	ROR
	ST $02
	ST C
	POP ACC
	PUSH ACC
	BP ACC, 0, g_unpack_core_wb
	MOV #HW_BLIT_A, $01
	LD C
	BE #$01, g_unpack_core_c1
	BE #$02, g_unpack_core_c2
	BE #$03, g_unpack_core_c3
	CALLF g_unpack_c4
	BR g_unpack_core_done
g_unpack_core_c1:
	CALLF g_unpack_c1
	BR g_unpack_core_done
g_unpack_core_c2:
	CALLF g_unpack_c2
	BR g_unpack_core_done
g_unpack_core_c3:
	CALLF g_unpack_c3
	BR g_unpack_core_done
g_unpack_core_wb:
	MOV #HWW_BLIT_B/256 + 256 * ((HWW_BLIT_B + $7F)/256 - HWW_BLIT_B/256), VRMAD2
	MOV #<HWW_BLIT_B, VRMAD1
	LD C
	BE #$01, g_unpack_core_d1
	BE #$02, g_unpack_core_d2
	BE #$03, g_unpack_core_d3
	CALLF g_unpack_d4
	BR g_unpack_core_done
g_unpack_core_d1:
	CALLF g_unpack_d1
	BR g_unpack_core_done
g_unpack_core_d2:
	CALLF g_unpack_d2
	BR g_unpack_core_done
g_unpack_core_d3:
	CALLF g_unpack_d3
	BR g_unpack_core_done
g_unpack_core_fb:
	LD B
	ST $01
	MOV #HWW_BLIT_B/256 + 256 * ((HWW_BLIT_B + $7F)/256 - HWW_BLIT_B/256), VRMAD2
	MOV #<HWW_BLIT_B, VRMAD1
	LD C
	BE #$01, g_unpack_core_b1
	BE #$02, g_unpack_core_b2
	BE #$03, g_unpack_core_b3
	CALLF g_unpack_b4
	BR g_unpack_core_done
g_unpack_core_b1:
	CALLF g_unpack_b1
	BR g_unpack_core_done
g_unpack_core_b2:
	BN PSW, 2, g_unpack_core_b2n
	CALLF g_unpack_b2f
	BR g_unpack_core_done
g_unpack_core_b2n:
	CALLF g_unpack_b2
	BR g_unpack_core_done
g_unpack_core_b3:
	CALLF g_unpack_b3
g_unpack_core_done:
	LD $00
	ADD #$FF
	LD TRL
	ADDC #$03
	AND #$FC
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	POP $01
	POP $03
	BN $01, 2, g_unpack_core_end
	LD $02
	ROL
	ROL
	ROL
	ST $02
	BZ g_unpack_core_end
	PUSH TRL
	PUSH TRH
	CALLF g_mirror_pass
	POP TRH
	POP TRL
g_unpack_core_end:
	RET

;----------------------
; | g_unpack_c1 / g_unpack_c2 / g_unpack_c3 / g_unpack_c4 / g_unpack_a1 / g_unpack_a2 / g_unpack_a2f / g_unpack_a3 / g_unpack_a4
; | Description: g_unpack_core's row loops into blit_a through @R1, the last byte masked by VRMAD1: whole-byte copies, then rows at a bit offset.
; | Author: suinevere
; | Dependencies: UCOPY1..UCOPY4, UNPACK_OB1..OB4, UNPACK_OB2F
; | Globals: blit_a
; | Params: R0 -- 0; R1 -- blit_a; R2 -- width rotated right 3; R3 -- rows; VRMAD1 -- the last byte's mask; TRH:TRL -- the table
; | Returns: N/A. Destroys ACC, B, C, R0, R1, R3, TRH, TRL, PSW.
;----------------------
g_unpack_c1:
	UCOPY1 @R1, $01
g_unpack_c2:
	UCOPY2 @R1, $01
g_unpack_c3:
	UCOPY3 @R1, $01
g_unpack_c4:
	UCOPY4 @R1, $01
g_unpack_a1:
	UNPACK_OB1 @R1, $01, VRMAD1
g_unpack_a2:
	UNPACK_OB2 @R1, $01, VRMAD1
g_unpack_a2f:
	UNPACK_OB2F @R1, $01, VRMAD1
g_unpack_a3:
	UNPACK_OB3 @R1, $01, VRMAD1
g_unpack_a4:
	UNPACK_OB4 @R1, $01, VRMAD1

;----------------------
; | g_unpack_d1 / g_unpack_d2 / g_unpack_d3 / g_unpack_d4 / g_unpack_b1 / g_unpack_b2 / g_unpack_b2f / g_unpack_b3 / g_unpack_b4
; | Description: g_unpack_core's row loops into blit_b through VTRBF, the last byte masked by R1: whole-byte copies, then rows at a bit offset.
; | Author: suinevere
; | Dependencies: UCOPY1..UCOPY4, UNPACK_OB1..OB4, UNPACK_OB2F
; | Globals: blit_b
; | Params: R0 -- 0; R1 -- the last byte's mask; R2 -- width rotated right 3; R3 -- rows; VRMAD -- blit_b, VSEL bit 4 clear; TRH:TRL -- the table
; | Returns: N/A. Destroys ACC, B, C, R0, R3, TRH, TRL, VRMAD1, PSW.
;----------------------
g_unpack_d1:
	UCOPY1 VTRBF, VRMAD1
g_unpack_d2:
	UCOPY2 VTRBF, VRMAD1
g_unpack_d3:
	UCOPY3 VTRBF, VRMAD1
g_unpack_d4:
	UCOPY4 VTRBF, VRMAD1
g_unpack_b1:
	UNPACK_OB1 VTRBF, VRMAD1, $01
g_unpack_b2:
	UNPACK_OB2 VTRBF, VRMAD1, $01
g_unpack_b2f:
	UNPACK_OB2F VTRBF, VRMAD1, $01
g_unpack_b3:
	UNPACK_OB3 VTRBF, VRMAD1, $01
g_unpack_b4:
	UNPACK_OB4 VTRBF, VRMAD1, $01

;----------------------
; | MROT1 / MROT2 / MROT3
; | Description: The lookup mirror's row loops for widths 17-24, 9-16 and 1-8. Each byte of the reversed row is a g_bit_rot page r lookup kept by B, ORed with the next byte's kept by C, in place.
; | Author: suinevere
; | Dependencies: g_bit_rot page r through TRH:TRL
; | Globals: blit_a, blit_b or ARG_BLOCK through %dst
; | Params: %dst -- @R1 or VTRBF; %inc -- $01 or VRMAD1, the row pointer; %t -- a third byte's temporary, VRMAD1 or $01
; | Returns: N/A
;----------------------
%macro MROT1 %dst, %inc, %t
.row%:
	LD %dst
	ST $00
	INC %inc
	LD %dst
	ST $02
	INC %inc
	LD %dst
	ST %t
	LD %inc
	SUB #$02
	ST %inc
	LD $02
	LDC
	AND C
	ST %dst
	LD %t
	LDC
	AND B
	OR %dst
	ST %dst
	INC %inc
	LD $00
	LDC
	AND C
	ST %dst
	LD $02
	LDC
	AND B
	OR %dst
	ST %dst
	INC %inc
	LD $00
	LDC
	AND B
	ST %dst
	INC %inc
	INC %inc
	DBNZ $03, .row%
	RET
%end

%macro MROT2 %dst, %inc
.row%:
	LD %dst
	ST $00
	INC %inc
	LD %dst
	ST $02
	DEC %inc
	LD $00
	LDC
	AND C
	ST %dst
	LD $02
	LDC
	AND B
	OR %dst
	ST %dst
	INC %inc
	LD $00
	LDC
	AND B
	ST %dst
	INC %inc
	INC %inc
	INC %inc
	DBNZ $03, .row%
	RET
%end

%macro MROT3 %dst, %inc
.row%:
	LD %dst
	LDC
	AND B
	ST %dst
	INC %inc
	INC %inc
	INC %inc
	INC %inc
	DBNZ $03, .row%
	RET
%end

;----------------------
; | g_mirror_row
; | Description: mirror_row: reverses the low width bits of v, the word mirrored whole and shifted right by 32 - width.
; | Author: suinevere
; | Dependencies: g_mirror_shift, g_mirror_r1..r3 (blit_b's loops on ARG_BLOCK), g_mirror_setup, g_mirror_b0_body, g_mirror_b32_body, game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ARG_BLOCK+0..3 -- v; ACC -- width
; | Returns: ARG_BLOCK+4..7 -- the mirrored row. Destroys ACC, B, C, TRL, TRH, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_mirror_row:
	ST $02
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	BNZ g_mirror_row_w
	JMPF g_mirror_row_zero
g_mirror_row_w:
	SUB #$21
	BP PSW, 7, g_mirror_row_n
	JMPF g_mirror_row_zero
g_mirror_row_n:
	MOV #$01, $03
	MOV #$20, ACC
	SUB $02
	ROR
	ROR
	ROR
	AND #$1F
	BZ g_mirror_row_wide
	ST $00
	MOV #<(ARG_BLOCK+0), VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+4), VRMAD1
	ST VTRBF
	MOV #<(ARG_BLOCK+1), VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+5), VRMAD1
	ST VTRBF
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+6), VRMAD1
	ST VTRBF
	MOV #<(ARG_BLOCK+3), VRMAD1
	LD VTRBF
	MOV #<(ARG_BLOCK+7), VRMAD1
	ST VTRBF
	MOV #<(ARG_BLOCK+8), ACC
	SUB $00
	ST VRMAD1
	SET1 VSEL, 4
	LD $00
	ST C
	MOV #$00, ACC
g_mirror_row_fill:
	ST VTRBF
	DBNZ C, g_mirror_row_fill
	CLR1 VSEL, 4
	CALLF g_mirror_shift
	MOV #<(ARG_BLOCK+4), VRMAD1
	LD $00
	BE #$01, g_mirror_row_q1
	BE #$02, g_mirror_row_q2
	JMPF g_mirror_r3
g_mirror_row_q1:
	JMPF g_mirror_r1
g_mirror_row_q2:
	JMPF g_mirror_r2
g_mirror_row_wide:
	LD VTRBF
	PUSH ACC
	INC VRMAD1
	LD VTRBF
	PUSH ACC
	INC VRMAD1
	LD VTRBF
	PUSH ACC
	INC VRMAD1
	LD VTRBF
	PUSH ACC
	MOV #$00, ACC
	SET1 VSEL, 4
	MOV #<(ARG_BLOCK+4), VRMAD1
	ST VTRBF
	ST VTRBF
	ST VTRBF
	ST VTRBF
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+4), VRMAD1
	CALLF g_mirror_setup
	BP PSW, 7, g_mirror_row_w32
	JMPF g_mirror_b0_body
g_mirror_row_w32:
	JMPF g_mirror_b32_body
g_mirror_row_zero:
	SET1 VSEL, 4
	MOV #<(ARG_BLOCK+4), VRMAD1
	MOV #$00, ACC
	ST VTRBF
	ST VTRBF
	ST VTRBF
	ST VTRBF
	CLR1 VSEL, 4
	RET

;----------------------
; | g_mirror_shift
; | Description: The lookup mirror's bit shift r = (32 - width) & 7: g_bit_rot's page r in TRH:TRL, B keeping a lookup's part shifted right r, C the spill.
; | Author: suinevere
; | Dependencies: g_bit_rot (256-aligned)
; | Globals: N/A
; | Params: R2 -- width
; | Returns: TRH:TRL -- g_bit_rot + 256 r; B -- 0xFF >> r; C -- its complement. Destroys ACC, PSW.
;----------------------
g_mirror_shift:
	MOV #$20, ACC
	SUB $02
	AND #$07
	ADD #>g_bit_rot
	ST TRH
	MOV #(g_bit_rot - g_bit_rot / 256 * 256) * 256, TRL
	MOV #$01, ACC
	LDC
	ST B
	ADD B
	DEC ACC
	ST B
	XOR #$FF
	ST C
	RET

;----------------------
; | g_mirror_setup
; | Description: The MUL mirror's shift for widths 25-32, whose r = 32 - w is under a byte: R2 gets 2^(8-r), 1 for width 32.
; | Author: suinevere
; | Dependencies: g_bit_rot
; | Globals: N/A
; | Params: R2 -- width, 25-32
; | Returns: R2 -- the multiplier; CY set for width 32; TRH:TRL -- g_bit_rot. Destroys ACC, B, R0, PSW.
;----------------------
g_mirror_setup:
	MOV #$20, ACC
	SUB $02
	ST B
	MOV #$08, ACC
	SUB B
	ST $00
	MOV #$01, ACC
g_mirror_setup_m:
	ROL
	DBNZ $00, g_mirror_setup_m
	ST $02
	MOV #<g_bit_rot, TRL
	MOV #>g_bit_rot, TRH
	LD B
	BZ g_mirror_setup_w32
	CLR1 PSW, 7
	RET
g_mirror_setup_w32:
	SET1 PSW, 7
	RET

;----------------------
; | g_mirror_pass
; | Description: Mirrors the height rows g_unpack_core just wrote to blit_a, or blit_b for flag bit 0. Up to 24 bits it uses g_bit_rot lookups, from 25 MUL.
; | Author: suinevere
; | Dependencies: g_mirror_shift, g_mirror_q1..q3, g_mirror_r1..r3, g_mirror_setup, g_mirror_a0, g_mirror_a32, g_mirror_b0, g_mirror_b32
; | Globals: blit_a, blit_b
; | Params: R1 -- flags (bit 0 blit_b); R2 -- width, 1-32; R3 -- height
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R0-R3, VRMAD, PSW.
;----------------------
g_mirror_pass:
	MOV #$20, ACC
	SUB $02
	ROR
	ROR
	ROR
	AND #$1F
	BZ g_mirror_pass_wide
	ST $00
	CALLF g_mirror_shift
	BP $01, 0, g_mirror_pass_b
	MOV #HW_BLIT_A, $01
	LD $00
	BE #$01, g_mirror_pass_a1
	BE #$02, g_mirror_pass_a2
	JMPF g_mirror_q3
g_mirror_pass_a1:
	JMPF g_mirror_q1
g_mirror_pass_a2:
	JMPF g_mirror_q2
g_mirror_pass_b:
	MOV #HWW_BLIT_B/256, VRMAD2
	MOV #<HWW_BLIT_B, VRMAD1
	LD $00
	BE #$01, g_mirror_pass_b1
	BE #$02, g_mirror_pass_b2
	JMPF g_mirror_r3
g_mirror_pass_b1:
	JMPF g_mirror_r1
g_mirror_pass_b2:
	JMPF g_mirror_r2
g_mirror_pass_wide:
	CALLF g_mirror_setup
	BP PSW, 7, g_mirror_pass_w32
	BP $01, 0, g_mirror_pass_wb
	MOV #HW_BLIT_A, $01
	JMPF g_mirror_a0
g_mirror_pass_wb:
	MOV #HWW_BLIT_B/256, VRMAD2
	MOV #<HWW_BLIT_B, VRMAD1
	JMPF g_mirror_b0
g_mirror_pass_w32:
	BP $01, 0, g_mirror_pass_b32
	MOV #HW_BLIT_A, $01
	JMPF g_mirror_a32
g_mirror_pass_b32:
	MOV #HWW_BLIT_B/256, VRMAD2
	MOV #<HWW_BLIT_B, VRMAD1
	JMPF g_mirror_b32

;----------------------
; | g_mirror_q1 / g_mirror_q2 / g_mirror_q3 / g_mirror_r1 / g_mirror_r2 / g_mirror_r3
; | Description: The lookup mirror's row loops for widths 17-24, 9-16 and 1-8, in place on blit_a through @R1 (q) or on work RAM rows through VTRBF (r).
; | Author: suinevere
; | Dependencies: MROT1, MROT2, MROT3
; | Globals: blit_a, blit_b, ARG_BLOCK+4..7
; | Params: R1 or VRMAD (VSEL bit 4 clear) -- the first row; R3 -- rows; B, C, TRH:TRL -- from g_mirror_shift
; | Returns: N/A. Destroys ACC, R0-R3, VRMAD1, PSW.
;----------------------
g_mirror_q1:
	MROT1 @R1, $01, VRMAD1
g_mirror_q2:
	MROT2 @R1, $01
g_mirror_q3:
	MROT3 @R1, $01
g_mirror_r1:
	MROT1 VTRBF, VRMAD1, $01
g_mirror_r2:
	MROT2 VTRBF, VRMAD1
g_mirror_r3:
	MROT3 VTRBF, VRMAD1

;----------------------
; | g_mirror_a0 / g_mirror_a32
; | Description: The MUL mirror pass over blit_a rows through @R1 for widths 25-31 and for width 32.
; | Author: suinevere
; | Dependencies: MLOAD, MINIT, MSTEP, MLAST, MNEXT, g_bit_rot through TRH:TRL
; | Globals: blit_a
; | Params: R1 -- the first row; R2 -- the multiplier; R3 -- rows
; | Returns: N/A. Destroys ACC, B, C, R0, R1, R3, PSW.
;----------------------
g_mirror_a0:
	MLOAD @R1, $01
	MINIT
	MSTEP @R1, $01
	MSTEP @R1, $01
	MSTEP @R1, $01
	MLAST @R1, $01
	MNEXT g_mirror_a0
g_mirror_a32:
	MLOAD @R1, $01
	MINIT
	LD C
	ST @R1
	INC $01
	MINIT
	LD C
	ST @R1
	INC $01
	MINIT
	LD C
	ST @R1
	INC $01
	MINIT
	LD C
	ST @R1
	INC $01
	MNEXT g_mirror_a32

;----------------------
; | g_mirror_b0 / g_mirror_b32
; | Description: The MUL mirror pass over work RAM rows through VTRBF for widths 25-31 and for width 32; each _body entry starts after the row's four bytes are pushed, for g_mirror_row.
; | Author: suinevere
; | Dependencies: MLOAD, MINIT, MSTEP, MLAST, MNEXT, g_bit_rot through TRH:TRL
; | Globals: blit_b, ARG_BLOCK
; | Params: VRMAD -- the first row, VSEL bit 4 clear; R2 -- the multiplier; R3 -- rows
; | Returns: N/A. Destroys ACC, B, C, R0, R3, VRMAD1, PSW.
;----------------------
g_mirror_b0:
	MLOAD VTRBF, VRMAD1
g_mirror_b0_body:
	MINIT
	MSTEP VTRBF, VRMAD1
	MSTEP VTRBF, VRMAD1
	MSTEP VTRBF, VRMAD1
	MLAST VTRBF, VRMAD1
	MNEXT g_mirror_b0
g_mirror_b32:
	MLOAD VTRBF, VRMAD1
g_mirror_b32_body:
	MINIT
	LD C
	ST VTRBF
	INC VRMAD1
	MINIT
	LD C
	ST VTRBF
	INC VRMAD1
	MINIT
	LD C
	ST VTRBF
	INC VRMAD1
	MINIT
	LD C
	ST VTRBF
	INC VRMAD1
	MNEXT g_mirror_b32

;----------------------
; | g_plot_rows
; | Description: plot_rows: clips to rows 0-31, turns rows into the VMU's bit order, shifts them by x and merges them into XRAM by OR, mask-clear then OR, or box-clear then OR.
; | Author: suinevere
; | Dependencies: g_tb_mark, g_plot_core, game.inc (ARG_BLOCK)
; | Globals: XRAM rows, XBNK, blit_a, blit_b
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ACC -- width; B -- height; ARG_BLOCK+2..5 -- bitmap, a card address whose low 16 bits are the table offset; C -- flags; ARG_BLOCK+6 -- unpacked, nonzero when the rows are in blit_a and blit_b
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R0-R3, VRMAD, FPR, PSW; leaves XBNK at the last row's bank and VSEL bit 4 clear.
;----------------------
g_plot_rows:
	ST $01
	LD C
	AND #$07
	ST C
	CLR1 VSEL, 4
	MOV #>(ARG_BLOCK+6), VRMAD2
	MOV #<(ARG_BLOCK+6), VRMAD1
	LD VTRBF
	BZ g_plot_rows_mark
	SET1 C, 7
	BR g_plot_rows_go
g_plot_rows_mark:
	CALLF g_tb_mark
g_plot_rows_go:
	LD $01
	JMPF g_plot_core

;----------------------
; | g_plot_plan / g_plot_pos
; | Description: Indexed (q + 4) * 8 + 4 (r not 0) + nb - 1, for x's shift of q bytes and r bits and nb source bytes. Plan: bit 7 left edge, bit 6 right edge, bits 0-2 full bytes, 0 for nothing; pos: k0, first screen byte in bits 2-3.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
g_plot_plan:
	.byte $00, $00, $00, $00, $00, $00, $00, $80, $00, $00, $00, $01, $00, $00, $80, $81
	.byte $00, $00, $01, $02, $00, $80, $81, $82, $00, $01, $02, $03, $80, $81, $82, $83
	.byte $01, $02, $03, $04, $01, $02, $03, $43, $01, $02, $03, $03, $01, $02, $42, $42
	.byte $01, $02, $02, $02, $01, $41, $41, $41, $01, $01, $01, $01, $40, $40, $40, $40
g_plot_pos:
	.byte $00, $00, $00, $00, $00, $00, $00, $03, $00, $00, $00, $03, $00, $00, $02, $02
	.byte $00, $00, $02, $02, $00, $01, $01, $01, $00, $01, $01, $01, $00, $00, $00, $00
	.byte $00, $00, $00, $00, $00, $00, $00, $00, $04, $04, $04, $04, $04, $04, $04, $04
	.byte $08, $08, $08, $08, $08, $08, $08, $08, $0C, $0C, $0C, $0C, $0C, $0C, $0C, $0C

;----------------------
; | g_plot_core
; | Description: plot_rows on registers: clips the rows, looks up which source and screen bytes the shift by x keeps, and runs the row loop for the source and the merge; nothing while draw_skip is set.
; | Author: suinevere
; | Dependencies: g_plot_plan, g_plot_pos, rt_fb_rows, g_plot_keep, g_plot_orb, g_plot_mkb, g_plot_ohb, g_plot_mhb, g_plot_or0, g_plot_mk0, g_plot_orr, g_plot_mkr, g_bit_rot, game.inc (ARG_BLOCK)
; | Globals: XRAM rows, XBNK, blit_a, blit_b, ARG_BLOCK+0..5 (scratch once y and bitmap are read)
; | Params: TRH:TRL -- x; ACC -- width; B -- height; C -- flags (SPRITE_*), PLOT_UNPACKED for rows in blit_a and blit_b; ARG_BLOCK+0..1 -- y; ARG_BLOCK+2..4 -- the bitmap as g_tb_mark leaves it
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R0-R3, VRMAD, PSW; leaves XBNK at the last row's bank and VSEL bit 4 clear. Height 0 (256 rows in the C, unreached) draws nothing, and a packed masked sprite drawn at x not a multiple of 8 must be under 64 rows.
;----------------------
g_plot_core:
	ST $01
	LD HW_DRAW_SKIP
	BZ g_plot_core_go
	RET
g_plot_core_go:
	PUSH B
	PUSH C
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST $00
	INC VRMAD1
	LD VTRBF
	BP ACC, 7, g_plot_core_up
	BNZ g_plot_core_quit2
	LD $00
	AND #$E0
	BNZ g_plot_core_quit2
	MOV #$20, ACC
	SUB $00
	ST C
	LD B
	BZ g_plot_core_quit2
	SUB C
	LD B
	BP PSW, 7, g_plot_core_down
	LD C
g_plot_core_down:
	ST $03
	MOV #$00, $02
	BR g_plot_core_x
g_plot_core_quit2:
	POP ACC
g_plot_core_quit1:
	POP ACC
	RET
g_plot_core_up:
	XOR #$FF
	ST $02
	LD $00
	XOR #$FF
	ADD #$01
	ST $00
	LD $02
	ADDC #$00
	BNZ g_plot_core_quit2
	LD B
	SUB $00
	BZ g_plot_core_quit2
	BP PSW, 7, g_plot_core_quit2
	ST $03
	SUB #$21
	BP PSW, 7, g_plot_core_upn
	MOV #$20, $03
g_plot_core_upn:
	LD $00
	ST $02
	MOV #$00, $00
g_plot_core_x:
	LD TRL
	BP TRH, 7, g_plot_core_xneg
	AND #$E0
	BNZ g_plot_core_quit2
	BR g_plot_core_xok
g_plot_core_xneg:
	BZ g_plot_core_xok
	SUB #$E1
	BP PSW, 7, g_plot_core_quit2
g_plot_core_xok:
	POP ACC
	ST VRMAD1
	MOV #$03, C
	BN ACC, 7, g_plot_core_nb
	LD $01
	SUB #$20
	BN PSW, 7, g_plot_core_nb
	LD $01
	BZ g_plot_core_quit1
	DEC ACC
	ROR
	ROR
	ROR
	AND #$1F
	ST C
g_plot_core_nb:
	LD TRL
	AND #$07
	ST B
	BZ g_plot_core_r0
	SET1 C, 2
g_plot_core_r0:
	LD TRL
	ADD #$20
	AND #$F8
	OR C
	ST C
	MOV #<g_plot_plan, TRL
	MOV #>g_plot_plan, TRH
	LDC
	BZ g_plot_core_quit1
	PUSH ACC
	MOV #<g_plot_pos, TRL
	MOV #>g_plot_pos, TRH
	LD C
	LDC
	ST C
	MOV #<rt_fb_rows, TRL
	MOV #>rt_fb_rows, TRH
	LD $00
	LDC
	INC ACC
	ST TRL
	LD C
	ROR
	ROR
	AND #$03
	ADD TRL
	ST TRH
	MOV #$00, XBNK
	BN $00, 4, g_plot_core_bank
	MOV #$01, XBNK
g_plot_core_bank:
	LD C
	AND #$03
	ST $00
	POP ACC
	ST C
	LD B
	ROL
	ROL
	ROL
	OR C
	ST C
	BN VRMAD1, 7, g_plot_core_rom
	POP ACC
	LD $02
	ROL
	ROL
	AND #$FC
	OR $00
	ADD #HW_BLIT_A
	ST $00
	LD TRH
	ST $02
	BP VRMAD1, 0, g_plot_core_nk
	BN VRMAD1, 1, g_plot_core_nk
	LD VRMAD1
	PUSH ACC
	CALLF g_plot_keep
	POP VRMAD1
g_plot_core_nk:
	MOV #(g_bit_rot - g_bit_rot / 256 * 256) * 256, TRL
	LD C
	AND #$38
	BZ g_plot_core_h
	ROR
	ROR
	ROR
	ADD #>g_bit_rot
	ST TRH
	MOV #$01, ACC
	LDC
	ST $01
	ADD $01
	DEC ACC
	ST $01
	XOR #$FF
	ST B
	BP VRMAD1, 0, g_plot_core_mask
	BP VRMAD1, 1, g_plot_core_clear
	JMPF g_plot_orb
g_plot_core_mask:
	MOV #HWW_BLIT_B/256 + 256 * ((HWW_BLIT_B + $7F)/256 - HWW_BLIT_B/256), VRMAD2
	JMPF g_plot_mkb
g_plot_core_clear:
	MOV #>ARG_BLOCK + 256 * (>ARG_BLOCK - HWW_BLIT_B/256 - 1), VRMAD2
	JMPF g_plot_mkb
g_plot_core_h:
	MOV #>g_bit_rot, TRH
	BP VRMAD1, 0, g_plot_core_hmask
	BP VRMAD1, 1, g_plot_core_hclear
	JMPF g_plot_ohb
g_plot_core_hmask:
	MOV #HWW_BLIT_B/256 + 256 * ((HWW_BLIT_B + $7F)/256 - HWW_BLIT_B/256), VRMAD2
	JMPF g_plot_mhb
g_plot_core_hclear:
	MOV #>ARG_BLOCK + 256 * (>ARG_BLOCK - HWW_BLIT_B/256 - 1), VRMAD2
	JMPF g_plot_mhb
g_plot_core_rom:
	LD TRH
	PUSH ACC
	LD VRMAD1
	PUSH ACC
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
	LD $02
	ROL
	ROL
	ST $02
	AND #$FC
	OR $00
	ADD TRL
	ST TRL
	LD $02
	AND #$03
	ADDC TRH
	ST TRH
	POP $00
	POP $02
	POP ACC
	ROL
	ROL
	ST $01
	LD C
	AND #$38
	BNZ g_plot_core_romr
	BP $00, 0, g_plot_core_mk0
	LD TRL
	ST $00
	LD TRH
	ST VRMAD1
	MOV #(g_bit_rot - g_bit_rot / 256 * 256) * 256, TRL
	JMPF g_plot_or0
g_plot_core_mk0:
	LD $01
	AND #$FC
	ADD TRL
	ST $00
	LD $01
	AND #$03
	ADDC TRH
	ST B
	LD $00
	ST $01
	LD TRL
	ST $00
	LD TRH
	ST VRMAD1
	MOV #(g_bit_rot - g_bit_rot / 256 * 256) * 256, TRL
	JMPF g_plot_mk0
g_plot_core_romr:
	PUSH $00
	PUSH $01
	MOV #<ARG_BLOCK, VRMAD1
	LD TRL
	ST VTRBF
	INC VRMAD1
	LD TRH
	ST VTRBF
	CALLF g_plot_masks
	POP ACC
	AND #$FC
	ST $00
	POP ACC
	BP ACC, 0, g_plot_core_mkr
	JMPF g_plot_orr
g_plot_core_mkr:
	JMPF g_plot_mkr

;----------------------
; | g_plot_keep
; | Description: Stages the box a cleared sprite clears, the low width bits of a row, as row bytes k0 to k0 + 3 at ARG_BLOCK+2..5 for g_plot_mkb and g_plot_mhb.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: ARG_BLOCK+2..5
; | Params: R0 -- the first kept byte's place in blit_a, HW_BLIT_A + 4 skip + k0; R1 -- width
; | Returns: N/A. Destroys ACC, VRMAD, PSW; keeps B, C, R0-R3, TRH, TRL; leaves VSEL bit 4 clear.
;----------------------
g_plot_keep:
	PUSH B
	PUSH C
	PUSH $02
	PUSH $03
	SET1 VSEL, 4
	MOV #>(ARG_BLOCK+2), VRMAD2
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD $01
	AND #$07
	ST C
	MOV #$00, ACC
	INC C
g_plot_keep_p:
	DBNZ C, g_plot_keep_ps
	ST $03
	LD $01
	ROR
	ROR
	ROR
	AND #$1F
	ST B
	LD $00
	SUB #HW_BLIT_A
	AND #$03
	ST C
	MOV #$04, $02
g_plot_keep_k:
	LD C
	SUB B
	BZ g_plot_keep_part
	BP PSW, 7, g_plot_keep_full
	MOV #$00, ACC
	BR g_plot_keep_st
g_plot_keep_ps:
	SET1 PSW, 7
	ROLC
	BR g_plot_keep_p
g_plot_keep_part:
	LD $03
	BR g_plot_keep_st
g_plot_keep_full:
	MOV #$FF, ACC
g_plot_keep_st:
	ST VTRBF
	INC C
	DBNZ $02, g_plot_keep_k
	CLR1 VSEL, 4
	POP $03
	POP $02
	POP C
	POP B
	RET

;----------------------
; | g_plot_masks
; | Description: The row loops' shift by r, plan bits 3-5, from g_bit_rot page r's entry 1 (0x80 >> r). R1 keeps a byte's part shifted right, B the spill, and TRH:TRL selects the page.
; | Author: suinevere
; | Dependencies: g_bit_rot (256-aligned)
; | Globals: N/A
; | Params: C -- the plan
; | Returns: R1 -- 0xFF >> r; B -- its complement; TRH:TRL -- g_bit_rot + 256 r. Destroys ACC, PSW.
;----------------------
g_plot_masks:
	LD C
	AND #$38
	ROR
	ROR
	ROR
	ADD #>g_bit_rot
	ST TRH
	MOV #(g_bit_rot - g_bit_rot / 256 * 256) * 256, TRL
	MOV #$01, ACC
	LDC
	ST $01
	ADD $01
	DEC ACC
	ST $01
	XOR #$FF
	ST B
	RET

;----------------------
; | PNEXT
; | Description: Puts the next frame buffer row's first screen byte in R2, from this row's in ACC. Even rows add 6, odd ones 10, and row 16 starts XRAM bank 1 at 0x80.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: XBNK
; | Params: ACC -- this row's first byte, 0x81-0xFA as rt_fb_row's address + 1 + 0-3
; | Returns: N/A
;----------------------
%macro PNEXT
	ADD #$03
	BP ACC, 3, .odd%
	ADD #$03
	BR .st%
.odd%:
	ADD #$07
	BN PSW, 7, .st%
	ADD #$80
	MOV #$01, XBNK
.st%:
	ST $02
%end

;----------------------
; | PH_ORB / PH_MKB
; | Description: One unpacked byte at a whole-byte x: the image byte at @R0 bit-reversed and ORed into XRAM, after PH_MKB clears the mask byte at VTRBF; both pointers step.
; | Author: suinevere
; | Dependencies: g_bit_rot page 0 through TRH:TRL
; | Globals: XRAM
; | Params: N/A
; | Returns: N/A
;----------------------
%macro PH_ORB
	LD @R0
	LDC
	OR @R2
	ST @R2
	INC $02
	INC $00
%end

%macro PH_MKB
	LD VTRBF
	LDC
	XOR #$FF
	AND @R2
	ST @R2
	LD @R0
	LDC
	OR @R2
	ST @R2
	INC $02
	INC $00
	INC VRMAD1
%end

;----------------------
; | g_plot_orb
; | Description: The row loop for unpacked rows ORed into XRAM: per row the left-edge byte, the full bytes and the right-edge byte the plan holds.
; | Author: suinevere
; | Dependencies: PF_ORB, PE_ORB, PL_ORB, PNEXT
; | Globals: XRAM rows, blit_a
; | Params: R0 -- the first row's first kept byte in blit_a; R1, B -- the shift masks; R2 -- its first screen byte; R3 -- rows; C -- the plan (bit 7 left edge, bit 6 right edge, bits 0-2 full bytes); TRH:TRL -- g_bit_rot page r
; | Returns: N/A. Destroys ACC, R0, R2, R3, PSW.
;----------------------
g_plot_orb:
	PUSH $00
	PUSH $02
	BN C, 7, g_plot_orb_f
	PE_ORB
g_plot_orb_f:
	BP C, 2, g_plot_orb_a4
	BP C, 1, g_plot_orb_a23
	BP C, 0, g_plot_orb_a1
	BR g_plot_orb_l
g_plot_orb_a23:
	BP C, 0, g_plot_orb_a3
	BR g_plot_orb_a2
g_plot_orb_a4:
	PF_ORB
g_plot_orb_a3:
	PF_ORB
g_plot_orb_a2:
	PF_ORB
g_plot_orb_a1:
	PF_ORB
g_plot_orb_l:
	BN C, 6, g_plot_orb_n
	PL_ORB
g_plot_orb_n:
	POP ACC
	PNEXT
	POP ACC
	ADD #$04
	ST $00
	DBNZ $03, g_plot_orb_more
	RET
g_plot_orb_more:
	JMPF g_plot_orb

;----------------------
; | g_plot_ohb
; | Description: The row loop for unpacked rows at a whole-byte x ORed into XRAM, C bytes a row.
; | Author: suinevere
; | Dependencies: PH_ORB, PNEXT
; | Globals: XRAM rows, blit_a
; | Params: R0 -- the first row's first kept byte in blit_a; R2 -- its first screen byte; R3 -- rows; C -- bytes a row, 1-4; TRH:TRL -- g_bit_rot page 0
; | Returns: N/A. Destroys ACC, R0, R2, R3, PSW.
;----------------------
g_plot_ohb:
	PUSH $02
	BP C, 2, g_plot_ohb_4
	BP C, 1, g_plot_ohb_23
	BR g_plot_ohb_1
g_plot_ohb_23:
	BP C, 0, g_plot_ohb_3
	BR g_plot_ohb_2
g_plot_ohb_4:
	PH_ORB
g_plot_ohb_3:
	PH_ORB
g_plot_ohb_2:
	PH_ORB
g_plot_ohb_1:
	PH_ORB
	MOV #$04, ACC
	SUB C
	ADD $00
	ST $00
	POP ACC
	PNEXT
	DBNZ $03, g_plot_ohb
	RET

;----------------------
; | g_plot_mkb
; | Description: The row loop for unpacked rows merged through a mask. Blit_b's row (VRMAD2 page 0) or the box staged at ARG_BLOCK+2 (page 1) is cleared from XRAM before the image is ORed in.
; | Author: suinevere
; | Dependencies: PF_MKB, PE_MKB, PL_MKB, PNEXT, g_plot_keep
; | Globals: XRAM rows, blit_a, blit_b, ARG_BLOCK+2..5
; | Params: as g_plot_orb, with VRMAD2 the mask's page and VSEL bit 4 clear
; | Returns: N/A. Destroys ACC, R0, R2, R3, VRMAD1, PSW.
;----------------------
g_plot_mkb:
	PUSH $00
	PUSH $02
	BP VRMAD2, 0, g_plot_mkb_box
	LD $00
	SUB #HW_BLIT_A - <HWW_BLIT_B
	ST VRMAD1
	BR g_plot_mkb_e
g_plot_mkb_box:
	MOV #<(ARG_BLOCK+2), VRMAD1
g_plot_mkb_e:
	BN C, 7, g_plot_mkb_f
	PE_MKB
g_plot_mkb_f:
	BP C, 2, g_plot_mkb_a4
	BP C, 1, g_plot_mkb_a23
	BN C, 0, g_plot_mkb_z
	JMPF g_plot_mkb_a1
g_plot_mkb_z:
	JMPF g_plot_mkb_l
g_plot_mkb_a23:
	BP C, 0, g_plot_mkb_a3
	JMPF g_plot_mkb_a2
g_plot_mkb_a4:
	PF_MKB
g_plot_mkb_a3:
	PF_MKB
g_plot_mkb_a2:
	PF_MKB
g_plot_mkb_a1:
	PF_MKB
g_plot_mkb_l:
	BN C, 6, g_plot_mkb_n
	PL_MKB
g_plot_mkb_n:
	POP ACC
	PNEXT
	POP ACC
	ADD #$04
	ST $00
	DBNZ $03, g_plot_mkb_more
	RET
g_plot_mkb_more:
	JMPF g_plot_mkb

;----------------------
; | g_plot_mhb
; | Description: As g_plot_mkb for rows at a whole-byte x, C bytes a row.
; | Author: suinevere
; | Dependencies: PH_MKB, PNEXT
; | Globals: XRAM rows, blit_a, blit_b, ARG_BLOCK+2..5
; | Params: as g_plot_ohb, with VRMAD2 the mask's page and VSEL bit 4 clear
; | Returns: N/A. Destroys ACC, R0, R2, R3, VRMAD1, PSW.
;----------------------
g_plot_mhb:
	PUSH $02
	BP VRMAD2, 0, g_plot_mhb_box
	LD $00
	SUB #HW_BLIT_A - <HWW_BLIT_B
	ST VRMAD1
	BR g_plot_mhb_go
g_plot_mhb_box:
	MOV #<(ARG_BLOCK+2), VRMAD1
g_plot_mhb_go:
	BP C, 2, g_plot_mhb_4
	BP C, 1, g_plot_mhb_23
	BR g_plot_mhb_1
g_plot_mhb_23:
	BP C, 0, g_plot_mhb_3
	BR g_plot_mhb_2
g_plot_mhb_4:
	PH_MKB
g_plot_mhb_3:
	PH_MKB
g_plot_mhb_2:
	PH_MKB
g_plot_mhb_1:
	PH_MKB
	MOV #$04, ACC
	SUB C
	ADD $00
	ST $00
	POP ACC
	PNEXT
	DBNZ $03, g_plot_mhb_more
	RET
g_plot_mhb_more:
	JMPF g_plot_mhb

;----------------------
; | g_plot_or0
; | Description: The row loop for table rows at a whole-byte x ORed into XRAM, C bytes a row. The source address is VRMAD1:R0 (rows 4-aligned); g_bit_rot page 0 switches in byte by byte.
; | Author: suinevere
; | Dependencies: PH_OR0, PNEXT
; | Globals: XRAM rows
; | Params: R0, VRMAD1 -- the first row's first kept byte's low and high address; R2 -- its first screen byte; R3 -- rows; C -- bytes a row, 1-4; TRL -- 0
; | Returns: N/A. Destroys ACC, TRH, R0, R2, R3, VRMAD1, PSW.
;----------------------
g_plot_or0:
	PUSH $00
	PUSH $02
	BP C, 2, g_plot_or0_4
	BP C, 1, g_plot_or0_23
	BR g_plot_or0_1
g_plot_or0_23:
	BP C, 0, g_plot_or0_3
	BR g_plot_or0_2
g_plot_or0_4:
	PH_OR0
g_plot_or0_3:
	PH_OR0
g_plot_or0_2:
	PH_OR0
g_plot_or0_1:
	PH_OR0
	POP ACC
	PNEXT
	POP ACC
	ADD #$04
	ST $00
	BN PSW, 7, g_plot_or0_n
	INC VRMAD1
g_plot_or0_n:
	DBNZ $03, g_plot_or0
	RET

;----------------------
; | g_plot_mk0
; | Description: As g_plot_or0 with the mask row at B:R1 cleared from XRAM before each image byte is ORed in.
; | Author: suinevere
; | Dependencies: PH_MK0, PNEXT
; | Globals: XRAM rows
; | Params: as g_plot_or0, with R1, B the first mask byte's low and high address
; | Returns: N/A. Destroys ACC, B, TRH, R0-R3, VRMAD1, PSW.
;----------------------
g_plot_mk0:
	PUSH $00
	PUSH $01
	PUSH $02
	BP C, 2, g_plot_mk0_4
	BP C, 1, g_plot_mk0_23
	BR g_plot_mk0_1
g_plot_mk0_23:
	BP C, 0, g_plot_mk0_3
	BR g_plot_mk0_2
g_plot_mk0_4:
	PH_MK0
g_plot_mk0_3:
	PH_MK0
g_plot_mk0_2:
	PH_MK0
g_plot_mk0_1:
	PH_MK0
	POP ACC
	PNEXT
	POP ACC
	ADD #$04
	ST $01
	BN PSW, 7, g_plot_mk0_i
	INC B
g_plot_mk0_i:
	POP ACC
	ADD #$04
	ST $00
	BN PSW, 7, g_plot_mk0_n
	INC VRMAD1
g_plot_mk0_n:
	DBNZ $03, g_plot_mk0_more
	RET
g_plot_mk0_more:
	JMPF g_plot_mk0

;----------------------
; | PSTAGE / PSTAGE_MASK
; | Description: Copies four table bytes from the row address at ARG_BLOCK+0..1 (plus R0 for the mask) to ARG_BLOCK+2..5, then points VRMAD1 at them and TRH:TRL at g_bit_rot's page r. PSTAGE also moves the row address on by four bytes, as the image is a row's last staging.
; | Author: suinevere
; | Dependencies: g_bit_rot (256-aligned), game.inc (ARG_BLOCK)
; | Globals: ARG_BLOCK+0..5
; | Params: ARG_BLOCK+0..1 -- the row's first kept byte's address; R0 -- the mask's offset (PSTAGE_MASK); C -- the plan; VRMAD2 -- ARG_BLOCK's page
; | Returns: VRMAD1 -- ARG_BLOCK+2; TRH:TRL -- g_bit_rot + 256 r. Destroys ACC, PSW; leaves VSEL bit 4 clear.
;----------------------
%macro PSTAGE
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST TRL
	ADD #$04
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ST TRH
	ADDC #$00
	ST VTRBF
	INC VRMAD1
	SET1 VSEL, 4
	MOV #$00, ACC
	LDC
	ST VTRBF
	MOV #$01, ACC
	LDC
	ST VTRBF
	MOV #$02, ACC
	LDC
	ST VTRBF
	MOV #$03, ACC
	LDC
	ST VTRBF
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD C
	AND #$38
	ROR
	ROR
	ROR
	ADD #>g_bit_rot
	ST TRH
	MOV #(g_bit_rot - g_bit_rot / 256 * 256) * 256, TRL
%end

%macro PSTAGE_MASK
	SET1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ADD $00
	ST TRL
	LD VTRBF
	ADDC #$00
	ST TRH
	MOV #$00, ACC
	LDC
	ST VTRBF
	MOV #$01, ACC
	LDC
	ST VTRBF
	MOV #$02, ACC
	LDC
	ST VTRBF
	MOV #$03, ACC
	LDC
	ST VTRBF
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD C
	AND #$38
	ROR
	ROR
	ROR
	ADD #>g_bit_rot
	ST TRH
	MOV #(g_bit_rot - g_bit_rot / 256 * 256) * 256, TRL
%end

;----------------------
; | g_plot_orr
; | Description: The row loop for table rows at an x within a byte ORed into XRAM: each row is staged at ARG_BLOCK+2..5 by PSTAGE and merged as g_plot_orb merges blit_a.
; | Author: suinevere
; | Dependencies: PSTAGE, PF_ORV, PE_ORV, PL_ORV, PNEXT
; | Globals: XRAM rows, ARG_BLOCK+0..5
; | Params: ARG_BLOCK+0..1 -- the first row's first kept byte's address; R1, B -- the shift masks; R2 -- its first screen byte; R3 -- rows; C -- the plan; VRMAD2 -- ARG_BLOCK's page
; | Returns: N/A. Destroys ACC, TRL, TRH, R2, R3, VRMAD1, PSW.
;----------------------
g_plot_orr:
	PUSH $02
	PSTAGE
	BN C, 7, g_plot_orr_f
	PE_ORV
g_plot_orr_f:
	BP C, 2, g_plot_orr_a4
	BP C, 1, g_plot_orr_a23
	BP C, 0, g_plot_orr_a1
	BR g_plot_orr_l
g_plot_orr_a23:
	BP C, 0, g_plot_orr_a3
	BR g_plot_orr_a2
g_plot_orr_a4:
	PF_ORV
g_plot_orr_a3:
	PF_ORV
g_plot_orr_a2:
	PF_ORV
g_plot_orr_a1:
	PF_ORV
g_plot_orr_l:
	BN C, 6, g_plot_orr_n
	PL_ORV
g_plot_orr_n:
	POP ACC
	PNEXT
	DBNZ $03, g_plot_orr_more
	RET
g_plot_orr_more:
	JMPF g_plot_orr

;----------------------
; | g_plot_mkr
; | Description: As g_plot_orr for a masked table sprite: the mask row, R0 bytes on, is staged and cleared from XRAM first, then the image row is staged and ORed in.
; | Author: suinevere
; | Dependencies: PSTAGE, PSTAGE_MASK, PF_ANV, PE_ANV, PL_ANV, PF_ORV, PE_ORV, PL_ORV, PNEXT
; | Globals: XRAM rows, ARG_BLOCK+0..5
; | Params: as g_plot_orr, with R0 -- 4 * height, the mask's offset (under 256)
; | Returns: N/A. Destroys ACC, TRL, TRH, R2, R3, VRMAD1, PSW.
;----------------------
g_plot_mkr:
	PUSH $02
	PSTAGE_MASK
	BN C, 7, g_plot_mkr_f
	PE_ANV
g_plot_mkr_f:
	BP C, 2, g_plot_mkr_a4
	BP C, 1, g_plot_mkr_a23
	BP C, 0, g_plot_mkr_a1
	BR g_plot_mkr_l
g_plot_mkr_a23:
	BP C, 0, g_plot_mkr_a3
	BR g_plot_mkr_a2
g_plot_mkr_a4:
	PF_ANV
g_plot_mkr_a3:
	PF_ANV
g_plot_mkr_a2:
	PF_ANV
g_plot_mkr_a1:
	PF_ANV
g_plot_mkr_l:
	BN C, 6, g_plot_mkr_i
	PL_ANV
g_plot_mkr_i:
	POP ACC
	PUSH ACC
	ST $02
	PSTAGE
	BN C, 7, g_plot_mkr_if
	PE_ORV
g_plot_mkr_if:
	BP C, 2, g_plot_mkr_i4
	BP C, 1, g_plot_mkr_i23
	BP C, 0, g_plot_mkr_i1
	BR g_plot_mkr_il
g_plot_mkr_i23:
	BP C, 0, g_plot_mkr_i3
	BR g_plot_mkr_i2
g_plot_mkr_i4:
	PF_ORV
g_plot_mkr_i3:
	PF_ORV
g_plot_mkr_i2:
	PF_ORV
g_plot_mkr_i1:
	PF_ORV
g_plot_mkr_il:
	BN C, 6, g_plot_mkr_n
	PL_ORV
g_plot_mkr_n:
	POP ACC
	PNEXT
	DBNZ $03, g_plot_mkr_more
	RET
g_plot_mkr_more:
	JMPF g_plot_mkr

;----------------------
; | SCOPY1 / SCOPY2 / SCOPY3 / SCOPY4
; | Description: The row loops for stored rows: each row's 1-4 bytes copied from TRH:TRL + R0, R0 stepping, then zeros up to the row's four bytes.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: blit_a or blit_b through %dst
; | Params: %dst -- @R1 (blit_a) or VTRBF (blit_b); %inc -- the pointer to step, $01 or VRMAD1
; | Returns: N/A
;----------------------
%macro SCOPY1 %dst, %inc
.row%:
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	MOV #$00, ACC
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	DBNZ $03, .row%
	RET
%end

%macro SCOPY2 %dst, %inc
.row%:
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	MOV #$00, ACC
	ST %dst
	INC %inc
	ST %dst
	INC %inc
	DBNZ $03, .row%
	RET
%end

%macro SCOPY3 %dst, %inc
.row%:
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	MOV #$00, ACC
	ST %dst
	INC %inc
	DBNZ $03, .row%
	RET
%end

%macro SCOPY4 %dst, %inc
.row%:
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	LD $00
	LDC
	ST %dst
	INC %inc
	INC $00
	DBNZ $03, .row%
	RET
%end

;----------------------
; | g_sprite_find
; | Description: Finds the rows stored ready to draw for a bitmap's table offset, width, height and mask flag through g_spr_hash and the 64-slot key arrays tables.s generates.
; | Author: suinevere
; | Dependencies: pocketstation/build/vmu_game/tables.s (g_spr_hash, g_spr_kb, g_spr_kw, g_spr_kh, g_spr_lo, g_spr_hi)
; | Globals: N/A
; | Params: TRH:TRL -- the bitmap's table offset; R1 -- width; B -- height; C -- flags, bit 0 masked
; | Returns: CY set and TRH:TRL the stored rows' address, image rows then mask rows, when stored; else CY clear. Destroys ACC, R0, R2, R3, TRH, TRL, PSW; keeps R1, B, C.
;----------------------
g_sprite_find:
	LD TRL
	ROR
	ROR
	AND #$3F
	ST $00
	LD TRH
	ST $03
	LD $01
	ROL
	ROL
	ROL
	ADD $03
	AND #$3F
	MOV #<g_spr_hash, TRL
	MOV #>g_spr_hash, TRH
	LDC
	ADD $00
	AND #$3F
	ST $02
	MOV #<g_spr_kb, TRL
	MOV #>g_spr_kb, TRH
	LDC
	BNE $03, g_sprite_find_no
	MOV #<g_spr_kw, TRL
	MOV #>g_spr_kw, TRH
	LD $02
	LDC
	BNE $01, g_sprite_find_no
	MOV #<g_spr_kh, TRL
	MOV #>g_spr_kh, TRH
	LD $02
	LDC
	BN C, 0, g_sprite_find_plain
	XOR #$80
	BR g_sprite_find_height
g_sprite_find_plain:
	AND #$7F
g_sprite_find_height:
	BNE B, g_sprite_find_no
	MOV #<g_spr_lo, TRL
	MOV #>g_spr_lo, TRH
	LD $02
	LDC
	ST $00
	MOV #<g_spr_hi, TRL
	MOV #>g_spr_hi, TRH
	LD $02
	LDC
	ST TRH
	LD $00
	ST TRL
	SET1 PSW, 7
	RET
g_sprite_find_no:
	CLR1 PSW, 7
	RET

;----------------------
; | g_spr_copy_a / g_spr_copy_b
; | Description: Copies R3 stored rows of (width + 7) / 8 bytes from TRH:TRL + R0 into blit_a through @R1 (copy_a) or blit_b through VTRBF (copy_b), zeros above.
; | Author: suinevere
; | Dependencies: SCOPY1..SCOPY4
; | Globals: blit_a, blit_b
; | Params: R0 -- the first byte's offset from TRH:TRL; R1 (copy_a) or VRMAD, VSEL bit 4 clear (copy_b) -- the first row; R2 -- width, 1-31; R3 -- rows; TRH:TRL -- the stored rows
; | Returns: R0 -- the offset after the last byte copied. Destroys ACC, R1 (copy_a) or VRMAD1 (copy_b), R3, PSW; keeps B, C, R2, TRH, TRL.
;----------------------
g_spr_copy_a:
	LD $02
	DEC ACC
	AND #$18
	BZ g_spr_ca1
	BE #$08, g_spr_ca2
	BE #$10, g_spr_ca3
	BR g_spr_ca4
g_spr_ca1:
	SCOPY1 @R1, $01
g_spr_ca2:
	SCOPY2 @R1, $01
g_spr_ca3:
	SCOPY3 @R1, $01
g_spr_ca4:
	SCOPY4 @R1, $01
g_spr_copy_b:
	LD $02
	DEC ACC
	AND #$18
	BZ g_spr_cb1
	BE #$08, g_spr_cb2
	BE #$10, g_spr_cb3
	BR g_spr_cb4
g_spr_cb1:
	SCOPY1 VTRBF, VRMAD1
g_spr_cb2:
	SCOPY2 VTRBF, VRMAD1
g_spr_cb3:
	SCOPY3 VTRBF, VRMAD1
g_spr_cb4:
	SCOPY4 VTRBF, VRMAD1

;----------------------
; | g_tb_find
; | Description: Where the GAME keeps a table extent. CY is set, with TRH the GAME page, when every file page from TRH:TRL to TRH:TRL + B - 1 is in app_tb_pages; else CY is clear.
; | Author: suinevere
; | Dependencies: app_tb_map, app_tb_first, app_tb_count
; | Globals: N/A
; | Params: TRH:TRL -- file offset; B -- bytes, 1-255, or 0 for 256
; | Returns: CY as above; TRH -- the GAME page when set, else kept. Keeps TRL, B, C and R0-R3; destroys ACC and PSW's other flags.
;----------------------
g_tb_find:
	PUSH TRL
	PUSH TRH
	LD B
	DEC ACC
	ADD TRL
	LD TRH
	ADDC #$00
	SUB #app_tb_first
	BP PSW, 7, g_tb_find_no
	ST TRL
	SUB #app_tb_count
	BN PSW, 7, g_tb_find_no
	LD TRH
	SUB #app_tb_first
	BP PSW, 7, g_tb_find_no
	PUSH ACC
	LD TRL
	MOV #<app_tb_map, TRL
	MOV #>app_tb_map, TRH
	LDC
	BE #$FF, g_tb_find_no1
	POP ACC
	LDC
	BE #$FF, g_tb_find_no
	POP TRL
	POP TRL
	ST TRH
	SET1 PSW, 7
	RET
g_tb_find_no1:
	POP ACC
g_tb_find_no:
	POP TRH
	POP TRL
	CLR1 PSW, 7
	RET

;----------------------
; | g_tb_extent
; | Description: The bytes unpack_rows reads for a bitmap, 4 * ceil(width * height / 32), at most 128.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- width, 0-32; B -- height, 1-32
; | Returns: ACC -- the byte count. Keeps B and C; destroys PSW's flags.
;----------------------
g_tb_extent:
	PUSH B
	PUSH C
	ST C
	MOV #$00, ACC
	MUL
	ST B
	LD C
	ADD #$1F
	ROR
	ROR
	ROR
	AND #$1F
	ST C
	LD B
	ADDC #$00
	ROR
	ROR
	ROR
	AND #$E0
	OR C
	AND #$FC
	POP C
	POP B
	RET

;----------------------
; | g_tb_mark
; | Description: Before g_plot_core plots a table bitmap: ARG_BLOCK+2..3 becomes its GAME address and ARG_BLOCK+4 0 when the GAME keeps its rows (4 height bytes, twice masked), else ARG_BLOCK+4 is 1.
; | Author: suinevere
; | Dependencies: g_tb_find, game.inc (ARG_BLOCK)
; | Globals: ARG_BLOCK+2..4 (the bitmap's value slot)
; | Params: B -- height; C -- flags, bit 0 masked; ARG_BLOCK+2..3 -- the bitmap's table offset
; | Returns: N/A. Keeps ACC, B, C, TRH and TRL; destroys VRMAD and PSW's flags; leaves VSEL bit 4 clear.
;----------------------
g_tb_mark:
	PUSH ACC
	PUSH TRL
	PUSH TRH
	PUSH B
	CLR1 VSEL, 4
	MOV #>(ARG_BLOCK+2), VRMAD2
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
	LD B
	ROL
	ROL
	AND #$FC
	ST B
	BN C, 0, g_tb_mark_one
	ADD B
	ST B
g_tb_mark_one:
	CALLF g_tb_find
	MOV #<(ARG_BLOCK+4), VRMAD1
	BN PSW, 7, g_tb_mark_save
	MOV #$00, VTRBF
	MOV #<(ARG_BLOCK+3), VRMAD1
	LD TRH
	ST VTRBF
	BR g_tb_mark_out
g_tb_mark_save:
	MOV #$01, VTRBF
g_tb_mark_out:
	POP B
	POP TRH
	POP TRL
	POP ACC
	RET

;----------------------
; | g_bit_keep
; | Description: The low n bits, 0xFF for n = 0, indexed by width & 7, for the last byte of a row.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
g_bit_keep:
	.byte $FF, $01, $03, $07, $0F, $1F, $3F, $7F

;----------------------
; | g_sprite_draw
; | Description: game_sprite_draw: draws a sprite into XRAM. Rows not 32 wide or mirrored go to blit_a (mask to blit_b) first, copied when stored ready to draw, else unpacked.
; | Author: suinevere
; | Dependencies: g_sprite_find, g_spr_copy_a, g_spr_copy_b, g_mirror_pass, g_tb_mark, g_tb_extent, g_tb_find, g_unpack_core, g_plot_core, game.inc (ARG_BLOCK)
; | Globals: XRAM rows, XBNK, blit_a, blit_b
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ACC -- width, 1-32; B -- height, 1-32; ARG_BLOCK+2..5 -- bitmap, a card address whose low 16 bits are the table offset; C -- flags (SPRITE_*)
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, R0-R3, VRMAD, FPR, PSW; leaves XBNK at the last row's bank and VSEL bit 4 clear.
;----------------------
g_sprite_draw:
	ST $01
	LD C
	AND #$07
	ST C
	LD $01
	BNE #$20, g_sprite_draw_unp
	BP C, 2, g_sprite_draw_unp
	CALLF g_tb_mark
	JMPF g_plot_core
g_sprite_draw_unp:
	PUSH TRL
	PUSH TRH
	PUSH $01
	PUSH B
	PUSH C
	CLR1 VSEL, 4
	MOV #>(ARG_BLOCK+2), VRMAD2
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
	LD $01
	AND #$07
	BZ g_sprite_draw_unpack
	CALLF g_sprite_find
	BN PSW, 7, g_sprite_draw_miss
	JMPF g_sprite_draw_hit
g_sprite_draw_miss:
	MOV #<(ARG_BLOCK+2), VRMAD1
	LD VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
g_sprite_draw_unpack:
	LD $01
	CALLF g_tb_extent
	BN C, 0, g_sprite_draw_ext
	ST $00
	ADD $00
g_sprite_draw_ext:
	PUSH B
	ST B
	CALLF g_tb_find
	POP B
	LD C
	AND #SPRITE_MIRROR
	ST C
	LD $01
	CALLF g_unpack_core
	POP C
	POP B
	POP ACC
	BN C, 0, g_sprite_draw_plot
	ST $01
	PUSH ACC
	PUSH B
	PUSH C
	LD C
	AND #SPRITE_MIRROR
	OR #$01
	ST C
	LD $01
	CALLF g_unpack_core
	POP C
	POP B
	POP ACC
g_sprite_draw_plot:
	POP TRH
	POP TRL
	SET1 C, 7
	JMPF g_plot_core
g_sprite_draw_hit:
	LD $01
	ST $02
	LD B
	ST $03
	MOV #$00, $00
	MOV #HW_BLIT_A, $01
	CALLF g_spr_copy_a
	BN C, 0, g_sprite_draw_flip
	MOV #HWW_BLIT_B/256 + 256 * ((HWW_BLIT_B + $7F)/256 - HWW_BLIT_B/256), VRMAD2
	MOV #<HWW_BLIT_B, VRMAD1
	LD B
	ST $03
	CALLF g_spr_copy_b
g_sprite_draw_flip:
	BN C, 2, g_sprite_draw_done
	BN C, 0, g_sprite_draw_flip_a
	PUSH $02
	PUSH B
	MOV #$01, $01
	LD B
	ST $03
	CALLF g_mirror_pass
	POP B
	POP $02
g_sprite_draw_flip_a:
	MOV #$00, $01
	LD B
	ST $03
	CALLF g_mirror_pass
g_sprite_draw_done:
	POP C
	POP B
	POP ACC
	JMPF g_sprite_draw_plot

;----------------------
; | ROM_SPRITES_OFF
; | Description: File offset of sprite.c's ROM_SPRITES: eight bytes per sprite id, an offset/size word then the bitmap's card address.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/sprite.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_SPRITES_OFF	EQU	$A03C

;----------------------
; | g_sprite_info
; | Description: game_sprite_info: sprite id's SpriteInfo at x, y: x plus the low six bits of its word's first byte, y plus its second, width and height from the next two, flags from the first byte's top two bits, then the bitmap word.
; | Author: suinevere
; | Dependencies: app_tables, ROM_SPRITES_OFF, game.inc (ARG_BLOCK), g_tr_x8
; | Globals: N/A
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ARG_BLOCK+2..3 -- id
; | Returns: ARG_BLOCK+0..11 -- the SpriteInfo (x, y, width, height, flags, a pad byte, bitmap), over y and id. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_sprite_info:
	LD TRL
	ST $00
	LD TRH
	ST $01
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST $02
	INC VRMAD1
	LD VTRBF
	ST $03
	INC VRMAD1
	LD VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ST TRH
	CALLF g_tr_x8
	LD TRL
	ADD #<(app_tables+ROM_SPRITES_OFF)
	ST TRL
	LD TRH
	ADDC #>(app_tables+ROM_SPRITES_OFF)
	ST TRH
	MOV #<ARG_BLOCK, VRMAD1
	MOV #$00, ACC
	LDC
	ST C
	AND #$3F
	ADD $00
	ST VTRBF
	INC VRMAD1
	MOV #$00, ACC
	ADDC $01
	ST VTRBF
	INC VRMAD1
	MOV #$01, ACC
	LDC
	ADD $02
	ST VTRBF
	INC VRMAD1
	MOV #$00, ACC
	ADDC $03
	ST VTRBF
	INC VRMAD1
	MOV #$02, ACC
	LDC
	ST VTRBF
	INC VRMAD1
	MOV #$03, ACC
	LDC
	ST VTRBF
	INC VRMAD1
	LD C
	ROL
	ROL
	AND #$03
	ST VTRBF
	INC VRMAD1
	INC VRMAD1
	MOV #$04, B
	MOV #$04, C
g_sprite_info_bitmap:
	LD C
	LDC
	ST VTRBF
	INC VRMAD1
	INC C
	DBNZ B, g_sprite_info_bitmap
	RET

;----------------------
; | ROM_FRAME_SETS_OFF
; | Description: File offset of sprite.c's ROM_FRAME_SETS: one card address per object type from 0x58, each pointing at that type's frame set.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/sprite.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_FRAME_SETS_OFF	EQU	$A384

;----------------------
; | g_frame_info
; | Description: game_frame_info for its two callers only: frame's dx, dy, bitmap word and the low byte of extra. The 0x80 set kind masks bit 31; others take dx from bits 24-30.
; | Author: suinevere
; | Dependencies: app_tables, ROM_FRAME_SETS_OFF, g_fi_add, g_tr_x4
; | Globals: N/A
; | Params: ACC -- type; B -- frame
; | Returns: R0-R3 (0x00-0x03) -- bitmap, low byte first; B -- dx; C -- dy; ACC -- extra's low byte. The word field is never returned. Destroys TRH, TRL, PSW; keeps VRMAD.
;----------------------
g_frame_info:
	ST TRL
	MOV #$00, TRH
	PUSH B
	CALLF g_tr_x4
	LD TRL
	ADD #<(app_tables+ROM_FRAME_SETS_OFF-$160)
	ST TRL
	LD TRH
	ADDC #>(app_tables+ROM_FRAME_SETS_OFF-$160)
	ST TRH
	MOV #$00, ACC
	LDC
	ST $00
	MOV #$01, ACC
	LDC
	ST $01
	LD $00
	ADD #<app_tables
	ST TRL
	LD $01
	ADDC #>app_tables
	ST TRH
	MOV #$01, ACC
	LDC
	ST C
	POP B
	MOV #$00, ACC
	LDC
	BNE #$80, g_frame_info_list
	MOV #$03, ACC
	CALLF g_fi_add
	PUSH C
	MOV #$04, ACC
	LDC
	ST $00
	MOV #$05, ACC
	LDC
	ST $01
	MOV #$06, ACC
	LDC
	ST $02
	MOV #$07, ACC
	LDC
	ST B
	AND #$7F
	ST $03
	MOV #$0B, ACC
	LDC
	ST C
	POP ACC
	RET
g_frame_info_list:
	BNE #$81, g_frame_info_plain
	PUSH C
	LD TRL
	ADD #$04
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	BR g_frame_info_word
g_frame_info_plain:
	MOV #$00, ACC
	PUSH ACC
g_frame_info_word:
	MOV #$02, ACC
	CALLF g_fi_add
	MOV #$00, ACC
	LDC
	ST $00
	MOV #$01, ACC
	LDC
	ST $01
	MOV #$02, ACC
	LDC
	ST $02
	MOV #$03, ACC
	LDC
	ST $03
	AND #$7F
	ST B
	MOV #$00, C
	POP ACC
	RET

;----------------------
; | g_fi_add
; | Description: Adds B shifted left ACC places to TRH:TRL, for g_frame_info's frame offsets.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- the shift, 1-8; B -- the frame; TRH:TRL -- the address
; | Returns: TRH:TRL -- the address plus B << ACC. Keeps B and C; destroys ACC, R0-R2, PSW.
;----------------------
g_fi_add:
	ST $02
	LD B
	ST $00
	MOV #$00, $01
g_fi_add_shift:
	LD $00
	ADD $00
	ST $00
	LD $01
	ADDC $01
	ST $01
	DBNZ $02, g_fi_add_shift
	LD TRL
	ADD $00
	ST TRL
	LD TRH
	ADDC $01
	ST TRH
	RET
