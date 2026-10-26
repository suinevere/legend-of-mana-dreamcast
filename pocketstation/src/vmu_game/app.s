;----------------------
; | app.s
; | Description: app.c's leaves in LC86K assembly: the quit request read and the idle count setter; and g_pet_check, which the GAME runs first to see that a pet save is there.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/app.c, pocketstation/build/vmu_game/rrl_ram.inc, pocketstation/src/vmu_rt/rt_input.s (rt_sys_flags), pocketstation/src/vmu_rt/rt_file.s (rt_save)
; | Globals: idle_lo, repeat_up
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
