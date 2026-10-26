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
; | Dependencies: g_add_capped_core, app_tables, ROM_STRIPS, game.inc (ARG_BLOCK), g_tr_x4
; | Globals: strip_a, strip_b, strip_c
; | Params: TRH:TRL -- strip (callers pass 0-3, 4-7 and 10 up); ARG_BLOCK+0..1 -- dst, a vmu_ram offset in work RAM; ACC -- add
; | Returns: TRH:TRL -- the length in bytes. Destroys ACC, B, C, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; nineteen bytes of stack on the add path.
;----------------------
g_strip_load:
	ST B
	CALLF g_tr_x4
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

;----------------------
; | g_rl_strip
; | Description: Loads strip TRH:TRL into REEL_STRIPS + B through g_strip_load with add ACC, setting ARG_BLOCK's dst for it.
; | Author: suinevere
; | Dependencies: g_strip_load, REEL_STRIPS, game.inc (ARG_BLOCK)
; | Globals: strip_a and the strips after it
; | Params: TRH:TRL -- strip; B -- the dst offset from REEL_STRIPS (0, $10 or $20); ACC -- add
; | Returns: TRH:TRL -- the length in bytes. Keeps ARG_BLOCK; destroys ACC, B, C, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_rl_strip:
	ST C
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH VTRBF
	LD B
	ADD #<REEL_STRIPS
	ST VTRBF
	INC VRMAD1
	PUSH VTRBF
	MOV #>REEL_STRIPS, ACC
	ADDC #$00
	ST VTRBF
	LD C
	CALLF g_strip_load
	MOV #<(ARG_BLOCK+1), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	RET

;----------------------
; | g_reels_load
; | Description: game_reels_load: the three-reel layout, strips 0-2 loaded into strips a-c, every reel at position 0x800 and stopped.
; | Author: suinevere
; | Dependencies: g_rl_strip, rrl_ram.inc (HW_REEL_PHASE, HW_REEL_COUNT, HW_REEL_LEN, HW_REEL_POS_A, HW_REEL_POS_B, HW_REEL_POS_C, HW_REEL_SPEED_A, HW_REEL_SPEED_B, HW_REEL_SPEED_C)
; | Globals: strip_a and the strips after it, reel_phase, reel_count, reel_len, reel_pos_a-c, reel_speed_a-c
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reels_load:
	MOV #$00, HW_REEL_PHASE
	MOV #$03, HW_REEL_COUNT
	MOV #$00, TRL
	MOV #$00, TRH
	MOV #$00, B
	MOV #$00, ACC
	CALLF g_rl_strip
	LD TRL
	ST HW_REEL_LEN
	MOV #$01, TRL
	MOV #$00, TRH
	MOV #$10, B
	MOV #$00, ACC
	CALLF g_rl_strip
	MOV #$02, TRL
	MOV #$00, TRH
	MOV #$20, B
	MOV #$00, ACC
	CALLF g_rl_strip
	MOV #$00, HW_REEL_POS_A
	MOV #$08, HW_REEL_POS_A+1
	MOV #$00, HW_REEL_POS_B
	MOV #$08, HW_REEL_POS_B+1
	MOV #$00, HW_REEL_POS_C
	MOV #$08, HW_REEL_POS_C+1
	MOV #$00, HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	MOV #$00, HW_REEL_SPEED_B
	MOV #$00, HW_REEL_SPEED_B+1
	MOV #$00, HW_REEL_SPEED_C
	MOV #$00, HW_REEL_SPEED_C+1
	RET

;----------------------
; | g_reels_load_strip3
; | Description: game_reels_load_strip3: the four-reel layout with strip 3 in strip_a, reel_len one short of its length, and an empty scratch pick list from 0x40.
; | Author: suinevere
; | Dependencies: g_rl_strip, rrl_ram.inc (HW_REEL_PHASE, HW_REEL_COUNT, HW_REEL_LEN, HW_SCRATCH_FIRST, HW_SCRATCH_COUNT)
; | Globals: strip_a, reel_phase, reel_count, reel_len, scratch_first, scratch_count
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reels_load_strip3:
	MOV #$00, HW_REEL_PHASE
	MOV #$04, HW_REEL_COUNT
	MOV #$03, TRL
	MOV #$00, TRH
	MOV #$00, B
	MOV #$00, ACC
	CALLF g_rl_strip
	LD TRL
	DEC ACC
	ST HW_REEL_LEN
	MOV #$40, HW_SCRATCH_FIRST
	MOV #$00, HW_SCRATCH_COUNT
	RET

;----------------------
; | g_reels_load_single
; | Description: game_reels_load_single: a held walk of kind 2, no reels sliding, and reel A raised by lap_mark and stopped, with reel B empty.
; | Author: suinevere
; | Dependencies: g_reel_a_raised, rrl_ram.inc (HW_WALK_BUSY, HW_WALK_KIND, HW_WALK_FRAME, HW_REEL_PHASE, HW_REEL_COUNT, HW_REEL_SLIDE, HW_LAP_MARK, HW_REEL_SPEED_A, HW_REEL_LEN_B)
; | Globals: seed, strip_a, strip_c, walk_busy, walk_kind, walk_frame, reel_phase, reel_count, reel_slide, reel_len, reel_pos_a, reel_speed_a, reel_len_b
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reels_load_single:
	MOV #$01, HW_WALK_BUSY
	MOV #$02, HW_WALK_KIND
	MOV #$00, HW_WALK_FRAME
	MOV #$00, HW_REEL_PHASE
	MOV #$00, HW_REEL_COUNT
	MOV #$00, HW_REEL_SLIDE
	LD HW_LAP_MARK
	ST B
	CALLF g_reel_a_raised
	MOV #$00, HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	MOV #$00, HW_REEL_LEN_B
	RET

;----------------------
; | g_reels_load_pick
; | Description: game_reels_load_pick: the one-reel pick layout, reel A raised by lap_mark, then three symbols drawn from it; returns how many distinct symbols it holds.
; | Author: suinevere
; | Dependencies: g_reel_a_raised, g_strip_pick_three, REEL_STRIPS, rrl_ram.inc (HW_REEL_PHASE, HW_REEL_COUNT, HW_LAP_MARK, HW_REEL_LEN, HW_REEL_SPEED_A, HW_REEL_LEN_B)
; | Globals: seed, strip_a, strip_c, reel_phase, reel_count, reel_len, reel_pos_a, scratch_kinds, scratch_first, scratch_count, reel_speed_a, reel_len_b
; | Params: ARG_BLOCK+0..2 -- drawn
; | Returns: ACC -- the distinct symbol count; ARG_BLOCK+0..2 -- the picks in draw order. Destroys B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reels_load_pick:
	MOV #$01, HW_REEL_PHASE
	MOV #$01, HW_REEL_COUNT
	LD HW_LAP_MARK
	ST B
	CALLF g_reel_a_raised
	LD HW_REEL_LEN
	MOV #<REEL_STRIPS, TRL
	MOV #>REEL_STRIPS, TRH
	CALLF g_strip_pick_three
	MOV #$00, HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	MOV #$00, HW_REEL_LEN_B
	RET

;----------------------
; | g_reels_load_pair
; | Description: game_reels_load_pair: the two-reel layout, reel A raised by lap_mark plus the bag kinds (none for two), strip scratch_count + 8 shuffled into strip_b at a random position.
; | Author: suinevere
; | Dependencies: g_reel_a_raised, g_rl_strip, g_strip_shuffle, g_reel_random_pos, REEL_STRIPS, rrl_ram.inc (HW_BAG_KIND_COUNT, HW_REEL_PHASE, HW_REEL_COUNT, HW_LAP_MARK, HW_SCRATCH_COUNT, HW_REEL_LEN_B, HW_REEL_POS_B, HW_REEL_SPEED_A, HW_REEL_SPEED_B)
; | Globals: seed, strip_a, strip_b, strip_c, bag_kind_count, reel_phase, reel_count, reel_len, reel_pos_a, scratch_count, reel_len_b, reel_pos_b, reel_speed_a, reel_speed_b
; | Params: ARG_BLOCK+0 -- last_roll
; | Returns: TRH:TRL -- the last value moved, or -1; ARG_BLOCK+0 -- the last first draw. Destroys ACC, B, C, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reels_load_pair:
	LD HW_BAG_KIND_COUNT
	PUSH ACC
	MOV #$00, HW_REEL_PHASE
	MOV #$02, HW_REEL_COUNT
	BNE #$02, g_reels_load_pair_add
	MOV #$00, ACC
g_reels_load_pair_add:
	ADD HW_LAP_MARK
	ST B
	CALLF g_reel_a_raised
	LD HW_SCRATCH_COUNT
	ADD #$08
	ST TRL
	MOV #$00, ACC
	ADDC #$00
	ST TRH
	POP ACC
	CLR1 PSW, 7
	RORC
	MOV #$10, B
	CALLF g_rl_strip
	LD TRL
	ST HW_REEL_LEN_B
	PUSH ACC
	MOV #<(REEL_STRIPS+$10), TRL
	MOV #>(REEL_STRIPS+$10), TRH
	CALLF g_strip_shuffle
	POP B
	PUSH TRL
	PUSH TRH
	LD B
	CALLF g_reel_random_pos
	LD TRL
	ST HW_REEL_POS_B
	LD TRH
	ST HW_REEL_POS_B+1
	POP TRH
	POP TRL
	MOV #$00, HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	MOV #$00, HW_REEL_SPEED_B
	MOV #$00, HW_REEL_SPEED_B+1
	RET

;----------------------
; | PAYOUT_ONE / PAYOUT_TEN / PAYOUT_TAIL / PAYOUT_LEFT / PAYOUT_RIGHT / POINTS_MAX
; | Description: reel.c's payout marker types and x offsets, and the points cap 99999.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/reel.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
PAYOUT_ONE	EQU	$67
PAYOUT_TEN	EQU	$66
PAYOUT_TAIL	EQU	$68
PAYOUT_LEFT	EQU	$F9
PAYOUT_RIGHT	EQU	8
POINTS_MAX	EQU	99999

;----------------------
; | g_payout
; | Description: game_payout: shows the win's markers, adds unit or ten units per reel, times the lap multiplier, to points capped at 99999, and records the win in scratch_lo/hi and lap_points.
; | Author: suinevere
; | Dependencies: g_markers_clear, g_marker_start, PAYOUT_*, POINTS_MAX, game.inc (ARG_BLOCK), rrl_ram.inc (HW_SCRATCH_COUNT, HW_STEPS, HW_SCRATCH_LO, HW_SCRATCH_HI, HW_LAP_POINTS, HWW_PAYOUT_UNIT, HWW_POINTS)
; | Globals: marker, scratch_count, payout_unit, steps, points, scratch_lo, scratch_hi, lap_points
; | Params: N/A
; | Returns: ARG_BLOCK+0..3 -- the win, little-endian. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_payout:
	CALLF g_markers_clear
	LD HW_SCRATCH_COUNT
	BE #$01, g_payout_pair
	BE #$03, g_payout_pair
	BE #$04, g_payout_pair
	MOV #PAYOUT_ONE, B
	BNE #$02, g_payout_one
	MOV #PAYOUT_TEN, B
g_payout_one:
	MOV #$00, C
	MOV #$00, ACC
	CALLF g_marker_start
	MOV #$01, ACC
	MOV #PAYOUT_TAIL, B
	MOV #$00, C
	CALLF g_marker_start
	BR g_payout_gain
g_payout_pair:
	MOV #PAYOUT_ONE, B
	BNE #$04, g_payout_left
	MOV #PAYOUT_TEN, B
g_payout_left:
	MOV #PAYOUT_LEFT, C
	MOV #$00, ACC
	CALLF g_marker_start
	MOV #$01, ACC
	MOV #PAYOUT_TAIL, B
	MOV #PAYOUT_LEFT, C
	CALLF g_marker_start
	LD HW_SCRATCH_COUNT
	MOV #PAYOUT_TEN, B
	BNE #$01, g_payout_right
	MOV #PAYOUT_ONE, B
g_payout_right:
	MOV #PAYOUT_RIGHT, C
	MOV #$02, ACC
	CALLF g_marker_start
	MOV #$03, ACC
	MOV #PAYOUT_TAIL, B
	MOV #PAYOUT_RIGHT, C
	CALLF g_marker_start
g_payout_gain:
	CLR1 VSEL, 4
	MOV #<HWW_PAYOUT_UNIT, VRMAD1
	MOV #>HWW_PAYOUT_UNIT, VRMAD2
	LD VTRBF
	ST $00
	ST C
	MOV #10, B
	MOV #$00, ACC
	MUL
	ST $03
	LD C
	ST $02
	LD HW_SCRATCH_COUNT
	BE #$01, g_payout_k1
	BE #$02, g_payout_k2
	BE #$03, g_payout_k3
	BE #$04, g_payout_k4
	LD $00
	ST TRL
	MOV #$00, TRH
	BR g_payout_times
g_payout_k1:
	LD $00
	ADD $00
	ST TRL
	MOV #$00, ACC
	ADDC #$00
	ST TRH
	BR g_payout_times
g_payout_k2:
	LD $02
	ST TRL
	LD $03
	ST TRH
	BR g_payout_times
g_payout_k3:
	LD $00
	ADD $02
	ST TRL
	LD $03
	ADDC #$00
	ST TRH
	BR g_payout_times
g_payout_k4:
	LD $02
	ADD $02
	ST TRL
	LD $03
	ADDC $03
	ST TRH
g_payout_times:
	MOV #$01, B
	LD HW_STEPS
	AND #$E0
	BZ g_payout_mul
	MOV #$02, B
	LD HW_STEPS
	AND #$C0
	BZ g_payout_mul
	MOV #$03, B
g_payout_mul:
	LD TRL
	ST C
	LD TRH
	MUL
	ST TRH
	LD C
	ST TRL
	MOV #<HWW_POINTS, VRMAD1
	MOV #>HWW_POINTS, VRMAD2
	LD VTRBF
	ADD TRL
	ST $00
	INC VRMAD1
	LD VTRBF
	ADDC TRH
	ST $01
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST $02
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST $03
	LD $00
	SUB #<POINTS_MAX
	LD $01
	SUBC #>POINTS_MAX
	LD $02
	SUBC #$01
	LD $03
	SUBC #$00
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	BP PSW, 7, g_payout_small
	MOV #<POINTS_MAX, ACC
	SUB $00
	ST VTRBF
	ST TRL
	INC VRMAD1
	MOV #>POINTS_MAX, ACC
	SUBC $01
	ST VTRBF
	ST TRH
	INC VRMAD1
	MOV #$01, ACC
	SUBC $02
	ST VTRBF
	INC VRMAD1
	MOV #$00, ACC
	SUBC $03
	ST VTRBF
	MOV #<POINTS_MAX, $00
	MOV #>POINTS_MAX, $01
	MOV #$01, $02
	MOV #$00, $03
	BR g_payout_store
g_payout_small:
	LD TRL
	ST VTRBF
	INC VRMAD1
	LD TRH
	ST VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
g_payout_store:
	MOV #<HWW_POINTS, VRMAD1
	MOV #>HWW_POINTS, VRMAD2
	LD $00
	ST VTRBF
	INC VRMAD1
	LD $01
	ST VTRBF
	INC VRMAD1
	LD $02
	ST VTRBF
	INC VRMAD1
	LD $03
	ST VTRBF
	LD TRL
	ST HW_SCRATCH_LO
	ADD HW_LAP_POINTS
	ST HW_LAP_POINTS
	LD TRH
	ST HW_SCRATCH_HI
	ADDC HW_LAP_POINTS+1
	ST HW_LAP_POINTS+1
	RET

;----------------------
; | g_mod640
; | Description: TRH:TRL modulo 640, as the reel steps take it from game_divide: 640 is 5 * 128, so one DIV of pos >> 7 by 5.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRH:TRL -- pos
; | Returns: TRH:TRL -- pos % 640. Destroys ACC, B, C, PSW; one byte of stack.
;----------------------
g_mod640:
	LD TRL
	AND #$7F
	PUSH ACC
	LD TRH
	ROL
	AND #$FE
	ST C
	LD TRL
	ROL
	AND #$01
	OR C
	ST C
	LD TRH
	ROL
	AND #$01
	MOV #$05, B
	DIV
	LD B
	ROR
	AND #$80
	ST C
	POP ACC
	OR C
	ST TRL
	LD B
	CLR1 PSW, 7
	RORC
	ST TRH
	RET

;----------------------
; | g_reel_back
; | Description: Moves a reel position back by a speed, wrapping by the strip's length times 640; only the low sixteen bits of that stride matter to the stored position.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRH:TRL -- pos; R1:R0 (0x01:0x00) -- speed; ACC -- len
; | Returns: TRH:TRL -- the new position. Destroys ACC, B, C, PSW.
;----------------------
g_reel_back:
	ST B
	LD TRL
	SUB $00
	ST TRL
	LD TRH
	SUBC $01
	ST TRH
	BN PSW, 7, g_reel_back_ret
	MOV #$80, C
	MOV #$02, ACC
	MUL
	ST B
	LD TRL
	ADD C
	ST TRL
	LD TRH
	ADDC B
	ST TRH
g_reel_back_ret:
	RET

;----------------------
; | g_reel_spin_step
; | Description: game_reel_spin_step: one frame of the single reel: slide in or out, speed up, hold, a random delay, slow down and stop on a symbol; then turns reel A back by its speed.
; | Author: suinevere
; | Dependencies: g_strip_next_symbol, g_sound_begin, g_random, g_reel_a_turn, game.inc (ARG_BLOCK), rrl_ram.inc (HW_REEL_PHASE, HW_REEL_SLIDE, HW_REEL_LEN, HW_REEL_POS_A, HW_REEL_SPEED_A, HW_REEL_DELAY, HW_REEL_COUNT, HW_WALK_LEFT, HW_WALK_BACK, HW_WALK_KIND, HW_MODE_WAIT, HW_BTN_NEW)
; | Globals: the fields above, seed, strip_a and what g_sound_begin writes
; | Params: N/A
; | Returns: N/A. Keeps ARG_BLOCK; destroys ACC, B, C, TRH, TRL, R0, R1, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reel_spin_step:
	LD HW_REEL_PHASE
	BNZ g_reel_spin_step_7
	INC HW_REEL_SLIDE
	LD HW_REEL_SLIDE
	BNE #12, g_reel_spin_step_t1
	JMPF g_reel_spin_step_next
g_reel_spin_step_t1:
	JMPF g_reel_a_turn
g_reel_spin_step_7:
	BNE #$07, g_reel_spin_step_1
	DEC HW_REEL_SLIDE
	LD HW_REEL_SLIDE
	BNZ g_reel_spin_step_t2
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
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
	ST HW_WALK_LEFT
	MOV #<(ARG_BLOCK+1), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	MOV #$00, HW_WALK_BACK
	JMPF g_reel_spin_step_next
g_reel_spin_step_t2:
	JMPF g_reel_a_turn
g_reel_spin_step_1:
	BNE #$01, g_reel_spin_step_2
	LD HW_REEL_SPEED_A
	ADD #$04
	ST C
	LD HW_REEL_SPEED_A+1
	ADDC #$00
	BNZ g_reel_spin_step_top
	LD C
	AND #$80
	BNZ g_reel_spin_step_top
	LD C
	ST HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	JMPF g_reel_a_turn
g_reel_spin_step_top:
	MOV #$80, HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	MOV #<300, HW_MODE_WAIT
	MOV #>300, HW_MODE_WAIT+1
	JMPF g_reel_spin_step_next
g_reel_spin_step_2:
	BNE #$02, g_reel_spin_step_3
	LD HW_BTN_NEW
	AND #$01
	BZ g_reel_spin_step_count
	MOV #$03, ACC
	CALLF g_sound_begin
	BR g_reel_spin_step_delay
g_reel_spin_step_count:
	LD HW_MODE_WAIT
	SUB #$01
	ST HW_MODE_WAIT
	LD HW_MODE_WAIT+1
	SUBC #$00
	ST HW_MODE_WAIT+1
	OR HW_MODE_WAIT
	BNZ g_reel_spin_step_t3
g_reel_spin_step_delay:
	MOV #$1E, TRL
	MOV #$00, TRH
	CALLF g_random
	LD TRL
	SUB #15
	BZ g_reel_spin_step_one
	BN PSW, 7, g_reel_spin_step_set
g_reel_spin_step_one:
	MOV #$01, ACC
g_reel_spin_step_set:
	ST HW_REEL_DELAY
	JMPF g_reel_spin_step_next
g_reel_spin_step_t3:
	JMPF g_reel_a_turn
g_reel_spin_step_3:
	BNE #$03, g_reel_spin_step_4
	DEC HW_REEL_DELAY
	LD HW_REEL_DELAY
	BNZ g_reel_spin_step_t4
	JMPF g_reel_spin_step_next
g_reel_spin_step_t4:
	JMPF g_reel_a_turn
g_reel_spin_step_4:
	BNE #$04, g_reel_spin_step_6
	LD HW_REEL_SPEED_A+1
	BNZ g_reel_spin_step_slow
	LD HW_REEL_SPEED_A
	SUB #$31
	BP PSW, 7, g_reel_spin_step_n5
g_reel_spin_step_slow:
	LD HW_REEL_SPEED_A
	SUB #$06
	ST HW_REEL_SPEED_A
	LD HW_REEL_SPEED_A+1
	SUBC #$00
	ST HW_REEL_SPEED_A+1
	JMPF g_reel_a_turn
g_reel_spin_step_n5:
	JMPF g_reel_spin_step_next
g_reel_spin_step_6:
	BNE #$06, g_reel_spin_step_t6
	LD HW_BTN_NEW
	AND #$01
	BNZ g_reel_spin_step_end
	DEC HW_MODE_WAIT
	LD HW_MODE_WAIT
	BNZ g_reel_spin_step_t7
g_reel_spin_step_end:
	LD HW_REEL_COUNT
	BZ g_reel_spin_step_walk
	INC HW_REEL_PHASE
	JMPF g_reel_spin_step_next
g_reel_spin_step_t6:
	JMPF g_reel_a_turn
g_reel_spin_step_t7:
	JMPF g_reel_a_turn
g_reel_spin_step_walk:
	MOV #$03, HW_WALK_KIND
g_reel_spin_step_next:
	INC HW_REEL_PHASE
;----------------------
; | g_reel_a_turn
; | Description: The tail of game_reel_spin_step: turns reel A back by its speed, in phase 5 stopping on a symbol boundary with a 0x5A-frame wait.
; | Author: suinevere
; | Dependencies: g_mod640, g_reel_back, rrl_ram.inc (HW_REEL_SPEED_A, HW_REEL_POS_A, HW_REEL_PHASE, HW_REEL_LEN, HW_MODE_WAIT)
; | Globals: reel_speed_a, reel_pos_a, reel_phase, reel_len, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0, R1, PSW.
;----------------------
g_reel_a_turn:
	LD HW_REEL_SPEED_A
	ST $00
	LD HW_REEL_SPEED_A+1
	ST $01
	OR $00
	BNZ g_reel_a_turn_move
	RET
g_reel_a_turn_move:
	LD HW_REEL_POS_A
	ST TRL
	LD HW_REEL_POS_A+1
	ST TRH
	LD HW_REEL_PHASE
	BNE #$05, g_reel_a_turn_back
	CALLF g_mod640
	LD $00
	SUB TRL
	LD $01
	SUBC TRH
	BP PSW, 7, g_reel_a_turn_pos
	MOV #$06, HW_REEL_PHASE
	MOV #$00, HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	MOV #$5A, HW_MODE_WAIT
	LD TRL
	ST $00
	LD TRH
	ST $01
g_reel_a_turn_pos:
	LD HW_REEL_POS_A
	ST TRL
	LD HW_REEL_POS_A+1
	ST TRH
g_reel_a_turn_back:
	LD HW_REEL_LEN
	CALLF g_reel_back
	LD TRL
	ST HW_REEL_POS_A
	LD TRH
	ST HW_REEL_POS_A+1
	RET

;----------------------
; | g_reel_pair_spin_step
; | Description: game_reel_pair_spin_step: one frame of the two reels' spin: speed each up, hold, a random delay, slow both, then wait; g_reel_pair_turn moves them.
; | Author: suinevere
; | Dependencies: g_wait16_done, g_sound_begin, g_random, g_reel_pair_turn, rrl_ram.inc (HW_REEL_PHASE, HW_REEL_SPEED_A, HW_REEL_SPEED_B, HW_REEL_DELAY, HW_MODE_WAIT, HW_BTN_NEW)
; | Globals: the fields above, seed and what g_sound_begin and g_reel_pair_turn write
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, PSW.
;----------------------
g_reel_pair_spin_step:
	LD HW_REEL_PHASE
	BNZ g_reel_pair_spin_step_2
	LD HW_REEL_SPEED_A
	ADD #$04
	ST C
	LD HW_REEL_SPEED_A+1
	ADDC #$00
	BNZ g_reel_pair_spin_step_atop
	LD C
	AND #$80
	BNZ g_reel_pair_spin_step_atop
	LD C
	ST HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	JMPF g_reel_pair_turn
g_reel_pair_spin_step_atop:
	MOV #$80, HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
	MOV #10, HW_MODE_WAIT
	MOV #$00, HW_MODE_WAIT+1
	JMPF g_reel_pair_spin_step_next
g_reel_pair_spin_step_2:
	BNE #$02, g_reel_pair_spin_step_13
	LD HW_REEL_SPEED_B
	ADD #$06
	ST C
	LD HW_REEL_SPEED_B+1
	ADDC #$00
	BNZ g_reel_pair_spin_step_btop
	LD C
	SUB #$70
	BN PSW, 7, g_reel_pair_spin_step_btop
	LD C
	ST HW_REEL_SPEED_B
	MOV #$00, HW_REEL_SPEED_B+1
	JMPF g_reel_pair_turn
g_reel_pair_spin_step_btop:
	MOV #$70, HW_REEL_SPEED_B
	MOV #$00, HW_REEL_SPEED_B+1
	MOV #10, HW_MODE_WAIT
	MOV #$00, HW_MODE_WAIT+1
	JMPF g_reel_pair_spin_step_next
g_reel_pair_spin_step_13:
	BE #$01, g_reel_pair_spin_step_hold
	BNE #$03, g_reel_pair_spin_step_4
g_reel_pair_spin_step_hold:
	CALLF g_wait16_done
	BZ g_reel_pair_spin_step_turn
	MOV #<300, HW_MODE_WAIT
	MOV #>300, HW_MODE_WAIT+1
	JMPF g_reel_pair_spin_step_next
g_reel_pair_spin_step_turn:
	JMPF g_reel_pair_turn
g_reel_pair_spin_step_4:
	BNE #$04, g_reel_pair_spin_step_5
	LD HW_BTN_NEW
	AND #$01
	BZ g_reel_pair_spin_step_wait
	MOV #$03, ACC
	CALLF g_sound_begin
	BR g_reel_pair_spin_step_delay
g_reel_pair_spin_step_wait:
	CALLF g_wait16_done
	BZ g_reel_pair_spin_step_turn
g_reel_pair_spin_step_delay:
	MOV #$1E, TRL
	MOV #$00, TRH
	CALLF g_random
	LD TRL
	SUB #15
	BZ g_reel_pair_spin_step_one
	BN PSW, 7, g_reel_pair_spin_step_set
g_reel_pair_spin_step_one:
	MOV #$01, ACC
g_reel_pair_spin_step_set:
	ST HW_REEL_DELAY
	JMPF g_reel_pair_spin_step_next
g_reel_pair_spin_step_5:
	BNE #$05, g_reel_pair_spin_step_6
	DEC HW_REEL_DELAY
	LD HW_REEL_DELAY
	BNZ g_reel_pair_spin_step_tl
	JMPF g_reel_pair_spin_step_next
g_reel_pair_spin_step_6:
	BNE #$06, g_reel_pair_spin_step_7
	LD HW_REEL_SPEED_A+1
	BNZ g_reel_pair_spin_step_a6
	LD HW_REEL_SPEED_A
	SUB #$31
	BP PSW, 7, g_reel_pair_spin_step_b6
g_reel_pair_spin_step_a6:
	LD HW_REEL_SPEED_A
	SUB #$06
	ST HW_REEL_SPEED_A
	LD HW_REEL_SPEED_A+1
	SUBC #$00
	ST HW_REEL_SPEED_A+1
g_reel_pair_spin_step_b6:
	LD HW_REEL_SPEED_B+1
	BNZ g_reel_pair_spin_step_s6
	LD HW_REEL_SPEED_B
	SUB #$31
	BP PSW, 7, g_reel_pair_spin_step_tl
g_reel_pair_spin_step_s6:
	LD HW_REEL_SPEED_B
	SUB #$08
	ST HW_REEL_SPEED_B
	LD HW_REEL_SPEED_B+1
	SUBC #$00
	ST HW_REEL_SPEED_B+1
g_reel_pair_spin_step_tl:
	JMPF g_reel_pair_turn
g_reel_pair_spin_step_7:
	BNE #$07, g_reel_pair_spin_step_tl
	LD HW_BTN_NEW
	AND #$01
	BNZ g_reel_pair_spin_step_next
	DEC HW_MODE_WAIT
	LD HW_MODE_WAIT
	BNZ g_reel_pair_spin_step_tl
g_reel_pair_spin_step_next:
	INC HW_REEL_PHASE
	JMPF g_reel_pair_turn

;----------------------
; | g_reel_pair_turn
; | Description: The tail of game_reel_pair_spin_step: reel A turns back and reel B forward by their speeds, each stopping on a symbol boundary while slowing in phase 6; both stopped moves to phase 7.
; | Author: suinevere
; | Dependencies: g_mod640, g_reel_back, rrl_ram.inc (HW_REEL_PHASE, HW_REEL_SPEED_A, HW_REEL_SPEED_B, HW_REEL_POS_A, HW_REEL_POS_B, HW_REEL_LEN, HW_REEL_LEN_B, HW_MODE_WAIT)
; | Globals: the fields above
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, PSW.
;----------------------
g_reel_pair_turn:
	LD HW_REEL_SPEED_A
	ST $00
	LD HW_REEL_SPEED_A+1
	ST $01
	OR $00
	BZ g_reel_pair_turn_b
	LD HW_REEL_PHASE
	BE #$02, g_reel_pair_turn_b
	BE #$03, g_reel_pair_turn_b
	LD HW_REEL_POS_A
	ST TRL
	LD HW_REEL_POS_A+1
	ST TRH
	LD HW_REEL_PHASE
	BNE #$06, g_reel_pair_turn_aback
	LD $01
	BNZ g_reel_pair_turn_aback
	LD $00
	SUB #$31
	BN PSW, 7, g_reel_pair_turn_aback
	CALLF g_mod640
	LD $00
	SUB TRL
	LD $01
	SUBC TRH
	BP PSW, 7, g_reel_pair_turn_apos
	LD TRL
	ST $00
	LD TRH
	ST $01
	MOV #$00, HW_REEL_SPEED_A
	MOV #$00, HW_REEL_SPEED_A+1
g_reel_pair_turn_apos:
	LD HW_REEL_POS_A
	ST TRL
	LD HW_REEL_POS_A+1
	ST TRH
g_reel_pair_turn_aback:
	LD HW_REEL_LEN
	CALLF g_reel_back
	LD TRL
	ST HW_REEL_POS_A
	LD TRH
	ST HW_REEL_POS_A+1
g_reel_pair_turn_b:
	LD HW_REEL_SPEED_B
	ST $00
	LD HW_REEL_SPEED_B+1
	ST $01
	OR $00
	BNZ g_reel_pair_turn_bset
	JMPF g_reel_pair_turn_end
g_reel_pair_turn_bset:
	LD HW_REEL_PHASE
	SUB #$02
	BN PSW, 7, g_reel_pair_turn_bgo
	JMPF g_reel_pair_turn_end
g_reel_pair_turn_bgo:
	LD HW_REEL_PHASE
	BNE #$06, g_reel_pair_turn_fwd
	LD $01
	BNZ g_reel_pair_turn_fwd
	LD $00
	SUB #$31
	BN PSW, 7, g_reel_pair_turn_fwd
	LD HW_REEL_POS_B
	ST TRL
	LD HW_REEL_POS_B+1
	ST TRH
	CALLF g_mod640
	MOV #<640, ACC
	SUB TRL
	ST TRL
	MOV #>640, ACC
	SUBC TRH
	ST TRH
	LD $00
	SUB TRL
	LD $01
	SUBC TRH
	BP PSW, 7, g_reel_pair_turn_fwd
	LD TRL
	ST $00
	LD TRH
	ST $01
	MOV #$00, HW_REEL_SPEED_B
	MOV #$00, HW_REEL_SPEED_B+1
g_reel_pair_turn_fwd:
	LD HW_REEL_POS_B
	ADD $00
	ST TRL
	LD HW_REEL_POS_B+1
	ADDC $01
	ST TRH
	MOV #$00, ACC
	ADDC #$00
	ST $02
	LD HW_REEL_LEN_B
	ST B
	MOV #$80, C
	MOV #$02, ACC
	MUL
	ST $03
	LD TRL
	SUB C
	PUSH ACC
	LD TRH
	SUBC $03
	PUSH ACC
	LD $02
	SUBC B
	POP C
	POP B
	BP PSW, 7, g_reel_pair_turn_store
	LD B
	ST TRL
	LD C
	ST TRH
g_reel_pair_turn_store:
	LD TRL
	ST HW_REEL_POS_B
	LD TRH
	ST HW_REEL_POS_B+1
g_reel_pair_turn_end:
	LD HW_REEL_PHASE
	BNE #$06, g_reel_pair_turn_ret
	LD HW_REEL_SPEED_A
	OR HW_REEL_SPEED_A+1
	OR HW_REEL_SPEED_B
	OR HW_REEL_SPEED_B+1
	BNZ g_reel_pair_turn_ret
	MOV #$07, HW_REEL_PHASE
	MOV #$5A, HW_MODE_WAIT
g_reel_pair_turn_ret:
	RET

;----------------------
; | g_sym_bitmap
; | Description: For g_symbols_draw: table + 12 * the strip's symbol at index, with the strip at ARG_BLOCK+9..10 and the table at +11..12 and +6..7.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: the strip in work RAM
; | Params: ACC -- index
; | Returns: R0-R3 (0x00-0x03) -- the bitmap address, low byte first. Destroys ACC, B, C, VRMAD, PSW; keeps TRH and TRL.
;----------------------
g_sym_bitmap:
	CLR1 VSEL, 4
	ST $00
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<(ARG_BLOCK+10), VRMAD1
	LD VTRBF
	ST $01
	MOV #<(ARG_BLOCK+9), VRMAD1
	LD VTRBF
	ADD $00
	ST $00
	LD $01
	ADDC #$FF
	ST VRMAD2
	LD $00
	ST VRMAD1
	LD VTRBF
	ST C
	MOV #12, B
	MOV #$00, ACC
	MUL
	ST $01
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<(ARG_BLOCK+11), VRMAD1
	LD VTRBF
	ADD C
	ST $00
	INC VRMAD1
	LD VTRBF
	ADDC $01
	ST $01
	MOV #<(ARG_BLOCK+6), VRMAD1
	LD VTRBF
	ADDC #$00
	ST $02
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST $03
	RET

;----------------------
; | g_symbols_draw
; | Description: symbols_draw: draws the strip's symbols from index rightwards, wrapping at len, until x reaches 32; returns the last symbol's bitmap.
; | Author: suinevere
; | Dependencies: g_sym_bitmap, g_sprite_draw, game.inc (ARG_BLOCK)
; | Globals: the strip and what g_sprite_draw writes
; | Params: TRH:TRL -- x; ARG_BLOCK+0..1 -- y; ACC -- index; B -- len; ARG_BLOCK+2..3 -- strip, a vmu_ram offset in work RAM; ARG_BLOCK+4..7 -- table; C -- width; ARG_BLOCK+8 -- height
; | Returns: ARG_BLOCK+9..12 -- the last bitmap. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW and ARG_BLOCK+0..8; leaves VSEL bit 4 clear; five bytes of stack on top of g_sprite_draw's.
;----------------------
g_symbols_draw:
	PUSH ACC
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #$02, $00
g_symbols_draw_move:
	LD $00
	ADD #<ARG_BLOCK
	ST VRMAD1
	LD VTRBF
	PUSH ACC
	LD VRMAD1
	ADD #$07
	ST VRMAD1
	POP VTRBF
	INC $00
	LD $00
	BNE #$06, g_symbols_draw_move
	POP ACC
g_symbols_draw_loop:
	PUSH TRL
	PUSH TRH
	PUSH ACC
	PUSH B
	PUSH C
	CALLF g_sym_bitmap
	MOV #<(ARG_BLOCK+2), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD $00
	ST VTRBF
	INC VRMAD1
	LD $01
	ST VTRBF
	INC VRMAD1
	LD $02
	ST VTRBF
	INC VRMAD1
	LD $03
	ST VTRBF
	MOV #<(ARG_BLOCK+8), VRMAD1
	LD VTRBF
	ST B
	POP ACC
	PUSH ACC
	MOV #$00, C
	CALLF g_sprite_draw
	POP C
	POP B
	POP ACC
	POP TRH
	POP TRL
	PUSH ACC
	INC ACC
	BNE B, g_symbols_draw_wrap
	MOV #$00, ACC
g_symbols_draw_wrap:
	ST $00
	LD TRL
	ADD C
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	BN ACC, 7, g_symbols_draw_right
	POP ACC
	LD $00
	BR g_symbols_draw_loop
g_symbols_draw_right:
	BNZ g_symbols_draw_done
	LD TRL
	SUB #32
	BN PSW, 7, g_symbols_draw_done
	POP ACC
	LD $00
	BR g_symbols_draw_loop
g_symbols_draw_done:
	POP ACC
	CALLF g_sym_bitmap
	MOV #<(ARG_BLOCK+9), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD $00
	ST VTRBF
	INC VRMAD1
	LD $01
	ST VTRBF
	INC VRMAD1
	LD $02
	ST VTRBF
	INC VRMAD1
	LD $03
	ST VTRBF
	RET

;----------------------
; | ROM_SYMBOLS_8X11_OFF / ROM_SYMBOLS_10X9_OFF
; | Description: Card addresses' low halves of the 8x11 and 10x9 reel symbol bitmaps; the high half is 0x0200.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/rrl_rom.h
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_SYMBOLS_8X11_OFF	EQU	$7DBC
ROM_SYMBOLS_10X9_OFF	EQU	$6520

;----------------------
; | g_reel_symbols
; | Description: For the reel draws: symbols_draw with the table and height given, keeping ARG_BLOCK+8..12 and returning the last bitmap at ARG_BLOCK+4..7.
; | Author: suinevere
; | Dependencies: g_symbols_draw, game.inc (ARG_BLOCK)
; | Globals: the strip and what g_sprite_draw writes
; | Params: TRH:TRL -- x; R0 -- y, signed; ACC -- index; B -- len; ARG_BLOCK+2..3 -- strip; C -- width; R1 -- height; R2, R3 -- the table's low half, low byte first
; | Returns: ARG_BLOCK+4..7 -- the last bitmap. Keeps ARG_BLOCK+8..12; destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW and ARG_BLOCK+0..1; leaves VSEL bit 4 clear; six bytes of stack on top of g_symbols_draw's.
;----------------------
g_reel_symbols:
	SET1 VSEL, 4
	MOV #<(ARG_BLOCK+8), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH ACC
	MOV #<ARG_BLOCK, VRMAD1
	LD $00
	ST VTRBF
	BN ACC, 7, g_reel_symbols_down
	MOV #$FF, VTRBF
	BR g_reel_symbols_table
g_reel_symbols_down:
	MOV #$00, VTRBF
g_reel_symbols_table:
	MOV #<(ARG_BLOCK+4), VRMAD1
	LD $02
	ST VTRBF
	LD $03
	ST VTRBF
	MOV #$00, VTRBF
	MOV #$02, VTRBF
	LD $01
	ST VTRBF
	CLR1 VSEL, 4
	POP ACC
	CALLF g_symbols_draw
	MOV #<(ARG_BLOCK+9), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #$04, B
g_reel_symbols_copy:
	LD VTRBF
	DEC VRMAD1
	DEC VRMAD1
	DEC VRMAD1
	DEC VRMAD1
	DEC VRMAD1
	ST VTRBF
	INC VRMAD1
	INC VRMAD1
	INC VRMAD1
	INC VRMAD1
	INC VRMAD1
	INC VRMAD1
	DBNZ B, g_reel_symbols_copy
	MOV #<(ARG_BLOCK+12), VRMAD1
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
; | g_reel_draw
; | Description: game_reel_draw: draws a reel's 8x11 symbols from pos (64 per pixel, 8 pixels a symbol) across row y; returns the last symbol's bitmap.
; | Author: suinevere
; | Dependencies: g_reel_symbols, ROM_SYMBOLS_8X11_OFF, game.inc (ARG_BLOCK)
; | Globals: the strip and what g_sprite_draw writes
; | Params: TRH:TRL -- y (callers pass 0, 0x0B and 0x16); ARG_BLOCK+0..1 -- pos; ACC -- len; ARG_BLOCK+2..3 -- strip
; | Returns: ARG_BLOCK+4..7 -- the last bitmap. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_reel_draw:
	ST B
	LD TRL
	ST $00
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ROL
	ROL
	AND #$03
	ST C
	INC VRMAD1
	LD VTRBF
	ST $02
	ROL
	ROL
	AND #$04
	OR C
	ST C
	MOV #$00, ACC
	SUB C
	ST TRL
	MOV #$00, ACC
	SUBC #$00
	ST TRH
	LD $02
	CLR1 PSW, 7
	RORC
	PUSH ACC
	MOV #11, $01
	MOV #<ROM_SYMBOLS_8X11_OFF, $02
	MOV #>ROM_SYMBOLS_8X11_OFF, $03
	MOV #$08, C
	POP ACC
	JMPF g_reel_symbols

;----------------------
; | g_run_fill
; | Description: set_fb_run for rows 0-63 with a byte repeated: frame buffer row bytes 1-4 for rows 0-31, else blit_a's word.
; | Author: suinevere
; | Dependencies: rt_fb_row, rrl_ram.inc (HW_BLIT_A)
; | Globals: the frame buffer, blit_a, XBNK
; | Params: ACC -- row, 0-63; B -- the byte, 0 or 0xFF
; | Returns: N/A. Destroys ACC, B, C, R1, R2, XBNK, PSW.
;----------------------
g_run_fill:
	PUSH B
	ST C
	SUB #32
	BN PSW, 7, g_run_fill_a
	LD C
	CALLF rt_fb_row
	LD B
	ST XBNK
	LD C
	INC ACC
	ST $02
	POP ACC
	ST @R2
	INC $02
	ST @R2
	INC $02
	ST @R2
	INC $02
	ST @R2
	RET
g_run_fill_a:
	ROL
	ROL
	AND #$FC
	ADD #HW_BLIT_A
	ST $01
	POP ACC
	ST @R1
	INC $01
	ST @R1
	INC $01
	ST @R1
	INC $01
	ST @R1
	RET

;----------------------
; | g_run_xor
; | Description: Flips bits 11-20 of a run row 0-63, the reel window's inner columns: in the frame buffer's big-endian bytes or blit_a's little-endian word.
; | Author: suinevere
; | Dependencies: rt_fb_row, rrl_ram.inc (HW_BLIT_A)
; | Globals: the frame buffer, blit_a, XBNK
; | Params: ACC -- row, 0-63
; | Returns: N/A. Destroys ACC, B, C, R1, R2, XBNK, PSW.
;----------------------
g_run_xor:
	ST C
	SUB #32
	BN PSW, 7, g_run_xor_a
	LD C
	CALLF rt_fb_row
	LD B
	ST XBNK
	LD C
	ADD #$02
	ST $02
	LD @R2
	XOR #$1F
	ST @R2
	INC $02
	LD @R2
	XOR #$F8
	ST @R2
	RET
g_run_xor_a:
	ROL
	ROL
	AND #$FC
	ADD #HW_BLIT_A+1
	ST $01
	LD @R1
	XOR #$F8
	ST @R1
	INC $01
	LD @R1
	XOR #$1F
	ST @R1
	RET

;----------------------
; | g_reel_window_draw
; | Description: game_reel_window_draw: frames the reel window's top or bottom edge at y, clears the rows inside, draws its 10x9 symbols from pos and inverts the inner columns; returns the last bitmap, or 0 when off screen.
; | Author: suinevere
; | Dependencies: g_run_fill, g_run_xor, g_reel_symbols, ROM_SYMBOLS_10X9_OFF, game.inc (ARG_BLOCK)
; | Globals: the frame buffer, blit_a, XBNK, the strip and what g_sprite_draw writes
; | Params: TRH:TRL -- y (callers pass -12 to 22); ARG_BLOCK+0..1 -- pos; ACC -- len; ARG_BLOCK+2..3 -- strip
; | Returns: ARG_BLOCK+4..7 -- the last bitmap or 0. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, XBNK, PSW; leaves VSEL bit 4 clear; two bytes of stack on top of g_reel_symbols'.
;----------------------
g_reel_window_draw:
	ST B
	LD TRL
	ST $00
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST TRL
	LD B
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	ST TRH
	MOV #$06, C
g_reel_window_draw_shift:
	CLR1 PSW, 7
	LD TRH
	RORC
	ST TRH
	LD TRL
	RORC
	ST TRL
	DBNZ C, g_reel_window_draw_shift
	LD $00
	BP ACC, 7, g_reel_window_draw_low
	SUB #16
	BP PSW, 7, g_reel_window_draw_low
	LD $00
	MOV #$FF, B
	CALLF g_run_fill
	LD $00
	DEC ACC
	ST $03
	INC $00
	LD $00
	BR g_reel_window_draw_rows
g_reel_window_draw_none:
	CLR1 VSEL, 4
	MOV #<(ARG_BLOCK+4), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	MOV #$00, VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	RET
g_reel_window_draw_low:
	LD $00
	ADD #9
	ST $03
	BP ACC, 7, g_reel_window_draw_none
	MOV #$FF, B
	CALLF g_run_fill
	LD $03
	BZ g_reel_window_draw_none
	LD $00
	MOV #$00, $00
g_reel_window_draw_rows:
	MOV #<(ARG_BLOCK+1), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	ST VTRBF
	PUSH $00
	PUSH $03
g_reel_window_draw_clear:
	LD $03
	BZ g_reel_window_draw_symbols
	LD $00
	MOV #$00, B
	CALLF g_run_fill
	INC $00
	DEC $03
	BR g_reel_window_draw_clear
g_reel_window_draw_symbols:
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD TRL
	OR TRH
	BNZ g_reel_window_draw_n
	LD VTRBF
	ST C
	MOV #10, B
	MOV #$00, ACC
	MUL
	ST TRH
	LD C
	ST TRL
g_reel_window_draw_n:
	LD TRL
	SUB #$01
	ST C
	LD TRH
	SUBC #$00
	MOV #10, B
	DIV
	LD C
	PUSH ACC
	MOV #$00, ACC
	SUB B
	ST TRL
	MOV #$00, ACC
	SUBC #$00
	ST TRH
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST B
	INC VRMAD1
	LD VTRBF
	ST $00
	MOV #9, $01
	MOV #<ROM_SYMBOLS_10X9_OFF, $02
	MOV #>ROM_SYMBOLS_10X9_OFF, $03
	MOV #10, C
	POP ACC
	CALLF g_reel_symbols
	POP $03
	POP $00
g_reel_window_draw_invert:
	LD $03
	BZ g_reel_window_draw_ret
	LD $00
	CALLF g_run_xor
	INC $00
	DEC $03
	BR g_reel_window_draw_invert
g_reel_window_draw_ret:
	RET
