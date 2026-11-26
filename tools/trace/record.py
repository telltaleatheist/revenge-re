#!/usr/bin/env python3
"""Records a key script's trace: runs the port under its harness and writes
one JSON line per @snap.

    tools/trace/record.py --exe BUILD/Revenge --game GAME_DIR --script SCRIPT.keys
        --out TRACE.jsonl [--work DIR] [--frames DIR] [--step-us N]

The port runs with no window and no sound, reading the game from GAME_DIR (it
never writes there) and writing its saves and snaps to WORK (emptied
first; a temporary folder, deleted afterwards, unless given).
--frames copies each snap's frame (the game's art) to DIR, outside the repo,
for tools/trace/frames.py. Exits 0 when the script ran to its end.
"""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile

TIMEOUT_SECONDS = 600


def digest(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()[:32]


def tokens_by_snap(script):
    """The script's tokens, grouped as each snap's keys: the tokens since the
    previous snap, its own @snap included."""
    groups, current = [], []
    with open(script, encoding="utf-8") as f:
        for line in f:
            for token in line.split("#", 1)[0].replace(",", " ").split():
                current.append(token)
                if token.startswith("@snap:"):
                    if any(name == token[6:] for name, _ in groups):
                        sys.exit(f"record: {os.path.basename(script)} takes {token} twice")
                    groups.append((token[6:], current))
                    current = []
    return groups


def run(exe, game, script, work, step_us=None):
    """Runs the port; returns (exit code, the run's output)."""
    if os.path.isdir(work):
        shutil.rmtree(work)
    os.makedirs(os.path.join(work, "saves"))
    env = dict(os.environ)
    env.update({
        "SDL_VIDEO_DRIVER": "dummy",
        "SDL_AUDIO_DRIVER": "dummy",
        "SDL_MAC_BACKGROUND_APP": "1",
        "REVENGE_ASSET_DIRECTORY": os.path.abspath(game),
        "REVENGE_SAVE_DIRECTORY": os.path.join(os.path.abspath(work), "saves") + os.sep,
        "REVENGE_HARNESS_SCRIPT": os.path.abspath(script),
        "REVENGE_HARNESS_OUT": os.path.abspath(work),
    })
    env.pop("REVENGE_SMOKE_INPUT", None)
    if step_us is not None:
        env["REVENGE_HARNESS_STEP_US"] = str(step_us)
    else:
        env.pop("REVENGE_HARNESS_STEP_US", None)
    try:
        done = subprocess.run([os.path.abspath(exe)], cwd=os.path.abspath(game), env=env,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        return -1, "timed out"
    return done.returncode, done.stdout.decode("utf-8", "replace")


def trace(script, work):
    """The trace lines from a finished run's snaps, in the script's order."""
    with open(os.path.join(work, "snaps.txt"), encoding="utf-8") as f:
        taken = f.read().split()
    records = []
    for (name, keys), snap in zip(tokens_by_snap(script), taken):
        if name != snap:
            sys.exit(f"record: the run took {snap} where the script has {name}")
        base = os.path.join(work, name)
        with open(base + ".state", encoding="utf-8") as f:
            state = [line.rstrip("\n") for line in f if line.strip()]
        records.append({
            "snap": name,
            "keys": keys,
            "state": state,
            "frame": {"indices": digest(base + ".indices"), "palette": digest(base + ".palette")},
            "save": digest(base + ".save"),
        })
    if len(records) != len(tokens_by_snap(script)):
        sys.exit(f"record: the run took {len(records)} of the script's {len(tokens_by_snap(script))} snaps")
    return records


def record(exe, game, script, work, step_us=None):
    """Runs the script and returns its trace records, or stops naming why."""
    code, output = run(exe, game, script, work, step_us)
    if code != 0 or not os.path.exists(os.path.join(work, "done.txt")):
        last = [line for line in output.splitlines() if line.startswith("harness:")]
        sys.exit(f"record: {os.path.basename(script)} didn't run to its end (exit {code})"
                 + (f": {last[-1]}" if last else ""))
    return trace(script, work)


def write(records, out):
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        for record_ in records:
            f.write(json.dumps(record_, separators=(",", ":")) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe", required=True)
    parser.add_argument("--game", required=True)
    parser.add_argument("--script", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--work")
    parser.add_argument("--frames")
    parser.add_argument("--step-us", type=int)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="revenge-trace-") as temporary:
        work = args.work or temporary
        records = record(args.exe, args.game, args.script, work, args.step_us)
        write(records, args.out)
        if args.frames:
            os.makedirs(args.frames, exist_ok=True)
            for record_ in records:
                for suffix in (".indices", ".palette"):
                    shutil.copyfile(os.path.join(work, record_["snap"] + suffix),
                                    os.path.join(args.frames, record_["snap"] + suffix))
    print(f"record: {len(records)} snaps to {args.out}")


if __name__ == "__main__":
    main()
