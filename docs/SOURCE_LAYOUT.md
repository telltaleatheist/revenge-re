# Source and evidence layout

This document describes the current tree. It does not define new runtime
layers; the preservation rules remain authoritative in
[`PRESERVATION_PLAN.md`](PRESERVATION_PLAN.md).

## Runtime source

```text
src/
    Original/   reconstructed executable procedures and data
    Support/    host representation bridges
    SDL/        SDL3 and operating-system boundary
```

`src/Original` is organized by subject for navigation:

```text
Audio/      Campaign/   Data/       Graphics/   Input/
IO/         Resources/  Runtime/    Tactical/   Text/
UI/         Units/      Utility/
```

These directories do not change the original call graph. A file is named for
what it holds (`Tactical/UnitDamage.cpp`); its first line names the original's
segments it holds and what it is (`// 2D1B: unit damage`). A large segment may
be split only between original procedure boundaries. Every procedure keeps its
`SEG:OFFSET` in its leading comment and in both ledgers.

A procedure or a variable that `src/Original` shares is declared once, in the
header beside the file that defines it: `Tactical/Tactical.h` declares what
the files of `src/Original/Tactical` define, and `Data/TacticalUnitData.h` what
`Data/TacticalUnitData.cpp` defines. A file includes the headers whose names it
uses, spelled with their folder (`#include "Tactical/Tactical.h"`).

A named constant sits beside its users: in the `.cpp` that alone uses it, in
the domain's internal header when the domain's files share it, in
`<Domain>/<Domain>.h` of the domain that owns it when other domains use it
too, and beside the variable whose values it names (`Data/<Name>.h`).
`include/revenge` keeps the ones `src/SDL` and `src/Support` share. Each is
defined once.

`tools/check_headers.py` (ctest `preservation.header_placement`) checks the
declarations, the includes and that each constant is defined once.

The files were first named for their segment (`REVENGE_2D1B_UNIT_DAMAGE`).
[`RENAMES.tsv`](RENAMES.tsv) lists each old name with its new one, and
`tools/rename_files.py` applies a row: the move, and every mention of the old
name in the tree (includes, CMake lists, the ledgers, tests and docs).

`src/Support` contains only facilities that have no original executable
procedure boundary, such as stored 16:16 address resolution, host callback
dispatch and indexed-framebuffer representation. It must not own gameplay
decisions.

`src/SDL` implements the host side of original DOS, BIOS, interrupt, input,
audio, video and filesystem wrappers. SDL headers and direct SDL calls are not
permitted in `src/Original`.

## Evidence and accounting

The decompiler output, assembly listings and project metadata the
reconstruction was checked against are not part of this tree; the ledgers
keep their paths as references.

```text
docs/
    METHOD_TABLE.csv
    SOURCE_CONVERSION_TABLE.csv
```

`METHOD_TABLE.csv` records behavioral verification against assembly.
`SOURCE_CONVERSION_TABLE.csv` independently records conversion to readable,
typed reconstructed C. A file move or readability change does not alter ASM
verification status.

`tools/Update-MethodTable.ps1` refreshed the mechanical inventory fields from
the evidence, which is kept outside this tree. Run `tools/check_ledgers.py` to
validate both ledgers. Manually audited
evidence and status fields must not be replaced by generated guesses.

## Other directories

- `include/` contains shared interfaces and packed data contracts.
- `tests/` contains focused behavioral and preservation tests.
- `tools/` contains validation, extraction and packaging utilities.
- `assets/original-game/` is an ignored local destination for legally obtained
  game data.
- `config/` contains the runtime configuration template used by packaging.
- `.github/` contains standalone repository CI.

The former `src/Rejected` tree was removed after its audit. Git history is the
recovery mechanism for that non-authoritative implementation.
