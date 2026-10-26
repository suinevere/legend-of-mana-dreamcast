;----------------------
; | game.s
; | Description: Ring Ring Land's GAME: the runtime, a VMS header with R2's short description, the game's name and one blank icon, and the tables the GAME keeps from RES1506.BIN (tables.s). App_main leaves through rt_power without a pet save (g_pet_check), else runs the game through g_entry.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_rt/{sfr.inc,rt.inc,rt_vectors.s,rt_ram.s,rt_events.s,rt_input.s,rt_lcd.s,rt_file.s,rt_sound.s,rt_power.s}, pocketstation/build/vmu_game/{rrl_ram.inc,tables.s}, pocketstation/src/vmu_game/{game.inc,routines.inc,Makefile}
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_rt/sfr.inc"
	.include "../../src/vmu_rt/rt.inc"
	.include "../../src/vmu_rt/rt_vectors.s"

;----------------------
; | rrl_header
; | Description: The VMS GAME header at 0x200: R2's own short description (dreamcast/src/card/card_store_kos.c's KOS_VMU_SHORT_DESC), Ring Ring Land's name as the long description and identifier, one blank icon (R2b has not drawn the real one yet).
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.org $0200
rrl_header:
	.byte "LEGEND OF MANA  "
	.byte "RING RING LAND                  "
	.byte "RING RING LAND  "
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
	.include "../../src/vmu_game/routines.inc"

;----------------------
; | app_main
; | Description: The GAME's entry, called once RAM is cleared and the scheduler is running: leaves through rt_power without a pet save, else runs Ring Ring Land, which never returns.
; | Author: suinevere
; | Dependencies: g_pet_check, g_entry, rt_power
; | Globals: N/A
; | Params: N/A
; | Returns: never
;----------------------
app_main:
	CALLF g_pet_check
	BNZ app_main_none
	JMPF g_entry
app_main_none:
	MOV #RT_POWER_EXIT, ACC
	JMPF rt_power

	.cnop 0, $04
	.include "../../build/vmu_game/tables.s"
