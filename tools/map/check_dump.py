#!/usr/bin/env python3
"""Checks a run's data against the original's image, by the map's places.

    tools/map/check_dump.py --exe BUILD/Revenge [--game GAME_DIR]

For each line of dump_checks.txt, runs the script with REVENGE_MAP_DUMP_DIR
set and compares the bytes the map's objects hold at those places at that
snap with the image's (README.md). GAME_DIR defaults to the REVENGE_GAME
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
    checks = []
    with open(CHECKS, encoding="utf-8") as f:
        for line in f:
            words = line.split()
            if words and not words[0].startswith("#"):
                checks.append((words[0], words[1], int(words[2], 16), int(words[3], 16), int(words[4], 16)))
    return checks


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
    checks = read_checks()
    with tempfile.TemporaryDirectory(prefix="revenge-dump-") as work:
        runs = {}
        for script, snap, offset, size, image_offset in checks:
            if script not in runs:
                folder = os.path.join(work, script + "-objects")
                os.makedirs(folder)
                os.environ["REVENGE_MAP_DUMP_DIR"] = folder
                record.record(args.exe, args.game, os.path.join(SCRIPTS, script + ".keys"),
                              os.path.join(work, script))
                del os.environ["REVENGE_MAP_DUMP_DIR"]
                runs[script] = folder
            objects = dumped(os.path.join(runs[script], snap + ".objects"))
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
    for problem in problems:
        print(f"check_dump: {problem}")
    print(f"check_dump: {len(checks) - len(problems)} of {len(checks)} checks hold")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
