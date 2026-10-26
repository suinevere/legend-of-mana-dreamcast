;----------------------
; | sfr.inc
; | Description: Names for the LC86K special function registers the VMU runtime touches, at their data addresses.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | ACC / PSW / B / C / TRL / TRH / SP
; | Description: The accumulator, program status word (bit 1 selects RAM bank 1), the B and C registers, the table reference pair and the stack pointer, which addresses RAM bank 0.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ACC	EQU	$100
PSW	EQU	$101
B	EQU	$102
C	EQU	$103
TRL	EQU	$104
TRH	EQU	$105
SP	EQU	$106

;----------------------
; | PCON / IE / EXT / OCR
; | Description: Power control (bit 0 halts), interrupt enable (bit 7 is the master switch), the code bank select the BIOS calls toggle, and the oscillator control register.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
PCON	EQU	$107
IE	EQU	$108
EXT	EQU	$10D
OCR	EQU	$10E

;----------------------
; | T0CNT / T0PRR / T0L / T0LR / T0H / T0HR
; | Description: Timer 0 control (run, 16-bit, overflow and interrupt bits), its prescaler reload, and its low and high counters and reloads.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
T0CNT	EQU	$110
T0PRR	EQU	$111
T0L	EQU	$112
T0LR	EQU	$113
T0H	EQU	$114
T0HR	EQU	$115

;----------------------
; | T1CNT / T1LC / T1LR
; | Description: Timer 1 control (bit 6 runs T1L, bit 5 the 16-bit mode, bit 4 sends T1LC to the pulse generator), T1L's compare value, where the PWM output goes high, and its reload, written through T1L's address.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
T1CNT	EQU	$118
T1LC	EQU	$11A
T1LR	EQU	$11B

;----------------------
; | XBNK
; | Description: XRAM bank select for the screen at 0x180-0x1FB: 0 rows 0-15, 1 rows 16-31, 2 the icons.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
XBNK	EQU	$125

;----------------------
; | P1 / P1DDR / P1FCR
; | Description: Port 1's latch, its write-only direction and function registers; bit 7 of each is P17, the buzzer pin, which outputs the latch ORed with Timer 1's PWM when P1FCR bit 7 is set.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
P1	EQU	$144
P1DDR	EQU	$145
P1FCR	EQU	$146

;----------------------
; | VSEL / VRMAD1 / VRMAD2 / VTRBF
; | Description: Work RAM access: VSEL bit 4 auto-increments the address, VRMAD1 and VRMAD2 bit 0 form the 9-bit address, and VTRBF transfers the byte there.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
VSEL	EQU	$163
VRMAD1	EQU	$164
VRMAD2	EQU	$165
VTRBF	EQU	$166

;----------------------
; | VCCR
; | Description: LCD control, write-only: bit 7 powers the display.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
VCCR	EQU	$127

;----------------------
; | FPR
; | Description: Flash control: bit 0 picks the flash bank LDF reads, bit 1 unlocks writes, which only the BIOS uses.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
FPR	EQU	$154

;----------------------
; | P3 / P3INT
; | Description: Port 3, the buttons (active-low; a 1 latch bit keeps the line's pull-up on, so it stays 0xFF), and its interrupt control: bit 2 detects held lines, bit 1 is the request flag, bit 0 enables the interrupt.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
P3	EQU	$14C
P3INT	EQU	$14E

;----------------------
; | P7 / I01CR
; | Description: Port 7, whose bit 0 is high while docked, and the INT0/INT1 control: for INT0, bits 2-3 pick the trigger (01 low level, 11 high level), bit 1 is the flag and bit 0 the enable.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
P7	EQU	$15C
I01CR	EQU	$15D

;----------------------
; | BTCR
; | Description: Base timer control; bit 1 is base timer 0's overflow flag, raised every half second from the quartz clock.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
BTCR	EQU	$17F
