;----------------------
; | page_call.s
; | Description: Programs flash page 0x1C800 in bank 1 through the BIOS page write at ROM 0xE024, entered with ROM 0x0103 pushed as its return so it comes back through FM_WRT_EX's tail, checks it with FM_VRF_EX, reads it back through FM_PRD_EX and exits with RAM 0x81; a failed check exits 0xEE, and a read of more than 128 bytes, reaching RAM 0x40, exits 0xED.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ACC	EQU	$100
EXT	EQU	$10D

	.org $0000
	JMPF start

;----------------------
; | fm_wrt / fm_vrf / fm_prd / exit
; | Description: The GAME's stubs for the BIOS flash write, verify, read and mode-change entries; the BIOS returns to the stub's RET, and fm_wrt's RET at 0x105 is also where the page write comes back.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: RAM bank 1 0x7D-0x7F -- flash bank and address; 0x80-0xFF -- the page
; | Returns: ACC -- 0x00 when fm_vrf matches; exit never returns
;----------------------
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

;----------------------
; | fm_page
; | Description: The BIOS page write (american_v1.05.s label_E024, label_E3CE): pushes ROM 0x0103 as the return address, so the ROM's RET lands on FM_WRT_EX's not1 ext,0 and jmpf 0x0105 back into flash.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: RAM bank 1 0x7C -- 1 for data polling; 0x7D-0x7F -- flash bank and page address; 0x80-0xFF -- the page
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH, FPR, R0.
;----------------------
	.org $0150
fm_page:
	MOV #$03, ACC
	PUSH ACC
	MOV #$01, ACC
	PUSH ACC
	NOT1 EXT, 0
	JMPF $E024

	.org $01F0
exit:
	NOT1 EXT, 0
	JMPF $01F0

	.org $0200
start:
	MOV #$80, $00
fill:
	LD $00
	XOR #$5A
	ST @R0
	INC $00
	LD $00
	BNZ fill
	MOV #$01, $7C
	MOV #$01, $7D
	MOV #$C8, $7E
	MOV #$00, $7F
	CALLF fm_page
	CALLF fm_vrf
	BZ verified
	MOV #$EE, ACC
	JMPF exit
verified:
	MOV #$00, $7F
	MOV #$80, $00
clear:
	MOV #$00, @R0
	INC $00
	LD $00
	BNZ clear
	MOV #$A5, $40
	CALLF fm_prd
	LD $40
	BE #$A5, read_one
	MOV #$ED, ACC
	JMPF exit
read_one:
	LD $81
	JMPF exit
