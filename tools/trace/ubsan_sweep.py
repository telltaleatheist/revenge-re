#!/usr/bin/env python3
"""Plays every trace script on a build with the undefined-behaviour sanitizer
and lists each place it flags: an index past an array, a misaligned or null
access, an overflow, a shift past the width.

    tools/trace/ubsan_sweep.py --build DIR [--game GAME_DIR] [--source DIR]
                               [--only NAME ...] [-- CMAKE_ARGUMENT ...]

Configures DIR (Clang or GCC, RelWithDebInfo) with
-fsanitize=bounds,undefined, recovering so one run reports every place,
builds the Revenge target and records every script with a golden through
check.py: the goldens and the campaign set. Arguments after "--" go to cmake
(the soundfont, say). The scripts' start saves come from the environment as
for check.py (REVENGE_ORIGINAL_SAVE, REVENGE_SAVE_STAGE_04, ...).

Each place is printed once, with how many times it was hit and the first
report's stack; the logs stay in DIR/ubsan. A place is either an original's
over-read the port must reproduce from the original's bytes (the title's
fade colour, the experience mask) or a port bug. Not a ctest test: it is run
at the end of each phase. Exits 0 when nothing is flagged and every script
matches its golden, 1 otherwise, 77 without a copy of the game.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SKIP = 77
FLAGS = "-fsanitize=bounds,undefined -fsanitize-recover=all -fno-omit-frame-pointer"
REPORT = re.compile(r"^(?P<place>\S+:\d+:\d+): runtime error: (?P<what>.*)$")


def configure_and_build(build, source, extra):
    subprocess.run(
        ["cmake", "-S", source, "-B", build, "-G", "Ninja",
         "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
         f"-DCMAKE_C_FLAGS={FLAGS}", f"-DCMAKE_CXX_FLAGS={FLAGS}",
         f"-DCMAKE_EXE_LINKER_FLAGS={FLAGS}"] + extra,
        check=True, stdout=subprocess.DEVNULL)
    subprocess.run(["cmake", "--build", build, "--target", "Revenge", "-j4"], check=True,
                   stdout=subprocess.DEVNULL)
    return os.path.join(build, "Revenge.exe" if os.name == "nt" else "Revenge")


def reports(logs):
    """{place: [count, what, first report's lines]} from every log in logs."""
    found = {}
    for name in sorted(os.listdir(logs)):
        with open(os.path.join(logs, name), encoding="utf-8", errors="replace") as log:
            lines = log.read().splitlines()
        for index, line in enumerate(lines):
            match = REPORT.match(line)
            if not match:
                continue
            place = os.path.relpath(match["place"], ROOT) if os.path.isabs(match["place"]) else match["place"]
            if place in found:
                found[place][0] += 1
                continue
            stack = []
            for following in lines[index + 1:]:
                if REPORT.match(following) or not following.strip():
                    break
                stack.append(following)
            found[place] = [1, match["what"], stack[:8]]
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", required=True)
    parser.add_argument("--game", default=os.environ.get("REVENGE_GAME", ""))
    parser.add_argument("--source", default=ROOT)
    parser.add_argument("--only", nargs="*")
    parser.add_argument("cmake", nargs="*")
    arguments = parser.parse_args()
    if not arguments.game:
        print("skipped: set REVENGE_GAME or --game to a copy of the game")
        return SKIP

    build = os.path.abspath(arguments.build)
    exe = configure_and_build(build, arguments.source, arguments.cmake)
    logs = os.path.join(build, "ubsan")
    os.makedirs(logs, exist_ok=True)
    for name in os.listdir(logs):
        os.remove(os.path.join(logs, name))

    environment = dict(os.environ)
    environment["UBSAN_OPTIONS"] = (
        f"print_stacktrace=1:halt_on_error=0:log_path={os.path.join(logs, 'ubsan')}")
    command = [sys.executable, "-I", os.path.join(HERE, "check.py"), "--exe", exe,
               "--game", arguments.game, "--work", os.path.join(build, "ubsan_work")]
    if arguments.only:
        command += ["--only"] + arguments.only
    checked = subprocess.run(command, env=environment)

    found = reports(logs)
    by_kind = collections.Counter(what.split(" ")[0] for _, what, _ in found.values())
    for place, (count, what, stack) in sorted(found.items()):
        print(f"{place}: {what} ({count} times)")
        for line in stack:
            print(f"    {line.strip()}")
    print(f"ubsan_sweep: {len(found)} places flagged"
          + (f" ({', '.join(f'{k} {n}' for k, n in sorted(by_kind.items()))})" if found else "")
          + f"; the scripts {'match' if checked.returncode == 0 else 'do not all match'} their goldens")
    return 0 if not found and checked.returncode == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
