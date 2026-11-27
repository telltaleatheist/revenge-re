# Building a package

Nothing publishes from this repository. There is no release workflow, and a
release is made only when the project owner has cleared it. The Windows
workflow builds and tests the headless configuration; packages are built by
hand on each platform, as below, and a person publishes them.

## What a package holds

`tools/build_package.py` builds the program in `build-package/` and stages:

- the program, `chr` (`chr.exe`; `Contents/MacOS/chr` in the Mac app;
  `usr/bin/chr` in the AppImage). The build's target stays `Revenge`, which
  the tools and the trace harness run; the package names it. SDL 3 (the
  pinned release CMake fetches) and the archive libraries are linked in, so
  the Mac and Linux programs load only system libraries, and the Windows one
  only those and the MSVC runtime beside it;
- `data/`: our rules, layout, starting state, the speaker's notes and the
  music's instrument table, beside the program (SDL's base path; a Mac app's
  `Contents/Resources`). Packaging refuses a tree without them;
- `data/music/FluidR3_GM-subset.sf2`, the music soundfont the build cuts from
  FluidR3 GM, refused unless it hashes as `data/music/soundfont_subset.sha256`
  pins;
- `data/setup/badge.png`, the setup screen's badge (`tools/package/revenge.png`);
- `LICENSE`, a `README.txt` and `licenses/`: the project's, `NOTICE.md`,
  `THIRD_PARTY_NOTICES.md`, SDL's and hidapi's, re-setup-kit's and its own
  third-party code's (stb, miniz, Monocypher), TinySoundFont's, FluidR3's
  `COPYING` and `README`, and libarchive's, zlib's and xz's. Packaging
  refuses a package missing any of them.

No `REVENGE.CFG` is packaged (a build tree's points at a game folder), and no
file of the original game, save or imported story is. The first start reads
the player's copy, imports the story into `story/` in the player's folder,
and plays from there and the game's resource files.

## Starting without a game folder

An installed copy is started with no arguments (a shortcut, a double-click).
It plays from `REVENGE_ASSET_DIRECTORY` or a `REVENGE.CFG` beside the program
when either is set, else from the `game_folder` line of `setup.txt` in the
player's folder while that folder is still the game, else from the player's
own copy (`install/` in that folder). With none, re-setup-kit's setup screen
opens first (`src/SDL/setup_host.cpp`, `src/Setup`). The player picks their
copy of the game (its `REVENGE.EXE`, any file in its folder, or the archive
it came in), and it is copied into `install/`, or into a folder of the
game's own with CHANGE FOLDER. The original is only read. A copy is known by
`REVENGE.EXE`'s size and SHA-256 (the US DOS release 1.00,
`docs/SUPPORTED_VERSIONS.md`), its files by `src/Setup/GameFilesTable.cpp`
(names, sizes and hashes only, written by `tools/setup/make_game_files.py`).

The player's folder is `REVENGE_SAVE_DIRECTORY`, or SDL's per-user folder:
`%APPDATA%\BattleTech\Revenge Rebuild` on Windows,
`~/Library/Application Support/BattleTech/Revenge Rebuild` on macOS and
`~/.local/share/BattleTech/Revenge Rebuild` on Linux (the family's folder,
re-setup-kit's `rekit::kFamilyOrg`). Nothing is written into the install
folder.

## The launcher's contract

The game id and folder key are `chr`.

- `chr --status` prints one JSON line (`"kit": 1`, game `chr`; files `ready`,
  `missing` or `incomplete`; `story import` pending until done; `"setup": 1`
  and `"play": 1`), exits 0 and writes nothing.
- `chr --setup` sets the game up on the setup screen and exits 0 ready,
  1 cancelled or 2 failed.
- `chr --setup` with `--describe`, `--search`, `--copies`, `--look <pick>` or
  `--install <pick> [--folder <dir>]` is the setup for the launcher's own
  screens (the kit's served setup): no window, JSON lines on stdout, the last
  with `done`. `--add` and `--first-run` answer that they are not a setup
  this game answers. What a look or an install logs goes to `setup.log` in
  the player's folder.
- The play options: `--windowed`, `--window WxH`, `--nosound`, `--hidden`,
  `--write-dir DIRECTORY` and `--ready-file FILE`.

`tools/package/served_setup.py` asks a built or packaged program through
pipes, as the launcher does. The `setup.status` test and every package's
smoke test run it; without the game, the parts that need a copy are skipped
and named.

A program built with a release's tag (`CHR_RELEASE_TAG`, which packaging sets
from the label) holds `MW3-PROGRAM-VERSION:<tag>` in its file, the string the
launcher reads, and each start with a player at it writes the kit's program
record into the player's folder. Any other build holds no such string and
writes no record; packaging refuses a program whose stamp is not its label's.

## Dependencies

- The archive libraries: libarchive 3.7.9 with zlib 1.3.1 and liblzma
  (xz 5.8.1), static, built from pinned sources by
  `tools/deps/build_archive.sh mac|linux` (into `build-deps/<platform>/prefix`)
  and `tools\deps\build_archive.cmd` (into `build-deps\archive`, clang-cl).
  Without them the setup reads folders only, and packaging refuses the build.
- The soundfont's source, fetched once per machine:
  `tools/deps/fetch_soundfont.sh` (Windows: `python tools\soundfont\subset.py
  fetch`), or `REVENGE_SOUNDFONT_SOURCE` names a copy. The build never
  downloads it.
- Linux: the AppImage tools, `tools/deps/get_appimage_tools.sh DIR`, each
  refused unless it has the hash pinned there.
- Windows: Visual Studio's C++ tools, and Inno Setup for `--installer`
  (`iscc` on the PATH, scoop's `inno-setup`, or Program Files).
- macOS: the Xcode command-line tools.

## Names

A release is tagged `chr-vX.Y.Z`, whose X.Y.Z is `VERSION`'s. The label below
is that tag, or any other name for a test build (put `test` in it, so it is
never taken for a release). Packaging with a release's label is refused
unless the tree is a git checkout at that tag with nothing changed, added or
missing (ignored files don't count). These names are in the launcher's
catalog; they change only with the launcher's.

| Platform | Made on | Files |
|---|---|---|
| Windows x64 | Windows, MSVC | `CHR-Rebuild-<label>-win64.zip`, and with `--installer` `CHR-Rebuild-<label>-Setup.exe` |
| macOS | a Mac | `CHR-Rebuild-<label>-mac.dmg` (`Revenge Rebuild.app`; a release's signed with a Developer ID and notarized, a test build's ad hoc), universal (x86_64 and arm64), macOS 12 and later |
| Linux x86-64 | Linux or WSL | `CHR-Rebuild-<label>-x86_64.AppImage` |

Each file gets a `.sha256` beside it. A release's assets are the four files
and one `SHA256SUMS` over them all (`tools/package/sums.py`).

    python3 -I tools/build_package.py --label LABEL [--game GAME] [--installer] [--appimage-tools DIR]

- **Windows**: the program is linked as a GUI program (`CHR_WINDOWS_GUI`),
  so no console window opens, with its icon (`tools/package/chr.rc`) and a
  UTF-8 manifest. `--installer` compiles `tools/package/installer.iss`: Setup
  installs to `Program Files\Revenge Rebuild` (or "only for me"), with
  Start-menu and desktop shortcuts and an uninstaller that leaves the
  player's folder. Its AppId is this project's own and stays the same every
  version, so a new Setup upgrades in place.
- **macOS**: the program goes to `Contents/MacOS`, `data/`, the read-me and
  the notices to `Contents/Resources`. Signing is re-setup-kit's
  `tools/macos/sign_notarize.sh`:
  - `check` runs before anything is built, so a build that couldn't be
    notarized stops there;
  - `app` (`sign_app`, with `tools/package/chr.entitlements`: no exceptions
    to the hardened runtime) signs before the smoke test, so what is started
    is what ships;
  - `dmg` (`notarize_dmg`) signs the dmg, notarizes it and staples the ticket
    to the dmg and the app. The `.sha256` is written after that, since
    stapling changes the dmg.

  With a developer identity and a notary profile, the app is signed with a
  Developer ID, the hardened runtime and a timestamp, and the dmg is
  notarized. Who signs comes only from the environment, set on the command
  itself and never written down: `RE_SIGN_IDENTITY` (the keychain's
  Developer ID Application identity, by its SHA-1) and `RE_NOTARY_PROFILE` (a
  `notarytool` keychain profile). Without them the kit refuses, unless
  `RE_ALLOW_UNSIGNED=1` says the build is not for release; then it is signed
  ad hoc and the read-me says how to open it the first time. The dmg is made
  from a folder with `hdiutil create`; nothing is mounted.
- **Linux**: `--appimage-tools DIR` names the folder the tools script
  filled. The AppImage holds `usr/bin/chr` with `data/` beside it and the
  notices in `usr/share/doc/chr`, the AppImage runtime's
  (`tools/package/appimage-runtime`) among them. Packaging unpacks it with
  `--appimage-extract` and starts the program from the unpacked folder, as the
  launcher does.

Every package's contents pass `tools/package/check_paths.py`: no local path
or user name in any file (`CHR_TRIM_PATHS` names the sources relative to the
tree), and no file of the game, by name. The one allowed use of the name is
in a public GitHub address: `github.com/<user>`, `api.github.com/repos/<user>`
and `raw.githubusercontent.com/<user>`, where the setup's catalog has its
copy.

The packaging's own parts have unit tests that need no build
(`tools/test_build_package.py`, ctest `package.build_package`): the signing's
commands with a made-up identity, the background copy, and `check_paths`.

    python3 -I -m unittest discover -s tools -p test_build_package.py -v

## The smoke test

Each package is checked from its own layout (the extracted zip, the app, the
AppImage's `usr/bin` and its unpacked copy):

- `--help` prints the usage;
- the served setup answers as the launcher asks it (`served_setup.py`);
- with a game folder (`--game DIR`, or `REVENGE_GAME`), the first start from
  an empty player's folder imports the story and reaches the game's first
  input wait. It runs the trace harness (`tools/trace/README.md`) with the
  one-line script `@snap:first_wait`: the harness takes that snap at the
  first wait, writes `done.txt` and ends the run, and the test checks those
  and `story/import.json`. The game folder is only read and is never packaged.
- then a second start, the story imported, shows the title: the smoke input
  (`REVENGE_SMOKE_INPUT`, `docs/CAMPAIGN_SMOKE_TEST.md`) asks for `shot:NAME`
  once a second until a capture holds a picture (at least 320x200 and more
  than 16 colours), and the run is stopped, since the title waits for a key.
  `--shots DIR` keeps that capture as `title-<platform>-<n>.bmp`. It shows
  the player's game, so DIR is a scratch folder, never the repo.

Nothing is shown: the video and audio drivers are `dummy`, the player's folder
is a temporary one (never the real one), and on a Mac the runs start a
temporary copy of the app whose Info.plist says `LSBackgroundOnly`
(`tools/package/background_app.py`): a program inside an app comes up in the
foreground, whatever the hidden run's settings say. The app's own signature
is checked first, and the app is only read. The copy is signed ad hoc again
without the hardened runtime, which an ad hoc program can't keep. So the
app's own program, as signed, is started once first, with `--status`
(`check_own_signature`, under `arch` for one half): `--status` prints one
line and opens no window, so it is no foreground app.

To run the same checks on a package already made:

    python3 -I tools/build_package.py --smoke DIR --game "$GAME"

DIR is the folder holding `chr` or `chr.exe`, the app, or an unpacked
AppImage's `squashfs-root`. With an app, `--arch x86_64` runs the Intel half
alone, which Rosetta runs on an Apple silicon Mac.

## Making a release

By hand, when the project owner has cleared it, and not before. What fails a
step stops the release.

1. Choose the version and the commit. `VERSION` holds X.Y.Z, and
   `CHANGELOG.md` and the README say what is true then.
2. Tag that commit on each building machine, locally: `git tag chr-vX.Y.Z`.
   Nothing is pushed yet.
3. Build the packages from the tag, each in a clean checkout with its
   submodules (`git submodule update --init`), with the game given, so the
   smoke test does its first run:

       # macOS, RE_SIGN_IDENTITY and RE_NOTARY_PROFILE set on this command alone
       tools/deps/build_archive.sh mac
       python3 -I tools/build_package.py --label chr-vX.Y.Z --game GAME
       # Linux (or WSL)
       tools/deps/build_archive.sh linux
       tools/deps/get_appimage_tools.sh DIR
       python3 -I tools/build_package.py --label chr-vX.Y.Z --game GAME --appimage-tools DIR
       # Windows
       tools\deps\build_archive.cmd
       python -I tools\build_package.py --label chr-vX.Y.Z --game GAME --installer

   Each run must end with its smoke test passed, the first run and the served
   setup included, none skipped. The dmg must also pass
   `spctl -a -vv -t install` and `xcrun stapler validate`.
4. Put the four packages in one empty folder, check each against its
   `.sha256`, then write the one list:

       python3 -I tools/package/sums.py FOLDER

5. Check each package by hand on its system: SDL starting, the setup, the
   campaign, a battle and its return, input, music and sound. The scripted
   campaign smoke path (`REVENGE_SMOKE_INPUT`, `docs/CAMPAIGN_SMOKE_TEST.md`)
   runs on a packaged program too; give it `REVENGE_SAVE_DIRECTORY` and
   `REVENGE_ASSET_DIRECTORY`.
6. Push the tag: `git push origin chr-vX.Y.Z`.
7. Publish, with the release notes as the text:

       gh release create chr-vX.Y.Z --verify-tag --title "Revenge Rebuild X.Y.Z" \
         --notes-file NOTES <the four packages> SHA256SUMS

   Not a draft and not a prerelease: the launcher takes only a published, full
   release, and installs nothing without the package's line in `SHA256SUMS`.
8. Download every asset again from the release and check it against
   `SHA256SUMS`.
