#!/usr/bin/env python3
"""tools/check_ledgers.py fails a ledger that names the wrong file.

Builds a one-procedure tree in a temporary folder: the two ledgers and the
C file. As built it passes; each case then breaks one thing a rename can
leave stale and expects check_ledgers to name it.
"""
import csv
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
CHECK = os.path.join(os.path.dirname(HERE), "tools", "check_ledgers.py")
METHOD_FIELDS = ["Address", "AnnotationName", "CName", "AnnotationFile", "OriginalCFile", "Signature",
                 "Status", "Evidence", "PlatformRedirect", "Uncertainty", "RejectedCandidate"]
CONVERSION_FIELDS = ["Address", "CName", "AsmVerificationStatus", "CurrentOriginalFile", "ProposedDomain",
                     "Wave", "ConversionStatus", "TypedDataDependencies", "TargetOriginalFile",
                     "ValidationEvidence", "Notes"]
SOURCE = "src/Original/Tactical/UnitDamage.cpp"
OTHER = "src/Original/Tactical/UnitState.cpp"


def write_tree(root, defined_in=SOURCE, method_file=SOURCE, conversion_file=SOURCE, conversion_name="ApplyDamage"):
    os.makedirs(os.path.join(root, "docs"))
    os.makedirs(os.path.join(root, "src", "Original", "Tactical"))
    for relative in (SOURCE, OTHER):
        with open(os.path.join(root, relative), "w") as f:
            if relative == defined_in:
                f.write("// 2D1B:0010\nvoid ApplyDamage(int amount)\n{\n    (void) amount;\n}\n")
            else:
                f.write("// 2D1B:0100\nint UnitState;\n")
    with open(os.path.join(root, "docs", "METHOD_TABLE.csv"), "w", newline="") as f:
        writer = csv.DictWriter(f, METHOD_FIELDS, quoting=csv.QUOTE_ALL)
        writer.writeheader()
        writer.writerow({"Address": "2D1B:0010", "AnnotationName": "ApplyDamage", "CName": "ApplyDamage",
                         "AnnotationFile": "evidence/annotations/REVENGE_2D1B.c", "OriginalCFile": method_file,
                         "Status": "Verified"})
    with open(os.path.join(root, "docs", "SOURCE_CONVERSION_TABLE.csv"), "w", newline="") as f:
        writer = csv.DictWriter(f, CONVERSION_FIELDS, quoting=csv.QUOTE_ALL)
        writer.writeheader()
        writer.writerow({"Address": "2D1B:0010", "CName": conversion_name, "CurrentOriginalFile": conversion_file,
                         "TargetOriginalFile": conversion_file})


def run(**case):
    with tempfile.TemporaryDirectory() as root:
        write_tree(root, **case)
        checked = subprocess.run([sys.executable, "-I", CHECK, "--root", root], capture_output=True, text=True)
        return checked.returncode, checked.stdout + checked.stderr


def main():
    failures = 0
    cases = [
        ("as built", {}, 0, "valid"),
        ("defined in another file", {"defined_in": OTHER}, 1, "ApplyDamage isn't defined in " + SOURCE),
        ("method table names another file", {"method_file": OTHER}, 1, "ApplyDamage isn't defined in " + OTHER),
        ("conversion table names another file", {"conversion_file": OTHER}, 1,
         "CurrentOriginalFile '" + OTHER + "' isn't METHOD_TABLE's"),
        ("conversion table names another function", {"conversion_name": "ApplyHeat"}, 1,
         "is ApplyHeat in SOURCE_CONVERSION_TABLE"),
    ]
    for what, case, code, expected in cases:
        returncode, output = run(**case)
        if returncode != code or expected not in output:
            print(f"FAIL: {what}: exit {returncode}, expected {code} and {expected!r}:\n{output}")
            failures += 1
    print("check_ledgers rules: ok" if not failures else f"check_ledgers rules: {failures} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
