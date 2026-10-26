;----------------------
; | reel.s
; | Description: reel.c in LC86K assembly: a reel's speed-up and advance, the 16-bit wait, strip rolls and random stops. Also the reels' steering, turning and step, next symbols, the shuffle and three picks; loading strips and reel A raised.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/reel.c, pocketstation/build/vmu_game/rrl_ram.inc, game.inc (ARG_BLOCK)
; | Globals: reel_phase, mode_wait, wait_hi, reel_len, reel_speed_a, reel_speed_b, reel_speed_c, reel_pos_a, reel_pos_b, reel_pos_c, strip_a, strip_b, strip_c, seed
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_speed_up
; | Description: speed_up: speed plus step, held at cap.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRH:TRL -- speed; ACC -- step; B -- cap (the callers' caps are under 256)
; | Returns: TRH:TRL -- the new speed. Destroys ACC, PSW.
;----------------------
g_speed_up:
	ADD TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	BP PSW, 7, g_speed_up_cap
	BNZ g_speed_up_cap
	LD TRL
	SUB B
	BN PSW, 7, g_speed_up_cap
	RET
g_speed_up_cap:
	LD B
	ST TRL
	MOV #$00, TRH
	RET

;----------------------
; | g_reel_advance
; | Description: reel_advance: moves a reel by its speed; below limit that is the new position, past it the position wraps by limit, except in reel phase 2 or later at a crawl (speed 0x38 or less), where the reel stops at 0 with its speed cleared.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK), rrl_ram.inc (HW_REEL_PHASE)
; | Globals: reel_phase
; | Params: ARG_BLOCK+0..1 -- *speed; TRH:TRL -- pos; ARG_BLOCK+2..5 -- limit, 32-bit
; | Returns: TRH:TRL -- the new position; ARG_BLOCK+0..1 -- *speed. Destroys ACC, B, C, R0-R2, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_reel_advance:
	SET1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST B
	LD VTRBF
	ST C
	LD TRL
	ADD B
	ST TRL
	LD TRH
	ADDC C
	ST TRH
	MOV #$00, ACC
	ADDC #$00
	ST $02
	LD TRL
	SUB VTRBF
	ST $00
	LD TRH
	SUBC VTRBF
	ST $01
	LD $02
	SUBC VTRBF
	MOV #$00, ACC
	SUBC VTRBF
	CLR1 VSEL, 4
	BP PSW, 7, g_reel_advance_ret
	LD HW_REEL_PHASE
	SUB #$02
	BP PSW, 7, g_reel_advance_wrap
	LD C
	BNZ g_reel_advance_wrap
	LD B
	SUB #$39
	BN PSW, 7, g_reel_advance_wrap
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #$00, VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	MOV #$00, TRL
	MOV #$00, TRH
	RET
g_reel_advance_wrap:
	LD $00
	ST TRL
	LD $01
	ST TRH
g_reel_advance_ret:
	RET

;----------------------
; | g_wait16_done
; | Description: wait16_done: counts the 16-bit wait in mode_wait and wait_hi down by one and returns 1 when it reaches 0, else 0.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_MODE_WAIT, HW_WAIT_HI)
; | Globals: mode_wait, wait_hi
; | Params: N/A
; | Returns: ACC -- 1 when the wait is over, else 0. Destroys nothing else.
;----------------------
g_wait16_done:
	LD HW_MODE_WAIT
	BNZ g_wait16_done_low
	DEC HW_WAIT_HI
g_wait16_done_low:
	DEC HW_MODE_WAIT
	LD HW_MODE_WAIT
	OR HW_WAIT_HI
	BZ g_wait16_done_over
	MOV #$00, ACC
	RET
g_wait16_done_over:
	MOV #$01, ACC
	RET

;----------------------
; | g_strip_roll
; | Description: strip_roll: a random value below len times 16, divided by 16, so a strip index below len.
; | Author: suinevere
; | Dependencies: g_random
; | Globals: seed
; | Params: ACC -- len
; | Returns: ACC -- the index. Destroys B, C, TRH, TRL, PSW; four bytes of stack.
;----------------------
g_strip_roll:
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
	ST B
	LD TRH
	ROL
	ROL
	ROL
	ROL
	AND #$F0
	OR B
	RET

;----------------------
; | g_reels_steer
; | Description: game_reels_steer: phase 0 speeds reels A, C, B up in turn as mode_wait counts to 0x28, phase 1 counts the 0xA0 wait down. Phase 2 slows the reels; each finished phase moves reel_phase on.
; | Author: suinevere
; | Dependencies: g_speed_up, g_reels_steer_down, rrl_ram.inc (HW_REEL_PHASE, HW_MODE_WAIT, HW_REEL_SPEED_A, HW_REEL_SPEED_B, HW_REEL_SPEED_C)
; | Globals: reel_phase, mode_wait, reel_speed_a, reel_speed_b, reel_speed_c
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, PSW.
;----------------------
g_reels_steer:
	LD HW_MODE_WAIT
	ST C
	LD HW_REEL_PHASE
	BZ g_reels_steer_up
	BE #$01, g_reels_steer_wait
	BE #$02, g_reels_steer_down_go
	RET
g_reels_steer_down_go:
	JMPF g_reels_steer_down
g_reels_steer_wait:
	DEC HW_MODE_WAIT
	LD HW_MODE_WAIT
	BNZ g_reels_steer_wait_ret
	INC HW_REEL_PHASE
g_reels_steer_wait_ret:
	RET
g_reels_steer_up:
	LD C
	SUB #$28
	BN PSW, 7, g_reels_steer_up_a
	INC C
	LD C
	ST HW_MODE_WAIT
g_reels_steer_up_a:
	LD HW_REEL_SPEED_A
	ST TRL
	LD HW_REEL_SPEED_A+1
	ST TRH
	MOV #$60, B
	MOV #$04, ACC
	CALLF g_speed_up
	LD TRL
	ST HW_REEL_SPEED_A
	LD TRH
	ST HW_REEL_SPEED_A+1
	LD C
	SUB #$14
	BP PSW, 7, g_reels_steer_ret
	LD HW_REEL_SPEED_C
	ST TRL
	LD HW_REEL_SPEED_C+1
	ST TRH
	MOV #$70, B
	MOV #$05, ACC
	CALLF g_speed_up
	LD TRL
	ST HW_REEL_SPEED_C
	LD TRH
	ST HW_REEL_SPEED_C+1
	LD C
	SUB #$28
	BP PSW, 7, g_reels_steer_ret
	LD HW_REEL_SPEED_B
	ST TRL
	LD HW_REEL_SPEED_B+1
	ST TRH
	MOV #$80, B
	MOV #$04, ACC
	CALLF g_speed_up
	LD TRL
	ST HW_REEL_SPEED_B
	LD TRH
	ST HW_REEL_SPEED_B+1
	BNZ g_reels_steer_ret
	LD TRL
	BNE #$80, g_reels_steer_ret
	LD HW_REEL_SPEED_A+1
	BNZ g_reels_steer_up_c
	LD HW_REEL_SPEED_A
	SUB #$60
	BP PSW, 7, g_reels_steer_ret
g_reels_steer_up_c:
	LD HW_REEL_SPEED_C+1
	BNZ g_reels_steer_up_end
	LD HW_REEL_SPEED_C
	SUB #$70
	BP PSW, 7, g_reels_steer_ret
g_reels_steer_up_end:
	MOV #$A0, HW_MODE_WAIT
	INC HW_REEL_PHASE
g_reels_steer_ret:
	RET

;----------------------
; | g_reels_steer_down
; | Description: game_reels_steer's phase 2, jumped to with C holding mode_wait. It counts mode_wait to 0x54 and slows A by 6, C by 5 and B by 4 a call, in turn, to the crawl 0x38.
; | Author: suinevere
; | Dependencies: g_reel_crawling, rrl_ram.inc (HW_REEL_PHASE, HW_MODE_WAIT, HW_REEL_SPEED_A, HW_REEL_SPEED_B, HW_REEL_SPEED_C)
; | Globals: reel_phase, mode_wait, reel_speed_a, reel_speed_b, reel_speed_c
; | Params: C -- mode_wait
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, PSW.
;----------------------
g_reels_steer_down:
	LD C
	SUB #$54
	BN PSW, 7, g_reels_steer_down_a
	INC C
	LD C
	ST HW_MODE_WAIT
g_reels_steer_down_a:
	LD HW_REEL_SPEED_A
	ST TRL
	LD HW_REEL_SPEED_A+1
	ST TRH
	CALLF g_reel_crawling
	BNZ g_reels_steer_down_ta
	LD TRL
	SUB #$06
	ST HW_REEL_SPEED_A
	LD TRH
	SUBC #$00
	ST HW_REEL_SPEED_A+1
g_reels_steer_down_ta:
	LD C
	SUB #$3C
	BP PSW, 7, g_reels_steer_down_ret
	LD HW_REEL_SPEED_C
	ST TRL
	LD HW_REEL_SPEED_C+1
	ST TRH
	CALLF g_reel_crawling
	BNZ g_reels_steer_down_tc
	LD TRL
	SUB #$05
	ST HW_REEL_SPEED_C
	LD TRH
	SUBC #$00
	ST HW_REEL_SPEED_C+1
g_reels_steer_down_tc:
	LD C
	SUB #$54
	BP PSW, 7, g_reels_steer_down_ret
	LD HW_REEL_SPEED_B
	ST TRL
	LD HW_REEL_SPEED_B+1
	ST TRH
	CALLF g_reel_crawling
	BNZ g_reels_steer_down_ac
	LD TRL
	SUB #$04
	ST HW_REEL_SPEED_B
	LD TRH
	SUBC #$00
	ST HW_REEL_SPEED_B+1
	RET
g_reels_steer_down_ac:
	LD HW_REEL_SPEED_A
	ST TRL
	LD HW_REEL_SPEED_A+1
	ST TRH
	CALLF g_reel_crawling
	BZ g_reels_steer_down_ret
	LD HW_REEL_SPEED_C
	ST TRL
	LD HW_REEL_SPEED_C+1
	ST TRH
	CALLF g_reel_crawling
	BZ g_reels_steer_down_ret
	INC HW_REEL_PHASE
g_reels_steer_down_ret:
	RET

;----------------------
; | g_reel_crawling
; | Description: Whether a reel speed is at the crawl (0x38, reel.c's REEL_CRAWL) or below.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRH:TRL -- the speed
; | Returns: ACC -- 1 when the speed is 0x38 or less, else 0. Destroys PSW.
;----------------------
g_reel_crawling:
	LD TRH
	BNZ g_reel_crawling_no
	LD TRL
	SUB #$39
	BN PSW, 7, g_reel_crawling_no
	MOV #$01, ACC
	RET
g_reel_crawling_no:
	MOV #$00, ACC
	RET

;----------------------
; | g_reel_turn
; | Description: reel_advance in registers for g_reels_turn: moves a reel by its speed, wrapping past limit. At a crawl in reel phase 2 or later it stops the reel at 0 with its speed cleared.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_REEL_PHASE)
; | Globals: reel_phase
; | Params: TRH:TRL -- pos; C:B -- speed; R1:R0 -- limit's bits 16 and 8-15 (its low byte is 0)
; | Returns: TRH:TRL -- the new position; C:B -- the new speed. Destroys ACC, R2, R3, PSW.
;----------------------
g_reel_turn:
	LD TRL
	ADD B
	ST TRL
	LD TRH
	ADDC C
	ST TRH
	MOV #$00, ACC
	ADDC #$00
	ST $02
	LD TRH
	SUB $00
	ST $03
	LD $02
	SUBC $01
	BP PSW, 7, g_reel_turn_ret
	LD HW_REEL_PHASE
	SUB #$02
	BP PSW, 7, g_reel_turn_wrap
	LD C
	BNZ g_reel_turn_wrap
	LD B
	SUB #$39
	BN PSW, 7, g_reel_turn_wrap
	MOV #$00, B
	MOV #$00, C
	MOV #$00, TRL
	MOV #$00, TRH
	RET
g_reel_turn_wrap:
	LD $03
	ST TRH
g_reel_turn_ret:
	RET

;----------------------
; | g_reels_turn
; | Description: game_reels_turn: advances each moving reel by its speed within reel_len times 512, then moves reel phase 3 to 4 once all three have stopped.
; | Author: suinevere
; | Dependencies: g_reel_turn, rrl_ram.inc (HW_REEL_LEN, HW_REEL_PHASE, HW_REEL_SPEED_A, HW_REEL_SPEED_B, HW_REEL_SPEED_C, HW_REEL_POS_A, HW_REEL_POS_B, HW_REEL_POS_C)
; | Globals: reel_len, reel_phase, reel_speed_a, reel_speed_b, reel_speed_c, reel_pos_a, reel_pos_b, reel_pos_c
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, PSW.
;----------------------
g_reels_turn:
	LD HW_REEL_LEN
	ADD HW_REEL_LEN
	ST $00
	MOV #$00, ACC
	ADDC #$00
	ST $01
	LD HW_REEL_SPEED_A
	ST B
	LD HW_REEL_SPEED_A+1
	ST C
	OR B
	BZ g_reels_turn_b
	LD HW_REEL_POS_A
	ST TRL
	LD HW_REEL_POS_A+1
	ST TRH
	CALLF g_reel_turn
	LD B
	ST HW_REEL_SPEED_A
	LD C
	ST HW_REEL_SPEED_A+1
	LD TRL
	ST HW_REEL_POS_A
	LD TRH
	ST HW_REEL_POS_A+1
g_reels_turn_b:
	LD HW_REEL_SPEED_B
	ST B
	LD HW_REEL_SPEED_B+1
	ST C
	OR B
	BZ g_reels_turn_c
	LD HW_REEL_POS_B
	ST TRL
	LD HW_REEL_POS_B+1
	ST TRH
	CALLF g_reel_turn
	LD B
	ST HW_REEL_SPEED_B
	LD C
	ST HW_REEL_SPEED_B+1
	LD TRL
	ST HW_REEL_POS_B
	LD TRH
	ST HW_REEL_POS_B+1
g_reels_turn_c:
	LD HW_REEL_SPEED_C
	ST B
	LD HW_REEL_SPEED_C+1
	ST C
	OR B
	BZ g_reels_turn_phase
	LD HW_REEL_POS_C
	ST TRL
	LD HW_REEL_POS_C+1
	ST TRH
	CALLF g_reel_turn
	LD B
	ST HW_REEL_SPEED_C
	LD C
	ST HW_REEL_SPEED_C+1
	LD TRL
	ST HW_REEL_POS_C
	LD TRH
	ST HW_REEL_POS_C+1
g_reels_turn_phase:
	LD HW_REEL_PHASE
	BNE #$03, g_reels_turn_ret
	LD HW_REEL_SPEED_A
	OR HW_REEL_SPEED_A+1
	OR HW_REEL_SPEED_B
	OR HW_REEL_SPEED_B+1
	OR HW_REEL_SPEED_C
	OR HW_REEL_SPEED_C+1
	BNZ g_reels_turn_ret
	MOV #$04, HW_REEL_PHASE
g_reels_turn_ret:
	RET

;----------------------
; | g_strip_next_index
; | Description: game_strip_next_symbol's index: pos / 64 / 10 with the CPU's 16/8 DIV, plus 1, wrapping to 0 at len.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- len; TRH:TRL -- pos
; | Returns: C -- the strip index. Keeps ACC; destroys B, PSW.
;----------------------
g_strip_next_index:
	PUSH ACC
	LD TRL
	ROL
	ROL
	AND #$03
	ST C
	LD TRH
	ROL
	ROL
	ST B
	AND #$FC
	OR C
	ST C
	LD B
	AND #$03
	MOV #$0A, B
	DIV
	INC C
	POP ACC
	BNE C, g_strip_next_index_ret
	MOV #$00, C
g_strip_next_index_ret:
	RET

;----------------------
; | g_strip_next_symbol
; | Description: game_strip_next_symbol: the strip's symbol after the one at pos (pos / 64 / 10, plus 1, wrapping to 0 at len), read from work RAM.
; | Author: suinevere
; | Dependencies: g_strip_next_index, game.inc (ARG_BLOCK)
; | Globals: strip_a, strip_b, strip_c (the strips every caller passes)
; | Params: ACC -- len; TRH:TRL -- pos; ARG_BLOCK+0..1 -- strip, a vmu_ram offset in work RAM (0x100 or more), as every caller's is
; | Returns: ACC -- the symbol. Destroys B, C, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear; three bytes of stack.
;----------------------
g_strip_next_symbol:
	CALLF g_strip_next_index
	SET1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST B
	LD VTRBF
	CLR1 VSEL, 4
	XCH B
	ADD C
	ST VRMAD1
	LD B
	ADDC #$FF
	ST VRMAD2
	LD VTRBF
	RET

;----------------------
; | g_reel_random_pos
; | Description: game_reel_random_pos: a random strip index below len times 0x280, the reel position of that symbol, one 16x8 MUL.
; | Author: suinevere
; | Dependencies: g_strip_roll
; | Globals: seed
; | Params: ACC -- len (strips hold at most 16 symbols)
; | Returns: TRH:TRL -- the position. Destroys ACC, B, C, PSW; six bytes of stack.
;----------------------
g_reel_random_pos:
	CALLF g_strip_roll
	ST B
	MOV #$02, ACC
	MOV #$80, C
	MUL
	ST TRH
	LD C
	ST TRL
	RET

;----------------------
; | g_reels_step
; | Description: game_reels_step: one frame of the reels, steering the spin's phase and then turning the reels.
; | Author: suinevere
; | Dependencies: g_reels_steer, g_reels_turn
; | Globals: as g_reels_steer and g_reels_turn
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, PSW.
;----------------------
g_reels_step:
	CALLF g_reels_steer
	JMPF g_reels_turn

;----------------------
; | g_reels_next_two
; | Description: game_reels_next_two: the symbols after the first two reels' stops, strip_a's into scratch_kinds and strip_b's into scratch_first.
; | Author: suinevere
; | Dependencies: g_strip_next_index, rrl_ram.inc (HW_REEL_LEN, HW_REEL_LEN_B, HW_REEL_POS_A, HW_REEL_POS_B, HWW_STRIP_A, HWW_STRIP_B, HW_SCRATCH_KINDS, HW_SCRATCH_FIRST)
; | Globals: reel_len, reel_len_b, reel_pos_a, reel_pos_b, strip_a, strip_b, scratch_kinds, scratch_first
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, VRMAD, PSW; three bytes of stack.
;----------------------
g_reels_next_two:
	LD HW_REEL_POS_A
	ST TRL
	LD HW_REEL_POS_A+1
	ST TRH
	LD HW_REEL_LEN
	CALLF g_strip_next_index
	LD C
	ADD #<HWW_STRIP_A
	ST VRMAD1
	MOV #$00, ACC
	ADDC #>HWW_STRIP_A
	ST VRMAD2
	LD VTRBF
	ST HW_SCRATCH_KINDS
	LD HW_REEL_POS_B
	ST TRL
	LD HW_REEL_POS_B+1
	ST TRH
	LD HW_REEL_LEN_B
	CALLF g_strip_next_index
	LD C
	ADD #<HWW_STRIP_B
	ST VRMAD1
	MOV #$00, ACC
	ADDC #>HWW_STRIP_B
	ST VRMAD2
	LD VTRBF
	ST HW_SCRATCH_FIRST
	RET

;----------------------
; | g_strip_shuffle
; | Description: game_strip_shuffle: runs g_strip_shuffle_core and stores its last first draw at last_roll.
; | Author: suinevere
; | Dependencies: g_strip_shuffle_core, game.inc (ARG_BLOCK)
; | Globals: seed
; | Params: ACC -- len; TRH:TRL -- strip, a vmu_ram offset in work RAM (strip_b, 0x1FB, from the one caller); ARG_BLOCK+0 -- last_roll's byte
; | Returns: TRH:TRL -- as g_strip_shuffle_core; ARG_BLOCK+0 -- the last pair's first draw. Destroys ACC, B, C, R0-R3, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_strip_shuffle:
	CALLF g_strip_shuffle_core
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD B
	ST VTRBF
	RET

;----------------------
; | g_strip_shuffle_core
; | Description: Eight draws of two random strip indexes below len, swapping the symbols at each pair that differs; the moved symbol is kept on the stack across the draws.
; | Author: suinevere
; | Dependencies: g_strip_roll
; | Globals: seed
; | Params: ACC -- len; TRH:TRL -- strip, a vmu_ram offset in work RAM
; | Returns: TRH:TRL -- the symbol the last swap took from the second draw's slot, or 0xFFFF when no pair swapped; B -- the last pair's first draw. Destroys ACC, C, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear; nine bytes of stack.
;----------------------
g_strip_shuffle_core:
	ST $00
	LD TRL
	ST $01
	LD TRH
	DEC ACC
	ST $02
	MOV #$08, $03
	CLR1 VSEL, 4
	MOV #$FF, ACC
	PUSH ACC
	PUSH ACC
g_strip_shuffle_draw:
	LD $00
	CALLF g_strip_roll
	PUSH ACC
	LD $00
	CALLF g_strip_roll
	ST C
	POP ACC
	ST B
	BE C, g_strip_shuffle_next
	LD $01
	ADD B
	ST VRMAD1
	LD $02
	ADDC #$00
	ST VRMAD2
	LD VTRBF
	ST TRL
	LD $01
	ADD C
	ST VRMAD1
	LD $02
	ADDC #$00
	ST VRMAD2
	LD VTRBF
	ST TRH
	LD TRL
	ST VTRBF
	LD $01
	ADD B
	ST VRMAD1
	LD $02
	ADDC #$00
	ST VRMAD2
	LD TRH
	ST VTRBF
	POP ACC
	POP ACC
	MOV #$00, ACC
	PUSH ACC
	PUSH TRH
g_strip_shuffle_next:
	DBNZ $03, g_strip_shuffle_draw
	POP TRL
	POP TRH
	RET

;----------------------
; | g_strip_pick_three
; | Description: game_strip_pick_three: the strip's symbol set in R0-R3, its count of symbols 0-9, three draws of set bits stored in draw order, then the picks sorted into scratch_kinds, scratch_first and scratch_count.
; | Author: suinevere
; | Dependencies: g_strip_roll, g_pt_scan, game.inc (ARG_BLOCK), rrl_ram.inc (HW_SCRATCH_KINDS, HW_SCRATCH_FIRST, HW_SCRATCH_COUNT)
; | Globals: seed, scratch_kinds, scratch_first, scratch_count
; | Params: ACC -- len (0 reads 256 bytes, as flagged in the C); TRH:TRL -- strip, a vmu_ram offset in work RAM (strip_a from the one caller); ARG_BLOCK+0..2 -- drawn
; | Returns: ACC -- how many distinct symbols 0-9 the strip holds; ARG_BLOCK+0..2 -- the picks in draw order. Hangs, as the C does, when a draw finds no set bit below 32. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear; eight bytes of stack.
;----------------------
g_strip_pick_three:
	ST C
	LD TRL
	ST VRMAD1
	LD TRH
	DEC ACC
	ST VRMAD2
	SET1 VSEL, 4
	MOV #$00, $00
	MOV #$00, $01
	MOV #$00, $02
	MOV #$00, $03
g_pt_symbol:
	LD VTRBF
	ST B
	SUB #$20
	BN PSW, 7, g_pt_symbol_next
	MOV #$01, TRH
	LD B
	AND #$07
	BZ g_pt_symbol_mask
	ST TRL
g_pt_symbol_shift:
	LD TRH
	ADD TRH
	ST TRH
	DBNZ TRL, g_pt_symbol_shift
g_pt_symbol_mask:
	LD B
	ROR
	ROR
	ROR
	AND #$03
	BNZ g_pt_symbol_1
	LD $00
	OR TRH
	ST $00
	BR g_pt_symbol_next
g_pt_symbol_1:
	BNE #$01, g_pt_symbol_2
	LD $01
	OR TRH
	ST $01
	BR g_pt_symbol_next
g_pt_symbol_2:
	BNE #$02, g_pt_symbol_3
	LD $02
	OR TRH
	ST $02
	BR g_pt_symbol_next
g_pt_symbol_3:
	LD $03
	OR TRH
	ST $03
g_pt_symbol_next:
	DBNZ C, g_pt_symbol
	MOV #$00, C
	LD $00
	ST TRL
	MOV #$08, TRH
g_pt_count:
	BN TRL, 0, g_pt_count_next
	INC C
g_pt_count_next:
	LD TRL
	ROR
	ST TRL
	DBNZ TRH, g_pt_count
	BN $01, 0, g_pt_count_9
	INC C
g_pt_count_9:
	BN $01, 1, g_pt_count_end
	INC C
g_pt_count_end:
	PUSH C
	PUSH C
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
g_pt_draw:
	POP ACC
	PUSH ACC
	CALLF g_strip_roll
	ST C
	MOV #$00, B
	LD $00
	CALLF g_pt_scan
	BP PSW, 7, g_pt_found_0
	LD $01
	CALLF g_pt_scan
	BP PSW, 7, g_pt_found_1
	LD $02
	CALLF g_pt_scan
	BP PSW, 7, g_pt_found_2
	LD $03
	CALLF g_pt_scan
	BP PSW, 7, g_pt_found_3
g_pt_spin:
	BR g_pt_spin
g_pt_found_0:
	LD $00
	XOR TRH
	ST $00
	BR g_pt_found
g_pt_found_1:
	LD $01
	XOR TRH
	ST $01
	BR g_pt_found
g_pt_found_2:
	LD $02
	XOR TRH
	ST $02
	BR g_pt_found
g_pt_found_3:
	LD $03
	XOR TRH
	ST $03
g_pt_found:
	LD B
	ST VTRBF
	POP ACC
	DEC ACC
	PUSH ACC
	LD VRMAD1
	BNE #<(ARG_BLOCK+3), g_pt_draw
	POP ACC
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST B
	LD VTRBF
	ST C
	LD VTRBF
	ST TRL
	CLR1 VSEL, 4
	LD B
	SUB C
	BP PSW, 7, g_pt_sort_2
	LD B
	XCH C
	ST B
g_pt_sort_2:
	LD B
	SUB TRL
	BP PSW, 7, g_pt_sort_3
	LD B
	XCH TRL
	ST B
g_pt_sort_3:
	LD C
	SUB TRL
	BP PSW, 7, g_pt_sort_put
	LD C
	XCH TRL
	ST C
g_pt_sort_put:
	LD B
	ST HW_SCRATCH_KINDS
	LD C
	ST HW_SCRATCH_FIRST
	LD TRL
	ST HW_SCRATCH_COUNT
	POP ACC
	RET

;----------------------
; | g_pt_scan
; | Description: g_strip_pick_three's bit search over one byte of the symbol set: each set bit either ends the search, when n is 0, or counts n down; B counts the bits passed.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- the set's byte; B -- the bit number of its bit 0; C -- n
; | Returns: CY set with B the bit and TRH its mask when found, else CY clear with B 8 higher; C -- n left. Destroys ACC, TRL, TRH.
;----------------------
g_pt_scan:
	ST TRL
	MOV #$01, TRH
g_pt_scan_bit:
	LD TRL
	AND TRH
	BZ g_pt_scan_next
	LD C
	BZ g_pt_scan_found
	DEC C
g_pt_scan_next:
	INC B
	LD TRH
	ADD TRH
	ST TRH
	BNZ g_pt_scan_bit
	CLR1 PSW, 7
	RET
g_pt_scan_found:
	SET1 PSW, 7
	RET

;----------------------
; | ROM_STRIPS / REEL_STRIPS
; | Description: The strip table's file offset (rrl_rom.h ROM_STRIPS, a card address per strip, each strip ending where the next begins) and the three loaded strips' vmu_ram offset (reel.c REEL_STRIPS, strip_a).
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/rrl_rom.h, pocketstation/src/vmu/reel.c, rrl_ram.inc (HWW_STRIP_A)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_STRIPS	EQU	$9E00
REEL_STRIPS	EQU	HWW_STRIP_A+$100

;----------------------
; | g_strip_load
; | Description: game_strip_load: copies strip strip's bytes, a halfword at a time, to dst, or with add nonzero writes them plus add capped at 9 through g_add_capped_core; returns the strip's length.
; | Author: suinevere
; | Dependencies: g_add_capped_core, app_tables, ROM_STRIPS, game.inc (ARG_BLOCK)
; | Globals: strip_a, strip_b, strip_c
; | Params: TRH:TRL -- strip (callers pass 0-3, 4-7 and 10 up); ARG_BLOCK+0..1 -- dst, a vmu_ram offset in work RAM; ACC -- add
; | Returns: TRH:TRL -- the length in bytes. Destroys ACC, B, C, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; nineteen bytes of stack on the add path.
;----------------------
g_strip_load:
	ST B
	LD TRL
	ADD TRL
	ST TRL
	LD TRH
	ADDC TRH
	ST TRH
	LD TRL
	ADD TRL
	ST TRL
	LD TRH
	ADDC TRH
	ST TRH
	LD TRL
	ADD #<(app_tables+ROM_STRIPS)
	ST TRL
	LD TRH
	ADDC #>(app_tables+ROM_STRIPS)
	ST TRH
	MOV #$00, ACC
	LDC
	ST $00
	MOV #$01, ACC
	LDC
	ST $01
	MOV #$04, ACC
	LDC
	SUB $00
	ST C
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST $02
	INC VRMAD1
	LD VTRBF
	ST $03
	LD C
	PUSH ACC
	LD $00
	ST TRL
	LD $01
	ST TRH
	LD B
	BZ g_strip_load_copy
	MOV #$01, C
	POP ACC
	PUSH ACC
	CALLF g_add_capped_core
	BR g_strip_load_out
g_strip_load_copy:
	LD $02
	ST VRMAD1
	LD $03
	SUB #$01
	ST VRMAD2
	LD TRL
	ADD #<app_tables
	ST TRL
	LD TRH
	ADDC #>app_tables
	ST TRH
	POP ACC
	PUSH ACC
	ST B
	MOV #$00, C
g_strip_load_byte:
	LD C
	LDC
	ST VTRBF
	INC VRMAD1
	LD VRMAD1
	BNZ g_strip_load_page
	INC VRMAD2
g_strip_load_page:
	INC C
	DBNZ B, g_strip_load_byte
g_strip_load_out:
	POP ACC
	ST TRL
	MOV #$00, TRH
	RET

;----------------------
; | g_reels_load_raised
; | Description: game_reels_load_raised: reel A's strip becomes strip_c plus lap_mark, each symbol capped at 9, in the five-reel layout, stopped at position 0.
; | Author: suinevere
; | Dependencies: g_add_capped_core, REEL_STRIPS, rrl_ram.inc (HW_STRIP_C_LEN, HW_REEL_PHASE, HW_REEL_COUNT, HW_REEL_LEN, HW_LAP_MARK, HW_REEL_POS_A, HW_REEL_SPEED_A)
; | Globals: strip_a, strip_c, reel_phase, reel_count, reel_len, reel_pos_a, reel_speed_a
; | Params: N/A (strip_c_len 1-255)
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reels_load_raised:
	MOV #$00, HW_REEL_PHASE
	MOV #$05, HW_REEL_COUNT
	LD HW_STRIP_C_LEN
	ST HW_REEL_LEN
	PUSH ACC
	LD HW_LAP_MARK
	ST B
	POP ACC
	CALLF g_reel_a_add
	MOV #$00, HW_REEL_POS_A
	MOV #$00, HW_REEL_POS_A+1
	MOV #$00, HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	RET

;----------------------
; | g_reel_a_add
; | Description: Writes strip_c plus add, each capped at 9, over reel A's strip at REEL_STRIPS, for g_reels_load_raised and g_reel_a_raised.
; | Author: suinevere
; | Dependencies: g_add_capped_core, REEL_STRIPS
; | Globals: strip_a, strip_c
; | Params: ACC -- len, 1-255; B -- add
; | Returns: N/A. Keeps B; destroys ACC, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reel_a_add:
	MOV #<(REEL_STRIPS+$20), TRL
	MOV #>(REEL_STRIPS+$20), TRH
	MOV #<REEL_STRIPS, $02
	MOV #>REEL_STRIPS, $03
	MOV #$00, C
	JMPF g_add_capped_core

;----------------------
; | g_reel_a_raised
; | Description: reel_a_raised: reel A's strip becomes strip_c plus add capped at 9, reel_len strip_c_len, at a random position; returns the length.
; | Author: suinevere
; | Dependencies: g_reel_a_add, g_reel_random_pos, rrl_ram.inc (HW_STRIP_C_LEN, HW_REEL_LEN, HW_REEL_POS_A)
; | Globals: seed, strip_a, strip_c, reel_len, reel_pos_a
; | Params: B -- add's low byte, which game_add_capped takes
; | Returns: ACC -- the length. Destroys B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reel_a_raised:
	LD HW_STRIP_C_LEN
	ST HW_REEL_LEN
	CALLF g_reel_a_add
	LD HW_STRIP_C_LEN
	CALLF g_reel_random_pos
	LD TRL
	ST HW_REEL_POS_A
	LD TRH
	ST HW_REEL_POS_A+1
	LD HW_STRIP_C_LEN
	RET
