#!/usr/bin/env python3
"""The game's fatal exits end through its exit (38C6:0010) with its message
shown and the original's exit code, and without a crash.

    tools/trace/fatal_exit.py --exe BUILD/Revenge [--game GAME_DIR]

Three runs, each from the title:

- save: a save whose version is too old (the original's save named by
  REVENGE_ORIGINAL_SAVE, its version word set to 0096h): the main menu's Load
  Game reads it, and the invalid-save dialog (0D73:4611) shows; Escape exits.
- allocation: a script's `fail:allocation` asks the far heap for 65520 bytes
  until it has none. The allocation error (0D56:0007) prints its message, the
  request and the bytes left; a key exits.
- abort: a script's `fail:abort` closes a tracked file's slot that isn't open
  (3390:0244). The error's box (3390:071E), then the fatal disk error's
  (3390:097B), each until a key; then the exit.

Each must exit with code 1 after its snaps and before the script's end (no
done.txt), and leave the save folder as it was. A message shows: the snap
while it's up differs from the one before it, and the allocation error's
texts, read from the imported story, are in the run's output. GAME_DIR
defaults to the REVENGE_GAME variable; without one, exit 77 (ctest's skip).
Without REVENGE_ORIGINAL_SAVE the save run alone is skipped. Exits 0 when
every run holds.
"""
import argparse
import json
import os
import struct
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import record  # noqa: E402

SKIP = 77
OLD_VERSION = 0x0096  # one below the oldest the game takes (0D73:4572)
CASES = {
    "save": ("# start-save: REVENGE_FATAL_SAVE\n"
             "idle:1000 Return idle:50 Right idle:50 @snap:before Return idle:300 "
             "@snap:message Escape\n"),
    "allocation": "idle:1000 @snap:before fail:allocation idle:50 @snap:message Return\n",
    "abort": "idle:1000 @snap:before fail:abort idle:50 @snap:message Return idle:50 @snap:fatal Return\n",
}
# Snaps that must differ: (earlier, later).
CHANGES = {
    "save": [("before", "message")],
    "allocation": [],
    "abort": [("before", "message"), ("message", "fatal")],
}
ALLOCATION_TEXTS = ["allocate_far_memory.1"] + ["print_allocation_error_and_exit.%d" % n for n in range(4)]


def listing(folder):
    """Every file under the folder: its path from there and its digest."""
    found = {}
    for root, _, files in os.walk(folder):
        for name in files:
            path = os.path.join(root, name)
            found[os.path.relpath(path, folder)] = record.digest(path)
    return found


def old_version_save(source, out):
    """The save with its version word (at the offset in its word 2) made old."""
    with open(source, "rb") as f:
        data = bytearray(f.read())
    (offset,) = struct.unpack_from("<h", data, 2)
    struct.pack_into("<h", data, offset, OLD_VERSION)
    with open(out, "wb") as f:
        f.write(data)


def story_texts(story):
    with open(os.path.join(story, "text.json"), encoding="utf-8") as f:
        return {entry["id"]: entry["text"] for entry in json.load(f)["texts"]}


def frame(run_dir, snap):
    return record.digest(os.path.join(run_dir, snap + ".indices"))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe", required=True)
    parser.add_argument("--game", default=os.environ.get("REVENGE_GAME", ""))
    args = parser.parse_args()
    if not (args.game and os.path.isdir(args.game)):
        print("fatal_exit: no game folder (REVENGE_GAME); skipped")
        return SKIP
    original_save = os.environ.get("REVENGE_ORIGINAL_SAVE", "")
    problems = []
    with tempfile.TemporaryDirectory(prefix="revenge-fatal-") as work:
        first = os.path.join(work, "first.keys")
        with open(first, "w", encoding="utf-8") as f:
            f.write("@snap:start\n")
        code, _ = record.run(args.exe, args.game, first, os.path.join(work, "first"))
        story = os.path.join(work, "first", "saves", "story")
        if code != 0 or not os.path.isdir(story):
            sys.exit(f"fatal_exit: the story import run failed (exit {code})")
        texts = story_texts(story)
        for name, script_text in CASES.items():
            env_extra = {"REVENGE_TRACE": "1"}
            if name == "save":
                if not os.path.isfile(original_save):
                    print("fatal_exit: save: no REVENGE_ORIGINAL_SAVE; skipped")
                    continue
                old_save = os.path.join(work, "OLDVERSION.DAT")
                old_version_save(original_save, old_save)
                # The script's start-save line names it (record.start_save).
                os.environ["REVENGE_FATAL_SAVE"] = old_save
            script = os.path.join(work, name + ".keys")
            with open(script, "w", encoding="utf-8") as f:
                f.write(script_text)
            run_dir = os.path.join(work, name)
            code, output = record.run(args.exe, args.game, script, run_dir, story=story, env_extra=env_extra)

            expected = {os.path.join("story", path): digest for path, digest in listing(story).items()}
            with open(os.path.join(run_dir, "expected-settings.txt"), "w", encoding="ascii", newline="\n") as f:
                f.write(record.TRACE_SETTINGS)
            expected["settings.txt"] = record.digest(os.path.join(run_dir, "expected-settings.txt"))
            if name == "save":
                expected["SAVEGAME.DAT"] = record.digest(old_save)
            found = listing(os.path.join(run_dir, "saves"))

            if code != 1:
                problems.append(f"{name}: exit {code}, not 1")
            wanted = [token[6:] for token in script_text.split() if token.startswith("@snap:")]
            taken = []
            if os.path.exists(os.path.join(run_dir, "snaps.txt")):
                with open(os.path.join(run_dir, "snaps.txt"), encoding="utf-8") as f:
                    taken = f.read().split()
            if taken != wanted:
                problems.append(f"{name}: snaps {taken}, not {wanted}")
            if os.path.exists(os.path.join(run_dir, "done.txt")):
                problems.append(f"{name}: the script ran to its end; the game didn't exit")
            for earlier, later in CHANGES[name]:
                if earlier in taken and later in taken and frame(run_dir, earlier) == frame(run_dir, later):
                    problems.append(f"{name}: the screen at {later} is the one at {earlier}: no message")
            if name == "allocation":
                for text_id in ALLOCATION_TEXTS:
                    if texts[text_id].strip() not in output:
                        problems.append(f"{name}: {text_id} isn't in the run's output")
                if "65520" not in output:
                    problems.append(f"{name}: the request's size isn't in the run's output")
            if name != "save" and f"fail: {name}" not in output:
                problems.append(f"{name}: no fail item in the run's trace")
            if found != expected:
                changed = sorted(set(found) ^ set(expected)) + sorted(
                    path for path in set(found) & set(expected) if found[path] != expected[path])
                problems.append(f"{name}: the save folder changed: {', '.join(changed)}")
            print(f"fatal_exit: {name}: exit {code}")
    for problem in problems:
        print("fatal_exit: " + problem)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
