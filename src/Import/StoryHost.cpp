// The game's door to the story file (revenge/story.h): imported from the
// player's copy when it is missing or outdated, then read, and every text and
// media piece put in its place before the game runs.
#include "revenge/story.h"

#include <cstdio>
#include <cstring>
#include <exception>
#include <vector>

#include "Story.h"

#include "revenge/debug.h"

namespace {

void place_all(const std::vector<std::string>& texts) {
    for (int id = 0; id < STORY_TEXT_COUNT; ++id)
        StoryText_Place(static_cast<StoryTextId>(id), texts[std::size_t(id)].data(), texts[std::size_t(id)].size());
}

void place_media(const std::vector<revenge::import::Media>& media) {
    for (const auto& item : media) std::memcpy(item.piece->target, item.bytes.data(), item.bytes.size());
}

void place_file(const std::filesystem::path& write_dir) {
    const auto story = revenge::import::load_story_text(write_dir);
    const auto path = (revenge::import::story_dir(write_dir) / "text.json").string();
    std::vector<bool> seen(STORY_TEXT_COUNT);
    for (const auto& [id, text] : story) {
        const int index = StoryText_Find(id.c_str());
        if (index < 0) RevengeFault("%s: the id \"%s\" is no text of the game's", path.c_str(), id.c_str());
        StoryText_Place(static_cast<StoryTextId>(index), text.data(), text.size());
        seen[std::size_t(index)] = true;
    }
    for (int id = 0; id < STORY_TEXT_COUNT; ++id)
        if (!seen[std::size_t(id)]) RevengeFault("%s has no text \"%s\"", path.c_str(), StoryTextSlots[id].id);
}

}  // namespace

extern "C" void StoryHost_Prepare(const char* executablePath, const char* writeDirectory) {
    const OriginalEdition* edition = OriginalImport_Edition();
    if (edition == nullptr) RevengeFault("the story is taken from an accepted copy, and none is loaded");
    try {
        if (writeDirectory == nullptr) {
            place_all(revenge::import::import_texts(*edition));
            place_media(revenge::import::import_media(*edition));
        } else {
            const std::filesystem::path write_dir(writeDirectory);
            if (revenge::import::story_state(write_dir, *edition) != revenge::import::StoryState::Current) {
                revenge::import::write_story(write_dir, *edition, revenge::import::import_texts(*edition),
                                             revenge::import::import_media(*edition), executablePath);
                std::fprintf(stderr, "imported the story file into %s\n",
                             revenge::import::story_dir(write_dir).string().c_str());
            }
            place_file(write_dir);
            place_media(revenge::import::load_story_media(write_dir, *edition));
        }
    } catch (const std::exception& e) {
        RevengeFault("the story: %s", e.what());
    }
    StoryText_FillTargets();
}
