#ifndef REVENGE_AUDIO_H
#define REVENGE_AUDIO_H

#include "revenge/types.h"
#include "revenge/original/audio_data.h"

struct RevengeGameRuntime;

enum
{
    MIDI_DRIVER_INTERNAL_SPEAKER = 0x00,
    MIDI_DRIVER_ROLAND = 0x04,
    MIDI_TIMER_CALLS_PER_MINUTE = 0x1110,
    SOUND_HARDWARE_AVAILABLE = 0x0001,
    SOUND_HARDWARE_UNAVAILABLE = 0xFFFF,
    PIT_INPUT_FREQUENCY_HZ = 0x001234DE,
    SPEAKER_EFFECT_NOTE_MAX = 0x5F,
    SPEAKER_EFFECT_COMMAND_LOOP1_BEGIN = 0xCD,
    SPEAKER_EFFECT_COMMAND_LOOP1_END = 0xCE,
    SPEAKER_EFFECT_COMMAND_LOOP2_BEGIN = 0xD1,
    SPEAKER_EFFECT_COMMAND_LOOP2_END = 0xD2,
    SPEAKER_EFFECT_COMMAND_SET_DURATION_SCALE = 0xE2,
    SPEAKER_EFFECT_COMMAND_FREQUENCY_HZ = 0xE6,
    SPEAKER_EFFECT_COMMAND_DECREASE_DURATION_SCALE = 0xE8,
    SPEAKER_EFFECT_COMMAND_INCREASE_DURATION_SCALE = 0xE9,
    SPEAKER_EFFECT_COMMAND_END = 0xFF
};

extern MIDIDriverDescriptorData NullMIDIDriverDescriptor;
extern MIDIDriverDescriptorData SpeakerMIDIDriverDescriptor;
extern MIDIDriverDescriptorData RolandMIDIDriverDescriptor;

enum
{
    SCENARIO_DIGITAL_SOUND_ENTRY_COUNT = 0x16,
    SCENARIO_DIGITAL_SOUND_ENTRY_SIZE = 0x0B,
    SCENARIO_DIGITAL_SOUND_FILE_NAME_CAPACITY = 0x10,
    SCENARIO_DIGITAL_SOUND_SEQUENCE_BYTE_COUNT = 0x017D,
    SCENARIO_DIGITAL_SOUND_SAMPLE_RATE_HZ = 0x1F40,
    SCENARIO_DIGITAL_SOUND_UNITY_SCALE = 0x6633,
    DIGITAL_WEAPON_EFFECT_COUNT = 0x10,
    DIGITAL_WEAPON_EFFECT_SAMPLE_RATE_HZ = 0x1F40,
    DIGITAL_WEAPON_EFFECT_UNITY_SCALE = 0x6633
};

typedef struct RevengeDigitalSoundEntry
{
    uint8_t soundCode;
    uint16_t resourceIndex;
    uint32_t sourceOffset;
    uint16_t sampleLength;
    uint16_t packedBufferOffset;
    char resourceFileName[SCENARIO_DIGITAL_SOUND_FILE_NAME_CAPACITY];
} RevengeDigitalSoundEntry;

typedef struct RevengeDigitalSoundCatalog
{
    RevengeDigitalSoundEntry entries[SCENARIO_DIGITAL_SOUND_ENTRY_COUNT];
    uint8_t sequenceData[SCENARIO_DIGITAL_SOUND_SEQUENCE_BYTE_COUNT];
} RevengeDigitalSoundCatalog;

typedef struct RevengeScenarioDigitalSoundPlayer
{
    const struct RevengeGameRuntime *game;
    RevengeDigitalSoundCatalog catalog;
    uint8_t *sampleBank;
    uint16_t sampleBankByteCount;
    uint8_t loadedEntries[SCENARIO_DIGITAL_SOUND_ENTRY_COUNT];
    uint8_t initialized;
} RevengeScenarioDigitalSoundPlayer;

typedef struct RevengeDigitalWeaponEffect
{
    int16_t resourceIndex;
    uint16_t playbackScale;
    uint16_t packedBufferOffset;
    uint16_t sampleLength;
    uint8_t loadState;
} RevengeDigitalWeaponEffect;

typedef struct RevengeDigitalWeaponEffectBank
{
    uint8_t *sampleBank;
    uint16_t sampleBankByteCount;
    RevengeDigitalWeaponEffect effects[DIGITAL_WEAPON_EFFECT_COUNT];
    uint8_t soundEffectsEnabled;
    uint8_t digitalDriverSelected;
} RevengeDigitalWeaponEffectBank;

typedef struct RevengeMusicResourcePlayer
{
    const struct RevengeGameRuntime *game;
    uint8_t *musicData;
    uint16_t musicDataByteCount;
    uint16_t cachedResourceIndex;
    uint8_t musicEnabled;
} RevengeMusicResourcePlayer;


uint8_t EnablePCSpeaker(void);
void SilencePCSpeaker(void);
void WritePCSpeakerPITDivisor(
    uint8_t divisorLowByte,
    uint8_t divisorHighByte);
void StartPCSpeakerFixedTone(void);
void DisablePCSpeaker(void);
void InstallMusicTimer(void);
void RestoreSystemTimer(void);
void InitializeSpeakerEffectState(void);
void StartPCSpeakerEffectStream(const uint8_t *effectStream);
void SetPCSpeakerEffectStopRequest(void);
void DecodePCSpeakerEffectTick(void);
/* 375F:0A62: one digital sample mixed and advanced for every playing one. */
void ServiceDigitalAudioTick(void);

void SetMIDITimingDivisor(uint16_t timerCallsPerMinute);
void ConfigureMIDITempo(
    uint16_t ticksPerQuarterNote,
    uint16_t beatsPerMinute);
void ConfigureMIDITimeSignature(
    uint16_t numerator,
    uint16_t denominator,
    uint16_t beatsPerMeasure);
void SendMIDIAllNotesOff(void);
uint16_t ReadMIDIVariableLengthQuantity(const uint8_t **cursor);
uint16_t StartMIDISequence(
    const uint8_t *midiData,
    uint16_t playbackFlags);
void PauseMIDISequence(void);
void ResumeMIDISequence(void);
void StopMIDISequence(void);
void ServiceMIDISequenceTick(void);
uint32_t DivideUnsigned32(uint32_t dividend, uint32_t divisor);
void DispatchMIDIByte(uint16_t midiByte);
void PrepareMIDIDriver(MIDIDriverDescriptorData *driverDescriptor);
void ActivateMIDIDriver(MIDIDriverDescriptorData *driverDescriptor);
void DeactivateMIDIDriver(void);
uint16_t MIDIDriverHandlesByte(uint16_t midiByte);
uint16_t ReleaseMIDINote(uint16_t channel, uint16_t note);
uint16_t StartMIDINote(
    uint16_t channel,
    uint16_t note,
    uint16_t velocity);
void IgnoreMIDIPolyphonicKeyPressure(
    uint16_t channel,
    uint16_t note,
    uint16_t pressure);
void IgnoreMIDIControllerChange(uint16_t channel, uint16_t value);
void ReleaseAllMIDIVoices(void);
void SetMIDIChannelProgram(uint16_t channel, uint16_t program);
void IgnoreMIDIChannelPressure(uint16_t channel, uint16_t pressure);
void IgnoreMIDIPitchBend(uint16_t channel, uint16_t bend);
void DispatchMIDISystemByte(uint16_t midiByte);
void ResetMIDIPlaybackState(void);
void ServiceActiveMIDIDriver(void);

void DisableMusicPlayback(void);
void EnableMusicPlayback(void);
void StopLoadedMusicTrack(void);

void InitializeDigitalSoundDriver(void);
void LoadDigitalWeaponEffectBank(void);
void ReleaseDigitalWeaponEffectBank(void);
void DispatchWeaponEffect(int16_t weaponEffectId);
void PlayDigitalWeaponEffect(uint16_t weaponEffectId);
struct DigitalSampleDescriptor;
void HandleDigitalWeaponEffectCompletion(
    struct DigitalSampleDescriptor *descriptor);
void PlayPCSpeakerEffectByIndex(uint16_t effectId);

int16_t ParseDigitalSoundCatalog(
    const uint8_t *executableData,
    uint32_t executableByteCount,
    RevengeDigitalSoundCatalog *catalog);
int16_t FindDigitalSoundEntryByCode(uint8_t soundCode);
const uint8_t *GetDigitalSoundSequenceFragment(
    const RevengeDigitalSoundCatalog *catalog,
    uint16_t dataSegmentOffset);
int16_t InitializeScenarioDigitalSoundPlayer(
    RevengeScenarioDigitalSoundPlayer *player,
    const struct RevengeGameRuntime *game);
void LoadScenarioDigitalSoundBank(int8_t scenarioCode);
void ReleaseDigitalSoundBuffer(void);
void ShutdownScenarioDigitalSoundPlayer(
    RevengeScenarioDigitalSoundPlayer *player);
uint16_t BuildScenarioDigitalSoundSequence(
    const RevengeScenarioDigitalSoundPlayer *player,
    int16_t scenarioCode,
    uint16_t narrationMode,
    uint16_t forceIndex,
    uint8_t alternateResult,
    uint8_t soundCodes[0x10]);
void PlayScenarioDigitalSoundSequence(int8_t scenarioCode);

const char *GetMusicResourceFileName(uint16_t resourceIndex);
void InitializeMusicResourcePlayer(
    RevengeMusicResourcePlayer *player,
    const struct RevengeGameRuntime *game,
    uint8_t musicEnabled);
void ShutdownMusicResourcePlayer(
    RevengeMusicResourcePlayer *player);

#endif
