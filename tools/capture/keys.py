#!/usr/bin/env python3
"""Compiles a key script (tools/trace/scripts/*.keys, the harness's syntax)
into KEYS.BIN for REVSNAP.COM, so one script drives the port and the original
under DOSBox-X alike.

    keys.py SCRIPT KEYS.BIN [--names NAMES.TXT]

KEYS.BIN: a word, the item count, then 20 bytes an item:
    kind (word: 1 key, 2 snap, 3 mouse), then three dwords, the waits added
    before it: idle:N (polls), steps:N (battle steps), still:N (polls without
    a step); then three words: for a key, the modifier flags (1 Shift, 2 Ctrl,
    4 Alt) and the scan code; for a mouse, X, Y and the buttons.

Key names are SDL's, mapped to the scan codes the port's keyboard gives them
(set 1, no E0 prefix: the arrows are the keypad's), so the game's own int 9
handler translates the same codes the port does. --names writes the @snap
names in capture order (RVS00000.BIN, RVS00001.BIN, ...), one a line.
"""
import argparse
import re
import struct
import sys

KEY, SNAP, MOUSE = 1, 2, 3
MAX_ITEMS = 2000
MAX_IDLE = 1000000

SCANS = {
    "escape": 0x01, "-": 0x0C, "minus": 0x0C, "=": 0x0D, "equals": 0x0D,
    "backspace": 0x0E, "tab": 0x0F, "[": 0x1A, "leftbracket": 0x1A,
    "]": 0x1B, "rightbracket": 0x1B, "return": 0x1C, "enter": 0x1C,
    "keypad enter": 0x1C, ";": 0x27, "semicolon": 0x27, "'": 0x28,
    "apostrophe": 0x28, "`": 0x29, "grave": 0x29, "\\": 0x2B, "backslash": 0x2B,
    ",": 0x33, "comma": 0x33, ".": 0x34, "period": 0x34, "/": 0x35, "slash": 0x35,
    "space": 0x39, "home": 0x47, "up": 0x48, "pageup": 0x49, "page up": 0x49,
    "left": 0x4B, "right": 0x4D, "end": 0x4F, "down": 0x50, "pagedown": 0x51,
    "page down": 0x51, "delete": 0x53,
}
for _index, _name in enumerate("1234567890"):
    SCANS[_name] = 0x02 + _index
for _row, _start in (("qwertyuiop", 0x10), ("asdfghjkl", 0x1E), ("zxcvbnm", 0x2C)):
    for _index, _name in enumerate(_row):
        SCANS[_name] = _start + _index
for _index in range(10):
    SCANS[f"f{_index + 1}"] = 0x3B + _index

MODIFIERS = (("shift+", 0x01), ("ctrl+", 0x02), ("alt+", 0x04))
SNAP_NAME = re.compile(r"[A-Za-z0-9_-]{1,47}")


def compile_script(path):
    """[(kind, idle, steps, still, a, b, c)], [snap names]; stops naming the bad token."""
    items, names, waits = [], [], {"idle": 0, "steps": 0, "still": 0}
    with open(path, encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            for token in line.split("#", 1)[0].replace(",", " ").split():
                where = f"{path}:{number}: {token!r}"
                lower = token.lower()
                wait = lower.split(":", 1)[0]
                if ":" in lower and wait in waits:
                    count = token.split(":", 1)[1]
                    if not count.isdigit() or int(count) > MAX_IDLE:
                        sys.exit(f"{where}: {wait}:N, N up to {MAX_IDLE}")
                    waits[wait] += int(count)
                    continue
                if token.startswith("@snap:"):
                    name = token[6:]
                    if not SNAP_NAME.fullmatch(name) or name in names:
                        sys.exit(f"{where}: a bad or repeated snap name")
                    names.append(name)
                    items.append((SNAP, *waits.values(), 0, 0, 0))
                elif lower.startswith("mouse:"):
                    parts = token[6:].split("/")
                    if len(parts) not in (2, 3) or not all(p.isdigit() for p in parts):
                        sys.exit(f"{where}: mouse:X/Y[/B]")
                    x, y = int(parts[0]), int(parts[1])
                    buttons = int(parts[2]) if len(parts) == 3 else 0
                    if x > 319 or y > 199 or buttons > 3:
                        sys.exit(f"{where}: X 0-319, Y 0-199, B 0-3")
                    items.append((MOUSE, *waits.values(), x, y, buttons))
                else:
                    flags, name = 0, lower
                    for prefix, bit in MODIFIERS:
                        while name.startswith(prefix):
                            flags |= bit
                            name = name[len(prefix):]
                    scan = SCANS.get(name.replace("_", " "))
                    if scan is None:
                        sys.exit(f"{where}: no such key")
                    items.append((KEY, *waits.values(), flags, scan, 0))
                waits = dict.fromkeys(waits, 0)
    if len(items) > MAX_ITEMS:
        sys.exit(f"{path}: {len(items)} items; REVSNAP holds {MAX_ITEMS}")
    return items, names


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("script")
    parser.add_argument("out")
    parser.add_argument("--names")
    args = parser.parse_args()
    items, names = compile_script(args.script)
    with open(args.out, "wb") as f:
        f.write(struct.pack("<H", len(items)))
        for item in items:
            f.write(struct.pack("<HIIIHHH", *item))
    if args.names:
        with open(args.names, "w", encoding="utf-8", newline="\n") as f:
            f.writelines(name + "\n" for name in names)
    print(f"{args.out}: {len(items)} items, {len(names)} snaps")


if __name__ == "__main__":
    main()
