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

