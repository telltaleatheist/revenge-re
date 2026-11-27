// 3A6A: tactical tables declarations
#ifndef REVENGE_DATA_TACTICAL_TABLES_H
#define REVENGE_DATA_TACTICAL_TABLES_H

#include "revenge/combat_data.h"
#include "revenge/types.h"

extern uint8_t WeaponAIHeatLimitByExperience[0x05];  /* the rules' */
extern const uint8_t SpriteFacingByDirection[0x08];
extern uint8_t EngagementModeByComparisonMask[0x100];  /* the rules' */
extern RevengeWeaponPriorityData WeaponPriorityData;  /* the rules' */
extern uint16_t WeaponPriorityCountTable[WEAPON_PRIORITY_PROFILE_COUNT];  /* the rules' */
extern DosAddress WeaponPriorityListPointers[WEAPON_PRIORITY_PROFILE_COUNT];  /* the rules' */
extern uint8_t WeaponPriorityProfileByRangeHeat[0x80];  /* the rules' */
extern uint16_t MovementTimingMaskTable[0x09];  /* the rules' */
/* Forty formations of three x and three y offsets: a facing band (0, 0Ah,
 * 14h, 1Eh) plus one of the ten formation modes. SaveDirectory follows. */
extern int8_t FormationOffsetTable[0xF0];  /* the rules' */
extern uint8_t FormationSelectionAggressive[0x0C];  /* the rules' */
extern uint8_t FormationSelectionDefensive[0x0C];  /* the rules' */
extern uint8_t FormationSelectionBalanced[0x0C];  /* the rules' */
extern int16_t FormationMarkerOriginX[0x0A];
extern int16_t FormationMarkerOriginY[0x0A];
extern int16_t FacingVectorStartX[0x10];
extern int16_t FacingVectorStartY[0x10];
extern int16_t FacingVectorEndX[0x10];
extern int16_t FacingVectorEndY[0x10];
extern int16_t MovementVectorX[0x20];
extern int16_t MovementVectorY[0x20];
extern const int16_t DirectionDeltaXTable[0x08];
extern const int16_t DirectionDeltaYTable[0x08];
extern int16_t NeighborMapOffsetTable[0x08];
extern uint16_t DirectionSearchMirror;
extern int16_t DirectionSearchCountdown;
extern const int16_t DirectionSearchDeltaTable[0x10];
/* DS:3D1D: the Improved Missiles damage bonus of each missile weapon, from
 * WEAPON_LRM_5 on. Its first byte is HitLocationDirectionSelectorTable's
 * last. */
extern uint8_t ImprovedMissileDamageBonusTable[0x07];  /* the rules' */
extern const uint8_t TerrainDamageQueueOccupiedMask[0x04];
extern const uint8_t TerrainDamageQueueIncrementMask[0x04];
extern uint16_t UnitActionMaskByExperience[0x04];  /* the rules' */

#endif
