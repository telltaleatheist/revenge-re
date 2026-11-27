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
