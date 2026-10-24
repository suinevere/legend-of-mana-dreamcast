;----------------------
; | rt_sound.s
; | Description: The runtime's buzzer: Timer 1's low half as an 8-bit pulse generator on P17, the pin the buzzer hangs on.
; | Author: suinevere
; | Dependencies: sfr.inc
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | rt_buzz
; | Description: vmu_buzz: tone t sets T1LR = t >> 1, a period of 256 - (t >> 1) cycles (3.9 kHz at t 1 up to 7.75 kHz at t 255 on the 1 MHz clock), and T1LC half a period on plus t's low bit, so every tone has its own setting.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: T1CNT, T1LR, T1LC, P1, P1DDR, P1FCR
; | Params: ACC -- 0 silences the buzzer, 1-255 a tone
; | Returns: N/A. Destroys ACC, B, C.
;----------------------
rt_buzz:
	BZ rt_buzz_off
	ST C
	CLR1 PSW, 7
	RORC
	ST B
	ST T1LR
	INC ACC
	CLR1 PSW, 7
	RORC
	XOR #$FF
	ADD #$81
	ADD B
	BN C, 0, rt_buzz_even
	INC ACC
rt_buzz_even:
	ST T1LC
	MOV #$80, P1DDR
	MOV #$80, P1FCR
	CLR1 P1, 7
	MOV #$50, T1CNT
	RET
rt_buzz_off:
	MOV #$00, P1FCR
	CLR1 P1, 7
	MOV #$00, T1CNT
	RET
