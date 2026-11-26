#!/usr/bin/env python3
"""Compares a candidate with the golden traces, snap by snap.

    tools/trace/check.py --exe BUILD/Revenge [--game GAME_DIR] [--only NAME ...] [--work DIR]
    tools/trace/check.py --traces DIR [--only NAME ...]

The first form records every script in tools/trace/scripts that has a golden
trace in tools/trace/traces (or those --only names) with the port at --exe;
the second compares traces already recorded (NAME.jsonl in DIR), such as the
rebuild's. GAME_DIR defaults to the REVENGE_GAME variable; without one, exit
77 (ctest's skip). For each snap it names what differs: the state lines (the
first that differ, both sides), the frame, its palette, the save block.
Exits 0 when everything matches.
"""
import argparse
import json
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import record  # noqa: E402

SCRIPTS = os.path.join(HERE, "scripts")
TRACES = os.path.join(HERE, "traces")
SKIP = 77


def load(path):
    with open(path, encoding="utf-8") as f:
        return [json.loads(line) for line in f if line.strip()]


def compare(name, golden, candidate):
    """Lines naming each difference; none when the traces match."""
    problems = []
    if [r["snap"] for r in golden] != [r["snap"] for r in candidate]:
        return [f"{name}: snaps {[r['snap'] for r in candidate]}, golden {[r['snap'] for r in golden]}"]
    for want, got in zip(golden, candidate):
        where = f"{name} @{want['snap']}"
        if want["keys"] != got["keys"]:
            problems.append(f"{where}: the script's keys differ from the golden's")
        if want["state"] != got["state"]:
            for index in range(max(len(want["state"]), len(got["state"]))):
                a = want["state"][index] if index < len(want["state"]) else "(none)"
                b = got["state"][index] if index < len(got["state"]) else "(none)"
                if a != b:
                    problems.append(f"{where}: state\n    golden    {a}\n    candidate {b}")
                    break
        for part in ("indices", "palette"):
            if want["frame"][part] != got["frame"][part]:
                problems.append(f"{where}: frame {part}")
        if want["save"] != got["save"]:
            problems.append(f"{where}: save block")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe")
    parser.add_argument("--game", default=os.environ.get("REVENGE_GAME", ""))
    parser.add_argument("--traces")
    parser.add_argument("--only", nargs="*")
    parser.add_argument("--work")
    args = parser.parse_args()
    if bool(args.exe) == bool(args.traces):
        sys.exit("check: give --exe or --traces")
    names = sorted(n[:-6] for n in os.listdir(TRACES) if n.endswith(".jsonl"))
    if args.only:
        unknown = sorted(set(args.only) - set(names))
        if unknown:
            sys.exit(f"check: no golden trace for {', '.join(unknown)}")
        names = [n for n in names if n in args.only]
    if args.exe and not (args.game and os.path.isdir(args.game)):
        print("check: no game folder (REVENGE_GAME); skipped")
        return SKIP
    with tempfile.TemporaryDirectory(prefix="revenge-check-") as temporary:
        return check(args, names, args.work or temporary)


def check(args, names, work):
    problems = []
    for name in names:
        golden = load(os.path.join(TRACES, name + ".jsonl"))
        if args.exe:
            candidate = record.record(args.exe, args.game, os.path.join(SCRIPTS, name + ".keys"),
                                      os.path.join(work, name))
        else:
            path = os.path.join(args.traces, name + ".jsonl")
            if not os.path.exists(path):
                problems.append(f"{name}: no candidate trace")
                continue
            candidate = load(path)
        found = compare(name, golden, candidate)
        print(f"check: {name}: " + ("matches" if not found else f"{len(found)} difference(s)")
              + f" ({len(golden)} snaps)")
        problems += found
    for problem in problems:
        print(problem)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
