#!/usr/bin/env python3
"""The host's quit (the window's close button, the system's quit, SIGTERM)
ends the game from anywhere, as the game's own quit ends it, and writes nothing.

    tools/trace/quit.py --exe BUILD/Revenge [--game GAME_DIR]

A script's `quit` item asks it as the host's quit is asked. It is asked at
the title, in the main menu and in the middle of the first battle (two steps
after the snap battle_steps.keys takes at step 4). Each run must end with
exit code 0 through the game's shutdown ("quit: asked" in the run's trace),
after the script's snaps and before the harness's own end (no done.txt),
and leave the save folder as it was: the imported story and settings.txt,
nothing added, nothing changed. GAME_DIR defaults to the REVENGE_GAME
variable; without one, exit 77 (ctest's skip). Exits 0 when all three runs
hold.
"""
import argparse
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import record  # noqa: E402

SKIP = 77


def battle_tokens():
    """battle_steps.keys up to its snap at step 4."""
    tokens = []
    for name, group in record.tokens_by_snap(os.path.join(HERE, "scripts", "battle_steps.keys")):
        tokens += group
        if name == "step04":
            return tokens
    sys.exit("quit: battle_steps.keys has no snap step04")


def listing(folder):
    """Every file under the folder: its path from there and its digest."""
    found = {}
    for root, _, files in os.walk(folder):
        for name in files:
            path = os.path.join(root, name)
            found[os.path.relpath(path, folder)] = record.digest(path)
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe", required=True)
    parser.add_argument("--game", default=os.environ.get("REVENGE_GAME", ""))
    args = parser.parse_args()
    if not (args.game and os.path.isdir(args.game)):
        print("quit: no game folder (REVENGE_GAME); skipped")
        return SKIP
    cases = {
        "title": ["idle:1000", "@snap:title", "quit"],
        "menu": ["idle:1000", "Return", "idle:50", "@snap:menu", "quit"],
        "battle": battle_tokens() + ["steps:2", "quit"],
    }
    problems = []
    with tempfile.TemporaryDirectory(prefix="revenge-quit-") as work:
        # The story, imported once, as a player's save folder holds it.
        first = os.path.join(work, "first.keys")
        with open(first, "w", encoding="utf-8") as f:
            f.write("@snap:start\n")
        code, output = record.run(args.exe, args.game, first, os.path.join(work, "first"))
        story = os.path.join(work, "first", "saves", "story")
        if code != 0 or not os.path.isdir(story):
            sys.exit(f"quit: the story import run failed (exit {code})")
        for name, tokens in cases.items():
            script = os.path.join(work, name + ".keys")
            with open(script, "w", encoding="utf-8") as f:
                f.write(" ".join(tokens) + "\n")
            run_dir = os.path.join(work, name)
            code, output = record.run(args.exe, args.game, script, run_dir, story=story,
                                      env_extra={"REVENGE_TRACE": "1"})
            saves = os.path.join(run_dir, "saves")
            expected = {os.path.join("story", path): digest for path, digest in listing(story).items()}
            with open(os.path.join(run_dir, "expected-settings.txt"), "w", encoding="ascii", newline="\n") as f:
                f.write(record.TRACE_SETTINGS)
            expected["settings.txt"] = record.digest(os.path.join(run_dir, "expected-settings.txt"))
            found = listing(saves)
            if code != 0:
                problems.append(f"{name}: exit {code}")
            if "quit: asked" not in output:
                problems.append(f"{name}: no quit in the run's trace")
            wanted = [token[6:] for token in tokens if token.startswith("@snap:")]
            taken = []
            if os.path.exists(os.path.join(run_dir, "snaps.txt")):
                with open(os.path.join(run_dir, "snaps.txt"), encoding="utf-8") as f:
                    taken = f.read().split()
            if taken != wanted:
                problems.append(f"{name}: snaps {taken}, not {wanted}: the quit came early")
            if os.path.exists(os.path.join(run_dir, "done.txt")):
                problems.append(f"{name}: the script ran to its end; the quit didn't end it")
            if found != expected:
                changed = sorted(set(found) ^ set(expected)) + sorted(
                    path for path in set(found) & set(expected) if found[path] != expected[path])
                problems.append(f"{name}: the save folder changed: {', '.join(changed)}")
            print(f"quit: {name}: exit {code}")
    for problem in problems:
        print("quit: " + problem)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
