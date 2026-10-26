;----------------------
; | input.s
; | Description: input.c in LC86K assembly: the buttons' interrupt latch, one direction's key repeat step, the buttons' setup and their poll each frame.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/input.c, pocketstation/build/vmu_game/rrl_ram.inc, game.inc (ARG_BLOCK), pocketstation/src/vmu_rt/rt_events.s (rt_irq_disable, rt_irq_enable)
; | Globals: btn_latch, btn_prev, btn_new, btn_press, hold_reload, hold_count, repeat_up, repeat_down, repeat_left, repeat_right, irq_handler
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_buttons_irq
; | Description: game_buttons_irq: latches the pressed button lines (bits 0-4) in btn_latch and disables those lines until the next poll, tail-calling rt_irq_disable.
; | Author: suinevere
; | Dependencies: rt_irq_disable, rrl_ram.inc (HW_BTN_LATCH)
; | Globals: btn_latch
; | Params: ACC -- lines, of which only bits 0-4 count
; | Returns: N/A. Destroys ACC, TRL, TRH, PSW (rt_irq_disable).
;----------------------
g_buttons_irq:
	AND #$1F
	ST TRL
	OR HW_BTN_LATCH
	ST HW_BTN_LATCH
	MOV #$00, TRH
	JMPF rt_irq_disable

;----------------------
; | g_repeat_step
; | Description: repeat_step: one direction's repeat count; unchanged while the button is up, 0x16 on a fresh press, else counted down, and on reaching 0 the bit is set in *press and the count restarts at 6.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- count; B -- bit; C -- held; ARG_BLOCK+0 -- fresh; ARG_BLOCK+1 -- *press
; | Returns: ACC -- the new count; ARG_BLOCK+1 -- *press. Destroys TRL, VRMAD; clears VSEL bit 4 (work RAM auto-increment) when the button is held.
;----------------------
g_repeat_step:
	ST TRL
	LD C
	AND B
	BZ g_repeat_step_count
	CLR1 VSEL, 4
	MOV #>ARG_BLOCK, VRMAD2
	MOV #<ARG_BLOCK, VRMAD1
	LD VTRBF
	AND B
	BNZ g_repeat_step_fresh
	DEC TRL
	LD TRL
	BNZ g_repeat_step_ret
	INC VRMAD1
	LD VTRBF
	OR B
	ST VTRBF
	MOV #$06, ACC
	RET
g_repeat_step_count:
	LD TRL
g_repeat_step_ret:
	RET
g_repeat_step_fresh:
	MOV #$16, ACC
	RET

;----------------------
; | g_buttons_init
; | Description: game_buttons_init: sets the fire hold's reload and count to hold, clears the button state, gives lines 0-4 the buttons handler (id 1) and enables them, tail-calling rt_irq_enable.
; | Author: suinevere
; | Dependencies: rt_irq_enable, rrl_ram.inc (HW_HOLD_RELOAD, HW_HOLD_COUNT, HW_BTN_LATCH, HW_BTN_PREV, HW_BTN_NEW, HW_BTN_PRESS, HWW_IRQ_HANDLER)
; | Globals: hold_reload, hold_count, btn_latch, btn_prev, btn_new, btn_press, irq_handler
; | Params: ACC -- hold
; | Returns: N/A. Destroys ACC, TRH, TRL, VRMAD, PSW; leaves VSEL bit 4 (work RAM auto-increment) clear.
;----------------------
g_buttons_init:
	ST HW_HOLD_RELOAD
	ST HW_HOLD_COUNT
	MOV #$00, HW_BTN_LATCH
	MOV #$00, HW_BTN_PREV
	MOV #$00, HW_BTN_NEW
	MOV #$00, HW_BTN_PRESS
	MOV #<HWW_IRQ_HANDLER, VRMAD1
	MOV #>HWW_IRQ_HANDLER, VRMAD2
	SET1 VSEL, 4
	MOV #$01, VTRBF
	MOV #$01, VTRBF
	MOV #$01, VTRBF
	MOV #$01, VTRBF
	MOV #$01, VTRBF
	CLR1 VSEL, 4
	MOV #$1F, TRL
	MOV #$00, TRH
	JMPF rt_irq_enable

;----------------------
; | g_rb_step
; | Description: repeat_step in registers for g_read_buttons: a direction's count, kept while up, 0x16 on a fresh press, else counted down. At 0 it sets the bit in press and restarts at 6.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: ACC -- count; R0 -- bit; B -- held; C -- fresh; TRL -- press
; | Returns: ACC -- the new count; TRL -- press. Destroys R1, PSW.
;----------------------
g_rb_step:
	ST $01
	LD B
	AND $00
	BZ g_rb_step_keep
	LD C
	AND $00
	BNZ g_rb_step_fresh
	DEC $01
	LD $01
	BNZ g_rb_step_ret
	LD TRL
	OR $00
	ST TRL
	MOV #$06, ACC
	RET
g_rb_step_keep:
	LD $01
g_rb_step_ret:
	RET
g_rb_step_fresh:
	MOV #$16, ACC
	RET

;----------------------
; | g_read_buttons
; | Description: game_read_buttons: polls the latched buttons into prev, new and press with the four directions' repeats, counts the fire hold when asked, clears the latch and re-enables the button lines.
; | Author: suinevere
; | Dependencies: g_rb_step, rt_irq_enable, rrl_ram.inc (HW_BTN_LATCH, HW_BTN_PREV, HW_BTN_NEW, HW_BTN_PRESS, HW_HOLD_COUNT, HW_HOLD_RELOAD, HW_REPEAT_UP, HW_REPEAT_DOWN, HW_REPEAT_LEFT, HW_REPEAT_RIGHT)
; | Globals: btn_latch, btn_prev, btn_new, btn_press, hold_count, hold_reload, repeat_up, repeat_down, repeat_left, repeat_right
; | Params: ACC -- check_hold, nonzero to count a fire-only hold down
; | Returns: ACC -- 1 when the fire hold ran out (its count reloads), else 0. Destroys B, C, TRH, TRL, R0, R1, PSW; three bytes of stack.
;----------------------
g_read_buttons:
	MOV #$00, TRH
	BZ g_read_buttons_reload
	LD HW_BTN_LATCH
	BNE #$01, g_read_buttons_reload
	DEC HW_HOLD_COUNT
	LD HW_HOLD_COUNT
	BNZ g_read_buttons_poll
	MOV #$01, TRH
g_read_buttons_reload:
	LD HW_HOLD_RELOAD
	ST HW_HOLD_COUNT
g_read_buttons_poll:
	LD HW_BTN_LATCH
	ST B
	XOR HW_BTN_PREV
	AND B
	ST C
	LD B
	ST HW_BTN_PREV
	LD C
	ST HW_BTN_NEW
	ST TRL
	MOV #$10, $00
	LD HW_REPEAT_UP
	CALLF g_rb_step
	ST HW_REPEAT_UP
	MOV #$08, $00
	LD HW_REPEAT_DOWN
	CALLF g_rb_step
	ST HW_REPEAT_DOWN
	MOV #$04, $00
	LD HW_REPEAT_LEFT
	CALLF g_rb_step
	ST HW_REPEAT_LEFT
	MOV #$02, $00
	LD HW_REPEAT_RIGHT
	CALLF g_rb_step
	ST HW_REPEAT_RIGHT
	LD TRL
	ST HW_BTN_PRESS
	MOV #$00, HW_BTN_LATCH
	PUSH TRH
	MOV #$1F, TRL
	MOV #$00, TRH
	CALLF rt_irq_enable
	POP ACC
	RET
