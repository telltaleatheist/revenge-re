// 3A6A: tactical tables
// TacticalData.cpp
// Immutable tactical tables recovered from the REVENGE.EXE data segment.

#include "Data/TacticalTables.h"

/* Filled at start from data/rules/combat.json (revenge/rules.h). */
uint16_t UnitActionMaskByExperience[0x04];

/* Filled at start from data/rules/weapons.json (revenge/rules.h). */
RevengeWeaponDefinition WeaponDefinitions[WEAPON_DEFINITION_COUNT];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
uint8_t WeaponAIHeatLimitByExperience[0x05];
const uint8_t SpriteFacingByDirection[0x08] = {
    0x00, 0x01, 0x01, 0x01, 0x02, 0x03, 0x03, 0x03
};
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
uint8_t EngagementModeByComparisonMask[0x100];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
RevengeWeaponPriorityData WeaponPriorityData;
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
uint16_t WeaponPriorityCountTable[WEAPON_PRIORITY_PROFILE_COUNT];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
DosAddress WeaponPriorityListPointers[WEAPON_PRIORITY_PROFILE_COUNT];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
uint8_t WeaponPriorityProfileByRangeHeat[0x80];
/* Filled at start from data/rules/combat.json (revenge/rules.h). */
uint16_t MovementTimingMaskTable[0x09];
const int16_t DirectionDeltaXTable[0x08] = {
    0x0000, 0x0001, 0x0001, 0x0001, 0x0000, -0x0001, -0x0001, -0x0001
};
const int16_t DirectionDeltaYTable[0x08] = {
    -0x0001, -0x0001, 0x0000, 0x0001, 0x0001, 0x0001, 0x0000, -0x0001
};
int16_t NeighborMapOffsetTable[0x08] = {
    -0x0060, -0x005F, 0x0001, 0x0061, 0x0060, 0x005F, -0x0001, -0x0061
};
uint16_t DirectionSearchMirror;
int16_t DirectionSearchCountdown = 0x000A;

const int16_t DirectionSearchDeltaTable[0x10] = {
    0x0007, -0x0006, 0x0005, -0x0004, 0x0003, -0x0002, 0x0001, 0x0000,
    -0x0007, 0x0006, -0x0005, 0x0004, -0x0003, 0x0002, -0x0001, 0x0000
};
/* Filled at start from data/rules/weapons.json (revenge/rules.h). */
uint8_t ImprovedMissileDamageBonusTable[0x07];
const uint8_t TerrainDamageQueueOccupiedMask[0x04] = {
    0xC0, 0x30, 0x0C, 0x03
};
const uint8_t TerrainDamageQueueIncrementMask[0x04] = {
    0x40, 0x10, 0x04, 0x01
};
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
int8_t FormationOffsetTable[0xF0];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
uint8_t FormationSelectionAggressive[0x0C];
uint8_t FormationSelectionDefensive[0x0C];
uint8_t FormationSelectionBalanced[0x0C];
/* Filled at start from data/ui/tactical_panel.json (revenge/rules.h). */
int16_t FormationMarkerOriginX[0x0A];
int16_t FormationMarkerOriginY[0x0A];
int16_t FacingVectorStartX[0x10];
int16_t FacingVectorStartY[0x10];
int16_t FacingVectorEndX[0x10];
int16_t FacingVectorEndY[0x10];
int16_t MovementVectorX[0x20];
int16_t MovementVectorY[0x20];
