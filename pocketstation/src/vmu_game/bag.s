;----------------------
; | bag.s
; | Description: bag.c in LC86K assembly: adding and removing bag items, the reveal's next item and scroll, the bag menu's blink, restart, row and move, and counting the bag's items by kind.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/bag.c, pocketstation/build/vmu_game/rrl_ram.inc, game.inc (ARG_BLOCK), system.s (g_rom_word_8094)
; | Globals: bag_count, bag_items, bag_cursor, bag_full, mode, mode_done, mode_wait, mode_phase, scene_id, scratch_first, scratch_count, scratch_kinds, scratch_lo, bag_nth_index, bag_kind_count, sound_req
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_bag_add
; | Description: game_bag_add: counts the 32-bit bag_count up, stores the item at the new last place, and sets mode 14 with bag_full when the bag now holds 0x33 items, else mode 3.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_BAG_COUNT, HWW_BAG_ITEMS, HW_BAG_FULL, HW_MODE)
; | Globals: bag_count, bag_items, bag_full, mode
; | Params: ACC -- item
; | Returns: N/A. Destroys ACC, B, C, TRL, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_bag_add:
	ST B
	CLR1 VSEL, 4
	MOV #>HWW_BAG_COUNT, VRMAD2
	MOV #<HWW_BAG_COUNT, VRMAD1
	LD VTRBF
	ADD #$01
	ST VTRBF
	ST C
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	ST TRL
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	OR TRL
	ST TRL
	INC VRMAD1
	LD VTRBF
	ADDC #$00
	ST VTRBF
	OR TRL
	ST TRL
	LD C
	ADD #<(HWW_BAG_ITEMS-1)
	ST VRMAD1
	MOV #>HWW_BAG_ITEMS, VRMAD2
	LD B
	ST VTRBF
	LD C
	XOR #$33
	OR TRL
	BNZ g_bag_add_room
	MOV #$01, HW_BAG_FULL
	MOV #$0E, HW_MODE
	RET
g_bag_add_room:
	MOV #$03, HW_MODE
	RET

;----------------------
; | g_bag_remove
; | Description: game_bag_remove: moves bag_cursor back one when asked and index is below it, counts the 32-bit bag_count down, and closes the gap by moving each later item down a place.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_BAG_COUNT, HWW_BAG_ITEMS, HW_BAG_CURSOR)
; | Globals: bag_count, bag_items, bag_cursor
; | Params: ACC -- index; B -- move_cursor (1 to move the cursor)
; | Returns: N/A. Destroys ACC, C, TRL, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear. The items moved are the low byte of the new count less index, as the bag holds under 256.
;----------------------
g_bag_remove:
	ST TRL
	CLR1 VSEL, 4
	LD B
	BNE #$01, g_bag_remove_count
	LD TRL
	BE HW_BAG_CURSOR, g_bag_remove_count
	BN PSW, 7, g_bag_remove_count
	DEC HW_BAG_CURSOR
g_bag_remove_count:
	MOV #>HWW_BAG_COUNT, VRMAD2
	MOV #<HWW_BAG_COUNT, VRMAD1
	LD VTRBF
	SUB #$01
	ST VTRBF
	ST C
	INC VRMAD1
	LD VTRBF
	SUBC #$00
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	SUBC #$00
	ST VTRBF
	INC VRMAD1
	LD VTRBF
	SUBC #$00
	ST VTRBF
	LD C
	SUB TRL
	BZ g_bag_remove_ret
	ST C
	LD TRL
	ADD #<(HWW_BAG_ITEMS+1)
	ST VRMAD1
	MOV #>HWW_BAG_ITEMS, VRMAD2
g_bag_remove_move:
	LD VTRBF
	DEC VRMAD1
	ST VTRBF
	INC VRMAD1
	INC VRMAD1
	DBNZ C, g_bag_remove_move
g_bag_remove_ret:
	RET

;----------------------
; | g_bag_reveal_next
; | Description: bag_reveal_next: when bag_cursor has reached the 32-bit bag_count it sets scene_id 0x3C and counts mode_done up, else it moves the cursor on and sets scene_id 0x14.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_BAG_COUNT, HW_BAG_CURSOR, HW_SCENE_ID, HW_MODE_DONE)
; | Globals: bag_count, bag_cursor, scene_id, mode_done
; | Params: N/A
; | Returns: N/A. Destroys ACC, VRMAD; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_bag_reveal_next:
	MOV #>HWW_BAG_COUNT, VRMAD2
	MOV #<HWW_BAG_COUNT, VRMAD1
	SET1 VSEL, 4
	LD VTRBF
	XOR HW_BAG_CURSOR
	OR VTRBF
	OR VTRBF
	OR VTRBF
	CLR1 VSEL, 4
	BNZ g_bag_reveal_next_on
	MOV #$3C, HW_SCENE_ID
	INC HW_MODE_DONE
	RET
g_bag_reveal_next_on:
	INC HW_BAG_CURSOR
	MOV #$14, HW_SCENE_ID
	RET

;----------------------
; | bag_scroll_frames
; | Description: The reveal scroll's frame for each of eight positions, bag.c's BAG_SCROLL_FRAMES.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/bag.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
bag_scroll_frames:
	.byte $00, $05, $08, $0A, $0B, $0A, $08, $05

;----------------------
; | g_bag_reveal_scroll
; | Description: bag_reveal_scroll: steps the 16-bit signed position in scratch_first and scratch_count back one, wrapping to 0x60 below -0xCF, and sets scratch_kinds to the scroll frame 0x48 behind it, 0 when that is negative or bit 3 is set.
; | Author: suinevere
; | Dependencies: bag_scroll_frames, rrl_ram.inc (HW_SCRATCH_FIRST, HW_SCRATCH_COUNT, HW_SCRATCH_KINDS)
; | Globals: scratch_first, scratch_count, scratch_kinds
; | Params: N/A
; | Returns: N/A. Destroys ACC, C, TRL, TRH, PSW.
;----------------------
g_bag_reveal_scroll:
	LD HW_SCRATCH_FIRST
	SUB #$31
	LD HW_SCRATCH_COUNT
	XOR #$80
	SUBC #$7F
	BN PSW, 7, g_bag_reveal_scroll_step
	MOV #$60, HW_SCRATCH_FIRST
	MOV #$00, HW_SCRATCH_COUNT
	BR g_bag_reveal_scroll_shown
g_bag_reveal_scroll_step:
	LD HW_SCRATCH_FIRST
	SUB #$01
	ST HW_SCRATCH_FIRST
	LD HW_SCRATCH_COUNT
	SUBC #$00
	ST HW_SCRATCH_COUNT
g_bag_reveal_scroll_shown:
	LD HW_SCRATCH_FIRST
	SUB #$48
	ST C
	LD HW_SCRATCH_COUNT
	SUBC #$00
	BN ACC, 7, g_bag_reveal_scroll_frame
g_bag_reveal_scroll_none:
	MOV #$00, HW_SCRATCH_KINDS
	RET
g_bag_reveal_scroll_frame:
	BP C, 3, g_bag_reveal_scroll_none
	MOV #<bag_scroll_frames, TRL
	MOV #>bag_scroll_frames, TRH
	LD C
	AND #$07
	LDC
	ST HW_SCRATCH_KINDS
	RET

;----------------------
; | g_menu_blink
; | Description: menu_blink: counts mode_wait down; when it reaches 0 it flips mode_phase's bit 0 and starts mode_wait again at 12 (bag.c's MENU_BLINK).
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_MODE_WAIT, HW_MODE_PHASE)
; | Globals: mode_wait, mode_phase
; | Params: N/A
; | Returns: N/A. Destroys ACC.
;----------------------
g_menu_blink:
	DEC HW_MODE_WAIT
	LD HW_MODE_WAIT
	BNZ g_menu_blink_ret
	LD HW_MODE_PHASE
	XOR #$01
	ST HW_MODE_PHASE
	MOV #$0C, HW_MODE_WAIT
g_menu_blink_ret:
	RET

;----------------------
; | g_menu_restart
; | Description: menu_restart: clears mode_phase and sets mode_wait to 12 (MENU_BLINK).
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_MODE_PHASE, HW_MODE_WAIT)
; | Globals: mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_menu_restart:
	MOV #$00, HW_MODE_PHASE
	MOV #$0C, HW_MODE_WAIT
	RET

;----------------------
; | g_menu_row
; | Description: menu_row: moves the menu's row in scene_id to 0 on Up or to 1 on Down when it is not there already, returning 1 when it moved, else 0.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_SCENE_ID)
; | Globals: scene_id
; | Params: ACC -- press, the new presses (BTN_UP 0x10, BTN_DOWN 0x08)
; | Returns: ACC -- 1 when the row moved, else 0. Destroys B.
;----------------------
g_menu_row:
	ST B
	LD HW_SCENE_ID
	BN B, 4, g_menu_row_down
	BZ g_menu_row_down
	MOV #$00, HW_SCENE_ID
	MOV #$01, ACC
	RET
g_menu_row_down:
	BN B, 3, g_menu_row_none
	XOR #$01
	BZ g_menu_row_none
	MOV #$01, HW_SCENE_ID
	MOV #$01, ACC
	RET
g_menu_row_none:
	MOV #$00, ACC
	RET

;----------------------
; | g_menu_moved
; | Description: menu_moved: asks for sound 0, clears mode_phase and sets mode_wait to 12 (bag.c's MENU_BLINK).
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_SOUND_REQ, HW_MODE_PHASE, HW_MODE_WAIT)
; | Globals: sound_req, mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_menu_moved:
	MOV #$00, HW_SOUND_REQ
	MOV #$00, HW_MODE_PHASE
	MOV #$0C, HW_MODE_WAIT
	RET

;----------------------
; | g_bag_kind_at
; | Description: The kind of bag item i, game_rom_word_8094 of its id kept to its low byte.
; | Author: suinevere
; | Dependencies: g_rom_word_8094, rrl_ram.inc (HWW_BAG_ITEMS)
; | Globals: bag_items
; | Params: R1 -- i, below 51
; | Returns: ACC -- the kind. Destroys B, C, TRH, TRL, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_bag_kind_at:
	CLR1 VSEL, 4
	LD $01
	ADD #<HWW_BAG_ITEMS
	ST VRMAD1
	MOV #>HWW_BAG_ITEMS, VRMAD2
	LD VTRBF
	CALLF g_rom_word_8094
	LD TRL
	RET

;----------------------
; | g_bag_count_first
; | Description: game_bag_count_first: counts the bag items of kind scratch_first over bag_count's low byte of items, keeps the count in scratch_lo and returns it.
; | Author: suinevere
; | Dependencies: g_bag_kind_at, rrl_ram.inc (HWW_BAG_COUNT, HW_SCRATCH_FIRST, HW_SCRATCH_LO)
; | Globals: bag_count, bag_items, scratch_first, scratch_lo
; | Params: N/A (bag_count 1-51; the C's 32-bit count would loop 2^32 times at 0)
; | Returns: ACC -- the count. Destroys B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_bag_count_first:
	CLR1 VSEL, 4
	MOV #<HWW_BAG_COUNT, VRMAD1
	MOV #>HWW_BAG_COUNT, VRMAD2
	LD VTRBF
	ST $00
	MOV #$00, $01
	LD HW_SCRATCH_FIRST
	ST $02
	MOV #$00, $03
g_bag_count_first_loop:
	CALLF g_bag_kind_at
	BNE $02, g_bag_count_first_next
	INC $03
g_bag_count_first_next:
	INC $01
	DBNZ $00, g_bag_count_first_loop
	LD $03
	ST HW_SCRATCH_LO
	RET

;----------------------
; | g_bag_find_nth
; | Description: game_bag_find_nth: the index of the bag item that is the scratch_count-th, from 0, of kind scratch_first; keeps it in bag_nth_index. Like the C, it never returns without one.
; | Author: suinevere
; | Dependencies: g_bag_kind_at, rrl_ram.inc (HW_SCRATCH_FIRST, HW_SCRATCH_COUNT, HW_BAG_NTH_INDEX)
; | Globals: bag_items, scratch_first, scratch_count, bag_nth_index
; | Params: N/A
; | Returns: ACC -- the index. Destroys B, C, TRH, TRL, R0-R2, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_bag_find_nth:
	LD HW_SCRATCH_FIRST
	ST $02
	LD HW_SCRATCH_COUNT
	ST $00
	MOV #$FF, $01
g_bag_find_nth_loop:
	INC $01
	CALLF g_bag_kind_at
	BNE $02, g_bag_find_nth_loop
	LD $00
	BZ g_bag_find_nth_done
	DEC $00
	BR g_bag_find_nth_loop
g_bag_find_nth_done:
	LD $01
	ST HW_BAG_NTH_INDEX
	RET

;----------------------
; | g_bag_kinds
; | Description: game_bag_kinds: the set of item kinds below 32 in the bag as a 32-bit mask, their count in bag_kind_count, its low byte in scratch_kinds and its lowest kind in scratch_first. Like the C, it never returns with an empty mask.
; | Author: suinevere
; | Dependencies: g_bag_kind_at, game.inc (ARG_BLOCK), rrl_ram.inc (HWW_BAG_COUNT, HW_SCRATCH_KINDS, HW_BAG_KIND_COUNT, HW_SCRATCH_FIRST)
; | Globals: bag_count, bag_items, scratch_kinds, bag_kind_count, scratch_first
; | Params: N/A (bag_count 1-51)
; | Returns: ARG_BLOCK+0..3 -- the mask, lowest byte first. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, PSW; leaves VSEL bit 4 clear.
;----------------------
g_bag_kinds:
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	MOV #$00, ACC
	ST VTRBF
	INC VRMAD1
	ST VTRBF
	INC VRMAD1
	ST VTRBF
	INC VRMAD1
	ST VTRBF
	MOV #<HWW_BAG_COUNT, VRMAD1
	MOV #>HWW_BAG_COUNT, VRMAD2
	LD VTRBF
	ST $00
	MOV #$00, $01
	MOV #$00, $03
g_bag_kinds_loop:
	CALLF g_bag_kind_at
	ST $02
	SUB #$20
	BN PSW, 7, g_bag_kinds_next
	LD $02
	ROR
	ROR
	ROR
	AND #$03
	ADD #<ARG_BLOCK
	ST VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD $02
	AND #$07
	ST B
	INC B
	MOV #$80, ACC
g_bag_kinds_bit:
	ROL
	DBNZ B, g_bag_kinds_bit
	ST C
	LD VTRBF
	AND C
	BNZ g_bag_kinds_next
	LD VTRBF
	OR C
	ST VTRBF
	INC $03
g_bag_kinds_next:
	INC $01
	DBNZ $00, g_bag_kinds_loop
	LD $03
	ST HW_BAG_KIND_COUNT
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST HW_SCRATCH_KINDS
	MOV #$00, B
g_bag_kinds_byte:
	LD VTRBF
	BNZ g_bag_kinds_low
	LD B
	ADD #$08
	ST B
	INC VRMAD1
	LD VRMAD1
	SUB #<(ARG_BLOCK+4)
	BNZ g_bag_kinds_byte
	MOV #<ARG_BLOCK, VRMAD1
	MOV #$00, B
	BR g_bag_kinds_byte
g_bag_kinds_low:
	BP ACC, 0, g_bag_kinds_found
	ROR
	INC B
	BR g_bag_kinds_low
g_bag_kinds_found:
	LD B
	ST HW_SCRATCH_FIRST
	RET
