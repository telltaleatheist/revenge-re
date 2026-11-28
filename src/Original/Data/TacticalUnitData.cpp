// 3A6A: tactical unit data
// TacticalUnitData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/TacticalUnitData.h"

int8_t TacticalSlotSpriteState[0x18];
uint8_t UnitTypeResourceCode[0x07];
uint8_t UnitTacticalOrderMode[0x18];
/* DS:88F3. 2208:060D and 0857 also index entry 18h, the map-cell selection's,
 * which is the free byte DS:890B before the next variable. */
int8_t UnitAttackTarget[0x19];
uint16_t UnitSpriteBaseId[0x18];
uint16_t UnitSpriteId[0x18];
uint16_t TacticalSpriteAnimationPhase;
uint8_t UnitTargetMapX[0x19];
uint8_t UnitTargetMapY[0x19];
uint8_t UnitRouteTargetMapX[0x19];
uint8_t UnitRouteTargetMapY[0x19];
SceneUnitRoute &SavedTacticalRoute = SceneUnitRoutes[SCENE_ROUTE_SAVED];
RevengeUnitRecord LiveScenarioUnits[0x18];
uint8_t ScenarioUnitMapX[0x18];
uint8_t ScenarioUnitMapY[0x18];
uint8_t ScenarioUnitFacing[0x18];
/* Filled at start from the story (src/Support/original_import_pieces.txt). */
uint16_t UnitActionMaskByExperiencePastEnd[0xFC];
/* Filled at start from the story (src/Support/original_import_pieces.txt). */
uint8_t WeaponAIHeatLimitByExperiencePastEnd[0xFC];
/* Filled at start from data/rules/units.json (revenge/rules.h). */
uint8_t ScenarioLanceAllegianceTable[0x07][0x03];
/* Filled at start from data/rules/weapons.json (revenge/rules.h). */
uint16_t AmmoCapacityMultiplierTable[0x0E];
RevengeTacticalCombatRatings UnitCombatRatings[0x1A];
uint8_t UnitLanceFormationState[0x18];
/* DS:03EA starts as the image has it, lances 1 and 2 on, until a battle's
 * scene header sets it (1A1F:1C48). */
uint8_t TacticalScenarioLanceEnabled[0x03] = { 0x01, 0x01, 0x00 };
/* Filled at start from data/rules/combat.json (revenge/rules.h). */
uint8_t SensorDamageProximityRangeTable[0x03];
uint8_t LanceFormationProfile[0x06];
uint8_t UnitEnemyEngagementActive[0x18];
uint8_t UnitMovementHeatExponent[0x18];
uint8_t UnitOverburnOrCloseAssaultEnabled[0x18];
uint8_t UnitFormationMode[0x18];
uint16_t UnitConsecutiveAttackTicks[0x18];
uint16_t FirstLanceOrdersEnabled;
uint8_t UnitWasAttackedThisTick[0x18];
uint8_t GroundTargetWasAttacked;
uint8_t UnitJumpingState[0x18];
uint8_t UnitJumpMovementRemaining[0x18];
uint8_t UnitDestructionMarkers[0x18][0x03];
uint8_t UnitEnemyInProximityRange[0x18];
uint8_t UnitShutdownState[0x18];
uint8_t UnitEnemyVisible[0x18];
/* DS:88A4. Entry 18h is the map-cell selection's (2208:0857), the free byte
 * DS:88BC. */
RevengeUnitSlot UnitTacticalTarget[0x19];
uint8_t UnitEnemyContactEstablished[0x18];
uint8_t UnitWithdrawalState[0x18];
RevengeUnitSlot UnitLastNarratedTarget[0x18];
uint8_t UnitPreferredEngagementMode[0x18];
uint8_t UnitTerrainType[0x18];
uint8_t UnitBelowHalfDurability[TACTICAL_UNIT_COUNT + 0x01];
uint8_t UnitCachedWeaponCounts[(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
uint8_t UnitWeaponRecoveryCounters[(TACTICAL_UNIT_COUNT + 0x01) * WEAPON_DEFINITION_COUNT];
uint8_t UnitHeatSinkCount[TACTICAL_UNIT_COUNT + 0x01];
uint32_t UnitWeaponAvailabilityMask[TACTICAL_UNIT_COUNT + 0x01];
uint16_t WeaponCountScratch[0x17];
uint16_t FriendlyUnitDestroyed;
uint8_t DamagedWeaponRangeFlags[0x03];
int16_t CurrentUnitDurabilityTotal;
int16_t MaximumUnitDurabilityTotal;
uint16_t HeatDissipationBySinkCountQ8_8[0x30];
uint16_t LastAttackingUnitIndex;
uint8_t ScenarioUnitNeedsUpdate[0x18];
