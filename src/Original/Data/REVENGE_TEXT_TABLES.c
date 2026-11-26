// REVENGE_TEXT_TABLES.c
// The text tables of REVENGE.EXE's data segment. The player's copy fills them
// at start (OriginalImport_Load, src/Support/original_import_pieces.txt); this file
// holds only their declarations and the DS offsets of the single strings.

#include "revenge/original_import.h"
#include "revenge/original/campaign_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/original/scene_data.h"

const char *TitleTextTable[0x12];

const char *MonthNameTextTable[0x0C];

const char *MissionDateLabelTextTable[0x0C];

const char *const ExitToDosPrompt = ORIGINAL_TEXT(0x2562);

const char *CampaignPathMenuTextTable[0x02];

const char *ScenarioVariantMenuTextTable[0x02];

const char *AlternateScenarioMenuTextTable[0x02];

const char *MainMenuTextTable[0x03];

const char *const PilotIdentityPrefixText = ORIGINAL_TEXT(0x2B04);
const char *const PilotNamePrefixText = ORIGINAL_TEXT(0x2B09);
const char *const DateCommaSpaceText = ORIGINAL_TEXT(0x2B0B);
const char *const DateSpaceText = ORIGINAL_TEXT(0x2B0D);
const char *const ScenarioCompleteHeadingText = ORIGINAL_TEXT(0x2B11);
const char *const ScenarioFailedHeadingText = ORIGINAL_TEXT(0x2B31);

const char *CampaignMessageTextTable[CAMPAIGN_MESSAGE_COUNT];

const char *const PilotRosterHeadingText = ORIGINAL_TEXT(0x47B4);

char ScenarioResourcePathText[0x0B];
const char *const SceneFileTooLargeText = ORIGINAL_TEXT(0x2F0E);

const char *GameOptionLabelTextTable[0x07];

const char *GameOptionToggleTextTable[0x02];

const char *NarrationModeLabelTextTable[0x03];

const char *InputDeviceLabelTextTable[0x02];

const char *const FocusUnitText = ORIGINAL_TEXT(0x46E6);
const char *const FocusLanceText = ORIGINAL_TEXT(0x46F1);

char *DisabledUnitNarrationTextTable[0x03];

char *DestroyedUnitNarrationTextTable[0x03];

char *ScenarioEventNarrationTextTable[0x1E];

const char *const SceneNarrationSeparatorText = ORIGINAL_TEXT(0x478D);

const char *UnitAllegianceLabelTextTable[0x02];

const char *const UnknownUnitLabel = ORIGINAL_TEXT(0x30D3);
const char *const UnitStatusSpace = ORIGINAL_TEXT(0x30D9);
const char *const UnitRosterSeparator = ORIGINAL_TEXT(0x30DB);
const char *const UnitStatusSeparator = ORIGINAL_TEXT(0x30DD);

const char *UnitStatHeaderTextTable[0x06];

const byte *UnitClassTypeLists[0x07];

const char *UnitClassLabelTextTable[0x07];

const char *TrainingDeploymentLabelTextTable[0x04];

const char *TrainingControlLabelTextTable[0x04];

const char *ScenarioVariantDescriptionTextTable[0x04];

const char *UnitEditorSectionLabelTextTable[0x04];

const char *ScenarioRouteLabelTextTable[0x04];

const char *UnitEditorSectionButtonLabelTextTable[0x04];

const char *TrainingDeploymentRosterLabelTextTable[0x02];

const char *UnitRosterColumnLabelTextTable[0x03];

const char *FriendlyEnemySuffixText;
const char *const TrainingCategoryHeadingText = ORIGINAL_TEXT(0x3390);
const char *TrainingCategoryLabelTextTable[0x02];

const char *const TrainingScenarioHeadingText = ORIGINAL_TEXT(0x339F);
char *TrainingScenarioLabelTextTable[0x06];

const char *const NoSelectionOptionsText = ORIGINAL_TEXT(0x33D0);
const char *const TrainingUnitSelectionPrompt = ORIGINAL_TEXT(0x33F1);

const char *PilotSurnameTextTable[0x3B];

const char *PilotGivenNameTextTable[0x3B];

const char *PilotExperienceLabelTextTable[0x04];

const char *PilotGunneryLabelTextTable[0x04];

const char *UnitMovementClassLabelTextTable[0x05];

const char *UnitArmorConditionLabelTextTable[0x06];

const char *const UnitStatusEquipmentSeparator = ORIGINAL_TEXT(0x45F8);
