// 3A6A: audio data declarations
#ifndef REVENGE_DATA_AUDIO_DATA_H
#define REVENGE_DATA_AUDIO_DATA_H

#include "Audio/Audio.h"
#include "revenge/audio.h"
#include "revenge/types.h"

#include <stddef.h>

#pragma pack(push, 1)
typedef struct DigitalWeaponEffectRecord
{
    uint16_t ResourceIndex;
    uint16_t PlaybackScale;
    uint16_t PackedBufferOffset;
    uint16_t SampleLength;
    uint16_t LoadState;
} DigitalWeaponEffectRecord;

typedef struct DigitalSoundEntryRecord
{
    uint8_t SoundCode;
    uint16_t ResourceIndex;
    uint32_t SourceFileOffset;
    uint16_t SampleLength;
    uint16_t PackedBufferOffset;
} DigitalSoundEntryRecord;

/* DigitalWeaponEffectRecord.LoadState: to be loaded, loaded into the bank,
 * or sharing the sample of an earlier effect with the same resource. */
inline constexpr uint16_t WEAPON_EFFECT_TO_LOAD = 0x00;
inline constexpr uint16_t WEAPON_EFFECT_LOADED = 0x01;
inline constexpr uint16_t WEAPON_EFFECT_SHARED = 0x02;

typedef struct DigitalSoundDataLayout
{
    uint16_t DriverDescriptorOffsets[0x09];
    /* DS:5950: the original's 0Fh sample descriptors, 34h bytes each. The
	 * port's are DigitalSamplePlaybackDescriptorTable, typed; these bytes
	 * stay as the story's media gives them. */
    uint8_t SampleDescriptorPlaces[0x0F][0x34];
    DigitalWeaponEffectRecord WeaponEffects[DIGITAL_WEAPON_EFFECT_COUNT];
    DigitalSoundEntryRecord SoundEntries[0x16];
    uint8_t ScenarioCodeStrings[0x017D];
} DigitalSoundDataLayout;
#pragma pack(pop)

static_assert(sizeof(DigitalWeaponEffectRecord) == 0x0A,
    "DigitalWeaponEffectRecord must retain its DOS layout");
static_assert(sizeof(DigitalSoundEntryRecord) == 0x0B,
    "DigitalSoundEntryRecord must retain its DOS layout");
static_assert(offsetof(DigitalSoundDataLayout, SampleDescriptorPlaces) == 0x0012,
    "digital sample descriptors must begin at DS:5950");
static_assert(offsetof(DigitalSoundDataLayout, WeaponEffects) == 0x031E,
    "weapon effects must begin at DS:5C5C");
static_assert(offsetof(DigitalSoundDataLayout, SoundEntries) == 0x03BE,
    "digital sound entries must begin at DS:5CFC");
static_assert(offsetof(DigitalSoundDataLayout, ScenarioCodeStrings) == 0x04B0,
    "scenario sound strings must begin at DS:5DEE");
static_assert(sizeof(DigitalSoundDataLayout) == 0x062D,
    "digital sound data must retain DS:593E-5F6A layout");

typedef union DigitalSoundDataImage
{
    DigitalSoundDataLayout Layout;
    uint8_t Bytes[0x062D];
} DigitalSoundDataImage;

enum
{
    DIGITAL_SOUND_DATA_FIRST_OFFSET = 0x593E,
    DIGITAL_SOUND_DATA_END_OFFSET = 0x5F6B,
    DIGITAL_SOUND_DATA_BYTE_COUNT =
        DIGITAL_SOUND_DATA_END_OFFSET - DIGITAL_SOUND_DATA_FIRST_OFFSET
};

extern DigitalSoundDataImage DigitalSoundData;
extern DigitalSampleDescriptor DigitalSamplePlaybackDescriptorTable[0x0F];
extern DigitalSampleDescriptor *ActiveDigitalSampleDescriptor;
extern DigitalDriverQueue ActiveDigitalDriverQueue;
extern uint16_t DigitalPlaybackRateHz;
extern uint8_t DigitalWeaponEffectRepeatCount;
extern uint8_t *DigitalWeaponSampleBuffer;
extern uint8_t *DigitalSoundBuffer;
extern uint8_t DigitalSoundDriverMutationGuard;
extern uint16_t NarrationMode;
/* NarrationMode's three settings; 2, the original data's, is full voice. */
inline constexpr uint16_t NARRATION_MODE_FULL_VOICE = 0x0002;
inline constexpr uint16_t NARRATION_MODE_COUNT = 0x0003;
extern uint16_t ActiveNarrationSubject;
extern uint16_t NarrationEmphasis;

#endif
