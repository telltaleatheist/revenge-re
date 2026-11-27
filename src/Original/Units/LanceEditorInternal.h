// 2507: lance editor declarations
#ifndef REVENGE_UNITS_LANCE_EDITOR_INTERNAL_H
#define REVENGE_UNITS_LANCE_EDITOR_INTERNAL_H

#include "revenge/input.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/video.h"
#include "revenge/debug.h"

#include <stdbool.h>

enum
{
    LIVE_SCENARIO_UNITS_OFFSET = 0x99E0,
    TEXT_WINDOW_TILE_SIZE_PIXELS = 0x0008,
    UNIT_CLASS_COUNT = 0x06,
    UNIT_CLASS_MENU_ENTRY_COUNT = 0x07,
    UNIT_CLASS_MENU_CANCEL_INDEX = 0x06,
    UNIT_SELECTION_TEXT_WINDOW_INDEX = 0x11,
    UNIT_SELECTION_UI_REGION_INDEX = 0x0B,
    UNIT_ALLEGIANCE_ENEMY = 0x01,
    UNIT_CLASS_OMNI_INDEX = 0x05,      /* its list is at DS:32D7 in the original */
    UNIT_ROSTER_COLUMN_X_ADJUSTMENT = 0x0001,
    UNIT_ROSTER_FIRST_ROW_Y = 0x00B0,
    UNIT_ROSTER_TEXT_WIDTH_PIXELS = 0x009E,
    /* The lance slot past the four: the editor's exit button. */
    LANCE_EDITOR_EXIT_SLOT = LANCE_UNIT_COUNT
};

int16_t RandomInRangeInclusive(uint16_t minimum, uint16_t maximum);
int16_t PopRandomUnitType(void);
int16_t PopRandomPilotId(void);
int16_t SelectUnitClass(void);
int16_t SelectUnitTypeFromList(const uint8_t *unitTypeIdList);
bool IsOmniUnitTypeList(const uint8_t *unitTypeIdList);
uint16_t ServiceMusicAndCheckKeyboardInput(void);
uint16_t RunGameOptionsMenu(uint16_t embeddedMode);
void DrawUnitAllegianceHeader(uint16_t headerColorIndex);
void RenderFormattedText(const char *text);
uint16_t ValidatePilotConfiguration(uint16_t selectionFamily);
uint16_t ValidateLanceConfiguration(uint16_t selectionFamily);
void ShowNoSelectionOptions(void);
void RunPilotSelection(uint16_t selectionFamily, int16_t selectionMode);
void PrepareLanceSelection(uint16_t firstRosterIndex, int16_t selectionMode);
void PresentStoryScreenAndMessage(
    uint16_t screenResourceIndex,
    uint16_t messageIndex);
void ShowStoryMessage(uint16_t messageIndex);
void DrawUnitRosterScreen(
    uint16_t useAlternateAssignmentLayout,
    uint16_t textColor);
void DrawRangeAndSpeedStatHeaders(uint16_t topY);
void DrawExperienceAndGunneryHeaders(uint16_t topY);
void LoadScreenPalette(uint16_t paletteIndex);
uint8_t *BuildDisplayBackingBuffer(
    uint16_t sourceTileX,
    uint16_t sourcePixelY,
    uint16_t widthTiles,
    uint16_t heightPixels);
void LoadGraphicsResourceToBuffers(
    uint16_t resourceIndex,
    uint8_t *decodeBuffer,
    uint8_t *displayBuffer);
void LoadGraphicsBufferRange(
    int16_t firstBufferIndex,
    int16_t lastBufferIndex);
void LoadUnitTemplateRecord(int16_t unitType);
void InitializeLanceAssignments(
    uint16_t firstRosterIndex,
    int16_t selectionMode);
void SnapshotLateCampaignLanceUnits(void);
int16_t RunLanceConfigurationEditor(
    uint16_t selectionFamily,
    uint16_t scenarioVariantIndex);
void PrepareLanceSelectionRecords(
    uint16_t firstRosterIndex,
    int16_t selectionMode,
    uint16_t loadPresentationResources);
void RunUnitAssignmentEditor(uint16_t firstRosterIndex);
void DrawSelectionScreenBackground(void);
void DrawLanceSelectionScreen(uint16_t configurationIndex);
void RefreshLanceSelectionSlots(
    uint16_t scenarioGroupIndex,
    uint16_t lanceGroupIndex,
    uint16_t showSelectionContents);
int16_t DrawUnitEditorSectionMenu(uint16_t configurationIndex);
void ClearUnitEditorSectionPanel(void);
void DrawLanceAssignmentSlot(
    uint16_t assignmentIndex,
    uint16_t selected,
    uint16_t drawExitSlot);
void PreparePilotSelectionRecords(
    uint16_t selectionFamily,
    int16_t selectionMode,
    uint16_t loadPresentationResources);
void RunPilotAssignmentEditor(uint16_t selectionFamily);
void DrawPilotSelectionScreen(uint16_t selectionFamily);
void DrawPilotSelectionSlot(
    uint16_t pilotRecordIndex,
    uint16_t redrawBackground,
    int16_t displayMode);
void RefreshPilotAssignmentSlots(
    uint16_t selectionFamily,
    uint16_t showPilotCards);
void FreeGraphicsBufferRange(
    int16_t firstBufferIndex,
    int16_t lastBufferIndex);
void LoadScenarioVariantGraphicsBuffers(void);
void LoadLanceConfigurationGraphicsBuffers(void);
void ShowLanceConfigurationBriefing(uint16_t configurationIndex);
void RunUnitAssignmentGrid(
    uint16_t scenarioGroupIndex,
    uint16_t lanceGroupIndex,
    uint16_t useSeparateScenarioGroups);
void RunPilotAssignmentGrid(uint16_t lanceGroupIndex);
void DrawUnitStatusScreen(
    uint16_t statusPanelIndex,
    uint16_t redrawBackground,
    uint16_t displayMode,
    uint16_t showDetailedStats);
void CountUnitWeaponsByType(const RevengeUnitRecord *unitRecord);
uint16_t CalculateUnitWeaponHeatScale(RevengeUnitRecord *unitRecord);
void OpenModalTextWindow(uint16_t windowIndex, uint16_t saveBackground);
void RestoreModalTextWindow(void);
void DrawTrainingCategoryOption(
    uint16_t optionIndex,
    uint16_t selectedState);
void DrawTrainingScenarioOption(
    uint16_t scenarioIndex,
    uint16_t selectedState);
void DrawTrainingScenarioSelectionScreen(void);
void DrawTrainingDeploymentSlot(
    uint16_t slotIndex,
    uint16_t selectedState);
void DrawTrainingDeploymentScreen(void);
void DrawTrainingUnitChoice(uint16_t choiceIndex, uint16_t selectedState);
void DrawTrainingControlChoice(uint16_t choiceIndex, uint16_t selectedState);
void DrawTrainingChoiceButton(
    uint16_t leftX,
    uint16_t topY,
    const char *label,
    uint16_t width);

/* A selection card's sprite, for drawing. The editor points every card it
 * builds at one of its graphics buffers before drawing it (2507:2BD9 for the
 * units, 2507:5016 for the pilots). A card loaded from a save keeps the
 * pointer the saving program had, unchanged by the load, so a card drawn
 * before the editor rebuilds it would point at nothing of ours: that is a
 * fault naming the card's pointer. */
const uint8_t *LanceEditorCardSprite(const UnitSelectionCard *card);

#endif
