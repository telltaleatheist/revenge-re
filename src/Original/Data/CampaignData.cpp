// 3A6A: campaign data
// CampaignData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "revenge/original_data.h"
#include "revenge/original/campaign_data.h"
#include "revenge/audio.h"
#include "revenge/video.h"
#include "REVENGE.h"
#include "revenge/story_text.h"

const char *const RestartConfirmationText =
    STORY_TEXT(RESTART_CONFIRMATION);
const char *const MissionMapHeaderText = STORY_TEXT(MISSION_MAP_HEADER);
char TextEntryBuffer[0x16];
uint16_t TextEntryCursorIndex;
uint8_t TextEntryActive;
const char TextEntryPunctuation[] = "!\"&'+,-./:;?";
const char TextCursorBlinkGlyph[] = "_";
const char *const InGameMenuTextTable[0x0B] = {
    STORY_TEXT(IN_GAME_MENU_0),             /* DS:2A89 */
    STORY_TEXT(IN_GAME_MENU_1),             /* DS:2A8E */
    STORY_TEXT(IN_GAME_MENU_2),         /* DS:2A93 */
    STORY_TEXT(IN_GAME_MENU_3),       /* DS:2A9C */
    STORY_TEXT(IN_GAME_MENU_4),  /* DS:2AA7 */
    STORY_TEXT(IN_GAME_MENU_5),      /* DS:2AB7 */
    STORY_TEXT(IN_GAME_MENU_6),        /* DS:2AC3 */
    STORY_TEXT(IN_GAME_MENU_7),  /* DS:2ACD */
    STORY_TEXT(IN_GAME_MENU_8),     /* DS:2ADD */
    STORY_TEXT(IN_GAME_MENU_9),     /* DS:2AEA */
    STORY_TEXT(IN_GAME_MENU_10)      /* DS:2AF7 */
};
const char *const SaveGameSeparatorText = STORY_TEXT(SAVE_GAME_SEPARATOR);

uint8_t TrainingScenarioCompletionFlags[0x06];
/* Filled at start from data/ui/screens.json (revenge/rules.h). */
int8_t ScreenPaletteResourceByScreen[CAMPAIGN_SCREEN_PALETTE_COUNT];
CampaignSaveDirectory SaveDirectory;
RevengeCampaignSnapshot SaveCampaignSnapshot;

/* Filled at start from data/start/new_campaign.json (revenge/rules.h). */
uint8_t InitialCampaignState[CAMPAIGN_PRIMARY_STATE_COUNT];
uint8_t ForwardCampaignState[CAMPAIGN_PRIMARY_STATE_COUNT];
uint8_t ReplayCampaignState[CAMPAIGN_PRIMARY_STATE_COUNT];
uint8_t InitialExtendedCampaignState[CAMPAIGN_EXTENDED_STATE_COUNT];
uint8_t ForwardExtendedCampaignState[CAMPAIGN_EXTENDED_STATE_COUNT];
uint8_t ReplayExtendedCampaignState[CAMPAIGN_EXTENDED_STATE_COUNT];

/* Filled at start from data/rules/campaign.json (revenge/rules.h). */
uint8_t ScenarioOrdinalBaseByCampaignStage[CAMPAIGN_STAGE_COUNT];

/* Filled at start from data/ui/mission.json (revenge/rules.h). */
/* Filled at start from data/ui/mission.json (revenge/rules.h). */
uint8_t MissionFadeColorTable[0x04];
uint16_t MissionBlinkRectX[CAMPAIGN_BLINK_RECT_COUNT];
uint16_t MissionBlinkRectY[CAMPAIGN_BLINK_RECT_COUNT];
uint8_t MissionBlinkRectExtent[CAMPAIGN_BLINK_RECT_COUNT];
/* Filled at start from data/rules/campaign.json (revenge/rules.h). */
uint8_t MissionDateMonthIndex[CAMPAIGN_MISSION_DATE_COUNT];
uint16_t MissionDateYear[CAMPAIGN_MISSION_DATE_COUNT];
uint16_t MissionDateAdvanceMonths[CAMPAIGN_MISSION_DATE_COUNT];
/* Filled at start from data/ui/mission.json (revenge/rules.h). */
/* Filled at start from data/ui/mission.json (revenge/rules.h). */
uint16_t MissionSpriteSourceTileX[CAMPAIGN_SPRITE_LAYOUT_COUNT];
uint16_t MissionSpriteSourcePixelY[CAMPAIGN_SPRITE_LAYOUT_COUNT];
uint16_t MissionSpriteWidthTiles[CAMPAIGN_SPRITE_LAYOUT_COUNT];
uint16_t MissionSpriteHeightPixels[CAMPAIGN_SPRITE_LAYOUT_COUNT];

/* Filled at start from data/ui/screens.json (revenge/rules.h). */
CampaignMessageEntry CampaignMessageTable[CAMPAIGN_MESSAGE_COUNT];

const uint8_t MonthLengthTable[0x0C] = {
    0x1F, 0x1C, 0x1F, 0x1E, 0x1F, 0x1E,
    0x1F, 0x1F, 0x1E, 0x1F, 0x1E, 0x1F
};
const uint16_t CalendarBoundaryTable[0x0D] = {
    0x0000, 0x001F, 0x003B, 0x005A, 0x0078, 0x0097, 0x00B5,
    0x00D4, 0x00F3, 0x0111, 0x0130, 0x014E, 0x016D
};
char StandardTimezoneName[0x04];
char DaylightTimezoneName[0x04];
uint16_t TimezoneOffsetLowWord = 0x4650;
uint16_t TimezoneOffsetHighWord = 0x0000;
uint16_t DaylightSavingEnabled = 0x0001;
const char TimezoneEnvironmentName[0x03] = "TZ";
const char DefaultStandardTimezoneName[0x04] = "EST";
const char DefaultDaylightTimezoneName[0x04] = "EDT";
