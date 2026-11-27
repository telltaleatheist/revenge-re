#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

struct tsf;

namespace revenge::audio
{

// The instrument table (data/music/instruments.json): which General MIDI
// program plays each MT-32 timbre the songs choose, and which General MIDI
// drum key each rhythm timbre. Numbers only; no text from the game.
//
//   gain_db           the music's level
//   fallback_program  for a timbre the table doesn't list
//   timbres           {"group": "a"|"b"|"memory", "number": 0-63,
//                      "program": 0-127}, as a patch names its timbre
//   rhythm            {"timbre": 0-127, "key": 0-127}: a rhythm timbre (64
//                      and up are the MT-32's own; below, memory timbres)
//   rhythm_keys       [[key, timbre], ...]: the MT-32's rhythm set-up as it
//                      starts, before the songs change it
//
// "mt32" and "note" are words for the reader, and skipped.
struct InstrumentTable
{
    static constexpr int kNone = -1;
    float gainDb = 0;
    int fallbackProgram = 0;
    std::array<int, 128> preset{};    // by timbre: group A 0-63, B 64-127
    std::array<int, 64> memory{};     // the memory timbres
    std::array<int, 128> rhythm{};    // by rhythm timbre: the drum key
    std::array<int, 128> rhythmKey{}; // by key: the timbre at start
};

// Parses and checks the table; throws std::runtime_error naming the problem.
InstrumentTable ParseInstrumentTable(std::string_view json);

// The songs as an MT-32 would play them, rendered from a General MIDI
// soundfont through TinySoundFont (MIT, extern/TinySoundFont). It takes the
// bytes the game sends its Roland driver and keeps what an MT-32 keeps of
// them: the patches and the rhythm set-up the songs write (Roland DT1
// messages to 05 00 00 and 03 01 10), and which MIDI channel each part
// hears (10 00 0D). A program change picks a patch, the patch a timbre, and
// the table that timbre's program; the patch's key shift and bender range
// apply. Channels the MT-32 doesn't hear are skipped. One thread; not an
// emulation of the MT-32's sound, only of what it is told.
class Music
{
  public:
    Music();
    Music(const Music &) = delete;
    Music &operator=(const Music &) = delete;
    ~Music();

    // The soundfont's bytes and the table. False, with `error`, if the bank
    // doesn't load or lacks a program the table names.
    bool load(const void *soundfont, std::size_t size, InstrumentTable table, uint32_t sampleRate, std::string &error);
    bool loaded() const { return bank_ != nullptr; }
    // The MT-32 as it starts: its own patches, parts and rhythm set-up.
    void reset();
    void midiByte(uint8_t value);
    // Interleaved stereo, overwritten.
    void render(float *stereo, uint32_t frames);
    // Voices sounding now (tests).
    int activeVoices() const;
    // The program a part plays now, or -1 (tests): parts 0-7.
    int partProgram(int part) const;
    // Whether a note has sounded with a timbre since the reset (the
    // jukebox's sheet): 0-63 group A, 64-127 group B, 128-191 the memory
    // timbres; and with a rhythm timbre (0-127).
    bool timbreHeard(int timbre) const;
    bool rhythmTimbreHeard(int timbre) const;
    // The soundfont's name for a program (bank 0), or "".
    std::string programName(int program) const;

  private:
    static constexpr int kParts = 9;         // 8 melodic parts, then the rhythm part
    static constexpr int kRhythmPart = 8;
    static constexpr int kPatchBytes = 8;
    static constexpr int kFirstRhythmKey = 24;
    static constexpr int kRhythmKeys = 85;   // 24 to 108
    static constexpr std::size_t kSysexCapacity = 256;
    static constexpr int kTimbres = 192;     // groups A and B, then the memory timbres

    struct Part
    {
        int channel = 0;     // the MIDI channel it hears, or -1
        int keyShift = 0;    // semitones
        int timbre = -1;     // the timbre its patch names (as timbreHeard counts them), or -1
        std::array<int8_t, 128> sounding{};  // by key sent: the key played, or -1
    };

    int partFor(int channel) const;
    void channelMessage();
    void sysex();
    void write(uint32_t address, const uint8_t *data, std::size_t count);
    void programChange(int part, int patch);
    void noteOn(int part, int key, int velocity);
    void noteOff(int part, int key);

    tsf *bank_ = nullptr;
    InstrumentTable table_;
    std::array<uint8_t, 128 * kPatchBytes> patches_{};
    std::array<uint8_t, kRhythmKeys * 4> rhythmSetup_{};
    std::array<Part, kParts> parts_{};
    std::array<bool, kTimbres> heard_{};
    std::array<bool, 128> rhythmHeard_{};
    uint8_t status_ = 0, data_[2] = {}, have_ = 0;
    bool inSysex_ = false;
    std::size_t sysexLength_ = 0;
    std::array<uint8_t, kSysexCapacity> sysex_{};
};

}  // namespace revenge::audio
