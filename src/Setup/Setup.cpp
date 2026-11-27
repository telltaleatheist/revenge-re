#include "Setup.h"

#include "FileCheck.h"

#include <rekit/archive.h>
#include <rekit/files.h>
#include <rekit/hash.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace chr::setup {
namespace {

// The one edition known (docs/SUPPORTED_VERSIONS.md): US DOS 1.00.
constexpr uint64_t kRevenge100Size = 256411;
constexpr const char* kRevenge100Sha256 = "c4502165e251f3e2a5005b5c0e450e6b335b14c2be9c035b0fd2d53cbb30667e";
// More than any REVENGE.EXE: a larger file is not one.
constexpr size_t kProgramLimit = 1u << 20;

std::string lower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

// `dir` ending in a separator (this system's, so a path built on it reads as one, as SDL_GetPrefPath names it).
std::string with_slash(std::string dir) {
  if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') dir += static_cast<char>(fs::path::preferred_separator);
  return dir;
}

std::string env(const char* name) {
  const char* v = std::getenv(name);
  return v ? v : "";
}

std::vector<uint8_t> read_file(const fs::path& p, size_t limit) {
  std::ifstream in(p, std::ios::binary);
  std::vector<uint8_t> out;
  if (!in) return out;
  out.resize(limit);
  in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(limit));
  out.resize(static_cast<size_t>(in.gcount()));
  return out;
}

bool is_dir(const std::string& p) {
  std::error_code ec;
  return !p.empty() && fs::is_directory(p, ec);
}

bool same_folder(const std::string& a, const std::string& b) {
  std::error_code ec;
  return !a.empty() && !b.empty() && fs::equivalent(a, b, ec);
}

// The folder's regular files' bytes, stopping once past `cap`.
bool larger_than(const std::string& dir, uint64_t cap) {
  std::error_code ec;
  uint64_t total = 0;
  for (auto it = fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec);
       !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
    std::error_code e2;
    if (it->is_regular_file(e2)) total += it->file_size(e2);
    if (total > cap) return true;
  }
  return false;
}

// One install at a time into a folder: an empty file beside it, locked while the copy is made and swapped in, and
// removed after. The system lets a lock go with the program that holds it, so a killed one's file stops nobody. A file
// system without locks stops nobody either.
class InstallLock {
 public:
  enum Got { kTaken, kBusy, kInTheWay };
  Got take(const fs::path& file) {
    // Not an empty file, so not ours: the player's, never touched.
    std::error_code ec;
    const fs::file_status there_now = fs::symlink_status(file, ec);
    if (fs::exists(there_now)) {
      if (!fs::is_regular_file(there_now)) return kInTheWay;
      const uintmax_t size = fs::file_size(file, ec);
      if (!ec && size != 0) return kInTheWay;
    }
#if defined(_WIN32)
    handle_ = CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (handle_ != INVALID_HANDLE_VALUE) return kTaken;
    const DWORD e = GetLastError();
    if (e == ERROR_SHARING_VIOLATION || e == ERROR_LOCK_VIOLATION) return kBusy;
    return e == ERROR_ACCESS_DENIED && fs::exists(file, ec) ? kBusy : kTaken;  // one being removed; else the copy says why
#else
    for (int tries = 0; tries < 100; ++tries) {
      const int fd = ::open(file.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0644);
      if (fd < 0) return kTaken;  // the copy itself says why nothing can be written there
      if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        const bool busy = errno == EWOULDBLOCK;
        ::close(fd);
        return busy ? kBusy : kTaken;
      }
      struct stat held, there;
      if (::fstat(fd, &held) == 0 && ::stat(file.c_str(), &there) == 0 && held.st_ino == there.st_ino &&
          held.st_dev == there.st_dev) {
        fd_ = fd, file_ = file;
        return kTaken;
      }
      ::close(fd);  // another's, removed between the two: again
    }
    return kBusy;
#endif
  }
  InstallLock() = default;
  InstallLock(const InstallLock&) = delete;
  InstallLock& operator=(const InstallLock&) = delete;
  ~InstallLock() {
#if defined(_WIN32)
    if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);  // and the file goes
#else
    if (fd_ < 0) return;
    ::unlink(file_.c_str());  // while it is held: nobody else's
    ::close(fd_);
#endif
  }

 private:
#if defined(_WIN32)
  HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
  int fd_ = -1;
  fs::path file_;
#endif
};

// `entry` ('/'-separated, as an archive stores it) under the game folder `root` ("" or ending '/'): its parts
// below root, made safe; false when outside root or unsafe.
bool under_root(const std::string& entry, const std::string& root, std::string* rel) {
  std::string e = entry;
  std::replace(e.begin(), e.end(), '\\', '/');
  if (lower(e.substr(0, root.size())) != lower(root)) return false;
  *rel = rekit::safe_relative(e.substr(root.size()));
  return !rel->empty();
}

// A short word for what a copy without a file lacks, by the kind of file: the words a --copies row uses.
std::string part_of(const std::string& name) {
  const std::string ext = lower(fs::path(name).extension().string());
  if (ext == ".mus") return "music";
  if (ext == ".bin") return "sounds";
  if (ext == ".map") return "maps";
  if (ext == ".dat" || ext == ".txt") return "story";
  return "pictures";
}

}  // namespace

std::string pref_folder(const char* organisation, const char* application) {
  std::string base;
#if defined(_WIN32)
  base = env("APPDATA");
#elif defined(__APPLE__)
  if (!env("HOME").empty()) base = env("HOME") + "/Library/Application Support";
#else
  base = env("XDG_DATA_HOME");
  if (base.empty() && !env("HOME").empty()) base = env("HOME") + "/.local/share";
#endif
  if (base.empty()) return "";
#if defined(_WIN32)
  return with_slash(base) + organisation + "\\" + application + "\\";
#else
  return with_slash(base) + organisation + "/" + application + "/";
#endif
}

std::string user_folder() {
  const std::string configured = env("REVENGE_SAVE_DIRECTORY");
  if (!configured.empty()) return with_slash(configured);
  return pref_folder(kOrganisation, kApplication);
}

std::string default_install(const std::string& user) {
  return user.empty() ? "" : (fs::path(user).lexically_normal() / "install").lexically_normal().string();
}

std::string edition(const std::vector<uint8_t>& bytes) {
  if (bytes.size() != kRevenge100Size) return "";
  return rekit::sha256_hex(bytes.data(), bytes.size()) == kRevenge100Sha256 ? "1.00" : "";
}

rekit::SourceRules rules() {
  rekit::SourceRules r;
  rekit::Clue program;
  program.kind = rekit::Clue::kInstall;
  program.path = "revenge.exe";
  program.limit = kProgramLimit;
  program.judge = [](const std::vector<uint8_t>& bytes) {
    rekit::Identity id;
    const std::string v = edition(bytes);
    if (v.empty()) {
      id.why = "REVENGE.EXE (" + std::to_string(bytes.size()) + " bytes) is not an edition this program knows";
      return id;
    }
    id.game = kId, id.version = v, id.rank = 1;
    return id;
  };
  r.clues.push_back(std::move(program));
  r.programs = {"revenge.exe", "setup.exe"};
  return r;
}

std::string recognise(const std::string& path) try {
  // What this program is asked for by mistake, each told by a file of its own; found by the kit's walk.
  const auto other = [](const char* game, int rank) {
    rekit::Identity id;
    id.game = game, id.rank = rank;
    return id;
  };
  rekit::SourceRules r;
  // A REVENGE.EXE of an edition not known (a known one is the game, and was found).
  r.clues.push_back({rekit::Clue::kInstall, "revenge.exe", kProgramLimit, [other](const std::vector<uint8_t>& b) {
                       return edition(b).empty() ? other("revenge", 30) : rekit::Identity{};
                     }});
  // The family's other DOS games by their program: The Crescent Hawk's Inception's and MechWarrior's are both
  // BTECH.EXE; MechWarrior 2's is MECH2.EXE.
  r.clues.push_back({rekit::Clue::kInstall, "btech.exe", kProgramLimit, [other](const std::vector<uint8_t>& b) {
                       return b.empty() ? rekit::Identity{} : other("btech", 10);
                     }});
  r.clues.push_back({rekit::Clue::kInstall, "mech2.exe", 8u << 20, [other](const std::vector<uint8_t>& b) {
                       return b.empty() ? rekit::Identity{} : other("mw2", 10);
                     }});
  r.programs = {"revenge.exe", "btech.exe", "mech2.exe"};

  const auto stop = std::chrono::steady_clock::now() + std::chrono::seconds(kRecogniseSeconds);
  const rekit::Found f =
      rekit::find_source(path, r, [stop](double, const std::string&) { return std::chrono::steady_clock::now() < stop; });
  if (f.kind == rekit::Found::kNothing) return "";
  const std::string plays = std::string(" This program plays ") + kTitle + ", the 1990 game whose program is REVENGE.EXE.";
  if (f.game == "revenge")
    return "REVENGE.EXE was found there, but it is an edition this program doesn't know. It knows one: the US DOS "
           "release 1.00.";
  if (f.game == "btech") return "That is another BattleTech game (its program is BTECH.EXE)." + plays;
  if (f.game == "mw2") return "That is MechWarrior 2, another game." + plays;
  return "";
} catch (...) {
  return "";
}

rekit::SearchHints hints() {
  rekit::SearchHints h;
  // The folder names a copy of the DOS game is kept under, alone and inside the usual Games, DOSBox and DOS
  // folders. Each found folder is looked into two levels down (find_install).
  const char* const names[] = {"Revenge", "CHRevenge", "Chrevenge", "Crescent Hawks Revenge", "BattleTech Revenge", "BTREVENG", "REVENGE"};
  const char* const parents[] = {"", "Games/", "DOSBox/", "DOS/", "GOG Games/"};
  for (const char* p : parents)
    for (const char* n : names) h.other_folders.push_back(std::string(p) + n);
  h.drop_in = true;
  return h;
}

bool story_imported(const std::string& user) {
  std::error_code ec;
  return !user.empty() && fs::is_regular_file(fs::path(with_slash(user)) / "story" / "import.json", ec);
}

Check check_install(const std::string& dir) {
  Check c;
  if (!is_dir(dir)) return c.why = "no such folder", c;
  const std::string program = rekit::any_case_path(dir, "REVENGE.EXE");
  if (program.empty()) return c.why = "no REVENGE.EXE", c;
  c.version = edition(read_file(program, kProgramLimit));
  if (c.version.empty()) return c.why = "REVENGE.EXE is not an edition this program knows", c;
  c.state = rekit::Status::kReady;
  return c;
}

std::string game_in(const std::string& dir, int depth) {
  if (!is_dir(dir)) return "";
  std::vector<std::string> level{dir};
  for (int d = 0; d <= depth && !level.empty(); ++d) {
    std::vector<std::string> next;
    for (const std::string& f : level) {
      if (!rekit::any_case_path(f, "REVENGE.EXE").empty()) return f;
      if (d == depth) continue;
      std::error_code ec;
      std::vector<std::string> subs;
      for (fs::directory_iterator it(f, fs::directory_options::skip_permission_denied, ec), end; !ec && it != end;
           it.increment(ec)) {
        std::error_code e2;
        if (!it->is_directory(e2) || it->is_symlink(e2)) continue;
        try {
          subs.push_back(it->path().string());
        } catch (const std::exception&) {  // a name that isn't text (Windows): no game is looked for under it
        }
      }
      std::sort(subs.begin(), subs.end());
      next.insert(next.end(), subs.begin(), subs.end());
    }
    level.swap(next);
  }
  return "";
}

std::string find_install(const std::string& remembered, const std::string& user, const std::string& program_dir,
                         bool* searched) {
  if (searched) *searched = false;
  const auto usable = [](const std::string& d) { return !d.empty() && check_install(d).state != rekit::Status::kMissing; };
  if (usable(remembered)) return remembered;
  const std::string own = default_install(user);
  if (usable(own)) return own;
  for (const std::string& place : rekit::install_candidates(hints(), program_dir)) {
    const std::string g = game_in(place, 2);
    if (usable(g)) {
      if (searched) *searched = true;
      return g;
    }
  }
  return "";
}

std::string find_other_install(const std::vector<std::string>& not_these, const std::string& user,
                               const std::string& program_dir) {
  (void)user;
  for (const std::string& place : rekit::install_candidates(hints(), program_dir)) {
    const std::string g = game_in(place, 2);
    if (g.empty() || check_install(g).state == rekit::Status::kMissing) continue;
    const bool known = std::any_of(not_these.begin(), not_these.end(), [&g](const std::string& d) { return same_folder(g, d); });
    if (!known) return g;
  }
  return "";
}

std::vector<rekit::sdl::CopyRow> copies(const std::string& in_use, const std::string& user, const std::string& program_dir) {
  std::vector<rekit::sdl::CopyRow> rows;
  const auto add = [&](const std::string& dir) {
    if (dir.empty()) return;
    const Check c = check_install(dir);
    if (c.state == rekit::Status::kMissing) return;
    if (std::any_of(rows.begin(), rows.end(), [&dir](const rekit::sdl::CopyRow& r) { return same_folder(r.path, dir); }))
      return;
    rekit::sdl::CopyRow row;
    row.path = dir, row.data = c.version, row.in_use = same_folder(dir, in_use);
    const FilesReport files = check_folder(dir);
    for (const auto* names : {&files.missing, &files.damaged})
      for (const std::string& name : *names)
        if (const std::string part = part_of(name);
            std::find(row.missing.begin(), row.missing.end(), part) == row.missing.end())
          row.missing.push_back(part);
    rows.push_back(std::move(row));
  };
  add(in_use);
  add(default_install(user));
  for (const std::string& place : rekit::install_candidates(hints(), program_dir)) add(game_in(place, 2));
  return rows;
}

std::string remembered_folder(const std::string& user) {
  if (user.empty()) return "";
  std::ifstream in(fs::path(with_slash(user)) / kSetupFile);
  std::string line, found;
  static constexpr char kKey[] = "game_folder";
  while (std::getline(in, line)) {
    const size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos || line.compare(start, sizeof(kKey) - 1, kKey) != 0) continue;
    const size_t eq = line.find_first_not_of(" \t", start + sizeof(kKey) - 1);
    if (eq == std::string::npos || line[eq] != '=') continue;
    std::string v = line.substr(eq + 1);
    v.erase(0, v.find_first_not_of(" \t"));
    while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back()))) v.pop_back();
    found = v;
  }
  return found;
}

bool remember_folder(const std::string& user, const std::string& dir) {
  if (user.empty() || dir.empty()) return false;
  std::error_code ec;
  fs::create_directories(user, ec);
  const fs::path file = fs::path(with_slash(user)) / kSetupFile, tmp = file.string() + ".new";
  {
    std::ofstream out(tmp, std::ios::trunc);
    out << "# The game's files, as the setup chose them. REVENGE_ASSET_DIRECTORY, when set, is used instead.\n"
        << "game_folder = " << dir << "\n";
    if (!out.flush()) return false;
  }
  fs::rename(tmp, file, ec);
  if (ec) {
    fs::remove(tmp, ec);
    return false;
  }
  return remembered_folder(user) == dir;
}

rekit::Status status(const std::string& remembered, const std::string& user, const std::string& program_dir,
                     const std::string& version) {
  rekit::Status st;
  st.game = kId, st.version = version, st.user = user;
  if (user.empty()) st.note = "the player's folder cannot be found";
  bool searched = false;
  const std::string found = find_install(remembered, user, program_dir, &searched);
  if (!found.empty()) {
    const Check c = check_install(found);
    st.where = found, st.data = c.version, st.state = c.state, st.missing = c.missing;
    if (searched) st.note = "found by the search, not set up yet";
  }
  if (!story_imported(user)) st.pending.push_back("story import");
  return st;
}

Installed install(const rekit::Found& found, const std::string& out_dir, const Progress& progress) {
  Installed r;
  const auto fail = [&r](const std::string& why) {
    r.error = why;
    return r;
  };
  if (found.kind != rekit::Found::kInstall || found.game != kId)
    return fail("That is not " + std::string(kTitle) + ": nothing was copied.");
  if (out_dir.empty()) return fail("There is no folder to copy the game into.");
  std::error_code ec;
  const fs::path dest(out_dir), tmp(out_dir + ".new"), lock_file(out_dir + ".lock");
  fs::create_directories(dest.parent_path(), ec);
  InstallLock lock;
  switch (lock.take(lock_file)) {
    case InstallLock::kTaken: break;
    case InstallLock::kBusy:
      return fail("Another setup is copying the game into " + dest.string() +
                  ". Wait until it has finished, then set the game up again.");
    case InstallLock::kInTheWay:
      return fail("The file " + lock_file.string() + " is in the way, and it was not touched. Move it somewhere else, or "
                  "delete it if you don't need it, then set the game up again.");
  }
  // tmp is ours only when it carries the marker (an earlier run's, stopped); else it is the player's, and never
  // deleted. It is made and marked before anything goes in it, so a run killed at any point leaves one that is ours.
  if (fs::exists(tmp, ec) && !fs::exists(tmp / kInstallMarker, ec) && !(fs::is_directory(tmp, ec) && fs::is_empty(tmp, ec)))
    return fail("The folder " + tmp.string() + " is in the way, and nothing in it was touched. Move that folder "
                "somewhere else, or delete it if you don't need it, then set the game up again.");
  const auto stage = [&](const std::string& from) -> std::string {  // "" or why not
    std::error_code e;
    if (fs::exists(tmp / kInstallMarker, e)) fs::remove_all(tmp, e);
    fs::create_directories(tmp, e);
    if (e) return "Could not make " + tmp.string() + ": " + e.message() + ".";
    std::ofstream mark(tmp / kInstallMarker);
    mark << from << std::flush;
    return mark ? "" : "Could not write in " + tmp.string() + ".";
  };
  fs::path ready = tmp;  // what is swapped in

  if (found.on_disk && found.steps.empty()) {
    // An installed copy in a folder: copied, never moved.
    const std::string from = (fs::path(found.path) / found.root).lexically_normal().string();
    const std::string src = is_dir(from) ? from : fs::path(from).parent_path().string();
    if (fs::exists(dest, ec) && same_folder(src, dest.string())) {
      r.ok = true, r.dir = dest.string();
      return r;
    }
    const fs::path real = fs::weakly_canonical(src, ec), staged = fs::weakly_canonical(tmp, ec);
    if (std::mismatch(staged.begin(), staged.end(), real.begin(), real.end()).first == staged.end())
      return fail("The folder " + src + " is a copy this setup didn't finish: pick your own copy.");
    if (larger_than(src, kMaxGameBytes))
      return fail("The folder " + src + " holds far more than " + std::string(kTitle) +
                  ", which is about 2 MB: nothing was copied. Put the game's files in a folder of their own and pick "
                  "that.");
    // Copied into a folder inside the marked one (the kit copies only into a new one).
    const std::string from_words = "Copied from " + src + " by the setup (the original was left as it was).\n";
    if (const std::string why = stage(from_words); !why.empty()) return fail(why);
    ready = tmp / "copy";
    const rekit::CopyResult c = rekit::copy_folder(src, ready.string(), [&](uint64_t done, uint64_t total, const std::string& f) {
      return !progress || progress(total ? double(done) / double(total) : 0.0, f);
    });
    if (!c.ok) {
      fs::remove_all(tmp, ec);  // ours: marked above
      r.cancelled = c.cancelled;
      return fail(c.cancelled ? "Stopped. Nothing was copied." : "Could not copy the game from " + src + ": " + c.error);
    }
    r.files = c.files, r.bytes = c.bytes;
    std::ofstream(ready / kInstallMarker) << from_words;
  } else {
    // In an archive (at any depth): the game's folder read out in one forward pass.
    if (!rekit::archives_supported()) return fail("This build cannot open archives: unpack it and pick its REVENGE.EXE.");
    std::vector<std::unique_ptr<rekit::ArchiveReader>> chain;
    chain.push_back(std::make_unique<rekit::ArchiveReader>());
    if (!chain.back()->open_file(found.path)) return fail("Could not open " + found.path + " (" + chain.back()->error() + ").");
    rekit::ArchiveReader::Entry e;
    for (const rekit::Step& s : found.steps) {
      if (s.kind != rekit::Step::kArchive)
        return fail("The game is inside a disc image: that is not how " + std::string(kTitle) + " came.");
      bool at = false;
      while (chain.back()->next(&e))
        if (e.path == s.entry) {
          at = true;
          break;
        }
      if (!at) return fail("Could not find " + s.entry + " again in " + found.path + ".");
      auto inner = std::make_unique<rekit::ArchiveReader>();
      if (!inner->open_stream(chain.back()->entry_stream()))
        return fail("Could not open " + s.entry + " (" + inner->error() + ").");
      chain.push_back(std::move(inner));
    }
    rekit::ArchiveReader& a = *chain.back();
    if (const std::string why = stage("Unpacked from " + found.path + " by the setup (the archive was left as it was).\n");
        !why.empty())
      return fail(why);
    std::vector<uint8_t> buf(1 << 16);
    std::string rel, why;
    const int64_t whole = [&] {
      std::error_code e3;
      const auto n = fs::file_size(found.path, e3);
      return e3 ? int64_t(0) : static_cast<int64_t>(n);
    }();
    while (why.empty() && a.next(&e)) {
      if (e.other || !under_root(e.path, found.root, &rel)) continue;
      if (lower(fs::path(rel).filename().string()) == kInstallMarker) continue;
      const fs::path out = tmp / fs::path(rel);
      if (e.dir) {
        fs::create_directories(out, ec);
        continue;
      }
      fs::create_directories(out.parent_path(), ec);
      std::ofstream f(out, std::ios::binary);
      if (!f) why = "Could not write " + out.string() + ".";
      for (int64_t n; why.empty() && (n = a.read(buf.data(), buf.size())) != 0;) {
        if (n < 0) why = "That archive is damaged (" + a.error() + ").";
        else if ((r.bytes += uint64_t(n)) > kMaxGameBytes) why = "That archive holds far more than the game: nothing was copied.";
        else f.write(reinterpret_cast<const char*>(buf.data()), n);
      }
      if (why.empty() && !f) why = "Could not write " + out.string() + ".";
      ++r.files;
      if (why.empty() && progress && !progress(whole ? double(chain.front()->position()) / double(whole) : 0.0, rel))
        r.cancelled = true, why = "Stopped. Nothing was copied.";
    }
    if (why.empty() && !a.error().empty()) why = "That archive is damaged (" + a.error() + ").";
    if (!why.empty()) {
      fs::remove_all(tmp, ec);  // ours: made above with the marker
      return fail(why);
    }
  }
  if (check_install(ready.string()).state == rekit::Status::kMissing) {
    fs::remove_all(tmp, ec);
    return fail("The copy has no REVENGE.EXE this program knows: nothing was changed.");
  }
  if (!rekit::swap_in_folder(ready.string(), dest.string(), kInstallMarker, ec)) {
    const std::string why = ec.message();
    fs::remove_all(tmp, ec);
    return fail("Could not put the game in " + dest.string() + " (" + why + "): nothing was changed.");
  }
  fs::remove_all(tmp, ec);  // the marked folder the copy was made in
  r.ok = true, r.dir = dest.string();
  return r;
}

bool release_tag(const std::string& tag) {
  static constexpr char kPrefix[] = "chr-v";
  if (tag.compare(0, sizeof(kPrefix) - 1, kPrefix) != 0) return false;
  int numbers = 0;
  bool digit = false;
  for (size_t i = sizeof(kPrefix) - 1; i <= tag.size(); ++i) {
    if (i < tag.size() && std::isdigit(static_cast<unsigned char>(tag[i]))) {
      digit = true;
      continue;
    }
    if (!digit) return false;
    ++numbers, digit = false;
    if (i < tag.size() && tag[i] != '.') return false;
  }
  return numbers == 3;
}

}  // namespace chr::setup
