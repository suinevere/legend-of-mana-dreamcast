;----------------------
; | undeclared.s
; | Description: A --call test image whose g_random is the real one but also sets VRMAD1, and whose g_sound_on sounds tone 96 as game_sound_on does but leaves PSW's indirect bank bit set, so call_compare must fail each on the register its manifest row does not declare.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_game/callbase.inc, pocketstation/src/vmu_game/rng.s, pocketstation/tools/call_compare.py
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_game/callbase.inc"

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
	MOV #$10, VRMAD1
	RET

g_sound_on:
	MOV #$60, ACC
	CALLF rt_buzz
	SET1 PSW, 3
	RET


app_tables:
