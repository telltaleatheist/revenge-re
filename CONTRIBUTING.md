# Contributing

This is a preservation reconstruction, not a redesign. Contributions should
make the original program more legible, verifiable or portable without
silently replacing its behavior.

## Preservation rules

1. Assembly and observed behavior from the supported executable are
   authoritative. Annotations and reconstructed C must be corrected when they
   disagree with that evidence.
2. Keep original procedures, data flow and BattleTech concepts recognizable.
   Do not invent replacement architecture or merge procedures for convenience.
3. Preserve original quirks and bugs by default. A compatibility fix needs a
   documented reason, focused test and explicit host/preservation boundary.
4. Preserve DOS-facing wrappers. SDL and operating-system behavior belongs in
   `src/SDL`; representation-only bridges belong in `src/Support`. Original
   game logic must not call SDL directly.
5. Do not add original executables, assets, extracted media, saves, screenshots
   or the game's text. The game's own data comes from the player's
   `REVENGE.EXE` at start (`src/Support/original_import.c`), and its text from
   the story file imported from that copy into the player's folder
   (`src/Import`, `include/revenge/story.h`): give a string a `text` line, or
   a table its line, in `src/Support/original_import_pieces.txt`, name a single
   string `STORY_TEXT(ID)`, then run `tools/import/make_import_table.py`.
   `tools/text_audit.py` must find nothing. The copy is accepted only as an edition and variant
   `src/Support/original_editions.txt` lists; for a copy it doesn't, the tool
   prints the line to add.
6. Cite the procedure's segment:offset and update `docs/METHOD_TABLE.csv` and
   `docs/SOURCE_CONVERSION_TABLE.csv` when verification or source ownership
   changes.

The detailed original-bug policy is in [`docs/ORIGINAL_BUGS.md`](docs/ORIGINAL_BUGS.md).

## Source style

The reconstructed C intentionally follows readable 1990-era C conventions.
Follow the existing file locally. Do not run a repository-wide formatter or
include drive-by whitespace changes. Prefer established C terminology and
BattleTech vocabulary over raw addresses, while retaining evidence references
where they aid comparison.

## Validation

Run before opening a pull request:

```sh
python3 tools/text_audit.py
python3 tools/import/make_import_table.py --check
python3 tools/check_ledgers.py
./Build.ps1 -Headless -Configuration Release
./Build.ps1 -Configuration Release -Package
```

Use small, focused commits. Explain original behavior, evidence used, and
whether a changed boundary is original logic or an SDL/host substitution.
Contributed project-authored material is accepted under the scoped MIT grant in
`LICENSE`; that grant does not extend to reconstructed or original-rights data.
