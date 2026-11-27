// The Crescent Hawks' Revenge's setup recipe for re-setup-kit (extern/re-setup-kit,
// DESIGN.md there): what a copy of the 1990 DOS game is, where it may be, how
// it is copied into the player's folder, and the launcher contract's
// --status and --setup --copies. The kit knows no game; everything Revenge's
// is here. Nothing in it reads or prints the game's text: files are judged by
// their size and SHA-256 alone.
#pragma once

#include <rekit/family_folder.h>
#include <rekit/sdl/setup_screen.h>
#include <rekit/search.h>
#include <rekit/sources.h>
#include <rekit/status.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace chr::setup {

inline constexpr const char* kId = "chr";  // the launcher contract's game id, and the launcher's catalog key
// The setup screen's sentences; the window's title (src/SDL/platform.cpp) names it the same.
inline constexpr const char* kTitle = "BattleTech: The Crescent Hawks' Revenge";
inline constexpr const char* kFolderName = "Revenge Rebuild";  // a game folder made inside a picked one
inline constexpr const char* kFolderKey = "chr";  // its mark's game=<key> (the kit's game_folder.h)
// The player's folder (saves, settings.txt, story/): SDL's per-user folder for these names
// (src/SDL/game_paths.cpp spells the same two), under the family's folder.
inline constexpr const char* kOrganisation = rekit::kFamilyOrg;
inline constexpr const char* kApplication = "Revenge Rebuild";
// In the player's folder: the game folder the setup chose, one "game_folder = <path>" line.
inline constexpr const char* kSetupFile = "setup.txt";
// In a folder our install made, and only there (rekit::swap_in_folder replaces only such a folder).
inline constexpr const char* kInstallMarker = "revenge-install.txt";
// The game is about 2 MB: a pick holding far more (a Games folder, a drive's top) is refused before copying.
inline constexpr uint64_t kMaxGameBytes = uint64_t(256) << 20;

// The per-user folder for (organisation, application), ending in a separator, the way SDL_GetPrefPath names it
// (%APPDATA%, ~/Library/Application Support, $XDG_DATA_HOME or ~/.local/share), WITHOUT making it: --status
// writes nothing. "" when the platform's base cannot be found.
std::string pref_folder(const char* organisation, const char* application);
// The player's folder: REVENGE_SAVE_DIRECTORY, else pref_folder(kOrganisation, kApplication). Not made here.
std::string user_folder();
// <user folder>/install: where the setup puts the game's files unless the player picks another folder.
std::string default_install(const std::string& user);

// REVENGE.EXE's edition by its bytes: "1.00" (256,411 bytes, docs/SUPPORTED_VERSIONS.md), "" when not a known one.
std::string edition(const std::vector<uint8_t>& bytes);

// What the kit's walk looks for: REVENGE.EXE judged by its edition; a pick of REVENGE.EXE or SETUP.EXE stands for
// its folder.
rekit::SourceRules rules();
// What a pick holds when the walk found no copy in it, in one sentence for the setup screen: a REVENGE.EXE of an
// edition this program doesn't know, or another BattleTech game's DOS copy, by its program. "" when it is none of
// those (the kit's own sentence stands) or the pick can't be read. Given any path; never throws; the look is stopped
// after a while (kRecogniseSeconds).
std::string recognise(const std::string& path);
inline constexpr int kRecogniseSeconds = 20;
// Where a DOS game's copy is usually kept: no installer, so no registry; the usual folder names under the system
// drive, the home folder and any Wine prefix, and a folder dropped beside the program.
rekit::SearchHints hints();

// Whether the story was imported (story/import.json in the player's folder).
bool story_imported(const std::string& user);

struct Check {
  rekit::Status::State state = rekit::Status::kMissing;
  std::string version;               // REVENGE.EXE's edition
  std::vector<std::string> missing;  // kIncomplete: in plain words
  std::string why;                   // kMissing: why this folder is not the game
};
// Whether `dir` is a playable copy: a known REVENGE.EXE in it. (After the story's import play never opens it, but it
// is what tells the copy, and the import is made again from it when the story folder goes.)
Check check_install(const std::string& dir);

// `dir` when it holds REVENGE.EXE, else the first folder under it, `depth` levels down at most (shallowest first,
// names sorted); "" when none.
std::string game_in(const std::string& dir, int depth = 2);

// The install a start uses: `remembered` (setup.txt's game_folder) while it is still the game, then the player's
// own default_install, then the search (hints(), each place looked into two levels down). `searched` says the answer
// came from the search (offered, never taken unasked). "" when none.
std::string find_install(const std::string& remembered, const std::string& user, const std::string& program_dir,
                         bool* searched);
// A copy the search finds that is none of `not_these` (the one in use, the player's own), or "": choosing again.
std::string find_other_install(const std::vector<std::string>& not_these, const std::string& user,
                               const std::string& program_dir);
// Every copy of the game on this computer, each once: `in_use` first when it is one, the player's own, then the
// search's. For the launcher's --setup --copies.
std::vector<rekit::sdl::CopyRow> copies(const std::string& in_use, const std::string& user, const std::string& program_dir);

// setup.txt's game_folder, read without SDL; "" when unset.
std::string remembered_folder(const std::string& user);
// setup.txt written with `dir` as the game folder (the player's folder made when missing). False when it can't be.
bool remember_folder(const std::string& user, const std::string& dir);

// The answer to `--status`: nothing written, no window. `version` is the program's. `remembered`: the game folder a
// start would use first (REVENGE_ASSET_DIRECTORY when set, else setup.txt's).
rekit::Status status(const std::string& remembered, const std::string& user, const std::string& program_dir,
                     const std::string& version);

// Installing what the kit's walk found into `out_dir`: a folder on disk copied (rekit::copy_folder), an archive's
// game folder unpacked in one forward read; staged in `out_dir`.new and swapped in (rekit::swap_in_folder), so a
// failure or a cancel leaves nothing and `out_dir` is replaced only when our install made it. The player's
// original (folder or archive) is only read, never moved or deleted. One at a time into a folder: `out_dir`.lock is
// locked meanwhile, and a second install is refused.
struct Installed {
  bool ok = false, cancelled = false;
  std::string dir;    // the folder now holding the game
  std::string error;  // why not, in plain words
  int files = 0;
  uint64_t bytes = 0;
};
using Progress = std::function<bool(double fraction, const std::string& file)>;  // false cancels
Installed install(const rekit::Found& found, const std::string& out_dir, const Progress& progress = nullptr);

// A released build's tag: "chr-v", then three numbers ("chr-v1.0.0"). Any other label is not one.
bool release_tag(const std::string& tag);
// This build's tag, kept in the program's file as "MW3-PROGRAM-VERSION:<tag>", where the launcher reads it without
// running the program; "" (and no such string in the file) when the build is not a release (CHR_RELEASE_TAG).
const char* build_tag();

}  // namespace chr::setup
