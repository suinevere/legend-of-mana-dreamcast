;----------------------
; | rt_power.s
; | Description: The runtime's power modes: standby on the event schedule, and the two ways out to the BIOS.
; | Author: suinevere
; | Dependencies: sfr.inc, rt.inc, rt_events.s (rt_idle, RTV_STATE, RT_ST_STANDBY), rt_vectors.s (rt_bios_exit)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | rt_power
; | Description: vmu_power. Standby is rt_idle with no handler, taking events, Timer 0 pacing them, until an enabled button or RTC line is latched, which stays latched for the next rt_idle; it writes RT_IDLE_MARK, so the harness counts it as an idle. Any other mode masks interrupts, stops both timers and the buzzer, and leaves to the BIOS through EXIT_EX.
; | Author: suinevere
; | Dependencies: rt_idle, rt_bios_exit
; | Globals: rt_reserved (RTV_STATE), T0CNT, T1CNT, P1FCR
; | Params: ACC -- RT_POWER_STANDBY, RT_POWER_EXIT or RT_POWER_EXIT_DOCKED
; | Returns: N/A for standby (destroys what rt_idle does); the exits never return, ACC holding the mode for the harness, which the BIOS ignores.
;----------------------
rt_power:
	BNE #RT_POWER_STANDBY, rt_power_leave
	SET1 RTV_STATE, RT_ST_STANDBY
	JMPF rt_idle
rt_power_leave:
	CLR1 IE, 7
	MOV #$00, T0CNT
	MOV #$00, T1CNT
	MOV #$00, P1FCR
	JMPF rt_bios_exit
