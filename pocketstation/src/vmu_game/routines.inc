;----------------------
; | routines.inc
; | Description: Every ported game routine's source, one file per C file in pocketstation/src/vmu/, included once by the real GAME and once by the --call image.
; | Author: suinevere
; | Dependencies: game.s, callimg.s, pocketstation/src/vmu_game/*.s
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_game/rng.s"
	.include "../../src/vmu_game/app.s"
	.include "../../src/vmu_game/bag.s"
	.include "../../src/vmu_game/beep.s"
	.include "../../src/vmu_game/board.s"
	.include "../../src/vmu_game/fb.s"
	.include "../../src/vmu_game/hw.s"
	.include "../../src/vmu_game/input.s"
	.include "../../src/vmu_game/menu.s"
	.include "../../src/vmu_game/misc.s"
	.include "../../src/vmu_game/mode.s"
	.include "../../src/vmu_game/reel.s"
	.include "../../src/vmu_game/screen.s"
	.include "../../src/vmu_game/slots.s"
	.include "../../src/vmu_game/sound.s"
	.include "../../src/vmu_game/sprite.s"
	.include "../../src/vmu_game/system.s"
	.include "../../src/vmu_game/timing.s"
