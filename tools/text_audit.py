#!/usr/bin/env python3
"""Checks that no text or data of the game's own is in the tree.

    tools/text_audit.py [GAME_DIR] [--root DIR] [--list]

GAME_DIR is the player's copy of the game (default: the REVENGE_GAME
variable). DIR is the tree to check (default: this tool's repository); its
files are the ones git tracks there, or every file when it is not a git
tree. --list prints one line per hit even when the allow list keeps it.

What is searched for, from the player's copy:
- REVENGE.EXE: every run of printable characters in its load image, and the
  same image with the game's digram text compression expanded (the two
  dictionaries at DS:05AA and DS:05BA), split at NULs and line breaks;
- every other file of the copy: its runs of printable characters.

What counts as the game's words is wording: at least MIN_WORDS words, or two
words in at least PHRASE_LENGTH characters. Single words and two-word names
(a weapon, a place, a button), which our identifiers, keys and prose share by
meaning, match ordinary text too often to search for; the keyed hashes of the
import and rule tables keep those out of the tree.

Four kinds of hit, each named by the file and line in the tree, the game
file and the offset of the match, and its length. Never the text.
- wording: a line of the game's that is wording, anywhere in a tracked or
  new (not ignored) file, case folded and as whole words;
- piece: a C string literal (adjacent literals joined) that is wording and
  occurs anywhere in any game file, case-blind: raw, or in REVENGE.EXE's
  text expanded. A piece of a longer line is caught too;
- literal: a C string literal that is wording and the whole of a
  NUL-terminated string of REVENGE.EXE (expanded or not), line breaks and
  all;
- bytes: a C array of hex bytes holding a run of 16 bytes of REVENGE.EXE's
  image, with at least four different values among them.
File names (the code opens the install's files by name) and printf formats
are not counted.

tools/text_audit_allow.txt lists the hits reviewed and kept: a path (or *),
the kind, the game file and offset, and the reason in our own words. A kept
hit is counted but doesn't fail; an entry that no longer hits is reported.

Exits 0 when the tree is clean, 1 on a hit, 77 (ctest's skip) without a copy
of the game.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_ROOT = os.path.dirname(HERE)
ALLOW_NAME = os.path.join("tools", "text_audit_allow.txt")
SKIP = 77
MIN_WORDS = 3
PHRASE_LENGTH = 20
LITERAL_MIN = 4
PIECE_MIN = 6
C_SOURCES = (".c", ".h", ".inc", ".cpp", ".hpp")
FILE_NAME = re.compile(r"^[A-Za-z0-9_'$~-]{1,8}\.[A-Za-z0-9]{1,3}$")
BYTE_RUN = 16
DATA_SEGMENT_OFFSET = 0x326A0          # 3A6A:0000 in the load image (base 0800)
DICTIONARY_FIRST = (0x05AA, 0x10)      # DS offset, length
DICTIONARY_SECOND = (0x05BA, 0x80)
NOT_TEXT = re.compile(r"^[A-Za-z0-9_'$~-]{1,8}\.[A-Za-z0-9]{1,3}$|%[-+ #0-9.]*l?[dsxXuc]")
TEXT_FILES = (".c", ".h", ".inc", ".cpp", ".hpp", ".md", ".txt", ".csv", ".py", ".ps1", ".cmake", ".json",
              ".cfg", ".yml", ".yaml", ".sln", ".vcxproj", ".filters", ".editorconfig", ".gitattributes",
              ".gitignore")


def find(game_dir, name):
    for entry in os.listdir(game_dir):
        if entry.upper() == name:
            return os.path.join(game_dir, entry)
    return None


def load_image(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:2] != b"MZ":
        sys.exit(f"text_audit: {path} is not an MZ program")
    return data[(data[8] | data[9] << 8) * 16:]


def expander(image):
    ds = image[DATA_SEGMENT_OFFSET:]
    first = ds[DICTIONARY_FIRST[0]:DICTIONARY_FIRST[0] + DICTIONARY_FIRST[1]]
    second = ds[DICTIONARY_SECOND[0]:DICTIONARY_SECOND[0] + DICTIONARY_SECOND[1]]

    def expand(data):
        out = bytearray()
        for value in data:
            while value & 0x80:
                index = (value & 0x7F) >> 3
                out.append(first[index])
                value = second[index * 8 + (value & 0x07)]
            out.append(value)
        return bytes(out)
    return expand


def is_wording(text):
    count = len(re.findall(r"[A-Za-z]{2,}", text))
    return count >= MIN_WORDS or (count >= 2 and len(text) >= PHRASE_LENGTH)


def looks_like_text(text):
    """Expansion of bytes that were never text gives letter soup; keep lines that read as words."""
    letters = sum(c.isalpha() or c in " ,.'!?-" for c in text)
    words = re.findall(r"[A-Za-z]+", text)
    vowelled = sum(bool(re.search(r"[aeiouyAEIOUY]", w)) for w in words)
    return letters >= 0.9 * len(text) and words and vowelled >= 0.8 * len(words) and " " in text


def printable_lines(data, start_offset=0):
    """(offset, text) of each printable line: NUL, CR and LF end one."""
    for m in re.finditer(rb"[\x20-\x7e]+", data):
        text = m.group().decode("ascii")
        stripped = text.strip()
        if stripped:
            yield start_offset + m.start() + (len(text) - len(text.lstrip())), stripped


def sources(game_dir):
    """wording {folded text: (file, offset, length)}, strings {text: (file, offset, length)}, image."""
    wording, strings = {}, {}
    exe = find(game_dir, "REVENGE.EXE")
    image = load_image(exe)
    expand = expander(image)

    def note(file, offset, text, expanded):
        if NOT_TEXT.search(text):
            return
        if expanded and not looks_like_text(text):
            return
        if is_wording(text):
            wording.setdefault(text.casefold(), (file, offset, len(text)))

    for offset, text in printable_lines(image):
        note("REVENGE.EXE", offset, text, False)
    for m in re.finditer(rb"[^\x00]{2,}", image):
        raw = m.group()
        if not any(b & 0x80 for b in raw):
            whole = raw
        else:
            whole = expand(raw)
            for offset, text in printable_lines(whole):
                note("REVENGE.EXE (expanded)", m.start(), text, True)
        tail = re.search(rb"[\x20-\x7e\r\n\t]*$", whole).group()
        if tail:
            text = tail.decode("ascii")
            if len(text) >= LITERAL_MIN and sum(c.isalpha() for c in text) >= 3 and not NOT_TEXT.search(text):
                strings.setdefault(text, ("REVENGE.EXE", m.start(), len(text)))
    for entry in sorted(os.listdir(game_dir)):
        path = os.path.join(game_dir, entry)
        if entry.upper() == "REVENGE.EXE" or not os.path.isfile(path):
            continue
        with open(path, "rb") as f:
            data = f.read()
        for offset, text in printable_lines(data):
            note(entry.upper(), offset, text, False)
    return wording, strings, image


def haystack(game_dir, image):
    """Every game file's bytes and REVENGE.EXE's expanded text, lowered, NUL-joined; with (start, file) marks."""
    expand = expander(image)
    parts, marks, at = [], [], 0
    def add(name, data):
        nonlocal at
        marks.append((at, name))
        parts.append(data.lower())
        at += len(data) + 1
    for entry in sorted(os.listdir(game_dir)):
        path = os.path.join(game_dir, entry)
        if not os.path.isfile(path):
            continue
        with open(path, "rb") as f:
            add(entry.upper(), f.read())
    expanded = bytearray()
    for m in re.finditer(rb"[^\x00]{2,}", image):
        if any(b & 0x80 for b in m.group()):
            expanded += expand(m.group()) + b"\0"
    add("REVENGE.EXE(expanded)", bytes(expanded))
    return b"\0".join(parts), marks


def locate(marks, position):
    file, start = marks[0][1], 0
    for mark_start, name in marks:
        if mark_start > position:
            break
        file, start = name, mark_start
    return file, position - start


def tracked(root):
    run = subprocess.run(["git", "-C", root, "ls-files", "-z", "--cached", "--others", "--exclude-standard"], capture_output=True)
    if run.returncode == 0 and os.path.isdir(os.path.join(root, ".git")):
        return [f.decode() for f in run.stdout.split(b"\0") if f]
    out = []
    for base, dirs, files in os.walk(root):
        dirs[:] = [d for d in dirs if d != ".git"]
        out += [os.path.relpath(os.path.join(base, f), root) for f in files]
    return sorted(out)


LITERAL = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
HEX_LIST = re.compile(r"(?:0x[0-9A-Fa-f]{1,2}\s*,\s*(?:/\*[^*]*\*/\s*)?){%d,}0x[0-9A-Fa-f]{1,2}" % (BYTE_RUN - 1))


def unescape(body):
    out, i = bytearray(), 0
    simple = {"n": 10, "r": 13, "t": 9, "0": 0, "\\": 92, '"': 34, "'": 39}
    while i < len(body):
        c = body[i]
        if c == "\\" and i + 1 < len(body):
            n = body[i + 1]
            if n == "x":
                m = re.match(r"[0-9A-Fa-f]{1,2}", body[i + 2:])
                if m:
                    out.append(int(m.group(), 16))
                    i += 2 + len(m.group())
                    continue
            if n in simple:
                out.append(simple[n])
                i += 2
                continue
        out += c.encode("latin-1", "replace")
        i += 1
    return bytes(out)


def joined_literals(text):
    """C literals, with adjacent literals ("a" "b") joined as the compiler joins them."""
    out = []
    pieces = list(LITERAL.finditer(text))
    i = 0
    while i < len(pieces):
        start = pieces[i].start()
        value = unescape(pieces[i].group(1))
        j = i
        while j + 1 < len(pieces) and not text[pieces[j].end():pieces[j + 1].start()].strip():
            j += 1
            value += unescape(pieces[j].group(1))
        out.append((start, value))
        i = j + 1
    return out


def allowed(root):
    path = os.path.join(root, ALLOW_NAME)
    out = set()
    if not os.path.exists(path):
        return out
    with open(path, encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            fields = line.split(None, 4)
            if len(fields) != 5:
                sys.exit(f"text_audit_allow.txt:{number}: expected a path, the kind, the file, the offset and the reason")
            out.add(tuple(fields[:4]))
    return out


def main():
    args = sys.argv[1:]
    root, listing = DEFAULT_ROOT, False
    if "--root" in args:
        i = args.index("--root")
        root = os.path.abspath(args[i + 1])
        del args[i:i + 2]
    if "--list" in args:
        listing = True
        args.remove("--list")
    game_dir = args[0] if args else os.environ.get("REVENGE_GAME", "")
    if not game_dir or not os.path.isdir(game_dir) or not find(game_dir, "REVENGE.EXE"):
        print("text_audit: no game folder with REVENGE.EXE (REVENGE_GAME); skipped")
        return SKIP
    wording, strings, image = sources(game_dir)
    pattern = re.compile(r"(?<![a-z0-9_])(?:" + "|".join(re.escape(w) for w in sorted(wording, key=len, reverse=True)) +
                         r")(?![a-z0-9_])")
    windows = {}
    for i in range(len(image) - BYTE_RUN + 1):
        window = image[i:i + BYTE_RUN]
        if len(set(window)) >= 4:
            windows.setdefault(window, i)
    hay, marks = haystack(game_dir, image)
    allow = allowed(root)
    used = set()
    hits = kept = 0
    files = tracked(root)
    for name in files:
        path = os.path.join(root, name)
        if name == ALLOW_NAME or not os.path.isfile(path):
            continue
        with open(path, "rb") as f:
            raw = f.read()
        if b"\0" in raw[:4096] and not name.lower().endswith(TEXT_FILES):
            continue                      # binary: images are reviewed by hand
        text = raw.decode("latin-1")
        found = []
        for m in pattern.finditer(text.casefold()):
            file, offset, length = wording[m.group()]
            found.append(("wording", m.start(), file, offset, length))
        for start, value in joined_literals(text) if name.lower().endswith(C_SOURCES) else ():
            if (len(value) >= PIECE_MIN and is_wording(value.decode("latin-1"))
                    and not FILE_NAME.match(value.decode("latin-1"))):
                position = hay.find(value.lower())
                if position >= 0:
                    file, offset = locate(marks, position)
                    found.append(("piece", start, file, offset, len(value)))
        if name.lower().endswith(C_SOURCES):
            for start, value in joined_literals(text):
                try:
                    decoded = value.decode("ascii")
                except UnicodeDecodeError:
                    continue
                if decoded in strings and is_wording(decoded):
                    file, offset, length = strings[decoded]
                    found.append(("literal", start, file, offset, length))
            for m in HEX_LIST.finditer(text):
                values = bytes(int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{1,2})", m.group()))
                runs = [windows[values[i:i + BYTE_RUN]] for i in range(len(values) - BYTE_RUN + 1)
                        if values[i:i + BYTE_RUN] in windows]
                if runs:
                    found.append(("bytes", m.start(), "REVENGE.EXE", runs[0], len(runs) + BYTE_RUN - 1))
        for kind, start, file, offset, length in found:
            line = text.count("\n", 0, start) + 1
            address = f"{offset:05X}"
            entry = next((e for e in ((name, kind, file.split()[0], address), ("*", kind, file.split()[0], address))
                          if e in allow), None)
            if entry:
                used.add(entry)
                kept += 1
                if not listing:
                    continue
            print(f"text_audit: {name}:{line}: {kind} {file} {address} ({length} bytes){' kept' if entry else ''}")
            if not entry:
                hits += 1
    for entry in sorted(allow - used):
        print(f"text_audit: {ALLOW_NAME}: {' '.join(entry)} no longer hits; remove it")
    print(f"text_audit: {len(wording)} lines and {len(strings)} strings of the game searched in {len(files)} files: "
          f"{hits} found, {kept} kept")
    return 1 if hits or allow - used else 0


if __name__ == "__main__":
    sys.exit(main())
