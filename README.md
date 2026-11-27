# Revenge Rebuild

**revenge-re** rebuilds BattleTech: The Crescent Hawks' Revenge (1990, the DOS
game by Westwood Associates, published by Infocom) for today's Windows, macOS
and Linux. It is built on
**[Elthial's Crescent Hawks' Revenge SDL port](https://github.com/Elthial/Crescent-Hawks-Revenge-sdl-port)**,
a readable C reconstruction of the original program, whose research and code it
continues. It is a fan project: unofficial, free, and not affiliated with or
endorsed by any holder of rights in the game.

**You need your own copy of the game.** None of the game's files, text, art or
sound are in this repository. The program reads what it needs from your DOS
copy when it starts, and never changes it. Only the US DOS release 1.00 is known
so far ([supported versions](docs/SUPPORTED_VERSIONS.md)).

## Status

In development. There is no release yet and nothing packaged: it is built and
run from source (below).

- Builds on Windows (MSVC), macOS 12 or later (one universal build for Intel
  and Apple silicon) and Linux (GCC).
- The game's text and data come from the player's copy: each piece is checked
  against offsets, lengths and keyed hashes before play
  (`src/Support/original_import*.c`, `tools/import`), and
  `tools/text_audit.py` checks that the tree holds none of it.
- Runs can be recorded and replayed exactly, and compared with the original
  running under DOSBox-X ([tools/trace](tools/trace/README.md)). Eleven scripts
  so far, from the title through the first two battles and a save and its load,
  match the original's state at every snapshot.
- Those comparisons, and building with more compilers, found bugs in the
  reconstruction, among them line of sight on every map. They are fixed here
  and reported upstream
  ([issue #1](https://github.com/Elthial/Crescent-Hawks-Revenge-sdl-port/issues/1)).
- Not yet: the rest of the campaign compared, music through a soundfont, the
  setup screens and packages, and the BattleTech launcher.

The tree started from Elthial's v1.0 (d75ee14). His evidence tree
(disassembly and annotations) and documentation images are not carried here;
his repository has them.

## Quick start

Requirements:

- Windows with Visual Studio C/C++ build tools;
- PowerShell;
- CMake and Ninja supplied by Visual Studio;
- SDL3 for the playable build; and
- a legally obtained copy of the original game data.

Run the asset-independent build and tests:

```powershell
./Build.ps1 -Headless -Configuration Release
```

On macOS (12 or later; the build is universal) and Linux, with CMake, Ninja
and a C17 compiler:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

SDL3 is fetched at the pinned release. On Linux it is built without XTest
(`SDL_X11_XTEST=OFF`, only synthetic input uses it), since many systems lack
its headers; `-DSDL_X11_XTEST=ON` turns it back on. SDL's X11, Wayland and
audio back ends need their usual development headers (the SDL wiki's
README-linux lists them). Set `REVENGE_GAME` to a copy of the game for the
tests that check its text and data; without one they are skipped.

For local play, put the original game files in `assets/original-game` and run:

```powershell
./Run-Revenge.ps1
```

Or use an installation elsewhere:

```powershell
./Run-Revenge.ps1 -OriginalAssetDirectory ../Chrevenge
```

The launcher builds `Revenge`, checks for `MEDIA.CFG`, prevents accidental
multiple instances and supplies the asset location without copying the game.
Use `-NoBuild` to run an existing executable and `-Trace` for host-boundary
diagnostics.

## Minimal runtime package

Create a distributable runtime shell with:

```powershell
./Build.ps1 -Configuration Release -Package
```

`build-release/package` contains exactly:

```text
Revenge.exe
REVENGE.CFG
```

The package deliberately contains no copyrighted game data.
`build-release/Revenge-1.0.0-SHA256SUMS.txt` records the SHA-256 of both package
files and is intentionally kept beside the package directory. `REVENGE.CFG`
locates the user's original installation:

```ini
asset_directory=.
```

The default permits the two files to be copied into an original installation.
SDL3 is statically linked into `Revenge.exe`, fixing each release to the SDL
version with which it was built and avoiding a separate runtime DLL.
A relative or absolute path may instead keep program and data separate.
`REVENGE_ASSET_DIRECTORY` overrides the configuration for launchers and test
environments.

The original installation is only read, so it may be on a read-only disk.
Whatever the game writes, its saves included, goes to a folder of our own:
SDL's per-user preferences folder, `BattleTech/Revenge Rebuild`, or
`REVENGE_SAVE_DIRECTORY` when it is set. A file the game changes is copied
there the first time and the copy is used from then on. The game's file
names are found whatever their case on disk.

The screen is drawn with SDL's default renderer for the platform. When it
can't start, the game falls back to SDL's software renderer, which needs no
graphics driver; `SDL_RENDER_DRIVER=software` chooses it from the start.

## Preservation model

This tree carries none of the original game's text, art or sound: the program
reads them from the player's own game files. The text, and the media and data
the program keeps with it (the sounds, the palette and the text's digram
tables; the setup's device records and two small tables), are taken from the
player's `REVENGE.EXE` once, into `story/` in the player's folder
(`text.json`, `media/<id>.bin`, `data/<id>.bin`, and `import.json` saying what
they were taken from, with each file's length and SHA-256), and read from
there; they are taken again when that folder is missing or from an older
version. That import is the only time the program reads the copy: play reads
the story folder and the game's other files, and asking for any `.EXE` stops
it with a fault. The rules, the screens' layout and a new campaign's starting
state are the program's own numbers, the original's to begin with:
`data/rules`, `data/ui` and `data/start`, read at start (`src/Rules/Rules.h`
has the files' shapes), and checked by the tests against
the original's bytes by keyed hash, never holding them. The
title artwork and screenshots of the upstream port (`docs/images`, which
`NOTICE.md` and `LICENSES/README.md` still mention) are left out.

| Area | Responsibility |
| --- | --- |
| `src/Original` | Reconstructed executable procedures and typed executable-owned data |
| `src/Support` | Host representation bridges with no claimed original procedure boundary |
| `src/SDL` | SDL3 and operating-system implementations below original DOS/hardware wrappers |
| `include` | Shared C interfaces and packed data contracts |
| `docs/METHOD_TABLE.csv` | Address-keyed behavioral verification ledger |
| `docs/SOURCE_CONVERSION_TABLE.csv` | Address-keyed readable-C conversion ledger |
| `tests` | Focused behavior, boundary and preservation checks |
| `tools` | Ledger, evidence and package validation utilities |

The governing rules are in
[`docs/PRESERVATION_PLAN.md`](docs/PRESERVATION_PLAN.md). The reconstructed
source model is described in
[`docs/RECONSTRUCTED_C_PLAN.md`](docs/RECONSTRUCTED_C_PLAN.md). The
[`documentation guide`](docs/README.md) distinguishes current policy from
historical audit records. Supported DOS executable hashes are recorded in
[`docs/SUPPORTED_VERSIONS.md`](docs/SUPPORTED_VERSIONS.md).

Contributions must follow [`CONTRIBUTING.md`](CONTRIBUTING.md), particularly the
original-bug and SDL-boundary rules. Release changes are in
[`CHANGELOG.md`](CHANGELOG.md).

## Verification and diagnostics

`tools/check_ledgers.py` (ctest `preservation.ledger_integrity`) checks paths, address/name uniqueness,
aliases, final verification states, addressless helpers and direct SDL access
from `src/Original`. Passing tests are regression evidence; the assembly and
observed original behavior remain authoritative.

Normal builds exclude diagnostics injected into reconstructed procedure bodies.
For an explicitly instrumented investigation:

```powershell
./Build.ps1 -Configuration Release -PreservationInstrumentation
./Run-Revenge.ps1 -NoBuild -Trace -OriginalAssetDirectory ../Chrevenge
```

Injected observations use `REVENGE_PRESERVATION_TRACE` so they remain visibly
separate from the reconstructed logic.

## Known limits

- The complete campaign has not been acceptance-tested.
- Presentation and audio are functional but are not cycle- or
  waveform-identical to DOS hardware.
- Stored 16:16 addresses and interrupt-era behavior still require host
  representation bridges where their numeric form is observable.
- Original assets are required for normal play and are not distributed here.

## Rights and licensing

Project-authored code and excluded reconstructed or third-party material are
described in [`LICENSE`](LICENSE), [`LICENSES/README.md`](LICENSES/README.md),
[`NOTICE.md`](NOTICE.md), and [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).
The original executable, raw game assets, saves and extracted asset dumps are
not included. The limited documentation images are covered by `NOTICE.md`.

This is an independent preservation and research project. It is not affiliated
with or endorsed by the game's publishers or the owners of BattleTech.
