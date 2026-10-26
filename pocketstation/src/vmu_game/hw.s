;----------------------
; | hw.s
; | Description: hw.c in LC86K assembly: a timer's interrupt line, the original's 32-by-16 restoring division and a timer line's switch on or off, and the capped add of one strip to another.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/hw.c, game.inc (ARG_BLOCK), pocketstation/src/vmu_rt/rt_events.s
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_timer_line
; | Description: timer_line: the interrupt line of timer n, the tick line 0x100 for timer 1, else the sound line 0x2000.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- n
; | Returns: TRH:TRL -- the line. Destroys ACC.
;----------------------
g_timer_line:
	MOV #$00, TRL
	MOV #$01, TRH
	XOR #$01
	BZ g_timer_line_ret
	MOV #$20, TRH
g_timer_line_ret:
	RET

;----------------------
; | g_divide
; | Description: game_divide: sixteen restoring steps of n by d as the original ran them, a step subtracting d from n's top half when the bit shifted out was set or the top half reaches d; the quotient bits fill n's low half, leaving the remainder on top.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ARG_BLOCK+0..3 -- n, 32-bit; TRH:TRL -- d, 16-bit
; | Returns: TRH:TRL -- the quotient; ARG_BLOCK+4..7 -- rem, the top half left. Destroys ACC, B, C, R0-R3, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_divide:
	SET1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	ST $00
	LD VTRBF
	ST $01
	LD VTRBF
	ST $02
	LD VTRBF
	ST $03
	MOV #$10, C
g_divide_step:
	LD $00
	ADD $00
	ST $00
	LD $01
	ADDC $01
	ST $01
	LD $02
	ADDC $02
	ST $02
	LD $03
	ADDC $03
	ST $03
	BP PSW, 7, g_divide_take
	LD $02
	SUB TRL
	ST B
	LD $03
	SUBC TRH
	BP PSW, 7, g_divide_next
	ST $03
	LD B
	ST $02
	INC $00
	DBNZ C, g_divide_step
	BR g_divide_done
g_divide_take:
	LD $02
	SUB TRL
	ST $02
	LD $03
	SUBC TRH
	ST $03
	INC $00
g_divide_next:
	DBNZ C, g_divide_step
g_divide_done:
	LD $02
	ST VTRBF
	LD $03
	ST VTRBF
	MOV #$00, VTRBF
	MOV #$00, VTRBF
	CLR1 VSEL, 4
	LD $00
	ST TRL
	LD $01
	ST TRH
	RET

;----------------------
; | g_timer_irq
; | Description: game_timer_irq: for timer n's line, on acknowledges then enables it, off disables then acknowledges it.
; | Author: suinevere
; | Dependencies: g_timer_line, rt_irq_ack, rt_irq_enable, rt_irq_disable
; | Globals: N/A
; | Params: B -- n, the timer (every caller passes 1 or 2); ACC -- on
; | Returns: N/A. Destroys ACC, C, TRH, TRL, PSW.
;----------------------
g_timer_irq:
	ST C
	LD B
	CALLF g_timer_line
	LD C
	BZ g_timer_irq_off
	CALLF rt_irq_ack
	JMPF rt_irq_enable
g_timer_irq_off:
	CALLF rt_irq_disable
	JMPF rt_irq_ack

;----------------------
; | g_add_capped
; | Description: game_add_capped: takes src from ARG_BLOCK, a card address when its top byte is 2 or more and else a vmu_ram offset, and runs g_add_capped_core. A card src keeps only its low 16 bits, the file offset.
; | Author: suinevere
; | Dependencies: g_add_capped_core, game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- count, 1-255 (callers pass a strip's length); TRH:TRL -- dst; B -- add (lap_mark plus at most 10, or bag_kind_count / 2); ARG_BLOCK+0..3 -- src
; | Returns: ACC -- the last byte written. Destroys C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear; seventeen bytes of stack on the card path through rt_rom8.
;----------------------
g_add_capped:
	PUSH ACC
	LD TRL
	ST $02
	LD TRH
	ST $03
	SET1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	LD VTRBF
	ST TRL
	LD VTRBF
	ST TRH
	LD VTRBF
	LD VTRBF
	CLR1 VSEL, 4
	MOV #$00, C
	SUB #$02
	BP PSW, 7, g_add_capped_go
	INC C
g_add_capped_go:
	POP ACC

;----------------------
; | g_add_capped_core
; | Description: Writes count bytes of src plus add to dst, each capped at 9, reading src from work RAM, or through rt_rom8 when it is a file offset.
; | Author: suinevere
; | Dependencies: rt_rom8
; | Globals: N/A
; | Params: ACC -- count, 1-255; B -- add; C -- 0 when TRH:TRL is a vmu_ram offset in work RAM (0x100-0x2FF), 1 when a file offset; TRH:TRL -- src; R2:R3 -- dst, a vmu_ram offset in work RAM
; | Returns: ACC -- the last byte written. Keeps B; destroys TRH, TRL, R0-R3, VRMAD, FPR, PSW.
;----------------------
g_add_capped_core:
	ST $00
	DEC $03
	BP C, 0, g_add_capped_byte
	DEC TRH
g_add_capped_byte:
	BP C, 0, g_add_capped_rom
	LD TRL
	ST VRMAD1
	LD TRH
	ST VRMAD2
	LD VTRBF
	BR g_add_capped_add
g_add_capped_rom:
	CALLF rt_rom8
g_add_capped_add:
	ADD B
	BP PSW, 7, g_add_capped_cap
	ST $01
	SUB #$0A
	BP PSW, 7, g_add_capped_put
g_add_capped_cap:
	MOV #$09, $01
g_add_capped_put:
	LD $02
	ST VRMAD1
	LD $03
	ST VRMAD2
	LD $01
	ST VTRBF
	INC $02
	LD $02
	BNZ g_add_capped_src
	INC $03
g_add_capped_src:
	INC TRL
	LD TRL
	BNZ g_add_capped_next
	INC TRH
g_add_capped_next:
	DBNZ $00, g_add_capped_byte
	LD $01
	RET
