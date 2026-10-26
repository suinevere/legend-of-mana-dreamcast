# VMU runtime

[English documentation](../../README.md) | [日本語](../../../jp/technical/reference/vmu-runtime.md) | [Ring Ring Land RAM](ring-ring-land-ram.md) | [Save file format](save-file.md)

Ring Ring Land's VMU port runs on a small runtime written in LC86K assembly. The
runtime gives the game the interface the VMU-shaped C uses,
`pocketstation/src/vmu/vmu_hw.h`, on the real VMU's timers, buttons, screen and
flash. This page is the runtime's reference: how to call it, where it keeps its
state, how it orders events, and what the test harness models of the BIOS.

The sources are in `pocketstation/src/vmu_rt/`. `rt_test.s` in its `test/`
folder is a test GAME that runs a shared scenario
(`pocketstation/tests/scenarios/*.scn`). The same scenario also runs as
`scenario_host` on `vmu_host.c`, the C model of the interface, and
`pocketstation/tests/test_runtime.py` requires the two event logs to match line
for line. The runtime has only run under the harness so far. The first real VMU
test belongs to the Dreamcast install milestone.

## Clock

The BIOS starts a GAME on the 32 kHz quartz oscillator. The runtime's reset
switches to the 6 MHz CF oscillator divided by 6, the fastest setting a VMU
keeps without a Dreamcast. That gives an instruction cycle of 1 microsecond, so
one 30.76 ms game frame (32.5 Hz) is about 30,760 cycles. The RC oscillator,
which the public documents suggest for an undocked VMU, would give only 1,500
to 2,250 cycles a frame.

## Calling convention

- Call every routine with `CALLF`. Arguments go in ACC, B, C and the 16-bit
  pair TRH:TRL. Results come back in ACC, or in TRH:TRL for an interrupt-line
  mask. Each routine's header in the source lists what else it destroys; the
  table below repeats it.
- RAM bank 1 is always selected (PSW `0x02`). The interrupt routines assume
  it, so code that switches to bank 0 must mask interrupts first.
- The stack is bank 0's, from `0x80` up, as the BIOS sets it.
- A routine that touches state an interrupt also touches clears IE bit 7 and
  puts IE back as it was. The game's interrupt handler is called from
  `rt_idle` with interrupts masked.
- Interrupt routines keep every register.

## What a GAME supplies

`rt_test.s` is the model. A GAME built on the runtime must:

- Include `sfr.inc`, `rt.inc` and then `rt_vectors.s` before anything else.
  `rt_vectors.s` fills `0x000`-`0x1F5`: the vectors, the reset path and, at
  `0x100`-`0x1F5`, the stubs and trampolines the BIOS returns through. Nothing
  else may be placed there.
- Put its VMS header at `0x200`, followed by the palette and icons. With one
  icon they end at `0x480`, and each further icon takes `0x200` more.
- Include the rest of the runtime (`rt_ram.s`, `rt_events.s`, `rt_input.s`,
  `rt_lcd.s`, `rt_file.s`, `rt_sound.s`, `rt_power.s`) anywhere after the
  icons.
- Define `app_main`, which the reset calls once RAM is cleared and the
  scheduler is running. If it returns, the VMU halts for good.
- Supply `app_tables`, the label `rt_rom8` adds a PocketStation-file offset to
  (see [Tables](#tables)).

The handler installed with `rt_set_handler` runs with interrupts masked. Timer
0 keeps only one pending overflow, so a handler, or any masked section, that
runs longer than one Timer 0 segment loses the overflows after the first, and
the scheduler falls behind real time.

## Memory

| Where | Bytes | Holds |
|---|---|---|
| Bank 1 `0x00`-`0x1F` | 32 | `rt_reserved`: the indirect registers R0-R3 at `0x00`-`0x03`, then the scheduler's counts, state and line masks |
| Bank 1 `0x20`-`0xF8` | 217 | the game's busiest fields, directly addressed |
| Bank 1 `0xF9`-`0xFF` | 7 | `rt_input`: buttons held, buttons read as released until let go, and the Sleep, quit and LCD flags (3 bytes used) |
| WRAM `0x000`-`0x1E7` | 488 | the game's other fields, read and written through `rt_r8` and `rt_w8` |
| WRAM `0x1E8`-`0x1FF` | 24 | `rt_save`: the pet save's sector count, sector 0's package page and every fourth of the first 88 blocks of its chain, found once at reset |
| Bank 0 from `0x80` | | the stack |
| XRAM | 192 | the screen, which the game draws into directly |

The game's fields carry `vmu_ram` offsets, which the
[Ring Ring Land RAM](ring-ring-land-ram.md) page maps. An offset below `0x100`
is bank 1 at the same address, and the rest is WRAM at the offset less `0x100`.
The VMU has no separate copy of `vmu_ram`'s 32-byte stack reserve.

## Routines

| Routine | `vmu_hw.h` | Arguments | Returns | Also destroys |
|---|---|---|---|---|
| `rt_r8` | `vmu_r8` | TRH:TRL offset `0x020`-`0x2FF` | ACC byte | R0, VRMAD |
| `rt_w8` | `vmu_w8` | TRH:TRL offset, ACC value | - | R0, VRMAD |
| `rt_rom8` | `vmu_rom8` | TRH:TRL offset into the PocketStation file | ACC byte, read with LDC at `app_tables` + offset | - |
| `rt_fl_block` | - | ACC block k of the pet save's chain | ACC its flash block | TRL, TRH, FPR, VRMAD |
| `rt_tick_mode` | `vmu_tick_mode` | ACC `RT_TICK_*` (0 off, 1 frame, 2 fast, 3 standby) | - | B, C, TRL, TRH, R1 |
| `rt_sound_play` | `vmu_sound_play` | TRH:TRL:B samples (24-bit), C rate (0 slow) | - | B, C, TRL, TRH, R1 |
| `rt_sound_stop` | `vmu_sound_stop` | - | - | B, C, TRL, TRH, R1 |
| `rt_irq_enable`, `rt_irq_disable` | `vmu_irq_enable`, `vmu_irq_disable` | TRH:TRL lines | - | - |
| `rt_irq_ack` | `vmu_irq_ack` | TRH:TRL lines | - | - |
| `rt_irq_pending`, `rt_irq_levels`, `rt_irq_mask` | `vmu_irq_pending`, `vmu_irq_levels`, `vmu_irq_mask` | - | TRH:TRL lines | - (B and C kept) |
| `rt_set_handler` | `vmu_set_handler` | TRH:TRL code address, or 0 | - | - |
| `rt_idle` | `vmu_idle` | - | - | B, C, TRL, TRH, R1 and whatever the handler destroys |
| `rt_buttons` | `vmu_buttons` | - | ACC `VMU_BTN_*` | B, C |
| `rt_sys_flags` | `vmu_sys_flags` | - | ACC `VMU_SYS_*` | B, C |
| `rt_fb_row` | (the frame buffer) | ACC row 0-31 | B the XBNK value, C the row's address less `0x100` | - (TRL, TRH kept) |
| `rt_present` | `vmu_present` | - | - | - |
| `rt_lcd` | `vmu_lcd` | ACC nonzero for on | - | - |
| `rt_buzz` | `vmu_buzz` | ACC tone 1-255, 0 for silence | - | B, C |
| `rt_save_read` | `vmu_save_read` | ACC sector, TRH:TRL offset `0x100`-`0x268` | ACC 0, or 1 on a bad sector or offset | B, C, R0, FPR |
| `rt_save_write` | `vmu_save_write` | ACC sector, TRH:TRL offset `0x100`-`0x268` | ACC 0, or 1 on a bad sector or offset or a page that does not verify | B, C, R0, R1, FPR |
| `rt_power` | `vmu_power` | ACC `RT_POWER_*` (standby, exit, exit docked) | standby returns; the exits never do | as `rt_idle` |

Every routine that does not return a value in ACC destroys it, except `rt_w8`,
`rt_present` and `rt_lcd`, which keep it.
The lines are `vmu_hw.h`'s: buttons `0x001F`, tick `0x0100`, RTC `0x0200`, dock
`0x0400` and sound `0x2000`.

The frame buffer lives in XRAM as the screen shows it, in VMU byte order. Bank 0
holds rows 0-15 and bank 1 rows 16-31, two 6-byte rows then a 4-byte gap.
`rt_fb_row` gives a row's place, and the game writes it through R2 or R3.
The game draws only bytes 1-4 of each row, so the reset's `rt_xram_clear`
zeroes all six once; otherwise the BIOS menu's leftovers show at the sides.
`rt_present` only marks the frame for the harness, as the XRAM rows are already
the screen.

## Scheduler

`vmu_host.c` defines when each event falls, in timer counts, and the runtime
ports its arithmetic:

- A tick mode's period is its reload plus one: 15,626 counts in frame and fast
  mode, 1,954 in standby.
- A sound ends after (samples + 1) * (counts per sample + 1) counts, 501
  counts a sample slow and 72 fast.
- The RTC flips every half-period of the current clock: 999,424 counts at the
  launcher's clock, 253,952 in frame mode.
- Events due on the same count fire together: the sound end first, then the
  tick and RTC in one handler call.

The order is computed in software, never taken from a hardware timer:

- Each `rt_idle` call takes exactly one event. It computes the nearest, halts
  until Timer 0 has run that far, steps that event, and calls the handler while
  an enabled line is latched.
- Timer 0 only measures elapsed time. Its overflow adds whole periods to a
  count (32 counts a period, 16 on clock 2, standby's and the slowest) and
  wakes the halt. It never fires an event, so the order is always the host's,
  even when the game is late.
- No elapsed count is lost when an overflow comes before it is taken: a
  restart adds a pending overflow's reload periods to the periods run, and a
  restart that finds the event already due credits the segment at once and
  carries the rest into the next one.
- When a tick mode starts or changes, the runtime waits up to one Timer 0
  period for a period boundary, so it loses no time; in frame mode that is up to
  63 cycles.

## Buttons, Sleep and Mode

- The buttons are polled at every `rt_idle` wake, by the base timer interrupt
  twice a second, and by the port 3 interrupt when an armed line goes down. A
  poll returns at once when port 3 has not changed.
- Sleep is invisible to the game. `rt_idle` stops Timer 0 with the counts run so
  far kept, blanks the LCD and halts until Sleep is pressed again; then it
  restores the LCD and restarts the schedule where it stopped. Buttons held at
  the wake read as released until they are let go, so the game sees them only
  after a new press.
- Mode does not leave the game. It sets `VMU_SYS_QUIT`, which the next
  `rt_sys_flags` read returns once and clears, and the game decides what to do.
- The dock line follows port 7 through INT0.

## BIOS calls and the pet save

The runtime calls the BIOS through stubs at fixed addresses in the GAME's first
512 bytes:

| GAME address | Calls | Used for |
|---|---|---|
| `0x0100` | FM_WRT_EX (ROM `0x0100`) | (not used; its RET at `0x0105` is where the page write comes back) |
| `0x0110` | FM_VRF_EX (ROM `0x0110`) | checking a written save page |
| `0x0120` | FM_PRD_EX (ROM `0x0120`) | reading the directory, the save and the header page that holds its CRC |
| `0x0130` | TIMER_EX (ROM `0x0130`) | the BIOS clock, on every base timer interrupt |
| `0x0150` | the page write at ROM `0xE024` | writing a save page |
| `0x01F0` | the mode change (ROM `0x01F0`) | leaving the game |

The pet save is `BASLUSP01013`, the file the Dreamcast port writes when it sends
a pet. The port keeps only the PS1 file's first `0x400` bytes, uncompressed,
since CARDA reads nothing past them and the game's pet sector is sector 6; it
reads the rest back as zeros. Every other save is compressed. The pet save must
stay a valid package for the port to read it back, so the runtime writes it
through
the BIOS's own page write, the one the Maple block-write command uses. FM_WRT_EX
refuses flash bank 1, where data files live; the page write at `0xE024` takes
either bank. The stub pushes ROM `0x0103` as the return address, so the page
write's RET runs FM_WRT_EX's tail back to the GAME at `0x0105`. Each sector
write is checked with FM_VRF_EX. The runtime then updates the package CRC
(XMODEM, as `vmu_image.vms_crc` does) and writes the header page holding it the
same way.

The CRC covers the whole package: five blocks for the slim pet. The runtime
caps the sector count at the first 22 blocks of the chain and patches the
stored CRC instead of recomputing it. XMODEM from 0 is linear:

1. Before the page write, while the page buffer holds the new page, the runtime
   reads the old page with LDF and takes the CRC of old XOR new.
2. It multiplies that by x^(1024 N) modulo the polynomial, where N is the number
   of package pages after the written one. The factors come from a 16-entry
   table, `rt_crc_pow`, one for each bit of N.
3. It XORs the result into the CRC the header already holds.

A package whose stored CRC was already wrong stays wrong. LDF from a GAME has
not yet been checked on hardware; R3b-4 is the first hardware run.

A save keeps interrupts masked for about 9,900 harness cycles (10 ms at 1 MHz). The harness does not count the BIOS calls on top of that: two
page writes, two FM_VRF_EX and two FM_PRD_EX. Timer 0 overflows beyond the first
are lost during that time, so the schedule's pacing lags by that much; the order
of events is unaffected.

## Tables

Ring Ring Land's tables are the 96 pages `0x4A00`-`0xA9FF` of the PocketStation
file, carried in the GAME's own flash as `app_tb_pages` and built from
`RES1506.BIN` at build time. `app_tables` is `app_tb_pages - 0x4A00`, so
`rt_rom8` reads file offset `off` with LDC at `app_tables + off`. Each page `PP`
also has an equate `app_tb_PP`, so `app_tb_PP + off` reads any offset of the
run, and `app_hi` is `app_tb_9D`. `app_tb_map` gives each page's place in the
GAME for code that forms an offset from a pointer (`g_tb_find`). With no pet
save the GAME leaves through `rt_power` before drawing anything.

## Debug ports

Three SFRs no VMU uses tell the harness what the runtime is doing. Real hardware
ignores the writes.

| SFR | Name | Write means |
|---|---|---|
| `0x16D` | `RT_IDLE_MARK` | an `rt_idle` call began; the harness numbers input events by these |
| `0x16E` | `RT_LOG` | a byte for the harness's `--log` (the test GAME's event log) |
| `0x16F` | `RT_PRESENT_PORT` | a frame is presented; the harness dumps state and moves its script on |

## The harness

`pocketstation/harness/vmu_harness.c` runs a GAME on the vendored LC86K core
(`cpu.c`, BSD-3, from Dmitry Grinberg's VMU emulator) with a 128 KB flash image.
It runs no BIOS code. It models the ROM entries above from the US BIOS
disassembly: `0x0100`, `0x0110`, `0x0120`, `0x0130`, `0xE024` and the exit at
`0x01F0`. After each flash routine it leaves `0xA5` in every register the
BIOS routine destroys, so code that holds a value across the call fails as it
would on the VMU; TIMER_EX keeps every register. It stops the run on any other
ROM entry, or on a page write's return to an address it does not know. It also
models the BIOS's GAME launch: the block count at bank 0 `0x6D`, OCR `0xA3`,
SP `0x7F` and RAM bank 1.

On the idle schedule the harness jumps a halted CPU to the next timer event. On
the real schedule it runs every cycle. With `--report --map GAME.map` it
prints, for the frames between two presents:

```
frames 299 period 30766
cycles/frame mean 3262 p90 3265 max 3634 frame 132
runtime cycles/frame mean 2053 max 2424
ticks 286 late max 73 cycles
counts drift min -5 max 13
missed ticks 0 (overrun 0, late 0)
```

- The period is the tick period in cycles: the tick mode's reload plus one
  counts, times a count's cycles at the mode's clock, read from the runtime's
  state.
- The cycles are the ones the CPU executed, halts left out.
- The runtime's share counts cycles at addresses inside the runtime's labels
  (`rt_`, not `rt_test`). `pocketstation/tools/vmu_map.py` writes the link map
  those addresses come from, as Waterbear has no map output.
- A missed tick is either an overrun, a frame that executed for longer than its
  tick period, or a late tick, one `rt_idle` takes more than two Timer 0
  periods after the later of its due time and the start of that `rt_idle`
  call. A tick waiting for the game's next idle is the host's semantics, not
  lateness. The first tick after a mode starts sets the due times, a period
  apart; time asleep and mode changes move them.
- The counts drift is how far the counts the runtime has stepped and holds
  stand from the counts Timer 0 has really run, at each `rt_ev_step`, relative
  to the first. `sync_overflow.scn` keeps it within a period with an overflow
  pending at an `rt_ev_sync`, and `pocketstation/tests/asm/late_tick.s` shows
  late ticks being counted.

These numbers are `frame_loop.scn`: frame mode, 300 frames, each idling once,
reading the buttons 20 times and presenting. The runtime takes about 7% of the
frame on average and 8% at most. No frame overruns and no tick is late.
`frame_overrun.scn`, whose frames read the buttons 500 times, overruns in all 7
of its frames. The harness does not
count BIOS cycles. On
hardware, the BIOS clock path adds about 15-20 bytes of bank 0 stack on top of
an interrupt's frame, which the harness does not exercise either.
