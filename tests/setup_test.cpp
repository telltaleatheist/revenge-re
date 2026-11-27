// The setup's recipe (src/Setup/Setup.h, FileCheck.h) without a window: the edition by its bytes, the release tag,
// the remembered folder, the file check's verdicts and words, and what a pick that isn't the game is said to be. With a
// copy of the game (REVENGE_GAME): it is the 1.00 edition, its files are all known, it is installed from a folder into
// the work folder and found there, and the original is left as it was. Prints names and counts only; nothing of the
// game is kept.
//
//   setup_test WORK_DIR

#include "Setup/FileCheck.h"
#include "Setup/Setup.h"

#include <rekit/status.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace setup = chr::setup;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) ++failures;
}

void write(const fs::path& p, const std::string& text) {
  fs::create_directories(p.parent_path());
  std::ofstream(p, std::ios::binary) << text;
}

std::vector<std::string> tree(const fs::path& root) {
  std::vector<std::string> out;
  std::error_code ec;
  for (auto it = fs::recursive_directory_iterator(root, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
    out.push_back(fs::relative(it->path(), root).generic_string());
  std::sort(out.begin(), out.end());
  return out;
}

void game_free(const fs::path& work) {
  check(setup::edition({}).empty(), "edition: nothing is no edition");
  check(setup::edition(std::vector<uint8_t>(256411, 0)).empty(), "edition: the right size with other bytes is none");
  check(setup::release_tag("chr-v1.0.0") && setup::release_tag("chr-v12.3.45"), "release_tag: chr-vX.Y.Z");
  for (const char* bad : {"", "chr-v1.0", "chr-v1.0.0.1", "chr-v1..0", "mw1r-v1.0.0", "chr-v1.0.0-rc1", "chr-vA.0.0"})
    check(!setup::release_tag(bad), std::string("release_tag: not \"") + bad + "\"");
  check(std::string(setup::build_tag()).empty(), "build_tag: none in a test build");

  const fs::path user = work / "user";
  check(setup::remembered_folder(user.string()).empty(), "remembered: nothing before the setup");
  check(setup::remember_folder(user.string(), "/some/where"), "remember: written");
  check(setup::remembered_folder(user.string()) == "/some/where", "remember: read back");
  check(setup::remember_folder(user.string(), "/else/where") && setup::remembered_folder(user.string()) == "/else/where",
        "remember: replaced");
  check(tree(user) == std::vector<std::string>{setup::kSetupFile}, "remember: setup.txt alone, no temporary left");
  check(!setup::story_imported(user.string()), "story: not imported");
  write(user / "story" / "import.json", "{}");
  check(setup::story_imported(user.string()), "story: imported");
  check(setup::default_install(user.string()) == (user / "install").lexically_normal().string(), "the default install");

  const fs::path notgame = work / "notgame";
  write(notgame / "README.TXT", "not a game\n");
  check(setup::check_install(notgame.string()).state == rekit::Status::kMissing, "check_install: no REVENGE.EXE");
  check(setup::recognise(notgame.string()).empty(), "recognise: nothing to say of an empty pick");
  const fs::path other = work / "other";
  write(other / "REVENGE.EXE", std::string(300, 'x'));
  check(setup::check_install(other.string()).why == "REVENGE.EXE is not an edition this program knows",
        "check_install: another edition");
  check(setup::recognise(other.string()).find("an edition this program doesn't know") != std::string::npos,
        "recognise: another edition of REVENGE.EXE");
  const fs::path btech = work / "btech";
  write(btech / "BTECH.EXE", "MZ");
  check(setup::recognise(btech.string()).find("another BattleTech game") != std::string::npos, "recognise: BTECH.EXE");
  check(setup::game_in(work.string(), 2) == other.string(), "game_in: the folder holding REVENGE.EXE");

  // The file check on a table of its own.
  const setup::GameFile files[] = {
      {"A.DAT", 3, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", setup::GameFile::When::kStart, "can't start"},
      {"B.MUS", 1, "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb", setup::GameFile::When::kSome, "later"},
  };
  const fs::path folder = work / "files";
  write(folder / "a.dat", "abc");
  write(folder / "B.MUS", "a");
  setup::FilesReport r = setup::check_folder(folder.string(), files, {});
  check(r.all_known() && setup::files_note(r).empty() && setup::files_status_note(r).empty(), "files: all known, any case");
  write(folder / "B.MUS", "b");
  r = setup::check_folder(folder.string(), files, {});
  check(r.different == std::vector<std::string>{"B.MUS"} && r.missing.empty(), "files: one differs");
  fs::remove(folder / "a.dat");
  r = setup::check_folder(folder.string(), files, {});
  check(r.missing == std::vector<std::string>{"A.DAT"} && r.effects == std::vector<std::string>{"can't start"},
        "files: one missing, and what that does");
  check(setup::files_note(r).find("Missing: A.DAT.") != std::string::npos, "files: the note names it");
  check(setup::files_status_note(r).rfind("Missing, damaged or different: ", 0) == 0, "files: the status's line");
  check(!setup::check_folder((work / "nowhere").string(), files, {}).unchecked.empty(), "files: no folder, not checked");
  check(!setup::game_files().empty() && setup::known_damage().empty(), "files: the table, and no known damage");

  rekit::Found nothing;
  check(!setup::install(nothing, (work / "into").string()).ok && !fs::exists(work / "into"), "install: nothing found, nothing made");
}

void with_game(const fs::path& work, const fs::path& game) {
  const setup::Check c = setup::check_install(game.string());
  check(c.state == rekit::Status::kReady && c.version == "1.00", "the copy: REVENGE.EXE 1.00");
  const setup::FilesReport r = setup::check_folder(game.string());
  check(r.all_known(), "the copy: " + setup::files_log(r));
  const rekit::Found f = rekit::find_source(game.string(), setup::rules());
  check(f.kind == rekit::Found::kInstall && f.game == setup::kId && f.version == "1.00", "the walk finds it");
  const std::vector<std::string> before = tree(game);
  const fs::path user = work / "game-user";
  const setup::Installed in = setup::install(f, setup::default_install(user.string()));
  check(in.ok && in.files > 0 && fs::is_regular_file(fs::path(in.dir) / "REVENGE.EXE"), "install: copied");
  check(tree(game) == before, "install: the original left as it was");
  check(setup::check_folder(in.dir).all_known(), "install: every file as it was");
  check(!fs::exists(in.dir + ".new") && !fs::exists(in.dir + ".lock"), "install: nothing left beside it");
  bool searched = true;
  check(setup::find_install("", user.string(), "", &searched) == in.dir && !searched, "find_install: the player's own install");
  const std::vector<rekit::sdl::CopyRow> rows = setup::copies(in.dir, user.string(), "");
  check(rows.size() == 1 && rows[0].in_use && rows[0].data == "1.00" && rows[0].missing.empty(), "copies: the install, in use");
  const rekit::Status st = setup::status("", user.string(), "", "9.9.9");
  check(st.state == rekit::Status::kReady && st.where == in.dir && st.data == "1.00" && st.game == "chr", "status: ready");
  const setup::Installed again = setup::install(f, in.dir);
  check(again.ok && setup::check_install(in.dir).state == rekit::Status::kReady, "install: again over our own install");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: setup_test WORK_DIR\n");
    return 2;
  }
  const fs::path work(argv[1]);
  std::error_code ec;
  fs::remove_all(work, ec);
  fs::create_directories(work);
  game_free(work);
  const char* game = std::getenv("REVENGE_GAME");
  if (game && *game) with_game(work, game);
  else std::printf("skipped: the copy's checks (no REVENGE_GAME)\n");
  std::printf("%d failed\n", failures);
  return failures ? 1 : 0;
}
