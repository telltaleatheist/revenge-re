#!/usr/bin/env python3
"""Writes src/SDL/map_dump.c: every data object the port's headers declare,
for the harness to write at each snap when REVENGE_MAP_DUMP_DIR names a
folder (tools/map/README.md).

    tools/map/make_map_dump.py [--check]

--check writes nothing and fails if the file isn't what this would write.
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import map_objects  # noqa: E402

OUT = os.path.join(map_objects.ROOT, "src", "SDL", "map_dump.c")
# The import's and the rules' own state: not the game's data (the rule
# table's length is the generated file's).
LEFT_OUT = {"include/revenge/original_import.h", "include/revenge/rules.h"}
# Left out of the snaps by name: the story's own constant table of where each
# text goes (its ids are the port's string literals).
NAMES_LEFT_OUT = {"StoryTextSlots"}
# The functions a pointer-typed object may hold, each hashed as its label;
# another function, or a static the headers don't name, hashes as "elsewhere".
# The empty stubs share one label: a linker may fold them into one address
# (MSVC's identical-code folding does), so their names can't be told apart.
FUNCTIONS = [("empty", "RuntimeExitStepStub"), ("empty", "RuntimeTerminationStub"),
             ("ShowResourceErrorMessage", "ShowResourceErrorMessage"),
             ("ShowFatalDiskErrorMessage", "ShowFatalDiskErrorMessage")]
POINTER_TYPEDEF = re.compile(r"typedef\s[^;{}]*\(\s*\*\s*(\w+)\s*\)\s*\(|typedef\s[^;{}()]*\*\s*(\w+)\s*;")


def pointer_typedefs():
    """The headers' typedef names that are pointers (to functions or data)."""
    names = set()
    for header in map_objects.headers():
        with open(os.path.join(map_objects.ROOT, header), encoding="utf-8") as f:
            text = map_objects.strip_comments(f.read())
        for m in POINTER_TYPEDEF.finditer(text):
            names.add(m.group(1) or m.group(2))
    return names


def is_pointer(type_text, typedefs):
    """A pointer, or an array of pointers, by its declared type."""
    return "*" in type_text or any(word in typedefs for word in re.findall(r"\w+", type_text))


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
        '#include "revenge/platform_backend.h"',
        '#include "revenge/sha256.h"',
        "",
        "#include <SDL3/SDL.h>",
        "#include <stdint.h>",
        "#include <stdio.h>",
        "#include <stdlib.h>",
        "#include <string.h>",
        "",
        "typedef struct MapDumpObject",
        "{",
        "\tconst char *name;",
        "\tconst void *object;",
        "\tsize_t size;          /* 0 for a string of unstated length */",
        "\tint pointers;         /* 1: a pointer or an array of them; -1: not by name */",
        "} MapDumpObject;",
        "",
        "static const MapDumpObject objects[] =",
        "{",
    ]
    typedefs = pointer_typedefs()
    for name, type_text, dimensions, _ in objects:
        kind = -1 if name in NAMES_LEFT_OUT else 1 if is_pointer(type_text, typedefs) else 0
        if dimensions == "[]":
            lines.append(f'\t{{ "{name}", {name}, 0, {kind} }},')
        else:
            lines.append(f'\t{{ "{name}", &{name}, sizeof({name}), {kind} }},')
    lines += [
        "};",
        "",
        "typedef void (*MapDumpFunction)(void);",
        "",
        "typedef struct MapDumpFunctionName",
        "{",
        "\tconst char *name;",
        "\tMapDumpFunction function;",
        "} MapDumpFunctionName;",
        "",
        "static const MapDumpFunctionName functions[] =",
        "{",
    ] + [f'\t{{ "{label}", (MapDumpFunction) {f} }},' for label, f in FUNCTIONS] + [
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
    ] + NAMES_C.split("\n")
    return "\n".join(lines)


NAMES_C = r"""
/* The snaps by name (MapDump_WriteNames). A pointer is a host address, so
 * its bytes differ from run to run and system to system: each pointer-sized,
 * pointer-aligned word of an object that points into a data object, or into
 * the DOS memory block, is hashed as that place's name and the offset in it
 * (the block's as the original's linear address). Every other byte is hashed
 * as it is. */

typedef struct MapDumpRegion
{
	uintptr_t start;
	size_t size;
	const char *name;       /* NULL: the DOS memory block */
	uint32_t linear;        /* the block's linear address */
} MapDumpRegion;

static MapDumpRegion *regions;
static size_t regionCount;

static size_t ObjectSize(size_t index)
{
	return objects[index].size != 0 ? objects[index].size
		: strlen((const char *) objects[index].object) + 1;
}

static int CompareRegions(const void *left, const void *right)
{
	uintptr_t a = ((const MapDumpRegion *) left)->start;
	uintptr_t b = ((const MapDumpRegion *) right)->start;

	return a < b ? -1 : a > b ? 1 : 0;
}

static void FindRegions(void)
{
	size_t count = sizeof(objects) / sizeof(objects[0]);
	const byte *block;
	uint32 capacity;
	word16 segment;
	size_t index;

	free(regions);
	regions = (MapDumpRegion *) calloc(count + 1, sizeof(MapDumpRegion));
	if (regions == NULL)
	{
		regionCount = 0;
		return;
	}
	regionCount = 0;
	for (index = 0; index < count; ++index)
	{
		regions[regionCount].start = (uintptr_t) objects[index].object;
		regions[regionCount].size = ObjectSize(index);
		regions[regionCount].name = objects[index].name;
		++regionCount;
	}
	block = SDLBackend_DOSMemoryBlock(&capacity, &segment);
	if (block != NULL)
	{
		regions[regionCount].start = (uintptr_t) block;
		regions[regionCount].size = capacity;
		regions[regionCount].linear = (uint32_t) segment << 4;
		++regionCount;
	}
	qsort(regions, regionCount, sizeof(MapDumpRegion), CompareRegions);
}

/* The region an address falls in, or NULL. An address just past an object's
 * end is not taken as that object's: what the linker puts there differs from
 * system to system (MSVC sets string literals beside named data). */
static const MapDumpRegion *Locate(uintptr_t address)
{
	size_t low = 0;
	size_t high = regionCount;

	if (address == 0)
		return NULL;
	while (low < high)
	{
		size_t middle = low + (high - low) / 2;

		if (regions[middle].start <= address)
			low = middle + 1;
		else
			high = middle;
	}
	if (low == 0)
		return NULL;
	if (address - regions[low - 1].start >= regions[low - 1].size)
		return NULL;
	return &regions[low - 1];
}

/* A pointer slot's place, written to the hash: the object or block it is
 * in, else the function it is, else "elsewhere" (a static the headers don't
 * name, a literal, a buffer on the stack). NULL is its zero bytes. */
static void HashPlace(Sha256 *hash, uintptr_t word, int *taken)
{
	const MapDumpRegion *region = Locate(word);
	char place[96];
	int length = 0;
	size_t index;

	*taken = 0;
	if (region != NULL)
	{
		size_t offset = (size_t) (word - region->start);

		length = region->name != NULL
			? snprintf(place, sizeof(place), "\x01%s+%zx\x01", region->name, offset)
			: snprintf(place, sizeof(place), "\x01dos:%lx\x01", (unsigned long) (region->linear + offset));
	}
	else if (word != 0)
	{
		for (index = 0; index < sizeof(functions) / sizeof(functions[0]); ++index)
		{
			uintptr_t function = 0;

			memcpy(&function, &functions[index].function,
				sizeof(function) < sizeof(functions[index].function) ? sizeof(function)
					: sizeof(functions[index].function));
			if (function == word)
			{
				length = snprintf(place, sizeof(place), "\x01%s()\x01", functions[index].name);
				break;
			}
		}
		if (length == 0)
			length = snprintf(place, sizeof(place), "\x01elsewhere\x01");
	}
	if (length > 0)
	{
		Sha256_Add(hash, place, (size_t) length);
		*taken = 1;
	}
}

/* The object's hash: its bytes, with each slot of an object declared as
 * pointers hashed as its place. Other objects hash their bytes alone: a number
 * that happens to equal an address there is not a pointer. */
static void HashObject(size_t index, char hex[17])
{
	const unsigned char *bytes = (const unsigned char *) objects[index].object;
	size_t size = ObjectSize(index);
	size_t at = 0;
	size_t run = 0;
	uint8_t digest[SHA256_DIGEST_BYTES];
	Sha256 hash;
	int digit;

	Sha256_Begin(&hash);
	while (at < size)
	{
		uintptr_t word;
		int taken = 0;

		if (objects[index].pointers > 0 && ((uintptr_t) (bytes + at)) % sizeof(void *) == 0
			&& size - at >= sizeof(void *))
		{
			memcpy(&word, bytes + at, sizeof(word));
			Sha256_Add(&hash, bytes + run, at - run);
			run = at;
			HashPlace(&hash, word, &taken);
		}
		if (!taken)
		{
			++at;
			continue;
		}
		at += sizeof(void *);
		run = at;
	}
	Sha256_Add(&hash, bytes + run, size - run);
	Sha256_End(&hash, digest);
	for (digit = 0; digit < 8; ++digit)
		snprintf(hex + digit * 2, 3, "%02x", digest[digit]);
}

void MapDump_WriteNames(FILE *file)
{
	size_t index;
	char hex[17];

	FindRegions();
	for (index = 0; index < sizeof(objects) / sizeof(objects[0]); ++index)
	{
		if (objects[index].pointers < 0)
			continue;
		HashObject(index, hex);
		fprintf(file, "%s %s\n", objects[index].name, hex);
	}
}
"""


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
