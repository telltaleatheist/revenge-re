#!/usr/bin/env python3
"""Records a key script twice and checks the two runs are byte-identical:
every snap's frame, palette, save block and state, and the traces.

    tools/trace/repeat.py --exe BUILD/Revenge [--game GAME_DIR] --script SCRIPT.keys
        [--work DIR]

GAME_DIR defaults to the REVENGE_GAME variable; without one, exit 77
(ctest's skip). Exits 0 when the runs are the same.
"""
import argparse
import filecmp
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import record  # noqa: E402

SKIP = 77


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe", required=True)
    parser.add_argument("--game", default=os.environ.get("REVENGE_GAME", ""))
    parser.add_argument("--script", required=True)
    parser.add_argument("--work")
    args = parser.parse_args()
    if not (args.game and os.path.isdir(args.game)):
        print("repeat: no game folder (REVENGE_GAME); skipped")
        return SKIP
    with tempfile.TemporaryDirectory(prefix="revenge-repeat-") as temporary:
        return repeat(args, args.work or temporary)


def repeat(args, work):
    runs = [os.path.join(work, "first"), os.path.join(work, "second")]
    traces = [record.record(args.exe, args.game, args.script, run) for run in runs]
    files = sorted(n for n in os.listdir(runs[0]) if n not in ("saves",))
    different = [n for n in files if not filecmp.cmp(os.path.join(runs[0], n), os.path.join(runs[1], n),
                                                     shallow=False)]
    missing = sorted(set(os.listdir(runs[1])) ^ set(os.listdir(runs[0])))
    name = os.path.basename(args.script)
    if different or missing or traces[0] != traces[1]:
        print(f"repeat: {name}: the runs differ: {', '.join(different + missing) or 'the traces'}")
        return 1
    print(f"repeat: {name}: two runs byte-identical ({len(traces[0])} snaps, {len(files)} files)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
