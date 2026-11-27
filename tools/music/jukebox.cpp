// The jukebox: every song of the game's, start to end, through the game's own
// sequencer and Roland driver at the original's rates (the driver's service
// at 1193182/400h Hz, the sequence at 1110h ticks a minute, as the host runs
// them), into the music (src/Audio/Music) on the soundfont, one WAV a song.
// Nothing is played aloud. For listening to data/music/instruments.json
// (tools/music/listening.py writes the sheet).
//
//     revenge_jukebox INSTRUMENTS.json SUBSET.sf2 GAME_DIR OUT_DIR [SONG ...]
//
// Each .MUS in GAME_DIR (any case; or only the SONGs named, without .MUS)
// starts from the MT-32 as it powers on, as if it were the first song the
// game played, and runs until the sequence ends and 3 s more ring out, or 300
// s. Out: OUT_DIR/SONG.wav (48 kHz, 16-bit stereo), and on stdout a JSON line
// a song: its file, length, peak and RMS, and the timbres (and rhythm
// timbres) a note sounded with, each with the program or key the table gives
// it and the soundfont's name for that program.
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Music.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
void JukeboxGame_Begin(void (*sink)(unsigned char));
int JukeboxGame_Start(const unsigned char* bytes, unsigned length);
int JukeboxGame_Playing(void);
void JukeboxGame_DriverTick(void);
void JukeboxGame_SequenceTick(void);
void JukeboxGame_Stop(void);
void JukeboxGame_End(void);
}

namespace fs = std::filesystem;
using revenge::audio::InstrumentTable;
using revenge::audio::Music;

namespace {

constexpr uint32_t kRate = 48000;
constexpr uint64_t kSecond = 1000000000;
constexpr uint64_t kDriverPeriod = kSecond * 0x400 / 0x1234DE;
constexpr uint64_t kSequencePeriod = kSecond * 60 / 0x1110;
constexpr uint64_t kTail = 3 * kSecond;
constexpr uint64_t kLongest = 300 * kSecond;
constexpr uint32_t kBlock = 4096;

Music* g_music = nullptr;

void Sink(unsigned char value) { g_music->midiByte(value); }

std::string Read(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("can't read " + path.string());
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

std::string Upper(std::string text) {
    for (char& letter : text) letter = static_cast<char>(std::toupper(static_cast<unsigned char>(letter)));
    return text;
}

std::string Quoted(const std::string& text) {
    std::string out = "\"";
    for (char c : text) {
        if (c == '"' || c == '\\') out += '\\';
        if (static_cast<unsigned char>(c) >= 0x20) out += c;
    }
    return out + "\"";
}

void PutWord(std::ofstream& out, uint32_t value, int bytes) {
    for (int index = 0; index < bytes; ++index) out.put(static_cast<char>(value >> (8 * index) & 0xFF));
}

void WavHeader(std::ofstream& out, uint64_t frames) {
    const uint32_t data = static_cast<uint32_t>(frames * 4);
    out.seekp(0);
    out.write("RIFF", 4);
    PutWord(out, 36 + data, 4);
    out.write("WAVEfmt ", 8);
    PutWord(out, 16, 4);
    PutWord(out, 1, 2);
    PutWord(out, 2, 2);
    PutWord(out, kRate, 4);
    PutWord(out, kRate * 4, 4);
    PutWord(out, 4, 2);
    PutWord(out, 16, 2);
    out.write("data", 4);
    PutWord(out, data, 4);
}

// One song to OUT_DIR/NAME.wav; its JSON line.
std::string Play(Music& music, const InstrumentTable& table, const std::string& name, const std::string& bytes,
                 const fs::path& outDir) {
    music.reset();
    if (!JukeboxGame_Start(reinterpret_cast<const unsigned char*>(bytes.data()), static_cast<unsigned>(bytes.size())))
        throw std::runtime_error(name + ": doesn't fit the game's song buffer");
    const fs::path file = outDir / (name + ".wav");
    std::ofstream out(file, std::ios::binary);
    if (!out) throw std::runtime_error("can't write " + file.string());
    WavHeader(out, 0);

    std::vector<float> block(kBlock * 2);
    uint64_t now = 0, nextDriver = 0, nextSequence = 0, frames = 0, end = 0;
    double peak = 0, squares = 0;
    bool cut = false;
    while (end == 0 || now < end) {
        now += kDriverPeriod;
        while (nextDriver <= now) {
            JukeboxGame_DriverTick();
            nextDriver += kDriverPeriod;
        }
        while (nextSequence <= now) {
            JukeboxGame_SequenceTick();
            nextSequence += kSequencePeriod;
        }
        const uint64_t due = now * kRate / kSecond;
        while (frames < due) {
            const uint32_t count = static_cast<uint32_t>(std::min<uint64_t>(due - frames, kBlock));
            music.render(block.data(), count);
            for (uint32_t index = 0; index < count * 2; ++index) {
                const double sample = block[index];
                peak = std::max(peak, std::fabs(sample));
                squares += sample * sample;
                PutWord(out, static_cast<uint16_t>(static_cast<int16_t>(std::lround(std::clamp(sample, -1.0, 1.0) * 32767))), 2);
            }
            frames += count;
        }
        if (end == 0 && !JukeboxGame_Playing()) end = now + kTail;
        if (end == 0 && now >= kLongest) {
            end = now;
            cut = true;
        }
    }
    JukeboxGame_Stop();
    WavHeader(out, frames);

    const double rms = frames == 0 ? 0 : std::sqrt(squares / static_cast<double>(frames * 2));
    char numbers[160];
    std::snprintf(numbers, sizeof(numbers), "\"seconds\": %.1f, \"peak\": %.3f, \"rms_dbfs\": %.1f, \"cut\": %s",
                  static_cast<double>(frames) / kRate, peak, rms > 0 ? 20 * std::log10(rms) : -120.0,
                  cut ? "true" : "false");
    std::string line = "{\"song\": " + Quoted(name) + ", \"file\": " + Quoted(name + ".wav") + ", " + numbers;
    line += ", \"timbres\": [";
    bool first = true;
    for (int timbre = 0; timbre < 192; ++timbre) {
        if (!music.timbreHeard(timbre)) continue;
        int program = timbre < 128 ? table.preset[static_cast<std::size_t>(timbre)]
                                   : table.memory[static_cast<std::size_t>(timbre - 128)];
        if (program == InstrumentTable::kNone) program = table.fallbackProgram;
        line += (first ? "" : ", ") + std::string("{\"timbre\": ") + std::to_string(timbre) +
                ", \"program\": " + std::to_string(program) + ", \"name\": " + Quoted(music.programName(program)) + "}";
        first = false;
    }
    line += "], \"rhythm\": [";
    first = true;
    for (int timbre = 0; timbre < 128; ++timbre) {
        if (!music.rhythmTimbreHeard(timbre)) continue;
        line += (first ? "" : ", ") + std::string("{\"timbre\": ") + std::to_string(timbre) +
                ", \"key\": " + std::to_string(table.rhythm[static_cast<std::size_t>(timbre)]) + "}";
        first = false;
    }
    return line + "]}";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 5) {
        std::fputs("usage: revenge_jukebox INSTRUMENTS.json SUBSET.sf2 GAME_DIR OUT_DIR [SONG ...]\n", stderr);
        return 2;
    }
    try {
        const InstrumentTable table = revenge::audio::ParseInstrumentTable(Read(argv[1]));
        const std::string soundfont = Read(argv[2]);
        const fs::path game = argv[3], outDir = argv[4];
        std::vector<std::string> only;
        for (int index = 5; index < argc; ++index) only.push_back(Upper(argv[index]));

        std::vector<fs::path> songs;
        for (const fs::directory_entry& entry : fs::directory_iterator(game)) {
            const std::string upper = Upper(entry.path().filename().string());
            if (upper.size() < 5 || upper.compare(upper.size() - 4, 4, ".MUS") != 0) continue;
            if (!only.empty() && std::find(only.begin(), only.end(), upper.substr(0, upper.size() - 4)) == only.end())
                continue;
            songs.push_back(entry.path());
        }
        std::sort(songs.begin(), songs.end(),
                  [](const fs::path& a, const fs::path& b) { return Upper(a.filename().string()) < Upper(b.filename().string()); });
        if (songs.empty()) throw std::runtime_error("no songs (.MUS) in " + game.string());
        fs::create_directories(outDir);

        static Music music;
        std::string error;
        if (!music.load(soundfont.data(), soundfont.size(), table, kRate, error))
            throw std::runtime_error(std::string(argv[2]) + ": " + error);
        g_music = &music;
        JukeboxGame_Begin(Sink);
        for (const fs::path& path : songs) {
            const std::string upper = Upper(path.filename().string());
            std::puts(Play(music, table, upper.substr(0, upper.size() - 4), Read(path), outDir).c_str());
            std::fflush(stdout);
        }
        JukeboxGame_End();
    } catch (const std::exception& problem) {
        std::fprintf(stderr, "revenge_jukebox: %s\n", problem.what());
        return 1;
    }
    return 0;
}
