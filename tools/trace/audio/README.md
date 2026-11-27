# The audio log's goldens

What the game asks to be heard, recorded from the port before its sound is
rebuilt (phase 3). Every later sound slice must leave these goldens as they
are, or name each change and why.

    tools/trace/audio.py check  --exe BUILD/Revenge --game GAME_DIR
    tools/trace/audio.py record --exe BUILD/Revenge --game GAME_DIR [--only NAME ...]
        [--devices card speaker] [--keep DIR]

ctest runs `check` as `audio.check`; it reads the game folder from
`REVENGE_GAME` and skips without one. Exit status: 0 all match, 1 a
difference, 77 skipped.

## The log

`REVENGE_AUDIO_LOG=FILE` turns it on (`include/revenge/audio_log.h`). Each
line is the host clock's time in microseconds (the harness's test clock, so a
script gives the same log every run), a kind, and its values:

| Line | Taken at |
| --- | --- |
| `midi SS D1 D2`, `midi reset`, `sysex N H` | the music sequencer's bytes to the music driver (`DispatchMIDIByte`) and its driver resets; a system-exclusive message is its length and a hash |
| `tone D` | the speaker: the PIT divisor while it sounds, 0 when silent (the effect timer and the speaker music driver) |
| `tone-effect I` | a speaker effect stream starting (`PlayPCSpeakerEffectByIndex`) |
| `pcm effect I N`, `pcm speech I N`, `pcm done` | a digital effect or speech sample starting, and the sample ending (`31FB`) |
| `snap NAME` | the harness's snap |

They are the game's requests, taken where the game makes them, not what a
device does with them, so they hold while the drivers underneath change.

## What runs

Every script with a golden trace (`../scripts`), and `scripts/music.keys`
(the title's tune to its end), each on two device settings written to the
run's `settings.txt`:

- `card`: `music = soundfont`, `sound = samples` (the game's Roland driver
  and its Sound Blaster path);
- `speaker`: `music = speaker`, `sound = speaker`.

The log is cut at each snap (what follows the last is the shutdown), and each
section is stored in `traces/NAME.DEVICE.jsonl` as its line count, its count
of each kind of line and the first 16 hex digits of its SHA-256. The goldens
hold no note or byte of the game's. `--keep DIR` leaves the whole logs there,
outside the repo, to compare by hand.

## What the logs show (2026-11-27)

- Card: each weapon or damage effect starts its sample (`pcm effect`), and
  speech plays through the card (`pcm speech`), waited for.
- Speaker: the same effects start speaker streams (`tone-effect`), and speech
  plays as a speaker bitstream (`pcm speech` then `pcm done` at once: the
  port doesn't time the bitstream).
- The sequencer sends the same notes on both settings. On the speaker the
  game's own set-up (its sysex for the speaker driver) gives the music one
  voice, which goes to the highest-priority channel sounding, so a few notes
  in many become tones. Until phase 3's music slice none did: the port kept
  426B:0004 (tested by the music timer) apart from 3A6A:8014 (written by
  `InitializeMusicDriver`), which are one byte, and its host never ran the
  driver's service, which the timer runs on every IRQ0 (1193182/400h Hz).
- Speaker effects are nearly silent: their notes look up the divisor table at
  426B:0020, which the original's image holds and the port leaves at zero, so
  only their explicit-frequency events sound. Phase 3's effects slice fills
  it, and the speaker goldens change there.
