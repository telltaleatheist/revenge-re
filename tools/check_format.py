#!/usr/bin/env python3
"""Checks that every C++ source and header is laid out as .clang-format says.

    tools/check_format.py [--fix]

Runs clang-format 21 over the tracked .cpp and .h files, less those in
.clang-format-ignore (the libraries in extern/ and the files tools write).
Other major versions lay some lines out differently, so with any other
version, or none, it skips. clang-format is CLANG_FORMAT, else clang-format
on the PATH, else Xcode's (xcrun --find clang-format); `pip install
clang-format==21.*` gives LLVM's, which formats the tree as Xcode's 21 does.

--fix rewrites the files in place instead. Not a ctest test: it is run with
each slice. Exits 0 when every file is formatted, 1 when one isn't, 77
without clang-format 21.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAJOR = 21
SKIP = 77
BATCH = 50


def find_clang_format():
    candidates = [os.environ.get("CLANG_FORMAT", ""), shutil.which("clang-format") or ""]
    if sys.platform == "darwin" and shutil.which("xcrun"):
        found = subprocess.run(["xcrun", "--find", "clang-format"], capture_output=True, text=True)
        candidates.append(found.stdout.strip() if found.returncode == 0 else "")
    for candidate in candidates:
        if not candidate:
            continue
        try:
            version = subprocess.run([candidate, "--version"], capture_output=True, text=True).stdout
        except OSError:
            continue
        match = re.search(r"clang-format version (\d+)\.", version)
        if match and int(match.group(1)) == MAJOR:
            return candidate, version.strip()
    return None, None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fix", action="store_true")
    arguments = parser.parse_args()

    clang_format, version = find_clang_format()
    if not clang_format:
        print(f"skipped: no clang-format {MAJOR} (set CLANG_FORMAT, or pip install clang-format=={MAJOR}.*)")
        return SKIP

    listed = subprocess.run(["git", "-C", ROOT, "ls-files", "*.cpp", "*.h"], capture_output=True, text=True,
                            check=True).stdout.split()
    files = [name for name in listed if not name.startswith("extern/")]
    mode = ["-i"] if arguments.fix else ["--dry-run", "-Werror"]
    unformatted = set()
    for start in range(0, len(files), BATCH):
        checked = subprocess.run([clang_format] + mode + files[start:start + BATCH], cwd=ROOT,
                                 capture_output=True, text=True)
        if checked.returncode != 0 and not arguments.fix:
            unformatted.update(re.findall(r"^(\S+?):\d+:\d+: (?:error|warning):", checked.stderr, re.M))
        elif checked.returncode != 0:
            sys.stderr.write(checked.stderr)
            return 1

    if arguments.fix:
        print(f"check_format: {len(files)} files formatted ({version})")
        return 0
    for name in sorted(unformatted):
        print(f"not formatted: {os.path.relpath(os.path.join(ROOT, name), ROOT)}")
    print(f"check_format: {len(files) - len(unformatted)} of {len(files)} files formatted ({version})")
    return 1 if unformatted else 0


if __name__ == "__main__":
    sys.exit(main())
