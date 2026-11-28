#!/usr/bin/env python3
"""tools/check_upgrades.py fails a switch that lost its gate, row, words or trace.

Builds a one-switch tree in a temporary folder: the switches' table, a gate,
the ORIGINAL_BUGS.md row, data/ui/upgrades.json and a trace script. As built
it passes; each case then breaks one thing and expects check_upgrades to name
it.
"""
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
CHECK = os.path.join(os.path.dirname(HERE), "tools", "check_upgrades.py")


def write(root, relative, text):
    path = os.path.join(root, relative)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def write_tree(root, keys=("walk_on",), gate="walk_on", gated=True, row="walk_on", words=("walk_on",),
               traced="walk_on", where="Tactical/Walk.cpp"):
    write(root, "src/Support/upgrades.cpp",
          "static const char *const UpgradeKeys[UPGRADE_COUNT] = { %s };\n"
          % ", ".join('"%s"' % key for key in keys))
    test = "    if (Upgrades_IsOn(UPGRADE_%s))\n        Walk();\n" % gate.upper() if gated else "    Walk();\n"
    write(root, "src/Original/Tactical/Walk.cpp",
          "// 2D1B:0010\nvoid Step(void)\n{\n    // FIX (upgrade_%s): ours.\n%s}\n" % (gate, test))
    write(root, "docs/ORIGINAL_BUGS.md",
          "## Switches\n\n| Key | What it changes | Where |\n| --- | --- | --- |\n"
          "| `upgrade_%s` | Walks. | `%s` |\n" % (row, where))
    write(root, "data/ui/upgrades.json",
          json.dumps({"switches": {key: {"label": "L", "help": "H"} for key in words}}))
    write(root, "tools/trace/scripts/walk.keys", "# upgrades: %s\nidle:10\n" % traced)


def run(**tree):
    with tempfile.TemporaryDirectory() as root:
        write_tree(root, **tree)
        done = subprocess.run([sys.executable, "-I", CHECK, "--root", root], capture_output=True, text=True)
        return done.returncode, done.stdout


def expect(name, want, **tree):
    code, out = run(**tree)
    if code != 1 or want not in out:
        print("check_upgrades_test: %s: wanted exit 1 naming %r, got exit %d:\n%s" % (name, want, code, out))
        return 1
    return 0


def main():
    code, out = run()
    if code != 0:
        print("check_upgrades_test: the whole tree fails:\n" + out)
        return 1
    failures = 0
    failures += expect("no gate", "no FIX (upgrade_walk_on) gate", gate="other")
    failures += expect("gate without its test", "has no Upgrades_IsOn(UPGRADE_WALK_ON)", gated=False)
    failures += expect("no row", "no row in ORIGINAL_BUGS.md", row="other")
    failures += expect("no words", "no entry in data/ui/upgrades.json", words=())
    failures += expect("no trace", "no trace script", traced="other")
    failures += expect("a gate for no switch", "FIX (upgrade_gone) names no switch", gate="gone")
    failures += expect("a row's file gone", "names Tactical/Run.cpp, which is not there", where="Tactical/Run.cpp")
    if failures == 0:
        print("check_upgrades_test: 7 cases")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
