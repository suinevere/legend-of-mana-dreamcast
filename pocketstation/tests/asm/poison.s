;----------------------
; | poison.s
; | Description: A --call test image with the real g_random and two probes: p_after_game copies B to C after calling g_random, and p_after_rt sets B and C, calls rt_buttons and leaves its result in TRL and B in TRH, so a test sees the harness poison what each callee declares it destroys.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_game/callbase.inc, pocketstation/src/vmu_game/rng.s, pocketstation/build/vmu_game/tables.s (rng.s reads app_tb_80), pocketstation/src/vmu_rt/rt_input.s (rt_buttons), pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_game/callbase.inc"
	.include "../../src/vmu_game/rng.s"

p_after_game:
	CALLF g_random
	LD B
	ST C
	RET

p_after_rt:
	MOV #$33, B
	MOV #$44, C
	CALLF rt_buttons
	ST TRL
	LD B
	ST TRH
	RET

	.cnop 0, $04
	.include "../../build/vmu_game/tables.s"
