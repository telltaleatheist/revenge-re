// 3A6A: tactical status data
// TacticalStatusData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/TacticalStatusData.h"
#include "revenge/story_text.h"

const char TacticalPaddingSpace[] = " ";
const char TacticalStatusSeparator[] = " ";
const char TacticalStatusPadding[] = " "; /* DS:30C2 */
const char *const UnitStatusArmorLabel = STORY_TEXT(UNIT_STATUS_ARMOR_LABEL);
const char *const UnitStatusHeatLabel = STORY_TEXT(UNIT_STATUS_HEAT_LABEL);
const char *const UnitStatusExitLabel = STORY_TEXT(UNIT_STATUS_EXIT_LABEL);
const char *const UnitStatusHeatDetailLabel = STORY_TEXT(UNIT_STATUS_HEAT_DETAIL_LABEL);
const char *const UnitStatusDamageLabel = STORY_TEXT(UNIT_STATUS_DAMAGE_LABEL);
const char *const UnitStatusRatingLabels =
    STORY_TEXT(UNIT_STATUS_RATING_LABELS);
const char *const IntelligenceHeading = STORY_TEXT(INTELLIGENCE_HEADING);
const char *const ScannedByActiveProbeText = STORY_TEXT(SCANNED_BY_ACTIVE_PROBE);
const char *const DetectedByText = STORY_TEXT(DETECTED_BY);
const char *const RadarSensorText = STORY_TEXT(RADAR_SENSOR);
const char *const EnemyAndFriendlyLancesText =
    STORY_TEXT(ENEMY_AND_FRIENDLY_LANCES);
const char *const UnknownUnitText = STORY_TEXT(UNKNOWN_UNIT);
const char *const NoSatelliteUplinkText = STORY_TEXT(NO_SATELLITE_UPLINK);
const char *const UplinkJammedText =
    STORY_TEXT(UPLINK_JAMMED);
const char *const OutOfScanningText = STORY_TEXT(OUT_OF_SCANNING);
const char *const RangeText = STORY_TEXT(RANGE_TEXT);
const char *const ArrowTagEquipmentText = STORY_TEXT(ARROW_TAG_EQUIPMENT);
/* DS:45F8, the string UnitStatusEquipmentSeparator names too. */
const char *const BapEquipmentText = STORY_TEXT(UNIT_STATUS_EQUIPMENT_SEPARATOR);
const char *const AccuracyEquipmentText = STORY_TEXT(ACCURACY_EQUIPMENT);
const char *const DoubleHeatSinkEquipmentText = STORY_TEXT(DOUBLE_HEAT_SINK_EQUIPMENT);
const char *const JumpEquipmentSuffix = STORY_TEXT(JUMP_EQUIPMENT_SUFFIX); /* DS:461D */
const char *const CaseEquipmentText = STORY_TEXT(CASE_EQUIPMENT);
const char *const MoveDestinationPromptSuffix = STORY_TEXT(MOVE_DESTINATION_PROMPT_SUFFIX);
const char *const FormationHeading = STORY_TEXT(FORMATION_HEADING);
const char *const SatelliteZoomText = STORY_TEXT(SATELLITE_ZOOM);
const char *const ContinueText = STORY_TEXT(CONTINUE_TEXT);
const char *const DoneText = STORY_TEXT(GAME_OPTION_LABEL_6);
const char NoTacticalTargetText[] = "?";
const char *const ViewSatelliteMapText = STORY_TEXT(VIEW_SATELLITE_MAP);
const char *const SelectStrafingAreaPrompt = STORY_TEXT(SELECT_STRAFING_AREA_PROMPT);
const char *const SelectArtilleryTargetPrompt = STORY_TEXT(SELECT_ARTILLERY_TARGET_PROMPT);
const char *const SelectBarrageStartPrompt =
    STORY_TEXT(SELECT_BARRAGE_START_PROMPT);
const char *const SelectBarrageEndPrompt = STORY_TEXT(SELECT_BARRAGE_END_PROMPT);
const char *const SelectFireTargetPrompt = STORY_TEXT(SELECT_FIRE_TARGET_PROMPT);
const char *TacticalUnitCommandTextTable[0x0E] = {
    STORY_TEXT(TACTICAL_UNIT_COMMAND_0), /* TacticalCommandsText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_1), /* TacticalLanceCommandsText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_2), /* TacticalTargetText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_3), /* TacticalCeaseFireAtText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_4), /* TacticalStopText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_5), /* TacticalWalkingSpeedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_6), /* TacticalRunningSpeedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_7), /* TacticalMoveText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_8), /* TacticalStoppedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_9), /* TacticalCruisingSpeedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_10), /* TacticalFlankSpeedText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_11), /* TacticalFireAtNoOneText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_12), /* TacticalFireDefensivelyText */
    STORY_TEXT(TACTICAL_UNIT_COMMAND_13) /* TacticalFireAtWillText */
};
const char *TacticalSupportCommandTextTable[0x08] = {
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_0), /* ShellCoordinateText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_1), /* CallOffShellingText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_2), /* RollingBarrageText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_3), /* CallOffBarrageText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_4), /* CallStrafingText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_5), /* CallOffStrafingText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_6), /* CloseAssaultText */
    STORY_TEXT(TACTICAL_SUPPORT_COMMAND_7) /* CeaseAssaultText */
};
const char *TacticalOverburnTextTable[0x02] = { STORY_TEXT(TACTICAL_OVERBURN_0) /* OverburnOffText */, STORY_TEXT(TACTICAL_OVERBURN_1) /* OverburnOnText */ };
const char *LanceStatusTextTable[0x08] = {
    STORY_TEXT(LANCE_STATUS_0), /* LanceCommandText */
    STORY_TEXT(LANCE_STATUS_1), /* LanceFireText */
    STORY_TEXT(LANCE_STATUS_2), /* LanceAssaultText */
    STORY_TEXT(LANCE_STATUS_3), /* LanceReconText */
    STORY_TEXT(LANCE_STATUS_4), /* LanceLightText */
    STORY_TEXT(LANCE_STATUS_5), /* LanceMediumText */
    STORY_TEXT(LANCE_STATUS_6), /* LanceHeavyText */
    STORY_TEXT(LANCE_STATUS_7) /* LanceAssaultWeightText */
};
/* DS:42D6 (2208:2181), the end of a longer string. */
const char *const DeadText = STORY_TEXT(DEAD_TEXT);
uint16_t PrimaryStatusRow;
uint16_t SecondaryStatusRow;
uint16_t TacticalUiSuppressed;
uint16_t PrimaryStatusHighlightColor;
uint16_t CachedPrimaryStatusHighlightColor;
uint16_t SecondaryStatusHighlightColor;
uint16_t CachedSecondaryStatusHighlightColor;
uint8_t TacticalStatusRowInvalidation[0x0D];
/* Filled at start from data/start/game.json (revenge/rules.h). */
uint16_t DisplayedTacticalStatusRow;
uint16_t TacticalStatusBlinkCountdown;
uint8_t TacticalStatusBlinkVisible;
const char TacticalNoUnitStatusText[] =
    "                           ";
const char *const FriendlyUnitStatusHeading = STORY_TEXT(FRIENDLY_UNIT_STATUS_HEADING); /* DS:4620 */
const char EnemyUnitStatusHeading[] = "   ";
const char UnitStatusSeparatorText[] = " ";
const char UnitHeatStatusPrefix[] = "/";
const char TacticalNarrationBlankText[] = "   ";
const char TacticalClockBlankText[] = "   ";
const char *const TacticalNarrationLabel = STORY_TEXT(TACTICAL_NARRATION_LABEL); /* DS:4624 */
const char *const CombatRatingLightText = STORY_TEXT(UNIT_CLASS_LABEL_0);
const char *const CombatRatingModerateText = STORY_TEXT(COMBAT_RATING_MODERATE);
const char *const CombatRatingHeavyText = STORY_TEXT(COMBAT_RATING_HEAVY);
const char *const CombatRatingBrutalText = STORY_TEXT(COMBAT_RATING_BRUTAL);
const char *const CombatRatingDevastatingText = STORY_TEXT(COMBAT_RATING_DEVASTATING);
const char *const CombatRatingIncredibleText = STORY_TEXT(COMBAT_RATING_INCREDIBLE);
const char *const CombatRatingNoneText = STORY_TEXT(COMBAT_RATING_NONE);
const char *CombatRatingDescriptionTable[0x07] = {
    STORY_TEXT(UNIT_CLASS_LABEL_0), /* CombatRatingLightText */
    STORY_TEXT(COMBAT_RATING_MODERATE), /* CombatRatingModerateText */
    STORY_TEXT(COMBAT_RATING_HEAVY), /* CombatRatingHeavyText */
    STORY_TEXT(COMBAT_RATING_BRUTAL), /* CombatRatingBrutalText */
    STORY_TEXT(COMBAT_RATING_DEVASTATING), /* CombatRatingDevastatingText */
    STORY_TEXT(COMBAT_RATING_INCREDIBLE), /* CombatRatingIncredibleText */
    STORY_TEXT(COMBAT_RATING_NONE) /* CombatRatingNoneText */
};
uint16_t TextOutputMode;
char SharedNumericTextBuffer[0x20];
const char PercentageSuffixText[] = "%";
