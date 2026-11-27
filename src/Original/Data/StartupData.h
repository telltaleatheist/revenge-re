// 3A6A: startup data declarations
#ifndef REVENGE_DATA_STARTUP_DATA_H
#define REVENGE_DATA_STARTUP_DATA_H

#include "revenge/types.h"

#pragma pack(push, 1)
typedef struct MediaConfigurationChoice
{
    char name[0x0C];
    uint8_t value;
} MediaConfigurationChoice;

typedef struct SoundConfigurationChoice
{
    char name[0x0C];
    uint8_t driverIndex;
    uint8_t hasNumericParameter;
} SoundConfigurationChoice;
#pragma pack(pop)

static_assert(sizeof(MediaConfigurationChoice) == 0x0D,
    "MediaConfigurationChoice must retain its DOS layout");
static_assert(sizeof(SoundConfigurationChoice) == 0x0E,
    "SoundConfigurationChoice must retain its DOS layout");

extern uint16_t MapWidth;
extern uint16_t MapHeight;
extern uint8_t VideoPathUnavailable;
extern uint16_t SavedBIOSVideoMode;
extern const uint16_t BIOSVideoModeByAdapter[0x07];
extern uint16_t GameInitialized;
extern uint16_t StartupResourceState;
extern uint8_t *InvalidSaveDialogBacking;
extern uint16_t NumericKeypadNavigationEnabled;
extern uint16_t StartupInputMode;
extern uint16_t SoundEffectsEnabled;
extern uint16_t DigitalSoundBusy;
extern uint16_t AppliedMusicEnabled;
extern uint16_t SoundDriverNumericParameter;
extern uint16_t InputDeviceType;
extern int8_t SoundDriverIndex;
extern uint8_t *Font6Resource;
extern uint8_t *Font8Resource;
extern uint8_t *SecondaryScreenBuffer;
extern uint8_t *PlanarScreenBuffer;
extern uint16_t MouseCursorBackingSourceTileX;
extern uint16_t MouseCursorBackingSourcePixelY;
extern uint16_t MouseCursorBackingWidthTiles;
extern uint16_t MouseCursorBackingHeightPixels;
extern uint8_t *SharedGraphicsBackingBuffers[0x26];
extern uint8_t *VideoFramebuffer;
extern uint16_t ResourceRuntimeInitialized;
extern const char MediaConfigurationFileName[];
extern const char *const InvalidSaveVersionText;
extern const char VersionDecimalSeparator[];
extern const char VersionTranslationSource[];
extern const char VersionTranslationReplacement[];
extern const char MediaConfigAssignmentDelimiter[];
extern const char VideoConfigTokenDelimiters[];
extern const char MusicConfigTokenDelimiters[];
extern const char SoundConfigTokenDelimiters[];
extern const char SoundParameterTokenDelimiters[];
extern const char InputConfigTokenDelimiters[];
/* MEDIA.CFG's values for each option. */
inline constexpr int VIDEO_CONFIGURATION_COUNT = 0x05;
inline constexpr int MUSIC_CONFIGURATION_COUNT = 0x07;
inline constexpr int INPUT_CONFIGURATION_COUNT = 0x02;
inline constexpr int SOUND_CONFIGURATION_COUNT = 0x09;
extern MediaConfigurationChoice VideoConfigurationRecords[VIDEO_CONFIGURATION_COUNT];
extern MediaConfigurationChoice MusicConfigurationRecords[MUSIC_CONFIGURATION_COUNT];
extern MediaConfigurationChoice InputConfigurationRecords[INPUT_CONFIGURATION_COUNT];
extern SoundConfigurationChoice SoundConfigurationRecords[SOUND_CONFIGURATION_COUNT];
extern const char CampaignClockSeparatorText[];
extern const char FormattedLineBreak[];
extern const char PathSeparatorText[];
extern const char PercentCloseText[];

/* DS:88BD is both the configured input-device selector and the tactical
 * mouse-edge scrolling flag. */
#define TacticalCursorAutoScroll InputDeviceType
/* DS:8432 and DS:8436 are entries 11h and 12h of the far-pointer table
 * beginning at DS:83EE.  Keep that original storage alias on the host: the
 * resource allocator and range-free procedure address the table entries,
 * while campaign presentation code uses their individual names. */
#define MissionSpriteBacking (SharedGraphicsBackingBuffers[0x11])
#define SavedMissionBacking  (SharedGraphicsBackingBuffers[0x12])
#define TacticalEffectSpriteTable \
    ((const uint8_t *const *) SharedGraphicsBackingBuffers)

extern char TacticalMapFileName[0x09];
extern char TacticalIconSetFileName[0x0D];
extern char TacticalDestroyIconFileName[0x0D];
extern char TacticalIconListFileName[0x0D];
extern char TacticalMechSetFileName[0x0D];

#endif
