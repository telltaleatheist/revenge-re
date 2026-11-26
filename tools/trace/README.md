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

- **A test clock** stands in for SDL's in every host timer: the game's timer
  interrupts, the BIOS tick count, delays, retrace waits and polls, the
  speaker's effects and the DOS date and time. It starts at zero and moves by the wait when the game waits, and by
  one step (`REVENGE_HARNESS_STEP_US`, 1000 µs unless set) when the game passes
  through a timer service or a retrace poll. Nothing sleeps, so a run takes
  seconds. The date is 1991-01-01 12:00:00 plus the clock.
- **Keys are fed at the game's own keyboard polls**, never by the host's speed:
  a key is pending on the 4th poll after the previous item, or that many more
  after `idle:N`, and, in a battle, once `steps:N` or `still:N` is met (below).
  No SDL event reaches the game; the mouse stays where the
  script or the game last put it, as a mouse nobody touches does.
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

## Trace records (`traces/*.jsonl`)

One JSON object a line, one line a snap:

```
{"snap": NAME, "keys": [the tokens since the previous snap],
 "state": ["state …", …],
 "frame": {"indices": SHA-256/128 of the 320×200 indexed frame,
           "palette": SHA-256/128 of the RGB colours it can show:
                      16 in the 16-colour mode, else 256},
 "save": SHA-256/128 of the 0x1031-byte save block}
```

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

# A run's snaps as PNGs, outside the repo
tools/trace/frames.py RUN_DIR OUT_DIR [--scale N]

# Every difference between two traces of a script (the port's and the original's)
tools/trace/diff.py --port DIR --original DIR [--frames PORT_FRAMES ORIGINAL_FRAMES] NAME ...
```

`check.py` names, for each snap, the first state line that differs (both
sides), the frame, the palette and the save block. ctest runs it as
`trace.goldens` and runs `repeat.py` on the battle as `trace.repeat`; both
read the game folder from `REVENGE_GAME` and skip without one.

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

What differs by nature, and `diff.py` marks as timing: the clock in the input
line, the timers, the battle's update delay. The poll counts are the same on
both sides; the number of polls a screen takes to settle is not, which is why
scripts wait with `idle:` generously and pace battles by steps.

### What the comparison found

Recorded with every script on 2026-11-26 (DOSBox-X 2024.03.01, the 1.00 DOS
image; `diff.py` with `--frames`). The state lines and the frames match at
every snap outside battles, and in the first battle every unit, the random
state and the clock match step by step through step 20. What else differs:

Not the port's:

- `title` @intro, the first poll: the original is still in DOS's text mode,
  the port's screen is blank.
- Battle frames, 8 to 100 pixels before contact: the status line and the
  clock's colon blink by passes.
- The selection cards' sprite pointers (save block 006C, bytes D-E of each
  15-byte card): the segment of a buffer the game allocates, which depends on
  where DOS loads it.

The port's, listed here until they are fixed:

1. **Line of sight walks the wrong cells.** The tracer's row steps
   (4184:08FB-092E) are the same memory as the eight words the map loader
   writes the map width into (3A6A:7A9B-7ACB, 1A1F:058F-05AC). The port keeps
   the tracer's tables constant at the image's 96 and writes the width to
   eight variables nothing reads; no map is 96 wide. In the first battle an
   enemy makes contact at step 22 in the port and not until later in the
   original, so `tactical` differs from its first message on (the subtick,
   the enemy's position, then everything).
2. **The clock's third byte is the subtick.** CampaignClockState[2] and
   TacticalClockSubtick are one byte in the original (DS:8339); the 3-byte
   copies into and out of the saved clock (1A1F:0233, 30F1) carry the subtick
   there and a byte nothing reads in the port. It shows in the state's clock
   line at every battle snap; the saved clock itself matches in these
   scripts, where it is saved while the subtick is 0.
3. **Initial values the image has and the port doesn't.**
   TacticalSelectedUnitIndex (DS:03DC) is FFFFh in the image, 0 in the port
   until a battle sets it; DS:1B8E is 0Fh in the image, 0 in the port (save
   block 005C); the pilot roster (DS:1D4C, 37 records of 4 bytes) holds each
   pilot's experience and gunnery in the image and zeros in the port, so the
   lance editor gives a 'Mech's live record (+8Ah, +93h) gunnery 0 and
   experience 0 where the original gives the pilot's.
