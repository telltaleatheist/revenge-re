#include "Music.h"

#include "../Support/Json.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#define TSF_IMPLEMENTATION
#define TSF_NO_STDIO  // the host reads the bank (SDL, UTF-8 paths); only tsf_load_memory is used
#include "tsf.h"

namespace revenge::audio
{
namespace
{

using support::Json;

// The MT-32's memory, as three 7-bit address bytes read as one number.
constexpr uint32_t Address(uint32_t high, uint32_t middle, uint32_t low) { return high << 14 | middle << 7 | low; }
constexpr uint32_t kRhythmSetup = Address(0x03, 0x01, 0x10);     // 4 bytes a key from 24
constexpr uint32_t kPatchMemory = Address(0x05, 0x00, 0x00);     // 8 bytes a patch
constexpr uint32_t kPartChannels = Address(0x10, 0x00, 0x0D);    // parts 1-8, then rhythm
constexpr uint8_t kRolandId = 0x41, kMt32Model = 0x16, kSetData = 0x12;
constexpr uint8_t kNoTimbre = 0x7F;
constexpr int kMaxVoices = 96;

// A patch: timbre group (0 A, 1 B, 2 memory, 3 rhythm), timbre number, key
// shift (24 is none), fine tune, bender range, assign mode, reverb, unused.
enum
{
    kGroup,
    kNumber,
    kKeyShift,
    kFineTune,
    kBenderRange
};

[[noreturn]] void Fail(const std::string &where, const std::string &why)
{
    throw std::runtime_error("instrument table: " + where + ": " + why);
}

void OnlyKeys(const Json &object, const std::string &where, std::initializer_list<std::string_view> keys)
{
    for (const auto &entry : object.object())
    {
        bool known = false;
        for (std::string_view key : keys) known = known || entry.first == key;
        if (!known) Fail(where, "unknown key \"" + entry.first + "\"");
    }
}

int Whole(const Json &object, std::string_view key, const std::string &where, int low, int high)
{
    const Json *value = object.find(key);
    if (value == nullptr) Fail(where, "needs \"" + std::string(key) + "\"");
    if (!value->is_number()) Fail(where, "\"" + std::string(key) + "\" must be a number");
    const double number = value->number();
    if (number != std::floor(number) || number < low || number > high)
        Fail(where, "\"" + std::string(key) + "\" must be a whole number from " + std::to_string(low) + " to " + std::to_string(high));
    return static_cast<int>(number);
}

void Words(const Json &object, const std::string &where)
{
    for (std::string_view key : { "mt32", "note" })
        if (const Json *word = object.find(key); word != nullptr && !word->is_string())
            Fail(where, "\"" + std::string(key) + "\" must be a string");
}

const Json::Array &List(const Json &root, std::string_view key)
{
    const Json *list = root.find(key);
    if (list == nullptr || !list->is_array()) Fail(std::string(key), "must be an array");
    return list->array();
}

}  // namespace

InstrumentTable ParseInstrumentTable(std::string_view text)
{
    Json root;
    try
    {
        root = support::parse_json(text);
    }
    catch (const support::JsonError &error)
    {
        Fail("the file", error.what());
    }
    if (!root.is_object()) Fail("the file", "must be an object");
    OnlyKeys(root, "the file", { "gain_db", "fallback_program", "timbres", "rhythm", "rhythm_keys" });
    InstrumentTable table;
    table.preset.fill(InstrumentTable::kNone);
    table.memory.fill(InstrumentTable::kNone);
    table.rhythm.fill(InstrumentTable::kNone);
    table.rhythmKey.fill(InstrumentTable::kNone);

    const Json *gain = root.find("gain_db");
    if (gain == nullptr || !gain->is_number() || gain->number() < -40 || gain->number() > 20)
        Fail("gain_db", "must be a number from -40 to 20");
    table.gainDb = static_cast<float>(gain->number());
    table.fallbackProgram = Whole(root, "fallback_program", "the file", 0, 127);

    for (const Json &entry : List(root, "timbres"))
    {
        if (!entry.is_object()) Fail("timbres", "each must be an object");
        OnlyKeys(entry, "timbres", { "group", "number", "program", "mt32", "note" });
        Words(entry, "timbres");
        const Json *group = entry.find("group");
        if (group == nullptr || !group->is_string()) Fail("timbres", "each needs a \"group\": a, b or memory");
        const int number = Whole(entry, "number", "timbres", 0, 63);
        const std::string where = "timbre " + group->string() + " " + std::to_string(number);
        int *slot = nullptr;
        if (group->string() == "a")
            slot = &table.preset[static_cast<std::size_t>(number)];
        else if (group->string() == "b")
            slot = &table.preset[static_cast<std::size_t>(64 + number)];
        else if (group->string() == "memory")
            slot = &table.memory[static_cast<std::size_t>(number)];
        else
            Fail(where, "\"group\" must be a, b or memory");
        if (*slot != InstrumentTable::kNone) Fail(where, "listed twice");
        *slot = Whole(entry, "program", where, 0, 127);
    }
    for (const Json &entry : List(root, "rhythm"))
    {
        if (!entry.is_object()) Fail("rhythm", "each must be an object");
        OnlyKeys(entry, "rhythm", { "timbre", "key", "mt32", "note" });
        Words(entry, "rhythm");
        const int timbre = Whole(entry, "timbre", "rhythm", 0, 126);
        const std::string where = "rhythm timbre " + std::to_string(timbre);
        int &slot = table.rhythm[static_cast<std::size_t>(timbre)];
        if (slot != InstrumentTable::kNone) Fail(where, "listed twice");
        slot = Whole(entry, "key", where, 0, 127);
    }
    for (const Json &pair : List(root, "rhythm_keys"))
    {
        if (!pair.is_array() || pair.array().size() != 2 || !pair.array()[0].is_number() ||
            !pair.array()[1].is_number())
            Fail("rhythm_keys", "each must be [key, timbre]");
        const double key = pair.array()[0].number(), timbre = pair.array()[1].number();
        if (key != std::floor(key) || key < 24 || key > 108 || timbre != std::floor(timbre) || timbre < 0 ||
            timbre > 126)
            Fail("rhythm_keys", "keys are 24 to 108 and timbres 0 to 126");
        int &slot = table.rhythmKey[static_cast<std::size_t>(key)];
        if (slot != InstrumentTable::kNone) Fail("rhythm_keys", "key " + std::to_string(static_cast<int>(key)) + " twice");
        slot = static_cast<int>(timbre);
    }
    return table;
}

Music::Music() = default;

Music::~Music()
{
    if (bank_ != nullptr) tsf_close(bank_);
}

bool Music::load(const void *soundfont, std::size_t size, InstrumentTable table, uint32_t sampleRate,
    std::string &error)
{
    if (bank_ != nullptr) tsf_close(bank_);
    bank_ = tsf_load_memory(soundfont, static_cast<int>(size));
    if (bank_ == nullptr)
    {
        error = "the soundfont did not load";
        return false;
    }
    auto lacks = [&](int bank, int program)
    { return tsf_get_presetindex(bank_, bank, program) < 0; };
    int missing = -1;
    if (lacks(0, table.fallbackProgram)) missing = table.fallbackProgram;
    for (int program : table.preset)
        if (program != InstrumentTable::kNone && lacks(0, program)) missing = program;
    for (int program : table.memory)
        if (program != InstrumentTable::kNone && lacks(0, program)) missing = program;
    if (missing >= 0 || lacks(128, 0))
    {
        error = missing >= 0 ? "the soundfont lacks program " + std::to_string(missing)
                             : std::string("the soundfont lacks the standard kit");
        tsf_close(bank_);
        bank_ = nullptr;
        return false;
    }
    table_ = std::move(table);
    tsf_set_output(bank_, TSF_STEREO_INTERLEAVED, static_cast<int>(sampleRate), table_.gainDb);
    tsf_set_max_voices(bank_, kMaxVoices);
    reset();
    return true;
}

void Music::reset()
{
    for (std::size_t patch = 0; patch < 128; ++patch)
    {
        uint8_t *bytes = &patches_[patch * kPatchBytes];
        const uint8_t start[kPatchBytes] = { static_cast<uint8_t>(patch / 64), static_cast<uint8_t>(patch % 64), 24, 50,
            12, 0, 1, 0 };
        std::copy(start, start + kPatchBytes, bytes);
    }
    for (int key = 0; key < kRhythmKeys; ++key)
    {
        const int timbre = table_.rhythmKey[static_cast<std::size_t>(kFirstRhythmKey + key)];
        uint8_t *bytes = &rhythmSetup_[static_cast<std::size_t>(key) * 4];
        bytes[0] = timbre == InstrumentTable::kNone ? kNoTimbre : static_cast<uint8_t>(timbre);
        bytes[1] = 100;  // level
        bytes[2] = 7;    // pan, the middle
        bytes[3] = 1;    // reverb
    }
    for (int part = 0; part < kParts; ++part)
    {
        parts_[static_cast<std::size_t>(part)].channel = part + 1;  // parts 1-8 on channels 2-9, rhythm on 10
        parts_[static_cast<std::size_t>(part)].sounding.fill(-1);
    }
    status_ = have_ = 0;
    inSysex_ = false;
    sysexLength_ = 0;
    heard_.fill(false);
    rhythmHeard_.fill(false);
    if (bank_ == nullptr) return;
    tsf_reset(bank_);
    for (int part = 0; part < kRhythmPart; ++part) programChange(part, part);
    tsf_channel_set_presetnumber(bank_, kRhythmPart, 0, 1);
}

int Music::partFor(int channel) const
{
    for (int part = 0; part < kParts; ++part)
        if (parts_[static_cast<std::size_t>(part)].channel == channel) return part;
    return -1;
}

void Music::midiByte(uint8_t value)
{
    if (bank_ == nullptr || value >= 0xF8) return;  // real-time bytes change nothing here
    if (value == 0xF0)
    {
        inSysex_ = true;
        sysexLength_ = 0;
        status_ = 0;
        return;
    }
    if (value == 0xF7)
    {
        if (inSysex_) sysex();
        inSysex_ = false;
        return;
    }
    if (value >= 0x80)
    {
        inSysex_ = false;
        status_ = value < 0xF0 ? value : 0;  // system common messages cancel running status
        have_ = 0;
        return;
    }
    if (inSysex_)
    {
        if (sysexLength_ < sysex_.size()) sysex_[sysexLength_] = value;
        ++sysexLength_;
        return;
    }
    if (status_ == 0) return;
    data_[have_++] = value;
    const uint8_t kind = status_ & 0xF0;
    if (have_ < (kind == 0xC0 || kind == 0xD0 ? 1 : 2)) return;
    channelMessage();
    have_ = 0;  // running status: the next data byte starts another message
}

void Music::sysex()
{
    // Roland, any device number, the MT-32, "set data", three address
    // bytes, the data, and a checksum that makes the address and data sum
    // to a multiple of 128. Anything else (the songs carry other synths'
    // set-ups too) an MT-32 ignores.
    if (sysexLength_ > sysex_.size() || sysexLength_ < 8 || sysex_[0] != kRolandId || sysex_[2] != kMt32Model ||
        sysex_[3] != kSetData)
        return;
    unsigned sum = 0;
    for (std::size_t index = 4; index < sysexLength_; ++index) sum += sysex_[index];
    if (sum % 128 != 0) return;
    write(Address(sysex_[4], sysex_[5], sysex_[6]), &sysex_[7], sysexLength_ - 8);
}

void Music::write(uint32_t address, const uint8_t *data, std::size_t count)
{
    for (std::size_t index = 0; index < count; ++index)
    {
        const uint32_t at = address + static_cast<uint32_t>(index);
        if (at >= kPatchMemory && at < kPatchMemory + patches_.size())
        {
            patches_[at - kPatchMemory] = data[index];
        }
        else if (at >= kRhythmSetup && at < kRhythmSetup + rhythmSetup_.size())
        {
            rhythmSetup_[at - kRhythmSetup] = data[index];
        }
        else if (at >= kPartChannels && at < kPartChannels + kParts)
        {
            parts_[at - kPartChannels].channel = data[index] < 16 ? data[index] : -1;
        }
        // The rest (the parts' own settings, the timbres' sounds) the songs
        // don't write, or changes nothing heard here.
    }
}

void Music::programChange(int part, int patch)
{
    const uint8_t *bytes = &patches_[static_cast<std::size_t>(patch) * kPatchBytes];
    const int number = bytes[kNumber] & 0x3F;
    int program = InstrumentTable::kNone;
    int timbre = -1;
    switch (bytes[kGroup])
    {
    case 0: program = table_.preset[static_cast<std::size_t>(timbre = number)]; break;
    case 1: program = table_.preset[static_cast<std::size_t>(timbre = 64 + number)]; break;
    case 2:
        program = table_.memory[static_cast<std::size_t>(number)];
        timbre = 128 + number;
        break;
    default: break;
    }
    if (program == InstrumentTable::kNone) program = table_.fallbackProgram;
    Part &state = parts_[static_cast<std::size_t>(part)];
    state.timbre = timbre;
    state.keyShift = std::clamp(static_cast<int>(bytes[kKeyShift]), 0, 48) - 24;
    tsf_channel_set_presetnumber(bank_, part, program, 0);
    tsf_channel_set_pitchrange(bank_, part, static_cast<float>(std::min(static_cast<int>(bytes[kBenderRange]), 24)));
}

void Music::noteOn(int part, int key, int velocity)
{
    Part &state = parts_[static_cast<std::size_t>(part)];
    int played = key + state.keyShift;
    float level = static_cast<float>(velocity) / 127.0f;
    if (part == kRhythmPart)
    {
        if (key < kFirstRhythmKey || key >= kFirstRhythmKey + kRhythmKeys) return;
        const uint8_t *setup = &rhythmSetup_[static_cast<std::size_t>(key - kFirstRhythmKey) * 4];
        played = setup[0] < 127 ? table_.rhythm[setup[0]] : InstrumentTable::kNone;
        if (setup[0] < 127) rhythmHeard_[setup[0]] = true;
        level *= static_cast<float>(std::min(static_cast<int>(setup[1]), 100)) / 100.0f;
    }
    if (played < 0 || played > 127) return;
    if (part != kRhythmPart && state.timbre >= 0) heard_[static_cast<std::size_t>(state.timbre)] = true;
    noteOff(part, key);
    state.sounding[static_cast<std::size_t>(key)] = static_cast<int8_t>(played);
    tsf_channel_note_on(bank_, part, played, level);
}

void Music::noteOff(int part, int key)
{
    Part &state = parts_[static_cast<std::size_t>(part)];
    int8_t &played = state.sounding[static_cast<std::size_t>(key)];
    if (played < 0) return;
    tsf_channel_note_off(bank_, part, played);
    played = -1;
}

void Music::channelMessage()
{
    const int part = partFor(status_ & 0x0F);
    if (part < 0) return;  // a channel no part hears
    const uint8_t first = data_[0], second = data_[1];
    switch (status_ & 0xF0)
    {
    case 0x90:
        if (second != 0)
        {
            noteOn(part, first, second);
            break;
        }
        [[fallthrough]];
    case 0x80: noteOff(part, first); break;
    case 0xC0:
        if (part != kRhythmPart) programChange(part, first);
        break;
    case 0xE0: tsf_channel_set_pitchwheel(bank_, part, first | second << 7); break;
    case 0xB0:
        switch (first)
        {
        case 7:   // volume
        case 11:  // expression
        case 64:  // hold
            tsf_channel_midi_control(bank_, part, first, second);
            break;
        case 10:  // the MT-32's pan runs the other way: 0 is right
            tsf_channel_midi_control(bank_, part, first, 127 - second);
            break;
        case 121:  // reset all controllers: the bender, expression and hold
            tsf_channel_set_pitchwheel(bank_, part, 0x2000);
            tsf_channel_midi_control(bank_, part, 11, 127);
            tsf_channel_set_sustain(bank_, part, 0);
            break;
        case 123:
        case 124:
        case 125:
        case 126:
        case 127:  // all notes off, as the MT-32 takes each
            for (int key = 0; key < 128; ++key) parts_[static_cast<std::size_t>(part)].sounding[static_cast<std::size_t>(key)] = -1;
            tsf_channel_note_off_all(bank_, part);
            break;
        default: break;  // the rest an MT-32 ignores, or changes nothing heard here
        }
        break;
    default: break;  // channel and key pressure
    }
}

void Music::render(float *stereo, uint32_t frames)
{
    if (bank_ == nullptr)
    {
        std::fill(stereo, stereo + frames * 2, 0.0f);
        return;
    }
    tsf_render_float(bank_, stereo, static_cast<int>(frames), 0);
}

int Music::activeVoices() const { return bank_ != nullptr ? tsf_active_voice_count(bank_) : 0; }

bool Music::timbreHeard(int timbre) const
{
    return timbre >= 0 && timbre < kTimbres && heard_[static_cast<std::size_t>(timbre)];
}

bool Music::rhythmTimbreHeard(int timbre) const
{
    return timbre >= 0 && timbre < 128 && rhythmHeard_[static_cast<std::size_t>(timbre)];
}

std::string Music::programName(int program) const
{
    if (bank_ == nullptr) return {};
    const int index = tsf_get_presetindex(bank_, 0, program);
    const char *name = index < 0 ? nullptr : tsf_get_presetname(bank_, index);
    return name == nullptr ? std::string() : std::string(name);
}

int Music::partProgram(int part) const
{
    return bank_ != nullptr && part >= 0 && part < kRhythmPart ? tsf_channel_get_preset_number(bank_, part) : -1;
}

}  // namespace revenge::audio
