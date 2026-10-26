;----------------------
; | system.s
; | Description: system.c in LC86K assembly: the LCD switch-on, the interrupt slots' handler ids, written and read, and the item table's entries.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/system.c, pocketstation/build/vmu_game/rrl_ram.inc, pocketstation/src/vmu_rt/rt_lcd.s (rt_lcd), tables.s (app_tb_80), rng.s (ROM_WORDS_8094)
; | Globals: irq_handler, fiq_driver
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_lcd_mode
; | Description: game_lcd_mode: turns the LCD on, tail-calling rt_lcd with 1.
; | Author: suinevere
; | Dependencies: rt_lcd
; | Globals: N/A
; | Params: N/A
; | Returns: N/A. Destroys ACC, PSW.
;----------------------
g_lcd_mode:
	MOV #$01, ACC
	JMPF rt_lcd

;----------------------
; | g_set_irq_handler
; | Description: game_set_irq_handler: stores a handler id in fiq_driver for line 13, else in irq_handler's slot for the line.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_IRQ_HANDLER, HW_FIQ_DRIVER)
; | Globals: irq_handler, fiq_driver
; | Params: ACC -- line, 0-13; B -- handler id
; | Returns: N/A. Destroys ACC, VRMAD, PSW.
;----------------------
g_set_irq_handler:
	BE #$0D, g_set_irq_handler_fiq
	ADD #<HWW_IRQ_HANDLER
	ST VRMAD1
	MOV #>HWW_IRQ_HANDLER, VRMAD2
	LD B
	ST VTRBF
	RET
g_set_irq_handler_fiq:
	LD B
	ST HW_FIQ_DRIVER
	RET

;----------------------
; | g_slot_handler
; | Description: slot_handler: the handler id for a line, fiq_driver for line 13, else irq_handler's slot for the line.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HWW_IRQ_HANDLER, HW_FIQ_DRIVER)
; | Globals: irq_handler, fiq_driver
; | Params: ACC -- line, 0-13
; | Returns: ACC -- the handler id. Destroys VRMAD, PSW.
;----------------------
g_slot_handler:
	BE #$0D, g_slot_handler_fiq
	ADD #<HWW_IRQ_HANDLER
	ST VRMAD1
	MOV #>HWW_IRQ_HANDLER, VRMAD2
	LD VTRBF
	RET
g_slot_handler_fiq:
	LD HW_FIQ_DRIVER
	RET

;----------------------
; | g_rom_word_8094
; | Description: game_rom_word_8094: entry index of the item table without its top bit, read with LDC from the kept page run.
; | Author: suinevere
; | Dependencies: app_tb_80, ROM_WORDS_8094 (rng.s)
; | Globals: N/A
; | Params: ACC -- index
; | Returns: TRH:TRL -- the entry & 0x7FFF. Destroys ACC, B, C, PSW.
;----------------------
g_rom_word_8094:
	ST B
	MOV #<(app_tb_80+ROM_WORDS_8094), TRL
	MOV #>(app_tb_80+ROM_WORDS_8094), TRH
	BN B, 7, g_rom_word_8094_low
	INC TRH
g_rom_word_8094_low:
	LD B
	ADD B
	ST C
	INC ACC
	LDC
	AND #$7F
	ST B
	LD C
	LDC
	ST TRL
	LD B
	ST TRH
	RET

;----------------------
; | g_quit
; | Description: game_quit: powers the game down and leaves to the BIOS. It never returns, so it is checked by reading.
; | Author: suinevere
; | Dependencies: g_power_down, rt_power, rt.inc (RT_POWER_EXIT)
; | Globals: N/A
; | Params: N/A
; | Returns: never
;----------------------
g_quit:
	CALLF g_power_down
	MOV #RT_POWER_EXIT, ACC
	JMPF rt_power

;----------------------
; | g_dock_quit
; | Description: game_dock_quit: powers the game down and leaves to the BIOS as docked. It never returns, so it is checked by reading.
; | Author: suinevere
; | Dependencies: g_power_down, rt_power, rt.inc (RT_POWER_EXIT_DOCKED)
; | Globals: N/A
; | Params: N/A
; | Returns: never
;----------------------
g_dock_quit:
	CALLF g_power_down
	MOV #RT_POWER_EXIT_DOCKED, ACC
	JMPF rt_power

;----------------------
; | g_dock_check
; | Description: game_dock_check: with the dock line raised quits as docked; else clears docked and puts the dock handler in irq slot 10.
; | Author: suinevere
; | Dependencies: rt_irq_levels, g_dock_quit, g_set_irq_handler, rt.inc (RT_LINE_DOCK), rrl_ram.inc (HW_DOCKED)
; | Globals: docked, irq_handler
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, TRH, TRL, VRMAD, PSW.
;----------------------
g_dock_check:
	CALLF rt_irq_levels
	LD TRH
	AND #>RT_LINE_DOCK
	BZ g_dock_check_off
	JMPF g_dock_quit
g_dock_check_off:
	MOV #$00, HW_DOCKED
	MOV #$04, B
	MOV #$0A, ACC
	JMPF g_set_irq_handler

;----------------------
; | g_call_handler
; | Description: game_call_handler: runs the game handler id names (buttons, timer tick, sound, dock) with the interrupt line; any other id does nothing.
; | Author: suinevere
; | Dependencies: g_buttons_irq, g_timer_tick, g_sound_fiq, g_dock_irq
; | Globals: what the handlers write
; | Params: ACC -- id; TRH:TRL -- line
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R1, VRMAD, PSW.
;----------------------
g_call_handler:
	BNE #$01, g_call_handler_tick
	LD TRL
	JMPF g_buttons_irq
g_call_handler_tick:
	BNE #$02, g_call_handler_sound
	JMPF g_timer_tick
g_call_handler_sound:
	BNE #$03, g_call_handler_dock
	JMPF g_sound_fiq
g_call_handler_dock:
	BNE #$04, g_call_handler_ret
	JMPF g_dock_irq
g_call_handler_ret:
	RET

;----------------------
; | g_irq_dispatch
; | Description: game_irq_dispatch: for each pending line from 13 down, calls its slot's handler with the line's bit, stopping once no pending bit is left.
; | Author: suinevere
; | Dependencies: g_slot_handler, g_call_handler
; | Globals: fiq_driver, irq_handler and what the handlers write
; | Params: TRH:TRL -- pending
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0, R1, VRMAD, PSW; five bytes of stack on top of the handler's.
;----------------------
g_irq_dispatch:
	MOV #13, C
	MOV #$00, $00
	MOV #$20, $01
g_irq_dispatch_line:
	LD TRL
	AND $00
	ST B
	LD TRH
	AND $01
	OR B
	BZ g_irq_dispatch_next
	LD C
	CALLF g_slot_handler
	BZ g_irq_dispatch_clear
	PUSH TRL
	PUSH TRH
	PUSH C
	PUSH $00
	PUSH $01
	ST B
	LD $00
	ST TRL
	LD $01
	ST TRH
	LD B
	CALLF g_call_handler
	POP $01
	POP $00
	POP C
	POP TRH
	POP TRL
g_irq_dispatch_clear:
	LD $00
	XOR #$FF
	AND TRL
	ST TRL
	LD $01
	XOR #$FF
	AND TRH
	ST TRH
	OR TRL
	BNZ g_irq_dispatch_next
	RET
g_irq_dispatch_next:
	CLR1 PSW, 7
	LD $01
	RORC
	ST $01
	LD $00
	RORC
	ST $00
	DEC C
	LD $01
	OR $00
	BNZ g_irq_dispatch_line
	RET

;----------------------
; | g_irq_callback
; | Description: game_irq_callback: dispatches the pending lines that are enabled, then acknowledges them, leaving the RTC line latched.
; | Author: suinevere
; | Dependencies: rt_irq_pending, rt_irq_mask, g_irq_dispatch, rt_irq_ack
; | Globals: what the handlers write
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0, R1, VRMAD, PSW; two bytes of stack on top of g_irq_dispatch's.
;----------------------
g_irq_callback:
	CALLF rt_irq_pending
	LD TRL
	ST B
	LD TRH
	ST C
	CALLF rt_irq_mask
	LD TRL
	AND B
	ST TRL
	LD TRH
	AND C
	ST TRH
	PUSH TRL
	PUSH TRH
	CALLF g_irq_dispatch
	POP TRH
	POP TRL
	LD TRH
	AND #$FD
	ST TRH
	JMPF rt_irq_ack
