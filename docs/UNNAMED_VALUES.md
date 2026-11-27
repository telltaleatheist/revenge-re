# Unnamed values

Numbers in `src/Original` that look as if they mean something but whose
meaning isn't established, so they stay numbers: a wrong name is worse than
a number. One line each, by folder. A finding that settles one names it
(with the finding in the constant's comment, `DISCOVERIES.md` or the
README) and removes its line here.

Not listed, because they stay numbers for good: resource, message, screen,
music, sound, graphics, stage, scenario and pilot numbers; provenance
offsets; `Data` table rows; drawing coordinates and palette indices; byte
assembly; header-byte offsets without a struct.

## Audio

- `MIDIPlaybackFlags` bit 02h (blocks the tick: `& 0x03) != 0x01`).
- MIDI channel-state flag 80h (set with 40h on a program load).
- The MIDI byte parser's dispatch states 00h-1Ah.
- MIDI controller 21h and the `>= 7Bh` controller test.
- Digital mix scale 0100h (`SetDigitalPlaybackTimingScale`, `RequestedMixScale`).
- Digital mix bias 0AF2h.
- `InterruptNestingGuard` FFFFh.
- Digital playback results 02h and FFFFh.
- `SpeakerEffectTicksRemaining` FFFFh.
- `MusicDriverMode` 02h (`MUSIC_DRIVER_MODE_2`: the mode's meaning).
- `PITFrequencyToDivisor`'s `<= 12h` (18 Hz and below overflow a 16-bit divisor).

## Campaign

- `CampaignFlags` 0010h: set only in scenario 37h when unit 1 is out (`BattleRuntime`).
- `SaveTransferState` 8000h: set by stage 21 before the save index round trip; the load menu offers training when it holds 8000h.
- `CAMPAIGN_FLAG_MILESTONE_UNKNOWN_04` (0100h) and `_05` (0200h): named for their selector, meaning not established.
- `PresentationDismissTimeout` 01C2h, 0258h and 012Ch.
- The in-game menu's 05h items (from option 4) and its window tops 09h and 0Ah.
- `RunGameOptionsMenu`'s result 04h.
- The time zone's daylight-saving month bounds 04h and 0Ah, year offsets 46h and B2h, and day 3Bh.

## Graphics

- The tile-to-pixel `<< 0x03` and `+ 0x07) >> 0x03` in every folder (a later pass may name the shift).
- `PaletteBankAnchors` stride 30h and RGB step 03h.

## Input

- `NormalizeNavigationKeyCode`'s codes 000Ch, 0060h, 007Ch, 007Eh, 005Ch and -00F4h (mapped to right, down, left, up and home).
- `KeyDownBits` byte and bit pairs (`[5] & 10h`, `[7] & 02h/04h/50h`, `[8] & 01h`) and `heldModifiers` 40h: modifier-key bits not mapped to key names.

## Resources

- The LZW code readers' nibble shift 04h and the bit assembly in `ReadWestwoodLzw14Code`.
- The RLE run commands (00h word run, `<= 7Fh` literal).

## Runtime

- `StartupInputMode` 0201h (the keyboard's force-extended-table and repeat bits, named only inside `Keyboard.cpp`).
- The screen buffers' 1FD00h allocation.
- The version radix 10h and field 02h.
- The retrace count 3Ch and timeout 012Ch.

## Tactical

- Route slot 18h (`Targeting` 563 and 700: one waypoint instead of four): likely the command route, not confirmed.
- The options and command menus' case rows (`TacticalControl`, `Commands`).
- The lance base masks 00FCh, 001Ch and 000Ch, and the `PseudoRandomByte` masks.
- The viewport start positions (cursor X 0Ah and 12h, Y 06h and 0Bh).
- The combat-rating loop's 05h (the first five of `RevengeTacticalCombatRatings`' six words).
- `Weapons`' `selectionMask` bits 02h-40h.

## Text, UI and Utility

- The glyph width limit 19h.
- The text entry's blink bit 0200h.
- `StringFields` 3Fh and 15h.
- The UI dialog margin FFFFh.

## Units

- The pilot editor's sections 1 and 2 (the lance editor's three are named).
