// 2D1B: movement, weapon, damage and terrain declarations
#ifndef REVENGE_TACTICAL_COMBAT_INTERNAL_H
#define REVENGE_TACTICAL_COMBAT_INTERNAL_H

// REVENGE_2D1B_TACTICAL_COMBAT.c
// Tactical movement, targeting, weapon, damage, and terrain procedures
// reconstructed from REVENGE.EXE segment 2D1B.

#include "Data/TacticalUnitData.h"

inline constexpr uint8_t TACTICAL_ENGAGEMENT_NONE = 0x00;
inline constexpr uint8_t TACTICAL_ENGAGEMENT_CLOSE = 0x01;
inline constexpr uint8_t TACTICAL_ENGAGEMENT_MEDIUM = 0x02;
inline constexpr uint8_t TACTICAL_ENGAGEMENT_LONG = 0x03;
inline constexpr uint8_t TACTICAL_ENGAGEMENT_WITHDRAW = 0x04;
inline constexpr uint8_t TACTICAL_ENGAGEMENT_POSITION = 0x05;


/* UnitActionMaskByExperience's entries (DS:3CF6). */
inline constexpr int ACTION_MASK_EXPERIENCE_COUNT = 0x04;

#define UnitRouteFirstX UnitRouteTargetMapX
#define UnitRouteFirstY UnitRouteTargetMapY

#define TACTICAL_COMBAT_RATINGS(unitIndex) \
    (&UnitCombatRatings[(unitIndex)])

void SelectUnitTacticalTargets(void);
void AdvanceUnitTacticalMovement(uint16_t unitIndex);
uint16_t ExecuteUnitWeaponAI(uint16_t unitIndex);
/* Ours (upgrade_lance_weapons): the weapon a priority entry fires for the
 * player's side (Weapons.cpp). */
uint16_t LanceWeaponsUpgradeChoice(uint32_t readyMask, uint16_t firedWeaponId);
void MoveUnitTowardMapCell(
    uint16_t unitIndex, uint16_t destinationX, uint16_t destinationY);
void UpdateUnitSpriteForFacing(uint16_t unitIndex, uint16_t facing);
void AdvanceUnitRouteWaypoint(uint16_t unitIndex);
void QueueLanceOrderNarration(uint16_t unitIndex);
void SelectLanceFormationMode(
    uint16_t leaderUnitIndex, uint16_t randomizeFormation);
void BeginTerrainDamageAtCell(
    uint16_t mapX, uint16_t mapY, uint16_t terrainType);
void CollapseStructureAtMapCell(
    int16_t mapX, int16_t mapY, uint16_t terrainType);
void ApplyTerrainImpact(
    int16_t mapX, int16_t mapY,
    uint16_t advanceBeforeRecording, uint16_t forceAnimationRecord);
uint16_t GeneratePseudoRandomByte(void);
void BuildArmWeaponAvailabilityMasks(uint16_t unitIndex);
int16_t ResolveWeaponAttack(
    int16_t attackerUnitIndex, int16_t weaponIndex, int16_t targetUnitIndex);
void QueueTacticalProjectile(
    uint16_t attackerUnitIndex,
    uint8_t weaponIndex,
    uint16_t targetUnitIndex,
    uint16_t shotHit);
void DrawWeaponAttackTrace(
    uint16_t attackerUnitIndex,
    int16_t targetMapX,
    int16_t targetMapY,
    uint16_t offsetEndpoints,
    uint16_t shotHit,
    uint16_t weaponIndex);
void CountUnitWeaponsByType(const RevengeUnitRecord *unitRecord);
uint16_t CountOrRemoveUnitHeatSinks(
    RevengeUnitRecord *unitRecord, uint16_t removeInstalledHeatSinks);
uint32_t CalculateSelectedWeaponRating(
    uint32_t weaponMask, const uint16_t *weaponCounts);
uint16_t DetermineRelativeAttackDirection(
    int16_t sourceX, int16_t sourceY, int16_t targetX, int16_t targetY);
uint16_t CalculateApproximateMapDistance(
    int16_t sourceX, int16_t sourceY, int16_t targetX, int16_t targetY);
uint16_t TraceMapLineOfSight(
    int16_t sourceX, int16_t sourceY, int16_t targetX, int16_t targetY,
    const uint8_t *terrainTypeMap,
    uint8_t specialTerrainMinimum, uint8_t specialTerrainMaximum,
    uint8_t blockingTerrainThreshold, uint8_t maximumSteps);
uint16_t Roll2D6(void);
uint16_t ReadTerrainDamageState(int16_t mapX, int16_t mapY);
void DispatchWeaponEffect(int16_t weaponEffectId);
void QueueTacticalImpactVisual(
    int16_t mapX, int16_t mapY, uint16_t impactClass);
void ApplyWeaponDamage(
    int16_t attackerUnitIndex, int16_t weaponIndex, int16_t targetUnitIndex);
void DestroyScenarioUnit(uint16_t unitIndex);
uint16_t FindOperationalLanceLeader(uint16_t unitIndex);
uint16_t ShouldDestroyUnitAfterLegLoss(void);
void RebuildLanceFormationAfterLeaderLoss(uint16_t firstUnitIndex);
uint16_t AdvanceTerrainDamageState(
    int16_t mapX, int16_t mapY, uint16_t maximumDamageState);

#endif
