#!/usr/bin/env python3
"""Writes the import table from the pieces list, the sources and the player's copy.

    tools/import/make_import_table.py [GAME_DIR] [--check]

GAME_DIR is the player's copy (default: the REVENGE_GAME variable). The rows
are src/Support/original_import_pieces.txt's pieces, then one text piece for
each DS offset that src or include names with ORIGINAL_TEXT(0x....), in offset
order. Each row keeps the piece's offset, count, length and the first 16 hex
digits of its keyed HMAC-SHA-256: never its bytes. The bytes hashed are the
ones OriginalImport_PieceBytes (src/Support/original_import.c) forms.

--check writes nothing: it exits 1 when src/Support/original_import_table.c
isn't what this would write. Without a copy of the game, 77 (ctest's skip).
"""
import hashlib
import hmac
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PIECES = os.path.join(ROOT, "src", "Support", "original_import_pieces.txt")
TABLE = os.path.join(ROOT, "src", "Support", "original_import_table.c")
KEY = "revenge-re import table: keyed so the hashes of short pieces can't be looked up"
DATA_SEGMENT = 0x326A0
DICTIONARY_FIRST, DICTIONARY_SECOND = 0x05AA, 0x05BA
SKIP = 77
KINDS = {
    "text_table": "ORIGINAL_PIECE_TEXT_TABLE",
    "expanded_text_table": "ORIGINAL_PIECE_EXPANDED_TEXT_TABLE",
    "mutable_text_table": "ORIGINAL_PIECE_MUTABLE_TEXT_TABLE",
    "pointed_text": "ORIGINAL_PIECE_POINTED_TEXT",
    "mutable_pointed_text": "ORIGINAL_PIECE_MUTABLE_POINTED_TEXT",
    "byte_lists": "ORIGINAL_PIECE_BYTE_LISTS",
    "bytes": "ORIGINAL_PIECE_BYTES",
}
USE = re.compile(r"\bORIGINAL_TEXT\(\s*0x([0-9A-Fa-f]{1,4})\s*\)")


def fail(message):
    sys.exit(f"make_import_table: {message}")


def image(game_dir):
    name = next((e for e in os.listdir(game_dir) if e.upper() == "REVENGE.EXE"), None)
    if name is None:
        return None
    with open(os.path.join(game_dir, name), "rb") as f:
        data = f.read()
    if data[:2] != b"MZ":
        fail(f"{name} is not an MZ program")
    return data[(data[8] | data[9] << 8) * 16:]


def pieces():
    headers, rows = [], []
    with open(PIECES, encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            fields = line.split("#", 1)[0].split()
            if not fields:
                continue
            if fields[0] == "header" and len(fields) == 2:
                headers.append(fields[1])
                continue
            if fields[0] not in KINDS or len(fields) < 3:
                fail(f"original_import_pieces.txt:{number}: expected KIND NAME DS-OFFSET [COUNT [STRIDE]]")
            kind, name, offset = fields[0], fields[1], int(fields[2], 16)
            count = int(fields[3], 16) if len(fields) > 3 else 0
            stride = int(fields[4], 16) if len(fields) > 4 else 4
            rows.append((kind, name, offset, count, stride))
    return headers, rows


def uses():
    found = set()
    for top in ("src", "include"):
        for base, _, files in os.walk(os.path.join(ROOT, top)):
            for name in files:
                if name.endswith((".c", ".h")):
                    with open(os.path.join(base, name), encoding="latin-1") as f:
                        found.update(int(m.group(1), 16) for m in USE.finditer(f.read()))
    return sorted(found)


def string_at(img, offset):
    end = img.index(b"\0", offset)
    return img[offset:end]


def entry(img, offset, index, stride):
    pointer = DATA_SEGMENT + offset + index * stride
    word, segment = img[pointer] | img[pointer + 1] << 8, img[pointer + 2] | img[pointer + 3] << 8
    return None if word == 0 and segment == 0 else segment * 16 + word


def expand(img, data):
    first, second = DATA_SEGMENT + DICTIONARY_FIRST, DATA_SEGMENT + DICTIONARY_SECOND
    out = bytearray()
    for value in data:
        while value & 0x80:
            index = (value & 0x7F) >> 3
            out.append(img[first + index])
            value = img[second + index * 8 + (value & 0x07)]
        out.append(value)
    return bytes(out)


def piece_bytes(img, kind, offset, count, stride):
    """As OriginalImport_PieceBytes."""
    if kind == "text":
        return string_at(img, DATA_SEGMENT + offset) + b"\0"
    if kind == "bytes":
        return img[DATA_SEGMENT + offset:DATA_SEGMENT + offset + count]
    out = bytearray()
    for index in range(count or 1):
        target = entry(img, offset, index, stride)
        if target is None:
            if kind != "byte_lists":
                out += b"\0"          # a null string is an empty one; a null list adds nothing
            continue
        if kind == "byte_lists":
            end = img.index(b"\xff", target)
            out += img[target:end + 1]
            continue
        text = string_at(img, target)
        out += (expand(img, text) if kind == "expanded_text_table" else text) + b"\0"
    return bytes(out)


def keyed(data):
    return hmac.new(KEY.encode(), data, hashlib.sha256).hexdigest()[:16]


def render(img):
    headers, rows = pieces()
    lines = [
        "/* Written by tools/import/make_import_table.py from original_import_pieces.txt,",
        " * the ORIGINAL_TEXT uses in src and include, and the player's REVENGE.EXE.",
        " * Offsets, counts, lengths and keyed hashes only. Don't edit by hand. */",
        '#include "revenge/original_import.h"',
    ]
    lines += [f'#include "{h}"' for h in headers]
    lines += ["", f'const char OriginalImportKey[] = "{KEY}";', "", "const OriginalPiece OriginalPieces[] =", "{"]
    checks = []
    for kind, name, offset, count, stride in rows:
        data = piece_bytes(img, kind, offset, count, stride)
        if kind == "bytes":
            length = str(count)
            checks.append(f"_Static_assert(sizeof({name}) == 0x{count:X}, \"{name} is the piece's size\");")
            target = f"&{name}"
        elif kind == "mutable_pointed_text":
            length = f"sizeof({name})"
            need = len(data)
            checks.append(f"_Static_assert(sizeof({name}) >= 0x{need:X}, \"{name} holds its text\");")
            target = name
        elif kind == "pointed_text":
            length = "0"
            target = f"(void *) &{name}"
        else:
            length = "0"
            target = f"(void *) {name}"
            checks.append(f"_Static_assert(sizeof({name}) / sizeof({name}[0]) == 0x{count:X}, "
                          f"\"{name} has the piece's entries\");")
        lines.append(f'\t{{ "{name}", {KINDS[kind]}, 0x{offset:04X}, 0x{count:X}, {stride}, {length}, '
                     f'{target}, "{keyed(data)}" }},')
    for offset in uses():
        data = piece_bytes(img, "text", offset, 0, 0)
        lines.append(f'\t{{ "text", ORIGINAL_PIECE_TEXT, 0x{offset:04X}, 0, 0, {len(data) - 1}, NULL, "{keyed(data)}" }},')
    lines += ["};", "", "const size_t OriginalPieceCount = sizeof(OriginalPieces) / sizeof(OriginalPieces[0]);", ""]
    lines += checks
    return "\n".join(lines) + "\n"


def main():
    args = sys.argv[1:]
    check = "--check" in args
    args = [a for a in args if a != "--check"]
    game_dir = args[0] if args else os.environ.get("REVENGE_GAME", "")
    img = image(game_dir) if game_dir and os.path.isdir(game_dir) else None
    if img is None:
        print("make_import_table: no game folder with REVENGE.EXE (REVENGE_GAME); skipped")
        return SKIP
    text = render(img)
    if check:
        current = open(TABLE, encoding="utf-8").read() if os.path.exists(TABLE) else ""
        if current != text:
            print("make_import_table: src/Support/original_import_table.c is out of date; run the tool")
            return 1
        print(f"make_import_table: the table is current ({text.count('ORIGINAL_PIECE_')} pieces)")
        return 0
    with open(TABLE, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print(f"make_import_table: wrote {os.path.relpath(TABLE, ROOT)} ({text.count('ORIGINAL_PIECE_')} pieces)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
