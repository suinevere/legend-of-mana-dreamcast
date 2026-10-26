;----------------------
; | callbase.inc
; | Description: The head of a --call test image: the runtime's include order and vectors, a VMS header with no icon, gen_ram.py hw's field equates and game.inc, the runtime, an app_main that returns, and call_return, the stub the harness returns a called routine to. The including file adds its routines and, for game routines, tables.s. Paths are written from pocketstation/, two levels up, so the file serves src/vmu_game and tests/asm alike.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_rt/*.s, *.inc, pocketstation/build/vmu_game/rrl_ram.inc, game.inc, pocketstation/harness/vmu_harness.c (--call)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_rt/sfr.inc"
	.include "../../src/vmu_rt/rt.inc"
	.include "../../src/vmu_rt/rt_vectors.s"

;----------------------
; | call_header
; | Description: The VMS GAME header at 0x200, with one blank icon after it.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.org $0200
call_header:
	.byte "RRL CALL        "
	.byte "RING RING LAND --CALL IMAGE     "
	.byte "RRL_CALL        "
	.word 1, 8, 0, 0
	.word 0, 0

	.org $0480
	.include "../../build/vmu_game/rrl_ram.inc"
	.include "../../src/vmu_game/game.inc"
	.include "../../src/vmu_rt/rt_ram.s"
	.include "../../src/vmu_rt/rt_events.s"
	.include "../../src/vmu_rt/rt_input.s"
	.include "../../src/vmu_rt/rt_lcd.s"
	.include "../../src/vmu_rt/rt_file.s"
	.include "../../src/vmu_rt/rt_sound.s"
	.include "../../src/vmu_rt/rt_power.s"

;----------------------
; | app_main
; | Description: The --call image runs no game loop: the harness enters routines directly, so app_main only returns.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
app_main:
	RET

;----------------------
; | call_return
; | Description: Where a --call routine returns to; the harness pushes this address before the call and stops when the PC reaches it.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c (CALL_HALT_LABEL)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
call_return:
	BR call_return
