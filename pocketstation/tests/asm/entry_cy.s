;----------------------
; | entry_cy.s
; | Description: A --call test image whose g_random adds the seed's first 1 with ADDC, so it is right only while CY happens to be clear at entry; the harness enters with CY set, and call_compare must fail it.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_game/callbase.inc, pocketstation/build/vmu_game/rrl_ram.inc, pocketstation/tools/call_compare.py
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_game/callbase.inc"

g_random:
	LD HW_SEED
	ADDC #$01
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


app_tables:
