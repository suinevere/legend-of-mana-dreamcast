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
; | Description: The XMODEM CRC from 0 of the old flash page XOR the new buffered page, the change it makes to the package CRC before later pages. Each byte step uses shifts, not a table.
; | Author: suinevere
; | Dependencies: N/A
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
	ROL
	ROL
	ROL
	ROL
	AND #$0F
	XOR C
	ST C
	ROR
	ROR
	ROR
	ST TRL
	LD C
	ROL
	ROL
	ROL
	ROL
	AND #$F0
	XOR $7C
	ST $01
	LD TRL
	AND #$1F
	XOR $01
	ST $01
	LD TRL
	AND #$E0
	XOR C
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
