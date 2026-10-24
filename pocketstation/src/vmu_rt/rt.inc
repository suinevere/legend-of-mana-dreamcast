;----------------------
; | rt.inc
; | Description: Equates the VMU runtime and its test programs share with the harness, and the runtime's calling convention. Routines take ACC, B, C and a 16-bit TRH:TRL, return ACC, and their headers list what else they destroy.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c, pocketstation/tools/scenario.py
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | RT_LOG
; | Description: Unused SFR 0x16E; the harness appends every byte written here to its --log file.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_LOG	EQU	$16E

;----------------------
; | RT_PRESENT_PORT
; | Description: Unused SFR 0x16F; any write marks a presented frame, where the harness dumps state and takes the next script sample.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_PRESENT_PORT	EQU	$16F

;----------------------
; | RT_IDLE_MARK
; | Description: Unused SFR 0x16D (no case in cpu.c, UNUSED in libevmu's map, only declared in the US BIOS); rt_idle writes it once a call, and the harness numbers --inputs idle indices by these writes.
; | Author: suinevere
; | Dependencies: pocketstation/harness/vmu_harness.c
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_IDLE_MARK	EQU	$16D

;----------------------
; | RT_EV_HANDLER / RT_EV_IDLE / RT_EV_ROM / RT_EV_SAVE / RT_EV_LOAD / RT_EV_PRESENT / RT_EV_EXIT / RT_EV_RAM / RT_EV_ERROR / RT_EV_BUTTONS / RT_EV_SYS / RT_EV_LEVELS
; | Description: Event-log tags written to RT_LOG ahead of each event's big-endian fields, as scenario.py decode reads them: h LINES, i N, r VALUE, w SECTOR STATUS, v SECTOR STATUS, p, x MODE, m OFF VALUE, error OP, b BUTTONS, s FLAGS, l LEVELS.
; | Author: suinevere
; | Dependencies: pocketstation/tools/scenario.py
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_EV_HANDLER	EQU	$68
RT_EV_IDLE	EQU	$69
RT_EV_ROM	EQU	$72
RT_EV_SAVE	EQU	$77
RT_EV_LOAD	EQU	$76
RT_EV_PRESENT	EQU	$70
RT_EV_EXIT	EQU	$78
RT_EV_RAM	EQU	$6D
RT_EV_ERROR	EQU	$45
RT_EV_BUTTONS	EQU	$62
RT_EV_SYS	EQU	$73
RT_EV_LEVELS	EQU	$6C

;----------------------
; | RT_LINE_BUTTONS / RT_LINE_TICK / RT_LINE_RTC / RT_LINE_DOCK / RT_LINE_SOUND
; | Description: The five interrupt lines as vmu_hw.h numbers them, passed to the rt_irq_* routines as a 16-bit mask in TRH:TRL.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/vmu_hw.h
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_LINE_BUTTONS	EQU	$001F
RT_LINE_TICK	EQU	$0100
RT_LINE_RTC	EQU	$0200
RT_LINE_DOCK	EQU	$0400
RT_LINE_SOUND	EQU	$2000

;----------------------
; | RT_TICK_OFF / RT_TICK_FRAME / RT_TICK_FAST / RT_TICK_STANDBY / RT_SOUND_SLOW / RT_SOUND_FAST
; | Description: rt_tick_mode's modes in ACC, as VMU_TICK_*, and rt_sound_play's rates in C, standing for VMU_SOUND_SLOW and VMU_SOUND_FAST.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/vmu_hw.h
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_TICK_OFF	EQU	0
RT_TICK_FRAME	EQU	1
RT_TICK_FAST	EQU	2
RT_TICK_STANDBY	EQU	3
RT_SOUND_SLOW	EQU	0
RT_SOUND_FAST	EQU	1

;----------------------
; | RT_POWER_STANDBY / RT_POWER_EXIT / RT_POWER_EXIT_DOCKED
; | Description: rt_power's modes in ACC, as VMU_POWER_*.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu/vmu_hw.h
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_POWER_STANDBY	EQU	1
RT_POWER_EXIT	EQU	2
RT_POWER_EXIT_DOCKED	EQU	3
