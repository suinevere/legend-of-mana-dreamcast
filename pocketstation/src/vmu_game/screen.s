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
