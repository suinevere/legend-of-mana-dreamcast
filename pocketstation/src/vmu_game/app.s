;----------------------
; | app.s
; | Description: app.c in LC86K assembly: the quit request read, the idle count setter, the quit dialog, the entry and the main loop; and g_pet_check, which the GAME runs first to see that a pet save is there.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/app.c, pocketstation/build/vmu_game/rrl_ram.inc, pocketstation/src/vmu_rt/rt_input.s (rt_sys_flags), pocketstation/src/vmu_rt/rt_file.s (rt_save)
; | Globals: seed, cycle16, mode, btn_prev, btn_press, idle_lo, repeat_up
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | g_quit_requested
; | Description: quit_requested: reads the system flags once and returns 1 when VMU_SYS_QUIT (bit 1) is set, else 0.
; | Author: suinevere
; | Dependencies: rt_sys_flags
; | Globals: N/A
; | Params: N/A
; | Returns: ACC -- 1 or 0. Destroys B, C, PSW (rt_sys_flags); two bytes of stack.
;----------------------
g_quit_requested:
	CALLF rt_sys_flags
	ROR
	AND #$01
	RET

;----------------------
; | g_set_idle
; | Description: set_idle: stores the idle count's low byte in idle_lo and its high byte in repeat_up, where the original kept the halfword.
; | Author: suinevere
; | Dependencies: rrl_ram.inc (HW_IDLE_LO, HW_REPEAT_UP)
; | Globals: idle_lo, repeat_up
; | Params: TRH:TRL -- n, the idle count
; | Returns: N/A. Destroys ACC.
;----------------------
g_set_idle:
	LD TRL
	ST HW_IDLE_LO
	LD TRH
	ST HW_REPEAT_UP
	RET

;----------------------
; | g_pet_check
; | Description: Whether the VMU holds a pet save: rt_save_find leaves its sector count in rt_save[0], 0 without one.
; | Author: suinevere
; | Dependencies: rt_save (rt_file.s)
; | Globals: rt_save (WRAM), VRMAD1, VRMAD2, VSEL
; | Params: N/A
; | Returns: ACC -- 0 when there is a pet save, 1 when not. Destroys VRMAD and PSW's flags; leaves VSEL bit 4 clear.
;----------------------
g_pet_check:
	MOV #RT_SAVE_HI, VRMAD2
	MOV #RT_SAVE_LO, VRMAD1
	CLR1 VSEL, 4
	LD VTRBF
	BZ g_pet_check_none
	MOV #$00, ACC
	RET
g_pet_check_none:
	MOV #$01, ACC
	RET

;----------------------
; | g_quit_dialog
; | Description: game_quit_dialog: shows the yes/no quit box until fire, up and down flipping the choice with a beep; yes quits, as does a system quit request. Keeps ARG_BLOCK.
; | Author: suinevere
; | Dependencies: g_sound_stop, g_quit_sound, g_n_arg, g_wait_ticks, g_read_buttons, g_quit_requested, g_quit, g_fb_clear, g_dock_screen, g_qd_rows, g_fb_present, game.inc (ARG_BLOCK), rrl_ram.inc (HW_BTN_PRESS)
; | Globals: btn_press and what the callees write
; | Params: N/A
; | Returns: N/A, and only for no. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear; five bytes of stack on top of its callees'.
;----------------------
g_quit_dialog:
	SET1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	PUSH VTRBF
	CLR1 VSEL, 4
	MOV #$00, ACC
	PUSH ACC
	CALLF g_sound_stop
	CALLF g_quit_sound
	MOV #$00, ACC
	CALLF g_n_arg
	MOV #$01, ACC
	CALLF g_wait_ticks
	MOV #$00, ACC
	CALLF g_read_buttons
g_quit_dialog_loop:
	CALLF g_quit_requested
	BZ g_quit_dialog_show
	JMPF g_quit
g_quit_dialog_show:
	CALLF g_fb_clear
	CALLF g_dock_screen
	POP ACC
	PUSH ACC
	CALLF g_qd_rows
	MOV #$01, ACC
	CALLF g_n_arg
	MOV #$01, ACC
	CALLF g_wait_ticks
	CALLF g_fb_present
	MOV #$00, ACC
	CALLF g_read_buttons
	LD HW_BTN_PRESS
	AND #$01
	BZ g_quit_dialog_move
	CALLF g_quit_sound
	POP ACC
	BZ g_quit_dialog_no
	JMPF g_quit
g_quit_dialog_no:
	MOV #$00, ACC
	CALLF g_read_buttons
	MOV #<(ARG_BLOCK+3), VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	CLR1 VSEL, 4
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	DEC VRMAD1
	POP VTRBF
	RET
g_quit_dialog_move:
	LD HW_BTN_PRESS
	AND #$18
	BZ g_quit_dialog_loop
	POP ACC
	XOR #$01
	PUSH ACC
	CALLF g_quit_sound
	BR g_quit_dialog_loop

;----------------------
; | g_qd_rows
; | Description: For g_quit_dialog: the box's two answer rows from the table, the chosen one inverted.
; | Author: suinevere
; | Dependencies: g_fb_put_rows, game.inc (ARG_BLOCK)
; | Globals: the frame buffer
; | Params: ACC -- yes, 0 or 1
; | Returns: N/A. Destroys ACC, B, C, TRH, TRL, R0-R3, VRMAD, FPR, PSW; leaves VSEL bit 4 clear.
;----------------------
g_qd_rows:
	PUSH ACC
	MOV #$B0, ACC
	CALLF g_qd_src
	POP ACC
	PUSH ACC
	XOR #$01
	ST C
	MOV #$07, B
	MOV #$08, ACC
	CALLF g_fb_put_rows
	MOV #$CC, ACC
	CALLF g_qd_src
	POP C
	MOV #$07, B
	MOV #$11, ACC
	JMPF g_fb_put_rows

;----------------------
; | g_qd_src
; | Description: For g_qd_rows: ARG_BLOCK+0..3 becomes card address 0x02006400 + ACC.
; | Author: suinevere
; | Dependencies: game.inc (ARG_BLOCK)
; | Globals: N/A
; | Params: ACC -- the low byte
; | Returns: N/A. Destroys VRMAD; leaves VSEL bit 4 clear.
;----------------------
g_qd_src:
	CLR1 VSEL, 4
	MOV #<ARG_BLOCK, VRMAD1
	MOV #>ARG_BLOCK, VRMAD2
	ST VTRBF
	INC VRMAD1
	MOV #$64, VTRBF
	INC VRMAD1
	MOV #$00, VTRBF
	INC VRMAD1
	MOV #$02, VTRBF
	RET

;----------------------
; | G_IDLE_FRAMES
; | Description: app.c's IDLE_FRAMES: the idle counter's restart value, 0xF00 frames without a button before standby.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
G_IDLE_FRAMES	EQU	$0F00

;----------------------
; | G_ENTRY_LINES
; | Description: The interrupt lines game_entry unmasks, RT_LINE_RTC and RT_LINE_DOCK together.
; | Author: suinevere
; | Dependencies: rt.inc (RT_LINE_RTC, RT_LINE_DOCK)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
G_ENTRY_LINES	EQU	$0600

;----------------------
; | g_entry
; | Description: game_entry: masks every line, installs g_irq_callback as the handler, unmasks the RTC and dock lines and runs the main loop; vmu_ram is already clear, as the runtime clears RAM before app_main.
; | Author: suinevere
; | Dependencies: rt_irq_disable, rt_set_handler, rt_irq_enable, g_irq_callback, g_main_loop
; | Globals: the runtime's mask and handler
; | Params: N/A
; | Returns: never
;----------------------
g_entry:
	MOV #$FF, TRL
	MOV #$3F, TRH
	CALLF rt_irq_disable
	MOV #<g_irq_callback, TRL
	MOV #>g_irq_callback, TRH
	CALLF rt_set_handler
	MOV #<G_ENTRY_LINES, TRL
	MOV #>G_ENTRY_LINES, TRH
	CALLF rt_irq_enable

;----------------------
; | g_main_loop
; | Description: game_main_loop: sets up the screen, tick, sound and state block, then runs a frame per tick (mode frame, seed stir, present, buttons), opening the quit dialog on a long fire hold or mode 0xFF and standing by after 0xF00 idle frames. It never returns, so it is checked by the whole-game run.
; | Author: suinevere
; | Dependencies: g_dock_check, g_fb_clear, g_fb_present, g_ticks_start, g_sound_init, g_state_reset, g_mode0_leave, g_set_idle, g_buttons_init, g_lcd_mode, g_n_arg, g_wait_ticks, g_read_buttons, g_quit_requested, g_quit, g_dock_screen, g_sound_play_waiting, g_mode_frame, g_random, g_quit_dialog, g_standby, rrl_ram.inc (HW_SEED, HW_CYCLE16, HW_BTN_PREV, HW_IDLE_LO, HW_REPEAT_UP, HW_MODE)
; | Globals: seed, cycle16, mode, btn_prev, idle_lo, repeat_up and what the callees write
; | Params: N/A
; | Returns: never
;----------------------
g_main_loop:
	CALLF g_dock_check
	CALLF g_fb_clear
	CALLF g_fb_present
	MOV #RT_TICK_FRAME, ACC
	CALLF g_ticks_start
	CALLF g_sound_init
	CALLF g_state_reset
	MOV #$00, HW_SEED
	MOV #$00, HW_SEED+1
	MOV #$10, HW_CYCLE16
	CALLF g_mode0_leave
g_main_loop_start:
	MOV #<G_IDLE_FRAMES, TRL
	MOV #>G_IDLE_FRAMES, TRH
	CALLF g_set_idle
	MOV #$60, ACC
	CALLF g_buttons_init
	CALLF g_lcd_mode
	MOV #$00, ACC
	CALLF g_n_arg
	MOV #$01, ACC
	CALLF g_wait_ticks
	MOV #$00, ACC
	CALLF g_read_buttons
	MOV #$01, ACC
	CALLF g_n_arg
	MOV #$01, ACC
	CALLF g_wait_ticks
	MOV #$00, ACC
	CALLF g_read_buttons
g_main_loop_frame:
	CALLF g_quit_requested
	BZ g_main_loop_run
	JMPF g_quit
g_main_loop_run:
	CALLF g_fb_clear
	CALLF g_dock_screen
	CALLF g_sound_play_waiting
	DEC HW_CYCLE16
	LD HW_CYCLE16
	BNZ g_main_loop_cycled
	MOV #$10, HW_CYCLE16
g_main_loop_cycled:
	CALLF g_mode_frame
	MOV #$00, TRL
	MOV #$01, TRH
	CALLF g_random
	MOV #$01, ACC
	CALLF g_n_arg
	MOV #$01, ACC
	CALLF g_wait_ticks
	CALLF g_fb_present
	MOV #$01, ACC
	CALLF g_read_buttons
	BZ g_main_loop_held
	CALLF g_quit_dialog
g_main_loop_held:
	MOV #<G_IDLE_FRAMES, TRL
	MOV #>G_IDLE_FRAMES, TRH
	LD HW_BTN_PREV
	AND #$1F
	BNZ g_main_loop_set
	LD HW_IDLE_LO
	SUB #$01
	ST TRL
	LD HW_REPEAT_UP
	SUBC #$00
	ST TRH
	OR TRL
	BNZ g_main_loop_set
	CALLF g_fb_clear
	CALLF g_fb_present
	CALLF g_standby
	JMPF g_main_loop_start
g_main_loop_set:
	CALLF g_set_idle
	LD HW_MODE
	XOR #$FF
	BNZ g_main_loop_frame
	MOV #$00, HW_MODE
	CALLF g_mode0_leave
	CALLF g_quit_dialog
	JMPF g_main_loop_frame
