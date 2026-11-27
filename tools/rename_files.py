#!/usr/bin/env python3
"""Renames source files and names as docs/RENAMES.tsv lists them.

    tools/rename_files.py apply [--domain DOMAIN ...] [--names]
    tools/rename_files.py check [REV]

docs/RENAMES.tsv has one row per rename, tab-separated: kind, old, new,
segments, note. Lines starting with # are comments.

- kind "file": old and new are paths from the repository's root, in the
  same folder. The file is moved with git mv; every mention of its old file
  name in the tracked text files becomes the new one (includes, CMake lists,
  the two ledgers, tests, docs), a header's include guard follows its new
  path, and the file's first line names the original's segments it holds and
  what it is ("// 2D1B: unit damage"), so the address survives the name.
- kind "name": an identifier; every whole-word use in the tracked text files
  becomes the new one, the trace goldens' object keys included.

The rows stay after they are applied: a reader of the tree the rebuild
started from finds each file and name here.

apply renames the file rows whose old file exists, those under
src/Original/DOMAIN only with --domain, and with --names the name rows too.

check rebuilds commit REV (default HEAD) from its parent: it applies REV's
RENAMES.tsv to a copy of the parent and compares the result with REV's
tree, so a rename commit holds the renames and nothing else. Exits 0 when
they match, 1 otherwise.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TABLE = "docs/RENAMES.tsv"
LEFT_ALONE = {TABLE, ".git-blame-ignore-revs"}


def git(root, *arguments, capture=True):
    return subprocess.run(["git", "-C", root] + list(arguments), check=True, text=True,
                          capture_output=capture).stdout


def read_table(path):
    rows = []
    with open(path, encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            fields = line.split("\t")
            if fields[0] == "kind":
                continue
            if len(fields) != 5 or fields[0] not in ("file", "name"):
                sys.exit(f"rename_files: {path}:{number}: expected kind, old, new, segments, note")
            rows.append(dict(zip(("kind", "old", "new", "segments", "note"), fields)))
    return rows


def text_files(root):
    """The tracked text files a rename may mention, as {relative: text}."""
    files = {}
    for relative in git(root, "ls-files", "-z").split("\0"):
        if not relative or relative in LEFT_ALONE or relative.startswith("extern/"):
            continue
        path = os.path.join(root, relative)
        if not os.path.isfile(path):
            continue
        with open(path, "rb") as f:
            data = f.read()
        if b"\0" in data:
            continue
        try:
            files[relative] = data.decode("utf-8")
        except UnicodeDecodeError:
            continue
    return files


def guard(path):
    """The include guard of a header at path: REVENGE_<FOLDER>_<NAME>_H."""
    folder, name = path.split("/")[-2], os.path.splitext(os.path.basename(path))[0]
    words = re.sub(r"(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", name)
    return f"REVENGE_{folder}_{words}_H".upper()


def head_line(row):
    return f"// {', '.join(row['segments'].split())}: {row['note']}\n"


def apply(root, rows):
    files = text_files(root)
    changes = []
    for row in rows:
        old, new = row["old"], row["new"]
        if row["kind"] == "file":
            if os.path.dirname(old) != os.path.dirname(new):
                sys.exit(f"rename_files: {old} -> {new} changes folder")
            git(root, "mv", old, new)
            files[new] = files.pop(old)
            if not files[new].startswith(head_line(row)):
                files[new] = head_line(row) + files[new]
            changes.append((re.compile(r"(?<![\w.])" + re.escape(os.path.basename(old)) + r"(?![\w])"),
                            os.path.basename(new)))
            if old.endswith(".h"):
                old_guard = os.path.splitext(os.path.basename(old))[0].upper() + "_H"
                changes.append((re.compile(r"\b" + re.escape(old_guard) + r"\b"), guard(new)))
        else:
            changes.append((re.compile(r"\b" + re.escape(old) + r"\b"), new))
    for relative, text in files.items():
        changed = text
        for pattern, replacement in changes:
            changed = pattern.sub(replacement, changed)
        if changed != text or relative in {r["new"] for r in rows if r["kind"] == "file"}:
            with open(os.path.join(root, relative), "w", encoding="utf-8", newline="") as f:
                f.write(changed)


def applicable(root, rows, domains=None, names=True):
    chosen = []
    files = None
    for row in rows:
        if row["kind"] == "file":
            if not os.path.isfile(os.path.join(root, row["old"])):
                continue
            if domains is not None and not any(row["old"].startswith(f"src/Original/{d}/") for d in domains):
                continue
            chosen.append(row)
        elif names:
            files = files if files is not None else text_files(root)
            pattern = re.compile(r"\b" + re.escape(row["old"]) + r"\b")
            if any(pattern.search(text) for text in files.values()):
                chosen.append(row)
    return chosen


def check(revision):
    commit = git(ROOT, "rev-parse", revision).strip()
    with tempfile.TemporaryDirectory() as work:
        copy = os.path.join(work, "parent")
        git(ROOT, "worktree", "add", "--quiet", "--detach", copy, commit + "^")
        try:
            table = git(ROOT, "show", f"{commit}:{TABLE}")
            with open(os.path.join(copy, TABLE), "w", encoding="utf-8", newline="") as f:
                f.write(table)
            rows = applicable(copy, read_table(os.path.join(copy, TABLE)))
            apply(copy, rows)
            git(copy, "add", "-A")
            rebuilt = git(copy, "write-tree").strip()
            expected = git(ROOT, "rev-parse", commit + "^{tree}").strip()
            differences = git(ROOT, "diff", "--stat", rebuilt, expected).strip()
        finally:
            git(ROOT, "worktree", "remove", "--force", copy)
    files = sum(1 for r in rows if r["kind"] == "file")
    names = len(rows) - files
    if differences:
        print(f"rename_files: {commit[:7]} is not its parent with {files} files and {names} names renamed:\n"
              f"{differences}")
        return 1
    print(f"rename_files: {commit[:7]} is its parent with {files} files and {names} names renamed, nothing else")
    return 0


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    applying = sub.add_parser("apply")
    applying.add_argument("--domain", nargs="+")
    applying.add_argument("--names", action="store_true")
    checking = sub.add_parser("check")
    checking.add_argument("revision", nargs="?", default="HEAD")
    arguments = parser.parse_args()
    if arguments.command == "check":
        return check(arguments.revision)
    rows = applicable(ROOT, read_table(os.path.join(ROOT, TABLE)), arguments.domain, arguments.names)
    if not rows:
        print("rename_files: nothing to rename")
        return 0
    apply(ROOT, rows)
    files = sum(1 for r in rows if r["kind"] == "file")
    print(f"rename_files: {files} files and {len(rows) - files} names renamed")
    return 0


if __name__ == "__main__":
    shutil.which("git") or sys.exit("rename_files: git not found")
    sys.exit(main())
