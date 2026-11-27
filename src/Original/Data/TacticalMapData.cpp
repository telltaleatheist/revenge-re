// 3A6A: tactical map data
// TacticalMapData.cpp
// Typed objects recovered from REVENGE.EXE DS segment 3A6A.

#include "Data/TacticalMapData.h"

uint8_t *DamagedTerrainSpriteLookup;
uint8_t TacticalEffectOverlayMap[0x00E4];
RevengeMapEdgeEffectRecord MapEdgeEffectQueue[MAP_EDGE_EFFECT_RECORD_COUNT];
uint16_t MapEdgeEffectCount;
const int8_t WeaponTraceMajorStepX[0x08] = { 0x00, 0x01, 0x01, 0x00, 0x00, -0x01, -0x01, 0x00 };
const int8_t WeaponTraceMajorStepY[0x08] = { -0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, -0x01 };
const int8_t WeaponTraceMinorStepX[0x08] = { 0x01, 0x00, 0x00, 0x01, -0x01, 0x00, 0x00, -0x01 };
const int8_t WeaponTraceMinorStepY[0x08] = { 0x00, -0x01, 0x01, 0x00, 0x00, 0x01, -0x01, 0x00 };
uint8_t *TerrainTypeMap;
uint8_t *TacticalOverviewMapBuffer;
uint8_t *PackedTerrainDamageMap;
uint8_t *ActiveTerrainDamageByte;
uint8_t *TerrainDamageCapabilityTable;
const uint8_t TerrainDamageCellIncrementMask[0x04] = { 0x01, 0x04, 0x10, 0x40 };
const uint8_t TerrainDamageExtractMask[0x04] = { 0x03, 0x0C, 0x30, 0xC0 };
const uint8_t TerrainDamageShift[0x04] = { 0x00, 0x02, 0x04, 0x06 };
const uint8_t TerrainDamageCellOccupiedMask[0x04] = { 0xC0, 0x30, 0x0C, 0x03 };
const uint8_t TerrainDamageStageIncrement[0x04] = { 0x40, 0x10, 0x04, 0x01 };
uint16_t PackedTerrainDamageRowBytes;
uint8_t DestructibleTerrainTypeBase;
uint8_t TerrainImpactAnimationTypeMinimum;
uint8_t TerrainImpactAnimationTypeMaximum;
uint16_t TerrainLineOfSightBlockingThreshold;
uint8_t TerrainLineOfSightMaximumSteps;
uint8_t TacticalTerrainEffectMode;
RevengeTerrainDamageQueueEntry TerrainDamageQueue[TERRAIN_DAMAGE_QUEUE_ENTRY_COUNT];
uint16_t ImpassableTerrainTypeMinimum;
int8_t TerrainToHitModifier[0x100];
RevengeTerrainImpactAnimationRecord
    TerrainImpactAnimationRecords[TERRAIN_IMPACT_ANIMATION_RECORD_COUNT];
uint16_t TacticalImpactQueueCount;
RevengeTacticalImpactRecord
    TacticalImpactQueue[TACTICAL_IMPACT_RECORD_COUNT];
uint16_t TerrainDamageQueueWordCount;
uint16_t TacticalProjectileCount;
RevengeTacticalProjectileRecord
    TacticalProjectileRecords[TACTICAL_PROJECTILE_RECORD_COUNT];
uint8_t ArtilleryShellingActive;
uint8_t RollingBarrageActive;
uint8_t StrafingRunActive;
uint8_t ArtilleryTargetMapX;
uint8_t ArtilleryTargetMapY;
uint8_t RollingBarrageEndMapX;
uint8_t RollingBarrageEndMapY;
uint8_t RollingBarrageStartMapX;
uint8_t RollingBarrageStartMapY;
uint8_t DropShipMapX;
uint8_t DropShipMapY;
uint8_t ArtillerySupportAvailable;
uint8_t StrafingSupportAvailable;
static uint16_t TacticalCellTerrainStorage[0xE4];
static uint16_t TacticalCellSpriteStorage[0xE4];
uint16_t *TacticalCellTerrainMap = TacticalCellTerrainStorage;
uint16_t *TacticalCellSpriteMap = TacticalCellSpriteStorage;
