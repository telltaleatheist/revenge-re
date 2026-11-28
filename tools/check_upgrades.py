#!/usr/bin/env python3
"""The UPGRADES switches against their gates (docs/ORIGINAL_BUGS.md, Switches).

Each switch in src/Support/upgrades.cpp's table must have:
  - a gate in src/: a comment `FIX (upgrade_<key>)` with a test of
    Upgrades_IsOn(UPGRADE_<KEY>) within the next 12 lines;
  - a row `upgrade_<key>` in ORIGINAL_BUGS.md's Switches table, whose
    files (in its last column) are there;
  - a label and help in data/ui/upgrades.json;
  - a trace script turning it on (`# upgrades: ... <key> ...`).
And every gate, row, JSON entry and `# upgrades:` line names a switch in the
table. Prints each fault and exits 1, or prints a summary and exits 0.
"""
import argparse
import json
import os
import re
import sys

KEYS_TABLE = re.compile(r"UpgradeKeys\[[^\]]*\]\s*=\s*\{([^}]*)\}", re.S)
GATE = re.compile(r"FIX \(upgrade_(\w+)\)")
ROW = re.compile(r"^\|\s*`upgrade_(\w+)`\s*\|.*\|([^|]*)\|\s*$", re.M)
TRACE = re.compile(r"^#\s*upgrades:\s*([\w ,]+?)\s*$", re.M)
GATE_REACH = 12


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", default=os.path.join(os.path.dirname(__file__), os.pardir))
    root = parser.parse_args().root
    faults = []

    table = KEYS_TABLE.search(read(os.path.join(root, "src", "Support", "upgrades.cpp")))
    if table is None:
        print("check_upgrades: src/Support/upgrades.cpp has no UpgradeKeys table")
        return 1
    keys = re.findall(r'"(\w+)"', table.group(1))

    gates = {}
    for folder, _, files in os.walk(os.path.join(root, "src")):
        for name in sorted(files):
            if not name.endswith((".cpp", ".c", ".h")):
                continue
            path = os.path.join(folder, name)
            lines = read(path).splitlines()
            for number, line in enumerate(lines):
                for key in GATE.findall(line):
                    where = "%s:%d" % (os.path.relpath(path, root), number + 1)
                    if key not in keys:
                        faults.append("%s: FIX (upgrade_%s) names no switch" % (where, key))
                        continue
                    test = "Upgrades_IsOn(UPGRADE_%s)" % key.upper()
                    if not any(test in later for later in lines[number:number + GATE_REACH]):
                        faults.append("%s: FIX (upgrade_%s) has no %s after it" % (where, key, test))
                    gates.setdefault(key, []).append(where)

    rows = {}
    for key, where in ROW.findall(read(os.path.join(root, "docs", "ORIGINAL_BUGS.md"))):
        if key not in keys:
            faults.append("ORIGINAL_BUGS.md: upgrade_%s names no switch" % key)
        rows[key] = where
        for path in re.findall(r"`([\w./]+\.(?:cpp|h|keys))`", where):
            candidates = [os.path.join(root, path), os.path.join(root, "src", "Original", path)]
            if not any(os.path.exists(c) for c in candidates):
                faults.append("ORIGINAL_BUGS.md: upgrade_%s names %s, which is not there" % (key, path))

    words = json.loads(read(os.path.join(root, "data", "ui", "upgrades.json"))).get("switches", {})
    for key in words:
        if key not in keys:
            faults.append("data/ui/upgrades.json: switches.%s names no switch" % key)

    traced = {}
    scripts = os.path.join(root, "tools", "trace", "scripts")
    for name in sorted(os.listdir(scripts)):
        if name.endswith(".keys"):
            for line in TRACE.findall(read(os.path.join(scripts, name))):
                for key in re.split(r"[ ,]+", line.strip()):
                    if key not in keys:
                        faults.append("tools/trace/scripts/%s: upgrades: %s names no switch" % (name, key))
                    traced.setdefault(key, []).append(name)

    for key in keys:
        if key not in gates:
            faults.append("upgrade_%s: no FIX (upgrade_%s) gate in src/" % (key, key))
        if key not in rows:
            faults.append("upgrade_%s: no row in ORIGINAL_BUGS.md's Switches table" % key)
        if key not in words:
            faults.append("upgrade_%s: no entry in data/ui/upgrades.json" % key)
        if key not in traced:
            faults.append("upgrade_%s: no trace script with `# upgrades: %s`" % (key, key))

    for fault in faults:
        print("check_upgrades: " + fault)
    if faults:
        return 1
    for key in keys:
        print("check_upgrades: upgrade_%s: gates %s; traces %s" % (key, ", ".join(gates[key]), ", ".join(traced[key])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
