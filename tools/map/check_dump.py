#!/usr/bin/env python3
"""Checks a run's data against the original's image, by the map's places.

    tools/map/check_dump.py --exe BUILD/Revenge [--game GAME_DIR]

Runs each script dump_checks.txt names with REVENGE_MAP_DUMP_DIR set. A
check line compares the bytes the map's objects hold at a place at a snap
with the image's; an agree line has every pair of objects the map lays over
one another, but for the noted ones, hold equal bytes where they meet at
every snap of the script, as the original's one byte would (README.md). GAME_DIR defaults to the REVENGE_GAME
variable; without one, exit 77 (ctest's skip).
"""
import argparse
import os
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "trace"))
import map_objects  # noqa: E402
import record  # noqa: E402

SKIP = 77
CHECKS = os.path.join(HERE, "dump_checks.txt")
SCRIPTS = os.path.join(os.path.dirname(HERE), "trace", "scripts")
DATA_SEGMENT = map_objects.linear("3A6A:0000")
# DOS 1.00: the data segment's place in the load image.
DATA_SEGMENT_IMAGE_OFFSET = 0x326A0


def image_data(game):
    for name in os.listdir(game):
        if name.upper() == "REVENGE.EXE":
            with open(os.path.join(game, name), "rb") as f:
                exe = f.read()
            header = struct.unpack_from("<H", exe, 8)[0] * 16
            return exe[header + DATA_SEGMENT_IMAGE_OFFSET:]
    sys.exit("check_dump: no REVENGE.EXE in the game folder")


def read_checks():
    checks, agrees = [], []
    with open(CHECKS, encoding="utf-8") as f:
        for line in f:
            words = line.split()
            if not words or words[0].startswith("#"):
                continue
            if words[0] == "agree":
                agrees.append(words[1])
            else:
                checks.append((words[0], words[1], int(words[2], 16), int(words[3], 16), int(words[4], 16)))
    return checks, agrees


def disagreements(folder, script):
    """The overlaps without a note whose objects differ where they meet."""
    noted = {}
    entries, overlaps, _ = map_objects.read_map(noted=noted)
    placed = {name: (map_objects.linear(words[2]) - DATA_SEGMENT, int(words[3], 16))
              for name, words in entries.items() if words[0] == "place"}
    found = []
    for snap in sorted(n[:-8] for n in os.listdir(folder) if n.endswith(".objects")):
        objects = dumped(os.path.join(folder, snap + ".objects"))
        for pair in sorted(overlaps - set(noted), key=sorted):
            first, second = sorted(pair)
            (first_start, first_size), (second_start, second_size) = placed[first], placed[second]
            low = max(first_start, second_start)
            high = min(first_start + first_size, second_start + second_size)
            if objects[first][low - first_start:high - first_start] \
                    != objects[second][low - second_start:high - second_start]:
                found.append(f"{script} {snap}: {first} and {second} differ at DS:{low:04X}-{high - 1:04X}")
    return found


def holders():
    """{DS offset: (object name, offset in the object)} for every placed byte."""
    entries, _, _ = map_objects.read_map()
    held = {}
    for words in entries.values():
        if words[0] != "place":
            continue
        start = map_objects.linear(words[2]) - DATA_SEGMENT
        for at in range(int(words[3], 16)):
            held.setdefault(start + at, (words[1], at))
    return held


def dumped(path):
    objects = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            words = line.split()
            objects[words[0]] = bytes.fromhex(words[2]) if len(words) > 2 else b""
    return objects


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe", required=True)
    parser.add_argument("--game", default=os.environ.get("REVENGE_GAME", ""))
    args = parser.parse_args()
    if not (args.game and os.path.isdir(args.game)):
        print("check_dump: no game folder (REVENGE_GAME); skipped")
        return SKIP
    image = image_data(args.game)
    held = holders()
    problems = []
    checks, agrees = read_checks()
    with tempfile.TemporaryDirectory(prefix="revenge-dump-") as work:
        runs = {}

        def run(script):
            if script not in runs:
                folder = os.path.join(work, script + "-objects")
                os.makedirs(folder)
                os.environ["REVENGE_MAP_DUMP_DIR"] = folder
                record.record(args.exe, args.game, os.path.join(SCRIPTS, script + ".keys"),
                              os.path.join(work, script))
                del os.environ["REVENGE_MAP_DUMP_DIR"]
                runs[script] = folder
            return runs[script]

        for script, snap, offset, size, image_offset in checks:
            objects = dumped(os.path.join(run(script), snap + ".objects"))
            run_bytes = bytearray()
            for at in range(offset, offset + size):
                if at not in held:
                    problems.append(f"{script} {snap}: no object is placed at DS:{at:04X}")
                    break
                name, inside = held[at]
                run_bytes.append(objects[name][inside])
            else:
                expected = image[image_offset:image_offset + size]
                if bytes(run_bytes) != expected:
                    problems.append(f"{script} {snap}: DS:{offset:04X} holds {run_bytes.hex()},"
                                    f" the image's DS:{image_offset:04X} {expected.hex()}")
        failed_checks = len(problems)
        for script in agrees:
            problems += disagreements(run(script), script)
    for problem in problems:
        print(f"check_dump: {problem}")
    print(f"check_dump: {len(checks) - failed_checks} of {len(checks)} checks hold;"
          f" {len(problems) - failed_checks} disagreements in {len(agrees)} scripts")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
