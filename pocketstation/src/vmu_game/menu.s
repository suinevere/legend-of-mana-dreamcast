;----------------------
; | menu.s
; | Description: menu.c in LC86K assembly: the menu's blink, a pick's sound and blink restart, and the sel_a, sel_b and three-row menus' moves.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/menu.c, pocketstation/build/vmu_game/rrl_ram.inc, slots.s (anim)
; | Globals: mode_wait, mode_phase, sound_req, sel_a, sel_b, btn_new, anim
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_blink
; | Description: blink: counts mode_wait down; when it reaches 0 it flips mode_phase's bit 0 and starts mode_wait again at 12 (menu.c's BLINK_FRAMES).
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_MODE_WAIT, HW_MODE_PHASE)
; | Globals: mode_wait, mode_phase
; | Params: N/A
; | Returns: N/A. Destroys ACC.
;----------------------
g_blink:
	DEC HW_MODE_WAIT
	LD HW_MODE_WAIT
	BNZ g_blink_ret
	LD HW_MODE_PHASE
	XOR #$01
	ST HW_MODE_PHASE
	MOV #$0C, HW_MODE_WAIT
g_blink_ret:
	RET

;----------------------
; | g_picked
; | Description: picked: asks for sound 0, clears mode_phase and sets mode_wait to 12 (menu.c's BLINK_FRAMES).
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_SOUND_REQ, HW_MODE_PHASE, HW_MODE_WAIT)
; | Globals: sound_req, mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys nothing.
;----------------------
g_picked:
	MOV #$00, HW_SOUND_REQ
	MOV #$00, HW_MODE_PHASE
	MOV #$0C, HW_MODE_WAIT
	RET

;----------------------
; | g_menu_pick_b
; | Description: game_menu_pick_b: a fresh Up moves sel_b to 1, a fresh Down to 0, unless it is there. A move asks for sound 0, sets anim byte 1 and restarts the blink; otherwise it blinks.
; | Author: suinevere
; | Dependencies: g_blink, rrl_ram.inc (HW_SEL_A, HW_SEL_B, HW_BTN_NEW, HW_SOUND_REQ, HW_MODE_PHASE, HW_MODE_WAIT, HWW_ANIM)
; | Globals: sel_a, sel_b, btn_new, sound_req, anim, mode_phase, mode_wait
; | Params: N/A
; | Returns: ACC -- 1 when sel_b moved, else 0. Destroys B, VRMAD, PSW.
;----------------------
g_menu_pick_b:
	LD HW_SEL_B
	BN HW_BTN_NEW, 4, g_menu_pick_b_down
	BE #$01, g_menu_pick_b_down
	MOV #$01, B
	BR g_menu_pick_b_go
g_menu_pick_b_down:
	BN HW_BTN_NEW, 3, g_menu_pick_b_none
	BZ g_menu_pick_b_none
	MOV #$00, B
g_menu_pick_b_go:
	MOV #$00, HW_SOUND_REQ
	LD B
	ST HW_SEL_B
	MOV #$01, ACC
	SUB HW_SEL_A
	ROL
	AND #$FE
	ADD B
	MOV #<(HWW_ANIM+1), VRMAD1
	MOV #>(HWW_ANIM+1), VRMAD2
	ST VTRBF
	MOV #$00, HW_MODE_PHASE
	MOV #$0C, HW_MODE_WAIT
	MOV #$01, ACC
	RET
g_menu_pick_b_none:
	CALLF g_blink
	MOV #$00, ACC
	RET

;----------------------
; | g_menu_pick_a
; | Description: game_menu_pick_a: with sel_a set a fresh Left clears it, with it clear a fresh Right sets it, each then asking for sound 0 and restarting the blink; otherwise it blinks.
; | Author: suinevere
; | Dependencies: g_picked, g_blink, rrl_ram.inc (HW_SEL_A, HW_BTN_NEW)
; | Globals: sel_a, btn_new, sound_req, mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys ACC.
;----------------------
g_menu_pick_a:
	LD HW_SEL_A
	BZ g_menu_pick_a_off
	BN HW_BTN_NEW, 2, g_menu_pick_a_blink
	MOV #$00, HW_SEL_A
	BR g_picked
g_menu_pick_a_off:
	BN HW_BTN_NEW, 1, g_menu_pick_a_blink
	MOV #$01, HW_SEL_A
	BR g_picked
g_menu_pick_a_blink:
	BR g_blink

;----------------------
; | g_menu_pick_row
; | Description: game_menu_pick_row: Up moves mode_done back a row, from 0 to 2, and Down on a row, from 2 to 0. Either then asks for sound 0 and restarts the blink; otherwise it blinks.
; | Author: suinevere
; | Dependencies: g_picked, g_blink, rrl_ram.inc (HW_BTN_PRESS, HW_MODE_DONE)
; | Globals: btn_press, mode_done, sound_req, mode_phase, mode_wait
; | Params: N/A
; | Returns: N/A. Destroys ACC, PSW.
;----------------------
g_menu_pick_row:
	LD HW_MODE_DONE
	BN HW_BTN_PRESS, 4, g_menu_pick_row_down
	BZ g_menu_pick_row_last
	DEC ACC
	BR g_menu_pick_row_set
g_menu_pick_row_last:
	MOV #$02, ACC
	BR g_menu_pick_row_set
g_menu_pick_row_down:
	BN HW_BTN_PRESS, 3, g_menu_pick_a_blink
	BE #$02, g_menu_pick_row_first
	INC ACC
	BR g_menu_pick_row_set
g_menu_pick_row_first:
	MOV #$00, ACC
g_menu_pick_row_set:
	ST HW_MODE_DONE
	BR g_picked
