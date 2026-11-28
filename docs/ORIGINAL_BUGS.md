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
- keeps saves interchangeable: a save made with a switch on loads with it
  off, and the reverse. The switch itself is never saved; what it changed in
  the campaign's state is saved as the original would save that state.

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
| `upgrade_restart_pilots` | A Restart from the in-game menu gives the campaign's replay copy the random pilot pool and count a new campaign starts with, where the original copies the initial copy's 15 zeros, so that every pilot the lance editor deals after it is pilot 0 (DISCOVERIES.md). Only the pilots the player is offered; the pool is saved with the campaign, and either pool loads with the switch on or off. | `Campaign/CampaignMenus.cpp`, at 0D73:274D's Restart (0D73:29AD-29D1); `tests/test_restart_pilots.cpp`; `restart_pilots` |
| `upgrade_lance_weapons` | A unit of the player's side, run by the computer, fires from priority list 14 the weapon each entry waits for, where the original fires the weapon the next entry waits for (DISCOVERIES.md). Sides as the original tells them (2D1B:08FA): units below 0Ch are the player's. Enemies fire as the original's. | `Tactical/Weapons.cpp`, in 2D1B:14A4; `tests/test_lance_ai.cpp`; `lance_weapons` |
| `upgrade_lance_tactics` | A unit of the player's side, choosing how close to fight (2D1B:08FA), judges itself, its target and its allies by firepower summed exactly, where the original's sum (0800:072C) leaks its mask's high word (DISCOVERIES.md; trace finding 6). The original's ratings are kept as they are; the exact ones are kept apart, so enemies, the status screen and the lance comparison (2208:26B1) see the original's. | `Tactical/UnitState.cpp`, at the end of 2D1B:0005 and in 2D1B:08FA; `tests/test_lance_ai.cpp`; `lance_tactics` |
