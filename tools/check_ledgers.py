#!/usr/bin/env python3
"""Checks the two preservation ledgers and the rules src/Original keeps.

    tools/check_ledgers.py [--root DIR]

docs/METHOD_TABLE.csv and docs/SOURCE_CONVERSION_TABLE.csv:
- every method address is SEG:OFF in upper-case hex, and each is listed once
  in each table;
- every method's status is final (Verified, Verified [SDL], Replaced or Not
  a procedure); a procedure has a C name and an annotation file, and a C file
  that exists unless it is Replaced (paths under evidence/, kept outside this
  tree, stay as references); Verified [SDL] names its platform redirect, and
  Replaced names what replaced it there and has no C file;
- the conversion table lists exactly the procedures of the method table;
- a C name used by two addresses lives in one file and says it's an alias;
- a procedure that isn't Replaced is defined in its C file (`Name(...) {`),
  and the conversion table names the same C name and file for it (its
  current and target files both), so a renamed file or function the ledgers
  missed fails here.

src/Original/**/*.cpp:
- a static function has a SEG:OFF in the 700 characters before it;
- no SDL call or header, and no RevengeTrace call.

Comparisons that aren't addresses ignore case. Exits 0 when all hold, 1
otherwise, naming each problem.
"""
import csv
import os
import re
import sys

ADDRESS = re.compile(r"^[0-9A-F]{4}:[0-9A-F]{4}$")
STATUSES = {"verified", "verified [sdl]", "replaced", "not a procedure"}
NOT_A_PROCEDURE = "not a procedure"
REPLACED = "replaced"
STATIC_FUNCTION = re.compile(r"(?m)^\s*static\s+(?:[A-Za-z_][A-Za-z0-9_]*\s+|\*\s*)+"
                             r"[A-Za-z_][A-Za-z0-9_]*\s*\([^;{}]*\)\s*\{")
SEGMENT_OFFSET = re.compile(r"[0-9A-Fa-f]{4}:[0-9A-Fa-f]{4}")
SDL_USE = re.compile(r"\bSDL_[A-Za-z][A-Za-z0-9_]*\b|^\s*#\s*include\s*[<\"]SDL", re.MULTILINE)
TRACE_USE = re.compile(r"\bRevengeTrace\s*\(")
ALIAS = re.compile(r"\balias\b", re.IGNORECASE)
PREFIX_CHARACTERS = 700


def definition(name):
    """A function definition of name: its name, parameters, then its body."""
    return re.compile(r"\b" + re.escape(name) + r"\s*\([^;{}]*\)\s*\{")


def read_table(path):
    with open(path, encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def status(row):
    return (row.get("Status") or "").strip().casefold()


def blank(value):
    return value is None or not value.strip()


def check(root):
    errors = []
    methods = read_table(os.path.join(root, "docs", "METHOD_TABLE.csv"))
    conversions = read_table(os.path.join(root, "docs", "SOURCE_CONVERSION_TABLE.csv"))

    def check_paths(address, field, value):
        if blank(value):
            errors.append(f"{address} has no {field}.")
            return
        for relative in re.split(r";\s*", value):
            if relative.startswith("evidence/"):
                continue
            if not os.path.exists(os.path.join(root, relative)):
                errors.append(f"{address} has missing {field} path: {relative}")

    for row in methods:
        address = row["Address"]
        if not ADDRESS.match(address):
            errors.append(f"Invalid method address: {address}")
        if status(row) not in STATUSES:
            errors.append(f"{address} has non-final status '{row['Status']}'.")
        if status(row) != NOT_A_PROCEDURE:
            if blank(row.get("CName")):
                errors.append(f"{address} has no CName.")
            check_paths(address, "AnnotationFile", row.get("AnnotationFile"))
        if status(row) not in (NOT_A_PROCEDURE, REPLACED):
            check_paths(address, "OriginalCFile", row.get("OriginalCFile"))
        if status(row) == "verified [sdl]" and blank(row.get("PlatformRedirect")):
            errors.append(f"{address} is Verified [SDL] without PlatformRedirect.")
        if status(row) == REPLACED:
            if blank(row.get("PlatformRedirect")):
                errors.append(f"{address} is Replaced without PlatformRedirect.")
            if not blank(row.get("OriginalCFile")):
                errors.append(f"{address} is Replaced but names a C file.")

    for name, rows in (("METHOD_TABLE", methods), ("SOURCE_CONVERSION_TABLE", conversions)):
        seen = {}
        for row in rows:
            seen.setdefault(row["Address"].casefold(), []).append(row["Address"])
        for addresses in seen.values():
            if len(addresses) > 1:
                errors.append(f"Duplicate {name} address: {addresses[0]}")

    procedures = {r["Address"].casefold(): r["Address"] for r in methods if status(r) != NOT_A_PROCEDURE}
    converted = {r["Address"].casefold(): r["Address"] for r in conversions}
    missing = sorted(procedures[k] for k in procedures.keys() - converted.keys())
    extra = sorted(converted[k] for k in converted.keys() - procedures.keys())
    if missing:
        errors.append("Missing conversion rows: " + ", ".join(missing))
    if extra:
        errors.append("Conversion rows without executable methods: " + ", ".join(extra))

    by_name = {}
    for row in methods:
        if status(row) != NOT_A_PROCEDURE and not blank(row.get("CName")):
            by_name.setdefault(row["CName"].casefold(), []).append(row)
    for rows in by_name.values():
        if len(rows) < 2:
            continue
        files = {(r.get("OriginalCFile") or "").casefold() for r in rows}
        documented = any(ALIAS.search(r.get("Evidence") or "") or ALIAS.search(r.get("Uncertainty") or "")
                         for r in rows)
        if len(files) != 1 or not documented:
            errors.append(f"Unexplained duplicate CName '{rows[0]['CName']}': "
                          + ", ".join(r["Address"] for r in rows))

    sources = {}
    for row in methods:
        if status(row) in (NOT_A_PROCEDURE, REPLACED) or blank(row.get("CName")):
            continue
        relative = (row.get("OriginalCFile") or "").strip()
        path = os.path.join(root, relative)
        if not os.path.isfile(path):
            continue
        if relative not in sources:
            with open(path, encoding="utf-8", errors="replace", newline="") as f:
                sources[relative] = f.read()
        if not definition(row["CName"]).search(sources[relative]):
            errors.append(f"{row['Address']} {row['CName']} isn't defined in {relative}.")

    by_address = {r["Address"].casefold(): r for r in methods}
    for row in conversions:
        method = by_address.get(row["Address"].casefold())
        if method is None or status(method) == NOT_A_PROCEDURE:
            continue
        if row.get("CName") != method.get("CName"):
            errors.append(f"{row['Address']} is {row.get('CName')} in SOURCE_CONVERSION_TABLE, "
                          f"{method.get('CName')} in METHOD_TABLE.")
        for field in ("CurrentOriginalFile", "TargetOriginalFile"):
            if (row.get(field) or "") != (method.get("OriginalCFile") or ""):
                errors.append(f"{row['Address']} {field} {row.get(field)!r} isn't METHOD_TABLE's "
                              f"OriginalCFile {method.get('OriginalCFile')!r}.")

    original = os.path.join(root, "src", "Original")
    for base, _, files in os.walk(original):
        for name in sorted(files):
            if not name.endswith(".cpp"):
                continue
            path = os.path.join(base, name)
            relative = os.path.relpath(path, root).replace(os.sep, "/")
            with open(path, encoding="utf-8", errors="replace", newline="") as f:
                text = f.read()
            for match in STATIC_FUNCTION.finditer(text):
                prefix = text[max(0, match.start() - PREFIX_CHARACTERS):match.start()]
                if not SEGMENT_OFFSET.search(prefix):
                    line = text.count("\n", 0, match.start()) + 1
                    errors.append(f"Addressless static function in {relative}:{line}.")
            if SDL_USE.search(text):
                errors.append(f"Direct SDL API or header use in Original: {relative}")
            if TRACE_USE.search(text):
                errors.append(f"Unisolated reconstruction trace in Original: {relative}")

    return errors, methods, conversions


def main():
    args = sys.argv[1:]
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    if args[:1] == ["--root"] and len(args) == 2:
        root = os.path.abspath(args[1])
    elif args:
        sys.exit("usage: check_ledgers.py [--root DIR]")
    errors, methods, conversions = check(root)
    for error in errors:
        print(f"check_ledgers: {error}", file=sys.stderr)
    if errors:
        print(f"check_ledgers: {len(errors)} problem(s)", file=sys.stderr)
        return 1
    counts = {}
    for row in methods:
        counts[row["Status"]] = counts.get(row["Status"], 0) + 1
    summary = ", ".join(f"{k}={v}" for k, v in sorted(counts.items(), key=lambda kv: kv[0].casefold()))
    print(f"check_ledgers: valid: {len(methods)} method rows; {len(conversions)} procedures; {summary}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
