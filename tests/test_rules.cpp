// The rules (revenge/rules.h, src/Rules): data/rules rebuilds each of the
// original's tables the rule table names, byte for byte, and a rules file
// that isn't right stops the program naming the file and the key.
//
//   test_rules from_data every table from data/rules hashes as the rule table
//                        says (no copy of the game needed)
//   test_rules in_copy   the player's copy (REVENGE_GAME) holds those bytes:
//                        each place's keyed hash, the image as DOS loads it
//   test_rules MODE      for the faults (CMakeLists.txt): data/rules with one
//                        thing wrong, which must stop Rules_Load
//
// Prints ids and counts only, never the game's bytes.
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "original_import_for_tests.h"
#include "rules_for_tests.h"

namespace fs = std::filesystem;

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("FAIL %s\n", what.c_str());
    }
}

std::string hash_of(const std::uint8_t* bytes, std::size_t length) {
    char hex[17];
    OriginalImport_KeyedHash(bytes, length, hex);
    return hex;
}

int rebuild() {
    RequireRules();
    std::size_t tuned = 0;
    for (std::size_t index = 0; index < RulePieceCount; ++index) {
        const RulePiece& piece = RulePieces[index];
        check(piece.size == piece.length, std::string(piece.id) + ": its object isn't its length");
        if (piece.keyedHash[0] == '\0') {
            ++tuned;
            continue;
        }
        check(hash_of(static_cast<const std::uint8_t*>(piece.object), piece.size) == piece.keyedHash,
              std::string(piece.id) + ": data/rules doesn't rebuild the original's bytes");
    }
    std::printf("rules.rebuild: %zu tables, %zu tuned, %d wrong\n", RulePieceCount, tuned, g_failures);
    return g_failures == 0 ? 0 : 1;
}

// The image as DOS loads it at 0800h: each relocated segment word moved.
std::vector<std::uint8_t> loaded_image() {
    std::vector<std::uint8_t> image(OriginalImage, OriginalImage + OriginalImport_ImageBytes());
    for (std::size_t index = 0; index < OriginalImport_RelocationCount(); ++index) {
        const std::size_t at = OriginalImport_Relocation(index);
        if (at + 1 >= image.size()) continue;
        const unsigned value = (image[at] | image[at + 1] << 8) + 0x0800;
        image[at] = std::uint8_t(value & 0xFF);
        image[at + 1] = std::uint8_t(value >> 8);
    }
    return image;
}

int exe() {
    RequireOriginalImport();
    const std::vector<std::uint8_t> image = loaded_image();
    std::size_t checked = 0;
    for (std::size_t index = 0; index < RulePieceCount; ++index) {
        const RulePiece& piece = RulePieces[index];
        if (piece.keyedHash[0] == '\0') continue;
        const std::size_t at = std::size_t(piece.segment - 0x0800) * 16 + piece.offset;
        check(at + piece.length <= image.size() && hash_of(&image[at], piece.length) == piece.keyedHash,
              std::string(piece.id) + ": the copy doesn't hold the rule table's bytes");
        ++checked;
    }
    std::printf("rules.exe: %zu tables checked, %d wrong\n", checked, g_failures);
    return g_failures == 0 ? 0 : 1;
}

std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// data/rules copied to a folder of the test's, with `from` made `to` in
// `file` (or with no `file` when `from` is null).
std::string broken_data(const char* mode, const char* file, const char* from, const char* to) {
    const fs::path folder = fs::current_path() / (std::string("rules-test-") + mode);
    fs::remove_all(folder);
    fs::create_directories(folder / "rules");
    for (const fs::directory_entry& entry : fs::directory_iterator(fs::path(CHR_DATA_DIRECTORY) / "rules"))
        fs::copy_file(entry.path(), folder / "rules" / entry.path().filename());
    const fs::path target = folder / "rules" / file;
    if (from == nullptr) {
        fs::remove(target);
        return folder.string();
    }
    std::string text = read(target);
    const std::size_t at = text.find(from);
    if (at == std::string::npos) {
        std::printf("FAIL %s: %s has no %s\n", mode, file, from);
        std::exit(1);
    }
    text.replace(at, std::strlen(from), to);
    std::ofstream(target, std::ios::binary) << text;
    return folder.string();
}

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "from_data";
    if (mode == "from_data") return rebuild();
    if (mode == "in_copy") return exe();

    struct Broken {
        const char* mode;
        const char* file;
        const char* from;
        const char* to;
    };
    static const Broken broken[] = {
        {"stray", "weapons.json", "\"small_laser\": {", "\"small_laser\": {\"rate_of_fire\": 1, "},
        {"absent", "weapons.json", "\"ppc\": {\"heat\": 10, ", "\"ppc\": {"},
        {"too_large", "weapons.json", "\"ppc\": {\"heat\": 10,", "\"ppc\": {\"heat\": 300,"},
        {"length", "weapons.json", "[45, ", "["},
        {"shared", "weapons.json", "\"improved_missile_damage_bonus\": 0}", "\"improved_missile_damage_bonus\": 1}"},
        {"location", "combat.json", "[\"center_rear_torso\", ", "[\"tail\", "},
        {"direction", "combat.json", "\"hit_table_by_direction\": [0,", "\"hit_table_by_direction\": [4,"},
        {"engagement", "ai.json", "\"engagement_by_comparison_mask\": [\n    \"withdraw\"",
         "\"engagement_by_comparison_mask\": [\n    \"retreat\""},
        {"overfull", "ai.json", "\"weapon_priorities\": [\n    [\n",
         "\"weapon_priorities\": [\n    [\n      {\"if_ready\": [\"ppc\"], \"fire\": \"ppc\"},\n"},
        {"nofile", "combat.json", nullptr, nullptr},
    };
    for (const Broken& one : broken)
        if (mode == one.mode) {
            const std::string folder = broken_data(one.mode, one.file, one.from, one.to);
            Rules_Load(folder.c_str());
            std::printf("FAIL %s: the rules were read\n", one.mode);
            return 1;
        }
    std::printf("test_rules: no mode %s\n", mode.c_str());
    return 2;
}
