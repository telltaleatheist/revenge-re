"""The launcher contract on the built program (re-setup-kit DESIGN.md): `--status` prints one JSON line, exits 0, opens
no window and writes nothing; the setup served to the launcher (tools/package/served_setup.py); `--setup` with no copy
and no player at the run exits 2 with {"error": ...} and opens no picker; with a copy (REVENGE_GAME) a hidden --setup
takes the test's pick (REVENGE_SETUP_PICK) and the next --status finds it ready. The launcher's start options are
checked for their words. A release's stamp is in the stamped program only. Every run has its own player folder
(REVENGE_SAVE_DIRECTORY) in the work folder; nothing of the game is kept. Prints names and counts only.

    status_setup_test.py --program build/Revenge --stamped build/revenge_stamped --work build/status_setup_work
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "package"))
import served_setup  # noqa: E402

failures = 0


def check(ok, what):
    global failures
    print(("ok   " if ok else "FAIL ") + what)
    if not ok:
        failures += 1


def run(program, args, env):
    return subprocess.run([str(program)] + args, env=env, input="", capture_output=True, text=True, timeout=120)


def stamps(program):
    return set(re.findall(rb"MW3-PROGRAM-VERSION:([^\0]*)\0", Path(program).read_bytes()))


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--program", required=True)
    p.add_argument("--stamped")
    p.add_argument("--work", required=True)
    a = p.parse_args()
    work = Path(a.work)
    shutil.rmtree(str(work), ignore_errors=True)
    work.mkdir(parents=True)
    game = os.environ.get("REVENGE_GAME") or None
    # A copy of the program alone, as a package has it: the build's REVENGE.CFG beside it would name the game folder.
    (work / "program").mkdir()
    program = work / "program" / Path(a.program).name
    shutil.copy2(a.program, str(program))
    a.program = str(program)

    skipped = []
    for passed, what in served_setup.checks(a.program, os.environ, work / "served", game):
        if passed is None:
            skipped.append(what)
        else:
            check(passed, what)

    env = served_setup.asking_env(os.environ, work / "alone")
    r = run(a.program, ["--setup"], env)
    try:
        error = json.loads(r.stdout.strip().splitlines()[-1]).get("error", "") if r.stdout.strip() else ""
    except ValueError:
        error = ""
    check(r.returncode == 2 and "No copy of" in error, "--setup, nothing set up and no player: exit 2 and why (%d)" % r.returncode)
    r = run(a.program, [], env)
    check(r.returncode == 1 and "No copy of" in r.stderr, "a start, nothing set up and no player: exit 1 and why (%d)" % r.returncode)
    check(not (work / "alone" / "install").exists(), "break: nothing installed without a pick")
    for bad in (["--window"], ["--window", "100x100"], ["--window", "800by600"], ["--write-dir"], ["--frob"], ["--setup", "x"]):
        r = run(a.program, bad, env)
        check(r.returncode == 2 and "Usage:" in r.stderr, "break: %s: the usage, exit 2 (%d)" % (" ".join(bad), r.returncode))
    r = run(a.program, ["--help"], env)
    check(r.returncode == 0 and "Usage:" in r.stdout and "--ready-file" in r.stdout, "--help: the usage with the start options")

    if game:
        picked = dict(env, REVENGE_SETUP_PICK=game, REVENGE_SAVE_DIRECTORY=str(work / "picked"))
        r = run(a.program, ["--setup"], picked)
        install = work / "picked" / "install"
        check(r.returncode == 0 and (install / "REVENGE.EXE").is_file() and
              served_setup.same(served_setup.remembered(work / "picked"), install), "--setup: the test's pick copied in and remembered")
        r = run(a.program, ["--status"], dict(env, REVENGE_SAVE_DIRECTORY=str(work / "picked")))
        st = json.loads(r.stdout) if r.returncode == 0 else {}
        check(st.get("files", {}).get("state") == "ready", "--status after --setup: ready")
    else:
        skipped += ["--setup: the test's pick", "--status after --setup"]

    check(stamps(a.program) == set(), "the build carries no release stamp")
    if a.stamped:
        check(stamps(a.stamped) == {b"chr-v0.0.1"}, "the stamped program carries its tag once")
    print("%d failed, %d skipped%s" % (failures, len(skipped), " (no REVENGE_GAME):" if skipped else ""))
    for what in skipped:
        print("  skipped: " + what)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
