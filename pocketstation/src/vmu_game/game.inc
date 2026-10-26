;----------------------
; | game.inc
; | Description: Equates the assembly game shares with call_compare.py, after gen_ram.py hw's field equates.
; | Author: suinevere
; | Dependencies: pocketstation/build/vmu_game/rrl_ram.inc, pocketstation/tools/call_compare.py (ARG_BLOCK)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------

;----------------------
; | ARG_BLOCK / ARG_BLOCK_SIZE
; | Description: The work RAM bytes no field uses (vmu_ram 0x2DB-0x2E7) where a routine's mem: arguments sit in manifest order, each its C size, followed by a mem: result; reached through VRMAD and VTRBF.
; | Author: suinevere
; | Dependencies: pocketstation/src/vmu_game/manifest.txt (ARGS, RESULT)
; | Globals: N/A
; | Params: N/A
; | Returns: N/A
;----------------------
ARG_BLOCK	EQU	$1DB
ARG_BLOCK_SIZE	EQU	13
