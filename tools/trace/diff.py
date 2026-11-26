#!/usr/bin/env python3
"""Lists every difference between two traces of the same script, snap by
snap: the port's against the original's (tools/capture, state.py), or any two.

    tools/trace/diff.py --port DIR --original DIR [--frames PORT_FRAMES ORIGINAL_FRAMES]
        [--all] NAME ...

DIR holds NAME.jsonl. For each snap it prints the state lines that differ,
both sides; lines that measure time (the input line's clock, the timers, the
battle's update delay) are marked "timing" and left out unless --all. With
--frames (folders holding NAME/SNAP.indices, .palette and .save, as
record.py --frames and state.py --frames write them) it also counts the
differing pixels and names the differing save-block fields by the address
the game keeps them at. Exits 0 when nothing but timing differs.
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import state  # noqa: E402


def load(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


def timing_only(port, original):
    """The parts of two state lines that measure time, blanked."""
    def strip(line):
        words = line.split()
        if line.startswith("state input"):
            return " ".join(words[:6])          # polls and keys taken, not the clock
        if line.startswith("state timer"):
            return "state timer"
        if line.startswith("state battle"):
            return " ".join(words[:-2])         # all but the update delay
        return line
    return strip(port) == strip(original)


def save_fields():
    """(first, last, label) for each save-block field, by the game's address."""
    fields = []
    for offset, size, source in state.SNAPSHOT:
        fields.append((offset, offset + size - 1, "DS:%04X" % source))
    offset, count, first, stride = state.PILOT_AVAILABILITY
    for index in range(count):
        fields.append((offset + index, offset + index, "DS:%04X" % (first + index * stride)))
    base = state.SNAPSHOT_SIZE
    for unit in range(state.UNIT_COUNT):
        start = base + unit * state.UNIT_SIZE
        fields.append((start, start + state.UNIT_SIZE - 1, "unit %d (DS:%04X)" % (unit, state.LIVE_UNITS + unit * state.UNIT_SIZE)))
    return fields


def save_differences(port, original):
    lines = []
    for first, last, label in save_fields():
        differing = [i for i in range(first, last + 1)
                     if i < len(port) and i < len(original) and port[i] != original[i]]
        if not differing:
            continue
        where = ", ".join("+%X port %02x original %02x" % (i - first, port[i], original[i]) for i in differing[:6])
        more = " ..." if len(differing) > 6 else ""
        lines.append("  save %04X %s: %s%s" % (first, label, where, more))
    if len(port) != len(original):
        lines.append("  save sizes %d and %d" % (len(port), len(original)))
    return lines


def read(folder, name, snap, suffix):
    path = os.path.join(folder, name, snap + suffix)
    if not os.path.exists(path):
        return None
    with open(path, "rb") as f:
        return f.read()


def compare(name, port_dir, original_dir, frames, show_all):
    port = load(os.path.join(port_dir, name + ".jsonl"))
    original = load(os.path.join(original_dir, name + ".jsonl"))
    real = 0
    print(f"== {name}: {len(port)} snaps, {len(original)} in the original's trace")
    for a, b in zip(port, original):
        lines = []
        for x, y in zip(a["state"], b["state"]):
            if x == y:
                continue
            timing = timing_only(x, y)
            if timing and not show_all:
                continue
            tag = "timing " if timing else ""
            lines.append(f"  {tag}port     {x}\n  {tag}original {y}")
            real += not timing
        for x in a["state"][len(b["state"]):]:
            lines.append(f"  port only {x}")
            real += 1
        for y in b["state"][len(a["state"]):]:
            lines.append(f"  original only {y}")
            real += 1
        for part in ("indices", "palette"):
            if a["frame"][part] != b["frame"][part]:
                real += 1
                detail = ""
                if frames and part == "indices":
                    p, o = read(frames[0], name, a["snap"], ".indices"), read(frames[1], name, a["snap"], ".indices")
                    if p is not None and o is not None:
                        detail = ": %d pixels" % sum(1 for i, j in zip(p, o) if i != j)
                lines.append(f"  frame {part} differ{detail}")
        if a["save"] != b["save"]:
            real += 1
            p = frames and read(frames[0], name, a["snap"], ".save")
            o = frames and read(frames[1], name, a["snap"], ".save")
            lines.extend(save_differences(p, o) if p and o else ["  save differs"])
        print(f"@{a['snap']}: " + ("same" if not lines else "\n" + "\n".join(lines)))
    return real


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--port", required=True)
    parser.add_argument("--original", required=True)
    parser.add_argument("--frames", nargs=2, metavar=("PORT_FRAMES", "ORIGINAL_FRAMES"))
    parser.add_argument("--all", action="store_true")
    parser.add_argument("names", nargs="+")
    args = parser.parse_args()
    real = sum(compare(name, args.port, args.original, args.frames, args.all) for name in args.names)
    sys.exit(1 if real else 0)


if __name__ == "__main__":
    main()
