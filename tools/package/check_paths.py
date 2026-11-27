#!/usr/bin/env python3
"""Fails when any file under a package folder carries a local path or the
building user's name (docs/RELEASING.md): every byte of every file, as ASCII
and as UTF-16, so a library's built-in strings count too. The public GitHub
addresses (github.com/<user>, api.github.com/repos/<user>, and
raw.githubusercontent.com/<user>, where the setup's catalog has its copy) are
the one allowed use of the name; a compiler may store such an address in
overlapping pieces, so the name may follow the address and a first part of
itself. Fails
too on a file of the original game by its name, or of what the first start
imports from it (the player's story/ folder), so no package can carry one.

    check_paths.py <folder> [extra name ...]   (tools/build_package.py)
"""
import getpass
import os
import re
import sys

root = sys.argv[1]
names = {n for n in [getpass.getuser(), os.environ.get("SUDO_USER", ""),
                     os.path.basename(os.path.expanduser("~"))] + sys.argv[2:] if len(n) >= 4}
words = [b"/Volumes/", b"/Users/", b"/home/", b"C:\\Users\\", b"C:/Users/"] + [n.encode() for n in sorted(names)]
pat = re.compile(b"|".join(re.escape(w) for w in words))
pat16 = re.compile(b"|".join(re.escape(w.decode().encode("utf-16-le")) for w in words))
allowed = re.compile(rb"(?:github\.com/(?:repos/)?|raw\.githubusercontent\.com/)([A-Za-z0-9-]{0,16})$", re.I)

# The game's files by name: its programs, the kinds its data files come in (src/Setup/GameFilesTable.cpp), its
# notes and the player's own files; and the story import's (text.json, import.json; media/ and data/ hold .bin).
game_file = re.compile(r"(revenge|setup)\.exe|.+\.(bin|cmp|col|cps|dat|fnt|icn|map|mus)"
                       r"|(msgs|help|windows)\.txt|(media|script)\.cfg|(text|import)\.json", re.I)

bad = []
for d, _, files in os.walk(root):
    for f in files:
        p = os.path.join(d, f)
        if game_file.fullmatch(f):
            bad.append(f"{os.path.relpath(p, root)}: a file of the original game")
            continue
        if os.path.islink(p):
            continue
        with open(p, "rb") as fh:
            data = fh.read()
        for m in pat.finditer(data):
            a = allowed.search(data[max(0, m.start() - 40):m.start()])
            if a and m.group().lower().startswith(a.group(1).lower()):
                continue
            ctx = data[max(0, m.start() - 30):m.end() + 50]
            bad.append(f"{os.path.relpath(p, root)}: {ctx.decode('latin-1')!r}")
            break
        else:
            if pat16.search(data):
                bad.append(f"{os.path.relpath(p, root)}: (UTF-16)")
for b in bad:
    print("not allowed in the package:", b)
if bad:
    sys.exit(1)
print(f"check_paths: no local path, user name or game file in {root}")
