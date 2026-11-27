#!/usr/bin/env python3
"""Plays every golden script with no program file in the game folder.

    tools/trace/exe_free.py --exe BUILD/Revenge [--game GAME_DIR] [--work DIR]

One run with the game folder as it is imports the story (revenge/story.h).
Then a folder of links to every file of the game but its program files (.EXE,
any case) stands for the install, and each script with a golden trace runs
there with that story in its save folder: each must match its golden and none
may import the story again. The port opens no program file at play (a fault
stops it if it tries), so the copy is needed once, for the import, and never
after. GAME_DIR defaults to the REVENGE_GAME variable; without one, exit 77
(ctest's skip). Exits 0 when every script matches.
"""
import argparse
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import check  # noqa: E402
import record  # noqa: E402

SKIP = 77
IMPORTED = "imported the story file"


def linked_install(game, folder):
    """A folder of links to the game's files, its program files left out."""
    os.makedirs(folder)
    left_out = []
    for name in sorted(os.listdir(game)):
        if name.upper().endswith(".EXE"):
            left_out.append(name)
            continue
        os.symlink(os.path.join(os.path.abspath(game), name), os.path.join(folder, name))
    return left_out


def exe_free(args, work):
    names = sorted(n[:-6] for n in os.listdir(check.TRACES) if n.endswith(".jsonl"))
    first = os.path.join(check.SCRIPTS, "title.keys")
    seed = os.path.join(work, "import")
    code, output = record.run(args.exe, args.game, first, seed)
    story = os.path.join(seed, "saves", "story")
    if code != 0 or IMPORTED not in output or not os.path.isdir(story):
        print(f"exe_free: the run with the copy didn't import the story (exit {code})")
        return 1
    install = os.path.join(work, "install")
    try:
        left_out = linked_install(args.game, install)
    except OSError as e:
        # Links need a right Windows gives only some users; never a copy.
        print(f"exe_free: can't link the game's files here ({e}); skipped")
        return SKIP
    if not left_out:
        print("exe_free: the game folder holds no program file to leave out")
        return 1
    print(f"exe_free: the story imported; playing without {', '.join(left_out)}")
    problems = []
    for name in names:
        script = os.path.join(check.SCRIPTS, name + ".keys")
        wanted = record.start_save(script)
        if wanted and wanted[1] is None:
            print(f"exe_free: {name}: skipped, no save in {wanted[0]}")
            continue
        run = os.path.join(work, name)
        code, output = record.run(args.exe, install, script, run, story=story)
        if code != 0 or not os.path.exists(os.path.join(run, "done.txt")):
            last = [line for line in output.splitlines() if line.startswith(("harness:", "fault:"))]
            problems.append(f"{name}: didn't run to its end (exit {code})" + (f": {last[-1]}" if last else ""))
            continue
        if IMPORTED in output:
            problems.append(f"{name}: imported the story again")
        found = check.compare(name, check.load(os.path.join(check.TRACES, name + ".jsonl")),
                              record.trace(script, run))
        print(f"exe_free: {name}: " + ("matches" if not found else f"{len(found)} difference(s)"))
        problems += found
    for problem in problems:
        print(problem)
    return 1 if problems else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--exe", required=True)
    parser.add_argument("--game", default=os.environ.get("REVENGE_GAME", ""))
    parser.add_argument("--work")
    args = parser.parse_args()
    if not (args.game and os.path.isdir(args.game)):
        print("exe_free: no game folder (REVENGE_GAME); skipped")
        return SKIP
    with tempfile.TemporaryDirectory(prefix="revenge-exe-free-") as temporary:
        return exe_free(args, args.work or temporary)


if __name__ == "__main__":
    sys.exit(main())
