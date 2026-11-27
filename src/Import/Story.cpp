#include "Story.h"

#include <cstdio>
#include <fstream>
#include <iterator>

#include "../Support/Json.h"
#include "revenge/story.h"

extern "C" {
#include "revenge/sha256.h"
}

namespace revenge::import {

namespace fs = std::filesystem;
using support::json_quote;
using support::parse_json;

namespace {

fs::path manifest_path(const fs::path& write_dir) { return story_dir(write_dir) / "import.json"; }
fs::path text_path(const fs::path& write_dir) { return story_dir(write_dir) / "text.json"; }

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw ImportError("can't read " + path.string());
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) throw ImportError("can't read " + path.string());
    return bytes;
}

// The temporary file is renamed over the old one, so a reader never sees half a file.
void write_file(const fs::path& path, const std::string& contents) {
    fs::create_directories(path.parent_path());
    const fs::path temp = path.string() + ".part";
    {
        std::ofstream out(temp, std::ios::binary);
        out << contents;
        out.close();
        if (!out) throw ImportError("can't write " + temp.string());
    }
    fs::rename(temp, path);
}

std::string sha256_hex(std::string_view bytes) {
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

// A text's address as the original names it: SEG:OFF, the image at 0800h.
std::string address(const StoryTextSource& source) {
    char text[16];
    const std::uint32_t base = std::uint32_t(source.segment - 0x0800) * 16;
    std::snprintf(text, sizeof text, "%04X:%04X", unsigned(source.segment),
                  unsigned(source.imageOffset - base));
    return text;
}

}  // namespace

fs::path story_dir(const fs::path& write_dir) { return write_dir / "story"; }

std::vector<std::string> import_texts(const OriginalEdition& edition) {
    std::vector<std::string> texts(STORY_TEXT_COUNT);
    std::vector<std::string> refused;
    std::vector<std::uint8_t> bytes(0x10000);
    for (int id = 0; id < STORY_TEXT_COUNT; ++id) {
        const StoryTextSource& source = edition.storyTexts[id];
        const std::size_t length = OriginalImport_TextBytes(&source, bytes.data(), bytes.size() - 1);
        char hash[17] = "";
        if (length != std::size_t(-1) && length < bytes.size()) {
            bytes[length] = 0;
            OriginalImport_KeyedHash(bytes.data(), length + 1, hash);
        }
        if (length != source.length || std::string_view(hash) != source.keyedHash) {
            refused.push_back(std::string(StoryTextSlots[id].id) + " (" +
                              (source.imageOffset == STORY_NO_SOURCE ? std::string("no address") : address(source)) +
                              ")");
            continue;
        }
        texts[id].assign(reinterpret_cast<const char*>(bytes.data()), length);
    }
    if (!refused.empty()) {
        std::string list;
        for (const auto& name : refused) list += "\n  " + name;
        throw ImportError(std::to_string(refused.size()) + " of " + std::to_string(STORY_TEXT_COUNT) +
                          " texts aren't " + edition.name + "'s by their length or keyed hash:" + list);
    }
    return texts;
}

std::string story_table_hash(const OriginalEdition& edition) {
    std::string all;
    char row[64];
    for (int id = 0; id < STORY_TEXT_COUNT; ++id) {
        const StoryTextSource& source = edition.storyTexts[id];
        std::snprintf(row, sizeof row, " %08lX %04X %u %u ", static_cast<unsigned long>(source.imageOffset),
                      unsigned(source.segment), unsigned(source.length), unsigned(source.expanded));
        all += StoryTextSlots[id].id;
        all += row;
        all += source.keyedHash;
        all += '\n';
    }
    return sha256_hex(all);
}

StoryState story_state(const fs::path& write_dir, const OriginalEdition& edition) {
    std::error_code ec;
    if (!fs::exists(manifest_path(write_dir), ec) || !fs::exists(text_path(write_dir), ec))
        return StoryState::Missing;
    try {
        const auto manifest = parse_json(read_file(manifest_path(write_dir)));
        const auto* version = manifest.find("import_version");
        if (!version || version->number() != kImportVersion) return StoryState::Outdated;
        const auto* hash = manifest.find("table_sha256");
        if (!hash || hash->string() != story_table_hash(edition)) return StoryState::Outdated;
    } catch (const std::exception&) {
        return StoryState::Outdated;
    }
    return StoryState::Current;
}

void write_story(const fs::path& write_dir, const OriginalEdition& edition, const std::vector<std::string>& texts,
                 const fs::path& exe) {
    std::string body;
    for (int id = 0; id < STORY_TEXT_COUNT; ++id) {
        const StoryTextSource& source = edition.storyTexts[id];
        if (!body.empty()) body += ",\n";
        body += "    {\"id\": " + json_quote(StoryTextSlots[id].id) + ", \"address\": " +
                (source.imageOffset == STORY_NO_SOURCE ? std::string("null") : json_quote(address(source))) +
                ", \"text\": " + json_quote(texts[std::size_t(id)]) + "}";
    }
    // The manifest goes last: a folder without one has no finished import.
    std::error_code ec;
    fs::remove(manifest_path(write_dir), ec);
    write_file(text_path(write_dir), "{\n  \"texts\": [\n" + body + "\n  ]\n}\n");
    write_file(manifest_path(write_dir),
               "{\n  \"import_version\": " + std::to_string(kImportVersion) + ",\n  \"sources\": {" +
                   json_quote(exe.filename().string()) + ": " + json_quote(sha256_hex(read_file(exe))) +
                   "},\n  \"edition\": " + json_quote(edition.name) + ",\n  \"table_sha256\": " +
                   json_quote(story_table_hash(edition)) + "\n}\n");
}

StoryText load_story_text(const fs::path& write_dir) {
    const fs::path path = text_path(write_dir);
    try {
        const auto file = parse_json(read_file(path));
        const auto* texts = file.find("texts");
        if (!texts) throw ImportError("it has no \"texts\"");
        StoryText out;
        for (const auto& entry : texts->array()) {
            const auto* id = entry.find("id");
            const auto* text = entry.find("text");
            if (!id || !text) throw ImportError("a text without an \"id\" or a \"text\"");
            if (!out.emplace(id->string(), text->string()).second)
                throw ImportError("the id " + json_quote(id->string()) + " twice");
        }
        return out;
    } catch (const std::exception& e) {
        throw ImportError(path.string() + ": " + e.what());
    }
}

}  // namespace revenge::import
