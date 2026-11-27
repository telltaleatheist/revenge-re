#!/usr/bin/env python3
"""Checks that each declaration sits in the header beside the file that defines it.

    tools/check_headers.py [--root ROOT]

docs/SOURCE_LAYOUT.md gives the rule. A procedure or a variable that
src/Original shares is declared once, in the header of the code that
defines it:

- src/Original/DOMAIN/DOMAIN.h declares what the .cpp files of
  src/Original/DOMAIN define (Audio/Audio.h, Tactical/Tactical.h);
- src/Original/Data/NAME.h declares what src/Original/Data/NAME.cpp
  defines (Data/TacticalData.h).

It fails when such a header declares a function or a variable that isn't
defined where the rule says, when two of them declare the same name, when
a file includes one by its bare name ("Audio.h" for "Audio/Audio.h"), and
when a header includes REVENGE.h or revenge/original_data.h, the two
headers everything was once declared in. Exits 0 when the tree keeps the
rule, 1 otherwise.
"""
import argparse
import os
import re
import sys

UMBRELLAS = ("REVENGE.h", "original_data.h")
# The headers that still include an umbrella. Each commit of the split
# takes its folder's out; the list goes with the umbrellas.
NOT_YET = {
    "include/revenge/original/lance_editor_data.h",
    "src/Original/Campaign/CampaignInternal.h",
    "src/Original/Tactical/BattleInternal.h",
    "src/Original/Tactical/CombatInternal.h",
    "src/Original/Tactical/CommandsInternal.h",
    "src/Original/Units/LanceEditorInternal.h",
}
ORIGINAL = "src/Original"
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)


def umbrella(included):
    """Whether an include names REVENGE.h or revenge/original_data.h, however it is spelled."""
    return os.path.basename(included) in UMBRELLAS


def mask(text):
    """text with its comments, strings and characters blanked, newlines kept."""
    out = []
    i, n = 0, len(text)
    while i < n:
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
        elif text.startswith("//", i):
            end = text.find("\n", i)
            end = n if end < 0 else end
        elif text[i] in "\"'":
            end = i + 1
            while end < n and text[end] != text[i] and text[end] != "\n":
                end += 2 if text[end] == "\\" else 1
            end += 1
        else:
            out.append(text[i])
            i += 1
            continue
        out.append(re.sub(r"[^\n]", " ", text[i:end]))
        i = end
    return "".join(out)


def statements(masked):
    """The top-level statements of a header, each on one line, preprocessor lines left out."""
    lines = [line for line in masked.split("\n") if not line.lstrip().startswith("#")]
    found, current, depth = [], [], 0
    for character in "\n".join(lines):
        current.append(character)
        if character in "({[":
            depth += 1
        elif character in ")}]":
            depth -= 1
        elif character == ";" and depth == 0:
            found.append(" ".join("".join(current).split()))
            current = []
    return found


def declarations(header_text):
    """[(kind, name)] for the header's function and variable declarations."""
    found = []
    for statement in statements(mask(header_text)):
        if re.match(r"(typedef|static_assert|struct|union|enum|using|static|inline|constexpr|template)\b",
                     statement):
            continue
        flat = re.sub(r"\[[^\]]*\]", "", statement[:-1])
        if statement.startswith("extern") and not re.search(r"\w\s*\((?!\s*\*)", flat):
            for declarator in flat[len("extern"):].split(","):
                found.append(("variable", re.findall(r"\w+", declarator.split("=")[0])[-1]))
            continue
        call = re.search(r"(\w+)\s*\((?!\s*\*)", flat)
        if call:
            found.append(("function", call.group(1)))
    return found


def defined(kind, name, masked):
    """Whether a masked source defines name at file scope."""
    if kind == "function":
        for match in re.finditer(r"^[A-Za-z_][^;{}\n]*?\b" + re.escape(name) + r"\s*\(", masked, re.M):
            depth, i = 1, match.end()
            while depth and i < len(masked):
                depth += {"(": 1, ")": -1}.get(masked[i], 0)
                i += 1
            if re.match(r"\s*(noexcept\s*)?\{", masked[i:]):
                return True
        return False
    declarator = r"\b" + re.escape(name) + r"\b\s*(\[[^\]]*\]\s*)*(=|;|\{)"
    return bool(re.search(r"^(?!extern\b|static\b|typedef\b|return\b)[A-Za-z_][^\n;(=]*?" + declarator, masked, re.M)
                or re.search(r"^(?!extern\b|static\b|typedef\b|return\b)[A-Za-z_]\w*\s*\n\s+" + declarator,
                             masked, re.M))


def owned_header(relative):
    """(header, the .cpp files whose definitions it may declare) for a header the rule covers, else None."""
    parts = relative.split("/")
    if relative.startswith(ORIGINAL + "/") and len(parts) == 4 and parts[3].endswith(".h"):
        domain, name = parts[2], parts[3][:-2]
        if domain == "Data":
            return f"{ORIGINAL}/Data/{name}.cpp"
        if name == domain:
            return f"{ORIGINAL}/{domain}/"
    return None


def tree(root):
    files = {}
    for top in ("include", "src", "tests", "tools"):
        for folder, _, names in os.walk(os.path.join(root, top)):
            for name in names:
                if name.endswith((".h", ".cpp")):
                    path = os.path.join(folder, name)
                    with open(path, encoding="utf-8", errors="replace") as f:
                        files[os.path.relpath(path, root).replace(os.sep, "/")] = f.read()
    return files


def check(root):
    files = tree(root)
    masked = {relative: mask(text) for relative, text in files.items()}
    problems = []
    owners = {}
    covered = {}
    for relative in sorted(files):
        owner = owned_header(relative)
        if owner is None:
            continue
        covered[os.path.basename(relative)] = relative[len(ORIGINAL) + 1:]
        sources = [s for s in masked if s.endswith(".cpp") and
                   (s == owner if owner.endswith(".cpp") else os.path.dirname(s) + "/" == owner)]
        for kind, name in declarations(files[relative]):
            if name in owners:
                problems.append(f"{relative}: {name} is declared in {owners[name]} too")
                continue
            owners[name] = relative
            if not any(defined(kind, name, masked[s]) for s in sources):
                where = owner if owner.endswith(".cpp") else owner + "*.cpp"
                problems.append(f"{relative}: the {kind} {name} isn't defined in {where}")
    for relative, text in sorted(files.items()):
        for included in INCLUDE.findall(text):
            if included in covered:
                problems.append(f'{relative}: includes "{included}"; spell it "{covered[included]}"')
            if relative.endswith(".h") and umbrella(included) and relative not in NOT_YET:
                problems.append(f'{relative}: includes "{included}"; include the headers it needs')
    for relative in sorted(NOT_YET):
        if relative in files and not any(umbrella(i) for i in INCLUDE.findall(files[relative])):
            problems.append(f"{relative}: no longer includes an umbrella; take it out of NOT_YET")
    return problems, len(owners), len({h for h in owners.values()})


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    arguments = parser.parse_args()
    problems, names, headers = check(arguments.root)
    for problem in problems:
        print("check_headers: " + problem)
    if problems:
        return 1
    print(f"check_headers: valid: {names} declarations in {headers} headers, each beside its definition")
    return 0


if __name__ == "__main__":
    sys.exit(main())
