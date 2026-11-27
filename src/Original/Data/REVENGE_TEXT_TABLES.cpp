// REVENGE_TEXT_TABLES.c
// The text tables of REVENGE.EXE's data segment. The player's copy fills them
// at start (OriginalImport_Load, src/Support/original_import_pieces.txt); this file
// holds only their declarations and the DS offsets of the single strings.

#include "revenge/combat_data.h"
#include "revenge/original_import.h"
#include "revenge/original/campaign_data.h"
#include "revenge/original/lance_editor_data.h"
#include "revenge/original/scene_data.h"
#include "revenge/story_text.h"

const char *TitleTextTable[0x12];

const char *MonthNameTextTable[0x0C];

const char *MissionDateLabelTextTable[0x0C];

const char *const ExitToDosPrompt = STORY_TEXT(EXIT_TO_DOS_PROMPT);

const char *CampaignPathMenuTextTable[0x02];

const char *ScenarioVariantMenuTextTable[0x02];

const char *AlternateScenarioMenuTextTable[0x02];

const char *MainMenuTextTable[0x03];

const char *const PilotIdentityPrefixText = STORY_TEXT(PILOT_IDENTITY_PREFIX);
const char *const PilotNamePrefixText = STORY_TEXT(PILOT_NAME_PREFIX);
const char *const DateCommaSpaceText = STORY_TEXT(DATE_COMMA_SPACE);
const char *const DateSpaceText = STORY_TEXT(DATE_SPACE);
const char *const ScenarioCompleteHeadingText = STORY_TEXT(SCENARIO_COMPLETE_HEADING);
const char *const ScenarioFailedHeadingText = STORY_TEXT(SCENARIO_FAILED_HEADING);

const char *CampaignMessageTextTable[CAMPAIGN_MESSAGE_COUNT];

const char *const PilotRosterHeadingText = STORY_TEXT(PILOT_ROSTER_HEADING);

char ScenarioResourcePathText[0x0B];
const char *const SceneFileTooLargeText = STORY_TEXT(SCENE_FILE_TOO_LARGE);

const char *GameOptionLabelTextTable[0x07];

const char *GameOptionToggleTextTable[0x02];

const char *NarrationModeLabelTextTable[0x03];

const char *InputDeviceLabelTextTable[0x02];

const char *const FocusUnitText = STORY_TEXT(FOCUS_UNIT);
const char *const FocusLanceText = STORY_TEXT(FOCUS_LANCE);

char *DisabledUnitNarrationTextTable[0x03];

char *DestroyedUnitNarrationTextTable[0x03];

char *ScenarioEventNarrationTextTable[0x1E];

const char *const SceneNarrationSeparatorText = STORY_TEXT(SCENE_NARRATION_SEPARATOR);

const char *UnitAllegianceLabelTextTable[0x02];

const char *const UnknownUnitLabel = STORY_TEXT(UNKNOWN_UNIT_LABEL);
const char *const UnitStatusSpace = STORY_TEXT(UNIT_STATUS_SPACE);
const char *const UnitRosterSeparator = STORY_TEXT(UNIT_ROSTER_SEPARATOR);
const char *const UnitStatusSeparator = STORY_TEXT(UNIT_STATUS_SEPARATOR);

const char *UnitStatHeaderTextTable[0x06];

const byte *UnitClassTypeLists[0x07];

const char *UnitClassLabelTextTable[0x07];

const char *WeaponNameTable[WEAPON_DEFINITION_COUNT];

const char *TrainingDeploymentLabelTextTable[0x04];

const char *TrainingControlLabelTextTable[0x04];

const char *ScenarioVariantDescriptionTextTable[0x04];

const char *UnitEditorSectionLabelTextTable[0x04];

const char *ScenarioRouteLabelTextTable[0x04];

const char *UnitEditorSectionButtonLabelTextTable[0x04];

const char *TrainingDeploymentRosterLabelTextTable[0x02];

const char *UnitRosterColumnLabelTextTable[0x03];

const char *FriendlyEnemySuffixText;
const char *const TrainingCategoryHeadingText = STORY_TEXT(TRAINING_CATEGORY_HEADING);
const char *TrainingCategoryLabelTextTable[0x02];

const char *const TrainingScenarioHeadingText = STORY_TEXT(TRAINING_SCENARIO_HEADING);
char *TrainingScenarioLabelTextTable[0x06];

const char *const NoSelectionOptionsText = STORY_TEXT(NO_SELECTION_OPTIONS);
const char *const TrainingUnitSelectionPrompt = STORY_TEXT(TRAINING_UNIT_SELECTION_PROMPT);

const char *PilotSurnameTextTable[0x3B];

const char *PilotGivenNameTextTable[0x3B];

const char *PilotExperienceLabelTextTable[0x04];

const char *PilotGunneryLabelTextTable[0x04];

const char *UnitMovementClassLabelTextTable[0x05];

const char *UnitArmorConditionLabelTextTable[0x06];

const char *const UnitStatusEquipmentSeparator = STORY_TEXT(UNIT_STATUS_EQUIPMENT_SEPARATOR);
