#!/usr/bin/env python3
"""Records a key script's trace: runs the port under its harness and writes
one JSON line per @snap.

    tools/trace/record.py --exe BUILD/Revenge --game GAME_DIR --script SCRIPT.keys
        --out TRACE.jsonl [--work DIR] [--frames DIR]

The port runs with no window and no sound, reading the game from GAME_DIR (it
never writes there) and writing its saves and snaps to WORK (emptied
first; a temporary folder, deleted afterwards, unless given).
--frames copies each snap's frame and save block (the game's art and data) to
DIR, outside the repo, for tools/trace/frames.py and diff.py. Exits 0 when the script ran to its end.

A script with a line `# start-save: VARIABLE` starts with the SAVEGAME.DAT that
the environment variable VARIABLE names in its save folder (a save written by
the original, kept outside the repo). Without the variable it can't run, and
is skipped (exit 77 here).

A script with a line `# upgrades: KEY ...` runs with those UPGRADES switches
on (include/revenge/upgrades.h); every other run has them all off, whatever
a settings file says. REVENGE_TRACE_UPGRADES, when set, stands for every
script's line: to see which traces a switch moves, run check.py with it.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

TIMEOUT_SECONDS = 600
SKIP = 77
# The settings every trace runs with, as the original's captures were made:
# the speaker for the music and the sound, and the keys (a new settings.txt
# starts with the mouse, whose cursor and battle cursor the scripts don't
# drive).
TRACE_SETTINGS = "video = vga\nmusic = speaker\nsound = speaker\ninput = keys\n"
START_SAVE = re.compile(r"^#\s*start-save:\s*(\w+)\s*$", re.M)
SET = re.compile(r"^#\s*set:\s*(\w+)\s*$", re.M)
UPGRADES = re.compile(r"^#\s*upgrades:\s*([\w ,]+?)\s*$", re.M)
GOLDENS = "goldens"


def start_save(script):
    """The save a script starts with: (variable, path or None), or None if none."""
    with open(script, encoding="utf-8") as f:
        found = START_SAVE.search(f.read())
    if not found:
        return None
    path = os.environ.get(found.group(1), "")
    return found.group(1), (path if path and os.path.isfile(path) else None)


def upgrades(script):
    """The UPGRADES switches a script runs with on: its `# upgrades:` line's
    keys, as REVENGE_TEST_UPGRADES takes them, or "" for none."""
    with open(script, encoding="utf-8") as f:
        found = UPGRADES.search(f.read())
    return " ".join(found.group(1).replace(",", " ").split()) if found else ""


def script_set(script):
    """The set a script belongs to: its `# set: NAME` line (campaign: the long
    runs through the campaign, a ctest of their own), else goldens."""
    with open(script, encoding="utf-8") as f:
        found = SET.search(f.read())
    return found.group(1) if found else GOLDENS


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


def run(exe, game, script, work, story=None, settings=None, env_extra=None):
    """Runs the port; returns (exit code, the run's output). With story, a
    story folder (an earlier run's saves/story) is put in the save folder
    first, so the run finds its story already imported. settings is the
    text of the save folder's settings.txt (the player's settings, in place
    of the install's MEDIA.CFG): TRACE_SETTINGS unless given. env_extra adds
    variables to the run's."""
    if os.path.isdir(work):
        shutil.rmtree(work)
    os.makedirs(os.path.join(work, "saves"))
    if story:
        shutil.copytree(story, os.path.join(work, "saves", "story"))
    if settings is None:
        settings = TRACE_SETTINGS
    with open(os.path.join(work, "saves", "settings.txt"), "w", encoding="ascii", newline="\n") as f:
        f.write(settings)
    wanted = start_save(script)
    if wanted:
        if wanted[1] is None:
            return SKIP, f"no save in {wanted[0]}"
        shutil.copyfile(wanted[1], os.path.join(work, "saves", "SAVEGAME.DAT"))
    env = dict(os.environ)
    env.update({
        "SDL_VIDEO_DRIVER": "dummy",
        "SDL_AUDIO_DRIVER": "dummy",
        "SDL_MAC_BACKGROUND_APP": "1",
        "REVENGE_ASSET_DIRECTORY": os.path.abspath(game),
        "REVENGE_SAVE_DIRECTORY": os.path.join(os.path.abspath(work), "saves") + os.sep,
        "REVENGE_HARNESS_SCRIPT": os.path.abspath(script),
        "REVENGE_HARNESS_OUT": os.path.abspath(work),
        "REVENGE_TEST_UPGRADES": os.environ.get("REVENGE_TRACE_UPGRADES", upgrades(script)),
    })
    env.pop("REVENGE_SMOKE_INPUT", None)
    env.pop("REVENGE_AUDIO_LOG", None)
    env.update(env_extra or {})
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
        with open(base + ".names", encoding="utf-8") as f:
            objects = dict(line.split() for line in f if line.strip())
        records.append({
            "snap": name,
            "keys": keys,
            "state": state,
            "frame": {"indices": digest(base + ".indices"), "palette": digest(base + ".palette")},
            "save": digest(base + ".save"),
            "objects": objects,
        })
    if len(records) != len(tokens_by_snap(script)):
        sys.exit(f"record: the run took {len(records)} of the script's {len(tokens_by_snap(script))} snaps")
    return records


def record(exe, game, script, work):
    """Runs the script and returns its trace records, or stops naming why."""
    code, output = run(exe, game, script, work)
    if code == SKIP and output.startswith("no save in "):
        print(f"record: {os.path.basename(script)} skipped: {output}")
        sys.exit(SKIP)
    if code != 0 or not os.path.exists(os.path.join(work, "done.txt")):
        last = [line for line in output.splitlines() if line.startswith("harness:")]
        sys.exit(f"record: {os.path.basename(script)} didn't run to its end (exit {code})"
                 + (f": {last[-1]}" if last else ""))
    return trace(script, work)


def write(records, out):
    """The records, one a line. A snap's objects are those that changed since
    the previous snap (all of them at the first); load() puts them back."""
    previous = None
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        for record_ in records:
            line = dict(record_)
            if "objects" in line:
                objects = line["objects"]
                if previous is not None:
                    if set(objects) != set(previous):
                        sys.exit(f"record: the objects at {line['snap']} aren't those of the snap before")
                    line["objects"] = {n: h for n, h in objects.items() if previous[n] != h}
                previous = objects
            f.write(json.dumps(line, separators=(",", ":")) + "\n")


def load(path):
    """A trace file's records, each snap's objects whole."""
    records = []
    current = None
    with open(path, encoding="utf-8") as f:
        for line in f:
            if not line.strip():
                continue
            record_ = json.loads(line)
            if "objects" in record_:
                current = dict(record_["objects"]) if current is None else {**current, **record_["objects"]}
                record_["objects"] = current
            records.append(record_)
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe", required=True)
    parser.add_argument("--game", required=True)
    parser.add_argument("--script", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--work")
    parser.add_argument("--frames")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="revenge-trace-") as temporary:
        work = args.work or temporary
        records = record(args.exe, args.game, args.script, work)
        write(records, args.out)
        if args.frames:
            os.makedirs(args.frames, exist_ok=True)
            for record_ in records:
                for suffix in (".indices", ".palette", ".save"):
                    shutil.copyfile(os.path.join(work, record_["snap"] + suffix),
                                    os.path.join(args.frames, record_["snap"] + suffix))
    print(f"record: {len(records)} snaps to {args.out}")


if __name__ == "__main__":
    main()
