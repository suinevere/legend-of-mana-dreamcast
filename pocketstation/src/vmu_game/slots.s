;----------------------
; | slots.s
; | Description: slots.c in LC86K assembly: clearing the animation and marker slots, asking whether a marker slot is in use, and starting and ticking the animations.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/slots.c, pocketstation/build/vmu_game/rrl_ram.inc, rng.s (g_random), app_hi
; | Globals: anim, marker, seed
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_anims_clear
; | Description: game_anims_clear: writes 0xFF to the first byte of each of the ten 4-byte animation slots in work RAM, marking them free.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_ANIM)
; | Globals: anim
; | Params: N/A
; | Returns: N/A. Destroys VRMAD.
;----------------------
g_anims_clear:
	MOV #>HWW_ANIM, VRMAD2
	MOV #<HWW_ANIM, VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_ANIM+4), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_ANIM+8), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_ANIM+12), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_ANIM+16), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_ANIM+20), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_ANIM+24), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_ANIM+28), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_ANIM+32), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_ANIM+36), VRMAD1
	MOV #$FF, VTRBF
	RET

;----------------------
; | g_markers_clear
; | Description: game_markers_clear: writes 0xFF to the first byte of each of the four 4-byte marker slots in work RAM, marking them free.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_MARKER)
; | Globals: marker
; | Params: N/A
; | Returns: N/A. Destroys VRMAD.
;----------------------
g_markers_clear:
	MOV #>HWW_MARKER, VRMAD2
	MOV #<HWW_MARKER, VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_MARKER+4), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_MARKER+8), VRMAD1
	MOV #$FF, VTRBF
	MOV #<(HWW_MARKER+12), VRMAD1
	MOV #$FF, VTRBF
	RET

;----------------------
; | g_marker_used
; | Description: game_marker_used: returns 1 when marker slot index's first byte is not 0xFF, else 0.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_MARKER)
; | Globals: marker
; | Params: ACC -- index, 0-3
; | Returns: ACC -- 1 when the slot is in use, else 0. Destroys VRMAD, PSW.
;----------------------
g_marker_used:
	ROL
	ROL
	ADD #<HWW_MARKER
	ST VRMAD1
	MOV #>HWW_MARKER, VRMAD2
	LD VTRBF
	INC ACC
	BZ g_marker_used_ret
	MOV #$01, ACC
g_marker_used_ret:
	RET

;----------------------
; | g_anim_at
; | Description: Points VRMAD at byte ACC of the anim slots, carrying into VRMAD2, so the address is right wherever gen_ram places anim.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_ANIM)
; | Globals: anim
; | Params: ACC -- the byte's index, 0-39
; | Returns: N/A. Destroys ACC, VRMAD, PSW.
;----------------------
g_anim_at:
	ADD #<HWW_ANIM
	ST VRMAD1
	MOV #>HWW_ANIM, ACC
	ADDC #$00
	ST VRMAD2
	RET

;----------------------
; | ROM_ANIM_DELAY
; | Description: The animation table's offset in RES1506.BIN, one byte an id: the first wait's range for game_anim_start and the hold for game_anims_tick (slots.c's ROM_ANIM_HOLD).
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/rrl_rom.h, pocketstation/src/vmu/slots.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ROM_ANIM_DELAY	EQU	$A7B0

;----------------------
; | g_anim_start
; | Description: game_anim_start: fills animation slot slot with id, arg, frame 0 and a first wait of 1 plus a random value below the id's table byte times 16, divided by 16.
; | Author: suinevere
; | Dependencies: g_anim_at, g_random, app_hi, ROM_ANIM_DELAY
; | Globals: anim, seed
; | Params: ACC -- slot, 0-9; B -- id; C -- arg
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear; four bytes of stack.
;----------------------
g_anim_start:
	ROL
	ROL
	AND #$FC
	CALLF g_anim_at
	SET1 VSEL, 4
	LD B
	ST VTRBF
	LD C
	ST VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
	MOV #<(app_hi+ROM_ANIM_DELAY), TRL
	MOV #>(app_hi+ROM_ANIM_DELAY), TRH
	LD B
	LDC
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
	INC ACC
	ST VTRBF
	RET

;----------------------
; | g_anims_tick
; | Description: game_anims_tick: for each of the 10 animation slots in use (id not 0xFF), counts its wait down. On 0 it flips the frame and reloads the wait from the id's table byte.
; | Author: suinevere
; | Dependencies: app_hi, ROM_ANIM_DELAY, rrl_ram.inc (HWW_ANIM)
; | Globals: anim
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_anims_tick:
	MOV #<HWW_ANIM, VRMAD1
	MOV #>HWW_ANIM, VRMAD2
	SET1 VSEL, 4
	MOV #$0A, C
g_anims_tick_loop:
	LD VTRBF
	BE #$FF, g_anims_tick_empty
	ST B
	LD VTRBF
	LD VTRBF
	ST TRL
	CLR1 VSEL, 4
	LD VTRBF
	SUB #$01
	BNZ g_anims_tick_store
	LD VRMAD1
	SUB #$01
	ST VRMAD1
	LD VRMAD2
	SUBC #$00
	ST VRMAD2
	LD TRL
	XOR #$01
	SET1 VSEL, 4
	ST VTRBF
	CLR1 VSEL, 4
	MOV #<(app_hi+ROM_ANIM_DELAY), TRL
	MOV #>(app_hi+ROM_ANIM_DELAY), TRH
	LD B
	LDC
g_anims_tick_store:
	SET1 VSEL, 4
	ST VTRBF
	DBNZ C, g_anims_tick_loop
	CLR1 VSEL, 4
	RET
g_anims_tick_empty:
	LD VTRBF
	LD VTRBF
	LD VTRBF
	DBNZ C, g_anims_tick_loop
	CLR1 VSEL, 4
	RET
