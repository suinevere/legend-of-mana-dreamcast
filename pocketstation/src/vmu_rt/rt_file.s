;----------------------
; | rt_file.s
; | Description: The runtime's files: table bytes read with LDF from the pet save BASLUSP01013's payload, and its 128-byte sectors. Sectors are read with FM_PRD_EX and written with the BIOS's page write (label_E024), which takes flash bank 1 where FM_WRT_EX refuses it. Each write also patches the package CRC R2 checks.
; | Author: suinevere
; | Dependencies: sfr.inc, rt_vectors.s (rt_bios_fm_prd, rt_bios_fm_vrf, rt_bios_fm_page), pocketstation/src/game/ram_map.txt (rt_save), dreamcast/tools/vmu_image.py (the name, the package and vms_crc)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | RT_SAVE_HI / RT_SAVE_LO / RT_SAVE_MAX / RT_SAVE_WALK
; | Description: rt_save's WRAM address (packed offset 0x2E8): the sector count (0 without a save), sector 0's package page, then RT_SAVE_MAX anchors, chain blocks 0, 4 ... 84. rt_save_find walks RT_SAVE_WALK blocks.
; | Author: suinevere
; | Dependencies: pocketstation/src/game/ram_map.txt
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
RT_SAVE_HI	EQU	$01
RT_SAVE_LO	EQU	$E8
RT_SAVE_MAX	EQU	22
RT_SAVE_WALK	EQU	88

;----------------------
; | rt_rom8
; | Description: vmu_rom8: the table byte at a file offset, read with LDC from the GAME's app_tables. No bound is checked.
; | Author: suinevere
; | Dependencies: app_tables (the GAME)
; | Globals: N/A
; | Params: TRH:TRL -- file offset
; | Returns: ACC -- the byte. Keeps B, C, TRH, TRL, VRMAD, VSEL, FPR and IE; destroys PSW's flags.
;----------------------
rt_rom8:
	PUSH TRL
	PUSH TRH
	LD TRL
	ADD #<app_tables
	ST TRL
	LD TRH
	ADDC #>app_tables
	ST TRH
	MOV #$00, ACC
	LDC
	POP TRH
	POP TRL
	RET

;----------------------
; | rt_fl_block
; | Description: The flash block of chain block k: rt_save's anchor for k rounded down to a multiple of 4 (the last past 84), then FAT links by LDF from block 254.
; | Author: suinevere
; | Dependencies: rt_save, sfr.inc (FPR)
; | Globals: rt_save (WRAM), the FAT (flash block 254), VRMAD1, VRMAD2, VSEL, FPR
; | Params: ACC -- chain block k (no interrupt handler may change FPR, TRH or TRL, and none does)
; | Returns: ACC -- the flash block. Keeps B and C; destroys TRL, TRH, FPR, VRMAD and PSW's flags; leaves VSEL bit 4 clear.
;----------------------
rt_fl_block:
	PUSH B
	ST B
	ROR
	ROR
	AND #$3F
	ST TRL
	SUB #RT_SAVE_MAX
	BP PSW, 7, rt_fl_block_in
	MOV #RT_SAVE_MAX-1, TRL
rt_fl_block_in:
	LD TRL
	ROL
	ROL
	AND #$FC
	XOR #$FF
	INC ACC
	ADD B
	INC ACC
	ST B
	LD TRL
	ADD #RT_SAVE_LO+2
	ST VRMAD1
	MOV #RT_SAVE_HI, VRMAD2
	CLR1 VSEL, 4
	LD VTRBF
	MOV #$01, FPR
rt_fl_block_hop:
	DBNZ B, rt_fl_block_link
	POP B
	RET
rt_fl_block_link:
	ST TRL
	ROL
	AND #$01
	OR #$FC
	ST TRH
	LD TRL
	ROL
	AND #$FE
	ST TRL
	LDF
	BR rt_fl_block_hop

;----------------------
; | rt_save_name
; | Description: BASLUSP01013's name in the VMU directory, vmu_image.vmu_name_for_ps1("BASLUSP01013"), as R2 stores it.
; | Author: suinevere
; | Dependencies: dreamcast/tools/vmu_image.py
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
rt_save_name:
	.byte "LOMXA764A6E6"

;----------------------
; | rt_save_find
; | Description: Once at reset: reads the root block's FAT block (0x46) and directory top and length (0x4A, 0x4C), then walks the directory pages down through FM_PRD_EX for BASLUSP01013. It reads its package header and chain, keeping the sector count (capped to 22 blocks), sector 0's page and every fourth of the first 88 blocks in rt_save. It then clears bank 1 from 0x7C and R2-R3; a FAT not at block 254 counts as no save.
; | Author: suinevere
; | Dependencies: rt_bios_fm_prd, rt_sf_match, rt_fl_addr, rt_save_name
; | Globals: rt_save (WRAM), RAM bank 1 0x00-0x03 and 0x7C-0xFF (0x7C holds the FAT block during the walk, R2 the blocks left to walk, R3 the walk's index), VRMAD1, VRMAD2, VSEL
; | Params: N/A (interrupts masked, bank 1 from 0x7C unused, as right after rt_ram_clear; the directory must lie in flash bank 1, as every VMU's does)
; | Returns: N/A. A package with an eyecatch or a payload that is not whole sectors counts as no save. Destroys ACC, B, C, TRL, TRH, R0, R1, FPR.
;----------------------
rt_save_find:
	MOV #$01, $7D
	MOV #$FE, $7E
	MOV #$00, $7F
	CALLF rt_bios_fm_prd
	LD $C6
	ST $7C
	BNE #$FE, rt_sf_none
	LD $CC
	ST B
	BZ rt_sf_none
	LD $CA
	MOV #$00, TRL
	CALLF rt_fl_addr
rt_sf_block:
	MOV #$04, C
rt_sf_page:
	CALLF rt_bios_fm_prd
	MOV #$80, $01
rt_sf_entry:
	CALLF rt_sf_match
	BZ rt_sf_found
	LD $01
	ADD #$20
	ST $01
	BNZ rt_sf_entry
	LD $7F
	ADD #$80
	ST $7F
	LD $7E
	ADDC #$00
	ST $7E
	DBNZ C, rt_sf_page
	LD $7E
	SUB #$04
	ST $7E
	DBNZ B, rt_sf_block
rt_sf_none:
	MOV #$00, ACC
	PUSH ACC
	PUSH ACC
	PUSH ACC
	MOV #$00, $03
	JMPF rt_sf_counted
rt_sf_found:
	LD $01
	ADD #$18
	ST $00
	LD @R0
	ST B
	LD $01
	ADD #$02
	ST $00
	LD @R0
	ST C
	LD B
	ST $02
	SUB #RT_SAVE_WALK+1
	BP PSW, 7, rt_sf_walk
	MOV #RT_SAVE_WALK, $02
rt_sf_walk:
	LD B
	SUB #RT_SAVE_MAX+1
	BP PSW, 7, rt_sf_limit
	MOV #RT_SAVE_MAX, B
rt_sf_limit:
	PUSH B
	MOV #$00, TRL
	LD C
	CALLF rt_fl_addr
	CALLF rt_bios_fm_prd
	LD $C0
	ROL
	ROL
	AND #$FC
	INC ACC
	PUSH ACC
	LD $C8
	AND #$7F
	OR $C4
	OR $C5
	BNZ rt_sf_unfit
	LD $CA
	OR $CB
	BNZ rt_sf_long
	BP $C9, 7, rt_sf_long
	LD $C8
	ROL
	AND #$01
	ST $00
	LD $C9
	ROL
	AND #$FE
	OR $00
	BR rt_sf_length
rt_sf_long:
	MOV #$FF, ACC
	BR rt_sf_length
rt_sf_unfit:
	MOV #$00, ACC
rt_sf_length:
	PUSH ACC
	MOV #RT_SAVE_HI, VRMAD2
	MOV #RT_SAVE_LO+2, VRMAD1
	SET1 VSEL, 4
	MOV #$00, $03
rt_sf_chain:
	LD $03
	AND #$03
	BNZ rt_sf_skip
	LD C
	ST VTRBF
rt_sf_skip:
	INC $03
	DBNZ $02, rt_sf_next
	BR rt_sf_counted
rt_sf_next:
	LD C
	ROL
	ROL
	AND #$03
	ST TRL
	LD $7C
	CALLF rt_fl_addr
	CALLF rt_bios_fm_prd
	LD C
	ROL
	AND #$7E
	ADD #$81
	ST $00
	LD @R0
	BNZ rt_sf_counted
	DEC $00
	LD @R0
	ST C
	BR rt_sf_chain
rt_sf_counted:
	CLR1 VSEL, 4
	POP ACC
	ST TRL
	POP ACC
	ST TRH
	POP ACC
	ST B
	LD $03
	SUB B
	BN PSW, 7, rt_sf_walked
	LD $03
	ST B
rt_sf_walked:
	LD B
	ROL
	ROL
	AND #$FC
	SUB TRH
	BN PSW, 7, rt_sf_cap
	MOV #$00, ACC
rt_sf_cap:
	ST C
	LD TRL
	SUB C
	BP PSW, 7, rt_sf_by_length
	LD C
	BR rt_sf_store
rt_sf_by_length:
	LD TRL
rt_sf_store:
	MOV #RT_SAVE_HI, VRMAD2
	MOV #RT_SAVE_LO, VRMAD1
	ST VTRBF
	MOV #RT_SAVE_LO+1, VRMAD1
	LD TRH
	ST VTRBF
	MOV #$7C, $00
rt_sf_clear:
	MOV #$00, @R0
	INC $00
	LD $00
	BNZ rt_sf_clear
	MOV #$00, $02
	MOV #$00, $03
	RET

;----------------------
; | rt_sf_match
; | Description: Whether the directory entry at R1 in the page buffer is a data file named rt_save_name.
; | Author: suinevere
; | Dependencies: rt_save_name
; | Globals: RAM bank 1 0x00 (R0), the entry at R1
; | Params: R1 -- the entry's bank 1 address
; | Returns: ACC -- 0 on a match, else 1. Keeps B and C; destroys TRL, TRH, R0.
;----------------------
rt_sf_match:
	PUSH B
	PUSH C
	LD @R1
	BNE #$33, rt_sf_match_no
	LD $01
	ADD #$04
	ST $00
	MOV #<rt_save_name, TRL
	MOV #>rt_save_name, TRH
	MOV #12, B
rt_sf_match_loop:
	MOV #$00, ACC
	LDC
	XOR @R0
	BNZ rt_sf_match_no
	INC $00
	INC TRL
	LD TRL
	BNZ rt_sf_match_next
	INC TRH
rt_sf_match_next:
	DBNZ B, rt_sf_match_loop
	MOV #$00, ACC
	BR rt_sf_match_out
rt_sf_match_no:
	MOV #$01, ACC
rt_sf_match_out:
	POP C
	POP B
	RET

;----------------------
; | rt_fl_addr
; | Description: The BIOS flash arguments for page PAGE of block BLOCK: 0x7D the bank (block 128 and up is bank 1), 0x7E:0x7F block * 512 + PAGE * 128 within it.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RAM bank 1 0x7D-0x7F
; | Params: ACC -- block; TRL -- page, 0-3
; | Returns: N/A. Destroys ACC.
;----------------------
rt_fl_addr:
	ST $7E
	ROL
	AND #$01
	ST $7D
	LD $7E
	ROL
	AND #$FE
	ST $7E
	LD TRL
	AND #$02
	BZ rt_fl_addr_low
	INC $7E
rt_fl_addr_low:
	LD TRL
	ROR
	AND #$80
	ST $7F
	RET

;----------------------
; | rt_sv_ram
; | Description: Whether 128 bytes from a packed offset lie in work RAM before rt_save (0x100-0x268), where rt_sv_swap can trade them with the BIOS page buffer at bank 1 0x80, which bank 1 ranges would overlap.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: TRH:TRL -- packed offset
; | Returns: ACC -- 0 when they do, else 1
;----------------------
rt_sv_ram:
	LD TRH
	BE #$01, rt_sv_ram_ok
	BNE #$02, rt_sv_ram_bad
	LD TRL
	SUB #$69
	BP PSW, 7, rt_sv_ram_ok
rt_sv_ram_bad:
	MOV #$01, ACC
	RET
rt_sv_ram_ok:
	MOV #$00, ACC
	RET

;----------------------
; | rt_sv_locate
; | Description: The flash page of a pet save sector from rt_save: package page n = sector 0's page + sector.
; | Author: suinevere
; | Dependencies: rt_sv_page
; | Globals: rt_save (WRAM), RAM bank 1 0x7D-0x7F, VRMAD1, VRMAD2, VSEL
; | Params: ACC -- sector
; | Returns: ACC -- 0 with 0x7D-0x7F set and C the package page, or 1 for a sector past the save. Destroys B, C, TRL, TRH, FPR.
;----------------------
rt_sv_locate:
	ST C
	MOV #RT_SAVE_HI, VRMAD2
	MOV #RT_SAVE_LO, VRMAD1
	SET1 VSEL, 4
	LD VTRBF
	ST B
	LD C
	SUB B
	BP PSW, 7, rt_sv_locate_in
	CLR1 VSEL, 4
	MOV #$01, ACC
	RET
rt_sv_locate_in:
	LD VTRBF
	CLR1 VSEL, 4
	ADD C
	CALLF rt_sv_page
	MOV #$00, ACC
	RET

;----------------------
; | rt_sv_page
; | Description: The BIOS flash arguments for package page n of the pet save: chain block n / 4 through rt_fl_block, page n % 4.
; | Author: suinevere
; | Dependencies: rt_fl_block, rt_fl_addr
; | Globals: rt_save (WRAM), RAM bank 1 0x7D-0x7F, VRMAD1, VRMAD2, VSEL, FPR
; | Params: ACC -- package page
; | Returns: C -- the package page. Keeps B; destroys ACC, TRL, TRH, FPR, VRMAD.
;----------------------
rt_sv_page:
	ST C
	ROR
	ROR
	AND #$3F
	CALLF rt_fl_block
	PUSH ACC
	LD C
	AND #$03
	ST TRL
	POP ACC
	JMPF rt_fl_addr

;----------------------
; | rt_sv_swap
; | Description: Trades 128 bytes of work RAM with the BIOS page buffer at bank 1 0x80-0xFF, which holds game fields and rt_input, so a second call puts both back.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: RAM bank 1 0x00 (R0) and 0x80-0xFF, work RAM, VRMAD1, VRMAD2, VSEL
; | Params: TRH:TRL -- packed offset, 0x100-0x268 (interrupts masked)
; | Returns: N/A. Keeps C, TRL and TRH; destroys ACC, B, R0.
;----------------------
rt_sv_swap:
	LD TRH
	DEC ACC
	ST VRMAD2
	LD TRL
	ST VRMAD1
	CLR1 VSEL, 4
	MOV #$80, $00
rt_sv_swap_loop:
	LD VTRBF
	ST B
	LD @R0
	ST VTRBF
	LD B
	ST @R0
	INC VRMAD1
	LD VRMAD1
	BNZ rt_sv_swap_next
	INC VRMAD2
rt_sv_swap_next:
	INC $00
	LD $00
	BNZ rt_sv_swap_loop
	RET

;----------------------
; | rt_sv_delta
; | Description: The XMODEM CRC from 0 of the old flash page XOR the new page in the buffer, which by the CRC's linearity is the change the new page makes to the package CRC before the pages after it; the old page is read with LDF, as the buffer is taken.
; | Author: suinevere
; | Dependencies: rt_crc_hi, rt_crc_lo
; | Globals: RAM bank 1 0x00-0x01, 0x7C, 0x7D-0x7F (read), 0x80-0xFF (read), FPR
; | Params: bank 1 0x7D -- flash bank; 0x7E:0x7F -- the page's address; 0x80-0xFF -- the new page (interrupts masked)
; | Returns: R1 (0x01) -- the CRC's high byte; 0x7C -- its low byte. Destroys ACC, C, TRL, TRH, R0, FPR.
;----------------------
rt_sv_delta:
	LD $7D
	AND #$01
	ST FPR
	MOV #$00, $01
	MOV #$00, $7C
	MOV #$80, $00
rt_sv_delta_byte:
	LD $00
	AND #$7F
	OR $7F
	ST TRL
	LD $7E
	ST TRH
	LDF
	XOR @R0
	XOR $01
	ST C
	MOV #<rt_crc_hi, TRL
	MOV #>rt_crc_hi, TRH
	LDC
	XOR $7C
	ST $01
	MOV #<rt_crc_lo, TRL
	MOV #>rt_crc_lo, TRH
	LD C
	LDC
	ST $7C
	INC $00
	LD $00
	BNZ rt_sv_delta_byte
	RET

;----------------------
; | rt_sv_after
; | Description: How many package pages follow package page n, from the header page in the buffer and rt_save: its payload length / 128 plus sector 0's page, less n + 1.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: rt_save (WRAM), RAM bank 1 0x7E-0x7F, 0xC8-0xCA (the header's payload length), VRMAD1, VRMAD2, VSEL
; | Params: ACC -- package page n; 0x80-0xFF -- header page 0
; | Returns: 0x7E:0x7F -- the page count, high byte first. Destroys ACC, B.
;----------------------
rt_sv_after:
	INC ACC
	ST B
	LD $C8
	ROL
	AND #$01
	ST $7F
	LD $C9
	ROL
	AND #$FE
	OR $7F
	ST $7F
	LD $C9
	ROL
	AND #$01
	ST $7E
	LD $CA
	ROL
	AND #$FE
	OR $7E
	ST $7E
	MOV #RT_SAVE_HI, VRMAD2
	MOV #RT_SAVE_LO+1, VRMAD1
	CLR1 VSEL, 4
	LD VTRBF
	ADD $7F
	ST $7F
	LD $7E
	ADDC #$00
	ST $7E
	LD $7F
	SUB B
	ST $7F
	LD $7E
	SUBC #$00
	ST $7E
	RET

;----------------------
; | rt_sv_shift
; | Description: Carries a CRC change past N pages of unchanged bytes: multiplies it by x^(1024 N) modulo the XMODEM polynomial, one rt_crc_pow factor for each set bit of N, each a 16-step shift-and-add product.
; | Author: suinevere
; | Dependencies: rt_crc_pow
; | Globals: RAM bank 1 0x00-0x01, 0x7C-0x7F
; | Params: R1 (0x01):0x7C -- the change, high byte first; 0x7E:0x7F -- N
; | Returns: R1 (0x01):0x7C -- the carried change. Destroys ACC, B, C, TRL, TRH, R0, 0x7D-0x7F.
;----------------------
rt_sv_shift:
	MOV #$00, $7D
rt_sv_shift_bit:
	LD $7E
	OR $7F
	BZ rt_sv_shift_done
	CLR1 PSW, 7
	LD $7E
	RORC
	ST $7E
	LD $7F
	RORC
	ST $7F
	BN PSW, 7, rt_sv_shift_next
	MOV #<rt_crc_pow, TRL
	MOV #>rt_crc_pow, TRH
	LD $7D
	LDC
	ST B
	LD $7D
	INC ACC
	LDC
	ST C
	MOV #$00, TRL
	MOV #$00, TRH
	MOV #16, $00
rt_sv_shift_step:
	LD TRL
	ADD TRL
	ST TRL
	LD TRH
	ADDC TRH
	ST TRH
	BN PSW, 7, rt_sv_shift_kept
	LD TRL
	XOR #$21
	ST TRL
	LD TRH
	XOR #$10
	ST TRH
rt_sv_shift_kept:
	LD C
	ADD C
	ST C
	LD B
	ADDC B
	ST B
	BN PSW, 7, rt_sv_shift_skip
	LD TRL
	XOR $7C
	ST TRL
	LD TRH
	XOR $01
	ST TRH
rt_sv_shift_skip:
	DBNZ $00, rt_sv_shift_step
	LD TRL
	ST $7C
	LD TRH
	ST $01
rt_sv_shift_next:
	INC $7D
	INC $7D
	BR rt_sv_shift_bit
rt_sv_shift_done:
	RET

;----------------------
; | rt_save_read
; | Description: vmu_save_read: with interrupts masked, swaps the RAM with the page buffer, reads the sector's page into it through FM_PRD_EX and swaps back, keeping bank 1's 0x7D-0xFF as they were.
; | Author: suinevere
; | Dependencies: rt_sv_ram, rt_sv_locate, rt_sv_swap, rt_bios_fm_prd
; | Globals: vmu_ram (work RAM), RAM bank 1 0x7D-0xFF (kept), rt_save
; | Params: ACC -- sector; TRH:TRL -- packed RAM offset, 0x100-0x268
; | Returns: ACC -- 0, or 1 for a sector past the save or another offset. Keeps TRH, TRL and IE; destroys B, C, R0, FPR.
;----------------------
rt_save_read:
	PUSH IE
	CLR1 IE, 7
	ST C
	CALLF rt_sv_ram
	BZ rt_sr_ram
	POP IE
	RET
rt_sr_ram:
	PUSH $7D
	PUSH $7E
	PUSH $7F
	PUSH TRL
	PUSH TRH
	LD C
	CALLF rt_sv_locate
	BNZ rt_sr_fail
	POP TRH
	POP TRL
	PUSH TRL
	PUSH TRH
	CALLF rt_sv_swap
	CALLF rt_bios_fm_prd
	POP TRH
	POP TRL
	CALLF rt_sv_swap
	MOV #$00, C
	BR rt_sr_out
rt_sr_fail:
	MOV #$01, C
	POP TRH
	POP TRL
rt_sr_out:
	POP $7F
	POP $7E
	POP $7D
	LD C
	POP IE
	RET

;----------------------
; | rt_save_write
; | Description: vmu_save_write: with interrupts masked, swaps the RAM with the page buffer, takes the CRC change from the old page by LDF, programs the page through the BIOS page write and checks it with FM_VRF_EX; then patches the header's CRC with that change carried past the later pages, writes header page 0 the same way, reads the sector's page back and swaps back, keeping bank 1's 0x7C-0xFF.
; | Author: suinevere
; | Dependencies: rt_sv_ram, rt_sv_locate, rt_sv_page, rt_sv_swap, rt_sv_delta, rt_sv_after, rt_sv_shift, rt_bios_fm_page, rt_bios_fm_vrf, rt_bios_fm_prd
; | Globals: vmu_ram (work RAM), RAM bank 1 0x7C-0xFF (kept), rt_save, the pet's flash page and header page
; | Params: ACC -- sector; TRH:TRL -- packed RAM offset, 0x100-0x268
; | Returns: ACC -- 0, or 1 for a sector past the save, another offset or a page that does not verify. Keeps TRH, TRL and IE; destroys B, C, R0, R1, FPR.
;----------------------
rt_save_write:
	PUSH IE
	CLR1 IE, 7
	ST C
	CALLF rt_sv_ram
	BZ rt_sw_ram
	POP IE
	RET
rt_sw_ram:
	PUSH $7C
	PUSH $7D
	PUSH $7E
	PUSH $7F
	PUSH TRL
	PUSH TRH
	LD C
	CALLF rt_sv_locate
	BZ rt_sw_found
	MOV #$01, C
	POP TRH
	POP TRL
	JMPF rt_sw_out
rt_sw_found:
	POP TRH
	POP TRL
	PUSH C
	PUSH TRL
	PUSH TRH
	CALLF rt_sv_swap
	CALLF rt_sv_delta
	PUSH $01
	PUSH $7C
	MOV #$01, $7C
	CALLF rt_bios_fm_page
	CALLF rt_bios_fm_vrf
	BZ rt_sw_crc
	ST C
	POP ACC
	POP ACC
	POP TRH
	POP TRL
	POP ACC
	CALLF rt_sv_swap
	JMPF rt_sw_out
rt_sw_crc:
	MOV #$00, ACC
	CALLF rt_sv_page
	CALLF rt_bios_fm_prd
	POP $7C
	POP $01
	POP TRH
	POP TRL
	POP ACC
	PUSH ACC
	PUSH TRL
	PUSH TRH
	CALLF rt_sv_after
	CALLF rt_sv_shift
	LD $7C
	XOR $C6
	ST $C6
	LD $01
	XOR $C7
	ST $C7
	MOV #$00, ACC
	CALLF rt_sv_page
	MOV #$01, $7C
	CALLF rt_bios_fm_page
	CALLF rt_bios_fm_vrf
	ST B
	POP TRH
	POP TRL
	POP ACC
	PUSH B
	PUSH TRL
	PUSH TRH
	CALLF rt_sv_page
	CALLF rt_bios_fm_prd
	POP TRH
	POP TRL
	CALLF rt_sv_swap
	POP C
rt_sw_out:
	POP $7F
	POP $7E
	POP $7D
	POP $7C
	LD C
	BZ rt_sw_ret
	MOV #$01, ACC
rt_sw_ret:
	POP IE
	RET

;----------------------
; | rt_crc_hi / rt_crc_lo
; | Description: The XMODEM CRC's byte table, high and low halves: entry i is the CRC of i followed by eight zero bits, so crc = (crc << 8) ^ table[(crc >> 8) ^ byte].
; | Author: suinevere
; | Dependencies: dreamcast/tools/vmu_image.py (vms_crc)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
rt_crc_hi:
	.byte $00, $10, $20, $30, $40, $50, $60, $70, $81, $91, $A1, $B1, $C1, $D1, $E1, $F1
	.byte $12, $02, $32, $22, $52, $42, $72, $62, $93, $83, $B3, $A3, $D3, $C3, $F3, $E3
	.byte $24, $34, $04, $14, $64, $74, $44, $54, $A5, $B5, $85, $95, $E5, $F5, $C5, $D5
	.byte $36, $26, $16, $06, $76, $66, $56, $46, $B7, $A7, $97, $87, $F7, $E7, $D7, $C7
	.byte $48, $58, $68, $78, $08, $18, $28, $38, $C9, $D9, $E9, $F9, $89, $99, $A9, $B9
	.byte $5A, $4A, $7A, $6A, $1A, $0A, $3A, $2A, $DB, $CB, $FB, $EB, $9B, $8B, $BB, $AB
	.byte $6C, $7C, $4C, $5C, $2C, $3C, $0C, $1C, $ED, $FD, $CD, $DD, $AD, $BD, $8D, $9D
	.byte $7E, $6E, $5E, $4E, $3E, $2E, $1E, $0E, $FF, $EF, $DF, $CF, $BF, $AF, $9F, $8F
	.byte $91, $81, $B1, $A1, $D1, $C1, $F1, $E1, $10, $00, $30, $20, $50, $40, $70, $60
	.byte $83, $93, $A3, $B3, $C3, $D3, $E3, $F3, $02, $12, $22, $32, $42, $52, $62, $72
	.byte $B5, $A5, $95, $85, $F5, $E5, $D5, $C5, $34, $24, $14, $04, $74, $64, $54, $44
	.byte $A7, $B7, $87, $97, $E7, $F7, $C7, $D7, $26, $36, $06, $16, $66, $76, $46, $56
	.byte $D9, $C9, $F9, $E9, $99, $89, $B9, $A9, $58, $48, $78, $68, $18, $08, $38, $28
	.byte $CB, $DB, $EB, $FB, $8B, $9B, $AB, $BB, $4A, $5A, $6A, $7A, $0A, $1A, $2A, $3A
	.byte $FD, $ED, $DD, $CD, $BD, $AD, $9D, $8D, $7C, $6C, $5C, $4C, $3C, $2C, $1C, $0C
	.byte $EF, $FF, $CF, $DF, $AF, $BF, $8F, $9F, $6E, $7E, $4E, $5E, $2E, $3E, $0E, $1E
rt_crc_lo:
	.byte $00, $21, $42, $63, $84, $A5, $C6, $E7, $08, $29, $4A, $6B, $8C, $AD, $CE, $EF
	.byte $31, $10, $73, $52, $B5, $94, $F7, $D6, $39, $18, $7B, $5A, $BD, $9C, $FF, $DE
	.byte $62, $43, $20, $01, $E6, $C7, $A4, $85, $6A, $4B, $28, $09, $EE, $CF, $AC, $8D
	.byte $53, $72, $11, $30, $D7, $F6, $95, $B4, $5B, $7A, $19, $38, $DF, $FE, $9D, $BC
	.byte $C4, $E5, $86, $A7, $40, $61, $02, $23, $CC, $ED, $8E, $AF, $48, $69, $0A, $2B
	.byte $F5, $D4, $B7, $96, $71, $50, $33, $12, $FD, $DC, $BF, $9E, $79, $58, $3B, $1A
	.byte $A6, $87, $E4, $C5, $22, $03, $60, $41, $AE, $8F, $EC, $CD, $2A, $0B, $68, $49
	.byte $97, $B6, $D5, $F4, $13, $32, $51, $70, $9F, $BE, $DD, $FC, $1B, $3A, $59, $78
	.byte $88, $A9, $CA, $EB, $0C, $2D, $4E, $6F, $80, $A1, $C2, $E3, $04, $25, $46, $67
	.byte $B9, $98, $FB, $DA, $3D, $1C, $7F, $5E, $B1, $90, $F3, $D2, $35, $14, $77, $56
	.byte $EA, $CB, $A8, $89, $6E, $4F, $2C, $0D, $E2, $C3, $A0, $81, $66, $47, $24, $05
	.byte $DB, $FA, $99, $B8, $5F, $7E, $1D, $3C, $D3, $F2, $91, $B0, $57, $76, $15, $34
	.byte $4C, $6D, $0E, $2F, $C8, $E9, $8A, $AB, $44, $65, $06, $27, $C0, $E1, $82, $A3
	.byte $7D, $5C, $3F, $1E, $F9, $D8, $BB, $9A, $75, $54, $37, $16, $F1, $D0, $B3, $92
	.byte $2E, $0F, $6C, $4D, $AA, $8B, $E8, $C9, $26, $07, $64, $45, $A2, $83, $E0, $C1
	.byte $1F, $3E, $5D, $7C, $9B, $BA, $D9, $F8, $17, $36, $55, $74, $93, $B2, $D1, $F0

;----------------------
; | rt_crc_pow
; | Description: x^(1024 * 2^k) modulo the XMODEM polynomial for k = 0-15, high byte first: the factor that carries a CRC change past 2^k pages of unchanged bytes.
; | Author: suinevere
; | Dependencies: dreamcast/tools/vmu_image.py (vms_crc)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
rt_crc_pow:
	.byte $36, $C4, $FD, $50, $AA, $9E, $88, $1C, $44, $58, $00, $02, $00, $04, $00, $10
	.byte $01, $00, $10, $21, $37, $30, $B8, $61, $AE, $FC, $8E, $29, $13, $FC, $36, $C4
