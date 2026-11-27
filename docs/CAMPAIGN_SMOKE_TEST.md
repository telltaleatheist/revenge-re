# Campaign acceptance record

This document separates the repeatable automated smoke path from broader manual
play testing. Neither is a claim that the complete campaign is verified.

## Automated smoke path

Established on 2026-09-29, the smoke path runs the SDL executable against a
private original-game installation. `REVENGE_SMOKE_INPUT` supplies delayed
items, `DELAY_MS:ITEM` comma-separated, each delay counted from the item
before:

- a key name as the trace scripts write them (`Shift+`, `Ctrl+`, `Alt+`
  allowed);
- `mouse:X/Y[/B]`: the mouse moved to X,Y of the game's 320x200, with the
  buttons B held (1 left, 2 right, 4 middle);
- `shot:NAME`: what the window shows next, saved as NAME.bmp in the folder
  `REVENGE_SMOKE_SHOTS` names.

Keys and the mouse are pushed on SDL's event queue as the devices' own events
and reach the game's keyboard and mouse handlers the way a real keyboard and
mouse do; nothing bypasses a menu or calls a game procedure directly.

| Check | Result |
| --- | --- |
| Title resources render before input | Pass |
| Main-menu keyboard navigation | Pass |
| New Game dispatches campaign stage 0 | Pass |
| Original save container can be loaded | Pass |
| Loaded campaign stage dispatches | Pass |
| Tactical resources, units and first frame initialize | Pass |
| Tactical interaction loop is entered | Pass |
| Confirmed title/options shutdown returns zero | Pass |

The keys used by this harness include `Return`, `Escape`, `Right`, `Down` and
`Up` (for example `REVENGE_SMOKE_INPUT=1500:Return,300:Down`, or with the
mouse `1500:mouse:40/188/1,100:mouse:40/188/0`). The harness is
optional and is inactive unless `REVENGE_SMOKE_INPUT` is set.

## Manual acceptance

Manual play testing has completed campaign missions 1 through 6. This covers
substantially more than the scripted tactical-entry smoke test, including
campaign transitions, lance assignment, tactical movement and targeting,
combat, mission completion, debriefing and continued save/campaign state.

The last behavior-changing acceptance baseline is commit `59f8cdab`. Later
commits through the documentation cleanup are preservation accounting,
repository organization and annotation readability work.

## Remaining validation

- Complete the remaining campaign and alternate branches.
- Exercise more save/load points across campaign stages.
- Compare presentation timing and audio effects with an original DOS run.
- Retest uncommon adapters and configuration combinations where practical.
- Record any discovered defect with the campaign stage, save state, input mode
  and trace output needed to reproduce it.

Original saves and assets used for acceptance remain private and are not part
of the repository.
