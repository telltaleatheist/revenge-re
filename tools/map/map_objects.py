"""The port's data objects and the map of where they sit in the original.

The objects are the data the port's headers declare extern (include/, and the
headers under src/ but for the SDL host's); tools/map/data_map.txt has a line
for each (README.md). check_map.py and make_map_dump.py read both through
this module.
"""
import os
import re
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MAP = os.path.join(ROOT, "tools", "map", "data_map.txt")

EXTERN = re.compile(
    r"^[ \t]*extern\s+([^;{}()#]*?)\b([A-Za-z_]\w*)\s*((?:\[[^;\]]*\]\s*)*);", re.M)


def headers():
    """The headers whose externs are the game's data, repository-relative."""
    try:
        listed = subprocess.run(["git", "ls-files", "include", "src"], cwd=ROOT,
                                capture_output=True, text=True, check=True).stdout.split()
    except (OSError, subprocess.CalledProcessError):
        listed = []
    if not listed:
        # A copy without git: every header under the two folders.
        for top in ("include", "src"):
            for folder, _, names in os.walk(os.path.join(ROOT, top)):
                listed += [os.path.relpath(os.path.join(folder, n), ROOT).replace(os.sep, "/") for n in names]
        listed.sort()
    return [f for f in listed if f.endswith(".h") and not f.startswith("src/SDL/")]


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def externs():
    """[(name, type, dimensions, header)] in header order, each name once."""
    seen = {}
    for header in headers():
        with open(os.path.join(ROOT, header), encoding="utf-8") as f:
            text = strip_comments(f.read())
        for m in EXTERN.finditer(text):
            name = m.group(2)
            if name not in seen:
                seen[name] = (name, " ".join(m.group(1).split()), re.sub(r"\s+", "", m.group(3)), header)
    return list(seen.values())


def linear(place):
    """SEG:OFF as a linear address of the image loaded at 0800h."""
    segment, offset = place.split(":")
    return (int(segment, 16) - 0x0800) * 16 + int(offset, 16)


def read_map(path=MAP):
    """(entries by name, overlap pairs, problems). An entry is the line's words."""
    entries, overlaps, problems = {}, set(), []
    with open(path, encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            words = line.split()
            if not words or words[0].startswith("#"):
                continue
            where = f"data_map.txt:{number}"
            kind = words[0]
            if kind == "overlap" and len(words) >= 3:
                overlaps.add(frozenset(words[1:3]))
                continue
            if kind == "place" and len(words) == 5 and re.fullmatch(r"[0-9A-F]{4}:[0-9A-F]{4}", words[2]) \
                    and re.fullmatch(r"[0-9A-F]+", words[3]) \
                    and words[4] in ("table", "import", "annotation", "run", "procedure"):
                pass
            elif kind in ("host", "unplaced") and len(words) >= 4 and re.fullmatch(r"[0-9A-F]+", words[2]):
                pass
            else:
                problems.append(f"{where}: not a map line")
                continue
            if words[1] in entries:
                problems.append(f"{where}: {words[1]} is listed twice")
            entries[words[1]] = words
    return entries, overlaps, problems
