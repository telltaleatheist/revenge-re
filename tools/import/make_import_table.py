#!/usr/bin/env python3
"""Writes the import tables from the editions list, the pieces list, the sources
and a copy of the game.

    tools/import/make_import_table.py [GAME_DIR] [--check]

src/Support/original_editions.txt lists the editions the rebuild knows and
their known variants; from it alone this writes
src/Support/original_import_editions.c. GAME_DIR (default: the REVENGE_GAME
variable) is a copy of one edition, found there by its layout; for it this
writes that edition's table, src/Support/original_import_table_<id>.c. A copy
of no listed edition, or of an unlisted variant, stops with the line to add.

A table's rows are src/Support/original_import_pieces.txt's pieces, its
single texts last. Each row keeps the piece's offset, count, length and the
first 16 hex digits of its keyed HMAC-SHA-256: never its bytes. The bytes
hashed are the ones OriginalImport_PieceBytes (src/Support/original_import.c)
forms.

Every text is also a story id (the pieces list says how they are named), and
for those this writes, besides the edition's table of where each is in the
image with its length and keyed hash, include/revenge/story_text.h (the ids,
and each one's place in StoryTextBuffer, sized by the copy's lengths) and
src/Support/story_text_slots.c (where each text goes once the story file is
read).

--check writes nothing: it exits 1 when a file isn't what this would write.
Without a copy of the game it checks the editions list only and exits 77
(ctest's skip) when that is current.
"""
import hashlib
import hmac
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PIECES = os.path.join(ROOT, "src", "Support", "original_import_pieces.txt")
EDITIONS = os.path.join(ROOT, "src", "Support", "original_editions.txt")
EDITIONS_C = os.path.join(ROOT, "src", "Support", "original_import_editions.c")
STORY_H = os.path.join(ROOT, "include", "revenge", "story_text.h")
SLOTS_C = os.path.join(ROOT, "src", "Support", "story_text_slots.c")
STORY_ID = re.compile(r"^[a-z][a-z0-9_]*(\.[0-9]+)?$")
NO_SOURCE = 0xFFFFFFFF
ID = re.compile(r"^[a-z][a-z0-9]*$")
HASH = re.compile(r"^[0-9a-f]{16}$")
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
TEXT_KINDS = ("text_table", "expanded_text_table", "mutable_text_table", "pointed_text", "mutable_pointed_text")


def fail(message):
    sys.exit(f"make_import_table: {message}")


def table_path(edition_id):
    return os.path.join(ROOT, "src", "Support", f"original_import_table_{edition_id}.c")


def program(game_dir):
    """The copy's REVENGE.EXE as (file, header, load image), or None."""
    name = next((e for e in os.listdir(game_dir) if e.upper() == "REVENGE.EXE"), None)
    if name is None:
        return None
    with open(os.path.join(game_dir, name), "rb") as f:
        data = f.read()
    if data[:2] != b"MZ":
        fail(f"{name} is not an MZ program")
    header = (data[8] | data[9] << 8) * 16
    return data, data[:header], data[header:]


def editions():
    """original_editions.txt as [{id, name, bytes, paragraphs, header, variants: [(label, hash)]}]."""
    found, by_id = [], {}
    with open(EDITIONS, encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            fields = line.split("#", 1)[0].split()
            where = f"original_editions.txt:{number}"
            if not fields:
                continue
            if fields[0] == "edition" and len(fields) >= 6:
                ident, size, paragraphs, header = fields[1:5]
                if not ID.match(ident) or ident in by_id or not HASH.match(header):
                    fail(f"{where}: expected edition ID FILE-BYTES HEADER-PARAGRAPHS HEADER-HASH NAME")
                edition = {"id": ident, "name": " ".join(fields[5:]), "bytes": int(size),
                           "paragraphs": int(paragraphs, 16), "header": header, "variants": []}
                found.append(edition)
                by_id[ident] = edition
            elif fields[0] == "variant" and len(fields) == 4:
                if fields[1] not in by_id or not HASH.match(fields[3]):
                    fail(f"{where}: expected variant EDITION-ID LABEL|- IMAGE-HASH, after its edition")
                by_id[fields[1]]["variants"].append(("" if fields[2] == "-" else fields[2], fields[3]))
            else:
                fail(f"{where}: expected an edition or a variant line")
    if not found:
        fail("original_editions.txt lists no edition")
    return found


def pieces():
    """(headers, table rows, single texts [(id, offset)])."""
    headers, rows, texts = [], [], []
    with open(PIECES, encoding="utf-8") as f:
        for number, line in enumerate(f, 1):
            fields = line.split("#", 1)[0].split()
            where = f"original_import_pieces.txt:{number}"
            if not fields:
                continue
            if fields[0] == "header" and len(fields) == 2:
                headers.append(fields[1])
                continue
            if fields[0] == "text":
                if len(fields) != 3 or not STORY_ID.match(fields[1]):
                    fail(f"{where}: expected text ID DS-OFFSET, the id lower case")
                texts.append((fields[1], int(fields[2], 16)))
                continue
            if fields[0] not in KINDS or len(fields) < 3:
                fail(f"{where}: expected KIND NAME DS-OFFSET [COUNT [STRIDE]]")
            kind, name, offset = fields[0], fields[1], int(fields[2], 16)
            count = int(fields[3], 16) if len(fields) > 3 else 0
            stride = int(fields[4], 16) if len(fields) > 4 else 4
            rows.append((kind, name, offset, count, stride))
    return headers, rows, texts


def story_name(name):
    """A table's or object's name as a story id: TitleTextTable -> title."""
    for suffix in ("TextTable", "Table", "Text"):
        if name.endswith(suffix) and len(name) > len(suffix):
            name = name[:-len(suffix)]
            break
    return re.sub(r"(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", name).lower()


def enum_name(story_id):
    return story_id.upper().replace(".", "_")


def story_texts():
    """Every story id, in the list's order: [{id, kind, name, offset, index, stride}]."""
    _, rows, texts = pieces()
    found = []
    for kind, name, offset, count, stride in rows:
        if kind not in TEXT_KINDS:
            continue
        if kind in ("pointed_text", "mutable_pointed_text"):
            found.append({"id": story_name(name), "kind": kind, "name": name, "offset": offset,
                          "index": 0, "stride": stride})
            continue
        for index in range(count):
            found.append({"id": f"{story_name(name)}.{index}", "kind": kind, "name": name, "offset": offset,
                          "index": index, "stride": stride})
    for ident, offset in texts:
        found.append({"id": ident, "kind": "text", "name": None, "offset": offset, "index": 0, "stride": 0})
    seen = set()
    for text in found:
        if text["id"] in seen or enum_name(text["id"]) in seen:
            fail(f"the story id {text['id']} is given twice")
        seen.update((text["id"], enum_name(text["id"])))
    return found


def story_segment(img, text):
    """The segment the original names the text by: its far pointer's, loaded at 0800h."""
    if text["kind"] == "text":
        return 0x3A6A
    pointer = DATA_SEGMENT + text["offset"] + text["index"] * text["stride"]
    return 0x0800 + (img[pointer + 2] | img[pointer + 3] << 8)


def story_source(img, text):
    """(image offset or None, expanded, bytes without the NUL), as OriginalImport_TextBytes."""
    if text["kind"] == "text":
        where = DATA_SEGMENT + text["offset"]
    else:
        where = entry(img, text["offset"], text["index"], text["stride"])
    if where is None:
        return None, False, b""
    data = string_at(img, where)
    expanded = text["kind"] == "expanded_text_table"
    return where, expanded, expand(img, data) if expanded else data


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


def piece_count():
    _, rows, texts = pieces()
    return len(rows) + len(texts)


def render_editions(known):
    count = piece_count()
    lines = [
        "/* Written by tools/import/make_import_table.py from original_editions.txt.",
        " * Layouts, piece counts and keyed hashes only. Don't edit by hand. */",
        '#include "revenge/original_import.h"',
        '#include "revenge/story_text.h"',
        "",
    ]
    for edition in known:
        lines.append(f"extern const OriginalPiece OriginalPieces_{edition['id']}[0x{count:X}];")
        lines.append(f"extern const StoryTextSource StoryTextSources_{edition['id']}[STORY_TEXT_COUNT];")
    for edition in known:
        lines += ["", f"static const OriginalVariant Variants_{edition['id']}[] =", "{"]
        lines += [f'\t{{ "{label}", "{digest}" }},' for label, digest in edition["variants"]]
        lines += ["};"]
    lines += ["", "const OriginalEdition OriginalEditions[] =", "{"]
    for edition in known:
        ident = edition["id"]
        lines.append(f'\t{{ "{edition["name"]}", {edition["bytes"]}, 0x{edition["paragraphs"]:X}, "{edition["header"]}", '
                     f"OriginalPieces_{ident}, 0x{count:X}, StoryTextSources_{ident},")
        lines.append(f"\t\tVariants_{ident}, sizeof(Variants_{ident}) / sizeof(Variants_{ident}[0]) }},")
    lines += ["};", "", "const size_t OriginalEditionCount = sizeof(OriginalEditions) / sizeof(OriginalEditions[0]);"]
    return "\n".join(lines) + "\n"


def identify(known, file, header, img):
    """The listed edition the copy is, or a stop with the line to add."""
    digest = keyed(header)
    for edition in known:
        if edition["bytes"] == len(file) and edition["paragraphs"] * 16 == len(header) and edition["header"] == digest:
            image_digest = keyed(img)
            if all(h != image_digest for _, h in edition["variants"]):
                fail(f"this copy is a variant of {edition['name']} that original_editions.txt doesn't list; "
                     f"its line would be: variant {edition['id']} LABEL {image_digest}")
            return edition
    fail("this copy is no edition original_editions.txt lists; its line would be: "
         f"edition ID {len(file)} {len(header) // 16:X} {digest} NAME")


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


def render(img, edition):
    headers, rows, texts = pieces()
    array = f"OriginalPieces_{edition['id']}"
    lines = [
        "/* Written by tools/import/make_import_table.py from original_import_pieces.txt",
        f" * and a copy of {edition['name']}. Offsets, counts, lengths and keyed hashes",
        " * only. Don't edit by hand. */",
        '#include "revenge/original_import.h"',
        '#include "revenge/story_text.h"',
    ]
    lines += [f'#include "{h}"' for h in headers]
    lines += ["", f'const char OriginalImportKey[] = "{KEY}";', "",
              f"const OriginalPiece {array}[0x{piece_count():X}] =", "{"]
    checks = []
    for kind, name, offset, count, stride in rows:
        data = piece_bytes(img, kind, offset, count, stride)
        if kind == "bytes":
            length, target = str(count), f"&{name}"
            checks.append(f"_Static_assert(sizeof({name}) == 0x{count:X}, \"{name} is the piece's size\");")
        elif kind == "byte_lists":
            length, target = "0", f"(void *) {name}"
            checks.append(f"_Static_assert(sizeof({name}) / sizeof({name}[0]) == 0x{count:X}, "
                          f"\"{name} has the piece's entries\");")
        else:
            length, target = "0", "NULL"     # a text: the story file places it
        lines.append(f'\t{{ "{name}", {KINDS[kind]}, 0x{offset:04X}, 0x{count:X}, {stride}, {length}, '
                     f'{target}, "{keyed(data)}" }},')
    for ident, offset in texts:
        data = piece_bytes(img, "text", offset, 0, 0)
        lines.append(f'\t{{ "{ident}", ORIGINAL_PIECE_TEXT, 0x{offset:04X}, 0, 0, {len(data) - 1}, NULL, "{keyed(data)}" }},')
    lines += ["};", ""]
    lines += [f"const StoryTextSource StoryTextSources_{edition['id']}[STORY_TEXT_COUNT] =", "{"]
    for text in story_texts():
        where, expanded, data = story_source(img, text)
        place = f"0x{NO_SOURCE:08X}UL" if where is None else f"0x{where:08X}UL"
        segment = 0 if where is None else story_segment(img, text)
        lines.append(f'\t{{ {place}, 0x{segment:04X}, 0x{len(data):X}, {1 if expanded else 0}, '
                     f'"{keyed(data + bytes(1))}" }}, /* {text["id"]} */')
    lines += ["};", ""]
    lines += checks
    return "\n".join(lines) + "\n"


def story_layout(img):
    """[(text, place, capacity)] in id order, the places in image order."""
    texts = story_texts()
    sized = []
    for order, text in enumerate(texts):
        where, _, data = story_source(img, text)
        sized.append((NO_SOURCE if where is None else where, order, text, len(data) + 1))
    places, at = {}, 0
    for _, order, _, capacity in sorted(sized, key=lambda s: (s[0], s[1])):
        places[order] = at
        at += capacity
    return [(text, places[order], capacity) for _, order, text, capacity in sized], at


def render_story_header(img, edition):
    layout, total = story_layout(img)
    lines = [
        "/* Written by tools/import/make_import_table.py from original_import_pieces.txt",
        f" * and a copy of {edition['name']}: the story's text ids and their places in",
        " * StoryTextBuffer, each the text's length and its NUL. Don't edit by hand. */",
        "#ifndef REVENGE_STORY_TEXT_H",
        "#define REVENGE_STORY_TEXT_H",
        "",
        "/* The ids, in original_import_pieces.txt's order. */",
        "typedef enum StoryTextId",
        "{",
    ]
    lines += [f"\tSTORY_ID_{enum_name(text['id'])}," for text, _, _ in layout]
    lines += ["\tSTORY_TEXT_COUNT", "} StoryTextId;", "",
              "/* Each text's place in StoryTextBuffer. */", "enum StoryTextPlace", "{"]
    lines += [f"\tSTORY_AT_{enum_name(text['id'])} = 0x{at:X}," for text, at, _ in layout]
    lines += [f"\tSTORY_TEXT_BYTES = 0x{total:X}", "};", "",
              "/* Every text the game shows, as the story file (src/Import) holds it:",
              " * filled once at start, before the game runs. */",
              "extern char StoryTextBuffer[STORY_TEXT_BYTES];", "",
              "/* A single text by its id, as STORY_TEXT(SAVE_GAME_SEPARATOR): an address",
              " * constant, so it may initialise a table, typed char * as a C string",
              " * literal is. */",
              "#define STORY_TEXT(name) (&StoryTextBuffer[STORY_AT_##name])", "",
              "#endif"]
    return "\n".join(lines) + "\n"


TARGETS = {
    "text_table": "STORY_TARGET_TEXT",
    "expanded_text_table": "STORY_TARGET_TEXT",
    "pointed_text": "STORY_TARGET_TEXT",
    "mutable_text_table": "STORY_TARGET_MUTABLE_TEXT",
    "mutable_pointed_text": "STORY_TARGET_ARRAY",
    "text": "STORY_TARGET_NONE",
}


def render_slots(img, edition):
    headers, rows, _ = pieces()
    layout, _ = story_layout(img)
    lines = [
        "/* Written by tools/import/make_import_table.py from original_import_pieces.txt",
        f" * and a copy of {edition['name']}: each story text's id, place and size, and",
        " * the table entry or array it fills. Don't edit by hand. */",
        '#include "revenge/story.h"',
    ]
    lines += [f'#include "{h}"' for h in headers]
    lines += ["", "const StoryTextSlot StoryTextSlots[STORY_TEXT_COUNT] =", "{"]
    for text, _, capacity in layout:
        kind, name, index = text["kind"], text["name"], text["index"]
        if kind == "text":
            destination, size = "NULL", "0"
        elif kind == "mutable_pointed_text":
            destination, size = name, f"sizeof({name})"
        elif kind == "pointed_text":
            destination, size = f"(void *) &{name}", "0"
        else:
            destination, size = f"(void *) &{name}[{index}]", "0"
        lines.append(f'\t{{ "{text["id"]}", STORY_AT_{enum_name(text["id"])}, 0x{capacity:X}, {TARGETS[kind]}, '
                     f"{destination}, {size} }},")
    lines += ["};", ""]
    for kind, name, offset, count, stride in rows:
        if kind in ("text_table", "expanded_text_table", "mutable_text_table"):
            lines.append(f"_Static_assert(sizeof({name}) / sizeof({name}[0]) == 0x{count:X}, "
                         f"\"{name} has the piece's entries\");")
        elif kind == "mutable_pointed_text":
            text = next(t for t in story_texts() if t["name"] == name)
            need = len(story_source(img, text)[2]) + 1
            lines.append(f"_Static_assert(sizeof({name}) >= 0x{need:X}, \"{name} holds its text\");")
    return "\n".join(lines) + "\n"


def settle(path, text, check):
    """Writes text to path, or with check compares; True when current."""
    relative = os.path.relpath(path, ROOT).replace(os.sep, "/")
    current = open(path, encoding="utf-8").read() if os.path.exists(path) else ""
    if check:
        if current != text:
            print(f"make_import_table: {relative} is out of date; run the tool")
            return False
        print(f"make_import_table: {relative} is current")
        return True
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print(f"make_import_table: wrote {relative}")
    return True


def main():
    args = sys.argv[1:]
    check = "--check" in args
    args = [a for a in args if a != "--check"]
    game_dir = args[0] if args else os.environ.get("REVENGE_GAME", "")
    known = editions()
    current = settle(EDITIONS_C, render_editions(known), check)
    copy = program(game_dir) if game_dir and os.path.isdir(game_dir) else None
    if copy is None:
        print("make_import_table: no game folder with REVENGE.EXE (REVENGE_GAME); its edition's table skipped")
        return SKIP if current else 1
    file, header, img = copy
    edition = identify(known, file, header, img)
    text = render(img, edition)
    written = [settle(table_path(edition["id"]), text, check),
               settle(STORY_H, render_story_header(img, edition), check),
               settle(SLOTS_C, render_slots(img, edition), check)]
    if not all(written):
        return 1
    print(f"make_import_table: {edition['name']}, {text.count('ORIGINAL_PIECE_')} pieces")
    return 0 if current else 1


if __name__ == "__main__":
    sys.exit(main())
