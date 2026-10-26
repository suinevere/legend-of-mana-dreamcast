;----------------------
; | relax.s
; | Description: vmu_relax.py's fixture: a near call, a branchable jump, a far call across a 4 KB block, a JMPF table, a numeric target and a macro body.
; | Author: suinevere
; | Dependencies: N/A
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
%macro FARCALL
	CALLF r_near
%end

	.org $0200
r_start:
	CALLF r_near
	JMPF r_close
r_close:
	CALLF r_far
	JMPF r_far
	JMPF r_near
	JMPF r_close
r_table_end:
	CALLF $0100
r_numeric_end:
	FARCALL
r_macro_end:
	RET
r_near:
	RET

	.org $1000
r_far:
	RET
