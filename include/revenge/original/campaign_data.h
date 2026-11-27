#ifndef REVENGE_ORIGINAL_CAMPAIGN_DATA_H
#define REVENGE_ORIGINAL_CAMPAIGN_DATA_H

#include "revenge/save_data.h"
#include "revenge/types.h"

#include <stddef.h>

enum
{
    CAMPAIGN_DATA_SEGMENT = 0x3A6A,
    CAMPAIGN_PRIMARY_STATE_COUNT = 0x07,
    CAMPAIGN_EXTENDED_STATE_COUNT = 0x0F,
    CAMPAIGN_STAGE_COUNT = 0x16
};

#pragma pack(push, 1)
/* A campaign message: its text's place in the reference edition (the game
 * shows CampaignMessageTextTable's, the story's), and its window's height in
 * tiles. */
typedef struct CampaignMessageEntry
{
    DosAddress text;
    int8 windowHeight;
} CampaignMessageEntry;

REVENGE_ANONYMOUS_STRUCTS_BEGIN
typedef union CampaignSaveDirectory
{
    struct
    {
        byte prefix[REVENGE_SAVE_DIRECTORY_PREFIX_SIZE];
        RevengeSaveSlotDescriptor slots[REVENGE_SAVE_SLOT_COUNT];
    };
    byte raw[REVENGE_SAVE_DIRECTORY_PREFIX_SIZE +
        REVENGE_SAVE_SLOT_COUNT * REVENGE_SAVE_DESCRIPTOR_SIZE];
} CampaignSaveDirectory;
REVENGE_ANONYMOUS_STRUCTS_END
#pragma pack(pop)

static_assert(sizeof(CampaignMessageEntry) == 0x05,
    "Campaign message entries must retain their original five-byte layout");
static_assert(offsetof(CampaignMessageEntry, windowHeight) == 0x04,
    "Campaign message heights must remain at entry offset 04h");
static_assert(offsetof(CampaignSaveDirectory, slots) ==
        REVENGE_SAVE_DIRECTORY_PREFIX_SIZE,
    "Save descriptors must follow the original four-byte directory prefix");

enum
{
    CAMPAIGN_TITLE_TEXT_COUNT = 0x12,
    CAMPAIGN_MONTH_COUNT = 0x0C,
    CAMPAIGN_MISSION_DATE_COUNT = 0x0C,
    CAMPAIGN_SCREEN_PALETTE_COUNT = 0x25,
    CAMPAIGN_BLINK_RECT_COUNT = 0x07,
    CAMPAIGN_SPRITE_LAYOUT_COUNT = 0x0C,
    CAMPAIGN_MESSAGE_COUNT = 0xB2
};

extern byte TrainingScenarioCompletionFlags[0x06];
/* DS:1704: the palette of screens 1-25h, the first entry screen 1's. The
 * original indexes it from DS:1703, the last cached-file descriptor's last
 * byte, with screens that start at 1. */
extern int8 ScreenPaletteResourceByScreen[CAMPAIGN_SCREEN_PALETTE_COUNT];
extern CampaignSaveDirectory SaveDirectory;
extern RevengeCampaignSnapshot SaveCampaignSnapshot;
/* DS:197B and DS:1993: the units' map positions a battle starts from are the
 * snapshot's (offsets 25h and 3Dh), the ones a save writes and a load reads. */
#define SavedScenarioUnitMapX (SaveCampaignSnapshot.scenarioUnitMapX)
#define SavedScenarioUnitMapY (SaveCampaignSnapshot.scenarioUnitMapY)

extern byte InitialCampaignState[CAMPAIGN_PRIMARY_STATE_COUNT];
extern byte ForwardCampaignState[CAMPAIGN_PRIMARY_STATE_COUNT];
extern byte ReplayCampaignState[CAMPAIGN_PRIMARY_STATE_COUNT];
extern byte InitialExtendedCampaignState[CAMPAIGN_EXTENDED_STATE_COUNT];
extern byte ForwardExtendedCampaignState[CAMPAIGN_EXTENDED_STATE_COUNT];
extern byte ReplayExtendedCampaignState[CAMPAIGN_EXTENDED_STATE_COUNT];

extern const char *TitleTextTable[CAMPAIGN_TITLE_TEXT_COUNT];
extern const char *MonthNameTextTable[CAMPAIGN_MONTH_COUNT];
extern const char *MissionDateLabelTextTable[CAMPAIGN_MISSION_DATE_COUNT];
extern byte ScenarioOrdinalBaseByCampaignStage[CAMPAIGN_STAGE_COUNT];

extern const char *const ExitToDosPrompt;
extern const char *CampaignPathMenuTextTable[0x02];
extern const char *ScenarioVariantMenuTextTable[0x02];
extern const char *AlternateScenarioMenuTextTable[0x02];
extern const char *MainMenuTextTable[0x03];
extern const char *const InGameMenuTextTable[0x0B];
extern const char *const SaveGameSeparatorText;

extern byte MissionFadeColorTable[0x04];
extern word16 MissionBlinkRectX[CAMPAIGN_BLINK_RECT_COUNT];
extern word16 MissionBlinkRectY[CAMPAIGN_BLINK_RECT_COUNT];
extern byte MissionBlinkRectExtent[CAMPAIGN_BLINK_RECT_COUNT];
extern byte MissionDateMonthIndex[CAMPAIGN_MISSION_DATE_COUNT];
extern word16 MissionDateYear[CAMPAIGN_MISSION_DATE_COUNT];
/* DS:2763: one word for each mission date, as the date tables beside it. */
extern word16 MissionDateAdvanceMonths[CAMPAIGN_MISSION_DATE_COUNT];
extern word16 MissionSpriteSourceTileX[CAMPAIGN_SPRITE_LAYOUT_COUNT];
extern word16 MissionSpriteSourcePixelY[CAMPAIGN_SPRITE_LAYOUT_COUNT];
extern word16 MissionSpriteWidthTiles[CAMPAIGN_SPRITE_LAYOUT_COUNT];
extern word16 MissionSpriteHeightPixels[CAMPAIGN_SPRITE_LAYOUT_COUNT];

extern const char *const PilotIdentityPrefixText;
extern const char *const PilotNamePrefixText;
extern const char *const DateCommaSpaceText;
extern const char *const DateSpaceText;
extern const char *const ScenarioCompleteHeadingText;
extern const char *const ScenarioFailedHeadingText;
extern CampaignMessageEntry CampaignMessageTable[CAMPAIGN_MESSAGE_COUNT];
extern const char *CampaignMessageTextTable[CAMPAIGN_MESSAGE_COUNT];

extern const char *const PilotRosterHeadingText;

#endif
