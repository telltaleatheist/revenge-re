// The story (revenge/story.h, src/Import): the texts the game reads are the
// ones the import took from the copy, the story file keeps them, and a story
// file or a copy that isn't right stops the game naming what.
//
//   match    every text is the bytes OriginalImport_PieceBytes gives for its
//            piece, and each table entry is its text; the story file, written
//            and read back, gives the same texts; each media and data
//            piece's object holds its bytes, placed from its file in
//            story/media or story/data, which import.json lists by its
//            length and SHA-256
//   state    no story file is missing, and so is one with text.json but no
//            import.json (an import cut short), or with a media file gone; a
//            written one is current; one of another import version or table
//            is outdated
//   refuse   a text whose keyed hash isn't the table's is refused at import,
//            by its id
//   twice, stray, absent, long
//            a text.json naming an id twice, an id the game hasn't, missing
//            one, or holding a text longer than its place: a fault naming it
//   media_length
//            a media file of another length than its piece's: a fault naming
//            the file
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Story.h"
#include "original_import_for_tests.h"
#include "revenge/story.h"

extern "C" {
#include "revenge/sha256.h"
}

// revenge/combat_data.h is C only (static_assert): its one table here.
extern "C" const char* WeaponNameTable[0x17];

namespace fs = std::filesystem;
namespace story = revenge::import;

namespace {

const char* executable_path() {
    static std::string found;
    for (const auto& entry : fs::directory_iterator(std::getenv("REVENGE_GAME"))) {
        std::string name = entry.path().filename().string();
        for (auto& c : name) c = char(std::toupper(static_cast<unsigned char>(c)));
        if (name == "REVENGE.EXE") found = entry.path().string();
    }
    return found.c_str();
}

fs::path fresh_folder(const char* name) {
    const fs::path folder = fs::current_path() / (std::string("story-test-") + name);
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

std::string read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

void write(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    out << text;
}

bool is_text_kind(OriginalPieceKind kind) {
    return kind != ORIGINAL_PIECE_BYTES && kind != ORIGINAL_PIECE_MEDIA && kind != ORIGINAL_PIECE_RELOCATED_BYTES;
}

// The pieces' texts in order are the story ids in order (make_import_table.py
// names them so): each piece's bytes are its ids' texts, each with its NUL.
void check_texts_are_the_pieces() {
    const OriginalEdition* edition = OriginalImport_Edition();
    std::vector<std::uint8_t> bytes(0x20000);
    int id = 0;
    std::size_t index;
    for (index = 0; index < edition->pieceCount; ++index) {
        const OriginalPiece& piece = edition->pieces[index];
        if (!is_text_kind(piece.kind) || piece.kind == ORIGINAL_PIECE_TEXT) continue;
        const std::size_t length = OriginalImport_PieceBytes(&piece, bytes.data(), bytes.size());
        std::string joined;
        const int entries = piece.count == 0 ? 1 : piece.count;
        for (int entry = 0; entry < entries; ++entry, ++id) {
            joined += &StoryTextBuffer[StoryTextSlots[id].at];
            joined += '\0';
        }
        assert(length == joined.size() && std::memcmp(bytes.data(), joined.data(), length) == 0);
    }
    for (index = 0; index < edition->pieceCount; ++index) {
        const OriginalPiece& piece = edition->pieces[index];
        if (piece.kind != ORIGINAL_PIECE_TEXT) continue;
        const std::size_t length = OriginalImport_PieceBytes(&piece, bytes.data(), bytes.size());
        assert(std::strcmp(StoryTextSlots[id].id, piece.name) == 0);
        assert(length == std::strlen(&StoryTextBuffer[StoryTextSlots[id].at]) + 1);
        assert(std::memcmp(bytes.data(), &StoryTextBuffer[StoryTextSlots[id].at], length) == 0);
        ++id;
    }
    assert(id == STORY_TEXT_COUNT);
    // The tables are filled with their entries' places.
    for (id = 0; id < STORY_TEXT_COUNT; ++id) {
        const StoryTextSlot& slot = StoryTextSlots[id];
        if (slot.target == STORY_TARGET_TEXT || slot.target == STORY_TARGET_MUTABLE_TEXT)
            assert(*static_cast<const char* const*>(slot.destination) == &StoryTextBuffer[slot.at]);
        else if (slot.target == STORY_TARGET_ARRAY)
            assert(std::strcmp(static_cast<const char*>(slot.destination), &StoryTextBuffer[slot.at]) == 0);
    }
}

std::string sha256_hex(const std::string& bytes) {
    static const char digits[] = "0123456789abcdef";
    Sha256 hash;
    std::uint8_t digest[SHA256_DIGEST_BYTES];
    Sha256_Begin(&hash);
    Sha256_Add(&hash, bytes.data(), bytes.size());
    Sha256_End(&hash, digest);
    std::string hex;
    for (const std::uint8_t byte : digest) {
        hex += digits[byte >> 4];
        hex += digits[byte & 0x0F];
    }
    return hex;
}

// Each media and data piece's object holds the piece's bytes in the copy,
// the relocated piece's as relocated; with a story folder, its file holds
// them too and import.json lists the file.
void check_media(const fs::path* folder) {
    const OriginalEdition* edition = OriginalImport_Edition();
    std::vector<std::uint8_t> bytes(0x10000);
    const std::string manifest = folder ? read(story::story_dir(*folder) / "import.json") : std::string();
    int count = 0;
    for (std::size_t index = 0; index < edition->pieceCount; ++index) {
        const OriginalPiece& piece = edition->pieces[index];
        if (!story::is_stored(piece)) continue;
        ++count;
        const std::size_t length = OriginalImport_PieceBytes(&piece, bytes.data(), bytes.size());
        assert(length == piece.length);
        if (piece.kind == ORIGINAL_PIECE_RELOCATED_BYTES)
            OriginalImport_RelocatePiece(&piece, bytes.data(), EXPERIENCE_MASK_WINDOW_LOAD_SEGMENT);
        assert(std::memcmp(piece.target, bytes.data(), length) == 0);
        if (folder == nullptr) continue;
        const std::string id = story::piece_id(piece);
        const std::string file = read(story::story_dir(*folder) / story::piece_file(piece));
        assert(file.size() == length && std::memcmp(file.data(), bytes.data(), length) == 0);
        const std::string listed = "\"" + id + "\": {\"file\": \"" + story::piece_file(piece) + "\", \"length\": " +
                                   std::to_string(length) + ", \"sha256\": \"" + sha256_hex(file) + "\"}";
        assert(manifest.find(listed) != std::string::npos);
    }
    assert(count == 13);
}

// Clears every media and data piece's object, so a check after proves it was
// placed.
void clear_media() {
    const OriginalEdition* edition = OriginalImport_Edition();
    for (std::size_t index = 0; index < edition->pieceCount; ++index)
        if (story::is_stored(edition->pieces[index]))
            std::memset(edition->pieces[index].target, 0, edition->pieces[index].length);
}

// Each weapon's name is the story text its WeaponDefinitions record (DS:0435,
// 0Fh bytes each) points to in the copy, not a text the game reads there.
void check_weapon_names() {
    for (int weapon = 0; weapon < 0x17; ++weapon) {
        const std::size_t record = ORIGINAL_DATA_SEGMENT_IMAGE_OFFSET + 0x0435 + std::size_t(weapon) * 0x0F;
        const auto word = [&](std::size_t at) {
            return std::size_t(std::uint8_t(OriginalImage[at])) | std::size_t(std::uint8_t(OriginalImage[at + 1])) << 8;
        };
        const char* in_copy = &OriginalImage[word(record + 2) * 16 + word(record)];
        assert(WeaponNameTable[weapon] != nullptr && WeaponNameTable[weapon] != in_copy);
        assert(WeaponNameTable[weapon] >= StoryTextBuffer && WeaponNameTable[weapon] < StoryTextBuffer + STORY_TEXT_BYTES);
        assert(std::strcmp(WeaponNameTable[weapon], in_copy) == 0);
    }
}

// A current story file whose text.json is `edit` applied to the written one.
fs::path edited_story(const char* name, std::string (*edit)(std::string)) {
    const fs::path folder = fresh_folder(name);
    story::write_story(folder, *OriginalImport_Edition(), story::import_texts(*OriginalImport_Edition()),
                       story::import_pieces(*OriginalImport_Edition()), executable_path());
    const fs::path text = story::story_dir(folder) / "text.json";
    write(text, edit(read(text)));
    return folder;
}

// The first text's line, without the comma that ends it.
std::string first_entry(const std::string& text) {
    const std::size_t start = text.find("    {\"id\"");
    return text.substr(start, text.find(",\n", start) - start);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "match";
    RequireOriginalImport();
    const OriginalEdition& edition = *OriginalImport_Edition();

    if (mode == "match") {
        check_texts_are_the_pieces();
        check_weapon_names();
        check_media(nullptr);
        const std::vector<char> from_copy(StoryTextBuffer, StoryTextBuffer + STORY_TEXT_BYTES);
        const fs::path folder = fresh_folder("match");
        std::memset(StoryTextBuffer, 0, STORY_TEXT_BYTES);
        clear_media();
        StoryHost_Prepare(executable_path(), folder.string().c_str());
        assert(std::memcmp(from_copy.data(), StoryTextBuffer, STORY_TEXT_BYTES) == 0);
        check_texts_are_the_pieces();
        check_media(&folder);
        // Read again, not imported again: the same.
        std::memset(StoryTextBuffer, 0, STORY_TEXT_BYTES);
        clear_media();
        StoryHost_Prepare(executable_path(), folder.string().c_str());
        assert(std::memcmp(from_copy.data(), StoryTextBuffer, STORY_TEXT_BYTES) == 0);
        check_media(&folder);
    } else if (mode == "state") {
        const fs::path folder = fresh_folder("state");
        assert(story::story_state(folder, edition) == story::StoryState::Missing);
        story::write_story(folder, edition, story::import_texts(edition), story::import_pieces(edition),
                           executable_path());
        assert(story::story_state(folder, edition) == story::StoryState::Current);
        const fs::path manifest = story::story_dir(folder) / "import.json";
        const std::string written = read(manifest);
        std::string other = written;
        const std::string version = "\"import_version\": " + std::to_string(story::kImportVersion);
        other.replace(other.find(version), version.size(), "\"import_version\": 0");
        write(manifest, other);
        assert(story::story_state(folder, edition) == story::StoryState::Outdated);
        other = written;
        const std::size_t hash = other.find("\"table_sha256\": \"") + 17;
        other[hash] = other[hash] == '0' ? '1' : '0';
        write(manifest, other);
        assert(story::story_state(folder, edition) == story::StoryState::Outdated);
        write(manifest, written);
        assert(story::story_state(folder, edition) == story::StoryState::Current);
        fs::remove(story::story_dir(folder) / "media" / "digital_sound.bin");
        assert(story::story_state(folder, edition) == story::StoryState::Missing);
        fs::remove(manifest);
        assert(story::story_state(folder, edition) == story::StoryState::Missing);
        // A missing or outdated file is imported again at start.
        StoryHost_Prepare(executable_path(), folder.string().c_str());
        assert(story::story_state(folder, edition) == story::StoryState::Current);
    } else if (mode == "refuse") {
        std::vector<StoryTextSource> sources(edition.storyTexts, edition.storyTexts + STORY_TEXT_COUNT);
        std::string wrong = sources[STORY_ID_SAVE_GAME_SEPARATOR].keyedHash;
        wrong[0] = wrong[0] == '0' ? '1' : '0';
        sources[STORY_ID_SAVE_GAME_SEPARATOR].keyedHash = wrong.c_str();
        sources[STORY_ID_TITLE_2].length = std::uint16_t(sources[STORY_ID_TITLE_2].length + 1);
        OriginalEdition other = edition;
        other.storyTexts = sources.data();
        try {
            (void) story::import_texts(other);
            assert(!"refused");
        } catch (const story::ImportError& e) {
            const std::string message = e.what();
            assert(message.find("2 of ") == 0);
            assert(message.find("\n  title.2 (") != std::string::npos);
            assert(message.find("\n  save_game_separator (3A6A:") != std::string::npos);
        }
    } else if (mode == "media_length") {
        const fs::path folder = edited_story("media_length", [](std::string t) { return t; });
        const fs::path sound = story::story_dir(folder) / "media" / "digital_sound.bin";
        const std::string bytes = read(sound);
        write(sound, bytes.substr(0, bytes.size() - 1));
        StoryHost_Prepare(executable_path(), folder.string().c_str());
        return 0;
    } else {
        fs::path folder;
        if (mode == "twice")
            folder = edited_story("twice", [](std::string t) {
                return t.insert(t.find("    {\"id\""), first_entry(t) + ",\n");
            });
        else if (mode == "stray")
            folder = edited_story("stray", [](std::string t) {
                std::string extra = first_entry(t);
                extra.replace(extra.find("\"title.0\""), 9, "\"title.99\"");
                return t.insert(t.find("    {\"id\""), extra + ",\n");
            });
        else if (mode == "absent")
            folder = edited_story("absent", [](std::string t) {
                return t.erase(t.find("    {\"id\""), first_entry(t).size() + 2);
            });
        else
            folder = edited_story("long", [](std::string t) {
                const std::size_t text = t.find("\"text\": \"", t.find("\"title.0\"")) + 9;
                return t.insert(text, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
            });
        StoryHost_Prepare(executable_path(), folder.string().c_str());
        return 0;
    }
    std::printf("story %s: ok\n", mode.c_str());
    return 0;
}
