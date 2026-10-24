;----------------------
; | flash_call.s
; | Description: Checks FM_WRT_EX refuses bank 1 and addresses past the GAME's blocks and writes page 0x0300, then reads FAT page 0x1FC80 through FM_PRD_EX and exits with ACC from RAM 0x80.
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
; | fm_wrt / fm_prd / exit
; | Description: The GAME's own stubs for the BIOS flash write, flash read and mode-change entries; the BIOS returns to the stub's RET.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: RAM bank 1 0x7D-0x7F -- flash bank and address; 0x80-0xFF -- the page
; | Returns: ACC -- 0x00 or 0xFF from fm_wrt; exit never returns
;----------------------
	.org $0100
fm_wrt:
	NOT1 EXT, 0
	JMPF $0100
	RET

	.org $0120
fm_prd:
	NOT1 EXT, 0
	JMPF $0120
	RET

	.org $01F0
exit:
	NOT1 EXT, 0
	JMPF $01F0

	.org $0200
start:
	MOV #$80, $00
fill:
	LD $00
	ST @R0
	INC $00
	LD $00
	BNZ fill
	MOV #$01, $7D
	MOV #$03, $7E
	MOV #$00, $7F
	CALLF fm_wrt
	BE #$FF, bank0
	MOV #$EE, ACC
	JMPF exit
bank0:
	MOV #$00, $7D
	CALLF fm_wrt
	BZ beyond
	MOV #$ED, ACC
	JMPF exit
beyond:
	MOV #$04, $7E
	CALLF fm_wrt
	BE #$FF, read
	MOV #$EC, ACC
	JMPF exit
read:
	MOV #$01, $7D
	MOV #$FC, $7E
	MOV #$80, $7F
	CALLF fm_prd
	LD $80
	JMPF exit
