# Legend of Mana — Dreamcast port

## Build

`dreamcast\compile.bat` builds `dreamcast/lom.elf` and `dreamcast/lom.cdi` inside the `lom-dc` Docker image
(`docker build -t lom-dc -f dockerfiles/dreamcast.Dockerfile dockerfiles`).

## Run in Flycast (automated)

Build the headless emulator image once (Flycast v2.7 built from source, a few minutes; it also has the GDB server and `gdb-multiarch` for debugging):

```
docker build -t lom-flycast -f dockerfiles/flycast.Dockerfile dockerfiles
```

Then:

```
python dreamcast/tools/flycast_run.py [--seconds N] [--until TEXT] [image]
```

It prints every serial line and exits 0 when `TEXT` appears (or after N seconds with no `TEXT`), 2 on a
`LOM HALT:` line, 3 on a timeout while waiting for `TEXT`. `python -m pytest -q dreamcast/tests/test_boot_title.py`
boots to TITLE.

### Serial recipe

The runner makes a temporary job directory holding `config/flycast/emu.cfg` with

```
[config]
Debug.SerialConsoleEnabled = yes
```

and runs `docker run --rm --init -v <job>:/job -v <image dir>:/disc:ro -e XDG_CONFIG_HOME=/job/config
-e XDG_DATA_HOME=/job/data lom-flycast xvfb-run -a flycast /disc/<image>`. Flycast on Linux writes each SCIF byte
straight to its stdout, so KallistiOS `dbgio` output (banner, `LOM: ...` lines) arrives on the pipe, CRLF-terminated,
about 3 seconds after launch. `--init` is required: with `xvfb-run` as PID 1 the container prints nothing at all.
Your own Flycast settings are never read or written.

## Run in Flycast (by hand, Windows)

The Windows Flycast build (`tools/flycast/flycast.exe`, gitignored) is fine for looking at the screen: open
`dreamcast/lom.cdi`; the counting video backend draws nothing yet, so the screen stays black while the serial log
(see above) walks `LOM: state 8` into `LOM: state 2` (TITLE). Its serial console does not work for capture: with
`Debug.SerialConsoleEnabled` it opens a separate "Flycast Serial Output" console that stays empty.

## Real hardware (GDEMU)

Copy `dreamcast/lom.cdi` to the SD card as `NN/disc.cdi` in a new numbered folder (e.g. `05/disc.cdi`), boot the
Dreamcast, pick the disc in the GDEMU menu; the screen stays black unless a `LOM HALT:` message is printed at the
top left.

## Controls

A, B, X and Y are Cross, Circle, Square and Triangle; the triggers are L1 and R1; C and Z, or the D-pad's left and
right while walking, are L2 and R2, and the D-pad's down while walking is Select (`src/input/pad_dc_map.c`).
Holding A, B, X, Y and Start together soft-resets: the game reads `1ST_READ.BIN` back from the disc (descrambling it
on a CD-R) and starts again at the Squaresoft logo once the buttons are let go (`src/system/soft_reset.c`). It waits
out a VMU save in progress.

A second controller on port B plays as the PS1's second player. A Jump Pack in either expansion slot of either
controller vibrates when the game's Vibration option is on: the PS1's large motor sets its strength, and the small
motor alone gives a light buzz (`src/input/rumble_dc.c`).
