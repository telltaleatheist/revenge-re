# Traces: the port's behaviour, pinned

A trace is what the port does under a key script: at each `@snap` in the
script, the named state, a hash of the frame and its palette, and a hash of the
block a save would write at that moment. The goldens in `traces/` were recorded
from the port as it is; a change that moves any of them changes what the game
does, and the rebuild is checked against them snap by snap.

Everything runs headless (SDL's dummy video and audio drivers), reads the game
from a folder it never writes to, and writes its saves and snaps to a work
folder.

## The harness

The port has the harness built in (`src/SDL/harness.c`). It is off unless
`REVENGE_HARNESS_SCRIPT` names a script and `REVENGE_HARNESS_OUT` an existing
folder; `record.py` sets both. When it is on:

- **A test clock** stands in for SDL's as the game's clock, in whole
  milliseconds: what the timer interrupts did (the BIOS tick, the music
  timer, the digital sample tick), delays, retrace waits and the DOS date and
  time all run from it. It starts at zero and moves by the wait when the game
  waits, and by one millisecond when the game passes through a timer service
  or waits for the script's next key. Nothing sleeps, so a run takes
  seconds. The date is 1991-01-01 12:00:00 plus the clock.
- **Keys are fed at the game's own keyboard polls**, never by the host's speed:
  a key is pending on the 4th poll after the previous item, or that many more
  after `idle:N`, and, in a battle, once `steps:N` or `still:N` is met (below).
  No SDL event reaches the game; a `mouse:` item calls the game's mouse
  handler when it is due, as REVSNAP does.
- **Nothing is shown**: frames are kept in memory, not presented.

Running the same script twice gives byte-identical snaps (`repeat.py`).

## Scripts (`scripts/*.keys`)

Tokens, separated by spaces, commas or new lines; `#` starts a comment.

| Token | Meaning |
|---|---|
| `Return`, `Right`, `A`, `F1`, … | a key, by its SDL name; `Shift+`, `Ctrl+`, `Alt+` prefixes |
| `idle:N` | N more polls before the next item |
| `steps:N` | the next item waits for N battle steps since the previous key or snap |
| `still:N` | the next item waits for N polls in a row without a battle step |
| `mouse:X/Y[/B]` | moves the mouse to X,Y (320×200) with buttons B |
| `@snap:NAME` | takes a snap at the next wait; names are unique in a script |
| `quit` | the host's quit (the window's close button), at the next timer service: the game's shutdown and exit (3390:0024), as after its own quit, code 0 |
| `fail:allocation`, `fail:abort` | at the next timer service: the heap's next request fails as if the host had no memory, and the game goes on until it asks (a fault naming the request's size and caller, exit code 70); or a fatal path forced, a tracked file's slot closed though it isn't open (the resource abort and its two messages), which waits for its keys, then exits with code 1 |

The game polls the keyboard on every pass while a screen animates, so a few
polls after a key the screen is often still the old one; `idle:` lets it
settle before the next key or snap.

A battle can't be paced by polls: it moves one step (its units, weapons and
scene) on each pass of its loop whose update countdown has run out, and how
many passes a game tick holds depends on the machine. A **battle step** is one
move of the battle clock's subtick (DS:8339); the battle loop polls at least
once a pass and steps at most once, so a poll that sees the subtick changed
counts one step. `steps:N` lands the next item at the first poll after the Nth
step, the same point of the battle on any machine. `still:N` waits until N
polls have gone by without a step: a message or menu is up and the battle is
stopped, which is how a script answers a message without knowing its step.
So while a battle runs, a key waits by `steps:` alone: an `idle:` there lands
it after as many steps as the machine fits in those polls. `idle:` is for the
waits while a menu, a message or a unit's commands are up and the battle is
stopped. (Every item also waits 4 polls, well inside one step.)

| Script | Covers |
|---|---|
| `title` | the start, the title, the main menu |
| `new_campaign` | a new campaign to the first screen that waits for a key |
| `lance_editor` | the lance editor: a 'Mech to each of four pilots, then out |
| `tactical` | through the story to the first battle and to its end, each message answered when it is up |
| `battle_steps` | the first battle at steps 1, 2, 4, 8, 12, 16 and 20 |
| `quit_scenario` | the first battle quit from its options menu, the in-game menu, the battle replayed |
| `options` | the main menu's game options: the menu, an item toggled, back |
| `load_empty` | Load Game with no game saved: the slots, the empty slot's notice, back |
| `first_win` | the first battle won: the leader moved across the enemy's road, the menu after the stage |
| `second_battle` | on to the second battle: the options menu, a move order, steps 8 and 16, its messages to the end |
| `save_load` | after the first stage, Save & Continue to slot 1; out of the second battle and Load Game, slot 1 |
| `stages_02_04` | campaign set; from the 1e save: the second battle, then the third (stage 2) won by answering its messages, the fourth (stage 3) won with the lance moved east at once, the fifth (stage 4) won with the ammunition carrier driven at flank speed to its exit; its story, then Save & Continue to slot 1 |
| `stage_05` | campaign set; from the original's save after stage 4: the story, the first of the two plans chosen (through the forest), its battle won hands-off at 07:14; the transmission after it, Save & Continue to slot 1 |
| `stage_06_lost` | campaign set; from the original's save after stage 5: the Dragon battle (stage 6) lost, the lance sent at the Dragon at once and its leader lost at 07:27; the defeat, the menu after it, slot 1 loaded again |

## Trace records (`traces/*.jsonl`)

One JSON object a line, one line a snap:

```
{"snap": NAME, "keys": [the tokens since the previous snap],
 "state": ["state …", …],
 "frame": {"indices": SHA-256/128 of the 320×200 indexed frame,
           "palette": SHA-256/128 of the RGB colours it can show:
                      16 in the 16-colour mode, else 256},
 "save": SHA-256/128 of the 0x1031-byte save block,
 "objects": {NAME: hash, …}}
```

`objects` is every data object the headers declare (the map's,
tools/map/README.md) by name, with the first 16 hex digits of the SHA-256 of
its bytes (`src/SDL/map_dump.c`). In an object declared as a pointer or an
array of them, each pointer is hashed as where it points: the object it is in
and the offset there, a heap block by its number in allocation order and the
offset in it, a function by name, or "elsewhere"; so the hash is the same on every run and system, and
stays so when the port moves its objects. Other objects hash their bytes as
they are. The
file holds a snap's objects that changed since the snap before (all at a
script's first); `record.load` reads them back whole. `check.py` names each
object that differs.

The state lines, one fact each:

| Line | What |
|---|---|
| `state random A B C` | the random generator's three bytes (4184:0E48) |
| `state input polls P reads R clock MS` | keyboard polls, keys read, the test clock in ms |
| `state campaign stage … phase … variant … flags … saved … training …` | the campaign's progress |
| `state timer game G animation A` | the game's two tick counters |
| `state clock H M S subtick T` | the battle clock |
| `state battle result … selected … overview … delay …` | the battle's state |
| `state cursor map X Y viewport X Y` | the map cursor and view |
| `state unit I deploy D at X Y durability S heat H engine E pilot P` | each battle unit on the map |

The generator is never seeded and draws only when the game asks, so its bytes
count the draws made so far.

The frames themselves are the game's art and never go in the repo:
`record.py --frames DIR` copies them out, and `frames.py RUN_DIR OUT_DIR`
turns a run's snaps into PNGs to look at.

## Tools

```
# Record a golden (or a candidate) from a build
tools/trace/record.py --exe BUILD/Revenge --game GAME_DIR \
    --script tools/trace/scripts/tactical.keys --out tools/trace/traces/tactical.jsonl

# Every golden against a build (or against traces already recorded)
tools/trace/check.py --exe BUILD/Revenge --game GAME_DIR
tools/trace/check.py --traces DIR

# The same script twice, byte for byte
tools/trace/repeat.py --exe BUILD/Revenge --game GAME_DIR --script tools/trace/scripts/tactical.keys

# Every golden with the story imported and no program file in the game folder
tools/trace/exe_free.py --exe BUILD/Revenge --game GAME_DIR

# What the game asks to be heard, on the card and the speaker (audio/README.md)
tools/trace/audio.py check --exe BUILD/Revenge --game GAME_DIR

# The host's quit at the title, in a menu and mid-battle; the game's fatal exits
tools/trace/quit.py --exe BUILD/Revenge --game GAME_DIR
tools/trace/fatal_exit.py --exe BUILD/Revenge --game GAME_DIR

# A run's snaps as PNGs, outside the repo
tools/trace/frames.py RUN_DIR OUT_DIR [--scale N]

# Every difference between two traces of a script (the port's and the original's)
tools/trace/diff.py --port DIR --original DIR [--frames PORT_FRAMES ORIGINAL_FRAMES] NAME ...
```

`check.py` names, for each snap, the first state line that differs (both
sides), the frame, the palette and the save block. ctest runs it as
`trace.goldens`, runs `repeat.py` on the battle as `trace.repeat`, and runs
`exe_free.py` as `trace.exe_free`: one run imports the story, then every
script plays from a folder of links to the game's files, its `.EXE` files left
out, and must match its golden without importing again. All three read the
game folder from `REVENGE_GAME` and skip without one.

A script with a line `# set: campaign` is one of the long runs through the
campaign. `trace.goldens` (`check.py --set goldens`) leaves those out, as do
`exe_free.py` and the audio check; `trace.campaign` (`--set campaign`) runs
them. The ctest labels are the same names: `ctest -L trace.goldens` for the
everyday run, `ctest -L trace.campaign` for the campaign.

`old_save` starts from a save the original wrote (its `# start-save:` line):
the 1e save_load run's SAVEGAME.DAT, kept outside the repo, named by
`REVENGE_ORIGINAL_SAVE`. Without that variable `check.py` skips that script
alone. The campaign scripts make a chain from it: each ends with Save &
Continue, and the SAVEGAME.DAT the original writes there (under DOSBox-X,
never the port's) starts the next script. Each link's save is named by its own
variable: `REVENGE_SAVE_STAGE_04` for the one stages_02_04 ends with, which
stage_05 starts from, `REVENGE_SAVE_STAGE_05` for stage_05's, which
stage 6's scripts start from.

To re-record a golden after an intended change, record over it and say in the
commit what moved and why.

## Timing and the random generator

At the clock steps 250, 500, 1000, 2000 and 4000 µs, the four scripts give the
same generator bytes, saves, units and campaign state at every snap; only the
counters measured in ticks, the animation phase in the battle's frames and the
battle's update countdown follow the step. Every draw in these runs comes from
the dice (2D6 rolls), which run once per game tick or per attack.

One caller draws per display pass instead: the map-edge effects some battle
scenes queue (2007:1AE3), which roll for each live effect on every pass of the
battle loop. On a machine where passes per tick vary, those draws interleave
differently with the dice. No scene in these scripts queues one; a script
that reaches one should be recorded at more than one step.

Two more things in a battle follow passes, not steps: the narration prompt's
countdown (PendingNarrationDelay, counted down by each redraw of the status
panel, so a message a lance other than the first reports stays readable for a
number of passes) and the blinking of the status line and the clock's colon.
Neither changes a state line in these scripts; the blinking changes battle
frames, which is why a battle's frames differ from the original's while its
state doesn't.

## The original (`tools/capture/`)

The same scripts drive the original game, REVENGE.EXE, under DOSBox-X, and its
traces compare with the port's line by line. Nothing from the game is in this
repo: the tools find the game's code in memory as it runs and check it by sum
before touching it, and patch nothing on disk.

- **REVSNAP.COM** (`revsnap.asm`, `nasm -f bin`) stays resident before the game
  starts. When the game installs its keyboard handler (int 21h AX=2509h), it
  checks the game's keyboard code against three sums and, if they match,
  points the game's "key pending?" and "read a key" routines at itself, in
  memory only. From then on every keyboard poll the game makes is one poll of
  the script, with the harness's rules: the 4th poll after the previous item,
  `idle:`, `steps:` (the subtick at DS:8339) and `still:`. A key goes in as
  the hardware would send it (make and break scan codes through the
  keyboard controller and IRQ 1), so the game's own interrupt handler
  translates it; a mouse item calls the game's mouse handler. The DOS date
  and time are 1991-01-01 12:00:00 plus the BIOS ticks since REVSNAP started.
  If a sum doesn't match it touches `NOPATCH.TXT` and lets the game run
  untouched.
- **A snap** writes `RVSnnnnn.BIN`: the video mode, the image's load segment,
  the poll and key counts, the ticks, the CRTC start and attribute registers,
  the DAC, the frame (in the 16-colour mode the four planes from the CRTC
  start, else the 64000 bytes at A000:0000) and the first 640K of memory. At
  the end of the script it writes `REVSNAPE.TXT`.
- **`keys.py`** compiles a script into `KEYS.BIN` for REVSNAP; **`run_wsl.sh`**
  runs DOSBox-X headless on Linux or WSL (a 386 at fixed cycles, no sound)
  with a copy of the game, REVSNAP and the keys, and stops it after the end
  marker; **`state.py`** turns the captures into a trace in the harness's
  format: the same state lines read from the game's own addresses, the frame
  and its palette (in the 16-colour mode the 16 colours the attribute
  registers pick), and the save block assembled field by field from where the
  game keeps each one.

```
nasm -f bin -o $REVSNAP_HOME/REVSNAP.COM tools/capture/revsnap.asm
tools/capture/keys.py tools/trace/scripts/NAME.keys KEYS.BIN
REVSNAP_HOME=DIR DOSBOX_X=dosbox-x tools/capture/run_wsl.sh NAME KEYS.BIN
tools/trace/state.py --script tools/trace/scripts/NAME.keys --run DIR/run-NAME --out NAME.jsonl [--frames DIR]
tools/trace/diff.py --port PORT_TRACES --original ORIGINAL_TRACES NAME
```

What differs by nature, and `diff.py` marks as timing: the polls and the
clock in the input line, the timers, the battle's update delay. Outside
battles the poll counts are the same on both sides; in a battle the number of
passes, and so of polls, a step takes is the machine's, which is why battles
are paced by steps. The number of polls a screen takes to settle differs too,
which is why scripts wait with `idle:` generously.

### What the comparison found

Recorded with every script on 2026-11-26 (DOSBox-X 2024.03.01, the 1.00 DOS
image; `diff.py` with `--frames`). The state lines and the save blocks match
at every snap of all eleven scripts, but for the cards' pointers below: the
first battle lost (`tactical`) and won (`first_win`), the second battle with
an order (`second_battle`), and a save and its load (`save_load`), where the
two SAVEGAME.DAT files differ only in those pointers too. What differs, none
of it the port's:

- `title` @intro, the first poll: the original is still in DOS's text mode,
  the port's screen is blank.
- Battle frames, 8 to 180 pixels: the status line and the clock's colon blink
  by passes.
- The selection cards' sprite pointers (save block 006C, bytes D-E of each
  15-byte card): the segment of a buffer the game allocates, which depends on
  where DOS loads it.

Not covered yet: the overview map (F10) opens only in a scenario whose data
enables it, and neither of the first two does; the training battle comes with
a later stage of the campaign.

What the comparison found in the port, and how each was fixed:

1. **Line of sight walked the wrong cells.** The tracer's row steps
   (4184:08FB-092E) are the memory the map loader writes the map's width
   into (3A6A:7A9B-7ACB, 1A1F:058F-05AC); the port kept them at the image's
   96 and wrote the width to eight variables nothing read, so in the first
   battle an enemy made contact at step 22, not 43. The loader now writes the
   tables (tactical.original_line_of_sight_map_width).
2. **The clock's third byte is the subtick** (DS:8339): the port kept a
   separate byte, so the clock saved on leaving a scenario (30F1:0006) and
   restored at a battle's start (1A1F:0233) didn't carry it. It is one byte
   now (tactical.original_scenario_clock_subtick).
3. **Initial values the image has.** The pilot roster (DS:1D4C, 59 records:
   each pilot's experience and gunnery, which the lance editor copies into the
   'Mechs' live records) and DS:1B8E started at zero in the port, the
   selected unit (DS:03DC) at 0 where the image has FFFFh, no unit. The first
   two come from the player's copy now, the third is FFFFh
   (import.original_initial_data_values). Without the pilots' gunnery the
   first battle parted from its second message on.
4. **The title text's fade-out starts one past its colours.** The original
   (0D73:2F5F) fades the line in through the four colours at DS:2718 and out
   from index 4 down, so its first fade-out step uses DS:271C, the low byte of
   MissionBlinkRectX[0]. An original quirk, kept. The port read index 4 past
   its C array, a byte that depended on the host's layout and differed
   between the Mac's two architectures; it reads MissionBlinkRectX[0] as it
   stands now (campaign.original_mission_fade_color). Found by UBSan.
5. **A scene's deploy assigns a pilot for slot FF.** The deploy opcode
   (1A1F:339F) compares its roster-slot byte, widened to a word, with FFFF,
   so it always calls AssignPilotRosterEntryToUnit (30F1:0AFC), and slot FF
   is read 0x3FC bytes on from the roster (DS:1D4C), at DS:2148: four bytes
   of a table of file names. The unit's pilot is FF and its gunnery that
   entry's, a number far above any pilot's. An original bug, kept. The port
   skipped the call for FF, so in stages_02_04 SCENE4's unit 16 had pilot 0
   and the template's gunnery where the original's has pilot FF. It assigns
   now, from those four bytes imported from the player's copy
   (PilotRosterRecordPastEnd; tactical.original_deploy_pilot).
6. **The weapon-rating sum leaks its mask's high word.** A unit's close,
   medium and long firepower (2D1B:0005) are sums of a rating per ready
   weapon, taken through a 32-bit mask of the weapons each range counts
   (0800:072C). Its loop tests the mask for zero with `or bx,bp`, which
   leaves the high word ORed into the low word on every pass, so a weapon in
   the high half (10h-16h) is never counted itself, and its bit walks down
   the low half and counts weapons the mask did not select. An original bug,
   kept. The port summed the mask as written: in stages_02_04, when SCENE4's
   unit 20 got weapon 10h back from recycling at step 195, the original rated
   its medium and long firepower at twice the port's, chose long range
   against unit 1, and at step 197 backed away from it where the port's
   closed in and fired. The loop is the original's now
   (tactical.original_weapon_rating). With it, every state line and save
   block of stages_02_04 matches the original's.
