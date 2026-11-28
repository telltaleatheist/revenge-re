# Original bugs and compatibility behavior

The supported DOS executable is the behavioral reference. A quirk demonstrated
by assembly or the original runtime is preserved by default, even when it is
undesirable by modern standards.

A change to original behavior is acceptable only when required for host safety,
interoperability or access to a modern platform boundary. Such a change must:

- be the smallest practical intervention;
- remain in `src/SDL` or `src/Support` whenever possible;
- identify the original procedure/address and observed behavior;
- carry a regression test or reproducible acceptance case; and
- be documented as an SDL/host compatibility decision rather than presented as
  original behavior.

Convenience, stylistic preference and speculative design improvement are not
sufficient reasons to fix an original game bug in the preservation branch.

## Switches

The project owner decided (2026-11-27) that the original's own bugs may also
be fixed behind a switch on the UPGRADES page (`include/revenge/upgrades.h`).
Such a fix is not a host change, and the rules above still hold for
everything else. Each switch:

- is off unless the player turns it on, so the game plays as the original
  did by default, and every recorded trace and golden runs with it off and
  stays byte-identical (the harness and the kit's `RE_KIT_TEST` turn every
  switch off, whatever the player's settings file says);
- gates its change at the original procedure it changes, which names the
  procedure's address and the switch, as `FIX (upgrade_<key>)`; with the
  switch off, the original's path runs unchanged;
- has a trace of its own with it on (a script's `# upgrades:` line), and a
  unit test of its gate;
- never makes the enemy stronger: the game is hard as it is, and a switch
  only takes away what stops the player's side, never what holds the enemy
  back (the project owner, 2026-11-27);
- changes nothing in a save, so a save loads alike with it on or off.

The player turns them on the UPGRADES page, from a line we add under the
game options menu's own (`Tactical/TacticalControl.cpp`, OpenUpgradesPage;
its words are ours, in `data/ui/upgrades.json`), and the page writes them to
`settings.txt` as `upgrade_<key> = on` or `off`. In test mode the line is
hidden but where a trace asks for it, and the page saves nothing.
`tools/check_upgrades.py` (ctest `preservation.upgrade_gates`) holds each
switch to its gate, its row below, its words and its trace.

The switches, their keys in `settings.txt` and what each changes are listed
below as they are added.

| Key | What it changes | Where |
| --- | --- | --- |
| `upgrade_lance_pursuit` | A lance waiting on the player's 'Mech after it has lost a leg, with no enemy in sight for a minute of the battle clock, is sent at the nearest enemy's cell, as the player's Move order would send it: the eighth battle's stall ends (DISCOVERIES.md, Stage 7). Only the player's own lance. | `Tactical/UnitState.cpp`, after 2D1B:08FA; `tests/test_lance_pursuit.cpp`; `stage_07_pursuit` |
