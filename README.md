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

1.0.0 is out for Windows, macOS and Linux on the
[Releases page](https://github.com/telltaleatheist/revenge-re/releases). What is
known to differ from the original, and what no one has played yet, is in its
notes. Your own copy of the game is needed (above): the first start sets up a
copy it finds on your computer, or else downloads release 1.00, and lets you
pick one yourself if neither works.

- Builds on Windows (MSVC), macOS 12 or later (one universal build for Intel
  and Apple silicon) and Linux (GCC).
- The game's own words, pictures and sounds come from the player's copy: the
  first start imports them into a story folder of the player's own, each piece
  checked by offset, length and keyed hash, and play never opens the game's
  program again (`trace.exe_free`). `tools/text_audit.py` checks that this tree
  holds none of them.
- The rules, the screen layout and a new campaign's starting state are ours, in
  `data/rules`, `data/ui` and `data/start`, each table checked against the
  player's copy (`rules.exe`).
- Music plays from a General MIDI soundfont, each MT-32 sound the songs choose
  mapped to its nearest instrument (`data/music`); `music = speaker` and `off`
  are there too. Effects and speech play as recorded samples, or as the
  speaker's tones with the speech kept clean (`sound = speaker`).
- Runs can be recorded and replayed exactly, and compared with the original
  running under DOSBox-X ([tools/trace](tools/trace/README.md)). Thirteen
  scripts, from the title through the first two battles, a save and its load,
  and a save made by the original itself, match the original's state.
- Those comparisons, and building with more compilers and the address
  sanitizer, found bugs in the reconstruction, among them line of sight on
  every map. They are fixed here and reported upstream
  ([issue #1](https://github.com/Elthial/Crescent-Hawks-Revenge-sdl-port/issues/1)).
- The setup and the packages for the BattleTech launcher are built
  ([packages](#packages-and-the-setup)); 1.0.0 is the first release.
- Not yet: the rest of the campaign compared, and the old hardware layers
  replaced (graphics, input and timing).

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

The music's soundfont is cut from FluidR3 GM (MIT) at build time, and a
build never downloads: fetch its pinned source once per machine with
`tools/deps/fetch_soundfont.sh` (on Windows, `python tools\soundfont\subset.py
fetch`), or set `REVENGE_SOUNDFONT_SOURCE` to a copy of
`fluid-soundfont_3.1.orig.tar.gz`. `tools/music/listening.py` renders every
song through the game's own sequencer and Roland driver (the build's
`revenge_jukebox`) to WAV files, with a sheet for judging
`data/music/instruments.json`; nothing is played aloud.

Run the asset-independent build and tests:

```powershell
./Build.ps1 -Headless -Configuration Release
```

On macOS (12 or later; the build is universal) and Linux, with CMake, Ninja
and a C++20 compiler:

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

## Playing

The game plays as the original did. On top of it:

- **UPGRADES.** The game options menu (GAME OPTIONS on the title, or the
  options in a battle) has an UPGRADES line of ours. Its page turns on fixes
  for the original's bugs, one switch each, all off to start with: lance
  pursuit (a lance that can't walk goes after the nearest enemy), restart
  pilots (a Restart deals new pilots as a new campaign does), lance weapons
  and lance tactics (your lance's computer-run 'Mechs fire and judge as they
  were meant to; enemies stay as the original had them). Each switch says what
  it does on the page. They are kept in `settings.txt`;
  [docs/ORIGINAL_BUGS.md](docs/ORIGINAL_BUGS.md) has the bugs behind them.
- **The setup menu.** Shift + Alt + F1 (Cmd + Shift + F1 on a Mac), or the
  UPGRADES page's Setup menu line, opens it over the game, which waits: the
  game files in use, updates, reporting a problem, checking the files,
  resetting the settings, and opening your folder.
- **The log.** Started from the launcher or a package, the game writes
  `revenge.log` in your folder (the start before's is `revenge.old.log`). It is
  the file to send with a problem report.

## Packages and the setup

`tools/build_package.py` builds the packages the BattleTech launcher installs:
`CHR-Rebuild-<tag>-win64.zip` (and an installer, `-Setup.exe`),
`CHR-Rebuild-<tag>-mac.dmg` (universal, macOS 12 or later),
`CHR-Rebuild-<tag>-x86_64.AppImage` and `CHR-Rebuild-<tag>-linux-x86_64.tar.gz`,
released as `chr-vX.Y.Z` with one `SHA256SUMS` over them. Each holds the program, `chr`, our `data/` and the
notices, and none of the game; its smoke test starts the packaged program,
headless, and checks the setup and a first run.
[docs/RELEASING.md](docs/RELEASING.md) has the recipe.

A package's first start without a game runs the setup (re-setup-kit's) in a
small window: a copy found on the computer is checked and set up at once (a
problem with it is shown first); with none found, release 1.00 is downloaded
from the launcher's signed list at once; a failure, a cancel or no download
opens the setup window, where the player picks their copy (its `REVENGE.EXE`,
any file in its folder, or the archive it came in). The copy goes into their
own folder and is played from there. `chr --status` and `chr --setup` answer the launcher (the kit's
contract, game id `chr`); `chr --help` lists the start options.

While the game runs, Shift + Alt + F1 (Cmd + Shift + F1 on a Mac), or the
UPGRADES page's Setup menu line, opens the kit's setup menu over it
(`src/SDL/kit_menu.cpp`): the game files in use (and choosing others, which
restarts the game into its setup), updates, reporting a problem, checking the
program's and the game's files, resetting the settings, and the player's
folder. The game stands still while it is open, its clock paused, so nothing
catches up when it closes. A package carries the record of its own files
(`rebuild-files.sha256`, written before the package is sealed; in a Mac app,
in `Contents/Resources`) for the file check. A start with no terminal (a
package's, the launcher's) writes its log to `revenge.log` in the player's
folder, the start before's kept as `revenge.old.log`. Tests and trace runs
set `RE_KIT_TEST=1`: no menu keys, no hint, nothing over the network.

A build tree runs from the game folder `REVENGE.CFG` beside the program
names:

```ini
asset_directory=.
```

A relative or absolute path may name a copy elsewhere, and
`REVENGE_ASSET_DIRECTORY` overrides it for launchers and test environments.
SDL3 is linked into the program, fixing each release to the SDL version it
was built with.

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
it with a fault. The rules, the screens' layout, a new campaign's starting
state and the speaker's notes are the program's own numbers, the original's
to begin with: `data/rules`, `data/ui`, `data/start` and `data/sound`, read
at start (`src/Rules/Rules.h`
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
