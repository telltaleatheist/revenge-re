#!/usr/bin/env python3
"""Checks tools/map/data_map.txt against the port's headers.

    tools/map/check_map.py

Every data object the headers declare has exactly one line, and every line
names one; no two placed objects share a byte unless an overlap line names the
pair; and src/SDL/map_dump.c is the one make_map_dump.py writes.
"""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import map_objects  # noqa: E402


def main():
    entries, overlaps, problems = map_objects.read_map()
    declared = {name for name, _, _, _ in map_objects.externs()}
    for name in sorted(declared - set(entries)):
        problems.append(f"{name} is declared but has no line")
    for name in sorted(set(entries) - declared):
        problems.append(f"{name} has a line but isn't declared")
    for pair in overlaps:
        for name in pair:
            if entries.get(name, ["?"])[0] != "place":
                problems.append(f"overlap names {name}, which isn't placed")

    placed = sorted((map_objects.linear(words[2]), int(words[3], 16), words[1])
                    for words in entries.values() if words[0] == "place")
    shared = set()
    for index, (start, size, name) in enumerate(placed):
        for other_start, _, other in placed[index + 1:]:
            if other_start >= start + size:
                break
            shared.add(frozenset((name, other)))
            if frozenset((name, other)) not in overlaps:
                problems.append(f"{name} and {other} share bytes and no overlap line names them")
    for pair in sorted(overlaps - shared, key=sorted):
        problems.append("overlap %s %s: they don't share a byte" % tuple(sorted(pair)))

    dump = subprocess.run([sys.executable, "-I", os.path.join(os.path.dirname(__file__), "make_map_dump.py"),
                           "--check"], capture_output=True, text=True)
    if dump.returncode != 0:
        problems.append(dump.stderr.strip() or "make_map_dump.py --check failed")

    for problem in problems:
        print(f"check_map: {problem}")
    kinds = {}
    for words in entries.values():
        kinds[words[0]] = kinds.get(words[0], 0) + 1
    print("check_map: %d objects: %d placed, %d host, %d unplaced; %d overlaps"
          % (len(entries), kinds.get("place", 0), kinds.get("host", 0), kinds.get("unplaced", 0), len(overlaps)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
