#include "FileCheck.h"

#include "Setup.h"

#include <rekit/archive.h>
#include <rekit/hash.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>

namespace fs = std::filesystem;

namespace chr::setup {
namespace {

std::string upper(std::string s) {
  for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return s;
}

// Whether a file of `name` with `size` could be one this program knows: the rest are never hashed.
bool worth_hashing(const std::string& name, uint64_t size, std::span<const GameFile> files, std::span<const KnownDamage> damage) {
  const auto is = [&](const auto& f) { return name == f.name && size == f.size; };
  return std::any_of(files.begin(), files.end(), is) || std::any_of(damage.begin(), damage.end(), is);
}
uint64_t largest(const std::string& name, std::span<const GameFile> files, std::span<const KnownDamage> damage) {
  uint64_t most = 0;
  for (const GameFile& f : files)
    if (name == f.name) most = std::max<uint64_t>(most, f.size);
  for (const KnownDamage& d : damage)
    if (name == d.name) most = std::max<uint64_t>(most, d.size);
  return most;
}

std::string sha(const std::vector<uint8_t>& bytes) { return rekit::sha256_hex(bytes.data(), bytes.size()); }

// "A, B, C and 4 more".
std::string some(const std::vector<std::string>& names, size_t most = kNamed, const char* between = ", ") {
  std::string out;
  for (size_t i = 0; i < names.size() && i < most; ++i) out += (i ? between : "") + names[i];
  if (names.size() > most) out += " and " + std::to_string(names.size() - most) + " more";
  return out;
}
std::string all(const std::vector<std::string>& names) { return names.empty() ? "none" : some(names, names.size()); }

FilesReport not_checked(const std::string& why) {
  FilesReport r;
  r.unchecked = why;
  return r;
}

using Seen = std::map<std::string, SeenFile>;  // by the name in upper case
FilesReport check_seen(const Seen& seen, std::span<const GameFile> files, std::span<const KnownDamage> damage) {
  return check_files(
      [&seen](const std::string& name) {
        const auto it = seen.find(name);
        return it == seen.end() ? std::nullopt : std::optional<SeenFile>(it->second);
      },
      files, damage);
}

FilesReport check_archive(const rekit::Found& found) {
  if (!rekit::archives_supported()) return not_checked("this build cannot open archives");
  std::error_code ec;
  const uint64_t whole = fs::file_size(found.path, ec);
  if (ec) return not_checked("the archive could not be read");
  if (whole > kMaxCheckedArchive) return not_checked("the archive is over " + std::to_string(kMaxCheckedArchive >> 20) + " MB");
  const auto damaged = [](const std::string& why) { return not_checked("the archive could not be read (" + why + ")"); };
  std::vector<std::unique_ptr<rekit::ArchiveReader>> chain;
  chain.push_back(std::make_unique<rekit::ArchiveReader>());
  if (!chain.back()->open_file(found.path)) return damaged(chain.back()->error());
  rekit::ArchiveReader::Entry e;
  for (const rekit::Step& s : found.steps) {
    if (s.kind != rekit::Step::kArchive) return not_checked("the game is in a disc image");
    bool at = false;
    while (!at && chain.back()->next(&e)) at = e.path == s.entry;
    if (!at) return damaged("an entry is gone");
    auto inner = std::make_unique<rekit::ArchiveReader>();
    if (!inner->open_stream(chain.back()->entry_stream())) return damaged(inner->error());
    chain.push_back(std::move(inner));
  }
  rekit::ArchiveReader& a = *chain.back();
  Seen seen;
  uint64_t read_in_all = 0;
  const std::string root = upper(found.root);
  while (a.next(&e)) {
    if (e.other || e.dir) continue;
    std::string path = upper(e.path);
    std::replace(path.begin(), path.end(), '\\', '/');
    if (path.compare(0, root.size(), root) != 0) continue;
    const std::string name = path.substr(root.size());
    const uint64_t most = largest(name, game_files(), known_damage());
    if (!most) continue;  // not a listed file, or one in a folder below
    // At most one byte more than the largest one known: what is longer differs, whatever it holds.
    std::vector<uint8_t> bytes(static_cast<size_t>(most) + 1);
    size_t have = 0;
    for (int64_t n; have < bytes.size() && (n = a.read(bytes.data() + have, bytes.size() - have)) != 0;
         have += static_cast<size_t>(n))
      if (n < 0) return damaged(a.error());
    bytes.resize(have);
    if ((read_in_all += have) > kMaxCheckedArchive) return not_checked("the archive holds too much");
    SeenFile f{have, {}};
    if (worth_hashing(name, have, game_files(), known_damage())) f.sha256 = sha(bytes);
    seen.insert_or_assign(name, std::move(f));  // as unpacking would: the last one stays
  }
  if (!a.error().empty()) return damaged(a.error());
  return check_seen(seen, game_files(), known_damage());
}

}  // namespace

FilesReport check_files(const FileReader& reader, std::span<const GameFile> files, std::span<const KnownDamage> damage) {
  FilesReport r;
  for (const GameFile& f : files) {
    const std::optional<SeenFile> seen = reader(f.name);
    if (seen && seen->size == f.size && seen->sha256 == f.sha256) continue;
    const bool known = seen && std::any_of(damage.begin(), damage.end(), [&](const KnownDamage& d) {
      return std::string(d.name) == f.name && seen->size == d.size && seen->sha256 == d.sha256;
    });
    if (seen && !known) {
      r.different.push_back(f.name);
      continue;
    }
    (known ? r.damaged : r.missing).push_back(f.name);
    if (std::find(r.effects.begin(), r.effects.end(), f.without) == r.effects.end()) r.effects.push_back(f.without);
  }
  return r;
}

FilesReport check_folder(const std::string& dir, std::span<const GameFile> files, std::span<const KnownDamage> damage) try {
  Seen seen;
  std::error_code ec;
  fs::directory_iterator it(dir, ec), end;
  for (; !ec && it != end; it.increment(ec)) {
    std::string name;
    try {
      name = upper(it->path().filename().string());
    } catch (const std::exception&) {
      continue;  // a name this system's code page can't hold is no listed file's
    }
    const uint64_t most = largest(name, files, damage);
    std::error_code e2;
    if (!most || !it->is_regular_file(e2)) continue;
    SeenFile f{it->file_size(e2), {}};
    if (e2) f.size = 0;
    if (worth_hashing(name, f.size, files, damage)) {
      std::vector<uint8_t> bytes(static_cast<size_t>(f.size));
      std::ifstream in(it->path(), std::ios::binary);
      in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
      if (in && static_cast<uint64_t>(in.gcount()) == f.size) f.sha256 = sha(bytes);
    }
    seen.emplace(name, std::move(f));
  }
  if (ec) return not_checked("the folder could not be read (" + ec.message() + ")");
  return check_seen(seen, files, damage);
} catch (const std::exception& e) {
  return not_checked(std::string("the folder could not be read (") + e.what() + ")");
}

FilesReport check_found(const rekit::Found& found) try {
  if (found.kind != rekit::Found::kInstall || found.game != kId) return not_checked("no copy of the game");
  if (!found.on_disk || !found.steps.empty()) return check_archive(found);
  const fs::path from = (fs::path(found.path) / found.root).lexically_normal();
  std::error_code ec;
  return check_folder((fs::is_directory(from, ec) ? from : from.parent_path()).string());
} catch (const std::exception& e) {
  return not_checked(std::string("it could not be read (") + e.what() + ")");
}

std::string files_note(const FilesReport& r) {
  if (!r.unchecked.empty()) return "Its files weren't checked: " + r.unchecked + ".";
  if (r.all_known()) return "";
  std::string out;
  const auto add = [&out](const std::string& sentence) { out += (out.empty() ? "" : " ") + sentence; };
  if (!r.missing.empty()) add("Missing: " + some(r.missing) + ".");
  if (!r.damaged.empty()) add("Known to be damaged: " + some(r.damaged) + ".");
  if (!r.effects.empty()) add("As it is, " + some(r.effects, 2, "; ") + ".");
  if (!r.different.empty())
    add((r.different.size() == 1 ? "1 file differs" : std::to_string(r.different.size()) + " files differ") +
        " from the copy this program knows: " + some(r.different) + ".");
  add(r.missing.empty() && r.damaged.empty() ? "You can continue. A changed or different edition may still work."
                                             : "You can continue, or choose another copy.");
  return out;
}

std::string files_pick_note(const FilesReport& r) {
  if (!r.unchecked.empty() || r.all_known()) return files_note(r);
  const auto count = [](const std::vector<std::string>& v, const char* plural) {
    return v.size() == 1 ? v[0] : std::to_string(v.size()) + plural;
  };
  if (r.missing.empty() && r.damaged.empty())
    return count(r.different, " files") + (r.different.size() == 1 ? " differs" : " differ") +
           " from the copy this program knows. A changed or different edition may still work. You can continue.";
  std::string out;
  const auto add = [&out](const std::string& part) { out += (out.empty() ? "" : ", ") + part; };
  if (!r.missing.empty()) add(count(r.missing, " files") + " missing");
  if (!r.damaged.empty()) add(count(r.damaged, " files") + " damaged");
  if (!r.different.empty()) add(count(r.different, " files") + " different");
  out += ": " + (r.effects.empty() ? std::string("something may stop") : r.effects[0]) +
         (r.effects.size() > 1 ? ", and more" : "") + ".";
  return out + " You can continue.";
}

std::string files_status_note(const FilesReport& r) {
  if (!r.unchecked.empty() || r.all_known()) return "";
  std::vector<std::string> names = r.missing;
  names.insert(names.end(), r.damaged.begin(), r.damaged.end());
  names.insert(names.end(), r.different.begin(), r.different.end());
  std::string out = r.missing.empty() && r.damaged.empty()
                        ? "Changed or a different edition, and may still work: "
                        : std::string("Missing") + (r.damaged.empty() && r.different.empty() ? ": " : ", damaged or different: ");
  if (r.missing.empty() && !r.damaged.empty()) out = r.different.empty() ? "Damaged: " : "Damaged or different: ";
  return out + some(names) + ".";
}

std::string files_log(const FilesReport& r) {
  if (!r.unchecked.empty()) return "files: not checked: " + r.unchecked;
  if (r.all_known()) return "files: all known";
  return "files: missing: " + all(r.missing) + "; known to be damaged: " + all(r.damaged) +
         "; different from the copy this program knows: " + all(r.different);
}

}  // namespace chr::setup
