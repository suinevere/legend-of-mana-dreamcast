;----------------------
; | rt_lcd.s
; | Description: The runtime's screen: where each frame buffer row lives in XRAM, the present mark, and the LCD switch.
; | Author: suinevere
; | Dependencies: sfr.inc, rt.inc, rt_input.s (RTI_FLAGS, RTI_F_LCD)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | rt_fb_row
; | Description: Where frame buffer row ROW lives: XRAM bank 0 holds rows 0-15 and bank 1 rows 16-31, each bank two 6-byte rows then a 4-byte gap from 0x180, so byte n of the row is at 0x100 + C + n (through @R2 or @R3).
; | Author: suinevere
; | Dependencies: rt_fb_rows
; | Globals: N/A
; | Params: ACC -- row, 0-31
; | Returns: B -- the XBNK value; C -- the low byte of the row's first address. Keeps TRL and TRH; destroys ACC.
;----------------------
rt_fb_row:
	PUSH TRL
	PUSH TRH
	ST B
	MOV #<rt_fb_rows, TRL
	MOV #>rt_fb_rows, TRH
	LDC
	ST C
	LD B
	ROL
	ROL
	ROL
	ROL
	AND #$01
	ST B
	POP TRH
	POP TRL
	RET

;----------------------
; | rt_fb_rows
; | Description: The 32-entry table rt_fb_row reads: each row's first XRAM address less 0x100, 0x80 + 16 * (row % 16 / 2) + 6 * (row % 2).
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
rt_fb_rows:
	.byte $80, $86, $90, $96, $A0, $A6, $B0, $B6, $C0, $C6, $D0, $D6, $E0, $E6, $F0, $F6
	.byte $80, $86, $90, $96, $A0, $A6, $B0, $B6, $C0, $C6, $D0, $D6, $E0, $E6, $F0, $F6

;----------------------
; | rt_present
; | Description: vmu_present: writes RT_PRESENT_PORT, the harness's frame point; on the VMU the XRAM rows are the screen already, so nothing else is due.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RT_PRESENT_PORT
; | Params: N/A
; | Returns: N/A
;----------------------
rt_present:
	MOV #$00, RT_PRESENT_PORT
	RET

;----------------------
; | rt_xram_clear
; | Description: Zeroes all six bytes of every screen row in XRAM banks 0 and 1, so the BIOS menu's leftovers do not show beside the game's 32 columns, which it never draws.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: XRAM rows 0-31, XBNK
; | Params: N/A (interrupts masked)
; | Returns: N/A. Destroys ACC, B, C, R2, PSW's flags; leaves XBNK at 1.
;----------------------
rt_xram_clear:
	MOV #$00, XBNK
	CALLF rt_xram_clear_bank
	MOV #$01, XBNK
rt_xram_clear_bank:
	MOV #$80, $02
	MOV #$08, C
rt_xram_clear_line:
	MOV #$0C, B
rt_xram_clear_byte:
	MOV #$00, @R2
	INC $02
	DBNZ B, rt_xram_clear_byte
	LD $02
	ADD #$04
	ST $02
	DBNZ C, rt_xram_clear_line
	RET

;----------------------
; | rt_lcd
; | Description: vmu_lcd: turns the display on or off through VCCR bit 7, keeping XRAM, and records it in RTI_F_LCD, as VCCR cannot be read back and Sleep's wake restores it from there.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: rt_input (RTI_FLAGS), VCCR
; | Params: ACC -- nonzero for on
; | Returns: N/A
;----------------------
rt_lcd:
	BZ rt_lcd_off
	SET1 RTI_FLAGS, RTI_F_LCD
	MOV #$80, VCCR
	RET
rt_lcd_off:
	CLR1 RTI_FLAGS, RTI_F_LCD
	MOV #$00, VCCR
	RET
