# Coding style

Write readable, assembly-loyal C++20. Preserve the original procedure
boundaries and gameplay decisions rather than replacing them with a new engine.
These rules follow the MW1 rebuild's (mech1-re); the source model they serve is
in [`RECONSTRUCTED_C_PLAN.md`](RECONSTRUCTED_C_PLAN.md).

- Use descriptive function and variable names, and BattleTech terminology for
  established concepts: BattleMechs, MechWarriors, Gunnery, Piloting, armour,
  internal structure, heat, critical hits, C-bills and the Great Houses.
- Keep identifiers free of native-address suffixes. Keep short procedure
  provenance comments (`// 3390:0A22`) where they help compare the code with
  the assembly.
- Define shared state once, in its owning source file, and declare it in the
  relevant header under `include/revenge`; make private state `static`.
- Use ordinary typed structures, arrays and references for known records.
  Proven parallel arrays and explicit field-copy loops are valid recovered
  code; don't regroup them into records or add accessors only to shorten it.
- Keep binary encoding at real file and transfer boundaries. Preserve
  observable aliases, overlapping access and native numeric identities where
  the game depends on them.
- Preserve integer widths, signedness, wrap, truncation, random-number order,
  partial writes and call order. Don't rely on signed overflow or on the
  host's pointer size.
- Let declarations, assignments, returns and prototyped arguments do their
  normal conversions. Keep an explicit conversion only where an intermediate
  width or a signed reading changes a comparison, shift, division or
  multiplication.
- Preserve the game's behaviour, not invalid memory reads: missing targets,
  sentinels, exhausted pools and map edges get explicit typed handling.
- Name meaningful constants, flags and continuation destinations. Avoid magic
  numbers, redundant temporaries and invented pass-through helpers.
- Use readable loops, `++`, `--` and compound assignments where the native
  width is kept.
- An integer constant that meets other types is `inline constexpr int` in a
  header (`constexpr int` in a source), not an unnamed enumerator: C++ gives
  an enumerator its enum's type, which C++20 deprecates in arithmetic with
  another enum or a float, and which MSVC can't see fits a narrower type
  through `?:`. As `int` it reads as C's enumerator did.
- Gameplay and the original drawing, decoding and audio logic live in
  `src/Original`. `src/SDL` is the lowest practical OS and hardware boundary;
  host representation the game needs belongs in `src/Support`.
- Comments explain purpose, ordering or a quirk, not each assignment.
- A fault the rebuild adds (a game file the play can't go on without, a rule
  of the port's own broken) calls `RevengeFault` (`revenge/debug.h`); it is
  never one of the original's DOS exits.

## The one language extension

A struct without a name inside a union is allowed, and nothing else beyond
standard C++20. The game reads the 96h-byte unit record, the save directory
and a scene route both as named fields and as raw bytes or another view of
the same bytes (`currentArmor` and `currentInternal` over
`currentDurability`); the anonymous struct keeps those names exactly as the
original's offsets are read, where naming it would put a member name into
every field access. C11 has the construct; GCC, Clang and MSVC accept it in
C++ as an extension and warn. `revenge/types.h` defines
`REVENGE_ANONYMOUS_STRUCTS_BEGIN` and `REVENGE_ANONYMOUS_STRUCTS_END`, which
silence exactly that warning, and only `revenge/unit.h`,
`revenge/original/campaign_data.h` and `revenge/original/scene_data.h` use
them, around those unions.
