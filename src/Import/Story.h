// The story file: the game's text, imported once from the player's own
// REVENGE.EXE into our write folder, as the MW1 rebuild (mech1-re) keeps its
// own:
//
//   <write dir>/story/text.json     every text: its id, its address in the
//                                   original's memory and the text itself
//   <write dir>/story/media/<id>.bin
//                                   each of the media pieces (the sounds, the
//                                   palette, the digram tables), as it is in
//                                   the original's memory
//   <write dir>/story/import.json   what was imported: the import version, the
//                                   exe's SHA-256, the edition, the table's
//                                   hash, and each media file's length and
//                                   SHA-256
//
// import.json is written last, so an interrupted import counts as missing.
// Text keeps the game's control codes and 8-bit characters as \u00NN. The
// ids are the story's (revenge/story_text.h); src/Import/StoryHost.cpp is
// the game's door to it.
#pragma once

#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "revenge/original_import.h"

namespace revenge::import {

struct ImportError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Raised the story file's version when what it holds, or how, changes.
inline constexpr int kImportVersion = 2;

std::filesystem::path story_dir(const std::filesystem::path& write_dir);

// Every text, by StoryTextId, from the image OriginalImport_Load accepted.
// A text whose length or keyed hash isn't the edition's table's is refused:
// ImportError names each such id and its address.
std::vector<std::string> import_texts(const OriginalEdition& edition);

// A media piece (ORIGINAL_PIECE_MEDIA) and its bytes. Its id is its name in
// lower case, words split by "_", without "Data" or "Table" at its end:
// DigitalSoundData is digital_sound.
struct Media {
    const OriginalPiece* piece;
    std::string id;
    std::string bytes;
};

std::string media_id(const OriginalPiece& piece);

// Every media piece's bytes from the image OriginalImport_Load accepted. One
// whose length or keyed hash isn't the edition's table's is refused, as a
// text is.
std::vector<Media> import_media(const OriginalEdition& edition);

// The hash of the edition's story table (ids, places, lengths, keyed hashes,
// and the media pieces'): a story file made under another table is outdated.
std::string story_table_hash(const OriginalEdition& edition);

enum class StoryState { Current, Missing, Outdated };
StoryState story_state(const std::filesystem::path& write_dir, const OriginalEdition& edition);

// Writes texts (import_texts's) as the story file in write_dir; exe is the
// file they came from, for import.json.
void write_story(const std::filesystem::path& write_dir, const OriginalEdition& edition,
                 const std::vector<std::string>& texts, const std::vector<Media>& media,
                 const std::filesystem::path& exe);

using StoryText = std::map<std::string, std::string, std::less<>>;

// story/text.json, id -> text. Throws ImportError naming the file if it can't
// be read, isn't the story file's shape, or names an id twice.
StoryText load_story_text(const std::filesystem::path& write_dir);

// story/media's files, one for each of the edition's media pieces. Throws
// ImportError naming the file if one can't be read or isn't its piece's
// length.
std::vector<Media> load_story_media(const std::filesystem::path& write_dir, const OriginalEdition& edition);

}  // namespace revenge::import
