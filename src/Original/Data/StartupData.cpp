// 3A6A: startup data
// StartupData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/StartupData.h"
#include "revenge/audio.h"
#include "revenge/video.h"
#include "revenge/story_text.h"

uint8_t VideoPathUnavailable;
uint16_t MapWidth;
uint16_t MapHeight;
uint16_t SavedBIOSVideoMode;
uint16_t GameInitialized;
uint16_t StartupResourceState; /* DS:5FCE: 0 in the image; startup sets it */
uint8_t *InvalidSaveDialogBacking;

const uint16_t BIOSVideoModeByAdapter[0x07] = {
    0x0004, 0x0009, 0x000D, 0x0013, 0x0007, 0x0002, 0x0000
};

const char MediaConfigurationFileName[] = "MEDIA.CFG";
char TacticalMapFileName[0x09] = "MAPx.MAP";
char TacticalIconSetFileName[0x0D] = "ICONSETx.ICN";
char TacticalDestroyIconFileName[0x0D] = "DESTROYx.ICN";
char TacticalIconListFileName[0x0D] = "ICONLIST.DAT";
char TacticalMechSetFileName[0x0D] = "MECHSETX.ICN";
const char *const InvalidSaveVersionText =
    STORY_TEXT(INVALID_SAVE_VERSION);
const char CampaignClockSeparatorText[] = ":";
const char FormattedLineBreak[] = "\r";
const char PathSeparatorText[] = "/";
const char PercentCloseText[] = "%)";
const char VersionDecimalSeparator[] = ".";

/* MEDIA.CFG's records (DS:3AD1-3C04), filled from the player's copy at start
 * (original_import_pieces.txt). */
MediaConfigurationChoice VideoConfigurationRecords[VIDEO_CONFIGURATION_COUNT];

MediaConfigurationChoice MusicConfigurationRecords[MUSIC_CONFIGURATION_COUNT];

MediaConfigurationChoice InputConfigurationRecords[INPUT_CONFIGURATION_COUNT];

SoundConfigurationChoice SoundConfigurationRecords[SOUND_CONFIGURATION_COUNT];

const char VersionTranslationSource[] = " ";
const char VersionTranslationReplacement[] = "0";
const char MediaConfigAssignmentDelimiter[] = "=";
const char VideoConfigTokenDelimiters[] = " \n\r";
const char MusicConfigTokenDelimiters[] = " \n\r";
const char SoundConfigTokenDelimiters[] = " \n\r";
const char SoundParameterTokenDelimiters[] = " \n\r";
const char InputConfigTokenDelimiters[] = " \n\r";

uint16_t NumericKeypadNavigationEnabled;
uint16_t StartupInputMode;
uint16_t SoundEffectsEnabled;
uint16_t DigitalSoundBusy;
uint16_t AppliedMusicEnabled;
uint16_t SoundDriverNumericParameter;
uint16_t InputDeviceType;
int8_t SoundDriverIndex;
uint8_t *Font6Resource;
uint8_t *Font8Resource;
uint8_t *SecondaryScreenBuffer;
uint8_t *PlanarScreenBuffer;
/* Initial values recovered from REVENGE.EXE DS:277B-2782. */
uint16_t MouseCursorBackingSourceTileX = 0x0000;
uint16_t MouseCursorBackingSourcePixelY = 0x0008;
uint16_t MouseCursorBackingWidthTiles = 0x0002;
uint16_t MouseCursorBackingHeightPixels = 0x0010;
uint8_t *SharedGraphicsBackingBuffers[0x26];
uint8_t *VideoFramebuffer;
uint16_t ResourceRuntimeInitialized;
