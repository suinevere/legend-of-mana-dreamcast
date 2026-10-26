# VMU game

[English documentation](../../README.md) | [日本語](../../../jp/technical/reference/vmu-game.md) | [VMU runtime](vmu-runtime.md) | [Ring Ring Land RAM](ring-ring-land-ram.md)

Ring Ring Land on the VMU is a GAME file written in LC86K assembly. Every
function of the VMU-shaped C in `pocketstation/src/vmu/` became a routine. The C
is still the oracle: each routine is checked against recorded calls of its C
function, and the whole GAME is checked frame by frame against `vmu_runner` on
all seven scripts. This page covers how the sources are laid out, the manifest
that tracks every function, how one routine is checked, and the whole-game
checks. The runtime the game runs on, with its calling convention and RAM, is
on the [VMU runtime](vmu-runtime.md) page.

## Sources

The sources are in `pocketstation/src/vmu_game/`.

| File | What it holds |
|---|---|
| `game.s` | The GAME's top level. It includes the runtime, puts the VMS header and icon at 0x200, includes the routines, and ends with `app_main` and the tables. |
| `callimg.s`, `callbase.inc` | The `--call` test image: the same routines with a small base that loads a recorded state and calls one routine. |
| `<file>.s` | One file per C file (`board.s` for `board.c`, and so on), listed in `routines.inc`. |
| `game.inc` | Shared equates: `ARG_BLOCK`, the work RAM block that holds arguments too wide for registers. |
| `manifest.txt` | One line per C function, described below. |
| `untaken.txt` | Branch outcomes that no record takes, each with the reason it is acceptable. |
| `Makefile` | `call` builds the test image and `hw.json`; `game` builds `rrl.vms` and `rrl.map`; `fixtures` builds the test programs. |

The tables (`tables.s`, 96 pages of `RES1506.BIN`) are generated at build time
by `pocketstation/tools/gen_tables.py` into `pocketstation/build/vmu_game/`.
Nothing derived from the disc is committed. Both images go through
`pocketstation/tools/vmu_relax.py`, which shrinks a 3-byte `CALLF` or `JMPF` to
a 2-byte `CALL`, `JMP` or `BR` wherever the target is in reach. It keeps a
`JMPF` far when the line next to it is also a `JMPF`, since that is a jump
table, so new jump tables must keep that form.

## GAME layout

- `rt_vectors.s` fills 0x000-0x1F5, then comes the VMS header at 0x200 and one
  icon, ending at 0x480.
- The runtime, then every routine.
- `app_main`: it leaves through `rt_power` when the VMU has no pet save.
  Otherwise it jumps to `g_entry`, which masks every line, installs
  `g_irq_callback` with `rt_set_handler`, unmasks the RTC and dock lines and
  falls into `g_main_loop`. Neither returns. The runtime has already cleared RAM
  before `app_main`, so the C's clear of `vmu_ram` is not repeated.
- The table pages, 256-aligned, with `g_bit_rot` right after them: eight pages,
  each holding every byte bit-reversed and rotated right by the page number.
  The sprite plotters use it to shift a row by x in one `LDC`.

At R3b-3's close the GAME is 61,684 bytes, 121 of flash bank 0's 128 blocks.
The pet save is 5 blocks. With two 14-block Legend of Mana saves, 46 of the
VMU's 200 user blocks are spare.

## Manifest

`manifest.txt` has one line per C function, including `game.h`'s inline
accessors, with eight columns:

```
NAME FILE LEVEL ROUTINE|inlined:CALLER ARGS RESULT DESTROYS STATUS
```

- **LEVEL** comes from `pocketstation/tools/vmu_callgraph.py`. Leaves are level
  0, every callee sits below its caller, and a recursion group shares one
  level. The routines were ported level by level, callees first.
- **ARGS** maps each parameter to `ACC`, `B`, `C`, `TR` (16 bits in TRH:TRL),
  `TRB` (24 bits in TRH:TRL:B) or `mem:NAME`, a slot in `ARG_BLOCK` (work RAM
  0x1DB-0x1E7), in order, little-endian.
- **RESULT** is a register, `mem:NAME` after the `mem:` arguments, `over:NAME`
  at `ARG_BLOCK+0` over value arguments the routine has already read, or `-`.
- **DESTROYS** lists what the routine may change. `--call` poisons those
  registers with 0xA5 when the routine returns, and a routine that changes
  anything else fails.
- **STATUS** is one of the following.
  - `verified`: the routine passes every scripted record.
  - `synthetic`: no script reaches the function, so it passes records built by
    `pocketstation/tests/host/record_synthetic.c`.
  - `reading:callers`: it is checked through its callers' records because its
    result fits no slot (only `game_frame_info`, whose 16-byte result does not
    fit the 13-byte `ARG_BLOCK`).
  - `reading:never-returns`: it never returns, so the whole-game run checks it.
    These are `game_entry`, `game_main_loop`, `game_quit` and `game_dock_quit`;
    no script quits or docks, so the last two were checked by reading.
  - `todo`: not ported. Only the nine inline accessors carry it, because they
    are inlined into their callers.

At the close, 182 functions are verified, 34 synthetic, 1 `reading:callers` and
4 `reading:never-returns`.

Routines that own no `ARG_BLOCK` slot but call routines that use it save the
slot around their body through `g_keep13`, `g_keep9`, `g_keep5` or `g_keep1`.
They put the body's address in R0:R1 and enter by `JMPF`, so the harness's
shadow call stack stays balanced.

## Checking one routine

1. `make -C pocketstation vmu-record` builds `vmu_runner-record`, whose
   generated layer (`pocketstation/tools/gen_record.py`) records sampled calls of
   every function: RAM, frame buffer, arguments, result and hardware calls. The
   sampler keeps at least one call for each distinct set of C branches taken.
   `test_vmu_game.py`'s `recordings` fixture runs the seven scripts and
   `record_synthetic` into `pocketstation/build/record/rec`.
2. `make -C pocketstation/harness` and `make -C pocketstation/src/vmu_game call`.
3. `python pocketstation/tools/call_compare.py FUNCTION`. Each record becomes a
   state file. `vmu_harness --call ROUTINE` loads it into bank 1, work RAM and
   XRAM, runs the routine and writes what it leaves. The compare checks every
   byte of bank 1, work RAM and XRAM, the result and the hardware calls. It
   prints `ok N` for N records, or the first difference. `--cycles` prints the
   routine's cycles per call.

`test_level_passes[N]` runs this for every routine of level N, and
`test_level_outcomes_covered[N]` requires every branch outcome of the level to
be taken by a passing record or listed in `untaken.txt`.

## Whole-game checks

`test_vmu_game.py`'s `whole_game` fixture runs each script twice: once through
`vmu_runner`, and once through the GAME under
`vmu_harness --schedule real --report`, with the pet save in the flash image.

- `test_vmu_game_matches_vmu_runner[SCRIPT]`: every frame dump (the 1 KB
  `vmu_ram` image and the 192-byte frame buffer) is equal on all seven scripts.
  `save` includes a reboot.
- `test_vmu_game_pet_save_matches[SCRIPT]`: the `BASLUSP01013` payload the GAME
  leaves equals the C's save packaged the same way, and the package CRC is
  valid. Of the seven, only `laps` changes saved bytes (0x300-0x30C); `race` and
  `save` write values the reference pet already holds.
- `test_race_and_menu_frames_fit`: on `race`, `menu` and `buttons`, the frames
  that outlast their tick are listed in `pocketstation/build/vmu_game/overruns.txt`
  with frame, scene (the `mode` field) and why. A frame excused as `changed`
  (its scene changed) or `saved` (a flash write ran) is allowed. The test fails
  if the unexcused ones grow past the counts accepted at R3b-3's close (race
  320, menu 0, buttons 588) or fall outside the board scenes 3, 4, 6 and 7.
- `test_stack_fits`: the deepest stack pointer seen is at most 0xF0. It is 0xE1
  on race, buttons, laps and save. The BIOS clock path adds about 15-20 bytes
  on hardware, which the harness does not run, so the stack reaches about 0xF5
  there, still under 0xFF.
- `test_game_block_count_recorded`: it writes the block count to
  `pocketstation/build/vmu_game/blocks.txt`, and the GAME must stay four blocks
  under 128.

`vmu_harness --report --map rrl.map --profile OUT` also writes one
`NAME ALL OVER` line per label: the cycles executed from that label to the next
over all frames and over overrun frames only. This is how to find what to
optimise in a frame that overruns.
