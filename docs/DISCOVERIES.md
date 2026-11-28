# Discoveries

Things the rebuild found in the original's own data or code while taking its
numbers out. Each is kept as the original has it, so play stays the same
(`ORIGINAL_BUGS.md` has the policy); this list is for players and for a later
page on what the original does.

- **A computer unit's weapon-priority list 14 is shifted by one.** The AI
  fires from 16 lists of entries, each "fire this weapon if one of these is
  ready". In list 14 the five weapons fired are shifted one place against
  their readiness: each entry fires the weapon the next entry waits for, and
  the last fires the first's. Every other list fires the weapon it waits
  for. Kept in `data/rules/ai.json` (`weapon_priorities`); `src/Rules/Rules.h`
  describes it.

- **After a Restart, every random pilot is pilot 0.** The campaign keeps three
  copies of its random pools: initial, forward and replay. The game draws from
  replay, which starts with pilots 18 to 32. Restart (0D73:29AD-29D1) copies
  the initial copy over replay, but the initial copy's pilot pool is 15 zeros,
  so from then on each random pilot drawn is pilot 0. Kept in
  `data/start/new_campaign.json`; `src/Rules/Rules.h` describes it.

- **An enemy the scene sends in "without a pilot" barely hits.** A scene's
  deploy names a pilot roster slot for the unit, or FF for none. The game
  compares the slot, widened to a word, with FFFF (1A1F:339F), which a byte
  never is, so it assigns a pilot even for FF. Slot FF's record lies far past
  the 59 pilots, among the program's file names, so the unit takes its
  gunnery from two letters of a file name: a target number in the seventies,
  where a pilot's is a few. The first is the fourth battle's heavy unit, which
  then almost never hits. Kept; `tools/trace/README.md` (finding 5).

- **A computer unit can misjudge its own firepower.** Before choosing how
  close to fight, a computer unit rates its close, medium and long firepower
  by adding up its ready weapons for each range (0800:072C). The adding loop
  damages its own list as it goes: a weapon from the second half of the
  weapon table is never counted, and instead it makes the loop count weapons
  from the first half that the range doesn't use. A unit can so believe it
  outguns its target at long range and back away from a fight it should
  close, or the other way round. The first seen is the fourth battle's
  vehicle, which backs away from the player's 'Mech it was closing on. Kept;
  `tools/trace/README.md` (finding 6).

- **Some enemies take their action timing from the unit-name table.** How
  often a unit acts in a battle depends on its pilot's experience, 0 to 3,
  through a table of four timings (2D1B:08FA). The game doesn't check the
  experience, and a deploy can give a unit 41h: the fourth battle's units
  11h-14h have it. They read their timing far past the table, from a pointer
  into the unit names whose value DOS sets when it loads the game, so their
  timing in the original varied from machine to machine with where DOS put
  the game. Loaded where the reference runs had it (segment 131Eh), they act
  two subticks in eight, like a green pilot; under plain DOSBox-X (0823h),
  four in eight. Kept as the reference runs had it; `tools/trace/README.md`
  (finding 7).

- **Those enemies fire only when cool.** The same units, with experience 41h,
  read their heat limit past its table too (2D1B:14A4): a byte of 0, or 2
  with overburn, where a pilot's limit is 8 to 20. They fire only while their
  heat is 0 (or 2), so once they have fired they wait to cool right down
  before firing again. Kept; `tools/trace/README.md` (finding 9).

- **Stage 7 can wait for ever.** Shortly before 07:38 the lance leader, the
  player's own 'Mech, loses its left leg and can no longer walk (its walking
  speed 0; an engine hit it took is not what stops it). The next 'Mech leads
  the lance from then on, but its route ends where it stands, and from
  07:38:10 no unit on either side has a target: a unit picks only an enemy in
  sight or close by (2D1B:08FA). The last enemy, far across the map, has its
  speed set to stop and never moves, so the battle never ends. Send one of
  the other two after it (F2 or F3, then Move), but only once the nearer
  enemy is down; sent earlier, it leaves the leader alone, the enemy destroys
  it, and the battle is lost. The original does exactly this
  (tools/trace/scripts/stage_07.keys, checked against its capture to 07:55).
  The UPGRADES switch `upgrade_lance_pursuit` does it for the player
  (ORIGINAL_BUGS.md, Switches).

- **Stage 8 is won only after a fight.** On the forest path the lance must
  reach the far side of the map, but the scene checks for that only once one
  of the pass group or the road group's leader has been attacked; until
  then a unit can stand on the far edge and nothing happens. Send one unit
  at the pass group, its fire mode set to fire on any enemy (the mode's
  second choice), while the leader crosses. The original does exactly this
  (tools/trace/scripts/stage_08.keys, checked against its capture at every
  snap).

- **Stage 9 needs the lance taken through one unit at a time.** The leader
  and the APC must both stand on the gate's four cells (194-195, 98-99),
  but the enemy column marches down the road ahead of them and parks on
  those cells. Between the start and the open east half the forest leaves
  only the strip along the map's top and the road's three-cell corridor up
  to its crossing at 146,57. A lance order keeps the leader's formation,
  so the lance jams in the corridor, and the leader alone at the gate is
  destroyed. Send the APC (F7) along the top strip. Send the leader up the
  corridor once the column has passed the crossing (about 10:38), then each
  follower (F2-F4) through the crossing on its own. Meet the enemies when
  they come out from the gate. The leader, a tank, cannot cross the lake
  north of the gate, so take it round the lake's west side. The APC stops a
  cell short of its first order and needs a second. The original does
  exactly this (tools/trace/scripts/stage_09.keys, checked against its
  capture at every snap; won at 10:49).

## Port bugs the comparison found

Not the original's: places where the port's code differed from it, fixed so
the port plays as the original does. `tools/trace/README.md` has the
evidence (What the comparison found).

- **Stage 8 started with one Pegasus fewer** (finding 12). The stage's
  setup left out one of the five Pegasus copies, unit 9.
- **A Move order blanked the leader's route** (finding 13). The route the
  game keeps while an order is chosen was copied the wrong way round, so the
  lance's followers were sent towards the map's corner.
