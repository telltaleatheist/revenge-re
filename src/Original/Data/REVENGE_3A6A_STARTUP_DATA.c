// REVENGE_3A6A_STARTUP_DATA.c
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "revenge/original_data.h"
#include "revenge/audio.h"
#include "revenge/far_memory.h"
#include "revenge/video.h"
#include "REVENGE.h"
#include "revenge/original_import.h"

word16 MemoryBlockSegment;
byte VideoPathUnavailable;
word16 MinimumProgramBreakOffset;
word16 MinimumProgramBreakSegment;
word16 CurrentProgramBreakOffset;
word16 MapWidth;
word16 MapHeight;
word16 CurrentProgramBreakSegment;
word16 MaximumProgramBreakOffset;
word16 MaximumProgramBreakSegment;
word16 SavedBIOSVideoMode;
word16 GameInitialized;
word16 StartupResourceState; /* DS:5FCE: 0 in the image; startup sets it */
byte *InvalidSaveDialogBacking;

const word16 BIOSVideoModeByAdapter[0x07] =
{
	0x0004, 0x0009, 0x000D, 0x0013, 0x0007, 0x0002, 0x0000
};

const char MediaConfigurationFileName[] = "MEDIA.CFG";
char TacticalMapFileName[0x09] = "MAPx.MAP";
char TacticalIconSetFileName[0x0D] = "ICONSETx.ICN";
char TacticalDestroyIconFileName[0x0D] = "DESTROYx.ICN";
char TacticalIconListFileName[0x0D] = "ICONLIST.DAT";
char TacticalMechSetFileName[0x0D] = "MECHSETX.ICN";
const char *const InvalidSaveVersionText =
	ORIGINAL_TEXT(0x2372);
const char CampaignClockSeparatorText[] = ":";
const char FormattedLineBreak[] = "\r";
const char PathSeparatorText[] = "/";
const char PercentCloseText[] = "%)";
const char VersionDecimalSeparator[] = ".";

/* MEDIA.CFG's records (DS:3AD1-3C04), filled from the player's copy at start
 * (original_import_pieces.txt). */
MediaConfigurationChoice VideoConfigurationRecords[0x05];

MediaConfigurationChoice MusicConfigurationRecords[0x07];

MediaConfigurationChoice InputConfigurationRecords[0x02];

SoundConfigurationChoice SoundConfigurationRecords[0x09];

const char VersionTranslationSource[] = " ";
const char VersionTranslationReplacement[] = "0";
const char MediaConfigAssignmentDelimiter[] = "=";
const char VideoConfigTokenDelimiters[] = " \n\r";
const char MusicConfigTokenDelimiters[] = " \n\r";
const char SoundConfigTokenDelimiters[] = " \n\r";
const char SoundParameterTokenDelimiters[] = " \n\r";
const char InputConfigTokenDelimiters[] = " \n\r";

word16 NumericKeypadNavigationEnabled;
word16 StartupInputMode;
word16 SoundEffectsEnabled;
word16 DigitalSoundBusy;
word16 AppliedMusicEnabled;
word16 SoundDriverNumericParameter;
word16 UseEgaTacticalFallback;
word16 InputDeviceType;
int8 SoundDriverIndex;
byte *Font6Resource;
byte *Font8Resource;
byte *SecondaryScreenBuffer;
byte *PlanarScreenBuffer;
/* Initial values recovered from REVENGE.EXE DS:277B-2782. */
word16 MouseCursorBackingSourceTileX = 0x0000;
word16 MouseCursorBackingSourcePixelY = 0x0008;
word16 MouseCursorBackingWidthTiles = 0x0002;
word16 MouseCursorBackingHeightPixels = 0x0010;
byte *SharedGraphicsBackingBuffers[0x20];
byte *VideoFramebuffer;
word16 ResourceRuntimeInitialized;
