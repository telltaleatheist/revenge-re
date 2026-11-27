// REVENGE_3A6A_TACTICAL_DATA.cpp
// Immutable tactical tables recovered from the REVENGE.EXE data segment.

#include "revenge/original_data.h"

/* Filled at start from data/rules/combat.json (revenge/rules.h). */
word16 UnitActionMaskByExperience[0x04];

/* Filled at start from data/rules/weapons.json (revenge/rules.h). */
RevengeWeaponDefinition WeaponDefinitions[WEAPON_DEFINITION_COUNT];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
byte WeaponAIHeatLimitByExperience[0x05];
const byte SpriteFacingByDirection[0x08] = {
    0x00, 0x01, 0x01, 0x01, 0x02, 0x03, 0x03, 0x03
};
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
byte EngagementModeByComparisonMask[0x100];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
RevengeWeaponPriorityData WeaponPriorityData;
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
word16 WeaponPriorityCountTable[WEAPON_PRIORITY_PROFILE_COUNT];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
DosAddress WeaponPriorityListPointers[WEAPON_PRIORITY_PROFILE_COUNT];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
byte WeaponPriorityProfileByRangeHeat[0x80];
/* Filled at start from data/rules/combat.json (revenge/rules.h). */
word16 MovementTimingMaskTable[0x09];
const int16 DirectionDeltaXTable[0x08] = {
    0x0000, 0x0001, 0x0001, 0x0001, 0x0000, -0x0001, -0x0001, -0x0001
};
const int16 DirectionDeltaYTable[0x08] = {
    -0x0001, -0x0001, 0x0000, 0x0001, 0x0001, 0x0001, 0x0000, -0x0001
};
int16 NeighborMapOffsetTable[0x08] = {
    -0x0060, -0x005F, 0x0001, 0x0061, 0x0060, 0x005F, -0x0001, -0x0061
};
word16 DirectionSearchMirror;
int16 DirectionSearchCountdown = 0x000A;

const int16 DirectionSearchDeltaTable[0x10] = {
    0x0007, -0x0006, 0x0005, -0x0004, 0x0003, -0x0002, 0x0001, 0x0000,
    -0x0007, 0x0006, -0x0005, 0x0004, -0x0003, 0x0002, -0x0001, 0x0000
};
/* Filled at start from data/rules/weapons.json (revenge/rules.h). */
byte ImprovedMissileDamageBonusTable[0x07];
const byte TerrainDamageQueueOccupiedMask[0x04] = {
    0xC0, 0x30, 0x0C, 0x03
};
const byte TerrainDamageQueueIncrementMask[0x04] = {
    0x40, 0x10, 0x04, 0x01
};
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
int8 FormationOffsetTable[0xF0];
/* Filled at start from data/rules/ai.json (revenge/rules.h). */
byte FormationSelectionAggressive[0x0C];
byte FormationSelectionDefensive[0x0C];
byte FormationSelectionBalanced[0x0C];
/* Filled at start from data/ui/tactical_panel.json (revenge/rules.h). */
int16 FormationMarkerOriginX[0x0A];
int16 FormationMarkerOriginY[0x0A];
int16 FacingVectorStartX[0x10];
int16 FacingVectorStartY[0x10];
int16 FacingVectorEndX[0x10];
int16 FacingVectorEndY[0x10];
int16 MovementVectorX[0x20];
int16 MovementVectorY[0x20];
