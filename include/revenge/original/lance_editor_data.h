#ifndef REVENGE_ORIGINAL_LANCE_EDITOR_DATA_H
#define REVENGE_ORIGINAL_LANCE_EDITOR_DATA_H

#include "revenge/original/campaign_data.h"
#include "Data/TacticalData.h"
#include "revenge/roster.h"
#include "revenge/debug.h"
#include "revenge/types.h"

/* DS:83EE-8485: 26h far pointers; the pilots' portraits fill 17h-25h. */
extern uint8_t *SharedGraphicsBackingBuffers[0x26];

enum
{
    LANCE_EDITOR_DATA_SEGMENT = 0x3A6A,
    UNIT_SELECTION_CARD_COUNT = 0x12,
    LANCE_EDITOR_GRAPHICS_BUFFER_COUNT = 0x20,
    LANCE_EDITOR_SECTION_COUNT = 0x04,
    /* The lance editor's sections after the briefing: the pilots, the
     * units, and the last, which ends the editor (in the pilot and unit
     * editors too). */
    LANCE_EDITOR_SECTION_PILOTS = 0x01,
    LANCE_EDITOR_SECTION_UNITS = 0x02,
    LANCE_EDITOR_SECTION_DONE = 0x03,
    LANCE_EDITOR_SECTION_LAYOUT_COUNT = 0x08,
    LANCE_EDITOR_UNIT_TYPE_COUNT = 0x59,
    LANCE_EDITOR_PILOT_NAME_COUNT = 0x3B
};

/* These aliases are two domain names for the same original DS bytes. */
#define TrainingScenarioAvailability TrainingScenarioCompletionFlags
#define RandomUnitTypePool           ReplayCampaignState
#define RandomPilotPool              ReplayExtendedCampaignState
/* DS:1B8E: the pool's count is the replay copy of its count. */
#define RandomSelectionPoolCount RandomPilotPoolCounts.replay

extern UnitSelectionCard UnitSelectionCards[UNIT_SELECTION_CARD_COUNT];
/* The cards as the image holds them at DS:1BBC (the rules' starting state),
 * read into UnitSelectionCards when the rules are loaded. */
extern RevengeSavedSelectionCard UnitSelectionCardStart[UNIT_SELECTION_CARD_COUNT];
extern uint16_t LanceEditorFirstBriefingPending;
extern uint16_t LanceEditorBriefingPresented;
extern uint16_t LanceEditorReloadGraphics;

extern const char *UnitAllegianceLabelTextTable[0x02];
extern const char *const UnknownUnitLabel;
extern const char *const UnitStatusSpace;
extern const char *const UnitRosterSeparator;
extern const char *const UnitStatusSeparator;
extern const char *UnitStatHeaderTextTable[0x06];

extern uint16_t UnitEditorLanceSlotCount[0x03];
extern uint8_t RandomUnitTypeCount;
extern uint8_t UnitEditorUnitSpriteIndex[LANCE_EDITOR_UNIT_TYPE_COUNT];
/* DS:3172: by unit type, where DrawCenteredClippedText splits the unit's
 * name (unit_name.N) onto two lines; the edition's own, with its names. */
extern uint8_t UnitNameSplitIndex[LANCE_EDITOR_UNIT_TYPE_COUNT];
extern uint8_t PredefinedUnitTypeTable[0x1E];
extern uint8_t UnitEditorSectionEnabled[0x1D];
extern uint8_t UnitEditorScenarioCardCountByFamily[0x07];
extern uint8_t RandomUnitTypeCountByFamily[0x0A];
extern uint8_t RandomUnitTypePoolByFamily[0x31];
extern uint8_t PilotPortraitIndexById[0x21];
extern uint16_t DefaultPilotIdsByScenario[0x14];
extern uint8_t UnitEditorPilotCardCountByFamily[0x07];

/* Host pointers into UnitClassTypeListBytes, or NULL, set by the rules. */
extern const uint8_t *UnitClassTypeLists[0x07];
extern const char *UnitClassLabelTextTable[0x07];
extern const char *TrainingDeploymentLabelTextTable[0x04];
extern const char *TrainingControlLabelTextTable[0x04];
extern const char *ScenarioVariantDescriptionTextTable[0x04];
extern const char *UnitEditorSectionLabelTextTable[0x04];
extern uint8_t FixedPilotIds[0x02];
extern uint8_t FixedUnitTypeIds[0x02];
extern uint8_t UnitClassTypeListBytes[0x44];
extern const char *ScenarioRouteLabelTextTable[0x04];
extern const char *UnitEditorSectionButtonLabelTextTable[0x04];
extern const char *TrainingDeploymentRosterLabelTextTable[0x02];
extern const char *UnitRosterColumnLabelTextTable[0x03];
extern const char *FriendlyEnemySuffixText;
extern const char *const TrainingCategoryHeadingText;
extern const char *TrainingCategoryLabelTextTable[0x02];
extern const char *const TrainingScenarioHeadingText;
extern char *TrainingScenarioLabelTextTable[0x06];
extern const char *const NoSelectionOptionsText;
extern const char *const TrainingUnitSelectionPrompt;

#pragma pack(push, 1)
typedef struct LanceEditorLayoutData
{
    uint16_t TrainingUnitChoiceLeft[0x0C];
    uint16_t TrainingUnitChoiceTop[0x0C];
    uint16_t TrainingControlChoiceLeft[0x04];
    uint16_t TrainingControlChoiceTop[0x04];
    /* Three group columns; 2507 also draws the frame's lines to [0x02] + 1. */
    uint16_t TrainingUnitGroupLeft[0x03];
    uint16_t TrainingUnitGroupLabelLeft[0x03];
    uint16_t TrainingUnitGroupLabelWidth[0x03];
    uint8_t AllegianceHeaderColor[0x02];
    uint16_t TrainingDeploymentSlotLeft[0x03];
    uint16_t TrainingControlChoiceColumn[0x02];
    uint16_t TrainingCategoryRegionLeft[0x02];
    uint16_t TrainingCategoryRegionTop[0x02];
    uint16_t TrainingCategoryRegionWidth[0x02];
    uint16_t TrainingCategoryRegionHeight[0x02];
    uint16_t TrainingScenarioRegionLeft[0x06];
    uint16_t TrainingScenarioRegionTop[0x06];
    uint16_t TrainingScenarioRegionWidth[0x06];
    uint16_t TrainingScenarioRegionHeight[0x06];
    /* Three deployment columns; [0x02] + 1 also ends the frame's lines. */
    uint16_t TrainingDeploymentFrameLeft[0x03];
    uint16_t TrainingDeploymentTextColor[0x02];
    uint16_t TrainingDeploymentBackgroundColor[0x02];
    uint16_t TrainingRosterLabelLeft[0x03];
    uint16_t TrainingRosterLabelWidth[0x03];
    uint16_t TrainingCategoryTextColor[0x02];
    uint16_t TrainingCategoryBackgroundColor[0x02];
    uint16_t TrainingScenarioTextColor[0x02];
    uint16_t TrainingScenarioBackgroundColor[0x02];
    uint16_t UnitEditorSectionLeft[0x08];
    uint16_t UnitEditorSectionTop[0x08];
    uint16_t UnitEditorSectionWidth[0x08];
    uint16_t UnitEditorSectionHeight[0x08];
    uint16_t ScenarioVariantTextLeft[0x02];
    uint16_t ScenarioVariantTextTop[0x02];
    uint16_t ScenarioVariantNormalColor;
    uint16_t ScenarioVariantSelectedColor;
    uint16_t ScenarioVariantLabelColor[0x04];
    uint16_t ScenarioVariantRegionLeft[0x02];
    uint16_t ScenarioVariantRegionTop[0x02];
    uint16_t ScenarioVariantRegionWidth[0x02];
    uint16_t ScenarioVariantRegionHeight[0x02];
    uint16_t UnitRosterLeftColumn;
    uint16_t UnitRosterRightColumn;
    uint16_t UnitEditorSectionRows[0x04];
    uint16_t UnitEditorSectionButtonRows[0x04];
    uint16_t UnitCardBackgroundColorByState[0x04];
    uint16_t UnitCardTextColorByState[0x04];
    uint16_t LanceAssignmentTextColor[0x02];
    uint16_t PilotEditorSectionRows[0x04];
    uint16_t PilotCardTextColor[0x04];
    uint16_t LanceBackingSourceTileX[0x04];
    uint16_t LanceBackingSourcePixelY[0x04];
    uint16_t LanceBackingWidthTiles[0x04];
    uint16_t LanceBackingHeightPixels[0x04];
    uint16_t ScenarioBackingSourceTileX[0x02];
    uint16_t ScenarioBackingSourcePixelY[0x02];
    uint16_t ScenarioBackingWidthTiles[0x02];
    uint16_t ScenarioBackingHeightPixels[0x02];
    uint16_t EditorBackingSourceTileX[0x08];
    uint16_t EditorBackingSourcePixelY[0x08];
    uint16_t EditorBackingWidthTiles[0x08];
    uint16_t EditorBackingHeightPixels[0x08];
} LanceEditorLayoutData;
#pragma pack(pop)

static_assert(sizeof(LanceEditorLayoutData) == 0x01D4,
    "Lance editor layout must retain DS:3470-3643 byte layout");

extern LanceEditorLayoutData LanceEditorLayout;

extern const char *PilotSurnameTextTable[LANCE_EDITOR_PILOT_NAME_COUNT];
extern const char *PilotGivenNameTextTable[LANCE_EDITOR_PILOT_NAME_COUNT];
extern const char *PilotExperienceLabelTextTable[0x04];
extern const char *PilotGunneryLabelTextTable[0x04];
extern const char *UnitMovementClassLabelTextTable[0x05];
extern const char *UnitArmorConditionLabelTextTable[0x06];
extern const char *const UnitStatusEquipmentSeparator;

#define UnitEditorGraphicsBuffer(index) \
    SharedGraphicsBackingBuffers[REVENGE_INDEX(SharedGraphicsBackingBuffers, (index))]

#endif
