;----------------------
; | rng.s
; | Description: rng.c's random numbers in LC86K assembly: the seed step, its scaling below a limit, a weighted pick, an item table entry's match, the odds-table picks, the event roll and TR's small multiplies.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/rng.c, pocketstation/src/vmu/rrl_rom.h, pocketstation/build/vmu_game/rrl_ram.inc, game.inc (ARG_BLOCK), tables.s (app_tb_80, app_tables)
; | Globals: seed, scratch_count, scene_id
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_random
; | Description: game_random: steps the seed, seed = (seed + 1) * 0x3D09 kept to 16 bits, and returns (limit * seed) >> 16, the 32-bit product built from two 16x8 MULs.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_SEED)
; | Globals: seed
; | Params: TRH:TRL -- limit, the exclusive upper bound
; | Returns: TRH:TRL -- a value in [0, limit). Destroys ACC, B, C, PSW; two bytes of stack.
;----------------------
g_random:
	LD HW_SEED
	ADD #$01
	ST HW_SEED
	LD HW_SEED+1
	ADDC #$00
	ST HW_SEED+1
	MOV #$3D, B
	LD HW_SEED
	ST C
	MOV #$00, ACC
	MUL
	PUSH C
	LD HW_SEED
	ST C
	LD HW_SEED+1
	MOV #$09, B
	MUL
	ST B
	LD C
	ST HW_SEED
	POP ACC
	ADD B
	ST HW_SEED+1
	LD HW_SEED
	ST C
	LD TRL
	ST B
	LD HW_SEED+1
	MUL
	PUSH B
	PUSH ACC
	LD HW_SEED
	ST C
	LD TRH
	ST B
	LD HW_SEED+1
	MUL
	ST TRL
	LD B
	ST TRH
	POP ACC
	ADD C
	POP ACC
	ADDC TRL
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	RET

;----------------------
; | g_pick_weighted
; | Description: pick_weighted: rolls below 100 and returns the index of the weight byte, lowest first, whose running total first passes the roll. Like the C, it never returns if the weights sum under 100.
; | Author: suinevere
; | Dependencies: g_random, game.inc (ARG_BLOCK)
; | Globals: seed
; | Params: ARG_BLOCK+0..3 -- weights, four bytes, lowest first
; | Returns: ACC -- the index. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear; four bytes of stack.
;----------------------
g_pick_weighted:
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
	CLR1 VSEL, 4
	MOV #$64, TRL
	MOV #$00, TRH
	CALLF g_random
	MOV #$00, B
	LD TRL
g_pick_weighted_loop:
	ST C
	SUB $00
	BP PSW, 7, g_pick_weighted_done
	ST C
	LD $01
	ST $00
	LD $02
	ST $01
	LD $03
	ST $02
	MOV #$00, $03
	INC B
	LD C
	BR g_pick_weighted_loop
g_pick_weighted_done:
	LD B
	RET

;----------------------
; | ROM_WORDS_8094
; | Description: File offset of the item table at 0x02008094 (rrl_rom.h ROM_WORDS_8094), halfwords lowest byte first, in the kept run from page 0x80.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/rrl_rom.h, tables.s (app_tb_80)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_WORDS_8094	EQU	$8094

;----------------------
; | g_item_matches
; | Description: item_matches: 1 when item table entry i has kind in its low byte and pick in its high byte, else 0. The entry is read with LDC from a kept page.
; | Author: suinevere
; | Dependencies: app_tb_80, ROM_WORDS_8094, g_tr_x2
; | Globals: N/A
; | Params: TRH:TRL -- i (game_pick_item passes 0-254); ACC -- kind; B -- pick
; | Returns: ACC -- 1 or 0. Destroys C, TRH, TRL, PSW; two bytes of stack beyond the return address.
;----------------------
g_item_matches:
	ST C
	CALLF g_tr_x2
	LD TRL
	ADD #<(app_tb_80+ROM_WORDS_8094)
	ST TRL
	LD TRH
	ADDC #>(app_tb_80+ROM_WORDS_8094)
	ST TRH
	MOV #$00, ACC
	LDC
	BNE C, g_item_matches_no
	MOV #$01, ACC
	LDC
	BNE B, g_item_matches_no
	MOV #$01, ACC
	RET
g_item_matches_no:
	MOV #$00, ACC
	RET

;----------------------
; | ROM_ODDS_SEL / ROM_ODDS_SET / ROM_ODDS_NTH / ROM_EVENT_ODDS
; | Description: File offsets of rrl_rom.h's odds tables: four weight bytes per row, and the event-odds lists one card address per scene from 7.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/rrl_rom.h
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_ODDS_SEL	EQU	$9EDC
ROM_ODDS_SET	EQU	$9EEC
ROM_ODDS_NTH	EQU	$9EF8
ROM_EVENT_ODDS	EQU	$9F0C

;----------------------
; | g_pick_odds
; | Description: pick_weighted on the four weight bytes at a file offset: lends them to g_pick_weighted in ARG_BLOCK+0..3 by LDC from app_tables, then puts the four bytes it found there back.
; | Author: suinevere
; | Dependencies: g_pick_weighted, app_tables, game.inc (ARG_BLOCK)
; | Globals: seed
; | Params: TRH:TRL -- file offset of the weights
; | Returns: ACC -- the index. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear; ten bytes of stack.
;----------------------
g_pick_odds:
	SET1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	LD TRL
	ADD #<app_tables
	ST TRL
	LD TRH
	ADDC #>app_tables
	ST TRH
	MOV #<ARG_BLOCK, VRMAD1
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
	CALLF g_pick_weighted
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<(ARG_BLOCK+3), VRMAD1
	POP VTRBF
	MOV #<(ARG_BLOCK+2), VRMAD1
	POP VTRBF
	MOV #<(ARG_BLOCK+1), VRMAD1
	POP VTRBF
	MOV #<ARG_BLOCK, VRMAD1
	POP VTRBF
	RET

;----------------------
; | g_pick_bag_nth
; | Description: game_pick_bag_nth: picks with the ROM_ODDS_NTH weights and keeps the index in scratch_count as well as returning it.
; | Author: suinevere
; | Dependencies: g_pick_odds, ROM_ODDS_NTH, rrl_ram.inc (HW_SCRATCH_COUNT)
; | Globals: seed, scratch_count
; | Params: N/A
; | Returns: ACC -- the index. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_pick_bag_nth:
	MOV #<ROM_ODDS_NTH, TRL
	MOV #>ROM_ODDS_NTH, TRH
	CALLF g_pick_odds
	ST HW_SCRATCH_COUNT
	RET

;----------------------
; | g_pick_by_sel
; | Description: game_pick_by_sel: picks with the ROM_ODDS_SEL row sel_a | sel_b << 1, kept to 16 bits as the C's index.
; | Author: suinevere
; | Dependencies: g_pick_odds, ROM_ODDS_SEL, rrl_ram.inc (HW_SEL_A, HW_SEL_B), g_tr_x2
; | Globals: seed, sel_a, sel_b
; | Params: N/A
; | Returns: ACC -- the index. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_pick_by_sel:
	LD HW_SEL_B
	ST TRL
	MOV #$00, TRH
	CALLF g_tr_x2
	LD TRL
	OR HW_SEL_A
	ST TRL
	MOV #<ROM_ODDS_SEL, ACC
	BR g_pick_row4

;----------------------
; | g_pick_by_set
; | Description: game_pick_by_set: picks with the ROM_ODDS_SET row odds_set.
; | Author: suinevere
; | Dependencies: g_pick_odds, ROM_ODDS_SET, rrl_ram.inc (HW_ODDS_SET)
; | Globals: seed, odds_set
; | Params: N/A
; | Returns: ACC -- the index. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_pick_by_set:
	LD HW_ODDS_SET
	ST TRL
	MOV #$00, TRH
	MOV #<ROM_ODDS_SET, ACC

;----------------------
; | g_pick_row4
; | Description: Tail of g_pick_by_sel and g_pick_by_set: offset = row * 4 + the table, then g_pick_odds. The table's high byte is $9E for both.
; | Author: suinevere
; | Dependencies: g_pick_odds, g_tr_x4
; | Globals: N/A
; | Params: TRH:TRL -- row; ACC -- the table offset's low byte
; | Returns: as g_pick_odds.
;----------------------
g_pick_row4:
	PUSH ACC
	CALLF g_tr_x4
	POP ACC
	ADD TRL
	ST TRL
	LD TRH
	ADDC #$9E
	ST TRH
	JMPF g_pick_odds

;----------------------
; | g_event_roll
; | Description: game_event_roll: rolls random(1600) >> 4 against the scene's event-odds list, weight bytes paired with next scenes, sets scene_id to the chosen entry's scene and returns its index.
; | Author: suinevere
; | Dependencies: g_random, app_tables, ROM_EVENT_ODDS, rrl_ram.inc (HW_SCENE_ID), g_tr_x4
; | Globals: seed, scene_id
; | Params: N/A
; | Returns: ACC -- the index. Destroys B, C, TRH, TRL, R0, R2, R3, PSW; four bytes of stack.
;----------------------
g_event_roll:
	LD HW_SCENE_ID
	SUB #$07
	ST TRL
	MOV #$00, ACC
	SUBC #$00
	ST TRH
	CALLF g_tr_x4
	LD TRL
	ADD #<(app_tables+ROM_EVENT_ODDS)
	ST TRL
	LD TRH
	ADDC #>(app_tables+ROM_EVENT_ODDS)
	ST TRH
	MOV #$00, ACC
	LDC
	ST $02
	MOV #$01, ACC
	LDC
	ST $03
	LD $02
	ADD #<app_tables
	ST $02
	LD $03
	ADDC #>app_tables
	ST $03
	MOV #$40, TRL
	MOV #$06, TRH
	CALLF g_random
	LD TRL
	ROR
	ROR
	ROR
	ROR
	AND #$0F
	ST $00
	LD TRH
	ROL
	ROL
	ROL
	ROL
	AND #$F0
	OR $00
	ST $00
	MOV #$00, B
g_event_roll_loop:
	LD $02
	ST TRL
	LD $03
	ST TRH
	MOV #$00, ACC
	LDC
	ST C
	LD $00
	SUB C
	BP PSW, 7, g_event_roll_done
	ST $00
	INC B
	LD $02
	ADD #$02
	ST $02
	LD $03
	ADDC #$00
	ST $03
	BR g_event_roll_loop
g_event_roll_done:
	MOV #$01, ACC
	LDC
	ST HW_SCENE_ID
	LD B
	RET

;----------------------
; | g_pick_item
; | Description: game_pick_item: how 2 gives an anim byte plus 0x40 or 0x48; otherwise a set pick, how 1 in scene 13 mostly a fixed item, then a random item entry matching the kind and pick.
; | Author: suinevere
; | Dependencies: g_pick_by_set, g_pick_by_sel, g_item_matches, g_random, rrl_ram.inc (HW_MODE_DONE, HW_ODDS_SET, HW_SCENE_ID, HW_SEL_A, HW_SEL_B, HWW_ANIM), g_tr_x16
; | Globals: seed, mode_done, odds_set (kept), scene_id, sel_a, sel_b, anim and what the pick routines read
; | Params: ACC -- how
; | Returns: TRH:TRL -- the item. Destroys ACC, B, C, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear. Like the C, it never returns when no entry matches.
;----------------------
g_pick_item:
	BNE #$02, g_pick_item_saved
	CALLF g_pick_by_set
	MOV #$40, C
	BE #$03, g_pick_item_anim
	MOV #$48, C
g_pick_item_anim:
	LD HW_MODE_DONE
	ADD #$05
	ROL
	ROL
	AND #$FC
	ADD #<HWW_ANIM
	ST VRMAD1
	MOV #>HWW_ANIM, ACC
	ADDC #$00
	ST VRMAD2
	CLR1 VSEL, 4
	LD VTRBF
	ADD C
	ST TRL
	MOV #$00, ACC
	ADDC #$00
	ST TRH
	RET
g_pick_item_saved:
	BNE #$03, g_pick_item_scene
	LD HW_ODDS_SET
	PUSH ACC
	MOV #$00, HW_ODDS_SET
	CALLF g_pick_by_set
	POP HW_ODDS_SET
	BR g_pick_item_kind
g_pick_item_scene:
	BNE #$01, g_pick_item_set
	LD HW_SCENE_ID
	BNE #13, g_pick_item_set
	MOV #100, TRL
	MOV #$00, TRH
	CALLF g_random
	LD TRL
	SUB #80
	BN PSW, 7, g_pick_item_three
	MOV #$00, TRH
	LD HW_SEL_A
	BNZ g_pick_item_a
	MOV #$34, TRL
	LD HW_SEL_B
	BZ g_pick_item_ret
	MOV #$15, TRL
	RET
g_pick_item_a:
	MOV #$08, TRL
	LD HW_SEL_B
	BZ g_pick_item_ret
	MOV #$1B, TRL
g_pick_item_ret:
	RET
g_pick_item_three:
	MOV #$03, ACC
	BR g_pick_item_kind
g_pick_item_set:
	CALLF g_pick_by_set
g_pick_item_kind:
	PUSH ACC
	CALLF g_pick_by_sel
	ST $00
	POP B
	MOV #$00, $01
	MOV #$00, $02
g_pick_item_count:
	LD $02
	ST TRL
	MOV #$00, TRH
	LD $00
	CALLF g_item_matches
	ADD $01
	ST $01
	INC $02
	LD $02
	BNE #255, g_pick_item_count
	LD $01
	ST TRL
	MOV #$00, TRH
	CALLF g_tr_x16
	PUSH B
	CALLF g_random
	POP B
	LD TRL
	ROR
	ROR
	ROR
	ROR
	AND #$0F
	ST $01
	LD TRH
	ROL
	ROL
	ROL
	ROL
	AND #$F0
	OR $01
	ST $01
	MOV #$00, $02
g_pick_item_find:
	LD $02
	ST TRL
	MOV #$00, TRH
	LD $00
	CALLF g_item_matches
	BZ g_pick_item_next
	LD $01
	BZ g_pick_item_found
	DEC $01
g_pick_item_next:
	INC $02
	BR g_pick_item_find
g_pick_item_found:
	LD $02
	ST TRL
	MOV #$00, TRH
	RET

;----------------------
; | g_tr_x16 / g_tr_x8 / g_tr_x4 / g_tr_x2
; | Description: Multiply TRH:TRL by 16, 8, 4 or 2: each entry doubles once and falls into the next smaller one.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRH:TRL -- the value
; | Returns: TRH:TRL -- the value times the entry's factor, modulo 65536. Destroys ACC, PSW; two bytes of stack beyond the return address.
;----------------------
g_tr_x16:
	CALLF g_tr_x2
g_tr_x8:
	CALLF g_tr_x2
g_tr_x4:
	CALLF g_tr_x2
g_tr_x2:
	LD TRL
	ADD TRL
	ST TRL
	LD TRH
	ADDC TRH
	ST TRH
	RET
