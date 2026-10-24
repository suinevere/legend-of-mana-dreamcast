;----------------------
; | rt_vectors.s
; | Description: The GAME's fixed first 512 bytes: the reset and interrupt vectors, the reset path that sets the stack, RAM bank and 1 MHz clock, and the stubs the BIOS calls return through.
; | Author: suinevere
; | Dependencies: sfr.inc, rt_ram.s, rt_events.s, rt_input.s, rt_file.s, app_main (the program's entry)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | rt_vec_reset / rt_vec_int0 .. rt_vec_port3
; | Description: The vector table at its fixed offsets: reset, INT0, INT1, INT2/T0L, INT3/base timer, T0H, T1, SIO0, SIO1, Maple and port 3; T0H paces the event schedule, INT0 follows the dock, the base timer and port 3 poll the buttons, and every other interrupt returns.
; | Author: suinevere
; | Dependencies: rt_reset, rt_t0h_isr, rt_int0_isr, rt_bt_isr, rt_p3_isr
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
	.org $0000
rt_vec_reset:
	JMPF rt_reset

	.org $0003
rt_vec_int0:
	JMPF rt_int0_isr

	.org $000B
rt_vec_int1:
	RETI

	.org $0013
rt_vec_int2:
	RETI

	.org $001B
rt_vec_int3:
	JMPF rt_bt_isr

	.org $0023
rt_vec_t0h:
	JMPF rt_t0h_isr

	.org $002B
rt_vec_t1:
	RETI

	.org $0033
rt_vec_sio0:
	RETI

	.org $003B
rt_vec_sio1:
	RETI

	.org $0043
rt_vec_maple:
	RETI

	.org $004B
rt_vec_port3:
	JMPF rt_p3_isr

;----------------------
; | rt_reset
; | Description: Puts the stack at bank 0 0x80, selects RAM bank 1 with R0 at 0x00 and masks every interrupt, making INT0 and INT1 low priority so IE bit 7 covers them. It then switches to the CF oscillator divided by 6 the way the BIOS starts it, clears RAM, finds the pet save, takes the input as it stands and starts the event schedule.
; | Author: suinevere
; | Dependencies: rt_ram_clear, rt_save_find, rt_input_init, rt_events_init, rt_main
; | Globals: N/A
; | Params: N/A
; | Returns: N/A (falls into rt_main)
;----------------------
	.org $0050
rt_reset:
	MOV #$7F, SP
	MOV #$02, PSW
	MOV #$03, IE
	CLR1 VSEL, 4
	MOV #$81, OCR
	MOV #$80, OCR
	MOV #$64, ACC
rt_reset_wait:
	DBNZ ACC, rt_reset_wait
	MOV #$92, OCR
	CALLF rt_ram_clear
	CALLF rt_save_find
	CALLF rt_input_init
	CALLF rt_events_init

;----------------------
; | rt_main
; | Description: Runs the program's app_main on cleared RAM; if it returns, masks interrupts and halts for good.
; | Author: suinevere
; | Dependencies: app_main
; | Globals: N/A
; | Params: N/A
; | Returns: N/A (never)
;----------------------
rt_main:
	CALLF app_main
rt_main_halt:
	CLR1 IE, 7
	SET1 PCON, 0
	BR rt_main_halt

;----------------------
; | rt_bios_fm_wrt / rt_bios_fm_vrf / rt_bios_fm_prd / rt_bios_exit
; | Description: Calls into the BIOS flash write, verify and read and the mode change; the BIOS comes back to the RET five bytes in, and the exit never comes back. rt_bios_fm_wrt's RET at 0x105 is also where rt_bios_fm_page comes back; rt_bios_timer, at 0x130, is described on its own.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: RAM bank 1 0x7D-0x7F -- flash bank and address; 0x80-0xFF -- the page; ACC -- the mode for rt_bios_exit
; | Returns: ACC -- 0x00 or an error from the write and verify
;----------------------
	.org $0100
rt_bios_fm_wrt:
	NOT1 EXT, 0
	JMPF $0100
	RET

	.org $0110
rt_bios_fm_vrf:
	NOT1 EXT, 0
	JMPF $0110
	RET

	.org $0120
rt_bios_fm_prd:
	NOT1 EXT, 0
	JMPF $0120
	RET

;----------------------
; | rt_bios_timer
; | Description: TIMER_EX, the BIOS's half-second clock update, which a GAME owes it on every base timer interrupt: masks interrupts while the BIOS runs on RAM bank 0, which comes back to 0x139.
; | Author: suinevere
; | Dependencies: BIOS ROM 0x0130 (american_v1.05.s TIMER_EX, label_3DC9)
; | Globals: bank 0 0x17-0x1F (the BIOS clock), 0x33, 0x34, 0x70, 0x72, 0x74; BTCR
; | Params: N/A
; | Returns: N/A. Keeps every register and IE.
;----------------------
	.org $0130
rt_bios_timer:
	PUSH IE
	CLR1 IE, 7
	NOT1 EXT, 0
	JMPF $0130
	POP IE
	RET

;----------------------
; | rt_bios_fm_page
; | Description: The BIOS's own page write (american_v1.05.s label_E024, a brf to label_E3CE), which the Maple block write uses and which, unlike FM_WRT_EX, takes either flash bank: pushes ROM 0x0103 as its return, so its RET lands on FM_WRT_EX's not1 ext,0 and jmpf 0x0105 back to flash.
; | Author: suinevere
; | Dependencies: BIOS ROM 0xE024 and 0x0103, rt_bios_fm_wrt's RET at 0x105
; | Globals: RAM bank 1 (the current bank) 0x00 (R0), 0x7C-0x7F, 0x80-0xFF; FPR
; | Params: bank 1 0x7C -- 1 for data polling; 0x7D -- flash bank; 0x7E:0x7F -- page address; 0x80-0xFF -- the page; interrupts masked
; | Returns: N/A (no status: rt_bios_fm_vrf checks the page). Destroys ACC, B, C, TRL, TRH, FPR, R0.
;----------------------
	.org $0150
rt_bios_fm_page:
	MOV #$03, ACC
	PUSH ACC
	MOV #$01, ACC
	PUSH ACC
	NOT1 EXT, 0
	JMPF $E024

	.org $01F0
rt_bios_exit:
	NOT1 EXT, 0
	JMPF $01F0
