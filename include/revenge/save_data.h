#ifndef REVENGE_SAVE_DATA_H
#define REVENGE_SAVE_DATA_H

#include <stddef.h>

#include "revenge/roster.h"
#include "revenge/types.h"
#include "revenge/unit.h"

enum
{
    REVENGE_SAVE_FILE_SIZE = 0x61C0,
    REVENGE_SAVE_BOUNDARY_COUNT = 0x09,
    REVENGE_SAVE_DIRECTORY_OFFSET = 0x0012,
    REVENGE_SAVE_DIRECTORY_PREFIX_SIZE = 0x04,
    REVENGE_SAVE_DESCRIPTOR_OFFSET = 0x0016,
    REVENGE_SAVE_DESCRIPTOR_SIZE = 0x16,
    REVENGE_SAVE_DESCRIPTOR_NAME_SIZE = 0x15,
    REVENGE_SAVE_EDITOR_LABEL_SIZE = 0x11,
    REVENGE_SAVE_SLOT_COUNT = 0x06,
    REVENGE_SAVE_PAYLOAD_SIZE = 0x1031,
    REVENGE_CAMPAIGN_SNAPSHOT_SIZE = 0x0221,
    REVENGE_SAVE_UNIT_BLOCK_OFFSET = 0x0221,
    REVENGE_SAVE_UNIT_COUNT = 0x18,
    REVENGE_SAVE_SCENARIO_RECORD_COUNT = 0x12,
    REVENGE_SAVE_SCENARIO_RECORD_SIZE = 0x0F,
    REVENGE_SAVE_LANCE_ASSIGNMENT_COUNT = 0x0D,
    REVENGE_SAVE_PILOT_COUNT = 0x25
};

#pragma pack(push, 1)
typedef struct RevengeSaveSlotDescriptor
{
    uint8_t occupied;
    char name[REVENGE_SAVE_DESCRIPTOR_NAME_SIZE];
} RevengeSaveSlotDescriptor;

typedef struct RevengeCampaignSnapshot
{
    uint16_t campaignStageIndex;
    uint16_t scenarioVariantIndex;
    uint8_t campaignPhase;
    uint8_t trainingSequenceFlag;
    uint16_t campaignFlags;
    uint16_t state24E1;
    uint16_t stateA967;
    uint16_t scene3RosterSelector9991;
    uint16_t state88F1;
    uint16_t state8925;
    uint16_t state99C5;
    uint16_t stateA965;
    uint16_t state8594;
    uint16_t lrmShipmentLoaded;
    uint16_t srmShipmentLoaded;
    uint8_t scenarioClock[0x03];
    uint8_t trainingScenarioCompletionFlags[0x06];
    uint8_t scenarioUnitMapX[REVENGE_SAVE_UNIT_COUNT];
    uint8_t scenarioUnitMapY[REVENGE_SAVE_UNIT_COUNT];
    uint8_t state1B85[0x07];
    uint8_t state1B8E;
    uint8_t state1BAD[0x0F];
    RevengeSavedSelectionCard scenarioRecords[REVENGE_SAVE_SCENARIO_RECORD_COUNT];
    RevengeUnitLanceAssignment
        lanceAssignments[REVENGE_SAVE_LANCE_ASSIGNMENT_COUNT];
    uint8_t pilotAvailability[REVENGE_SAVE_PILOT_COUNT];
} RevengeCampaignSnapshot;

typedef struct RevengeSaveSlot
{
    RevengeSaveSlotDescriptor descriptor;
    RevengeCampaignSnapshot campaign;
    uint8_t liveUnits[REVENGE_SAVE_UNIT_COUNT][UNIT_RECORD_SIZE];
} RevengeSaveSlot;

typedef struct RevengeSaveGame
{
    uint8_t directoryPrefix[REVENGE_SAVE_DIRECTORY_PREFIX_SIZE];
    RevengeSaveSlot slots[REVENGE_SAVE_SLOT_COUNT];
} RevengeSaveGame;
#pragma pack(pop)

static_assert(sizeof(RevengeSaveSlotDescriptor) ==
        REVENGE_SAVE_DESCRIPTOR_SIZE,
    "Save-slot descriptors must retain their DOS file layout");
static_assert(offsetof(RevengeSaveSlotDescriptor, name) == 0x01,
    "Save-slot names must remain at descriptor offset 01h");
static_assert(sizeof(RevengeCampaignSnapshot) ==
        REVENGE_CAMPAIGN_SNAPSHOT_SIZE,
    "Campaign snapshots must retain their DOS file layout");
static_assert(sizeof(RevengeSavedSelectionCard) == REVENGE_SAVE_SCENARIO_RECORD_SIZE,
    "Scenario cards must retain their save-file record size");
static_assert(offsetof(RevengeCampaignSnapshot, scenarioClock) == 0x1C,
    "Campaign clock must remain at snapshot offset 1Ch");
static_assert(offsetof(RevengeCampaignSnapshot, scenarioUnitMapX) == 0x25,
    "Scenario unit X coordinates must remain at snapshot offset 25h");
static_assert(offsetof(RevengeCampaignSnapshot, scenarioRecords) == 0x6C,
    "Scenario records must remain at snapshot offset 6Ch");
static_assert(offsetof(RevengeCampaignSnapshot, lanceAssignments) == 0x17A,
    "Lance assignments must remain at snapshot offset 17Ah");
static_assert(offsetof(RevengeCampaignSnapshot, pilotAvailability) == 0x1FC,
    "Pilot availability must remain at snapshot offset 1FCh");
static_assert(sizeof(RevengeSaveSlot) ==
        REVENGE_SAVE_DESCRIPTOR_SIZE + REVENGE_SAVE_PAYLOAD_SIZE,
    "Save slots must remain one descriptor plus one original payload");

#endif
