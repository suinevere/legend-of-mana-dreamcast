;----------------------
; | clobber.s
; | Description: Loads ACC, B, C, TRL, TRH, FPR and R0 with known values before each of FM_PRD_EX, FM_VRF_EX, the BIOS page write and TIMER_EX, and logs all seven after each, so a test sees what each routine leaves; the page write comes back through FM_WRT_EX's RET at 0x105.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c, pocketstation/src/vmu_rt/rt.inc
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_rt/rt.inc"
ACC	EQU	$100
B	EQU	$102
C	EQU	$103
TRL	EQU	$104
TRH	EQU	$105
IE	EQU	$108
EXT	EQU	$10D
FPR	EQU	$154
PCON	EQU	$107

	.org $0000
	JMPF start

	.org $0100
fm_wrt:
	NOT1 EXT, 0
	JMPF $0100
	RET

	.org $0110
fm_vrf:
	NOT1 EXT, 0
	JMPF $0110
	RET

	.org $0120
fm_prd:
	NOT1 EXT, 0
	JMPF $0120
	RET

	.org $0130
timer:
	NOT1 EXT, 0
	JMPF $0130

	.org $0139
	RET

	.org $0150
fm_page:
	MOV #$03, ACC
	PUSH ACC
	MOV #$01, ACC
	PUSH ACC
	NOT1 EXT, 0
	JMPF $E024

	.org $0200
start:
	CLR1 IE, 7
	MOV #$01, $7C
	MOV #$01, $7D
	MOV #$C8, $7E
	MOV #$00, $7F
	CALL load
	CALLF fm_prd
	CALL dump
	CALL load
	CALLF fm_vrf
	CALL dump
	CALL load
	CALLF fm_page
	CALL dump
	CALL load
	CALLF timer
	CALL dump
	SET1 PCON, 0

;----------------------
; | load
; | Description: Sets B 0x22, C 0x33, TRL 0x44, TRH 0x55, FPR 0x01, R0 0x66 and ACC 0x11.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: the registers above
;----------------------
load:
	MOV #$22, B
	MOV #$33, C
	MOV #$44, TRL
	MOV #$55, TRH
	MOV #$01, FPR
	MOV #$66, $00
	MOV #$11, ACC
	RET

;----------------------
; | dump
; | Description: Writes ACC, B, C, TRL, TRH, FPR and R0 to RT_LOG in that order.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
dump:
	ST RT_LOG
	LD B
	ST RT_LOG
	LD C
	ST RT_LOG
	LD TRL
	ST RT_LOG
	LD TRH
	ST RT_LOG
	LD FPR
	ST RT_LOG
	LD $00
	ST RT_LOG
	RET
