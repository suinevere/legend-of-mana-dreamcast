;----------------------
; | wrong_hw.s
; | Description: A --call test image whose g_sound_on sounds tone 96 through rt_buzz as game_sound_on does, and whose g_sound_off sounds tone 1 where game_sound_off silences the buzzer, so call_compare passes the first and names the second's hardware call.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_game/callbase.inc, pocketstation/src/vmu_rt/rt_sound.s (rt_buzz), pocketstation/tools/call_compare.py
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.include "../../src/vmu_game/callbase.inc"

g_sound_on:
	MOV #$60, ACC
	CALLF rt_buzz
	RET

g_sound_off:
	MOV #$01, ACC
	CALLF rt_buzz
	RET


app_tables:
