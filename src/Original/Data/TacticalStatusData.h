// 3A6A: tactical status data declarations
#ifndef REVENGE_DATA_TACTICAL_STATUS_DATA_H
#define REVENGE_DATA_TACTICAL_STATUS_DATA_H

#include "revenge/types.h"

extern const char TacticalPaddingSpace[];
extern const char TacticalStatusSeparator[];
extern const char TacticalStatusPadding[];
extern const char *const UnitStatusArmorLabel;
extern const char *const UnitStatusHeatLabel;
extern const char *const UnitStatusExitLabel;
extern const char *const UnitStatusHeatDetailLabel;
extern const char *const UnitStatusDamageLabel;
extern const char *const UnitStatusRatingLabels;
extern const char *const IntelligenceHeading;
extern const char *const ScannedByActiveProbeText;
extern const char *const DetectedByText;
extern const char *const RadarSensorText;
extern const char *const EnemyAndFriendlyLancesText;
extern const char *const UnknownUnitText;
extern const char *const NoSatelliteUplinkText;
extern const char *const UplinkJammedText;
extern const char *const OutOfScanningText;
extern const char *const RangeText;
extern const char *const ArrowTagEquipmentText;
extern const char *const BapEquipmentText;
extern const char *const AccuracyEquipmentText;
extern const char *const DoubleHeatSinkEquipmentText;
extern const char *const JumpEquipmentSuffix;
extern const char *const CaseEquipmentText;
extern const char *const MoveDestinationPromptSuffix;
extern const char *const FormationHeading;
extern const char *const SatelliteZoomText;
extern const char *const ContinueText;
extern const char *const DoneText;
extern const char NoTacticalTargetText[];
extern const char *const ViewSatelliteMapText;
extern const char *const SelectStrafingAreaPrompt;
extern const char *const SelectArtilleryTargetPrompt;
extern const char *const SelectBarrageStartPrompt;
extern const char *const SelectBarrageEndPrompt;
extern const char *const SelectFireTargetPrompt;
extern const char *TacticalUnitCommandTextTable[0x0E];
extern const char *TacticalSupportCommandTextTable[0x08];
extern const char *TacticalOverburnTextTable[0x02];
extern const char *LanceStatusTextTable[0x08];
extern const char *const DeadText;
extern uint16_t PrimaryStatusRow;
extern uint16_t SecondaryStatusRow;
extern uint16_t TacticalUiSuppressed;
extern uint16_t PrimaryStatusHighlightColor;
extern uint16_t CachedPrimaryStatusHighlightColor;
extern uint16_t SecondaryStatusHighlightColor;
extern uint16_t CachedSecondaryStatusHighlightColor;
/* DS:B574: 0Dh bytes; the status slots the code and the scenes use are 0-8. */
extern uint8_t TacticalStatusRowInvalidation[0x0D];

#define TacticalContactMarkerPending \
    TacticalStatusRowInvalidation[0x07]

extern uint16_t DisplayedTacticalStatusRow;
extern uint16_t TacticalStatusBlinkCountdown;
extern uint8_t TacticalStatusBlinkVisible;
extern const char TacticalNoUnitStatusText[];
extern const char *const FriendlyUnitStatusHeading;
extern const char EnemyUnitStatusHeading[];
extern const char UnitStatusSeparatorText[];
extern const char UnitHeatStatusPrefix[];
extern const char TacticalNarrationBlankText[];
extern const char TacticalClockBlankText[];
extern const char *const TacticalNarrationLabel;
extern const char *const CombatRatingLightText;
extern const char *const CombatRatingModerateText;
extern const char *const CombatRatingHeavyText;
extern const char *const CombatRatingBrutalText;
extern const char *const CombatRatingDevastatingText;
extern const char *const CombatRatingIncredibleText;
extern const char *const CombatRatingNoneText;
extern const char *CombatRatingDescriptionTable[0x07];

extern uint16_t TextOutputMode;
extern char SharedNumericTextBuffer[0x20];
extern const char PercentageSuffixText[];

#endif
