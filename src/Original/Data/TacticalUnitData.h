// 3A6A: tactical unit data declarations
#ifndef REVENGE_DATA_TACTICAL_UNIT_DATA_H
#define REVENGE_DATA_TACTICAL_UNIT_DATA_H

#include "revenge/battletech.h"
#include "revenge/combat_data.h"
#include "revenge/original/scene_data.h"
#include "revenge/types.h"
#include "revenge/unit.h"

extern int8_t TacticalSlotSpriteState[0x18];
extern uint8_t UnitTypeResourceCode[0x07];
extern uint8_t UnitTacticalOrderMode[0x18];
extern int8_t UnitAttackTarget[0x19];
extern uint16_t UnitSpriteBaseId[0x18];
extern uint16_t UnitSpriteId[0x18];
extern uint16_t TacticalSpriteAnimationPhase;
extern uint8_t UnitTargetMapX[0x19];
extern uint8_t UnitTargetMapY[0x19];
extern uint8_t UnitRouteTargetMapX[0x19];
extern uint8_t UnitRouteTargetMapY[0x19];
/* DS:87C9, the routes' last record (SCENE_ROUTE_SAVED): a unit's route kept
 * while a Move or target is chosen, to give back on Escape and to hand to
 * the lance's followers. */
extern SceneUnitRoute &SavedTacticalRoute;
extern RevengeUnitRecord LiveScenarioUnits[0x18];
extern uint8_t ScenarioUnitMapX[0x18];
extern uint8_t ScenarioUnitMapY[0x18];
extern uint8_t ScenarioUnitFacing[0x18];
/* DS:5928: seven rows; the scenarios use rows 0-4 and 6. */
extern uint8_t ScenarioLanceAllegianceTable[0x07][0x03];  /* the rules' */
extern uint16_t AmmoCapacityMultiplierTable[0x0E];     /* the rules' */
/* DS:B011 and DS:B01D are the two scratch records immediately after the
 * 18 tactical-unit rating records. */
extern RevengeTacticalCombatRatings UnitCombatRatings[0x1A];

#define DisplayedUnitCombatRatings UnitCombatRatings[0x18]
#define ComparedUnitCombatRatings  UnitCombatRatings[0x19]

extern uint8_t UnitLanceFormationState[0x18];
extern uint8_t TacticalScenarioLanceEnabled[0x03];
extern uint8_t SensorDamageProximityRangeTable[0x03];  /* the rules' */
extern uint8_t LanceFormationProfile[0x06];
extern uint8_t UnitEnemyEngagementActive[0x18];
extern uint8_t UnitMovementHeatExponent[0x18];
extern uint8_t UnitOverburnOrCloseAssaultEnabled[0x18];
extern uint8_t UnitFormationMode[0x18];
extern uint16_t UnitConsecutiveAttackTicks[0x18];
extern uint16_t FirstLanceOrdersEnabled;
extern uint8_t UnitWasAttackedThisTick[0x18];
extern uint8_t GroundTargetWasAttacked;
extern uint8_t UnitJumpingState[0x18];
extern uint8_t UnitJumpMovementRemaining[0x18];
extern uint8_t UnitDestructionMarkers[0x18][0x03];
extern uint8_t UnitEnemyInProximityRange[0x18];
extern uint8_t UnitShutdownState[0x18];
extern uint8_t UnitEnemyVisible[0x18];
extern RevengeUnitSlot UnitTacticalTarget[0x19];
extern uint8_t UnitEnemyContactEstablished[0x18];
extern uint8_t UnitWithdrawalState[0x18];

/* DS:884B, DS:884E and DS:8853: the withdrawal states of the roster's units
 * 0Ch, 0Fh and 14h, which a later scenario's rosters test for withdrawal. */
#define CampaignUnitThirteenAvailabilityFlag (UnitWithdrawalState[0x0C])
#define CampaignUnitFifteenOverrideFlag      (UnitWithdrawalState[0x0F])
#define CampaignUnitFourteenAvailabilityFlag (UnitWithdrawalState[0x14])

extern RevengeUnitSlot UnitLastNarratedTarget[0x18];
extern uint8_t UnitPreferredEngagementMode[0x18];
extern uint8_t UnitTerrainType[0x18];
/* RecalculateUnitCombatRatings uses slot 18h as the status-panel scratch
 * record in addition to the 18h live tactical unit slots. */
extern uint8_t UnitBelowHalfDurability[TACTICAL_UNIT_COUNT + 0x01];
extern uint8_t UnitCachedWeaponCounts[(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
extern uint8_t UnitWeaponRecoveryCounters[(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
extern uint8_t UnitHeatSinkCount[TACTICAL_UNIT_COUNT + 0x01];
extern uint32_t UnitWeaponAvailabilityMask[TACTICAL_UNIT_COUNT + 0x01];
extern uint16_t WeaponCountScratch[0x17];
extern uint16_t FriendlyUnitDestroyed;
/* What an experience past those four reads: the original's words after the
 * table, through DS:3EF5 (src/Support/original_import_pieces.txt). */
extern uint16_t UnitActionMaskByExperiencePastEnd[0xFC];
/* What a heat-limit index past WeaponAIHeatLimitByExperience's five reads:
 * the original's bytes after the table, through DS:04E0
 * (src/Support/original_import_pieces.txt). */
extern uint8_t WeaponAIHeatLimitByExperiencePastEnd[0xFC];
extern uint8_t DamagedWeaponRangeFlags[0x03];
extern int16_t CurrentUnitDurabilityTotal;
extern int16_t MaximumUnitDurabilityTotal;
/* DS:176A: 30h words, one for each heat-sink count a unit's 2Fh equipment
 * slots allow. */
extern uint16_t HeatDissipationBySinkCountQ8_8[0x30];  /* the rules' */
extern uint16_t LastAttackingUnitIndex;
extern uint8_t ScenarioUnitNeedsUpdate[0x18];

#endif
