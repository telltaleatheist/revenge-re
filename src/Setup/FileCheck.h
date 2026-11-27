// The game's files against the ones this program knows: what a copy lacks, what
// in it is known to be damaged, and what differs. A report for the player, who
// can always go on; it stops nothing. Names, sizes and SHA-256 only.
// REVENGE.EXE has its own check (Setup.h) and isn't here; nor are the files
// that are the player's own (SAVEGAME.DAT, MEDIA.CFG, SCRIPT.CFG) or any file
// the program doesn't open (SETUP.EXE).
#pragma once

#include <rekit/sources.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace chr::setup
{

// A file the program opens, as the copy the project knows holds it
// (GameFilesTable.cpp, written by tools/setup/make_game_files.py, which says
// how each was seen opened).
struct GameFile
{
    enum class When
    {
        kStart,  // opened on the way to the title and the main menu
        kPlay,   // opened in the campaign's first stages
        kSome,   // only later in the campaign, or under a setting
    };
    const char *name;  // upper case
    uint32_t size;
    const char *sha256;
    When when;
    const char *without;  // what a play without it does
};
std::span<const GameFile> game_files();
// A file known to be damaged, by its bytes. None is known yet.
struct KnownDamage
{
    const char *name;
    uint32_t size;
    const char *sha256;
};
std::span<const KnownDamage> known_damage();

// What is at a listed file's name: its size, and its SHA-256, which the reader
// may leave empty when no listed file of that name has that size.
struct SeenFile
{
    uint64_t size = 0;
    std::string sha256;
};
// The file of that name (any case), nullopt when there is none (a folder of the name is none).
using FileReader = std::function<std::optional<SeenFile>(const std::string &name)>;

struct FilesReport
{
    std::vector<std::string> missing, damaged, different;  // names, in the list's order
    std::vector<std::string> effects;  // what a play without the missing and damaged ones does, each once
    std::string unchecked;             // why nothing was checked; "" when it was
    bool all_known() const { return unchecked.empty() && missing.empty() && damaged.empty() && different.empty(); }
};
FilesReport check_files(const FileReader &reader, std::span<const GameFile> files = game_files(),
    std::span<const KnownDamage> damage = known_damage());

// The files of a folder; of what the kit's walk found (a folder, or a folder in an archive: one forward read of the
// archive, only the listed files' bytes kept, and not at all past kMaxCheckedArchive). Never throws.
FilesReport check_folder(const std::string &dir, std::span<const GameFile> files = game_files(),
    std::span<const KnownDamage> damage = known_damage());
FilesReport check_found(const rekit::Found &found);
inline constexpr uint64_t kMaxCheckedArchive = uint64_t(64) << 20;

// For the player: "" when every file is one this program knows. At most kNamed names a list, "and N more".
std::string files_note(const FilesReport &report);
inline constexpr size_t kNamed = 3;
// The same in short, beside INSTALL: the screen has three lines for it, with what the copy is and the kit's own words.
// A file is named when it is the only one of its kind, else they are counted.
std::string files_pick_note(const FilesReport &report);
// For the launcher's one line (--status); "" when there is nothing to say, or it wasn't checked.
std::string files_status_note(const FilesReport &report);
// For the log: one line, every name.
std::string files_log(const FilesReport &report);

}  // namespace chr::setup
