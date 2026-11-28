# Changelog

All notable preservation releases are documented here. This project uses
semantic release numbers for its own reconstructed source and host layer; they
do not identify an original DOS release.

## 1.0.1 - 2026-11-28

- re-setup-kit d094522. Updating a Windows zip copy from the setup menu works
  when the game was started from its own folder: the updater's helper leaves
  the folder before moving it and starts the game in it again. 1.0.0's zip
  copies take 1.0.1 by hand once.
- The start-up update find is a notice in the corner ("UPDATE AVAILABLE
  <version> · press <keys>", `update_notice_seconds` in
  `setup-kit-settings.json`, 15 by default), not a prompt over the game.
- The log follows an update: the download and its source, the signed sums
  checked, the swap, and the next start's finished A to B.
- A folder copy at a drive's top says up front that it can't update itself.

## 1.0.0 packages (chr-v1.0.0) - 2026-11-27

- The setup, from re-setup-kit: a copy of the game is known by REVENGE.EXE
  (the US DOS release 1.00) and its files by size and SHA-256, and is copied
  into the player's own folder from a folder or the archive it came in.
- The BattleTech launcher's contract: `--status`, `--setup` and its served
  asks, and the start options (`--windowed`, `--window`, `--nosound`,
  `--hidden`, `--write-dir`, `--ready-file`).
- Packages for Windows (zip and installer), macOS (universal dmg) and Linux
  (AppImage and tarball) from `tools/build_package.py`, with `SHA256SUMS`; the program is
  named `chr` in them. Replaces the minimal Windows runtime package
  (`Build.ps1 -Package`).

## 1.0.0 - 2026-10-02

- Established the first standalone preservation snapshot.
- Added the C17 reconstruction, self-contained evidence and verification tests.
- Added the SDL3 replacement for DOS, BIOS and hardware boundaries.
- Added a statically linked Windows runtime package that requires user-supplied
  original game data.
- Declared Windows x64 as the only validated v1.0 host platform.
