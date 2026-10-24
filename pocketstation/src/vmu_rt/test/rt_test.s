;----------------------
; | rt_test.s
; | Description: The runtime's test GAME: interprets the compiled scenario (scenario.py compile) built in from pocketstation/build/vmu_rt/scenario.bin, with scenario.py's test table as its tables, writing the event log to RT_LOG as scenario_host prints it. An op not implemented, or a bad argument, logs an error event and stops.
; | Author: suinevere
; | Dependencies: sfr.inc, rt.inc, rt_vectors.s, rt_ram.s, rt_events.s, rt_input.s, rt_lcd.s, rt_file.s, rt_sound.s, rt_power.s, pocketstation/tools/scenario.py, pocketstation/tests/host/scenario_host.c, pocketstation/src/vmu_rt/Makefile
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../sfr.inc"
	.include "../rt.inc"
	.include "../rt_vectors.s"

;----------------------
; | OP_END / OP_TICK / OP_SOUND / OP_SOUND_STOP / OP_ENABLE / OP_DISABLE / OP_ACK / OP_IDLE / OP_PRESS / OP_RELEASE / OP_DOCK / OP_SAVE_WRITE / OP_SAVE_READ / OP_ROM / OP_DRAW / OP_PRESENT / OP_BUZZ / OP_LCD / OP_POWER / OP_LOG_RAM / OP_RAM_WRITE / OP_LOG_BUTTONS / OP_LOG_SYS / OP_LOG_LEVELS / OP_LOG_RUN / OP_FILL / OP_MASK / OP_UNMASK
; | Description: The scenario op codes, as scenario.py's OPS table has them; the input ops are only stepped over, as the harness drives them.
; | Author: suinevere
; | Dependencies: pocketstation/tools/scenario.py
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
OP_END	EQU	$00
OP_TICK	EQU	$01
OP_SOUND	EQU	$02
OP_SOUND_STOP	EQU	$03
OP_ENABLE	EQU	$04
OP_DISABLE	EQU	$05
OP_ACK	EQU	$06
OP_IDLE	EQU	$07
OP_PRESS	EQU	$08
OP_RELEASE	EQU	$09
OP_DOCK	EQU	$0A
OP_SAVE_WRITE	EQU	$0B
OP_SAVE_READ	EQU	$0C
OP_ROM	EQU	$0D
OP_DRAW	EQU	$0E
OP_PRESENT	EQU	$0F
OP_BUZZ	EQU	$10
OP_LCD	EQU	$11
OP_POWER	EQU	$12
OP_LOG_RAM	EQU	$13
OP_RAM_WRITE	EQU	$14
OP_LOG_BUTTONS	EQU	$15
OP_LOG_SYS	EQU	$16
OP_LOG_LEVELS	EQU	$17
OP_LOG_RUN	EQU	$18
OP_FILL	EQU	$19
OP_MASK	EQU	$1A
OP_UNMASK	EQU	$1B

;----------------------
; | rt_test_header
; | Description: The VMS GAME header the BIOS reads at 0x200, with one blank icon after it.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.org $0200
rt_test_header:
	.byte "RT TEST         "
	.byte "VMU RUNTIME SCENARIO TEST       "
	.byte "RT_TEST         "
	.word 1, 8, 0, 0
	.word 0, 0

	.org $0480

;----------------------
; | app_main
; | Description: Installs the logging handler, then runs the scenario's ops in order until the end op, or until an op this GAME does not implement, which it logs as an error event.
; | Author: suinevere
; | Dependencies: rt_set_handler, rt_test_fetch, rt_test_handler and the rt_test_* ops
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
app_main:
	MOV #<rt_test_handler, TRL
	MOV #>rt_test_handler, TRH
	CALLF rt_set_handler
	MOV #<rt_test_scenario, TRL
	MOV #>rt_test_scenario, TRH
rt_test_loop:
	CALLF rt_test_fetch
	BNE #OP_END, rt_test_op_ram
	RET
rt_test_op_ram:
	BNE #OP_RAM_WRITE, rt_test_op_log
	JMPF rt_test_ram_write
rt_test_op_log:
	BNE #OP_LOG_RAM, rt_test_op_tick
	JMPF rt_test_log_ram
rt_test_op_tick:
	BNE #OP_TICK, rt_test_op_sound
	JMPF rt_test_tick
rt_test_op_sound:
	BNE #OP_SOUND, rt_test_op_stop
	JMPF rt_test_sound
rt_test_op_stop:
	BNE #OP_SOUND_STOP, rt_test_op_enable
	JMPF rt_test_sound_stop
rt_test_op_enable:
	BNE #OP_ENABLE, rt_test_op_disable
	JMPF rt_test_enable
rt_test_op_disable:
	BNE #OP_DISABLE, rt_test_op_ack
	JMPF rt_test_disable
rt_test_op_ack:
	BNE #OP_ACK, rt_test_op_idle
	JMPF rt_test_ack
rt_test_op_idle:
	BNE #OP_IDLE, rt_test_op_input
	JMPF rt_test_idle
rt_test_op_input:
	BE #OP_PRESS, rt_test_op_skip
	BE #OP_RELEASE, rt_test_op_skip
	BNE #OP_DOCK, rt_test_op_buttons
rt_test_op_skip:
	JMPF rt_test_skip
rt_test_op_buttons:
	BNE #OP_LOG_BUTTONS, rt_test_op_sys
	JMPF rt_test_log_buttons
rt_test_op_sys:
	BNE #OP_LOG_SYS, rt_test_op_levels
	JMPF rt_test_log_sys
rt_test_op_levels:
	BNE #OP_LOG_LEVELS, rt_test_op_run
	JMPF rt_test_log_levels
rt_test_op_run:
	BNE #OP_LOG_RUN, rt_test_op_fill
	JMPF rt_test_log_run
rt_test_op_fill:
	BNE #OP_FILL, rt_test_op_draw
	JMPF rt_test_fill
rt_test_op_draw:
	BNE #OP_DRAW, rt_test_op_present
	JMPF rt_test_draw
rt_test_op_present:
	BNE #OP_PRESENT, rt_test_op_rom
	JMPF rt_test_present
rt_test_op_rom:
	BNE #OP_ROM, rt_test_op_write
	JMPF rt_test_rom
rt_test_op_write:
	BNE #OP_SAVE_WRITE, rt_test_op_read
	JMPF rt_test_save_write
rt_test_op_read:
	BNE #OP_SAVE_READ, rt_test_op_buzz
	JMPF rt_test_save_read
rt_test_op_buzz:
	BNE #OP_BUZZ, rt_test_op_lcd
	JMPF rt_test_buzz
rt_test_op_lcd:
	BNE #OP_LCD, rt_test_op_power
	JMPF rt_test_lcd
rt_test_op_power:
	BNE #OP_POWER, rt_test_op_mask
	JMPF rt_test_power
rt_test_op_mask:
	BNE #OP_MASK, rt_test_op_unmask
	CLR1 IE, 7
	JMPF rt_test_loop
rt_test_op_unmask:
	BNE #OP_UNMASK, rt_test_error
	SET1 IE, 7
	JMPF rt_test_loop
rt_test_error:
	MOV #RT_EV_ERROR, RT_LOG
	ST RT_LOG
rt_test_done:
	RET

;----------------------
; | rt_test_handler
; | Description: The handler: logs h with the latched, enabled lines and acknowledges them, as scenario_host's handler does.
; | Author: suinevere
; | Dependencies: rt_irq_mask, rt_irq_pending, rt_irq_ack
; | Globals: N/A
; | Params: N/A
; | Returns: N/A. Destroys ACC, B, C, TRL, TRH.
;----------------------
rt_test_handler:
	CALLF rt_irq_mask
	LD TRL
	ST C
	LD TRH
	ST B
	CALLF rt_irq_pending
	LD TRL
	AND C
	ST TRL
	LD TRH
	AND B
	ST TRH
	MOV #RT_EV_HANDLER, RT_LOG
	LD TRH
	ST RT_LOG
	LD TRL
	ST RT_LOG
	CALLF rt_irq_ack
	RET

;----------------------
; | rt_test_tick / rt_test_sound_stop
; | Description: The tick op (rt_tick_mode with MODE, a mode above 3 being a bad argument) and the sound_stop op (rt_sound_stop).
; | Author: suinevere
; | Dependencies: rt_test_fetch, rt_tick_mode, rt_sound_stop, rt_test_error
; | Globals: N/A
; | Params: scenario bytes -- MODE (1) for tick
; | Returns: N/A (back to rt_test_loop). Destroys ACC, B, C.
;----------------------
rt_test_tick:
	CALLF rt_test_fetch
	ST C
	AND #$FC
	BZ rt_test_tick_ok
	MOV #OP_TICK, ACC
	JMPF rt_test_error
rt_test_tick_ok:
	LD C
	PUSH TRL
	PUSH TRH
	CALLF rt_tick_mode
	POP TRH
	POP TRL
	JMPF rt_test_loop

rt_test_sound_stop:
	PUSH TRL
	PUSH TRH
	CALLF rt_sound_stop
	POP TRH
	POP TRL
	JMPF rt_test_loop

;----------------------
; | rt_test_sound
; | Description: The sound op: rt_sound_play with SAMPLES and RATE (0 slow, 1 fast); a count past 24 bits or another rate is a bad argument.
; | Author: suinevere
; | Dependencies: rt_sound_play, rt_test_error
; | Globals: N/A
; | Params: scenario bytes -- SAMPLES (4, big-endian), RATE (2)
; | Returns: N/A (back to rt_test_loop). Destroys ACC, B, C.
;----------------------
rt_test_sound:
	MOV #$00, ACC
	LDC
	BNZ rt_test_sound_bad
	MOV #$04, ACC
	LDC
	BNZ rt_test_sound_bad
	MOV #$05, ACC
	LDC
	ST C
	SUB #$02
	BP PSW, 7, rt_test_sound_ok
rt_test_sound_bad:
	MOV #OP_SOUND, ACC
	JMPF rt_test_error
rt_test_sound_ok:
	MOV #$03, ACC
	LDC
	ST B
	LD TRL
	ADD #$06
	PUSH ACC
	LD TRH
	ADDC #$00
	PUSH ACC
	MOV #$01, ACC
	LDC
	PUSH ACC
	MOV #$02, ACC
	LDC
	ST TRL
	POP TRH
	CALLF rt_sound_play
	POP TRH
	POP TRL
	JMPF rt_test_loop

;----------------------
; | rt_test_enable / rt_test_disable / rt_test_ack
; | Description: The enable, disable and ack ops: rt_irq_enable, rt_irq_disable or rt_irq_ack with LINES.
; | Author: suinevere
; | Dependencies: rt_test_lines, rt_irq_enable, rt_irq_disable, rt_irq_ack
; | Globals: N/A
; | Params: scenario bytes -- LINES (2, big-endian)
; | Returns: N/A (back to rt_test_loop). Destroys ACC.
;----------------------
rt_test_enable:
	CALLF rt_test_lines
	CALLF rt_irq_enable
	POP TRH
	POP TRL
	JMPF rt_test_loop

rt_test_disable:
	CALLF rt_test_lines
	CALLF rt_irq_disable
	POP TRH
	POP TRL
	JMPF rt_test_loop

rt_test_ack:
	CALLF rt_test_lines
	CALLF rt_irq_ack
	POP TRH
	POP TRL
	JMPF rt_test_loop

;----------------------
; | rt_test_lines
; | Description: Reads a 16-bit LINES argument and leaves the position after it on the caller's stack, under the caller's own return, for the caller to pop into TRH:TRL.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRH:TRL -- position of LINES
; | Returns: TRH:TRL -- LINES; the stack -- the next position (pop TRH, then TRL). Destroys ACC, B, C.
;----------------------
rt_test_lines:
	POP B
	POP C
	LD TRL
	ADD #$02
	PUSH ACC
	LD TRH
	ADDC #$00
	PUSH ACC
	PUSH C
	PUSH B
	MOV #$00, ACC
	LDC
	PUSH ACC
	MOV #$01, ACC
	LDC
	ST TRL
	POP TRH
	RET

;----------------------
; | rt_test_idle
; | Description: The idle op: calls rt_idle N times, logging i 1..N after each.
; | Author: suinevere
; | Dependencies: rt_idle
; | Globals: N/A
; | Params: scenario bytes -- N (2, big-endian)
; | Returns: N/A (back to rt_test_loop). Destroys ACC, B, C.
;----------------------
rt_test_idle:
	LD TRL
	ADD #$02
	PUSH ACC
	LD TRH
	ADDC #$00
	PUSH ACC
	MOV #$01, ACC
	LDC
	PUSH ACC
	MOV #$00, ACC
	LDC
	PUSH ACC
	MOV #$00, TRL
	MOV #$00, TRH
rt_test_idle_loop:
	POP B
	POP C
	LD TRL
	BNE C, rt_test_idle_more
	LD TRH
	BNE B, rt_test_idle_more
	POP TRH
	POP TRL
	JMPF rt_test_loop
rt_test_idle_more:
	PUSH C
	PUSH B
	INC TRL
	LD TRL
	BNZ rt_test_idle_call
	INC TRH
rt_test_idle_call:
	PUSH TRL
	PUSH TRH
	CALLF rt_idle
	POP TRH
	POP TRL
	MOV #RT_EV_IDLE, RT_LOG
	LD TRH
	ST RT_LOG
	LD TRL
	ST RT_LOG
	BR rt_test_idle_loop

;----------------------
; | rt_test_ram_write
; | Description: The ram_write op: writes VALUE at the packed OFF through rt_w8.
; | Author: suinevere
; | Dependencies: rt_test_fetch, rt_w8
; | Globals: N/A
; | Params: scenario bytes -- OFF (2, big-endian), VALUE (1)
; | Returns: N/A (back to rt_test_loop). Destroys B and C.
;----------------------
rt_test_ram_write:
	CALLF rt_test_fetch
	ST B
	CALLF rt_test_fetch
	ST C
	CALLF rt_test_fetch
	PUSH TRH
	PUSH TRL
	PUSH ACC
	LD B
	ST TRH
	LD C
	ST TRL
	POP ACC
	CALLF rt_w8
	POP TRL
	POP TRH
	JMPF rt_test_loop

;----------------------
; | rt_test_log_ram
; | Description: The log_ram op: reads the packed OFF through rt_r8 and logs m OFF VALUE.
; | Author: suinevere
; | Dependencies: rt_test_fetch, rt_r8
; | Globals: N/A
; | Params: scenario bytes -- OFF (2, big-endian)
; | Returns: N/A (back to rt_test_loop). Destroys B and C.
;----------------------
rt_test_log_ram:
	CALLF rt_test_fetch
	ST B
	CALLF rt_test_fetch
	ST C
	PUSH TRH
	PUSH TRL
	LD B
	ST TRH
	LD C
	ST TRL
	CALLF rt_r8
	MOV #RT_EV_RAM, RT_LOG
	PUSH ACC
	LD B
	ST RT_LOG
	LD C
	ST RT_LOG
	POP ACC
	ST RT_LOG
	POP TRL
	POP TRH
	JMPF rt_test_loop

;----------------------
; | rt_test_skip
; | Description: The press, release and dock ops: steps over their three argument bytes, as the harness applies them to ports 3 and 7.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: scenario bytes -- MASK or ON (1), AT_IDLE (2)
; | Returns: N/A (back to rt_test_loop). Destroys ACC.
;----------------------
rt_test_skip:
	LD TRL
	ADD #$03
	ST TRL
	LD TRH
	ADDC #$00
	ST TRH
	JMPF rt_test_loop

;----------------------
; | rt_test_log_buttons / rt_test_log_sys
; | Description: The log_buttons and log_sys ops: log b with rt_buttons or s with rt_sys_flags.
; | Author: suinevere
; | Dependencies: rt_buttons, rt_sys_flags
; | Globals: N/A
; | Params: N/A
; | Returns: N/A (back to rt_test_loop). Destroys ACC, B, C.
;----------------------
rt_test_log_buttons:
	CALLF rt_buttons
	MOV #RT_EV_BUTTONS, RT_LOG
	ST RT_LOG
	JMPF rt_test_loop

rt_test_log_sys:
	CALLF rt_sys_flags
	MOV #RT_EV_SYS, RT_LOG
	ST RT_LOG
	JMPF rt_test_loop

;----------------------
; | rt_test_log_levels
; | Description: The log_levels op: logs l with rt_irq_levels.
; | Author: suinevere
; | Dependencies: rt_irq_levels
; | Globals: N/A
; | Params: N/A
; | Returns: N/A (back to rt_test_loop). Destroys ACC.
;----------------------
rt_test_log_levels:
	PUSH TRL
	PUSH TRH
	CALLF rt_irq_levels
	MOV #RT_EV_LEVELS, RT_LOG
	LD TRH
	ST RT_LOG
	LD TRL
	ST RT_LOG
	POP TRH
	POP TRL
	JMPF rt_test_loop

;----------------------
; | rt_test_log_run / rt_test_fill
; | Description: The log_run op, logging m OFF VALUE through rt_r8 for COUNT bytes from OFF, and the fill op, writing START + i at OFF + i through rt_w8 for COUNT bytes.
; | Author: suinevere
; | Dependencies: rt_test_fetch, rt_r8, rt_w8
; | Globals: RAM bank 1 0x02-0x03 (R2, R3) as the count and the value
; | Params: scenario bytes -- OFF (2, big-endian), COUNT (1), and START (1) for fill
; | Returns: N/A (back to rt_test_loop). Destroys ACC, B, C, R0.
;----------------------
rt_test_log_run:
	CALLF rt_test_fetch
	ST B
	CALLF rt_test_fetch
	ST C
	CALLF rt_test_fetch
	ST $02
	PUSH TRH
	PUSH TRL
	LD B
	ST TRH
	LD C
	ST TRL
rt_test_log_run_loop:
	LD $02
	BZ rt_test_run_done
	DEC $02
	CALLF rt_r8
	MOV #RT_EV_RAM, RT_LOG
	PUSH ACC
	LD TRH
	ST RT_LOG
	LD TRL
	ST RT_LOG
	POP ACC
	ST RT_LOG
	INC TRL
	LD TRL
	BNZ rt_test_log_run_loop
	INC TRH
	BR rt_test_log_run_loop

rt_test_fill:
	CALLF rt_test_fetch
	ST B
	CALLF rt_test_fetch
	ST C
	CALLF rt_test_fetch
	ST $02
	CALLF rt_test_fetch
	ST $03
	PUSH TRH
	PUSH TRL
	LD B
	ST TRH
	LD C
	ST TRL
rt_test_fill_loop:
	LD $02
	BZ rt_test_run_done
	DEC $02
	LD $03
	CALLF rt_w8
	INC $03
	INC TRL
	LD TRL
	BNZ rt_test_fill_loop
	INC TRH
	BR rt_test_fill_loop
rt_test_run_done:
	POP TRL
	POP TRH
	JMPF rt_test_loop

;----------------------
; | rt_test_draw / rt_test_present
; | Description: The draw op, writing VALUE to byte COL of frame buffer row ROW in XRAM through rt_fb_row and @R2, a row past 31 or a byte past 5 being a bad argument; and the present op, logging p and calling rt_present.
; | Author: suinevere
; | Dependencies: rt_test_fetch, rt_fb_row, rt_present, rt_test_error
; | Globals: RAM bank 1 0x02-0x03 (R2, R3), XBNK, XRAM
; | Params: scenario bytes -- ROW (1), COL (1), VALUE (1) for draw
; | Returns: N/A (back to rt_test_loop). Destroys ACC, B, C.
;----------------------
rt_test_draw:
	CALLF rt_test_fetch
	ST $02
	CALLF rt_test_fetch
	ST $03
	CALLF rt_test_fetch
	PUSH ACC
	LD $02
	SUB #32
	BN PSW, 7, rt_test_draw_bad
	LD $03
	SUB #6
	BN PSW, 7, rt_test_draw_bad
	LD $02
	CALLF rt_fb_row
	LD B
	ST XBNK
	LD C
	ADD $03
	ST $02
	POP ACC
	ST @R2
	JMPF rt_test_loop
rt_test_draw_bad:
	POP ACC
	MOV #OP_DRAW, ACC
	JMPF rt_test_error

rt_test_present:
	MOV #RT_EV_PRESENT, RT_LOG
	CALLF rt_present
	JMPF rt_test_loop

;----------------------
; | rt_test_rom
; | Description: The rom op: logs r with the table byte rt_rom8 reads at OFF.
; | Author: suinevere
; | Dependencies: rt_test_fetch, rt_rom8
; | Globals: N/A
; | Params: scenario bytes -- OFF (2, big-endian)
; | Returns: N/A (back to rt_test_loop). Destroys ACC, B, C.
;----------------------
rt_test_rom:
	CALLF rt_test_fetch
	ST B
	CALLF rt_test_fetch
	ST C
	PUSH TRH
	PUSH TRL
	LD B
	ST TRH
	LD C
	ST TRL
	CALLF rt_rom8
	MOV #RT_EV_ROM, RT_LOG
	ST RT_LOG
	POP TRL
	POP TRH
	JMPF rt_test_loop

;----------------------
; | rt_test_save_write / rt_test_save_read
; | Description: The save_write and save_read ops: rt_save_write or rt_save_read of SECTOR with the RAM at OFF, logging w or v with SECTOR and the status; a sector past 255 fails with status 1 without the call, as it lies past any save.
; | Author: suinevere
; | Dependencies: rt_test_fetch, rt_save_write, rt_save_read
; | Globals: RAM bank 1 0x02-0x03 (R2, R3) as the sector's high byte and the event tag
; | Params: scenario bytes -- SECTOR (2, big-endian), OFF (2, big-endian)
; | Returns: N/A (back to rt_test_loop). Destroys ACC, B, C, R0.
;----------------------
rt_test_save_write:
	MOV #RT_EV_SAVE, $03
	BR rt_test_save
rt_test_save_read:
	MOV #RT_EV_LOAD, $03
rt_test_save:
	CALLF rt_test_fetch
	ST $02
	CALLF rt_test_fetch
	PUSH ACC
	CALLF rt_test_fetch
	ST B
	CALLF rt_test_fetch
	ST C
	POP ACC
	PUSH TRH
	PUSH TRL
	PUSH ACC
	LD B
	ST TRH
	LD C
	ST TRL
	LD $02
	BNZ rt_test_save_past
	POP ACC
	PUSH ACC
	BP $03, 0, rt_test_save_do_write
	CALLF rt_save_read
	BR rt_test_save_log
rt_test_save_do_write:
	CALLF rt_save_write
	BR rt_test_save_log
rt_test_save_past:
	MOV #$01, ACC
rt_test_save_log:
	ST C
	LD $03
	ST RT_LOG
	LD $02
	ST RT_LOG
	POP ACC
	ST RT_LOG
	LD C
	ST RT_LOG
	POP TRL
	POP TRH
	JMPF rt_test_loop

;----------------------
; | rt_test_buzz / rt_test_lcd
; | Description: The buzz op (rt_buzz with TONE) and the lcd op (rt_lcd with ON).
; | Author: suinevere
; | Dependencies: rt_test_fetch, rt_buzz, rt_lcd
; | Globals: N/A
; | Params: scenario bytes -- TONE (1) or ON (1)
; | Returns: N/A (back to rt_test_loop). Destroys ACC, B, C.
;----------------------
rt_test_buzz:
	CALLF rt_test_fetch
	CALLF rt_buzz
	JMPF rt_test_loop

rt_test_lcd:
	CALLF rt_test_fetch
	CALLF rt_lcd
	JMPF rt_test_loop

;----------------------
; | rt_test_power
; | Description: The power op: rt_power with MODE, 1-3, logging x MODE before an exit as scenario_host does; another mode is a bad argument.
; | Author: suinevere
; | Dependencies: rt_test_fetch, rt_power, rt_test_error
; | Globals: N/A
; | Params: scenario bytes -- MODE (1)
; | Returns: N/A (back to rt_test_loop after standby; the exits never return). Destroys ACC, B, C, R1.
;----------------------
rt_test_power:
	CALLF rt_test_fetch
	ST C
	BZ rt_test_power_bad
	SUB #4
	BN PSW, 7, rt_test_power_bad
	LD C
	BE #RT_POWER_STANDBY, rt_test_power_standby
	MOV #RT_EV_EXIT, RT_LOG
	ST RT_LOG
	CALLF rt_power
rt_test_power_standby:
	PUSH TRH
	PUSH TRL
	CALLF rt_power
	POP TRL
	POP TRH
	JMPF rt_test_loop
rt_test_power_bad:
	MOV #OP_POWER, ACC
	JMPF rt_test_error

;----------------------
; | rt_test_fetch
; | Description: Reads the scenario byte at TRH:TRL from this GAME's flash and steps TRH:TRL past it.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRH:TRL -- scenario position
; | Returns: ACC -- the byte; TRH:TRL -- the next position
;----------------------
rt_test_fetch:
	MOV #$00, ACC
	LDC
	INC TRL
	PUSH ACC
	LD TRL
	BNZ rt_test_fetch_done
	INC TRH
rt_test_fetch_done:
	POP ACC
	RET

	.include "../rt_ram.s"
	.include "../rt_events.s"
	.include "../rt_input.s"
	.include "../rt_lcd.s"
	.include "../rt_file.s"
	.include "../rt_sound.s"
	.include "../rt_power.s"

;----------------------
; | app_tables
; | Description: The program's tables for rt_rom8: scenario.py's 512-byte test table, which the Makefile writes to pocketstation/build/vmu_rt/tables.bin; no disc data.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_rt/Makefile, pocketstation/tools/scenario.py
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
app_tables:
	.include bytes "../../../build/vmu_rt/tables.bin"

;----------------------
; | rt_test_scenario
; | Description: The compiled scenario, copied to pocketstation/build/vmu_rt/scenario.bin by the Makefile before each build.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_rt/Makefile
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
rt_test_scenario:
	.include bytes "../../../build/vmu_rt/scenario.bin"
