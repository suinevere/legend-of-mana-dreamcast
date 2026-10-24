;----------------------
; | rt_ram.s
; | Description: vmu_ram on the VMU: packed offsets below 0x100 are RAM bank 1 at the same address, 0x100-0x2FF are work RAM at offset minus 0x100. Offsets 0x000-0x01F are the runtime's own block (rt_reserved).
; | Author: suinevere
; | Dependencies: sfr.inc, pocketstation/tools/gen_ram.py (the placement it mirrors)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | rt_r8
; | Description: Reads a vmu_ram byte through R0, which lives in the runtime's block at bank 1 0x00, with interrupts masked so a handler's own access cannot retarget R0 or VRMAD.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: vmu_ram (bank 1, work RAM), R0
; | Params: TRH:TRL -- packed offset, 0x020-0x2FF (below 0x020 is the runtime's block)
; | Returns: ACC -- the byte. Destroys R0, and VRMAD1 and VRMAD2 for a work RAM offset; IE is restored.
;----------------------
rt_r8:
	PUSH IE
	CLR1 IE, 7
	LD TRH
	BNZ rt_r8_wram
	LD TRL
	ST $00
	LD @R0
	POP IE
	RET
rt_r8_wram:
	DEC ACC
	ST VRMAD2
	LD TRL
	ST VRMAD1
	LD VTRBF
	POP IE
	RET

;----------------------
; | rt_w8
; | Description: Writes a vmu_ram byte through R0, which lives in the runtime's block at bank 1 0x00, with interrupts masked so a handler's own access cannot retarget R0 or VRMAD.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: vmu_ram (bank 1, work RAM), R0
; | Params: TRH:TRL -- packed offset, 0x020-0x2FF (below 0x020 is the runtime's block); ACC -- value
; | Returns: N/A. Destroys R0, and VRMAD1 and VRMAD2 for a work RAM offset; ACC, TRH, TRL and IE are kept.
;----------------------
rt_w8:
	PUSH IE
	CLR1 IE, 7
	XCH TRH
	BNZ rt_w8_wram
	XCH TRH
	XCH TRL
	ST $00
	XCH TRL
	ST @R0
	POP IE
	RET
rt_w8_wram:
	DEC ACC
	ST VRMAD2
	INC ACC
	XCH TRH
	XCH TRL
	ST VRMAD1
	XCH TRL
	ST VTRBF
	POP IE
	RET

;----------------------
; | rt_ram_clear
; | Description: Clears all of RAM bank 1 and all 512 bytes of work RAM, as vmu_host_init starts the game.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: vmu_ram (bank 1, work RAM)
; | Params: N/A
; | Returns: N/A. Destroys VRMAD1 and VRMAD2.
;----------------------
rt_ram_clear:
	MOV #$FF, $00
rt_ram_clear_bank1:
	MOV #$00, @R0
	DBNZ $00, rt_ram_clear_bank1
	MOV #$00, VRMAD1
	MOV #$00, VRMAD2
	SET1 VSEL, 4
rt_ram_clear_wram:
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	DBNZ $00, rt_ram_clear_wram
	CLR1 VSEL, 4
	RET
