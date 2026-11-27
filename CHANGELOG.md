# Changelog

All notable preservation releases are documented here. This project uses
semantic release numbers for its own reconstructed source and host layer; they
do not identify an original DOS release.

## Unreleased

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
