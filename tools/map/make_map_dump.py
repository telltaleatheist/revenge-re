#!/usr/bin/env python3
"""Writes src/SDL/map_dump.c: every data object the port's headers declare,
for the harness to write at each snap when REVENGE_MAP_DUMP_DIR names a
folder (tools/map/README.md).

    tools/map/make_map_dump.py [--check]

--check writes nothing and fails if the file isn't what this would write.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import map_objects  # noqa: E402

OUT = os.path.join(map_objects.ROOT, "src", "SDL", "map_dump.c")
# The import's own state: not the game's data.
LEFT_OUT = {"include/revenge/original_import.h"}


def include_name(header):
    if header.startswith("include/"):
        return header[len("include/"):]
    if header.startswith("src/Original/"):
        return header[len("src/Original/"):]
    return header


def source():
    objects = [o for o in map_objects.externs() if o[3] not in LEFT_OUT]
    headers = []
    for _, _, _, header in objects:
        if header not in headers:
            headers.append(header)
    lines = [
        "/* Written by tools/map/make_map_dump.py from the headers' externs; don't edit. */",
        "",
        "#if defined(_MSC_VER)",
        "#define _CRT_SECURE_NO_WARNINGS",
        "#endif",
        "",
        '#include "REVENGE.h"',
    ]
    lines += [f'#include "{include_name(h)}"' for h in headers if include_name(h) != "REVENGE.h"]
    lines += [
        '#include "map_dump.h"',
        "",
        "#include <SDL3/SDL.h>",
        "#include <stdio.h>",
        "#include <string.h>",
        "",
        "typedef struct MapDumpObject",
        "{",
        "\tconst char *name;",
        "\tconst void *object;",
        "\tsize_t size;          /* 0 for a string of unstated length */",
        "} MapDumpObject;",
        "",
        "static const MapDumpObject objects[] =",
        "{",
    ]
    for name, _, dimensions, _ in objects:
        if dimensions == "[]":
            lines.append(f'\t{{ "{name}", {name}, 0 }},')
        else:
            lines.append(f'\t{{ "{name}", &{name}, sizeof({name}) }},')
    lines += [
        "};",
        "",
        "void MapDump_Write(const char *snap)",
        "{",
        '\tconst char *folder = SDL_getenv("REVENGE_MAP_DUMP_DIR");',
        "\tchar path[4096];",
        "\tFILE *file;",
        "\tsize_t index;",
        "",
        "\tif (folder == NULL || folder[0] == '\\0')",
        "\t\treturn;",
        '\tsnprintf(path, sizeof(path), "%s/%s.objects", folder, snap);',
        '\tfile = fopen(path, "wb");',
        "\tif (file == NULL)",
        "\t\treturn;",
        "\tfor (index = 0; index < sizeof(objects) / sizeof(objects[0]); ++index)",
        "\t{",
        "\t\tconst unsigned char *bytes = (const unsigned char *) objects[index].object;",
        "\t\tsize_t size = objects[index].size != 0 ? objects[index].size",
        "\t\t\t: strlen((const char *) objects[index].object) + 1;",
        "\t\tsize_t at;",
        "",
        '\t\tfprintf(file, "%s %lu ", objects[index].name, (unsigned long) size);',
        "\t\tfor (at = 0; at < size; ++at)",
        '\t\t\tfprintf(file, "%02x", bytes[at]);',
        "\t\tfputc('\\n', file);",
        "\t}",
        "\tfclose(file);",
        "}",
        "",
    ]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    text = source()
    if args.check:
        try:
            with open(OUT, encoding="utf-8") as f:
                current = f.read()
        except OSError:
            current = None
        if current != text:
            sys.exit("make_map_dump: src/SDL/map_dump.c isn't current; run tools/map/make_map_dump.py")
        return
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print(f"make_map_dump: wrote {os.path.relpath(OUT, map_objects.ROOT)}")


if __name__ == "__main__":
    main()
