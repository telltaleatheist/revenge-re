#!/usr/bin/env python3
"""Writes src/SDL/map_dump.cpp: every data object the port's headers declare,
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

OUT = os.path.join(map_objects.ROOT, "src", "SDL", "map_dump.cpp")
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
             ("ShowFatalDiskErrorMessage", "ShowFatalDiskErrorMessage"),
             ("HandleDigitalWeaponEffectCompletion", "HandleDigitalWeaponEffectCompletion")]
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


FUNCTION_POINTER = re.compile(r"\(\s*\*\s*(\w+)\s*((?:\[[^\]]*\]\s*)*)\)")
DECLARATOR = re.compile(r"^(.*?)(\w+)\s*((?:\[[^\]]*\]\s*)*)$", re.S)
# Types with no pointer in them: the game's scalars and the C library's.
SCALARS = {"char", "short", "int", "long", "unsigned", "signed", "bool", "float",
           "double", "size_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int8_t",
           "int16_t", "int32_t", "int64_t", "DosAddress"}


SCALAR_TYPEDEF = re.compile(r"\btypedef\s+([\w\s]+?)\s+(\w+)\s*;")
STRUCT_START = re.compile(r"\b(?:typedef\s+)?(struct|union)\s+(\w+)?\s*\{")


def parse_members(body):
    """[(member, type text, dimensions, pointer)] of a struct's body. An
    anonymous struct or union inside gives its members as the outer's (C11);
    a named one, each prefixed with its name."""
    members = []
    at = 0
    while at < len(body):
        inner = STRUCT_START.search(body, at)
        end = body.find(";", at)
        if end < 0:
            break
        if inner and inner.start() < end:
            depth, close = 0, inner.end() - 1
            for close in range(inner.end() - 1, len(body)):
                depth += {"{": 1, "}": -1}.get(body[close], 0)
                if depth == 0:
                    break
            nested = parse_members(body[inner.end():close])
            tail_end = body.find(";", close)
            name = body[close + 1:tail_end].strip()
            if name:
                d = DECLARATOR.match(name)
                prefix = d.group(2) + "[0]" * d.group(3).count("[") + "." if d else ""
                members += [(prefix + m[0], m[1], m[2], m[3]) for m in nested]
            else:
                members += nested
            at = tail_end + 1
            continue
        declaration = re.sub(r":\s*\d+\s*$", "", " ".join(body[at:end].split()))
        at = end + 1
        if not declaration:
            continue
        pointer = FUNCTION_POINTER.search(declaration)
        if pointer:
            members.append((pointer.group(1), "", re.sub(r"\s+", "", pointer.group(2)), True))
            continue
        parts = declaration.split(",")
        first = DECLARATOR.match(parts[0].strip())
        if not first:
            continue
        base = first.group(1).replace("*", " ").strip()
        for index, part in enumerate(parts):
            d = DECLARATOR.match(part.strip()) if index else first
            if not d:
                continue
            type_text = d.group(1) if index == 0 else base + " " + d.group(1)
            members.append((d.group(2), " ".join(type_text.split()),
                            re.sub(r"\s+", "", d.group(3)), "*" in d.group(1)))
    return members


def structs():
    """{type name: [(member, type text, dimensions, pointer)]} for every struct
    or union the headers define, by tag and by typedef name; the scalar
    typedefs join SCALARS."""
    found = {}
    for header in map_objects.headers():
        with open(os.path.join(map_objects.ROOT, header), encoding="utf-8") as f:
            text = map_objects.strip_comments(f.read())
        text = re.sub(r"^\s*#[^\n]*", "", text, flags=re.M)
        for m in SCALAR_TYPEDEF.finditer(text):
            if base_type(m.group(1)) in SCALARS and "struct" not in m.group(1) and "union" not in m.group(1):
                SCALARS.add(m.group(2))
        at = 0
        while True:
            m = STRUCT_START.search(text, at)
            if not m:
                break
            depth, close = 0, m.end() - 1
            for close in range(m.end() - 1, len(text)):
                depth += {"{": 1, "}": -1}.get(text[close], 0)
                if depth == 0:
                    break
            members = parse_members(text[m.end():close])
            tail = re.match(r"\s*(\w+)?\s*;", text[close + 1:])
            for name in (m.group(2), tail.group(1) if tail else None):
                if name:
                    found[name] = members
            at = close + 1
    return found


def base_type(type_text):
    """The type's name, without qualifiers or struct/union."""
    words = [w for w in re.findall(r"\w+", type_text)
             if w not in ("const", "volatile", "struct", "union", "static", "unsigned", "signed")]
    return words[-1] if words else ""


def pointer_paths(members, typedefs, layouts, prefix, depth, unresolved):
    """[(member designator, [repeat designator])]: each pointer inside a
    struct of these members. A repeat designator is an array member whose
    elements repeat the path (counted and spaced in C)."""
    paths = []
    for name, type_text, dimensions, pointer in members:
        designator = prefix + name
        element = designator + "[0]" * dimensions.count("[")
        repeats = [designator] if dimensions else []
        if pointer or base_type(type_text) in typedefs:
            paths.append((element, repeats))
            continue
        kind = base_type(type_text)
        if kind in layouts and depth < 8:
            for inner, inner_repeats in pointer_paths(layouts[kind], typedefs, layouts,
                                                      element + ".", depth + 1, unresolved):
                paths.append((inner, repeats + inner_repeats))
        elif kind not in SCALARS:
            unresolved.add(kind)
    return paths


def include_name(header):
    if header.startswith("include/"):
        return header[len("include/"):]
    if header.startswith("src/Original/"):
        return header[len("src/Original/"):]
    return header


def source():
    # By name, so a declaration that moves between headers leaves the table as it is.
    objects = sorted((o for o in map_objects.externs() if o[3] not in LEFT_OUT), key=lambda o: o[0])
    headers = sorted({header for _, _, _, header in objects})
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
        "#include <stddef.h>",
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
    layouts = structs()
    slots = []
    unresolved = set()
    for index, (name, type_text, dimensions, _) in enumerate(objects):
        if name in NAMES_LEFT_OUT or is_pointer(type_text, typedefs):
            continue
        kind = base_type(type_text)
        if kind not in layouts:
            if kind not in SCALARS:
                unresolved.add(kind)
            continue
        c_type = " ".join(w for w in type_text.split() if w not in ("const", "volatile"))
        element = name + "[0]" * dimensions.count("[")
        for path, repeats in pointer_paths(layouts[kind], typedefs, layouts, "", 0, unresolved):
            counts = []
            if dimensions:
                counts.append((f"sizeof({name}) / sizeof({element})", f"sizeof({element})"))
            for repeat in repeats:
                array = f"(({c_type} *) 0)->{repeat}"
                inner = array + "[0]" * 1
                counts.append((f"sizeof({array}) / sizeof({inner})", f"sizeof({inner})"))
            if len(counts) > 2:
                sys.exit(f"make_map_dump: {name}.{path} repeats {len(counts)} times over; the table holds two")
            counts += [("1", "0")] * (2 - len(counts))
            slots.append(f"\t{{ {index}, offsetof({c_type}, {path}), {counts[0][0]}, {counts[0][1]}, "
                         f"{counts[1][0]}, {counts[1][1]} }},")
    if unresolved - {""}:
        sys.exit("make_map_dump: no layout found for " + ", ".join(sorted(unresolved - {""})))
    for name, type_text, dimensions, _ in objects:
        kind = -1 if name in NAMES_LEFT_OUT else 1 if is_pointer(type_text, typedefs) else 0
        if dimensions == "[]":
            lines.append(f'\t{{ "{name}", {name}, 0, {kind} }},')
        else:
            lines.append(f'\t{{ "{name}", &{name}, sizeof({name}), {kind} }},')
    lines += [
        "};",
        "",
        "/* Each pointer inside an object that isn't itself a pointer or an array",
        " * of them: its offset, repeated count1 times stride1 apart and each of",
        " * those count2 times stride2 apart (the arrays it sits in). */",
        "typedef struct MapDumpSlot",
        "{",
        "\tsize_t object;",
        "\tsize_t offset;",
        "\tsize_t count1;",
        "\tsize_t stride1;",
        "\tsize_t count2;",
        "\tsize_t stride2;",
        "} MapDumpSlot;",
        "",
        "static const MapDumpSlot pointerSlots[] =",
        "{",
    ] + (slots or ["\t{ 0, 0, 0, 0, 0, 0 },"]) + [
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
 * a block of the heap, is hashed as that place's name and the offset in it
 * (a block's name is its number in allocation order, revenge/heap.h). Every
 * other byte is hashed as it is. */

typedef struct MapDumpRegion
{
	uintptr_t start;
	size_t size;
	const char *name;
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
	size_t index;

	free(regions);
	regions = (MapDumpRegion *) calloc(count, sizeof(MapDumpRegion));
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

/* A pointer slot's place, written to the hash: the object or heap block it
 * is in, else the function it is, else "elsewhere" (a static the headers don't
 * name, a literal, a buffer on the stack). NULL is its zero bytes. */
static void HashPlace(Sha256 *hash, uintptr_t word, int *taken)
{
	const MapDumpRegion *region = Locate(word);
	char place[96];
	int length = 0;
	size_t index;
	uint32_t number;
	uint32_t offset;

	*taken = 0;
	if (region != NULL)
	{
		length = snprintf(place, sizeof(place), "\x01%s+%zx\x01", region->name,
			(size_t) (word - region->start));
	}
	else if (word != 0 && Heap_Find(word, &number, &offset))
	{
		length = snprintf(place, sizeof(place), "\x01heap:%lu+%lx\x01",
			(unsigned long) number, (unsigned long) offset);
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

static int CompareOffsets(const void *left, const void *right)
{
	size_t a = *(const size_t *) left;
	size_t b = *(const size_t *) right;

	return a < b ? -1 : a > b ? 1 : 0;
}

/* The offsets of the object's pointers, ascending: every aligned slot of an
 * object declared as pointers, else its struct members that are (the slot
 * table). *count is how many; the caller frees the list. */
static size_t *PointerOffsets(size_t index, size_t size, size_t *count)
{
	size_t capacity = 0;
	size_t *offsets = NULL;
	size_t slot;

	*count = 0;
	if (objects[index].pointers > 0)
	{
		const unsigned char *bytes = (const unsigned char *) objects[index].object;
		size_t at;

		offsets = (size_t *) malloc((size / sizeof(void *) + 1) * sizeof(size_t));
		if (offsets == NULL)
			return NULL;
		for (at = 0; at + sizeof(void *) <= size; ++at)
		{
			if (((uintptr_t) (bytes + at)) % sizeof(void *) == 0)
				offsets[(*count)++] = at;
		}
		return offsets;
	}
	for (slot = 0; slot < sizeof(pointerSlots) / sizeof(pointerSlots[0]); ++slot)
	{
		const MapDumpSlot *entry = &pointerSlots[slot];
		size_t first;
		size_t second;

		if (entry->object != index)
			continue;
		for (first = 0; first < entry->count1; ++first)
		{
			for (second = 0; second < entry->count2; ++second)
			{
				size_t at = entry->offset + first * entry->stride1 + second * entry->stride2;

				if (at + sizeof(void *) > size)
					continue;
				if (*count == capacity)
				{
					size_t *grown;

					capacity = capacity == 0 ? 16 : capacity * 2;
					grown = (size_t *) realloc(offsets, capacity * sizeof(size_t));
					if (grown == NULL)
					{
						free(offsets);
						*count = 0;
						return NULL;
					}
					offsets = grown;
				}
				offsets[(*count)++] = at;
			}
		}
	}
	if (*count > 1)
		qsort(offsets, *count, sizeof(size_t), CompareOffsets);
	return offsets;
}

/* The object's hash: its bytes, with each pointer in it hashed as its place
 * (PointerOffsets). Other bytes hash as they are: a number that happens to
 * equal an address is not a pointer. */
static void HashObject(size_t index, char hex[17])
{
	const unsigned char *bytes = (const unsigned char *) objects[index].object;
	size_t size = ObjectSize(index);
	size_t count;
	size_t *offsets = PointerOffsets(index, size, &count);
	size_t run = 0;
	size_t slot;
	uint8_t digest[SHA256_DIGEST_BYTES];
	Sha256 hash;
	int digit;

	Sha256_Begin(&hash);
	for (slot = 0; slot < count; ++slot)
	{
		size_t at = offsets[slot];
		uintptr_t word;
		int taken = 0;

		if (at < run)
			continue;
		memcpy(&word, bytes + at, sizeof(word));
		Sha256_Add(&hash, bytes + run, at - run);
		run = at;
		HashPlace(&hash, word, &taken);
		if (taken)
			run = at + sizeof(void *);
	}
	free(offsets);
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
            sys.exit("make_map_dump: src/SDL/map_dump.cpp isn't current; run tools/map/make_map_dump.py")
        return
    with open(OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print(f"make_map_dump: wrote {os.path.relpath(OUT, map_objects.ROOT)}")


if __name__ == "__main__":
    main()
