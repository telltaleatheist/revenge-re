#!/usr/bin/env python3
"""Writes src/Setup/GameFilesTable.cpp: the game's files the program opens,
with the size and SHA-256 they have in the copy the project knows (US DOS
1.00, docs/SUPPORTED_VERSIONS.md).

    python3 -I tools/setup/make_game_files.py GAME_DIR > src/Setup/GameFilesTable.cpp

Reads the folder and writes names, sizes and hashes, never the game's bytes.
Refuses a folder whose REVENGE.EXE isn't 1.00, and a listed file it lacks.

When each file is opened, as measured on 2026-11-27: the 13 trace scripts
(tools/trace/scripts) were run once each on a copy of the game whose files'
access times were set back, and the files read in each run were noted.

  start  read on the way to the title and the main menu (title.keys)
  play   read by the other scripts: a new campaign, the lance editor, the
         first two battles, a save and its load
  some   read by none of them: the later stages, the other maps and scenes,
         and the ending

Left out of the list: REVENGE.EXE (its own check, by its edition), the
player's own files (SAVEGAME.DAT, MEDIA.CFG, SCRIPT.CFG: a copy's differ by
what its player did), and the files the program never names (SETUP.EXE, the
original's own setup, and MSGS.TXT, its messages, which only SETUP.EXE names;
HELP.TXT and WINDOWS.TXT, notes for the player).
"""
import hashlib
import os
import sys

EDITION = (256411, "c4502165e251f3e2a5005b5c0e450e6b335b14c2be9c035b0fd2d53cbb30667e")
LEFT_OUT = {"REVENGE.EXE", "SAVEGAME.DAT", "MEDIA.CFG", "SCRIPT.CFG", "SETUP.EXE", "MSGS.TXT", "HELP.TXT", "WINDOWS.TXT"}

START = ["BTBORDER.CPS", "FONT6.FNT", "FONT8.FNT", "INFOCOM.BIN", "TITLE.COL", "TITLE.CPS", "TITLE.MUS"]
PLAY = ["BOOMS.CMP", "COMM.CPS", "DESTROY0.ICN", "DETHMEC1.MUS", "DETHMEC2.MUS", "DPHRASES.BIN", "DSATTACK.COL",
        "DSATTACK.CPS", "DSLYONS.COL", "DSLYONS.CPS", "FACES.COL", "HAWKLOGO.COL", "HAWKLOGO.CPS", "ICONLIST.DAT",
        "ICONSET0.ICN", "ISMAP1.CPS", "ISMAPS.COL", "KURT.CPS", "MAP0.MAP", "MARCH1.MUS", "MECHS1.CPS", "MECHS2.CPS",
        "MECHSET1.ICN", "MECHTYPE.DAT", "QUADPICS.COL", "QUADPICS.CPS", "RIP.COL", "RIP.CPS", "SCENE1.DAT",
        "SCENE2.DAT", "SELCTION.COL", "SUBTITLE.MUS", "SWAMP.COL", "SWAMP.CPS", "VICTORY.MUS"]

WITHOUT = {
    "start": "the game can't start",
    "play": "the campaign stops when it needs it",
    "some": "a later stage of the campaign stops when it needs it",
}


def files_of(folder):
    """The folder's files by their name in upper case."""
    return {name.upper(): os.path.join(folder, name) for name in os.listdir(folder)
            if os.path.isfile(os.path.join(folder, name))}


def read(path):
    with open(path, "rb") as f:
        return f.read()


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__.split("\n\n")[1])
    files = files_of(sys.argv[1])
    program = read(files.get("REVENGE.EXE") or sys.exit("no REVENGE.EXE there"))
    if (len(program), hashlib.sha256(program).hexdigest()) != EDITION:
        sys.exit("REVENGE.EXE there is not the 1.00 edition")
    listed = set(START) | set(PLAY)
    some = sorted(n for n in files if n not in listed and n not in LEFT_OUT)
    groups = [("start", START), ("play", PLAY), ("some", some)]
    for _, names in groups:
        for name in names:
            if name not in files:
                sys.exit(f"the copy has no {name}")
    print("""// The game's files the program opens, as the copy the project knows holds them
// (US DOS 1.00, docs/SUPPORTED_VERSIONS.md). Written by
// tools/setup/make_game_files.py, which says how each was seen opened; don't
// edit. Names, sizes and SHA-256 only.
#include "FileCheck.h"

namespace chr::setup {
namespace {

using When = GameFile::When;
constexpr GameFile kFiles[] = {""")
    for when, names in groups:
        print(f"    // {when}")
        for name in names:
            data = read(files[name])
            print(f'    {{"{name}", {len(data)}, "{hashlib.sha256(data).hexdigest()}", When::k{when.capitalize()}, '
                  f'"{WITHOUT[when]}"}},')
    print("""};

}  // namespace

std::span<const GameFile> game_files() { return kFiles; }
std::span<const KnownDamage> known_damage() { return {}; }

}  // namespace chr::setup""")


if __name__ == "__main__":
    main()
