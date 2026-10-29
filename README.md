# Legend of Mana for Dreamcast

A port of the PlayStation game **Legend of Mana** (North American release, `SLUS-01013`) to the **Sega Dreamcast**, built from a byte-for-byte matching decompilation of the original game. It runs on real hardware through GDEMU or MODE and in emulators such as Flycast.

The repository holds no game data. Everything copyrighted (graphics, music, movies, text, scripts) is converted from **your own disc image**, either by the [release kit](#playing-it-the-release-kit) or when you [build from source](#building-from-source).

## What works

- The whole game from the Squaresoft logo onward: menus, the world map, every field scene, battles, the ranch, shops and Home.
- Graphics on the PowerVR from the game's own PS1 drawing commands, the PS1's 320x240 drawn at 640x480.
- Music and sound effects through the game's own AKAO driver on the AICA sound chip, with the PS1's reverb.
- The FMV movies, decoded in software with their audio.
- Saves on the VMU, each PS1 save file kept as one compressed VMU file.
- **Ring Ring Land on the VMU.** The minigame the PS1 game sends to a PocketStation was decompiled and rewritten for the VMU. Sending a pet from the ranch installs it on the VMU, where you play it on its own.
- Dreamcast button art in the game's prompts and help screens, a second player on controller B, Jump Pack vibration, a soft reset (A+B+X+Y+Start) and 50/60 Hz on PAL consoles.

See [`dreamcast/README.md`](dreamcast/README.md) for the controls and the details.

## Playing it: the release kit

Each [release](../../releases) has a `lom-dc-kit` zip for Windows x64, Linux x64 and macOS (Apple silicon). It turns your disc image into the Dreamcast disc without Docker, Python or a compiler.

1. Dump your **Legend of Mana (USA)** disc as a `.cue` and `.bin`. The kit checks the `.bin` against SHA-1 `c1b536c99f0d390584eb30462a7e37f2bbef3902`, the redump dump, and names that SHA-1 if yours differs.
2. Unzip the kit and keep its `kit` folder next to the program.
3. Run it with your disc:
   - **Windows:** drag the `.cue` onto `lom-dc-kit.exe`, or run `lom-dc-kit.exe "Legend of Mana (USA).cue"`.
   - **Linux:** `./lom-dc-kit "Legend of Mana (USA).cue"`
   - **macOS:** `xattr -d com.apple.quarantine lom-dc-kit`, then `./lom-dc-kit "Legend of Mana (USA).cue"`
4. After a few minutes it writes a `Legend of Mana (Dreamcast)` folder beside your disc holding `disc.gdi` and three tracks, byte for byte the release's own build. Copy that folder to GDEMU or MODE, or open `disc.gdi` in an emulator.

About 2 GB of free space is needed while it runs. `lom-dc-kit --help` lists the options (`-o` for another output folder, `--overwrite` to replace an earlier GDI). The kit makes the GDI only; build from source for a CDI to burn.

## Building from source

You need Docker and the North American disc image at `disc/us/LegendOfMana.bin`. On Windows, after building the Docker images once, `dreamcast\compile.bat` builds the GDI folder `dreamcast/gdi/` and the CD-R image `dreamcast/lom.cdi`. [`dreamcast/README.md`](dreamcast/README.md) covers the toolchain images, every build target, the tests, the release workflow and how the port works.

The PS1 matching build of the decompilation still works here and keeps the decompiled C honest; its guide is [docs/en/decompilation.md](docs/en/decompilation.md) ([日本語](docs/jp/decompilation.md)).

## Repository layout

| Path | What it holds |
|---|---|
| `src/`, `include/`, `asm/`, `config/`, `mk/` | The matching decompilation of the PS1 game, shared with the port. |
| `dreamcast/` | The Dreamcast program: its makefile, sources (`dreamcast/src/`), tests and tools. |
| `port/` | The layer between the game and the Dreamcast: the PS1 SDK reimplemented (`port/psyq/`, `port/sound/`, `port/input/`, ...), the disc converter (`port/tools/`) and the release kit (`port/kit/`). |
| `pocketstation/` | Ring Ring Land: the PocketStation reference runner, the decompiled C, and the VMU game in LC86K assembly. |
| `dockerfiles/` | The toolchain images: `lom-dc` (KallistiOS), `lom-pocket` (PocketStation and VMU) and `lom-flycast` (headless emulator for tests). |
| `docs/` | Architecture guides and reference for the game and the port. |
| `disc/` | Where your own disc image goes. Ignored by git. |
| `tools/` | Decompilation tools: splat extensions, the compressor, objdiff configuration. |

## Relationship to the decompilation

The decompilation itself is [celophi/lom-decomp](https://github.com/celophi/lom-decomp). This repository is a standalone Dreamcast project built on top of it and is not set up to merge back. Decompilation improvements are brought forward from upstream from time to time; the port's changes to shared files stay small and keep the PS1 build matching.

## Reverse-engineering provenance

This project was created independently by analyzing the publicly released retail version of Legend of Mana and reconstructing its behavior and machine code through disassembly, decompilation, binary comparison, runtime analysis, and publicly available technical documentation and tools.

No leaked or otherwise non-public *Legend of Mana* source code, debug symbol files, internal symbol maps, developer documentation, or other confidential materials from Square or Square Enix have been used in the creation of this project, and this repository will **never** include them. Names, data structures and file boundaries are reconstructed from the retail binaries and observed behavior, and should not be taken as the original developers' own.

## Legal

See [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES.md) for licensing details.

The MIT License applies to original material authored for this project to the extent that the contributors hold the rights necessary to license that material. It does not grant rights to preexisting copyrighted material originating from the original game or from third-party SDKs, libraries, or development software.

This is an independent reverse-engineering and preservation project. It is not affiliated with or endorsed by Square, Square Enix, Sony, Sega, or any other rights holder. No game executable, overlay binaries, artwork, audio or other copyrighted game data is committed to this repository or included in its releases; you supply them from your own legally obtained copy of the game.

*Legend of Mana* and related names and assets are the property of their respective owners.

## Thanks

Thanks to Squaresoft and everyone who worked on *Legend of Mana*, and to celophi and the contributors of the [Legend of Mana decompilation](https://github.com/celophi/lom-decomp) this port stands on.

This project also builds on tools and research from the decompilation and homebrew communities, including:

- [KallistiOS](https://github.com/KallistiOS/KallistiOS) and [mkdcdisc](https://gitlab.com/simulant/mkdcdisc)
- [Flycast](https://github.com/flyinghead/flycast)
- [Waterbear](https://crates.io/crates/waterbear)
- [pcsx-redux's PocketStation core and OpenPSK](https://github.com/pcsx-redux/pocketstation), and Dmitry Grinberg's VMU emulator core
- [PyInstaller](https://pyinstaller.org)
- [splat](https://github.com/ethteck/splat), [spimdisasm](https://github.com/Decompollaborate/spimdisasm), [maspsx](https://github.com/mkst/maspsx), [old-gcc](https://github.com/decompals/old-gcc), [objdiff](https://github.com/encounter/objdiff), [decomp-permuter](https://github.com/simonlindholm/decomp-permuter), [m2c](https://github.com/matt-kempster/m2c), [wibo](https://github.com/decompals/wibo), [psyq-obj-parser](https://github.com/mkst/psyq-obj-parser), [decomp.me](https://decomp.me) and [decomp.dev](https://decomp.dev)
