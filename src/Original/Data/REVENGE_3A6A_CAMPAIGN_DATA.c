// REVENGE_3A6A_CAMPAIGN_DATA.c
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "revenge/original_data.h"
#include "revenge/original/campaign_data.h"
#include "revenge/audio.h"
#include "revenge/far_memory.h"
#include "revenge/video.h"
#include "REVENGE.h"
#include "revenge/original_import.h"

const char *const RestartConfirmationText =
	ORIGINAL_TEXT(0x2597);
const char *const MissionMapHeaderText = ORIGINAL_TEXT(0x2489);
char TextEntryBuffer[0x16];
word16 TextEntryCursorIndex;
byte TextEntryActive;
const char TextEntryPunctuation[] = "!\"&'+,-./:;?";
const char TextCursorBlinkGlyph[] = "_";
const char * const InGameMenuTextTable[0x0B] =
{
	ORIGINAL_TEXT(0x2A89),             /* DS:2A89 */
	ORIGINAL_TEXT(0x2A8E),             /* DS:2A8E */
	ORIGINAL_TEXT(0x2A93),         /* DS:2A93 */
	ORIGINAL_TEXT(0x2A9C),       /* DS:2A9C */
	ORIGINAL_TEXT(0x2AA7),  /* DS:2AA7 */
	ORIGINAL_TEXT(0x2AB7),      /* DS:2AB7 */
	ORIGINAL_TEXT(0x2AC3),        /* DS:2AC3 */
	ORIGINAL_TEXT(0x2ACD),  /* DS:2ACD */
	ORIGINAL_TEXT(0x2ADD),     /* DS:2ADD */
	ORIGINAL_TEXT(0x2AEA),     /* DS:2AEA */
	ORIGINAL_TEXT(0x2AF7)      /* DS:2AF7 */
};
const char *const SaveGameSeparatorText = ORIGINAL_TEXT(0x266E);

byte TrainingScenarioCompletionFlags[0x06];
int8 ScreenPaletteResourceByScreen[CAMPAIGN_SCREEN_PALETTE_COUNT];
CampaignSaveDirectory SaveDirectory;
RevengeCampaignSnapshot SaveCampaignSnapshot;

byte InitialCampaignState[CAMPAIGN_PRIMARY_STATE_COUNT];
byte ForwardCampaignState[CAMPAIGN_PRIMARY_STATE_COUNT];
byte ReplayCampaignState[CAMPAIGN_PRIMARY_STATE_COUNT];
byte InitialExtendedCampaignState[CAMPAIGN_EXTENDED_STATE_COUNT];
byte ForwardExtendedCampaignState[CAMPAIGN_EXTENDED_STATE_COUNT];
byte ReplayExtendedCampaignState[CAMPAIGN_EXTENDED_STATE_COUNT];

byte ScenarioOrdinalBaseByCampaignStage[CAMPAIGN_STAGE_COUNT];

byte MissionFadeColorTable[0x04];
word16 MissionBlinkRectX[CAMPAIGN_BLINK_RECT_COUNT];
word16 MissionBlinkRectY[CAMPAIGN_BLINK_RECT_COUNT];
byte MissionBlinkRectExtent[CAMPAIGN_BLINK_RECT_COUNT];
byte MissionDateMonthIndex[CAMPAIGN_MISSION_DATE_COUNT];
word16 MissionDateYear[CAMPAIGN_MISSION_DATE_COUNT];
word16 MissionDateAdvanceMonths[0x10];
word16 MissionSpriteSourceTileX[CAMPAIGN_SPRITE_LAYOUT_COUNT];
word16 MissionSpriteSourcePixelY[CAMPAIGN_SPRITE_LAYOUT_COUNT];
word16 MissionSpriteWidthTiles[CAMPAIGN_SPRITE_LAYOUT_COUNT];
word16 MissionSpriteHeightPixels[CAMPAIGN_SPRITE_LAYOUT_COUNT];

CampaignMessageEntry CampaignMessageTable[CAMPAIGN_MESSAGE_COUNT];

const byte MonthLengthTable[0x0C] =
{
	0x1F, 0x1C, 0x1F, 0x1E, 0x1F, 0x1E,
	0x1F, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F
};
const word16 CalendarBoundaryTable[0x0E] =
{
	0x001F, 0x0000, 0x001F, 0x003B, 0x005A, 0x0078, 0x0097,
	0x00B5, 0x00D4, 0x00F3, 0x0111, 0x0130, 0x014E, 0x016D
};
char StandardTimezoneName[0x04];
char DaylightTimezoneName[0x04];
word16 TimezoneOffsetLowWord = 0x4650;
word16 TimezoneOffsetHighWord = 0x0000;
word16 DaylightSavingEnabled = 0x0001;
const char TimezoneEnvironmentName[0x03] = "TZ";
const char DefaultStandardTimezoneName[0x04] = "EST";
const char DefaultDaylightTimezoneName[0x04] = "EDT";

