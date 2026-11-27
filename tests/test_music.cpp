// The music (src/Audio/Music): the instrument table read strictly, the
// MT-32's patches and rhythm set-up kept as the songs write them, and the
// notes played on the soundfont subset.
//
//     test_music table INSTRUMENTS.json
//     test_music play  INSTRUMENTS.json SUBSET.sf2
//     test_music songs INSTRUMENTS.json     every timbre the game's songs
//                                           write is in the table (needs
//                                           REVENGE_GAME; 77 without it)
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Music.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

using revenge::audio::InstrumentTable;
using revenge::audio::Music;

namespace {

std::string Read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::fprintf(stderr, "test_music: can't read %s\n", path.string().c_str());
        std::exit(1);
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

bool Refused(const std::string& json, const char* says) {
    try {
        (void)revenge::audio::ParseInstrumentTable(json);
    } catch (const std::runtime_error& error) {
        return std::string(error.what()).find(says) != std::string::npos;
    }
    return false;
}

const char* const kSmall =
    R"({"gain_db": 0, "fallback_program": 48, "timbres": [], "rhythm": [], "rhythm_keys": []})";

void Table(const std::string& json) {
    const InstrumentTable table = revenge::audio::ParseInstrumentTable(json);
    assert(table.preset[64 + 4] == 36);  // B group, timbre 4: a slap bass
    assert(table.preset[1] == InstrumentTable::kNone);
    assert(table.rhythm[64] == 36 && table.rhythmKey[36] == 64);
    (void)revenge::audio::ParseInstrumentTable(kSmall);
    assert(Refused("[]", "must be an object"));
    assert(Refused(R"({"gain_db": 0, "fallback_program": 48, "timbres": [], "rhythm": [], "rhythm_keys": [], "x": 1})",
                   "unknown key \"x\""));
    assert(Refused(R"({"gain_db": 0, "fallback_program": 128, "timbres": [], "rhythm": [], "rhythm_keys": []})",
                   "\"fallback_program\" must be a whole number from 0 to 127"));
    assert(Refused(R"({"gain_db": 0, "fallback_program": 1, "timbres": [{"group": "c", "number": 0, "program": 1}],
                      "rhythm": [], "rhythm_keys": []})",
                   "\"group\" must be a, b or memory"));
    assert(Refused(R"({"gain_db": 0, "fallback_program": 1, "timbres": [{"group": "a", "number": 0, "program": 1},
                      {"group": "a", "number": 0, "program": 2}], "rhythm": [], "rhythm_keys": []})",
                   "timbre a 0: listed twice"));
    assert(Refused(R"({"gain_db": 0, "fallback_program": 1, "timbres": [], "rhythm": [],
                      "rhythm_keys": [[20, 64]]})",
                   "keys are 24 to 108"));
    assert(Refused(R"({"gain_db": 0, "fallback_program": 1, "timbres": [], "rhythm": [], "rhythm_keys": [],)",
                   "JSON at line"));
}

void Send(Music& music, std::initializer_list<int> bytes) {
    for (int value : bytes) music.midiByte(static_cast<uint8_t>(value));
}

// A Roland DT1 to the MT-32 at `address`, its checksum made.
void Write(Music& music, int high, int middle, int low, std::initializer_list<int> data) {
    std::vector<int> bytes = {0xF0, 0x41, 0x10, 0x16, 0x12, high, middle, low};
    int sum = high + middle + low;
    for (int value : data) {
        bytes.push_back(value);
        sum += value;
    }
    bytes.push_back((128 - sum % 128) % 128);
    bytes.push_back(0xF7);
    for (int value : bytes) music.midiByte(static_cast<uint8_t>(value));
}

float Peak(Music& music, uint32_t frames) {
    std::vector<float> stereo(frames * 2);
    music.render(stereo.data(), frames);
    float peak = 0;
    for (float sample : stereo) {
        assert(std::isfinite(sample));
        peak = std::fmax(peak, std::fabs(sample));
    }
    return peak;
}

void Play(const std::string& json, const std::string& soundfont) {
    const std::string bank = Read(soundfont);
    Music music;
    std::string error;
    assert(music.load(bank.data(), bank.size(), revenge::audio::ParseInstrumentTable(json), 48000, error));

    // Parts 1-8 start on patches 1-8: timbres A0-A7, A0 a piano.
    assert(music.partProgram(0) == 0);
    // Patch 3 written to hold B4 (a slap bass); part 1 (channel 2) picks it.
    Write(music, 0x05, 0x00, 0x18, {0x01, 0x04, 24, 50, 12, 0, 1, 0});
    Send(music, {0xC1, 0x03});
    assert(music.partProgram(0) == 36);
    // A bad checksum, and another maker's set-up, change nothing.
    Send(music, {0xF0, 0x41, 0x10, 0x16, 0x12, 0x05, 0x00, 0x18, 0x00, 0x00, 24, 50, 12, 0, 1, 0, 0x00, 0xF7});
    Send(music, {0xF0, 0x7F, 0x02, 0x01, 0x01, 0xF7});
    Send(music, {0xC1, 0x03});
    assert(music.partProgram(0) == 36);
    // A timbre the table doesn't list plays the fallback.
    Write(music, 0x05, 0x00, 0x20, {0x00, 0x01, 24, 50, 12, 0, 1, 0});
    Send(music, {0xC2, 0x04});
    assert(music.partProgram(1) == 48);

    // Channel 1 has no part: nothing sounds.
    Send(music, {0x90, 60, 100});
    assert(music.activeVoices() == 0);
    // Part 1 sounds, and stops; its timbre, B4, is heard from its first
    // note.
    assert(!music.timbreHeard(64 + 4) && !music.timbreHeard(0));
    Send(music, {0x91, 40, 100});
    assert(music.activeVoices() > 0);
    assert(music.timbreHeard(64 + 4) && !music.timbreHeard(1));
    assert(music.programName(36) != "" && music.programName(127) == "");
    assert(Peak(music, 4800) > 0.001f);
    Send(music, {0x81, 40, 0});
    for (int block = 0; block < 40 && music.activeVoices() > 0; ++block) (void)Peak(music, 4800);
    assert(music.activeVoices() == 0);

    // The rhythm part: key 24 set to timbre 64 (a bass drum) sounds; key
    // 16 is below the set-up and key 52 has no timbre at start.
    Send(music, {0x99, 16, 100, 0x99, 52, 100});
    assert(music.activeVoices() == 0);
    Write(music, 0x03, 0x01, 0x10, {64, 100, 7, 0});
    assert(!music.rhythmTimbreHeard(64));
    Send(music, {0x99, 24, 100});
    assert(music.activeVoices() > 0);
    assert(music.rhythmTimbreHeard(64));
    Send(music, {0xB9, 123, 0});
    for (int block = 0; block < 40 && music.activeVoices() > 0; ++block) (void)Peak(music, 4800);
    assert(music.activeVoices() == 0);

    // Part 1 moved to channel 1 (10 00 0D): channel 2 is no part's now.
    Write(music, 0x10, 0x00, 0x0D, {0x00});
    Send(music, {0x91, 40, 100});
    assert(music.activeVoices() == 0);
    Send(music, {0x90, 40, 100});
    assert(music.activeVoices() > 0);

    // Back to the MT-32 as it starts: nothing heard yet.
    music.reset();
    assert(music.partProgram(0) == 0);
    assert(!music.timbreHeard(64 + 4) && !music.rhythmTimbreHeard(64));
    for (int block = 0; block < 4 && music.activeVoices() > 0; ++block) (void)Peak(music, 480);
    assert(music.activeVoices() == 0);
}

// Every Roland DT1 to the patch memory and the rhythm set-up in the game's
// songs, read as bytes from the .MUS files: each timbre must be listed.
int Songs(const std::string& json) {
    const char* game = std::getenv("REVENGE_GAME");
    if (game == nullptr || *game == '\0') {
        std::puts("test_music: skipped, no REVENGE_GAME");
        return 77;
    }
    const InstrumentTable table = revenge::audio::ParseInstrumentTable(json);
    int songs = 0, writes = 0, missing = 0;
    for (const auto& entry : std::filesystem::directory_iterator(game)) {
        std::string name = entry.path().filename().string();
        for (char& letter : name) letter = static_cast<char>(std::toupper(static_cast<unsigned char>(letter)));
        if (name.size() < 4 || name.compare(name.size() - 4, 4, ".MUS") != 0) continue;
        ++songs;
        const std::string bytes = Read(entry.path());
        const std::string dt1 = "\x41\x10\x16\x12";
        for (std::size_t at = bytes.find(dt1); at != std::string::npos; at = bytes.find(dt1, at + 1)) {
            const std::size_t end = bytes.find('\xF7', at);
            if (end == std::string::npos || end < at + 9) continue;
            auto byte = [&](std::size_t index) { return static_cast<unsigned char>(bytes[index]); };
            const unsigned address = byte(at + 4) << 14 | byte(at + 5) << 7 | byte(at + 6);
            const std::size_t first = at + 7, count = end - 1 - first;
            ++writes;
            if (address >= (5u << 14) && address < (5u << 14) + 1024) {
                for (std::size_t index = (8 - (address % 8)) % 8; index + 1 < count; index += 8) {
                    const int group = byte(first + index), number = byte(first + index + 1);
                    const int program = group == 0   ? table.preset[static_cast<std::size_t>(number)]
                                        : group == 1 ? table.preset[static_cast<std::size_t>(64 + number)]
                                        : group == 2 ? table.memory[static_cast<std::size_t>(number)]
                                                     : 0;
                    if (program == InstrumentTable::kNone) {
                        std::printf("test_music: %s names timbre %d/%d, which the table lacks\n", name.c_str(), group,
                                    number);
                        ++missing;
                    }
                }
            } else if (address >= (3u << 14) + 0x90 && address < (3u << 14) + 0x90 + 85 * 4) {
                for (std::size_t index = (4 - ((address - ((3u << 14) + 0x90)) % 4)) % 4; index < count; index += 4) {
                    const int timbre = byte(first + index);
                    if (timbre < 127 && table.rhythm[static_cast<std::size_t>(timbre)] == InstrumentTable::kNone) {
                        std::printf("test_music: %s names rhythm timbre %d, which the table lacks\n", name.c_str(),
                                    timbre);
                        ++missing;
                    }
                }
            }
        }
    }
    std::printf("test_music: %d songs, %d writes, %d timbres missing\n", songs, writes, missing);
    return songs > 0 && missing == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fputs("usage: test_music table|play|songs INSTRUMENTS.json [SUBSET.sf2]\n", stderr);
        return 2;
    }
    const std::string mode = argv[1], json = Read(argv[2]);
    if (mode == "table") {
        Table(json);
        return 0;
    }
    if (mode == "play" && argc > 3) {
        Play(json, argv[3]);
        return 0;
    }
    if (mode == "songs") return Songs(json);
    return 2;
}
