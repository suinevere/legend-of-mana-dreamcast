# Legend of Mana — Dreamcast port

How the port is built, tested, released and put together. For what the project is and how players use the release kit, see the [project README](../README.md).

- [How the port works](#how-the-port-works)
- [Setting up](#setting-up)
- [Building](#building)
- [Running](#running)
- [Controls](#controls)
- [Testing](#testing)
- [Releases and the release kit](#releases-and-the-release-kit)
- [Debugging](#debugging)
- [Source layout](#source-layout)

## How the port works

The decompiled C of the PS1 game (`src/`, `include/`) compiles for the Dreamcast's SH-4 with KallistiOS. It runs almost unmodified: the PlayStation SDK calls it makes are reimplemented underneath it.

- **Graphics.** The game's GPU command lists are decoded and drawn on the PowerVR, with a mirror of PS1 VRAM, a texture cache and the PS1's blend modes (`dreamcast/src/video/`). The PS1's 320x240 is drawn at 640x480.
- **GTE.** The geometry coprocessor is emulated in software (`port/psyq/gte_soft.c`), with its tables taken from your disc.
- **Sound.** The game's own AKAO music driver runs on the AICA, the SPU's voices, ADPCM samples, noise and long streamed voices are reproduced on top, and the PS1's Studio Large reverb runs as an AICA DSP program (`port/sound/`, `dreamcast/src/sound/`).
- **Movies.** STR movies are decoded in software (MDEC and VLC) on a second thread, with their XA audio streamed to the AICA (`dreamcast/src/movie/`).
- **Disc.** The game's CD-ROM requests are served from a converted copy of the disc data in the GD-ROM's ISO 9660 filesystem, through our own mount and PIO reads (`dreamcast/src/cd/`).
- **Overlays.** Each of the PS1 game's 17 overlays is linked against the main program and loaded from the disc into a fixed region, as on the PS1 (`dreamcast/src/boot/`, `port/tools/overlay_link.py`).
- **Memory cards.** Each PS1 card file is kept as one compressed VMU file (`dreamcast/src/card/`).
- **Pads.** Dreamcast controllers are mapped onto the PS1 pad, with a second player, Jump Pack vibration and a soft reset (`dreamcast/src/input/`).

### The disc converter

Nothing from the game is committed. Before compiling, `pre.makefile` runs the converter in `port/tools/` on your PS1 image:

- `convert_disc.py` writes `dreamcast/cd/data/`: every resource and overlay data file, re-encoded where the Dreamcast needs it (audio banks to AICA ADPCM, Dreamcast button art in place of the PS1's, the English PocketStation lines, the ranch's Ring Ring Land option), plus `RESINDEX.BIN`.
- `libgte_tables.py`, `akao_data.py`, `main_data.py` and `vmu_portraits.py` write `dreamcast/gen/`: the GTE tables, AKAO's data block, storage for main-executable symbols the C does not define, and the pets' VMU portraits. These are linked into `lom.elf`.
- `overlay_link.py` and `gen_pending.py` write each overlay's linker script and the stubs for anything not yet ported.

`make` reruns the converter only when your image or a converter script changes.

### Ring Ring Land on the VMU

Ring Ring Land is the board-race minigame Legend of Mana sends to a PocketStation. The Dreamcast has none, so the minigame was ported to the VMU:

- **Decompiled.** Every function of the PocketStation program (`RES1506.BIN`, ARM code) was turned into C and checked call by call against the original on an emulated PocketStation (`pocketstation/`).
- **Rewritten for the VMU.** That C was reshaped into a VMU-sized program, then rewritten by hand in LC86K assembly for the VMU's 8-bit CPU, on a small runtime of its own (`pocketstation/src/vmu_game/`, `pocketstation/src/vmu_rt/`). The game matches the C frame for frame, pet save included, on seven scripted play sessions.
- **Installed by the game.** Sending a pet to Ring Ring Land from the ranch installs the minigame on that VMU as its GAME file. The game asks first if another VMU minigame is there, and refuses with a message if the VMU has no room. Unplug the VMU and start Ring Ring Land from its menu, then bring the pet home from the ranch.

The GAME takes 121 of the VMU's 200 blocks, starting at block 0, because the VMU runs a game straight from flash; the pet save takes 5 more. See [VMU game](../docs/en/technical/reference/vmu-game.md) and [VMU runtime](../docs/en/technical/reference/vmu-runtime.md).

## Setting up

You need **Docker** (Docker Desktop on Windows and macOS), **Python 3** for the tests and tools, and the **North American disc image** as one `.bin` at `disc/us/LegendOfMana.bin` with SHA-1 `c1b536c99f0d390584eb30462a7e37f2bbef3902`. Set `LOM_PS1_IMAGE` to read the image from somewhere else.

Build the three toolchain images once, from the repository root:

```bash
docker build -t lom-dc -f dockerfiles/dreamcast.Dockerfile dockerfiles
docker build -t lom-pocket -f dockerfiles/pocketstation.dockerfile dockerfiles
docker build -t lom-flycast -f dockerfiles/flycast.Dockerfile dockerfiles
```

| Image | What it holds | Needed for |
|---|---|---|
| `lom-dc` | KallistiOS v2.1.1 and its SH-4 GCC, mkdcdisc, Python | every Dreamcast build (about an hour to build the first time) |
| `lom-pocket` | ARM GCC, the PocketStation reference runner's tools, the Waterbear LC86K assembler, Python with unicorn and capstone | the VMU GAME and the PocketStation tests |
| `lom-flycast` | Flycast v2.7 built from source, headless, with its GDB server | the Flycast tests and `tools/flycast_run.py` |

## Building

On Windows, run the batch files in `dreamcast/` from any folder:

| Command | Builds |
|---|---|
| `dreamcast\compile.bat` | `dreamcast/lom.elf`, the GD-ROM folder `dreamcast/gdi/` and the CD-R image `dreamcast/lom.cdi` |
| `dreamcast\compile-debug.bat` | the debug disc `dreamcast/gdi-debug/` and `lom_debug.cdi`, whose title menu also offers the hidden debug start |
| `dreamcast\compile-vmu.bat` | Ring Ring Land's VMU GAME alone, `pocketstation/build/vmu_game/rrl.vms` |
| `dreamcast\compile.bat test-image` | the scripted test discs: `lom_test.cdi` and one `lom_<script>_test.cdi` per pad script |
| `dreamcast\compile.bat pending` | regenerates the stubs for symbols the port does not define yet |
| `dreamcast\compile.bat clean` | removes the Dreamcast build output |

`compile.bat` runs the converter and `compile-vmu.bat` first, because the disc carries the VMU GAME. Each step is a `make` that skips work already up to date, so one command builds everything from a fresh clone. The GDI and CDI are built as a pair: if either is missing, both are rebuilt. The GDI is written to `gdi.tmp/` and only moved into place once mkdcdisc succeeds.

On Linux or macOS, run the same steps directly from the repository root (`dreamcast/compile.sh [all|gdi|clean|pending]` wraps the last one):

```bash
docker run --rm -v "$PWD:/work" -w /work lom-dc bash -lc "make -C dreamcast gen/.pre_build.stamp"
docker run --rm -v "$PWD:/work" -w /work lom-pocket make -C pocketstation/src/vmu_game game
docker run --rm -v "$PWD:/work" -w /work lom-dc bash -lc "make -C dreamcast all"
```

Use `make -C dreamcast test-image` or `debug-gdi` the same way for the test and debug discs.

## Running

- **GDEMU or MODE:** copy the contents of `dreamcast/gdi/` into a new numbered folder on the SD card.
- **CD-R:** burn `dreamcast/lom.cdi`. The soft reset descrambles `1ST_READ.BIN` itself on a CD-R.
- **Flycast:** open `dreamcast/gdi/disc.gdi` or `dreamcast/lom.cdi`.
- **VMU:** saves and Ring Ring Land use the VMU in controller A's first slot.

Flycast is more forgiving than the hardware in a few ways (division by zero, PS1 addresses, AICA timing, GD-ROM drive timing), so check on a real Dreamcast before trusting a Flycast-only result.

## Controls

| Dreamcast | PS1 |
|---|---|
| A, B, X, Y | Cross, Circle, Square, Triangle |
| L, R triggers | L1, R1 |
| C, Z (or D-pad left/right while walking) | L2, R2 |
| D-pad down while walking | Select |
| Start | Start |

The mapping is in `src/input/pad_dc_map.c`. Holding **A, B, X, Y and Start** together soft-resets: the game reads `1ST_READ.BIN` back from the disc and starts again at the Squaresoft logo once the buttons are let go, waiting out a VMU save in progress (`src/system/soft_reset.c`).

A second controller on port B plays as the PS1's second player. A Jump Pack in either expansion slot of either controller vibrates when the game's Vibration option is on: the PS1's large motor sets its strength, and the small motor alone gives a light buzz (`src/input/rumble_dc.c`).

On a PAL console the game boots at 60 Hz; hold A on controller A while it boots for 50 Hz. A VGA cable always gets 60 Hz VGA (`src/video/dc_video_mode.c`, `src/system/dc_system.c`).

## Testing

All tests are pytest. Run them from the repository root; tests whose prerequisites are missing skip with the reason.

| Suite | Command | Needs | Time |
|---|---|---|---|
| Release kit | `python -m pytest port/tests/kit` | the disc and a built GDI for the disc tests | under a minute, plus about 7 minutes for `test_real_round_trip` |
| Converter and port tools | `python -m pytest port/tests` (includes the kit) | the disc for the converter tests | under a minute |
| Dreamcast host tests | `python -m pytest dreamcast/tests -k host` | a host `gcc` (MSYS2 or WSL on Windows); `test_libpress_host.py` also needs `ffmpeg` and the disc | a few minutes |
| All Dreamcast tests | `python -m pytest dreamcast/tests` | the above, plus the disc; the Flycast tests need `lom-flycast` and the discs from `compile.bat` and `compile.bat test-image`, and skip without the image | **45 minutes or more** with Flycast |
| One Flycast test | `python -m pytest dreamcast/tests/test_field.py` | as above | a few minutes each |
| PocketStation and VMU | `docker run --rm -v "$PWD:/work" -w /work lom-pocket python3 -m pytest pocketstation/tests` | `lom-pocket`, `pocketstation/compile.sh` once | several minutes |

What each Dreamcast group covers:

- **`*_host.py`** compile one piece of the port (the GPU decoder, PowerVR submitter, texture cache, MDEC, SPU shadow, VMU format, pad mapping, ...) with the host compiler against a C test in `tests/host/` and run it.
- **Flycast tests** (`test_boot_title.py`, `test_field.py`, `test_exit_to_map.py`, `test_render_screens.py`, `test_sound.py`, `test_movie.py`, `test_card_vmu.py`, `test_pet_transfer_flycast.py`, `test_soft_reset.py`, `test_video_pal.py` and `test_scene_sweep.py`) boot a scripted disc in `lom-flycast`. They drive the game with a pad script, then read its serial log, screens, sound and VMU image and compare them with PS1 references. They fail, rather than skip, when `lom_test.elf` is older than the sources (`tests/stale_image.py`).
- **Converter checks** (`test_button_art.py`, `test_ranch_patch.py`, `test_vmu_art.py`, `test_overlay_text.py`, `test_spu_sample_map_disc.py`, ...) check what the converter writes into `cd/data/`.

The pad scripts are in `tests/pad_scripts/*.pad`. `tools/gen_pad_script.py` compiles one into the test build; each extra test disc carries a `PADSEL.TXT` naming the script it runs. The full Flycast run is long, so run the tests for what you changed and the whole suite once per milestone.

## Releases and the release kit

Publishing a GitHub release runs [`.github/workflows/release-kit.yml`](../.github/workflows/release-kit.yml), which attaches a `lom-dc-kit` zip per platform. The kit rebuilds this exact GDI from the user's own disc, and holds no game data.

**How it works.**

1. CI downloads the PS1 `.bin` from the `LOM_URL` secret and builds the GDI with the steps above.
2. `port/kit/make_template.py` zeroes every byte of `track03.bin` that came from the disc and records each gap in `manifest.json`:
   - every converter file in the ISO;
   - the GTE tables, AKAO data, main data and VMU portraits inside `1ST_READ.BIN` (AKAO's relocated pointers stay);
   - the `RES1506.BIN` pages inside `RRL.VMS`.

   What is left is stored as compressed literals (`track03.lit`, a few MB) beside `track01.bin`, `track02.raw` and `disc.gdi`, which hold no game data.
3. `port/kit/leak_check.py` fails the run if any 256-byte chunk of the disc or of the converter output appears anywhere in the kit, at any alignment.
4. The workflow rebuilds the GDI from the kit and the disc with `port/kit/lom_dc_kit.py` and compares every file with the build.
5. `port/kit/freeze.py` freezes `lom_dc_kit.py` with PyInstaller on Windows, Linux and macOS runners, so users need no Python.
6. The disc and every build output holding its data are deleted from the runner.

On the user's machine, `lom-dc-kit` checks the disc's SHA-1, runs the same converter, writes each gap back, re-encodes every 2352-byte sector (EDC and ECC included) and checks each track's SHA-1 before the output folder appears.

**Setting up the repository.** Add `LOM_URL` as a repository **secret** holding a direct link to the PS1 `.bin`. The workflow refuses to run while `LOM_URL` exists as a repository variable, since the run log shows variables to anyone who can read the repository. The first run builds the KallistiOS image (about 1–2 hours) and pushes it to GHCR, keyed by the Dockerfile's hash; later runs pull it. Run the workflow by hand from the Actions tab with a tag to rebuild a release's kit.

**Running the steps by hand:**

```bash
python port/kit/make_template.py --gdi dreamcast/gdi --elf dreamcast/lom.elf --image disc/us/LegendOfMana.bin --out kit-data --tag v-local --sha local
python port/kit/leak_check.py --kit kit-data --image disc/us/LegendOfMana.bin
python port/kit/lom_dc_kit.py disc/us/LegendOfMana.bin --kit kit-data -o out-gdi
python -m pip install pyinstaller==6.11.1
python port/kit/freeze.py --tag v-local --platform windows-x64 --kit-data kit-data --out zips
```

The frozen program refuses a kit folder from another release (its tag is checked against `manifest.json`); the source `lom_dc_kit.py` reports `dev` and accepts any kit. The Linux binary is built on Ubuntu 22.04 and needs glibc 2.35 or newer.

## Debugging

**Serial log.** The port prints `LOM: ...` lines on the serial port, and a `LOM HALT:` line, also drawn at the top left of the screen, when it stops on an error. `tools/flycast_run.py` boots an image in `lom-flycast` and prints the serial log:

```bash
python dreamcast/tools/flycast_run.py [--seconds N] [--until TEXT] [image]
```

It exits 0 when `TEXT` appears (or after N seconds with no `TEXT`), 2 on a `LOM HALT:` line and 3 on a timeout waiting for `TEXT`. It makes a temporary job folder holding `config/flycast/emu.cfg` with `Debug.SerialConsoleEnabled = yes` and runs `docker run --rm --init ... lom-flycast xvfb-run -a flycast /disc/<image>`. Flycast on Linux writes each serial byte to its stdout, so the log arrives on the pipe about 3 seconds after launch. `--init` is required: with `xvfb-run` as PID 1 the container prints nothing. Your own Flycast settings are never read or written. The Windows Flycast build is fine for watching the screen, but its serial console stays empty.

**Debug disc.** `compile-debug.bat` adds the hidden debug start to the title menu.

**Tools** in `dreamcast/tools/`:

| Tool | Use |
|---|---|
| `flycast_run.py` | boot an image headless and capture the serial log |
| `scene_sweep.py` | enter every FIELD scene on Flycast in batches, resuming past any halt |
| `audio_capture.py`, `audio_compare.py`, `sweep_check.py` | capture Flycast's audio and compare songs with DuckStation dumps |
| `frame_diff.py` | diff a Flycast capture against a PS1 reference screenshot |
| `str_frames.py` | pull STR movie frames out of the PS1 image for the decoder tests |
| `vmu_image.py` | build and read Flycast VMU images for the card tests |
| `aica_dsp.py`, `aica_reverb.py` | generate and simulate the reverb DSP program |
| `gen_pad_script.py`, `gen_sound_sweep.py` | compile pad scripts and generate the sound sweep script |

## Source layout

| Path | What it holds |
|---|---|
| `dreamcast/src/boot/` | startup, overlay loading, state reporting |
| `dreamcast/src/video/` | GPU decoding, PowerVR submission, VRAM mirror, texture cache, video mode |
| `dreamcast/src/sound/` | AICA driver, streams, AKAO tick |
| `dreamcast/src/movie/` | software MDEC and STR decoding, movie audio |
| `dreamcast/src/cd/` | GD-ROM filesystem, CD-ROM requests, resource index |
| `dreamcast/src/card/` | VMU saves, VMU screens, the Ring Ring Land install |
| `dreamcast/src/input/` | pad mapping, pad scripts, Jump Pack |
| `dreamcast/src/system/` | soft reset, boot descrambling, exception and hang reports |
| `dreamcast/src/field/`, `wmap/`, `overlays/` | Dreamcast-side pieces of particular overlays |
| `dreamcast/makefile`, `pre.makefile`, `dreamcast.ld` | the build, the converter step and the main link |
| `dreamcast/gen/`, `cd/data/` | generated from your disc; ignored by git |
| `port/psyq/`, `port/sound/`, `port/input/`, `port/system/`, `port/boot/`, `port/stubs/` | the PS1 SDK reimplemented, shared by every target |
| `port/tools/` | the disc converter and link tools |
| `port/kit/` | the release kit |
