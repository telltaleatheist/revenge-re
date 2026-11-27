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
    byte soundCode;
    uint16 resourceIndex;
    uint32 sourceOffset;
    uint16 sampleLength;
    uint16 packedBufferOffset;
    char resourceFileName[SCENARIO_DIGITAL_SOUND_FILE_NAME_CAPACITY];
} RevengeDigitalSoundEntry;

typedef struct RevengeDigitalSoundCatalog
{
    RevengeDigitalSoundEntry entries[SCENARIO_DIGITAL_SOUND_ENTRY_COUNT];
    byte sequenceData[SCENARIO_DIGITAL_SOUND_SEQUENCE_BYTE_COUNT];
} RevengeDigitalSoundCatalog;

typedef struct RevengeScenarioDigitalSoundPlayer
{
    const struct RevengeGameRuntime *game;
    RevengeDigitalSoundCatalog catalog;
    byte *sampleBank;
    uint16 sampleBankByteCount;
    byte loadedEntries[SCENARIO_DIGITAL_SOUND_ENTRY_COUNT];
    byte initialized;
} RevengeScenarioDigitalSoundPlayer;

typedef struct RevengeDigitalWeaponEffect
{
    int16 resourceIndex;
    uint16 playbackScale;
    uint16 packedBufferOffset;
    uint16 sampleLength;
    byte loadState;
} RevengeDigitalWeaponEffect;

typedef struct RevengeDigitalWeaponEffectBank
{
    byte *sampleBank;
    uint16 sampleBankByteCount;
    RevengeDigitalWeaponEffect effects[DIGITAL_WEAPON_EFFECT_COUNT];
    byte soundEffectsEnabled;
    byte digitalDriverSelected;
} RevengeDigitalWeaponEffectBank;

typedef struct RevengeMusicResourcePlayer
{
    const struct RevengeGameRuntime *game;
    byte *musicData;
    uint16 musicDataByteCount;
    uint16 cachedResourceIndex;
    byte musicEnabled;
} RevengeMusicResourcePlayer;


byte EnablePCSpeaker(void);
void SilencePCSpeaker(void);
void WritePCSpeakerPITDivisor(
    byte divisorLowByte,
    byte divisorHighByte);
void StartPCSpeakerFixedTone(void);
void DisablePCSpeaker(void);
void InstallMusicTimer(void);
void RestoreSystemTimer(void);
void InitializeSpeakerEffectState(void);
void StartPCSpeakerEffectStream(const byte *effectStream);
void SetPCSpeakerEffectStopRequest(void);
void DecodePCSpeakerEffectTick(void);
/* 375F:0A62: one digital sample mixed and advanced for every playing one. */
void ServiceDigitalAudioTick(void);

void SetMIDITimingDivisor(uint16 timerCallsPerMinute);
void ConfigureMIDITempo(
    uint16 ticksPerQuarterNote,
    uint16 beatsPerMinute);
void ConfigureMIDITimeSignature(
    uint16 numerator,
    uint16 denominator,
    uint16 beatsPerMeasure);
void SendMIDIAllNotesOff(void);
uint16 ReadMIDIVariableLengthQuantity(const byte **cursor);
uint16 StartMIDISequence(
    const byte *midiData,
    uint16 playbackFlags);
void PauseMIDISequence(void);
void ResumeMIDISequence(void);
void StopMIDISequence(void);
void ServiceMIDISequenceTick(void);
uint32 DivideUnsigned32(uint32 dividend, uint32 divisor);
void DispatchMIDIByte(uint16 midiByte);
void PrepareMIDIDriver(MIDIDriverDescriptorData *driverDescriptor);
void ActivateMIDIDriver(MIDIDriverDescriptorData *driverDescriptor);
void DeactivateMIDIDriver(void);
uint16 MIDIDriverHandlesByte(uint16 midiByte);
uint16 ReleaseMIDINote(uint16 channel, uint16 note);
uint16 StartMIDINote(
    uint16 channel,
    uint16 note,
    uint16 velocity);
void IgnoreMIDIPolyphonicKeyPressure(
    uint16 channel,
    uint16 note,
    uint16 pressure);
void IgnoreMIDIControllerChange(uint16 channel, uint16 value);
void ReleaseAllMIDIVoices(void);
void SetMIDIChannelProgram(uint16 channel, uint16 program);
void IgnoreMIDIChannelPressure(uint16 channel, uint16 pressure);
void IgnoreMIDIPitchBend(uint16 channel, uint16 bend);
void DispatchMIDISystemByte(uint16 midiByte);
void ResetMIDIPlaybackState(void);
void ServiceActiveMIDIDriver(void);

void DisableMusicPlayback(void);
void EnableMusicPlayback(void);
void StopLoadedMusicTrack(void);

void InitializeDigitalSoundDriver(void);
void LoadDigitalWeaponEffectBank(void);
void ReleaseDigitalWeaponEffectBank(void);
void DispatchWeaponEffect(int16 weaponEffectId);
void PlayDigitalWeaponEffect(uint16 weaponEffectId);
void HandleDigitalWeaponEffectCompletion(
    uint16 descriptorOffset,
    uint16 descriptorSegment);
void PlayPCSpeakerEffectByIndex(uint16 effectId);

int16 ParseDigitalSoundCatalog(
    const byte *executableData,
    uint32 executableByteCount,
    RevengeDigitalSoundCatalog *catalog);
int16 FindDigitalSoundEntryByCode(byte soundCode);
const byte *GetDigitalSoundSequenceFragment(
    const RevengeDigitalSoundCatalog *catalog,
    uint16 dataSegmentOffset);
int16 InitializeScenarioDigitalSoundPlayer(
    RevengeScenarioDigitalSoundPlayer *player,
    const struct RevengeGameRuntime *game);
void LoadScenarioDigitalSoundBank(int8 scenarioCode);
void ReleaseDigitalSoundBuffer(void);
void ShutdownScenarioDigitalSoundPlayer(
    RevengeScenarioDigitalSoundPlayer *player);
uint16 BuildScenarioDigitalSoundSequence(
    const RevengeScenarioDigitalSoundPlayer *player,
    int16 scenarioCode,
    uint16 narrationMode,
    uint16 forceIndex,
    byte alternateResult,
    byte soundCodes[0x10]);
void PlayScenarioDigitalSoundSequence(int8 scenarioCode);

const char *GetMusicResourceFileName(uint16 resourceIndex);
void InitializeMusicResourcePlayer(
    RevengeMusicResourcePlayer *player,
    const struct RevengeGameRuntime *game,
    byte musicEnabled);
void ShutdownMusicResourcePlayer(
    RevengeMusicResourcePlayer *player);

#endif
